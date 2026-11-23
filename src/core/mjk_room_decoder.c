/**
 * @file mjk_room_decoder.c
 * @brief Implementación de decodificación de salas, 33 puertas USA y descompresor 4-tier.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: room_decoder_contract.md (asm/src/prg.asm:6741-6865 y roomdata-us.asm)
 */

#include "mjk_room_decoder.h"
#include "mjk_player.h"
#include "mjk_apu.h"
#include <string.h>

void MjkRoom_DecodeGeometry(uint8_t room_id, uint8_t out_metatiles[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W], bool* out_is_horizontal) {
    if (!out_metatiles) return;
    if (room_id >= MJK_TOTAL_ROOMS_COUNT) room_id = 0;

    MjkAdjacentRoom adj = g_adjacent_rooms[room_id];
    bool is_horizontal = (adj.flags & 0x04) != 0;
    if (out_is_horizontal) *out_is_horizontal = is_horizontal;

    MjkRoomHalfScreen half = g_room_half_screens[room_id];

    if (is_horizontal) {
        // Horizontal Room (+32 autoincrement PPU): half_0 = top 8 rows, half_1 = bottom 7 rows
        for (int c = 0; c < MJK_ROOM_TILES_W; ++c) {
            // Mitad superior (filas 0..7)
            uint8_t chunk_top = g_half_screen_layouts[half.half_0][c];
            if (chunk_top >= 0x60) chunk_top -= 0x0B;
            for (int r = 0; r < 8; ++r) {
                out_metatiles[r][c] = g_layout_chunks[chunk_top][r];
            }
            // Mitad inferior (filas 8..14)
            uint8_t chunk_bot = g_half_screen_layouts[half.half_1][c];
            if (chunk_bot >= 0x60) chunk_bot -= 0x0B;
            for (int r = 8; r < 15; ++r) {
                out_metatiles[r][c] = g_layout_chunks[chunk_bot][r - 8];
            }
        }
    } else {
        // Vertical Room (+1 autoincrement PPU): half_0 = left 8 cols, half_1 = right 8 cols
        for (int r = 0; r < MJK_ROOM_TILES_H; ++r) {
            // Mitad izquierda (columnas 0..7)
            uint8_t chunk_left = g_half_screen_layouts[half.half_0][r];
            if (chunk_left >= 0x60) chunk_left -= 0x0B;
            for (int c = 0; c < 8; ++c) {
                out_metatiles[r][c] = g_layout_chunks[chunk_left][c];
            }
            // Mitad derecha (columnas 8..15)
            uint8_t chunk_right = g_half_screen_layouts[half.half_1][r];
            if (chunk_right >= 0x60) chunk_right -= 0x0B;
            for (int c = 8; c < 16; ++c) {
                out_metatiles[r][c] = g_layout_chunks[chunk_right][c - 8];
            }
        }
    }
}

void MjkRoom_WriteNametableString(uint8_t out_nametable[30][32], uint8_t out_subpalettes[30][32],
                                  uint16_t ppu_addr_rel_2000, const uint8_t* tile_ids,
                                  size_t count, uint8_t subpalette) {
    if (!out_nametable || !out_subpalettes || !tile_ids) return;
    int row = (int)(ppu_addr_rel_2000 / 32);
    int col = (int)(ppu_addr_rel_2000 % 32);

    for (size_t i = 0; i < count; ++i) {
        int c = col + (int)i;
        int r = row;
        if (c >= 32) {
            r += c / 32;
            c %= 32;
        }
        if (r < 30) {
            out_nametable[r][c] = tile_ids[i];
            out_subpalettes[r][c] = subpalette;
        }
    }
}

void MjkRoom_BuildNametable(uint8_t room_id, uint8_t out_nametable[30][32], uint8_t out_subpalettes[30][32]) {
    if (!out_nametable || !out_subpalettes) return;

    uint8_t metatiles[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W];
    bool is_horizontal = false;
    MjkRoom_DecodeGeometry(room_id, metatiles, &is_horizontal);

    for (int mr = 0; mr < MJK_ROOM_TILES_H; ++mr) {
        for (int mc = 0; mc < MJK_ROOM_TILES_W; ++mc) {
            uint8_t m_id = metatiles[mr][mc];
            MjkMetatileDef def = (m_id < MJK_METATILES_COUNT) ? g_metatile_definitions[m_id] : (MjkMetatileDef){0x24, 0x24, 0x24, 0x24};
            uint8_t sub = MjkRoomData_GetMetatileSubpalette(m_id);

            int ty = mr * 2;
            int tx = mc * 2;

            out_nametable[ty][tx]        = def.tl;
            out_nametable[ty][tx + 1]    = def.tr;
            out_nametable[ty + 1][tx]    = def.bl;
            out_nametable[ty + 1][tx + 1]= def.br;

            out_subpalettes[ty][tx]        = sub;
            out_subpalettes[ty][tx + 1]    = sub;
            out_subpalettes[ty + 1][tx]    = sub;
            out_subpalettes[ty + 1][tx + 1]= sub;
        }
    }

    // Superposición de cadenas auténticas de la Pantalla de Título (Sala 0)
    if (room_id == 0) {
        // "PUSH START BUTTON": PPU $01C8 (row 14, col 8)
        // Subpaleta 0 para texto blanco puro ($30) con borde negro ($0F)
        static const uint8_t s_push[] = {
            0x19, 0x1E, 0x1C, 0x11, 0x24, 0x1C, 0x1D, 0x0A, 0x1B, 0x1D, 0x24, 0x0B, 0x1E, 0x1D, 0x1D, 0x18, 0x17
        };
        MjkRoom_WriteNametableString(out_nametable, out_subpalettes, 0x01C8, s_push, sizeof(s_push), 0);

        // "(C) TECMO,LTD.": PPU $0213 (row 16, col 19)
        static const uint8_t s_tecmo[] = {
            0x2C, 0x24, 0x1D, 0x0E, 0x0C, 0x16, 0x18, 0x2D, 0x15, 0x1D, 0x0D, 0x61
        };
        MjkRoom_WriteNametableString(out_nametable, out_subpalettes, 0x0213, s_tecmo, sizeof(s_tecmo), 1);

        // "HI  GDV": PPU $0056 (row 2, col 22)
        static const uint8_t s_higdv[] = {
            0x11, 0x12, 0x24, 0x10, 0x0D, 0x1F
        };
        MjkRoom_WriteNametableString(out_nametable, out_subpalettes, 0x0056, s_higdv, sizeof(s_higdv), 1);

        // "™": PPU $019E (row 12, col 30)
        static const uint8_t s_tm[] = { 0xDC, 0xDD };
        MjkRoom_WriteNametableString(out_nametable, out_subpalettes, 0x019E, s_tm, sizeof(s_tm), 3);
    }
}

void MjkRoom_LoadAuthenticPalettes(uint8_t room_id, uint8_t out_pal_32[32]) {
    if (!out_pal_32) return;

    if (room_id == 0) {
        memcpy(out_pal_32, MainPalette, 32);
        return;
    }

    uint8_t pal_set_id = 0;
    const uint8_t* rdata = g_room_data_table[room_id];
    if (rdata && (rdata[0] & 0xF0) == 0xE0) {
        pal_set_id = rdata[0] & 0x0F;
    }
    if (pal_set_id >= MJK_PALETTE_SETS_COUNT) {
        pal_set_id = 0;
    }

    // Copiar MainPalette por defecto
    memcpy(out_pal_32, MainPalette, 32);

    // Asignar los 12 bytes del set a las subpaletas de fondo 1, 2 y 3 (slots 4..15)
    // PRESERVANDO out_pal_32[1..3] para el HUD
    const uint8_t* bg_set = BackgroundPaletteSets[pal_set_id];
    memcpy(&out_pal_32[4], bg_set, 12);

    // Unificar el Backdrop universal (byte 0 del set) en todos los slots múltiplos de 4
    uint8_t backdrop = bg_set[0];
    for (int i = 0; i < 32; i += 4) {
        out_pal_32[i] = backdrop;
    }
}

void MjkRoom_Load(MjkRoomContext* room, uint8_t room_id) {
    if (!room) return;
    memset(room, 0, sizeof(MjkRoomContext));
    room->room_id = room_id;

    // 1. Crear el perímetro de muros exteriores indestructibles
    for (int x = 0; x < MJK_ROOM_TILES_W; ++x) {
        room->metatiles[0 * MJK_ROOM_TILES_W + x]  = METATILE_BRICK_SOLID; // Techo
        room->metatiles[14 * MJK_ROOM_TILES_W + x] = METATILE_BRICK_SOLID; // Suelo
    }
    for (int y = 0; y < MJK_ROOM_TILES_H; ++y) {
        room->metatiles[y * MJK_ROOM_TILES_W + 0]  = METATILE_BRICK_SOLID; // Pared izquierda
        room->metatiles[y * MJK_ROOM_TILES_W + 15] = METATILE_BRICK_SOLID; // Pared derecha
    }

    // 2. Generar plataformas intermedias características de la pirámide
    for (int x = 3; x <= 12; ++x) {
        room->metatiles[4 * MJK_ROOM_TILES_W + x]  = METATILE_BRICK_SOLID; // Plataforma superior
        room->metatiles[8 * MJK_ROOM_TILES_W + x]  = METATILE_BRICK_SOLID; // Plataforma media
        room->metatiles[11 * MJK_ROOM_TILES_W + x] = METATILE_BRICK_SOLID; // Plataforma baja
    }

    // 3. Colocar puerta de salida cerrada en (x=14, y=12)
    room->door_x = 14;
    room->door_y = 12;
    room->door_condition_id = (room_id % MJK_DOOR_CONDITIONS_COUNT);
    room->door_is_open = false;
    room->metatiles[12 * MJK_ROOM_TILES_W + 14] = METATILE_DOOR_CLOSED;

    // 4. Distribuir bombas en plataformas
    room->active_fire_bomb_index = 0;
    room->chain_active = false;
    room->fire_bombs_collected = 0;
    room->total_bombs_collected = 0;
}

uint8_t MjkRoom_ExtractDoorCondition(uint8_t room_id, bool* out_has_condition) {
    if (out_has_condition) *out_has_condition = false;
    if (room_id >= 256) return 0;

    // Sala especial Room $61 (Warp Secreto Round 11 -> Round 6 / loc_A977: SBC #1 -> $1F)
    if (room_id == 0x61) {
        if (out_has_condition) *out_has_condition = true;
        return 0x1F;
    }

    const uint8_t* rdata = g_room_data_table[room_id];
    if (!rdata) return 0;

    // Verificar cabecera $Ex (Byte 0: %1110 PPPP)
    if ((rdata[0] & 0xF0) != 0xE0) return 0;

    // Byte 1: %UUUU UMMM (bits 7-3 = UUUUU, bits 2-0 = MMM RoomMusic)
    uint8_t u = (uint8_t)(rdata[1] >> 3);
    if (u == 0) return 0; // UUUUU == 0: no tiene condición periódica

    if (out_has_condition) *out_has_condition = true;
    return (uint8_t)(u - 1); // Condición normalizada 0..31
}

uint8_t MjkRoom_GetAdjacentRoom(uint8_t current_room, uint8_t entry_door_type) {
    if (current_room >= MJK_TOTAL_ROOMS_COUNT) return 0;
    uint8_t dir = (uint8_t)((entry_door_type >> 2) & 0x03);
    const MjkAdjacentRoom* adj = &g_adjacent_rooms[current_room];
    switch (dir) {
        case 0: return adj->up;
        case 1: return adj->down;
        case 2: return adj->right;
        case 3: return adj->left;
        default: return 0;
    }
}

void MjkRoom_HandleOpenDoorMaybe(MjkGameState* game) {
    if (!game) return;
    if (game->door_is_open) return;

    game->door_is_open = true;
    game->door_is_animating = true;
    game->door_anim_timer = 32;
    game->door_anim_phase = 0;
    game->door_anim_dir = 0x81; // Bit 7 = 1 -> Apertura
    MjkAPU_PlaySFX(&game->apu, SFX_DOOR_OPEN);

    // Efectos colaterales según condición canónica (RES-017 / RES-022)
    if (game->door_condition_id == 0x09) {
        game->door_check_flag_09 = 0x80;
    } else if (game->door_condition_id == 0x0C && (game->maybe_bomb_thing & MJK_BOMB_THING_PERFECT_BONUS)) {
        game->maybe_bomb_thing &= ~MJK_BOMB_THING_PERFECT_BONUS; // Consume bit 7 canónico (prg.asm:6875)
    } else if (game->door_condition_id == 0x1F) {
        game->door_check_flag_1f = 1;
    } else if (game->door_condition_id == 0x1E) {
        game->door_check_flag_1e_deaths = 0;
    }

    // Actualizar metatile en la matriz activa de sala
    int dy = game->door_tile_y;
    int dx = game->door_tile_x;
    if (dy < MJK_ROOM_TILES_H && dx < MJK_ROOM_TILES_W) {
        game->room_metatiles[dy * MJK_ROOM_TILES_W + dx] = METATILE_DOOR_OPEN;
    }

    // Transición canónica a GameState_B_AnimateDoorOpen ($0B)
    game->current_state = GAME_STATE_B_ANIMATE_DOOR_OPEN;
    game->sub_state = 0;
    game->state_timer = 0;
}

void MjkRoom_OpenBombRoomDoor(MjkGameState* game) {
    if (!game) return;
    MjkRoom_HandleOpenDoorMaybe(game);
}

bool MjkRoom_CheckDoorCondition(uint8_t condition_id, const JackPlayer* jack, const MjkGameState* game) {
    if (!jack || !game) return false;

    /*
     * Tabla de evaluación de las 33 condiciones de puertas USA (REV_US)
     * Documentadas en CheckAndMaybeOpenDoor (prg.asm:6745-6780) y RES-017.
     */
    switch (condition_id) {
        case 0x00: // DoorCond_rts: Inactiva / cerrada permanente
        case 0x01: // DoorCond_rts
            return false;

        case 0x02: // DoorCond_02_JumpedOn: Jack aterriza o salta sobre la puerta
            return (jack->vel_y > 0 || jack->subpixel_vy > 0);

        case 0x03: // DoorCond_rts
        case 0x04: // DoorCond_rts
        case 0x05: // DoorCond_rts
        case 0x06: // DoorCond_rts
            return false;

        case DOOR_COND_MIGHTY3: // 0x07: DoorCond_XX_Mighty3: Poder máximo Mighty Level 3
            return can_open_mighty3_door(jack);

        case 0x08: // DoorCond_rts
            return false;

        case 0x09: // DoorCond_09_FBWarp2: Warp Inverso Round 4 -> Round 7 (Crystal Door)
            return ((game->door_check_flag_09 & 0x80) == 0 && (game->door_check_flag_09 & 0x01) != 0);

        case 0x0A: // En JP: StageTimer == 0x30. En US (REV_US): Desactivada (DoorCond_rts)
            return false;

        case 0x0B: // DoorCond_rts
            return false;

        case 0x0C: // DoorCond_0C_AllFireBombs: Bonus perfecto (bit 7 de maybe_bomb_thing) o 20+ bombas de fuego
            if ((game->maybe_bomb_thing & MJK_BOMB_THING_PERFECT_BONUS) != 0) {
                return true;
            }
            return (game->fire_bombs_collected >= 20 || (game->bomb_count > 0 && game->fire_bombs_collected >= game->bomb_count));

        case 0x0D: // DoorCond_rts
            return false;

        case 0x0E: // En JP: StageTimer == 0x60. En US (REV_US): Redirigida a DoorCond_XX_Mighty3
            return can_open_mighty3_door(jack);

        case 0x0F: // DoorCond_rts
        case 0x10: // DoorCond_rts
        case 0x11: // DoorCond_rts
        case 0x12: // DoorCond_rts
        case 0x13: // DoorCond_rts
        case 0x14: // DoorCond_rts
        case 0x15: // DoorCond_rts
        case 0x16: // DoorCond_rts
            return false;

        case 0x17: // DoorCond_XX_Mighty3
        case 0x18: // DoorCond_XX_Mighty3
            return can_open_mighty3_door(jack);

        case 0x19: // En JP: TensScore30. En US: Redirigida a DoorCond_1D_FBWarp1
        case 0x1D: // DoorCond_1D_FBWarp1: Secret FB Warp (4 condiciones conjuntas)
            return (game->mighty_coins == 0 &&
                    game->stage_timer_seconds == 0 &&
                    (jack->sprite_id == 6 || jack->state == JACK_STATE_FALLING));

        case 0x1A: // En JP: TensScore70. En US: Desactivada (RTS)
            return false;

        case 0x1B: // DoorCond_XX_Mighty3
            return can_open_mighty3_door(jack);

        case 0x1C: // DoorCond_rts
            return false;

        case 0x1E: // DoorCond_1E_R5R11Warp: Round 5 -> Round 11 (2 muertes contra fuego)
            return (game->door_check_flag_1e_deaths >= 2 && jack->is_alive);

        case 0x1F: // DoorCond_1F_R11WarpBack: Room $61 (R11 -> R6)
            return (game->door_check_flag_1f == 0);

        case 0x20: // DoorCond_20_US: Anomalía/remanente US (inalcanzable en flujo estándar)
            return (jack->power_level >= MIGHTY_POWER_BLUE);

        default:
            return false;
    }
}

uint16_t MjkRoom_CollectBomb(MjkRoomContext* room, uint8_t bomb_index) {
    if (!room || bomb_index >= MJK_BOMBS_PER_ROOM) return 0;

    room->total_bombs_collected++;

    // Si la cadena ya está activa y Jack recoge la bomba con mecha encendida
    if (room->chain_active && bomb_index == room->active_fire_bomb_index) {
        room->fire_bombs_collected++;
        room->active_fire_bomb_index = (uint8_t)((bomb_index + 1) % MJK_BOMBS_PER_ROOM);
        room->power_coin_bomb_counter += 2;
        if (room->power_coin_bomb_counter >= MJK_POWER_COIN_THRESHOLD) {
            room->power_coin_trigger_flag = 1;
        }
        return 200; // Puntos por bomba encendida
    } else {
        // Primera bomba recogida o recogida de bomba apagada
        room->chain_active = true;
        room->active_fire_bomb_index = (uint8_t)((bomb_index + 1) % MJK_BOMBS_PER_ROOM);
        room->power_coin_bomb_counter += 1;
        if (room->power_coin_bomb_counter >= MJK_POWER_COIN_THRESHOLD) {
            room->power_coin_trigger_flag = 1;
        }
        return 100; // Puntos por bomba normal
    }
}

static void LocalBcdAdd(uint8_t score_bcd[4], uint32_t pts) {
    if (!score_bcd || pts == 0) return;
    uint32_t val = 0;
    for (int i = 3; i >= 0; --i) {
        val = val * 100 + ((score_bcd[i] >> 4) * 10 + (score_bcd[i] & 0x0F));
    }
    val += pts;
    for (int i = 0; i < 4; ++i) {
        uint8_t lo = (uint8_t)(val % 10); val /= 10;
        uint8_t hi = (uint8_t)(val % 10); val /= 10;
        score_bcd[i] = (uint8_t)((hi << 4) | lo);
    }
}

static void LocalSpawnFloatingScore(MjkGameState* game, int16_t x, int16_t y, uint16_t value) {
    if (!game) return;
    for (int i = 0; i < MJK_MAX_FLOATING_SCORES; ++i) {
        if (!game->floating_scores[i].active) {
            game->floating_scores[i].active = true;
            game->floating_scores[i].x = x;
            game->floating_scores[i].y = y;
            game->floating_scores[i].value = value;
            game->floating_scores[i].timer = 30;
            return;
        }
    }
}

void MjkGame_SpawnBombPuff(MjkGameState* game, int16_t x, int16_t y, uint16_t points) {
    if (!game) return;
    for (int i = 0; i < MJK_MAX_BOMB_PUFFS; ++i) {
        if (!game->bomb_puffs[i].active) {
            game->bomb_puffs[i].active = true;
            game->bomb_puffs[i].phase = 1;
            game->bomb_puffs[i].timer = 32;
            game->bomb_puffs[i].x = x;
            game->bomb_puffs[i].y = y;
            game->bomb_puffs[i].points = points;
            return;
        }
    }
    // Fallback: reutilizar slot 0 si el pool está saturado
    game->bomb_puffs[0].active = true;
    game->bomb_puffs[0].phase = 1;
    game->bomb_puffs[0].timer = 32;
    game->bomb_puffs[0].x = x;
    game->bomb_puffs[0].y = y;
    game->bomb_puffs[0].points = points;
}

void MjkGame_UpdateBombPuffs(MjkGameState* game) {
    if (!game) return;
    for (int i = 0; i < MJK_MAX_BOMB_PUFFS; ++i) {
        MjkBombPuffEffect* puff = &game->bomb_puffs[i];
        if (!puff->active) continue;

        if (puff->timer > 0) {
            puff->timer--;
        }
        if (puff->timer == 0) {
            if (puff->phase == 1) {
                // Transición canónica Fase 1 (Humo Metasprite $25) -> Fase 2 (Puntuación) tras 32 frames
                puff->phase = 2;
                puff->timer = 32;
            } else {
                // Fin de Fase 2 tras otros 32 frames (64 frames totales)
                puff->active = false;
            }
        }
    }
}

void MjkGame_UpdateBombs(MjkGameState* game) {
    MjkGame_UpdateBombPuffs(game);
}

uint16_t MjkRoom_CollectBombCanonical(MjkGameState* game, uint8_t bomb_index) {
    if (!game || bomb_index >= game->bomb_count) return 0;
    MjkBombEntity* bomb = &game->bombs[bomb_index];
    if (bomb->collected) return 0;

    uint8_t val = bomb->state;
    if (val == 0) val = MJK_BOMB_STATE_UNLIT;

    // a) Evaluar mecha activa (val & 0x40): X=2 (+200 pts) vs apagada X=1 (+100 pts)
    uint8_t x = (val & MJK_BOMB_FLAG_FUSE_LIT) ? 2 : 1;
    uint16_t pts = (uint16_t)(x * 100);

    // Sonido canónico de recogida de bomba ($08 encendida vs $07 apagada)
    if (val & MJK_BOMB_FLAG_FUSE_LIT) {
        MjkAPU_PlaySFX(&game->apu, SFX_FIRE_BOMB);
    } else {
        MjkAPU_PlaySFX(&game->apu, SFX_COLLECT_BOMB);
    }

    // b) Acumulador de Power Coin: power_coin_bomb_counter += X
    if (game->power_coin_trigger_flag == 0) {
        game->power_coin_bomb_counter += x;
        if (game->power_coin_bomb_counter >= MJK_POWER_COIN_THRESHOLD) {
            game->power_coin_trigger_flag = 1;
            if (game->is_bomb_room) {
                game->power_coin_trigger_flag |= 2; // = 3 (spawn direct en bomb room)
                game->transmute_timer = 320;
                game->enemy_coin_timer = 320;
                game->jack.enemy_coin_timer = 320;
                game->transmute_streak = 0;
                MjkAPU_PlaySFX(&game->apu, SFX_POWER_COIN_SPAWN);
            } else {
                MjkAPU_PlaySFX(&game->apu, SFX_BOMB_CHEST_BONUS);
            }
        }
    }

    // Puntuación BCD canónica inmediata (prg.asm:5008 AddScore)
    LocalBcdAdd(game->score_bcd, pts);

    int screen_bx;
    if (bomb->room_id == game->jack.room_id || bomb->room_id == 0) {
        screen_bx = bomb->tile_x * 16 - game->camera_x;
    } else if (game->jack.room_id < MJK_TOTAL_ROOMS_COUNT && g_adjacent_rooms[game->jack.room_id].right == bomb->room_id) {
        screen_bx = (256 - game->camera_x) + (bomb->tile_x * 16);
    } else {
        screen_bx = bomb->tile_x * 16 - game->camera_x;
    }
    int screen_by = bomb->tile_y * 16;

    // CANÓNICO (sub_A270 / prg.asm:5647-5667 / TASK-F7-009 / RES-036):
    // Iniciar el efecto visual en 2 fases: Humo metasprite $25 durante 32 frames, luego puntuación 32 frames.
    MjkGame_SpawnBombPuff(game, (int16_t)(screen_bx + 8), (int16_t)(screen_by + 8), pts);

    // c) Salas normales (loc_9EB2) vs d) Salas de bombas (loc_9F4B)
    if (!game->is_bomb_room) {
        if (game->room_bomb_picked_flag == 0) {
            // Primer pickup: si starter (val & 0x80 / 0x85), maybe_bomb_thing = 1; si no starter, 0.
            game->room_bomb_picked_flag = 1;
            if (val & 0x80) {
                game->maybe_bomb_thing = MJK_BOMB_THING_STARTED; // 0x01
            } else {
                game->maybe_bomb_thing = 0;
            }

            // Buscar siguiente bomba hacia adelante y encender mecha
            int next_bomb = -1;
            for (uint8_t i = bomb_index + 1; i < game->bomb_count; ++i) {
                if (!game->bombs[i].collected && (game->bombs[i].room_id == bomb->room_id || game->bombs[i].room_id == 0)) {
                    next_bomb = i;
                    break;
                }
            }
            if (next_bomb == -1) {
                for (uint8_t i = bomb_index + 1; i < game->bomb_count; ++i) {
                    if (!game->bombs[i].collected) {
                        next_bomb = i;
                        break;
                    }
                }
            }
            if (next_bomb != -1) {
                game->bombs[next_bomb].state |= MJK_BOMB_FLAG_FUSE_LIT;
                game->active_fire_bomb_index = (uint8_t)next_bomb;
                game->chain_active = true;
            } else {
                game->chain_active = false;
            }
        } else {
            // Pickups subsecuentes:
            if ((val & MJK_BOMB_FLAG_FUSE_LIT) == 0) {
                // Si apagada: maybe_bomb_thing |= 0x02 (cadena rota), NO propagar mecha (se apaga).
                game->maybe_bomb_thing |= MJK_BOMB_THING_BROKEN; // 0x02
                game->chain_active = false;
            } else {
                // Si encendida: si final (val & 0x80) y maybe_bomb_thing == 1 -> otorgar 10.000 pts BCD, |= 0x80
                if (val & 0x80) {
                    if (game->maybe_bomb_thing == MJK_BOMB_THING_STARTED) {
                        game->maybe_bomb_thing |= MJK_BOMB_THING_PERFECT_BONUS; // 0x80
                        LocalBcdAdd(game->score_bcd, 10000);
                        LocalSpawnFloatingScore(game, (int16_t)screen_bx, (int16_t)(screen_by - 12), 10000);
                        pts += 10000;
                        MjkAPU_PlaySFX(&game->apu, SFX_BOMB_BONUS);
                    }
                }

                // Buscar siguiente bomba hacia adelante y encender mecha (val |= 0x40)
                int next_bomb = -1;
                for (uint8_t i = bomb_index + 1; i < game->bomb_count; ++i) {
                    if (!game->bombs[i].collected && (game->bombs[i].room_id == bomb->room_id || game->bombs[i].room_id == 0)) {
                        next_bomb = i;
                        break;
                    }
                }
                if (next_bomb == -1) {
                    for (uint8_t i = bomb_index + 1; i < game->bomb_count; ++i) {
                        if (!game->bombs[i].collected) {
                            next_bomb = i;
                            break;
                        }
                    }
                }
                if (next_bomb != -1) {
                    game->bombs[next_bomb].state |= MJK_BOMB_FLAG_FUSE_LIT;
                    game->active_fire_bomb_index = (uint8_t)next_bomb;
                    game->chain_active = true;
                } else {
                    game->chain_active = false;
                }
            }
        }
    } else {
        // d) Salas de bombas (loc_9F4B): 23 bombas, wrapping circular
        if (game->room_bomb_picked_flag == 0) {
            game->room_bomb_picked_flag = 1;
            game->fire_bombs_collected = 0;
            game->fire_bombs_collected_total = 0;

            // Encender primera mecha en orden circular (loc_9F8C)
            for (uint8_t step = 1; step < game->bomb_count; ++step) {
                uint8_t cand = (uint8_t)((bomb_index + step) % game->bomb_count);
                if (!game->bombs[cand].collected) {
                    game->bombs[cand].state |= MJK_BOMB_FLAG_FUSE_LIT;
                    game->active_fire_bomb_index = cand;
                    game->chain_active = true;
                    game->bomb_fuse.spark_sprite_y = (uint8_t)((game->bombs[cand].tile_y * 16) - 4);
                    game->bomb_fuse.spark_sprite_x = (uint8_t)((game->bombs[cand].tile_x * 16) + 8);
                    game->bomb_fuse.spark_sprite_tile = 0x0D;
                    game->bomb_fuse.spark_sprite_attr = 0x12;
                    game->bomb_fuse.spark_visible = true;
                    break;
                }
            }
        } else {
            // Pickups subsecuentes:
            if (val & MJK_BOMB_FLAG_FUSE_LIT) {
                game->fire_bombs_collected++;
            }
            game->fire_bombs_collected_total++;

            if (game->fire_bombs_collected_total >= 23) {
                // 23 recogidas -> ocultar chispa y abrir puerta
                game->bomb_fuse.spark_sprite_y = 0xF8;
                game->bomb_fuse.spark_visible = false;
                MjkRoom_OpenBombRoomDoor(game);
            } else {
                if ((val & MJK_BOMB_FLAG_FUSE_LIT) == 0) {
                    // Si apagada -> no avanzar mecha (mantiene posición)
                } else {
                    // Si encendida -> encender siguiente bomba intacta en orden circular y actualizar chispa
                    for (uint8_t step = 1; step < game->bomb_count; ++step) {
                        uint8_t cand = (uint8_t)((bomb_index + step) % game->bomb_count);
                        if (!game->bombs[cand].collected) {
                            game->bombs[cand].state |= MJK_BOMB_FLAG_FUSE_LIT;
                            game->active_fire_bomb_index = cand;
                            game->chain_active = true;
                            game->bomb_fuse.spark_sprite_y = (uint8_t)((game->bombs[cand].tile_y * 16) - 4);
                            game->bomb_fuse.spark_sprite_x = (uint8_t)((game->bombs[cand].tile_x * 16) + 8);
                            game->bomb_fuse.spark_sprite_tile = 0x0D;
                            game->bomb_fuse.spark_sprite_attr = 0x12;
                            game->bomb_fuse.spark_visible = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Consumir bomba
    bomb->collected = true;
    bomb->state = MJK_BOMB_STATE_EMPTY;
    game->total_bombs_collected++;
    game->greed_jump_counter = 0;

    // Sincronizar estado en MjkBombFuseSystem
    game->bomb_fuse.maybe_bomb_thing = game->maybe_bomb_thing;
    game->bomb_fuse.room_bomb_picked_flag = game->room_bomb_picked_flag;
    game->bomb_fuse.power_coin_bomb_counter = game->power_coin_bomb_counter;
    game->bomb_fuse.power_coin_trigger_flag = game->power_coin_trigger_flag;
    game->bomb_fuse.fire_bombs_collected = game->fire_bombs_collected;
    game->bomb_fuse.fire_bombs_collected_total = game->fire_bombs_collected_total;

    return pts;
}

/* =========================================================================
 * IMPLEMENTACIÓN CANÓNICA: RoomDataRAM, ReadRoomData y Blitteo (Fase 6 / RES-018)
 * ========================================================================= */

void MjkRoom_ReadRoomData(uint8_t room_id, uint8_t slot_idx, MjkRoomDataRAM* ram,
                          uint8_t room_index_in_section, uint8_t total_rooms_in_section,
                          uint8_t player_crystal_balls) {
    if (!ram || slot_idx >= MJK_ROOMDATA_TOTAL_SLOTS) return;

    MjkRoomDataSlot* slot = (slot_idx < MJK_ROOMDATA_SECTION_SLOTS)
                          ? &ram->slots[slot_idx]
                          : &ram->scratch;

    // Inicializar slot completo a 0
    memset(slot->raw, 0, MJK_ROOMDATA_SLOT_SIZE);
    slot->room_id = room_id;
    slot->padding = 0;

    const uint8_t* rdata = g_room_data_table[room_id];
    if (!rdata || room_id == 0) {
        slot->raw[2] = MJK_OBJ_STATE_EMPTY;
        return;
    }

    // Regla de mecha: byte_B (GameState_2_LoadRoom: prg.asm:3211-3226)
    uint8_t byte_B = 0;
    if (total_rooms_in_section > 0 && room_index_in_section == total_rooms_in_section - 1 && room_id < 0x90) {
        byte_B = 2; // Última sala de sección: mecha en última bomba
    }
    if (room_index_in_section == 0 && room_id < 0x90) {
        byte_B = 1; // Primera sala de sección: mecha en primera bomba (prg.asm:3303)
    }

    uint8_t byte_C = 0;
    if (room_id >= 0x90 && room_id < 0xA0) {
        byte_C = 1; // Bomb Room
    }

    uint8_t byte_E = 1; // Base de objetos: en NES byte_E se incrementa antes del primer objeto -> raw[2]
    uint8_t byte_AC = 0;
    size_t rom_pos = 0;

    while (rom_pos < 512) {
        uint8_t b = rdata[rom_pos];
        if (b == 0xFF) {
            byte_E++;
            if (byte_E < MJK_ROOMDATA_SLOT_SIZE) {
                slot->raw[byte_E] = 0xFF;
            } else {
                slot->raw[MJK_ROOMDATA_SLOT_SIZE - 1] = 0xFF;
            }
            break;
        }

        // Cabecera escénica $Ex: 3 bytes contiguos sin escribir en RoomDataRAM (RES-020)
        if ((b & 0xF0) == 0xE0) {
            rom_pos += 3;
            continue;
        }

        rom_pos++;
        uint8_t cmd_obj = 0;

        if (byte_AC != 0) {
            byte_AC--;
            cmd_obj = 0; // Bomba adicional en cadena
            if (byte_B != 2) {
                byte_B = 0; // Apaga flag si era la primera bomba
            }
        } else {
            uint8_t hi = (uint8_t)(b >> 4);
            uint8_t lo = (uint8_t)(b & 0x0F);
            if (hi == 0) {
                byte_AC = (lo == 0) ? 0 : (uint8_t)(lo - 1);
                if (room_index_in_section == 0 && room_id < 0x90) {
                    byte_B = 1;
                }
                if (byte_C != 0) {
                    byte_AC = 0x18; // 24 bombas fijas en Bomb Room
                }
                rom_pos++; // Salta la coordenada YX de la primera bomba
                cmd_obj = 0;
            } else {
                rom_pos++; // Salta la coordenada YX o el primer argumento
                cmd_obj = b;
            }
        }

        uint8_t hi = (uint8_t)(cmd_obj >> 4);
        uint8_t lo = (uint8_t)(cmd_obj & 0x0F);

        if (hi < 0x0B) {
            byte_E++;
            if (byte_E >= MJK_ROOMDATA_SLOT_SIZE - 1) {
                byte_E = MJK_ROOMDATA_SLOT_SIZE - 2; // Preserva espacio para terminador 0xFF
            }
        }

        if (hi == 0) {
            // JT_94AD_0: Bomba
            uint8_t val = 0x05;
            if (byte_B == 1) {
                val = 0x85;
            } else if (byte_B == 2 && byte_AC == 0) {
                val = 0x85;
            }
            slot->raw[byte_E] = val;
        } else if (hi == 1 || hi == 2 || hi == 3 || hi == 4 || hi == 9 || hi == 0x0A) {
            // JT_94AD_multi: Cofres visibles y ocultos
            uint8_t val = 0x10;
            if (hi == 3 || hi == 4 || hi == 0x0A) {
                val = 0x11; // Cofre naranja
            }
            if (lo == 6) {
                // Esfinge tiene byte extra de SphinxEventID
                rom_pos++;
            }
            if ((cmd_obj & 0x80) != 0) {
                val |= 0x20; // Flag oculto para cofres $90..$AF -> $30, $31
            }
            slot->raw[byte_E] = val;
        } else if (hi == 5 || hi == 6) {
            // JT_94AD_5_6: Bloque oculto multi-hit ($50..$6F)
            slot->raw[byte_E] = 0x33; // 0x13 | 0x20
        } else if (hi == 7) {
            // JT_94AD_7: Ítem expuesto
            uint8_t val = lo;
            if (val == 9 && (player_crystal_balls & 1) != 0) {
                val = 0x0F;
            } else if (val == 0x0A && (player_crystal_balls & 2) != 0) {
                val = 0x0F;
            }
            slot->raw[byte_E] = val;
        } else if (hi == 8) {
            // JT_94AD_8_Door: Puerta de salida
            uint8_t door_arg = rdata[rom_pos - 1];
            uint8_t door_type = (uint8_t)(door_arg & 7);
            uint8_t door_val = g_door_state_tile_table[door_type];
            slot->raw[byte_E] = door_val;
        }
    }
}

void MjkRoom_InitSection(MjkRoomDataRAM* ram, uint8_t section_id, uint8_t player_crystal_balls) {
    if (!ram) return;
    if (section_id > MJK_SECTIONS_COUNT) return;

    // Acepta tanto 0 como 1 para la primera sección (Sección 1-1: Salas 1..6)
    uint8_t sec_idx = (section_id == 0) ? 0 : (uint8_t)(section_id - 1);
    const uint8_t* room_list = g_section_rooms_table[sec_idx];
    if (!room_list) return;

    // Contar cantidad de salas activas en la sección (hasta 8 salas)
    uint8_t count = 0;
    while (room_list[count] != 0 && count < MJK_ROOMDATA_SECTION_SLOTS) {
        count++;
    }

    // Inicializar cada sala en su respectivo slot [0..count-1]
    for (uint8_t i = 0; i < count; ++i) {
        uint8_t r_id = room_list[i];
        MjkRoom_ReadRoomData(r_id, i, ram, i, count, player_crystal_balls);
    }

    // Limpiar slots restantes de la sección si hay menos de 8 salas
    for (uint8_t i = count; i < MJK_ROOMDATA_SECTION_SLOTS; ++i) {
        memset(ram->slots[i].raw, 0, MJK_ROOMDATA_SLOT_SIZE);
    }
}

void MjkRoom_BlitObjects(uint8_t room_id, const uint8_t* slot_data,
                         uint8_t out_metatiles[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W]) {
    if (!slot_data || !out_metatiles || room_id == 0) return;

    const uint8_t* rdata = g_room_data_table[room_id];
    if (!rdata) return;

    uint8_t slot_obj_idx = 2; // Estados dinámicos inician en slot_data[2]
    uint8_t byte_AC = 0;
    size_t rom_pos = 0;

    while (rom_pos < 512 && slot_obj_idx < MJK_ROOMDATA_SLOT_SIZE) {
        uint8_t b = rdata[rom_pos];
        if (b == 0xFF) break;

        // Cabecera escénica $Ex: 3 bytes contiguos sin blittear ni consumir slot (RES-020: 1.3)
        if ((b & 0xF0) == 0xE0) {
            rom_pos += 3;
            continue;
        }

        rom_pos++;
        uint8_t cmd_obj = 0;
        uint8_t yx = 0;

        if (byte_AC != 0) {
            byte_AC--;
            cmd_obj = 0; // Bomba repetida
            yx = b;
        } else {
            uint8_t hi = (uint8_t)(b >> 4);
            uint8_t lo = (uint8_t)(b & 0x0F);
            if (hi == 0) {
                byte_AC = (lo == 0) ? 0 : (uint8_t)(lo - 1);
                if (room_id >= 0x90 && room_id < 0xA0) {
                    byte_AC = 0x18;
                }
                cmd_obj = 0;
                yx = rdata[rom_pos++];
            } else {
                cmd_obj = b;
                yx = rdata[rom_pos++];
            }
        }

        uint8_t hi = (uint8_t)(cmd_obj >> 4);
        uint8_t lo = (uint8_t)(cmd_obj & 0x0F);

        if (hi >= 0x0B) {
            continue; // Comandos $B..$D no consumen slot ni blittean
        }

        uint8_t state = slot_data[slot_obj_idx++];
        if (state == MJK_OBJ_STATE_EMPTY) break;

        if (hi == 8) {
            // Puerta ($80..$8F): Blittea 2 metatiles contiguos
            uint8_t door_idx = lo;
            if (door_idx < 16) {
                const MjkDoorPositionTile* dpos = &g_door_position_tile_table[door_idx];
                uint8_t y_offset = (state & MJK_OBJ_STATE_DOOR_OPEN) ? 2 : 0;
                uint8_t tile1 = (uint8_t)(dpos->closed_tile + y_offset);
                uint8_t tile2 = (uint8_t)(dpos->closed_tile + y_offset + 1);

                uint8_t y1 = (uint8_t)(dpos->y_tile >> 4);
                uint8_t x1 = (uint8_t)(dpos->y_tile & 0x0F);
                uint8_t y2 = (uint8_t)(dpos->x_tile >> 4);
                uint8_t x2 = (uint8_t)(dpos->x_tile & 0x0F);

                if (y1 < MJK_ROOM_TILES_H && x1 < MJK_ROOM_TILES_W) {
                    out_metatiles[y1][x1] = tile1;
                }
                if (y2 < MJK_ROOM_TILES_H && x2 < MJK_ROOM_TILES_W) {
                    out_metatiles[y2][x2] = tile2;
                }
            }
            continue;
        }

        if (lo == 6 && (hi == 1 || hi == 2 || hi == 3 || hi == 4 || hi == 9 || hi == 0x0A)) {
            rom_pos++; // Salta SphinxEventID
        }

        // Filtro de visibilidad auténtico (sub_96F5 / prg.asm:3746-3769)
        if (yx == 0) continue;
        if ((state & MJK_OBJ_STATE_HIDDEN) != 0) continue;
        if (state == MJK_OBJ_STATE_COLLECTED) continue;

        uint8_t y = (uint8_t)(yx >> 4);
        uint8_t x = (uint8_t)(yx & 0x0F);
        if (y < MJK_ROOM_TILES_H && x < MJK_ROOM_TILES_W) {
            uint8_t tile_id = g_item_to_tile_table[state & 0x1F];
            out_metatiles[y][x] = tile_id;
        }
    }
}

void MjkRoom_LoadWithRAM(MjkRoomContext* room, uint8_t room_id, MjkRoomDataRAM* ram,
                         uint8_t section_id, uint8_t player_crystal_balls) {
    if (!room) return;
    memset(room, 0, sizeof(MjkRoomContext));
    room->room_id = room_id;

    if (room_id == 0) return;

    // 1. Gestión de persistencia de RAM (IfNotTitleScreenRoomDoThings: prg.asm:3084-3135)
    MjkRoomDataRAM fallback_ram;
    if (!ram) {
        ram = &fallback_ram;
        memset(&fallback_ram, 0, sizeof(MjkRoomDataRAM));
    }

    const uint8_t* slot_data = NULL;
    int found_slot = -1;

    for (int i = 0; i < MJK_ROOMDATA_SECTION_SLOTS; ++i) {
        if (ram->slots[i].room_id == room_id) {
            found_slot = i;
            break;
        }
    }

    if (found_slot >= 0) {
        ram->scratch.room_id = 0; // Limpia scratch
        slot_data = ram->slots[found_slot].raw;
    } else {
        // Carga en Scratch Slot 8 ($0683)
        if (ram->scratch.room_id != room_id) {
            MjkRoom_ReadRoomData(room_id, MJK_ROOMDATA_SCRATCH_SLOT, ram, 0, 1, player_crystal_balls);
        }
        slot_data = ram->scratch.raw;
    }

    // 2. Decodificación de geometría 4-tier
    uint8_t metatiles_2d[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W];
    bool is_horizontal = false;
    MjkRoom_DecodeGeometry(room_id, metatiles_2d, &is_horizontal);

    // 3. Superposición de objetos dinámicos de RAM (sub_9696)
    MjkRoom_BlitObjects(room_id, slot_data, metatiles_2d);

    // 4. Copia a matriz plana de MjkRoomContext
    for (int r = 0; r < MJK_ROOM_TILES_H; ++r) {
        for (int c = 0; c < MJK_ROOM_TILES_W; ++c) {
            room->metatiles[r * MJK_ROOM_TILES_W + c] = metatiles_2d[r][c];
        }
    }

    // 5. Extracción de metadatos de puerta y bombas
    bool has_cond = false;
    room->door_condition_id = MjkRoom_ExtractDoorCondition(room_id, &has_cond);

    const uint8_t* rdata = g_room_data_table[room_id];
    if (rdata) {
        uint8_t slot_obj_idx = 2;
        uint8_t byte_AC = 0;
        size_t rom_pos = 0;
        uint8_t bomb_count = 0;

        while (rom_pos < 512 && slot_obj_idx < MJK_ROOMDATA_SLOT_SIZE) {
            uint8_t b = rdata[rom_pos];
            if (b == 0xFF) break;

            // Cabecera escénica $Ex: 3 bytes contiguos (RES-020)
            if ((b & 0xF0) == 0xE0) {
                rom_pos += 3;
                continue;
            }

            rom_pos++;
            uint8_t cmd_obj = 0;

            if (byte_AC != 0) {
                byte_AC--;
                cmd_obj = 0;
            } else {
                uint8_t hi = (uint8_t)(b >> 4);
                uint8_t lo = (uint8_t)(b & 0x0F);
                if (hi == 0) {
                    byte_AC = (lo == 0) ? 0 : (uint8_t)(lo - 1);
                    if (room_id >= 0x90 && room_id < 0xA0) byte_AC = 0x18;
                    cmd_obj = 0;
                    rom_pos++;
                } else {
                    cmd_obj = b;
                    rom_pos++;
                }
            }

            uint8_t hi = (uint8_t)(cmd_obj >> 4);
            uint8_t lo = (uint8_t)(cmd_obj & 0x0F);

            if (hi >= 0x0B) continue;

            uint8_t state = slot_data[slot_obj_idx++];
            if (state == MJK_OBJ_STATE_EMPTY) break;

            if (hi == 0) {
                // Bomba
                if ((state & MJK_OBJ_STATE_LIT_INITIAL) || (state & MJK_OBJ_STATE_LIT_FUSE)) {
                    room->active_fire_bomb_index = bomb_count;
                }
                bomb_count++;
            } else if (hi == 8) {
                // Puerta
                uint8_t door_idx = lo;
                if (door_idx < 16) {
                    const MjkDoorPositionTile* dpos = &g_door_position_tile_table[door_idx];
                    room->door_y = (uint8_t)(dpos->y_tile >> 4);
                    room->door_x = (uint8_t)(dpos->y_tile & 0x0F);
                    room->door_is_open = (state & MJK_OBJ_STATE_DOOR_OPEN) != 0;
                }
            } else if (lo == 6 && (hi == 1 || hi == 2 || hi == 3 || hi == 4 || hi == 9 || hi == 0x0A)) {
                rom_pos++;
            }
        }
    }

    (void)section_id;
}

int MjkRoom_FindSlot(const MjkRoomDataRAM* ram, uint8_t room_id) {
    if (!ram || room_id == 0) return -1;
    for (int i = 0; i < MJK_ROOMDATA_SECTION_SLOTS; ++i) {
        if (ram->slots[i].room_id == room_id) return i;
    }
    if (ram->scratch.room_id == room_id) return MJK_ROOMDATA_SCRATCH_SLOT;
    return -1;
}

void MjkRoom_SyncObjectStateToRAM(MjkRoomDataRAM* ram, uint8_t room_id, uint8_t ram_slot_idx, uint8_t state) {
    if (!ram || room_id == 0 || ram_slot_idx >= MJK_ROOMDATA_SLOT_SIZE) return;
    int slot_idx = MjkRoom_FindSlot(ram, room_id);
    if (slot_idx < 0) return;

    if (slot_idx < MJK_ROOMDATA_SECTION_SLOTS) {
        ram->slots[slot_idx].raw[ram_slot_idx] = state;
    } else {
        ram->scratch.raw[ram_slot_idx] = state;
    }
}
