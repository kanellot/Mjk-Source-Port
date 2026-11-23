/**
 * @file mjk_game.c
 * @brief Implementación del orquestador del frame NMI, bucle a 60 Hz y blitter 2C02.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: Pipeline NMI (prg.asm:91-123) y nes_ground_truth_spec.md
 */

#include "mjk_game.h"
#include "mjk_ppu.h"
#include "mjk_room_decoder.h"
#include "mjk_room_data.h"
#include "mjk_player.h"
#include "mjk_hud.h"
#include <stdio.h>
#include <string.h>

void MjkGame_SpawnFloatingScore(MjkGameState* game, int16_t x, int16_t y, uint16_t value) {
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

void MjkGame_UpdateFloatingScores(MjkGameState* game) {
    if (!game) return;
    for (int i = 0; i < MJK_MAX_FLOATING_SCORES; ++i) {
        if (game->floating_scores[i].active) {
            game->floating_scores[i].y -= 1;
            if (game->floating_scores[i].timer > 0) {
                game->floating_scores[i].timer--;
            }
            if (game->floating_scores[i].timer == 0) {
                game->floating_scores[i].active = false;
            }
        }
    }
}

void MjkGame_UpdateJackPalette(MjkGameState* game) {
    if (!game) return;
    uint8_t lvl = game->jack.mighty_level;
    if (game->jack.power_level > lvl) lvl = (uint8_t)game->jack.power_level;
    if (lvl > 3) lvl = 3;

    // Jack utiliza de forma fija la Subpaleta de Sprite 1 (direcciones PPU $3F14-$3F17,
    // índices 20..23 en la paleta global de 32 colores).
    // MainPalette (data.asm:264-265):
    //   - Color 0 (transparente): $21 (índice 20)
    //   - Color 1: $30 (Blanco - ribetes, ojos, guantes) (índice 21)
    //   - Color 2: $12 (Azul medio - capucha, traje) (índice 22)
    //   - Color 3: Color dinámico según nivel Mighty (índice 23, PPU $3F17)
    // NOTA: Los índices 17, 18, 19 corresponden a Sprite Palette 0, NO a Jack.
    game->palette_rgba[21] = MjkPPU_ColorToARGB(0x30);
    game->palette_rgba[22] = MjkPPU_ColorToARGB(0x12);
    game->palette_rgba[23] = MjkPPU_ColorToARGB(mjk_player_get_palette_color(lvl));
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

void MjkGame_OpenChest(MjkGameState* game, uint8_t chest_idx) {
    if (!game || chest_idx >= game->chest_count) return;
    MjkChestEntity* ch = &game->chests[chest_idx];
    if (ch->is_open) return;

    // Cofre naranja (Item $11): inmune si PlayerMightyLevel == 0; requiere nivel >= 1.
    if (ch->chest_type == 1 && !can_open_orange_chest(&game->jack)) {
        return;
    }

    // Si cofre oculto ($30): primer salto lo revela (pasa a $10 cerrado y dibuja metatile cerrado)
    if (ch->is_hidden) {
        ch->is_hidden = false;
        ch->jumps_needed = 1;
        uint8_t m_id = (ch->chest_type == 1) ? 0x4A : 0x28; // 0x4A naranja cerrado, 0x28 rojo cerrado
        if (ch->tile_y < MJK_ROOM_TILES_H && ch->tile_x < MJK_ROOM_TILES_W && (ch->room_id == game->current_room || ch->room_id == 0)) {
            game->room_metatiles[ch->tile_y * MJK_ROOM_TILES_W + ch->tile_x] = m_id;
        }
        uint8_t ram_val = (ch->chest_type == 1) ? 0x11 : 0x10;
        MjkRoom_SyncObjectStateToRAM(&game->room_data_ram, ch->room_id, ch->ram_slot_idx, ram_val);
        // prg.asm:5392-5431: En la NES original abrir/revelar cofres NO emite sonido
        return;
    }

    // Segundo salto (o primer salto si era visible): abre el cofre
    ch->is_open = true;
    ch->reward_spawned = true;
    game->greed_jump_counter = 0;

    // Si ítem 7 (Tecmo Plate) o ítem 9 (Extra Coin) y ya se recolectó 1UP en la ronda, sustituye por ítem 8 (Mighty Coin)
    if ((ch->item_id == 7 || ch->item_id == 9) && game->collected_1up_this_round != 0) {
        ch->item_id = 8;
    }

    // Si el acumulador de 20 bombas activó power coin trigger ($0357 == 1), spawnea Power Coin y pasa trigger a 3
    if (game->power_coin_trigger_flag == 1) {
        game->power_coin_trigger_flag = 3;
        game->bomb_fuse.power_coin_trigger_flag = 3;
        game->transmute_timer = 320;
        game->enemy_coin_timer = 320;
        game->jack.enemy_coin_timer = 320;
        game->transmute_streak = 0;
        MjkAPU_PlaySFX(&game->apu, SFX_POWER_COIN_MUSIC);
    }

    // sub_A1A6 escribe el ítem en room_data / RoomDataRAM (o $0F si spawnea monstruo >= 9)
    if (ch->item_id >= 9) {
        if (ch->tile_y < MJK_ROOM_TILES_H && ch->tile_x < MJK_ROOM_TILES_W && (ch->room_id == game->current_room || ch->room_id == 0)) {
            game->room_metatiles[ch->tile_y * MJK_ROOM_TILES_W + ch->tile_x] = 0x2F;
        }
        MjkRoom_SyncObjectStateToRAM(&game->room_data_ram, ch->room_id, ch->ram_slot_idx, 0x0F);
        ch->collected = true; // Monstruo brotó y el hueco queda vacío
    } else {
        uint8_t m_id = g_item_to_tile_table[ch->item_id & 0x1F];
        if (ch->tile_y < MJK_ROOM_TILES_H && ch->tile_x < MJK_ROOM_TILES_W && (ch->room_id == game->current_room || ch->room_id == 0)) {
            game->room_metatiles[ch->tile_y * MJK_ROOM_TILES_W + ch->tile_x] = m_id;
        }
        MjkRoom_SyncObjectStateToRAM(&game->room_data_ram, ch->room_id, ch->ram_slot_idx, ch->item_id);
    }
}

void MjkGame_HitChestFromBelow(MjkGameState* game, uint8_t chest_idx) {
    if (!game || chest_idx >= game->chest_count) return;
    MjkChestEntity* ch = &game->chests[chest_idx];
    if (ch->is_open) return;

    // Cofre oculto: inmune desde abajo (no se revela desde abajo según prg.asm:5263-5266)
    if (ch->is_hidden) {
        return;
    }

    // Requisitos de nivel Mighty para abrir desde abajo:
    // Cofres normales: requiere PlayerMightyLevel >= 2 (Naranja o Rojo)
    // Cofres naranjas: requiere PlayerMightyLevel >= 1 (Azul, Naranja o Rojo)
    if (ch->chest_type == 1) {
        if (!can_open_orange_chest(&game->jack)) return;
    } else {
        if (!can_headbutt_red_chest(&game->jack)) return;
    }

    MjkGame_OpenChest(game, chest_idx);
}

void MjkGame_HitSecretBlock(MjkGameState* game, uint8_t block_idx) {
    if (!game || block_idx >= game->secret_block_count) return;
    MjkSecretBlockEntity* sb = &game->secret_blocks[block_idx];
    if (sb->revealed) return;

    uint8_t coord = (uint8_t)((sb->tile_y << 4) | (sb->tile_x & 0x0F));
    if (coord != game->secret_block_active_coord) {
        game->secret_block_active_coord = coord;
        game->secret_block_hits_left = sb->hits_required;
    }

    if (game->secret_block_hits_left > 0) {
        game->secret_block_hits_left--;
        sb->hits_left = game->secret_block_hits_left;
    }

    if (game->secret_block_hits_left == 0) {
        sb->revealed = true;
        game->secret_block_active_coord = 0x0F;
        if (sb->tile_y < MJK_ROOM_TILES_H && sb->tile_x < MJK_ROOM_TILES_W && (sb->room_id == game->current_room || sb->room_id == 0)) {
            game->room_metatiles[sb->tile_y * MJK_ROOM_TILES_W + sb->tile_x] = 0x2F; // Metatile ladrillo sólido
        }
        MjkRoom_SyncObjectStateToRAM(&game->room_data_ram, sb->room_id, sb->ram_slot_idx, 0x0F);
        MjkAPU_PlaySFX(&game->apu, SFX_SECRET_BLOCK); // Sonido $0E (Sound_SecretBlockRevealed)
    }
}

void MjkGame_CollectChestItem(MjkGameState* game, uint8_t chest_idx) {
    if (!game || chest_idx >= game->chest_count) return;
    MjkChestEntity* ch = &game->chests[chest_idx];
    if (!ch->is_open || ch->collected) return;

    ch->collected = true;
    if (ch->tile_y < MJK_ROOM_TILES_H && ch->tile_x < MJK_ROOM_TILES_W && (ch->room_id == game->current_room || ch->room_id == 0)) {
        game->room_metatiles[ch->tile_y * MJK_ROOM_TILES_W + ch->tile_x] = 0x2F; // Metatile vacío $2F
    }
    MjkRoom_SyncObjectStateToRAM(&game->room_data_ram, ch->room_id, ch->ram_slot_idx, 0x0F);

    int screen_cx = ch->tile_x * 16 - (int)game->camera_x;
    int screen_cy = ch->tile_y * 16 - (int)game->camera_y;
    if (game->jack.room_id < MJK_TOTAL_ROOMS_COUNT && ch->room_id != game->jack.room_id && ch->room_id != 0) {
        const MjkAdjacentRoom* adj = &g_adjacent_rooms[game->jack.room_id];
        if (ch->room_id == adj->right) {
            screen_cx = (256 - (int)game->camera_x) + (ch->tile_x * 16);
        } else if (ch->room_id == adj->left) {
            screen_cx = (-256 - (int)game->camera_x) + (ch->tile_x * 16);
        } else if (ch->room_id == adj->down) {
            screen_cy = (240 - (int)game->camera_y) + (ch->tile_y * 16);
        } else if (ch->room_id == adj->up) {
            screen_cy = (-240 - (int)game->camera_y) + (ch->tile_y * 16);
        }
    }

    uint32_t pts = 0;
    switch (ch->item_id) {
        case 0: pts = 100; MjkAPU_PlaySFX(&game->apu, SFX_FIRE_BOMB); break; // $08 (Sound_BombCollectedLit)
        case 1: pts = 300; MjkAPU_PlaySFX(&game->apu, SFX_FIRE_BOMB); break; // $08 (Sound_BombCollectedLit)
        case 2:
            pts = 1000;
            game->transmute_timer = 320;
            game->enemy_coin_timer = 320;
            game->jack.enemy_coin_timer = 320;
            MjkAPU_PlaySFX(&game->apu, SFX_FIRE_BOMB);
            break;
        case 3:
        case 0x47: // Gold Coin
            pts = 500;
            game->gold_coins_collected++;
            MjkAPU_PlaySFX(&game->apu, SFX_COIN_COLLECTED); // $04 (Sound_CoinCollected)
            break;
        case 4: // Mighty Drink
            mjk_game_collect_mighty_drink(game);
            MjkGame_SpawnFloatingScore(game, (int16_t)screen_cx, (int16_t)(screen_cy - 12), 1000);
            pts = 0;
            break;
        case 5:
        case 0x27: // Extra Bomb
            pts = 100;
            game->total_bombs_collected++;
            game->power_coin_bomb_counter++;
            if (game->power_coin_bomb_counter >= MJK_POWER_COIN_THRESHOLD) {
                if (game->power_coin_trigger_flag == 0) {
                    game->power_coin_trigger_flag = 1;
                    game->bomb_fuse.power_coin_trigger_flag = 1;
                }
            }
            MjkAPU_PlaySFX(&game->apu, SFX_FIRE_BOMB);
            break;
        case 6:
        case 0x34: // Sphinx (abre puerta)
            pts = 10000;
            MjkRoom_HandleOpenDoorMaybe(game);
            MjkAPU_PlaySFX(&game->apu, SFX_DOOR_OPEN); // $0F (Sphinx abre puerta)
            break;
        case 7: // Tecmo Plate
            pts = 100000;
            mjk_game_award_1up(game);
            break;
        case 8:
        case 0x8F: // Mighty Coin / Power Coin [P]
            pts = 1000;
            game->mighty_coins++;
            game->jack.mighty_coins = game->mighty_coins;
            game->transmute_timer = 320;
            game->enemy_coin_timer = 320;
            game->jack.enemy_coin_timer = 320;
            if (game->mighty_coins >= 10) {
                game->mighty_coins = 0;
                game->jack.mighty_coins = 0;
                MjkGame_ChangeState(game, GAME_STATE_7_TORTURE_ROOM);
            }
            MjkAPU_PlaySFX(&game->apu, SFX_COIN_COLLECTED); // $04 (Sound_CoinCollected)
            break;
        case 9: pts = 0; MjkAPU_PlaySFX(&game->apu, SFX_CRYSTAL_BALL); break; // $0B (Sound_CrystalBall)
        case 10: pts = 0; game->gdv_value++; MjkAPU_PlaySFX(&game->apu, SFX_CRYSTAL_BALL); break; // $0B (Sound_CrystalBall)
        case 11: case 12: case 13:
        case 0x63: case 0x90: case 0x91: // Royal Family (abre puerta)
            pts = 50000;
            MjkRoom_HandleOpenDoorMaybe(game);
            MjkAPU_PlaySFX(&game->apu, SFX_DOOR_OPEN); // $0F (Royal Family)
            break;
        case 14: pts = 1000000; MjkAPU_PlaySFX(&game->apu, SFX_COLLECTED_FULL_FAMILY); break; // $23 (Beelzebub / Best Ending)
        default: pts = 1000; MjkAPU_PlaySFX(&game->apu, SFX_COIN_COLLECTED); break;
    }

    if (pts > 0) {
        LocalBcdAdd(game->score_bcd, pts);
        MjkGame_SpawnFloatingScore(game, (int16_t)screen_cx, (int16_t)(screen_cy - 12), (uint16_t)(pts > 65535 ? 9999 : pts));
    }
}

void MjkGame_UpdateChestsAndSecrets(MjkGameState* game) {
    if (!game) return;

    // Actualizar animaciones de recogida de bombas (RES-036 / TASK-F7-009)
    MjkGame_UpdateBombPuffs(game);

    if (!game->jack.is_alive) return;

    int jx = FIXED_TO_INT(game->jack.pos_x);
    int jy = FIXED_TO_INT(game->jack.pos_y);

    // 1. Interacción con Cofres del Tesoro
    for (uint8_t c = 0; c < game->chest_count; ++c) {
        MjkChestEntity* ch = &game->chests[c];

        int screen_cx = ch->tile_x * 16 - (int)game->camera_x;
        int screen_cy = ch->tile_y * 16 - (int)game->camera_y;
        if (game->jack.room_id < MJK_TOTAL_ROOMS_COUNT && ch->room_id != game->jack.room_id && ch->room_id != 0) {
            const MjkAdjacentRoom* adj = &g_adjacent_rooms[game->jack.room_id];
            if (ch->room_id == adj->right) {
                screen_cx = (256 - (int)game->camera_x) + (ch->tile_x * 16);
            } else if (ch->room_id == adj->left) {
                screen_cx = (-256 - (int)game->camera_x) + (ch->tile_x * 16);
            } else if (ch->room_id == adj->down) {
                screen_cy = (240 - (int)game->camera_y) + (ch->tile_y * 16);
            } else if (ch->room_id == adj->up) {
                screen_cy = (-240 - (int)game->camera_y) + (ch->tile_y * 16);
            }
        }

        bool h_overlap = (jx + 12 >= screen_cx && jx + 4 <= screen_cx + 16);

        // A) Si el cofre ya está abierto pero el ítem no ha sido recogido: Jack lo recoge al tocarlo
        if (ch->is_open && !ch->collected) {
            int center_x = jx + 8;
            int center_y = jy + 8;
            bool center_overlap = (center_x >= screen_cx && center_x < screen_cx + 16 &&
                                   center_y >= screen_cy && center_y < screen_cy + 16);
            bool aabb_overlap = (jx + 12 >= screen_cx && jx + 4 <= screen_cx + 16 &&
                                 jy + 12 >= screen_cy && jy + 4 <= screen_cy + 16);

            if (center_overlap || aabb_overlap) {
                MjkGame_CollectChestItem(game, c);
            }
            continue;
        }

        if (ch->is_open) continue;

        // B) Salto / aterrizaje sobre la tapa superior
        if (h_overlap && jy + 16 >= screen_cy && jy + 16 <= screen_cy + 8 &&
            (game->jack.vel_y > 0 || game->jack.subpixel_vy > 0)) {
            
            // Rebote físico hacia arriba
            game->jack.vel_y = -3;
            game->jack.subpixel_vy = -768;
            game->jack.jump_accumulator = 0x2000;
            game->jack.jump_step_index = 32;
            game->jack.state = JACK_STATE_JUMPING;
            game->jack.on_ground = false;

            if (ch->chest_type == 1 && !can_open_orange_chest(&game->jack)) {
                MjkAPU_PlaySfx(&game->apu, SFX_COLLECT_BOMB);
            } else {
                MjkGame_OpenChest(game, c);
            }
        }
        // C) Golpe con la cabeza desde abajo (Jack ascendiendo)
        else if (h_overlap && jy <= screen_cy + 16 && jy >= screen_cy + 8 &&
                 (game->jack.vel_y < 0 || game->jack.subpixel_vy < 0)) {
            
            if (!ch->is_hidden) {
                bool can_open_below = false;
                if (ch->chest_type == 1) {
                    can_open_below = can_open_orange_chest(&game->jack);
                } else {
                    can_open_below = can_headbutt_red_chest(&game->jack);
                }
                if (can_open_below) {
                    MjkGame_OpenChest(game, c);
                    game->jack.vel_y = 0;
                    game->jack.subpixel_vy = 0;
                    game->jack.jump_accumulator = 0x4000;
                    game->jack.state = JACK_STATE_FALLING;
                    game->jack.ceiling_hit = true;
                }
            }
        }
    }

    // 2. Interacción con Bloques Secretos de Pasillos ($6N)
    for (uint8_t s = 0; s < game->secret_block_count; ++s) {
        MjkSecretBlockEntity* sb = &game->secret_blocks[s];
        if (sb->revealed) continue;

        int screen_sx = sb->tile_x * 16 - (int)game->camera_x;
        int screen_sy = sb->tile_y * 16 - (int)game->camera_y;
        if (game->jack.room_id < MJK_TOTAL_ROOMS_COUNT && sb->room_id != game->jack.room_id && sb->room_id != 0) {
            const MjkAdjacentRoom* adj = &g_adjacent_rooms[game->jack.room_id];
            if (sb->room_id == adj->right) {
                screen_sx = (256 - (int)game->camera_x) + (sb->tile_x * 16);
            } else if (sb->room_id == adj->left) {
                screen_sx = (-256 - (int)game->camera_x) + (sb->tile_x * 16);
            } else if (sb->room_id == adj->down) {
                screen_sy = (240 - (int)game->camera_y) + (sb->tile_y * 16);
            } else if (sb->room_id == adj->up) {
                screen_sy = (-240 - (int)game->camera_y) + (sb->tile_y * 16);
            }
        }

        bool h_overlap = (jx + 12 >= screen_sx && jx + 4 <= screen_sx + 16);

        // Golpe con la cabeza desde abajo
        if (h_overlap && jy <= screen_sy + 16 && jy >= screen_sy + 8 &&
            (game->jack.vel_y < 0 || game->jack.subpixel_vy < 0)) {
            
            MjkGame_HitSecretBlock(game, s);
            game->jack.vel_y = 0;
            game->jack.subpixel_vy = 0;
            game->jack.jump_accumulator = 0x4000;
            game->jack.state = JACK_STATE_FALLING;
            game->jack.ceiling_hit = true;
        }
        // Impacto desde arriba por aterrizaje
        else if (h_overlap && jy + 16 >= screen_sy && jy + 16 <= screen_sy + 8 &&
                 (game->jack.vel_y > 0 || game->jack.subpixel_vy > 0)) {
            
            game->jack.vel_y = -3;
            game->jack.subpixel_vy = -768;
            game->jack.jump_accumulator = 0x2000;
            game->jack.jump_step_index = 32;
            game->jack.state = JACK_STATE_JUMPING;
            game->jack.on_ground = false;

            MjkGame_HitSecretBlock(game, s);
        }
    }
}

// Prototipo de la máquina de estados definida en mjk_state_machine.c
void MjkGame_ExecuteState(MjkGameState* game, const MjkInputState* inputs);

static void RenderCopyrightScreen(MjkGameState* game, uint32_t* fb) {
    if (!fb) return;
    if (!game->assets || !game->assets->active_chr_bank) {
        for (int i = 0; i < MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT; ++i) fb[i] = 0xFF000000;
        return;
    }

    const uint8_t* chr_bank = game->assets->active_chr_bank;
    const uint8_t* bg_table = chr_bank + 4096;

    // Paleta de Pantalla de Copyright / Título: MainPalette (fondo $21 Azul Cielo)
    uint8_t nes_pal[32];
    uint32_t pal_argb[32];
    MjkRoom_LoadAuthenticPalettes(0, nes_pal);
    MjkPPU_LoadPaletteARGB(nes_pal, pal_argb);

    // 1. Rellenar todo el fondo con el color $21 (Azul Cielo Tecmo: pal_argb[0])
    uint32_t bg_color = pal_argb[0];
    for (int i = 0; i < MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT; ++i) {
        fb[i] = bg_color;
    }

    // 2. Renderizar cadenas canónicas en Nametable 0 (Subpaleta 0)
    const uint32_t* sub0 = &pal_argb[0];

    static const struct { int x, y; const char* text; } s_texts[] = {
        { 48, 64,  "MIGHTY BOMB JACK" },
        { 32, 96,  "TM AND (C)      TECMO,LTD." },
        { 80, 112, "LICENSED BY" },
        { 32, 128, "NINTENDO OF AMERICA INC." }
    };

    for (size_t t = 0; t < sizeof(s_texts)/sizeof(s_texts[0]); ++t) {
        int tx = s_texts[t].x;
        int ty = s_texts[t].y;
        const char* str = s_texts[t].text;
        for (int i = 0; str[i] != '\0'; ++i) {
            char ch = str[i];
            uint8_t t_id = 0x24; // Espacio
            if (ch >= 'A' && ch <= 'Z') t_id = (uint8_t)(0x0A + (ch - 'A'));
            else if (ch >= '0' && ch <= '9') t_id = (uint8_t)(ch - '0');
            else if (ch == '.') t_id = 0x61;
            else if (ch == ',') t_id = 0x2D;
            else if (ch == '(') t_id = 0x2C; // '(C)'
            else if (ch == ')') t_id = 0x24; // Espacio
            MjkPPU_BlitTile(fb, tx + i * 8, ty, bg_table + (size_t)t_id * 16, sub0, true, false, false);
        }
    }

    // Glifos TM ($DC, $DD) en Fila 8, X=184, 192
    MjkPPU_BlitTile(fb, 184, 64, bg_table + (size_t)0xDC * 16, sub0, true, false, false);
    MjkPPU_BlitTile(fb, 192, 64, bg_table + (size_t)0xDD * 16, sub0, true, false, false);

    // 3. Sprites OAM "1987" en Fila 12 (Y=95):
    // X=104 ('1'), X=112 ('9'), X=120 ('8'), X=128 ('7') con subpaleta 0
    MjkOamSprite yr_sprites[4] = {
        { 95, 0x01, 0x00, 104 },
        { 95, 0x09, 0x00, 112 },
        { 95, 0x08, 0x00, 120 },
        { 95, 0x07, 0x00, 128 }
    };
    for (int i = 0; i < 4; ++i) {
        MjkPPU_BlitSprite(fb, chr_bank, &yr_sprites[i], &pal_argb[16]);
    }
}

static void RenderTitleScreen(MjkGameState* game, uint32_t* fb) {
    if (!fb) return;
    if (!game->assets || !game->assets->active_chr_bank) {
        // Fondo negro seguro si no hay assets cargados
        for (int i = 0; i < MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT; ++i) fb[i] = 0xFF000000;
        return;
    }

    const uint8_t* chr_bank = game->assets->active_chr_bank;
    const uint8_t* bg_table = chr_bank + 4096;

    // 1. Decodificar geometría 4-tier de la Sala 0 con sus cadenas auténticas
    uint8_t nametable[30][32];
    uint8_t subpalettes[30][32];
    MjkRoom_BuildNametable(0, nametable, subpalettes);

    // 2. Cargar paleta de Sala 0 (MainPalette)
    uint8_t nes_pal[32];
    uint32_t pal_argb[32];
    MjkRoom_LoadAuthenticPalettes(0, nes_pal);
    MjkPPU_LoadPaletteARGB(nes_pal, pal_argb);

    // 3. Volcar nametable de fondo al framebuffer (256x240)
    for (int ty = 0; ty < 30; ++ty) {
        for (int tx = 0; tx < 32; ++tx) {
            uint8_t t_id = nametable[ty][tx];
            uint8_t sub = subpalettes[ty][tx];
            const uint32_t* sub_colors = &pal_argb[sub * 4];
            MjkPPU_BlitTile(fb, tx * 8, ty * 8, bg_table + (size_t)t_id * 16, sub_colors, false, false, false);
        }
    }

    // 4. Sprites de la Pantalla de Título:
    // a) Jack en Pantalla de Título (prg.asm:7597-7601 y 7636):
    // Posición ancla ($28=40, $78=120). Estado espera = Sprite 0, Caída hacia pirámide = Sprite 3.
    int jx = 40;
    int jy = 120;
    if (game->title_screen_state == TITLE_STATE_5_JACK_FALLING) {
        jx = FIXED_TO_INT(game->title_jack_x);
        jy = FIXED_TO_INT(game->title_jack_y);
    }

    uint8_t title_spr = (game->title_screen_state == TITLE_STATE_5_JACK_FALLING) ? 3 : 0;
    MjkOamSprite wing_sprites[4];
    Jack_AssembleMetasprite(title_spr, (int16_t)jx, (int16_t)jy, false, wing_sprites);
    for (int i = 0; i < 4; ++i) {
        MjkPPU_BlitSprite(fb, chr_bank, &wing_sprites[i], &pal_argb[16]);
    }

    // b) Dígito de High GDV en X=200, Y=24 (Subpaleta 0: blanco con borde)
    uint8_t low_digit = game->gdv_value & 0x0F;
    MjkPPU_BlitTile(fb, 200, 24, chr_bank + (size_t)low_digit * 16, &pal_argb[0], true, false, false);

    uint8_t hi_digit = (game->gdv_value >> 4) & 0x0F;
    if (hi_digit != 0) {
        MjkPPU_BlitTile(fb, 192, 24, chr_bank + (size_t)hi_digit * 16, &pal_argb[0], true, false, false);
    }
}

static void BlitCanonicalRoomDoors(const MjkGameState* game, uint8_t room_id,
                                   uint8_t nametable[30][32], uint8_t subpalettes[30][32]) {
    if (!game || room_id == 0 || room_id >= MJK_TOTAL_ROOMS_COUNT) return;
    const uint8_t* rdata = g_room_data_table[room_id];
    if (!rdata) return;

    int slot_idx = MjkRoom_FindSlot(&game->room_data_ram, room_id);
    const uint8_t* ram_raw = NULL;
    if (slot_idx >= 0) {
        ram_raw = (slot_idx < MJK_ROOMDATA_SECTION_SLOTS)
                ? game->room_data_ram.slots[slot_idx].raw
                : game->room_data_ram.scratch.raw;
    }

    size_t rom_pos = 0;
    uint8_t byte_AC = 0;
    uint8_t ram_obj_idx = 1;

    while (rom_pos < 512) {
        uint8_t b = rdata[rom_pos];
        if (b == 0xFF) break;
        if ((b & 0xF0) == 0xE0) {
            rom_pos += 3;
            continue;
        }
        if (byte_AC != 0) {
            byte_AC--;
            rom_pos++;
            ram_obj_idx++;
            continue;
        }

        rom_pos++;
        uint8_t hi = (uint8_t)(b >> 4);
        uint8_t lo = (uint8_t)(b & 0x0F);
        if (hi < 0x0B) ram_obj_idx++;

        if (hi == 0) {
            byte_AC = (lo == 0) ? 0 : (uint8_t)(lo - 1);
            if (room_id >= 0x90 && room_id < 0xA0) byte_AC = 0x18;
            rom_pos++;
        } else if (hi == 8) {
            uint8_t door_arg = rdata[rom_pos++];
            uint8_t state = g_door_state_tile_table[door_arg & 7];
            if (ram_raw && ram_obj_idx < MJK_ROOMDATA_SLOT_SIZE && ram_raw[ram_obj_idx] != 0 && ram_raw[ram_obj_idx] != 0xFF) {
                state = ram_raw[ram_obj_idx];
            }
            if (room_id == game->current_room && game->door_is_open &&
                lo == (game->entry_door_type & 0x0F)) {
                state |= MJK_OBJ_STATE_DOOR_OPEN;
            }

            if (lo < 16) {
                const MjkDoorPositionTile* dpos = &g_door_position_tile_table[lo];
                uint8_t base_tile = dpos->closed_tile;
                uint8_t y_offset = 0;
                if (state & MJK_OBJ_STATE_DOOR_OPEN) {
                    y_offset = 2;
                } else if (state & 0x80) {
                    base_tile = dpos->open_tile;
                }
                uint8_t m_ids[2] = { (uint8_t)(base_tile + y_offset), (uint8_t)(base_tile + y_offset + 1) };
                uint8_t coords[2] = { dpos->y_tile, dpos->x_tile };

                for (int k = 0; k < 2; ++k) {
                    uint8_t my = (uint8_t)(coords[k] >> 4);
                    uint8_t mx = (uint8_t)(coords[k] & 0x0F);
                    uint8_t m_id = m_ids[k];
                    if (my < 15 && mx < 16 && m_id < MJK_METATILES_COUNT) {
                        MjkMetatileDef def = g_metatile_definitions[m_id];
                        uint8_t sub = MjkRoomData_GetMetatileSubpalette(m_id);
                        nametable[my * 2][mx * 2]         = def.tl; subpalettes[my * 2][mx * 2]         = sub;
                        nametable[my * 2][mx * 2 + 1]     = def.tr; subpalettes[my * 2][mx * 2 + 1]     = sub;
                        nametable[my * 2 + 1][mx * 2]     = def.bl; subpalettes[my * 2 + 1][mx * 2]     = sub;
                        nametable[my * 2 + 1][mx * 2 + 1] = def.br; subpalettes[my * 2 + 1][mx * 2 + 1] = sub;
                    }
                }
            }
        } else {
            rom_pos++;
            if (lo == 6 && (hi == 1 || hi == 2 || hi == 3 || hi == 4 || hi == 9 || hi == 0x0A)) {
                rom_pos++;
            }
        }
    }
}

/**
 * @brief Mapea un valor de puntuación a los tiles canónicos de metasprite OAM (RES-034).
 * Pattern Table 0: tiles 0x30..0x3F, renderizado con Subpaleta Sprite 0 (blanco brillante).
 */
static void MjkFloatingScore_GetTiles(uint32_t value, uint8_t *out_tl, uint8_t *out_tr, uint8_t *out_br) {
    if (!out_tl || !out_tr || !out_br) return;

    *out_tl = 0x31;
    *out_tr = 0x30;
    *out_br = 0x0F;

    switch (value) {
        case 10:    *out_tl = 0x0F; *out_tr = 0x36; *out_br = 0x0F; break; // "10"
        case 20:    *out_tl = 0x0F; *out_tr = 0x38; *out_br = 0x0F; break; // "20"
        case 30:    *out_tl = 0x0F; *out_tr = 0x39; *out_br = 0x0F; break; // "30"
        case 50:    *out_tl = 0x0F; *out_tr = 0x3A; *out_br = 0x0F; break; // "50"
        case 100:   *out_tl = 0x31; *out_tr = 0x30; *out_br = 0x0F; break; // " 100"
        case 200:   *out_tl = 0x32; *out_tr = 0x30; *out_br = 0x0F; break; // " 200"
        case 300:   *out_tl = 0x33; *out_tr = 0x30; *out_br = 0x0F; break; // " 300"
        case 400:   *out_tl = 0x32; *out_tr = 0x30; *out_br = 0x3C; break; // " 200" + "x2"
        case 500:   *out_tl = 0x34; *out_tr = 0x30; *out_br = 0x0F; break; // " 500"
        case 600:   *out_tl = 0x32; *out_tr = 0x30; *out_br = 0x3D; break; // " 200" + "x3"
        case 700:   *out_tl = 0x34; *out_tr = 0x30; *out_br = 0x3C; break; // Fallback adaptativo
        case 800:   *out_tl = 0x35; *out_tr = 0x30; *out_br = 0x0F; break; // " 800"
        case 1000:  *out_tl = 0x36; *out_tr = 0x30; *out_br = 0x0F; break; // "1000"
        case 1200:  *out_tl = 0x37; *out_tr = 0x30; *out_br = 0x0F; break; // "1200"
        case 1500:  *out_tl = 0x33; *out_tr = 0x30; *out_br = 0x3F; break; // " 300" + "x5"
        case 2000:  *out_tl = 0x38; *out_tr = 0x30; *out_br = 0x0F; break; // "2000"
        case 2400:  *out_tl = 0x37; *out_tr = 0x30; *out_br = 0x3C; break; // "1200" + "x2"
        case 3000:  *out_tl = 0x39; *out_tr = 0x30; *out_br = 0x0F; break; // "3000"
        case 4000:  *out_tl = 0x38; *out_tr = 0x30; *out_br = 0x3C; break; // "2000" + "x2"
        case 5000:  *out_tl = 0x3A; *out_tr = 0x30; *out_br = 0x0F; break; // "5000"
        case 6000:  *out_tl = 0x38; *out_tr = 0x30; *out_br = 0x3D; break; // "2000" + "x3"
        case 8000:  *out_tl = 0x38; *out_tr = 0x30; *out_br = 0x3E; break; // "2000" + "x4"
        case 10000: *out_tl = 0x31; *out_tr = 0x30; *out_br = 0x0F; break; // " 100" (o base 10k)
        case 20000: *out_tl = 0x32; *out_tr = 0x30; *out_br = 0x0F; break; // " 200" (o base 20k)
        case 30000: *out_tl = 0x33; *out_tr = 0x30; *out_br = 0x0F; break; // " 300" (o base 30k)
        case 40000: *out_tl = 0x31; *out_tr = 0x30; *out_br = 0x3E; break; // " 100" + "x4"
        case 50000: *out_tl = 0x3A; *out_tr = 0x30; *out_br = 0x0F; break; // "5000" (o base 50k)
        default: {
            if (value >= 10000) {
                uint8_t d = (uint8_t)(value / 10000);
                if (d == 1)      { *out_tl = 0x31; *out_br = 0x0F; }
                else if (d == 2) { *out_tl = 0x32; *out_br = 0x0F; }
                else if (d == 3) { *out_tl = 0x33; *out_br = 0x0F; }
                else if (d == 4) { *out_tl = 0x31; *out_br = 0x3E; }
                else if (d == 5) { *out_tl = 0x3A; *out_br = 0x0F; }
                else             { *out_tl = 0x31; *out_br = 0x0F; }
                *out_tr = 0x30;
            } else if (value >= 1000) {
                uint8_t th = (uint8_t)(value / 1000);
                if (th == 1)      *out_tl = 0x36;
                else if (th == 2) *out_tl = 0x38;
                else if (th == 3) *out_tl = 0x39;
                else if (th == 5) *out_tl = 0x3A;
                else              *out_tl = 0x36;
                *out_tr = 0x30;
                *out_br = 0x0F;
            } else if (value >= 100) {
                uint8_t h = (uint8_t)(value / 100);
                if (h == 1)      { *out_tl = 0x31; *out_br = 0x0F; }
                else if (h == 2) { *out_tl = 0x32; *out_br = 0x0F; }
                else if (h == 3) { *out_tl = 0x33; *out_br = 0x0F; }
                else if (h == 4) { *out_tl = 0x32; *out_br = 0x3C; }
                else if (h == 5) { *out_tl = 0x34; *out_br = 0x0F; }
                else if (h == 6) { *out_tl = 0x32; *out_br = 0x3D; }
                else if (h == 8) { *out_tl = 0x35; *out_br = 0x0F; }
                else             { *out_tl = 0x31; *out_br = 0x0F; }
                *out_tr = 0x30;
            } else {
                *out_tl = 0x0F;
                *out_tr = 0x36;
                *out_br = 0x0F;
            }
            break;
        }
    }
}

static void BlitMetasprite16x16(uint32_t* fb, const uint8_t* chr_bank, int ex, int ey,
                                uint8_t tl, uint8_t tr, uint8_t bl, uint8_t br,
                                const uint32_t* subpal, bool flip_h, bool flip_v) {
    if (ex <= -16 || ex >= MJK_SCREEN_WIDTH || ey <= -16 || ey >= MJK_SCREEN_HEIGHT) return;

    uint8_t q_tl = tl, q_tr = tr, q_bl = bl, q_br = br;

    if (flip_h) {
        // Intercambio horizontal: TL <-> TR, BL <-> BR
        uint8_t tmp = q_tl; q_tl = q_tr; q_tr = tmp;
        tmp = q_bl; q_bl = q_br; q_br = tmp;
    }
    if (flip_v) {
        // Intercambio vertical: TL <-> BL, TR <-> BR
        uint8_t tmp = q_tl; q_tl = q_bl; q_bl = tmp;
        tmp = q_tr; q_tr = q_br; q_br = tmp;
    }

    MjkPPU_BlitTile(fb, ex,     ey,     chr_bank + (size_t)q_tl * 16, subpal, true, flip_h, flip_v);
    MjkPPU_BlitTile(fb, ex + 8, ey,     chr_bank + (size_t)q_tr * 16, subpal, true, flip_h, flip_v);
    MjkPPU_BlitTile(fb, ex,     ey + 8, chr_bank + (size_t)q_bl * 16, subpal, true, flip_h, flip_v);
    MjkPPU_BlitTile(fb, ex + 8, ey + 8, chr_bank + (size_t)q_br * 16, subpal, true, flip_h, flip_v);
}

static void RenderInGameScreen(MjkGameState* game, uint32_t* fb) {
    if (!fb) return;
    if (!game->assets || !game->assets->active_chr_bank) {
        for (int i = 0; i < MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT; ++i) fb[i] = 0xFF000000;
        return;
    }

    const uint8_t* chr_bank = game->assets->active_chr_bank;
    const uint8_t* bg_table = chr_bank + 4096;

    // Estado 1: ROUND INTRO (Fondo negro puro $0F, Jack + 'x' + vidas BCD en el centro)
    if (game->current_state == GAME_STATE_1_ROUND_INTRO) {
        uint8_t nes_pal[32];
        uint32_t pal_argb[32];
        MjkRoom_LoadAuthenticPalettes(1, nes_pal);
        MjkPPU_LoadPaletteARGB(nes_pal, pal_argb);

        // Fondo negro puro ($0F: RGB 0, 0, 0)
        for (int i = 0; i < MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT; ++i) {
            fb[i] = 0xFF000000;
        }

        // Sprites centrados (Slot OAM $0280..$0298 en NES):
        // 1. Jack saludando/idle en el centro: ancla (112, 96) (prg.asm:7686-7688)
        MjkOamSprite intro_jack[4];
        Jack_AssembleMetasprite(0, 112, 96, false, intro_jack);
        for (int i = 0; i < 4; ++i) {
            MjkPPU_BlitSprite(fb, chr_bank, &intro_jack[i], &pal_argb[16]);
        }

        // 2. Glifo 'x' (tile 0x0C) en X=128, Y=92
        MjkOamSprite x_sprite = { 92, 0x0C, 0x00, 128 };
        MjkPPU_BlitSprite(fb, chr_bank, &x_sprite, &pal_argb[16]);

        // 3. Vidas BCD (tile 0x03) en X=144, Y=92
        uint8_t lives_tile = (uint8_t)(game->lives_count & 0x0F);
        MjkOamSprite lives_sprite = { 92, lives_tile, 0x00, 144 };
        MjkPPU_BlitSprite(fb, chr_bank, &lives_sprite, &pal_argb[16]);

        return;
    }

    // 1. Decodificar geometría 4-tier de la sala actual (Round 1 = Sala 1 Mazmorra Verde)
    uint8_t room_id = game->current_room ? game->current_room : 1;
    uint8_t nametable[30][32];
    uint8_t subpalettes[30][32];
    MjkRoom_BuildNametable(room_id, nametable, subpalettes);

    // 2. Cargar paleta auténtica (Set 0 para Sala 1, unificando backdrop y preservando HUD)
    uint8_t nes_pal[32];
    uint32_t pal_argb[32];
    MjkRoom_LoadAuthenticPalettes(room_id, nes_pal);

    // Aplicar color dinámico Mighty a la Subpaleta de Sprite 1 de Jack (data.asm:306-307 y prg.asm:1774-1779)
    uint8_t lvl = game->jack.mighty_level;
    if (game->jack.power_level > lvl) lvl = (uint8_t)game->jack.power_level;
    if (lvl > 3) lvl = 3;
    nes_pal[21] = 0x30;
    nes_pal[22] = 0x12;
    nes_pal[23] = mjk_player_get_palette_color(lvl);

    MjkPPU_LoadPaletteARGB(nes_pal, pal_argb);
    memcpy(game->palette_rgba, pal_argb, sizeof(game->palette_rgba));

    // 3. Colocar Bombas esféricas rojas ($27 -> tiles { $E0, $E1, $E2, $E3 }) de la sala activa (TASK-F6-022)
    for (uint8_t b = 0; b < game->bomb_count; ++b) {
        if (!game->bombs[b].collected && (game->bombs[b].room_id == room_id || game->bombs[b].room_id == 0)) {
            int by = game->bombs[b].tile_y;
            int bx = game->bombs[b].tile_x;
            if (by < 15 && bx < 16) {
                nametable[by * 2][bx * 2]         = 0xE0; subpalettes[by * 2][bx * 2]         = 3;
                nametable[by * 2][bx * 2 + 1]     = 0xE1; subpalettes[by * 2][bx * 2 + 1]     = 3;
                nametable[by * 2 + 1][bx * 2]     = 0xE2; subpalettes[by * 2 + 1][bx * 2]     = 3;
                nametable[by * 2 + 1][bx * 2 + 1] = 0xE3; subpalettes[by * 2 + 1][bx * 2 + 1] = 3;
            }
        }
    }

    // 3b. Colocar Cofres del Tesoro de la sala activa (TASK-F6-023)
    for (uint8_t c = 0; c < game->chest_count; ++c) {
        if (game->chests[c].room_id == room_id || game->chests[c].room_id == 0) {
            if (game->chests[c].is_hidden) continue;
            if (game->chests[c].collected) continue;
            int cy = game->chests[c].tile_y;
            int cx = game->chests[c].tile_x;
            if (cy < 15 && cx < 16) {
                uint8_t tl, tr, bl, br;
                uint8_t chest_mt = (game->chests[c].chest_type == 1) ? 0x4A : 0x28;
                uint8_t sub = MjkRoomData_GetMetatileSubpalette(chest_mt);
                if (!game->chests[c].is_open) {
                    if (game->chests[c].chest_type == 1) {
                        tl = 0x99; tr = 0x9B; bl = 0xEA; br = 0xEB; // Naranja cerrado ($4A)
                    } else {
                        tl = 0xE8; tr = 0xE9; bl = 0xEA; br = 0xEB; // Rojo cerrado ($28)
                    }
                } else {
                    uint8_t m_id = g_item_to_tile_table[game->chests[c].item_id & 0x1F];
                    MjkMetatileDef def = (m_id < MJK_METATILES_COUNT) ? g_metatile_definitions[m_id] : (MjkMetatileDef){0x24, 0x24, 0x24, 0x24};
                    tl = def.tl; tr = def.tr; bl = def.bl; br = def.br;
                    sub = MjkRoomData_GetMetatileSubpalette(m_id);
                }
                nametable[cy * 2][cx * 2]         = tl; subpalettes[cy * 2][cx * 2]         = sub;
                nametable[cy * 2][cx * 2 + 1]     = tr; subpalettes[cy * 2][cx * 2 + 1]     = sub;
                nametable[cy * 2 + 1][cx * 2]     = bl; subpalettes[cy * 2 + 1][cx * 2]     = sub;
                nametable[cy * 2 + 1][cx * 2 + 1] = br; subpalettes[cy * 2 + 1][cx * 2 + 1] = sub;
            }
        }
    }

    // 3b2. Colocar Bloques Secretos revelados de la sala activa ($6N)
    for (uint8_t s = 0; s < game->secret_block_count; ++s) {
        if ((game->secret_blocks[s].room_id == room_id || game->secret_blocks[s].room_id == 0) && game->secret_blocks[s].revealed) {
            int sy = game->secret_blocks[s].tile_y;
            int sx = game->secret_blocks[s].tile_x;
            if (sy < 15 && sx < 16) {
                nametable[sy * 2][sx * 2]         = 0x8A; subpalettes[sy * 2][sx * 2]         = 0;
                nametable[sy * 2][sx * 2 + 1]     = 0x8A; subpalettes[sy * 2][sx * 2 + 1]     = 0;
                nametable[sy * 2 + 1][sx * 2]     = 0x8A; subpalettes[sy * 2 + 1][sx * 2]     = 0;
                nametable[sy * 2 + 1][sx * 2 + 1] = 0x8A; subpalettes[sy * 2 + 1][sx * 2 + 1] = 0;
            }
        }
    }

    // 3c. Superponer puertas canónicas reales de la sala (RES-029)
    BlitCanonicalRoomDoors(game, room_id, nametable, subpalettes);

    // 4. Volcar fondo a framebuffer considerando scroll continuo y costura multi-sala (RES-019B)
    for (int ty = 0; ty < 30; ++ty) {
        for (int tx = 0; tx < 32; ++tx) {
            int px = tx * 8 - game->camera_x;
            int py = ty * 8 - game->camera_y;
            if (px <= -8 || px >= MJK_SCREEN_WIDTH || py <= -8 || py >= MJK_SCREEN_HEIGHT) continue;
            uint8_t t_id = nametable[ty][tx];
            uint8_t sub = subpalettes[ty][tx];
            const uint32_t* sub_colors = &pal_argb[sub * 4];
            MjkPPU_BlitTile(fb, px, py, bg_table + (size_t)t_id * 16, sub_colors, false, false, false);
        }
    }

    // Costura continua de sala adyacente (Seam Parity):
    if (room_id < MJK_TOTAL_ROOMS_COUNT) {
        const MjkAdjacentRoom* adj = &g_adjacent_rooms[room_id];

        // Costura horizontal derecha (CurrentRoomID -> RightRoom)
        if (game->camera_x > 0 && adj->right != 0) {
            uint8_t right_nametable[30][32];
            uint8_t right_subpalettes[30][32];
            MjkRoom_BuildNametable(adj->right, right_nametable, right_subpalettes);

            // Proyección continua de bombas y cofres de la sala contigua (TASK-F6-029 / RES-020)
            for (uint8_t b = 0; b < game->bomb_count; ++b) {
                if (game->bombs[b].room_id == adj->right && !game->bombs[b].collected) {
                    int by = game->bombs[b].tile_y;
                    int bx = game->bombs[b].tile_x;
                    if (by < 15 && bx < 16) {
                        right_nametable[by * 2][bx * 2]         = 0xE0; right_subpalettes[by * 2][bx * 2]         = 3;
                        right_nametable[by * 2][bx * 2 + 1]     = 0xE1; right_subpalettes[by * 2][bx * 2 + 1]     = 3;
                        right_nametable[by * 2 + 1][bx * 2]     = 0xE2; right_subpalettes[by * 2 + 1][bx * 2]     = 3;
                        right_nametable[by * 2 + 1][bx * 2 + 1] = 0xE3; right_subpalettes[by * 2 + 1][bx * 2 + 1] = 3;
                    }
                }
            }

            for (uint8_t c = 0; c < game->chest_count; ++c) {
                if (game->chests[c].room_id == adj->right) {
                    if (game->chests[c].is_hidden || game->chests[c].collected) continue;
                    int cy = game->chests[c].tile_y;
                    int cx = game->chests[c].tile_x;
                    if (cy < 15 && cx < 16) {
                        uint8_t tl, tr, bl, br;
                        uint8_t chest_mt = (game->chests[c].chest_type == 1) ? 0x4A : 0x28;
                        uint8_t sub = MjkRoomData_GetMetatileSubpalette(chest_mt);
                        if (!game->chests[c].is_open) {
                            if (game->chests[c].chest_type == 1) {
                                tl = 0x99; tr = 0x9B; bl = 0xEA; br = 0xEB;
                            } else {
                                tl = 0xE8; tr = 0xE9; bl = 0xEA; br = 0xEB;
                            }
                        } else {
                            uint8_t m_id = g_item_to_tile_table[game->chests[c].item_id & 0x1F];
                            MjkMetatileDef def = (m_id < MJK_METATILES_COUNT) ? g_metatile_definitions[m_id] : (MjkMetatileDef){0x24, 0x24, 0x24, 0x24};
                            tl = def.tl; tr = def.tr; bl = def.bl; br = def.br;
                            sub = MjkRoomData_GetMetatileSubpalette(m_id);
                        }
                        right_nametable[cy * 2][cx * 2]         = tl; right_subpalettes[cy * 2][cx * 2]         = sub;
                        right_nametable[cy * 2][cx * 2 + 1]     = tr; right_subpalettes[cy * 2][cx * 2 + 1]     = sub;
                        right_nametable[cy * 2 + 1][cx * 2]     = bl; right_subpalettes[cy * 2 + 1][cx * 2]     = sub;
                        right_nametable[cy * 2 + 1][cx * 2 + 1] = br; right_subpalettes[cy * 2 + 1][cx * 2 + 1] = sub;
                    }
                }
            }

            for (uint8_t s = 0; s < game->secret_block_count; ++s) {
                if (game->secret_blocks[s].room_id == adj->right && game->secret_blocks[s].revealed) {
                    int sy = game->secret_blocks[s].tile_y;
                    int sx = game->secret_blocks[s].tile_x;
                    if (sy < 15 && sx < 16) {
                        right_nametable[sy * 2][sx * 2]         = 0x8A; right_subpalettes[sy * 2][sx * 2]         = 0;
                        right_nametable[sy * 2][sx * 2 + 1]     = 0x8A; right_subpalettes[sy * 2][sx * 2 + 1]     = 0;
                        right_nametable[sy * 2 + 1][sx * 2]     = 0x8A; right_subpalettes[sy * 2 + 1][sx * 2]     = 0;
                        right_nametable[sy * 2 + 1][sx * 2 + 1] = 0x8A; right_subpalettes[sy * 2 + 1][sx * 2 + 1] = 0;
                    }
                }
            }

            BlitCanonicalRoomDoors(game, adj->right, right_nametable, right_subpalettes);

            for (int ty = 0; ty < 30; ++ty) {
                for (int tx = 0; tx < 32; ++tx) {
                    int px = (256 - game->camera_x) + tx * 8;
                    if (px <= -8 || px >= MJK_SCREEN_WIDTH) continue;
                    uint8_t t_id = right_nametable[ty][tx];
                    uint8_t sub = right_subpalettes[ty][tx];
                    const uint32_t* sub_colors = &pal_argb[sub * 4];
                    MjkPPU_BlitTile(fb, px, ty * 8, bg_table + (size_t)t_id * 16, sub_colors, false, false, false);
                }
            }
        } else if (game->camera_x < 0 && adj->left != 0) {
            // Costura horizontal izquierda (CurrentRoomID -> LeftRoom)
            uint8_t left_nametable[30][32];
            uint8_t left_subpalettes[30][32];
            MjkRoom_BuildNametable(adj->left, left_nametable, left_subpalettes);
            BlitCanonicalRoomDoors(game, adj->left, left_nametable, left_subpalettes);

            for (int ty = 0; ty < 30; ++ty) {
                for (int tx = 0; tx < 32; ++tx) {
                    int px = (-256 - game->camera_x) + tx * 8;
                    if (px <= -8 || px >= MJK_SCREEN_WIDTH) continue;
                    uint8_t t_id = left_nametable[ty][tx];
                    uint8_t sub = left_subpalettes[ty][tx];
                    const uint32_t* sub_colors = &pal_argb[sub * 4];
                    MjkPPU_BlitTile(fb, px, ty * 8, bg_table + (size_t)t_id * 16, sub_colors, false, false, false);
                }
            }
        }

        // Costura vertical inferior (CurrentRoomID -> DownRoom)
        if (game->camera_y > 0 && adj->down != 0) {
            uint8_t down_nametable[30][32];
            uint8_t down_subpalettes[30][32];
            MjkRoom_BuildNametable(adj->down, down_nametable, down_subpalettes);

            for (int ty = 0; ty < 30; ++ty) {
                for (int tx = 0; tx < 32; ++tx) {
                    int py = (240 - game->camera_y) + ty * 8;
                    if (py <= -8 || py >= MJK_SCREEN_HEIGHT) continue;
                    uint8_t t_id = down_nametable[ty][tx];
                    uint8_t sub = down_subpalettes[ty][tx];
                    const uint32_t* sub_colors = &pal_argb[sub * 4];
                    MjkPPU_BlitTile(fb, tx * 8, py, bg_table + (size_t)t_id * 16, sub_colors, false, false, false);
                }
            }
        } else if (game->camera_y < 0 && adj->up != 0) {
            // Costura vertical superior (CurrentRoomID -> UpRoom)
            uint8_t up_nametable[30][32];
            uint8_t up_subpalettes[30][32];
            MjkRoom_BuildNametable(adj->up, up_nametable, up_subpalettes);

            for (int ty = 0; ty < 30; ++ty) {
                for (int tx = 0; tx < 32; ++tx) {
                    int py = (-240 - game->camera_y) + ty * 8;
                    if (py <= -8 || py >= MJK_SCREEN_HEIGHT) continue;
                    uint8_t t_id = up_nametable[ty][tx];
                    uint8_t sub = up_subpalettes[ty][tx];
                    const uint32_t* sub_colors = &pal_argb[sub * 4];
                    MjkPPU_BlitTile(fb, tx * 8, py, bg_table + (size_t)t_id * 16, sub_colors, false, false, false);
                }
            }
        }
    }

    // 5. Superponer HUD Oficial NES (Subpaleta 0 con transparent_zero = true):
    // a) 'T' (tile $1D) en X=56, Y=16
    MjkPPU_BlitTile(fb, MJK_HUD_TIMER_ICON_X, MJK_HUD_TIMER_ICON_Y, bg_table + (size_t)0x1D * 16, &pal_argb[0], true, false, false);

    // b) Temporizador BCD de 2 dígitos en X=64, 72, Y=16 (fuente de dígitos en chr_bank)
    uint16_t timer = game->stage_timer_seconds;
    uint8_t t_tens = (uint8_t)((timer / 10) % 10);
    uint8_t t_units = (uint8_t)(timer % 10);
    MjkPPU_BlitTile(fb, MJK_HUD_TIMER_TENS_X, MJK_HUD_TIMER_Y, chr_bank + (size_t)t_tens * 16, &pal_argb[0], true, false, false);
    MjkPPU_BlitTile(fb, MJK_HUD_TIMER_UNITS_X, MJK_HUD_TIMER_Y, chr_bank + (size_t)t_units * 16, &pal_argb[0], true, false, false);

    // c) Puntuación BCD de 8 dígitos en X=88..144 (paso de 8px), Y=16 con supresión de ceros no significativos
    uint8_t digits[8];
    digits[0] = (game->score_bcd[3] >> 4) & 0x0F;
    digits[1] = game->score_bcd[3] & 0x0F;
    digits[2] = (game->score_bcd[2] >> 4) & 0x0F;
    digits[3] = game->score_bcd[2] & 0x0F;
    digits[4] = (game->score_bcd[1] >> 4) & 0x0F;
    digits[5] = game->score_bcd[1] & 0x0F;
    digits[6] = (game->score_bcd[0] >> 4) & 0x0F;
    digits[7] = game->score_bcd[0] & 0x0F;

    bool leading = true;
    for (int i = 0; i < 8; ++i) {
        uint8_t d = digits[i];
        if (i < 7 && leading && d == 0) {
            continue; // Cero no significativo: no se dibuja (permanece fondo)
        }
        leading = false;
        MjkPPU_BlitTile(fb, MJK_HUD_SCORE_X_START + i * MJK_HUD_SCORE_STEP_X, MJK_HUD_SCORE_Y, chr_bank + (size_t)d * 16, &pal_argb[0], true, false, false);
    }

    // d) Icono 'M' (tile $16) en X=184, Y=16
    MjkPPU_BlitTile(fb, MJK_HUD_MIGHTY_ICON_X, MJK_HUD_MIGHTY_ICON_Y, bg_table + (size_t)0x16 * 16, &pal_argb[0], true, false, false);

    // e) Monedas Mighty en X=192, Y=16
    uint8_t coins = (uint8_t)(game->mighty_coins % 10);
    MjkPPU_BlitTile(fb, MJK_HUD_MIGHTY_COINS_X, MJK_HUD_MIGHTY_COINS_Y, chr_bank + (size_t)coins * 16, &pal_argb[0], true, false, false);

    // 6. Volcar Sprites OAM de Jack (TASK-F6-016: Ground Truth OAM y Subpaleta 1)
    // En GAME_STATE_9_EXIT_ROOM, ClearFourSpritesInY oculta a Jack del OAM
    // NOTA CANÓNICA: Jack reside en coordenadas de pantalla (RES-019, DrawPlayer en prg.asm:1640)
    if ((game->jack.is_alive || game->current_state == GAME_STATE_5_MAIN_LOOP) &&
        game->current_state != GAME_STATE_9_EXIT_ROOM) {
        int jx = FIXED_TO_INT(game->jack.pos_x);
        int jy = FIXED_TO_INT(game->jack.pos_y);

        // Determinación del Sprite canónico según estado cinemático y vital (prg.asm:1604-1635)
        uint8_t spr_id = Jack_DetermineSprite(&game->jack, game->total_frames_rendered);
        game->jack.sprite_id = spr_id;

        MjkOamSprite jack_sp[4];
        // En NES (DrawPlayer prg.asm:1640), el ancla es el centro del metasprite (jx+8, jy+8)
        Jack_AssembleMetasprite(spr_id, (int16_t)(jx + 8), (int16_t)(jy + 8), game->jack.facing_left, jack_sp);

        for (int i = 0; i < 4; ++i) {
            // El blitter recibe &pal_argb[16] (base de paletas de sprites).
            // Dado que jack_sp[i].attributes tiene el bit 0 activo (subpaleta 1),
            // el blitter leerá de &pal_argb[16 + 1 * 4] = &pal_argb[20] (Subpaleta de Sprite 1 de Jack).
            MjkPPU_BlitSprite(fb, chr_bank, &jack_sp[i], &pal_argb[16]);
        }
    }

    // 7. Chispa de mecha (OAM #4, tile $0D, paleta 2):
    // FUENTE DE VERDAD: RES-022 (asm/src/prg.asm:5369-5388).
    // Posición Y-4, X+8, Attr 0x12, alterna tile 0x0D <-> 0x0F cada 3 frames.
    if ((game->bomb_fuse.spark_visible || (game->is_bomb_room && game->bomb_fuse.spark_sprite_y != 0xF8)) &&
        game->active_fire_bomb_index < game->bomb_count &&
        !game->bombs[game->active_fire_bomb_index].collected) {
        int sx = game->bombs[game->active_fire_bomb_index].tile_x * 16 + 8 - game->camera_x;
        int sy = game->bombs[game->active_fire_bomb_index].tile_y * 16 - 4;
        uint8_t spark_tile = ((game->total_frames_rendered / 3) % 2 == 0) ? 0x0D : 0x0F;
        MjkOamSprite spark_sp = {
            (uint8_t)sy,  // Y = -4 px sobre el tope de la bomba
            spark_tile,   // Alterna entre $0D y $0F cada 3 frames
            0x12,         // Atributos canónicos 0x12 (Subpaleta 2, prioridad sobre fondo)
            (uint8_t)sx   // X = +8 px centrado horizontalmente
        };
        MjkPPU_BlitSprite(fb, chr_bank, &spark_sp, &pal_argb[16]);
    }

    // 7.5 Efecto Canónico de Humo y Puntuación al Recoger Bombas (TASK-F7-009 / RES-036)
    // FUENTE DE VERDAD: prg.asm:5671-5746, data.asm:402 (Metasprite $25)
    for (int p = 0; p < MJK_MAX_BOMB_PUFFS; ++p) {
        const MjkBombPuffEffect* puff = &game->bomb_puffs[p];
        if (!puff->active) continue;

        int px = puff->x;
        int py = puff->y;
        if (px < -16 || px >= MJK_SCREEN_WIDTH + 16 || py < -16 || py >= MJK_SCREEN_HEIGHT + 16) continue;

        const uint32_t* sprite_pal0 = &pal_argb[16];

        if (puff->phase == 1) {
            // Fase 1 (32 frames): Metasprite $25 (4 tiles $8C simétricos con máscara 0xE4 y Subpaleta 0)
            const uint8_t* puff_tile = chr_bank + (size_t)0x8C * 16;
            MjkPPU_BlitTile(fb, px - 8, py - 8, puff_tile, sprite_pal0, true, false, false);
            MjkPPU_BlitTile(fb, px,     py - 8, puff_tile, sprite_pal0, true, true,  false);
            MjkPPU_BlitTile(fb, px - 8, py,     puff_tile, sprite_pal0, true, false, true);
            MjkPPU_BlitTile(fb, px,     py,     puff_tile, sprite_pal0, true, true,  true);
        } else if (puff->phase == 2) {
            // Fase 2 (32 frames): Puntuación flotante canónica ("100" o "200")
            uint8_t tl = 0x0F;
            uint8_t tr = 0x30;
            uint8_t br = 0x0F;
            MjkFloatingScore_GetTiles((uint32_t)puff->points, &tl, &tr, &br);

            if (tl != 0x0F) {
                MjkPPU_BlitTile(fb, px - 8, py - 8, chr_bank + (size_t)tl * 16, sprite_pal0, true, false, false);
            }
            if (tr != 0x0F) {
                MjkPPU_BlitTile(fb, px, py - 8, chr_bank + (size_t)tr * 16, sprite_pal0, true, false, false);
            }
            if (br != 0x0F) {
                MjkPPU_BlitTile(fb, px, py, chr_bank + (size_t)br * 16, sprite_pal0, true, false, false);
            }
        }
    }

    // 8. Sprites de Enemigos y Monedas de Transmutación (TASK-F6-025, TASK-F7-010 / RES-028)
    // NOTA CANÓNICA: Entidades residen en coordenadas de pantalla (RES-019, loc_B7E4)
    for (int i = 0; i < MJK_MAX_ENEMIES; ++i) {
        const EnemyEntity* e = &game->enemies[i];
        if (!e->active) continue;
        int ex = FIXED_TO_INT(e->pos_x);
        int ey = FIXED_TO_INT(e->pos_y);
        if (ex <= -16 || ex >= MJK_SCREEN_WIDTH || ey <= -16 || ey >= MJK_SCREEN_HEIGHT) continue;

        // Caso 1: Humo de spawn / mutación (Status & 0x08)
        if (e->status & 0x08) {
            if (e->flash_timer >= 16) {
                // Primeros 16 frames: invisible
                continue;
            }
            // Siguientes 16 frames: nube de humo Metasprite $25 con Paleta 0
            // SpritesTable[$25] = {0xE4, 0x8C, 0x8C, 0x8C, 0x8C}
            const uint32_t* pal0 = &pal_argb[16 + 0 * 4];
            BlitMetasprite16x16(fb, chr_bank, ex, ey, 0x8C, 0x8C, 0x8C, 0x8C, pal0, false, false);
            continue;
        }

        // Caso 2: Moneda transmutada (Status & 0x10 o transmute_timer > 0)
        if ((e->status & 0x10) || game->transmute_timer > 0) {
            // SpritesTable[$26] = {0xE5, 0x7E, 0x7F, 0x8E, 0x8F} con Paleta 1 y parpadeo al final
            uint8_t subpal = 1;
            if (game->transmute_timer < 64) {
                subpal = (uint8_t)((game->transmute_timer / 4) % 4);
            }
            const uint32_t* coin_pal = &pal_argb[16 + subpal * 4];
            BlitMetasprite16x16(fb, chr_bank, ex, ey, 0x7E, 0x7F, 0x8E, 0x8F, coin_pal, false, false);
            continue;
        }

        // Caso 3: Renderizado canónico de los 6 arquetipos y la Momia
        uint8_t tl = 0x24, tr = 0x24, bl = 0x24, br = 0x24;
        uint8_t subpal = 3;
        bool flip_h = !e->facing_left; // H-Flip al mirar a la derecha
        bool flip_v = false;
        uint8_t anim_step = (uint8_t)((e->state_timer / 5) % 4);

        switch (e->type) {
            case ENEMY_TYPE_MUMMY: {
                // Momia: Sprite Pal 3
                subpal = 3;
                if (e->status & 0x02) {
                    // Momia cayendo: Metasprites $10 ($60,$61,$62,$63) y $11 ($64,$65,$66,$67)
                    if (anim_step % 2 == 0) {
                        tl = 0x60; tr = 0x61; bl = 0x62; br = 0x63;
                    } else {
                        tl = 0x64; tr = 0x65; bl = 0x66; br = 0x67;
                    }
                } else {
                    // Momia caminando: Metasprites $0E ($70,$71,$72,$73) y $0F ($74,$75,$76,$77)
                    if (anim_step % 2 == 0) {
                        tl = 0x70; tr = 0x71; bl = 0x72; br = 0x73;
                    } else {
                        tl = 0x74; tr = 0x75; bl = 0x76; br = 0x77;
                    }
                }
                break;
            }

            case ENEMY_TYPE_HANEZO: {
                // Hanezo (Albatros / red bouncy guy): Sprite Pal 2
                // data.asm:37-41: Frame 0 ($13: $58..$5B), Frame 1 ($12: $5C..$5F), Frame 2 ($13 H-Flip), Frame 3 ($12)
                subpal = 2;
                if (anim_step == 0) {
                    tl = 0x58; tr = 0x59; bl = 0x5A; br = 0x5B;
                    flip_h = false;
                } else if (anim_step == 1) {
                    tl = 0x5C; tr = 0x5D; bl = 0x5E; br = 0x5F;
                    flip_h = false;
                } else if (anim_step == 2) {
                    tl = 0x58; tr = 0x59; bl = 0x5A; br = 0x5B;
                    flip_h = true;
                } else {
                    tl = 0x5C; tr = 0x5D; bl = 0x5E; br = 0x5F;
                    flip_h = true;
                }
                break;
            }

            case ENEMY_TYPE_GEJI_SHOGUN: {
                // Geji Shogun (red caterpillar): Sprite Pal 2
                // Metasprites $0C ($C0..$C3) y $0D ($C4..$C7)
                subpal = 2;
                if (anim_step % 2 == 0) {
                    tl = 0xC0; tr = 0xC1; bl = 0xC2; br = 0xC3;
                } else {
                    tl = 0xC4; tr = 0xC5; bl = 0xC6; br = 0xC7;
                }
                break;
            }

            case ENEMY_TYPE_GAMEIDO: {
                // Gameido (green turtle): Sprite Pal 3
                // Metasprites $16 ($A8..$AB), $17 ($AC..$AF), $18 ($B8..$BB)
                subpal = 3;
                uint8_t g_step = (uint8_t)((e->state_timer / 5) % 3);
                if (g_step == 0) {
                    tl = 0xA8; tr = 0xA9; bl = 0xAA; br = 0xAB;
                } else if (g_step == 1) {
                    tl = 0xAC; tr = 0xAD; bl = 0xAE; br = 0xAF;
                } else {
                    tl = 0xB8; tr = 0xB9; bl = 0xBA; br = 0xBB;
                }
                break;
            }

            case ENEMY_TYPE_DOKURON: {
                // Dokuron (green skull): Sprite Pal 3
                // Metasprites $14 ($68..$6B) y $15 ($6C..$6F)
                subpal = 3;
                if (anim_step % 2 == 0) {
                    tl = 0x68; tr = 0x69; bl = 0x6A; br = 0x6B;
                } else {
                    tl = 0x6C; tr = 0x6D; bl = 0x6E; br = 0x6F;
                }
                break;
            }

            case ENEMY_TYPE_DESUFA: {
                // Desufa (red fireball): Sprite Pal 0
                // Metasprite $07 ($48,$49,$4A,$4B) con V-Flip en paso 2 (data.asm:55-58)
                subpal = 0;
                tl = 0x48; tr = 0x49; bl = 0x4A; br = 0x4B;
                flip_h = false;
                flip_v = (anim_step % 2 != 0);
                break;
            }

            case ENEMY_TYPE_HORUS: {
                // Horus (green bird): Sprite Pal 3
                subpal = 3;
                if (e->vel_x == 0 && e->vel_y != 0) {
                    // Frontal: Metasprites $0A ($D0..$D3) y $0B ($D4..$D7)
                    if (anim_step % 2 == 0) {
                        tl = 0xD0; tr = 0xD1; bl = 0xD2; br = 0xD3;
                    } else {
                        tl = 0xD4; tr = 0xD5; bl = 0xD6; br = 0xD7;
                    }
                    flip_h = false;
                } else {
                    // Lateral: Metasprites $08 ($78..$7B) y $09 ($7C..$7F)
                    if (anim_step % 2 == 0) {
                        tl = 0x78; tr = 0x79; bl = 0x7A; br = 0x7B;
                    } else {
                        tl = 0x7C; tr = 0x7D; bl = 0x7E; br = 0x7F;
                    }
                }
                break;
            }

            default: {
                // Fallback a Momia
                subpal = 3;
                tl = 0x70; tr = 0x71; bl = 0x72; br = 0x73;
                break;
            }
        }

        const uint32_t* pal = &pal_argb[16 + subpal * 4];
        BlitMetasprite16x16(fb, chr_bank, ex, ey, tl, tr, bl, br, pal, flip_h, flip_v);
    }

    // 9. Marcadores de puntuación flotante auténticos (TASK-F7-007, RES-034)
    for (int i = 0; i < MJK_MAX_FLOATING_SCORES; ++i) {
        if (!game->floating_scores[i].active) continue;
        int fx = game->floating_scores[i].x;
        int fy = game->floating_scores[i].y;
        if (fx < -16 || fx >= MJK_SCREEN_WIDTH + 16 || fy < -16 || fy >= MJK_SCREEN_HEIGHT + 16) continue;

        uint8_t tl = 0x0F;
        uint8_t tr = 0x30;
        uint8_t br = 0x0F;
        MjkFloatingScore_GetTiles((uint32_t)game->floating_scores[i].value, &tl, &tr, &br);

        const uint32_t* sprite_pal0 = &pal_argb[16];

        if (tl != 0x0F) {
            MjkPPU_BlitTile(fb, fx - 8, fy - 8, chr_bank + (size_t)tl * 16, sprite_pal0, true, false, false);
        }
        if (tr != 0x0F) {
            MjkPPU_BlitTile(fb, fx, fy - 8, chr_bank + (size_t)tr * 16, sprite_pal0, true, false, false);
        }
        if (br != 0x0F) {
            MjkPPU_BlitTile(fb, fx, fy, chr_bank + (size_t)br * 16, sprite_pal0, true, false, false);
        }
    }

    // 10. Aviso de Sala de Tortura (TASK-F6-028)
    if (game->current_state == GAME_STATE_7_TORTURE_ROOM) {
        static const struct { int x, y; const char* text; } s_torture_txt[] = {
            { 80, 80,  "TORTURE ROOM" },
            { 80, 104, "JUMPS LEFT:" }
        };
        for (size_t t = 0; t < sizeof(s_torture_txt)/sizeof(s_torture_txt[0]); ++t) {
            int tx = s_torture_txt[t].x;
            int ty = s_torture_txt[t].y;
            const char* str = s_torture_txt[t].text;
            for (int i = 0; str[i] != '\0'; ++i) {
                char ch = str[i];
                uint8_t t_id = 0x24;
                if (ch >= 'A' && ch <= 'Z') t_id = (uint8_t)(0x0A + (ch - 'A'));
                else if (ch == ':') t_id = 0x2E;
                MjkPPU_BlitTile(fb, tx + i * 8, ty, bg_table + (size_t)t_id * 16, &pal_argb[0], true, false, false);
            }
        }
        uint8_t j_tens = (uint8_t)((game->torture_jumps_left / 10) % 10);
        uint8_t j_units = (uint8_t)(game->torture_jumps_left % 10);
        MjkPPU_BlitTile(fb, 176, 104, chr_bank + (size_t)j_tens * 16, &pal_argb[0], true, false, false);
        MjkPPU_BlitTile(fb, 184, 104, chr_bank + (size_t)j_units * 16, &pal_argb[0], true, false, false);
    }

    // 11. Clipping de los 8 píxeles izquierdos de PPU en pasillos horizontales (RES-019 / PPUMask)
    if (camera_is_left_clipped(&game->camera)) {
        for (int y = 0; y < MJK_SCREEN_HEIGHT; ++y) {
            for (int x = 0; x < 8; ++x) {
                fb[y * MJK_SCREEN_WIDTH + x] = pal_argb[0];
            }
        }
    }
}

void MjkGame_Init(MjkGameState* game, MjkAssetPackage* assets) {
    if (!game) return;
    memset(game, 0, sizeof(MjkGameState));
    game->assets = assets;
    game->in_game = false;
    game->title_screen_state = TITLE_STATE_0_COPYRIGHT_INIT;
    game->copyright_timer = 260;
    game->title_jack_x = FIXED_FROM_INT(40);
    game->title_jack_y = FIXED_FROM_INT(120);
    game->title_jack_vy = 0;
    game->current_state = GAME_STATE_0_INIT;
    game->current_round = 1;
    game->current_room = 0;
    game->lives_count = 3;
    game->stage_timer_seconds = 45;
    Jack_Init(&game->jack, 64, 192);
    MjkAPU_Init(&game->apu);
    {
        MjkRoomContext ctx;
        MjkRoom_Load(&ctx, 1);
        memcpy(game->room_metatiles, ctx.metatiles, sizeof(game->room_metatiles));
    }

    camera_init(&game->camera);
    game->camera_x = 0;
    game->camera_y = 0;
    game->is_horizontal_room = false;

    // Inicializar bombas y puerta de Sala 1 por defecto
    static const struct { uint8_t r, c; } s_init_bombs[] = {
        { 8, 5 }, { 8, 7 }, { 3, 9 }, { 6, 11 }, { 6, 13 }
    };
    game->bomb_count = 5;
    for (uint8_t b = 0; b < game->bomb_count; ++b) {
        game->bombs[b].tile_y = s_init_bombs[b].r;
        game->bombs[b].tile_x = s_init_bombs[b].c;
        game->bombs[b].collected = false;
        game->bombs[b].room_id = 1;
        game->bombs[b].state = (b == 0) ? MJK_BOMB_STATE_STARTER_OR_END : MJK_BOMB_STATE_UNLIT;
    }
    game->active_fire_bomb_index = 0;
    game->door_tile_x = 0xFF;
    game->door_tile_y = 0xFF;
    game->door_condition_id = 0;
    game->door_condition_active = false;
    game->door_is_open = false;

    game->maybe_bomb_thing = 0;
    game->room_bomb_picked_flag = 0;
    game->power_coin_bomb_counter = 0;
    game->power_coin_trigger_flag = 0;
    game->fire_bombs_collected = 0;
    game->fire_bombs_collected_total = 0;
    game->total_bombs_collected = 0;
    game->chain_active = false;
    game->is_bomb_room = false;

    game->bomb_fuse.spark_sprite_y = 0xF8;
    game->bomb_fuse.spark_sprite_tile = 0x0D;
    game->bomb_fuse.spark_sprite_attr = 0x12;
    game->bomb_fuse.spark_sprite_x = 0;
    game->bomb_fuse.spark_anim_timer = 3;
    game->bomb_fuse.spark_visible = false;

    // Inicializar paleta NES por defecto (RGBA32)
    uint8_t nes_pal[32];
    MjkRoom_LoadAuthenticPalettes(0, nes_pal);
    MjkPPU_LoadPaletteARGB(nes_pal, game->palette_rgba);
    MjkGame_UpdateJackPalette(game);
}

void MjkGame_StartNewGame(MjkGameState* game) {
    if (!game) return;
    game->in_game = true;
    game->current_round = 1;
    game->current_room = 1;
    game->lives_count = 3;
    game->stage_timer_seconds = 45;
    Jack_Init(&game->jack, 64, 192);
    MjkGame_UpdateJackPalette(game);
    MjkAPU_Init(&game->apu);
    {
        MjkRoomContext ctx;
        MjkRoom_Load(&ctx, 1);
        memcpy(game->room_metatiles, ctx.metatiles, sizeof(game->room_metatiles));
    }

    camera_init(&game->camera);
    game->camera_x = 0;
    game->camera_y = 0;
    game->is_horizontal_room = false;

    // Inicializar bombas y puerta de Sala 1 por defecto
    static const struct { uint8_t r, c; } s_start_bombs[] = {
        { 8, 5 }, { 8, 7 }, { 3, 9 }, { 6, 11 }, { 6, 13 }
    };
    game->bomb_count = 5;
    for (uint8_t b = 0; b < game->bomb_count; ++b) {
        game->bombs[b].tile_y = s_start_bombs[b].r;
        game->bombs[b].tile_x = s_start_bombs[b].c;
        game->bombs[b].collected = false;
        game->bombs[b].room_id = 1;
        game->bombs[b].state = (b == 0) ? MJK_BOMB_STATE_STARTER_OR_END : MJK_BOMB_STATE_UNLIT;
    }
    game->active_fire_bomb_index = 0;
    game->door_tile_x = 0xFF;
    game->door_tile_y = 0xFF;
    game->door_condition_id = 0;
    game->door_condition_active = false;
    game->door_is_open = false;

    game->maybe_bomb_thing = 0;
    game->room_bomb_picked_flag = 0;
    game->power_coin_bomb_counter = 0;
    game->power_coin_trigger_flag = 0;
    game->fire_bombs_collected = 0;
    game->fire_bombs_collected_total = 0;
    game->total_bombs_collected = 0;
    game->chain_active = false;
    game->is_bomb_room = false;

    game->bomb_fuse.spark_sprite_y = 0xF8;
    game->bomb_fuse.spark_sprite_tile = 0x0D;
    game->bomb_fuse.spark_sprite_attr = 0x12;
    game->bomb_fuse.spark_sprite_x = 0;
    game->bomb_fuse.spark_anim_timer = 3;
    game->bomb_fuse.spark_visible = false;

    memset(game->bomb_puffs, 0, sizeof(game->bomb_puffs));

    MjkGame_ChangeState(game, GAME_STATE_0_INIT);
}

void MjkGame_SimulateFrame(MjkGameState* game, const MjkInputState* inputs, MjkFrameOutput* output) {
    if (!game) return;

    /* ========================================================================
     * PASO 1: PRÓLOGO NMI ($8081)
     * ======================================================================== */

    /* ========================================================================
     * PASO 2: OAM DMA ($809A)
     * ======================================================================== */

    /* ========================================================================
     * PASO 3: HANDLE PPU UPDATES ($80BE)
     * ======================================================================== */

    /* ========================================================================
     * PASO 4: UPDATE JOYPADS ($8818)
     * ======================================================================== */
    MjkInputState effective_inputs = {0, 0, 0};
    if (inputs) {
        effective_inputs.held = inputs->held;
        effective_inputs.just_pressed = (inputs->held) & ~(game->prev_input_held);
        effective_inputs.just_released = ~(inputs->held) & (game->prev_input_held);
    }

    /* ========================================================================
     * PASO 5: MAIN LOGIC HANDLER ($81A4)
     * ======================================================================== */
    if (!game->in_game) {
        switch (game->title_screen_state) {
            case TITLE_STATE_0_COPYRIGHT_INIT:
                game->copyright_timer = 260;
                game->title_screen_state = TITLE_STATE_1_COPYRIGHT_WAIT;
                break;

            case TITLE_STATE_1_COPYRIGHT_WAIT:
                if (effective_inputs.just_pressed & MJK_BTN_START) {
                    game->title_screen_state = TITLE_STATE_2_TITLE_INIT;
                    game->state_timer = 0;
                } else {
                    if (game->copyright_timer > 0) {
                        game->copyright_timer--;
                    }
                    if (game->copyright_timer == 0) {
                        game->title_screen_state = TITLE_STATE_2_TITLE_INIT;
                        game->state_timer = 0;
                    }
                }
                break;

            case TITLE_STATE_2_TITLE_INIT:
                /* En NES (prg.asm:7533-7570 TitleScreen_0 y prg.asm:2724 DrawFullScreen),
                 * el dibujado completo de la pantalla de título a VRAM toma ~208.000 ciclos de CPU con NMI
                 * deshabilitado, extendiéndose durante exactamente 8 fotogramas PPU (frames 179-186)
                 * antes de incrementar TitleScreenState ($0045) de 2 a 3 en prg.asm:7567. */
                game->state_timer++;
                if (game->state_timer >= 8) {
                    game->title_screen_state = TITLE_STATE_3_TITLE_PLAYER;
                    game->state_timer = 0;
                }
                break;

            case TITLE_STATE_3_TITLE_PLAYER:
                /* TitleScreen_1 (prg.asm:7595-7604): inicializa PlayerStruct, coloca a Jack en
                 * Y=$78 (120), X=$28 (40) y avanza inmediatamente de estado 3 a 4 (1 fotograma). */
                game->title_jack_x = FIXED_FROM_INT(40);
                game->title_jack_y = FIXED_FROM_INT(120);
                game->title_jack_vy = 0;
                game->title_screen_state = TITLE_STATE_4_WAIT_START;
                game->state_timer = 0;
                break;

            case TITLE_STATE_4_WAIT_START:
                if (effective_inputs.just_pressed & (MJK_BTN_START | MJK_BTN_A)) {
                    game->title_screen_state = TITLE_STATE_5_JACK_FALLING;
                    game->title_jack_x = FIXED_FROM_INT(40);
                    game->title_jack_y = FIXED_FROM_INT(120);
                    game->title_jack_vy = 0;
                    game->state_timer = 0;
                }
                break;

            case TITLE_STATE_5_JACK_FALLING:
                // Caída parabólica canónica de 73 frames o hasta Y >= 216 (prg.asm:7644-7659)
                game->state_timer++;
                game->title_jack_vy += 8; // 8 subpíxeles por frame de aceleración
                game->title_jack_y += game->title_jack_vy;
                if (game->state_timer >= 73 || FIXED_TO_INT(game->title_jack_y) >= 216) {
                    game->in_game = true;
                    game->title_screen_state = TITLE_STATE_0_COPYRIGHT_INIT;
                    MjkGame_ChangeState(game, GAME_STATE_0_INIT);
                }
                break;

            default:
                game->title_screen_state = TITLE_STATE_0_COPYRIGHT_INIT;
                break;
        }
    } else {
        MjkGame_ExecuteState(game, &effective_inputs);
    }

    /* ========================================================================
     * PASO 6: HANDLE SOUND ($B105)
     * ======================================================================== */
    if (output) {
        output->audio_sample_count = MJK_APU_SAMPLES_FRAME;
        // Silencio canónico absoluto durante Copyright Screen, Title Screen, Round Intro (GameState 1),
        // Load Room (GameState 2) y Draw Screen (GameState 3) según RES-013 y prg.asm:7496-7700.
        if (!game->in_game || game->current_state < GAME_STATE_4_ENTER_ROOM) {
            memset(output->audio_samples, 0, MJK_APU_SAMPLES_FRAME * sizeof(int16_t));
            if (game->apu.music_enabled) {
                MjkAPU_StopMusic(&game->apu);
            }
        } else {
            // Disparar música chiptune al pasar a sala activa en GameState 4 (Enter Room) / GameState 5
            // Comando canónico $16 (Music_MainWithIntro / MUSIC_MAIN_WITH_INTRO) según prg.asm:615-621
            if (!game->apu.music_enabled) {
                MjkAPU_PlayMusic(&game->apu, MUSIC_MAIN_WITH_INTRO);
            }
            MjkAPU_GenerateFrameSamples(&game->apu, output->audio_samples, MJK_APU_SAMPLES_FRAME);
        }
    }

    /* ========================================================================
     * PASO 7: EPÍLOGO Y FLANCOS DE ENTRADA ($80AC)
     * ======================================================================== */
    if (inputs) {
        game->prev_input_held = inputs->held;
    }
    game->total_frames_rendered++;

    /* ========================================================================
     * GENERACIÓN VISUAL EN FRAMEBUFFER (256x240) CON BLITTER 2C02
     * ======================================================================== */
    if (output) {
        if (!game->in_game) {
            if (game->title_screen_state <= TITLE_STATE_1_COPYRIGHT_WAIT) {
                RenderCopyrightScreen(game, output->framebuffer);
            } else {
                RenderTitleScreen(game, output->framebuffer);
            }
        } else {
            RenderInGameScreen(game, output->framebuffer);
        }
    }
}

/* ============================================================================
 * POWER COINS, TRANSMUTACIÓN, 1UP Y MIGHTY POWER (TASK-F6-025 / RES-024)
 * ============================================================================ */

// asm/src/data.asm:848-879 ScoreAddTable (Score_200, Score_300, Score_500, Score_800, Score_1200, Score_2000)
const uint16_t k_transmuted_coin_scores[6] = { 200, 300, 500, 800, 1200, 2000 };

bool mjk_game_use_mighty_coin(mjk_game_t* game) {
    if (!game || game->is_bomb_room) return false;
    game->jack.mighty_coins = game->mighty_coins;
    bool ok = mjk_player_use_mighty_coin(&game->jack);
    if (ok) {
        game->mighty_coins = game->jack.mighty_coins;
        if (game->jack.mighty_level == 3) {
            mjk_game_activate_power_coin(game);
        }
        MjkGame_UpdateJackPalette(game);
        MjkAPU_PlaySFX(&game->apu, SFX_USED_MIGHTY_COIN);
    }
    return ok;
}

void mjk_game_activate_power_coin(mjk_game_t* game) {
    if (!game) return;
    game->enemy_coin_timer = 320;
    game->transmute_timer = 320;
    game->transmute_streak = 0;
    game->jack.enemy_coin_timer = 320;
    // Pistas de BGM / SFX
    MjkAPU_PlaySFX(&game->apu, SFX_POWER_COIN_MUSIC);
}

void mjk_game_tick_power_coin(mjk_game_t* game) {
    if (!game) return;
    if (game->enemy_coin_timer > 0) {
        game->enemy_coin_timer--;
        game->transmute_timer = game->enemy_coin_timer;
        game->jack.enemy_coin_timer = game->enemy_coin_timer;
        if (game->enemy_coin_timer == 0) {
            game->power_coin_bomb_counter = 0;
            game->power_coin_trigger_flag = 0;
            game->bomb_fuse.power_coin_bomb_counter = 0;
            game->bomb_fuse.power_coin_trigger_flag = 0;
        }
    } else if (game->transmute_timer > 0) {
        game->transmute_timer--;
        game->enemy_coin_timer = game->transmute_timer;
        game->jack.enemy_coin_timer = game->transmute_timer;
        if (game->transmute_timer == 0) {
            game->power_coin_bomb_counter = 0;
            game->power_coin_trigger_flag = 0;
            game->bomb_fuse.power_coin_bomb_counter = 0;
            game->bomb_fuse.power_coin_trigger_flag = 0;
        }
    }
}

bool mjk_game_is_power_coin_active(const mjk_game_t* game) {
    if (!game) return false;
    return (game->enemy_coin_timer > 0 || game->transmute_timer > 0);
}

uint16_t mjk_game_get_transmuted_coin_score(uint8_t streak) {
    if (streak == 0) streak = 1;
    if (streak > 6) streak = 6;
    return k_transmuted_coin_scores[streak - 1];
}

uint16_t mjk_game_collect_transmuted_coin(mjk_game_t* game) {
    if (!game) return 0;
    if (game->transmute_streak < 6) {
        game->transmute_streak++;
    }
    uint16_t pts = k_transmuted_coin_scores[game->transmute_streak - 1];
    LocalBcdAdd(game->score_bcd, pts);
    game->gold_coins_collected++;
    if (game->gold_coins_collected >= 40) {
        // Umbral de 40 monedas alcanzado: habilita Extra Coin
    }
    MjkAPU_PlaySFX(&game->apu, SFX_COIN_COLLECTED);
    return pts;
}

void mjk_game_award_1up(mjk_game_t* game) {
    if (!game) return;
    mjk_player_award_1up(&game->jack);
    game->lives_count = game->jack.lives;
    game->collected_1up_this_round = 1;
    game->extra_lives_found++;
    MjkAPU_PlaySFX(&game->apu, SFX_1UP_CAPTIVE);
}

void mjk_game_collect_mighty_drink(mjk_game_t* game) {
    if (!game) return;
    LocalBcdAdd(game->score_bcd, 1000);
    game->stage_timer_seconds += 16; // +$10 BCD (+16s)
    if (game->stage_timer_seconds >= 0xA0 || game->stage_timer_seconds >= 100) {
        MjkGame_TriggerTortureRoom(game);
    }
    if (game->jack.mighty_level < 3) {
        game->jack.mighty_level++;
        game->jack.power_level = (MightyPowerLevel)game->jack.mighty_level;
        MjkGame_UpdateJackPalette(game);
    }
    MjkAPU_PlaySFX(&game->apu, SFX_TIME_PICKUP);
}

/* ============================================================================
 * CODICIA, SALA DE TORTURA Y SISTEMA GDV (TASK-F6-028 / RES-027)
 * ============================================================================ */

uint8_t MjkGame_AddAToGDV(uint8_t current_gdv, uint8_t points) {
    // prg.asm:1095-1117 AddAToGDV / BCDCheckFirstDigit
    uint8_t tens = (uint8_t)((current_gdv >> 4) & 0x0F);
    uint8_t ones = (uint8_t)(current_gdv & 0x0F);
    ones += points;
    while (ones >= 10) {
        ones -= 10;
        tens++;
    }
    if (tens > 9) tens = 9; // Límite BCD 99
    return (uint8_t)((tens << 4) | ones);
}

uint8_t MjkGame_CalculateFinalGDV(const MjkGameState* game, int8_t ending_type) {
    if (!game) return 0;
    uint8_t gdv = game->gdv_value;

    // 1. Rondas superadas bonus (prg.asm:1050-1053): (LastBombRoomCleared & 0xF) / 2 (max 7)
    uint8_t round_bonus = (uint8_t)((game->current_round & 0x0F) / 2);
    if (round_bonus > 7) round_bonus = 7;
    gdv = MjkGame_AddAToGDV(gdv, round_bonus);

    // 2. Vidas extras encontradas bonus (prg.asm:1054-1056): ExtraLivesFound / 2
    uint8_t lives_bonus = (uint8_t)(game->extra_lives_found / 2);
    gdv = MjkGame_AddAToGDV(gdv, lives_bonus);

    // 3. Puntuación bonus (prg.asm:1057-1070): dividido por 200,000 (0..5)
    uint32_t score = 0;
    for (int i = 3; i >= 0; --i) {
        score = score * 100 + ((game->score_bcd[i] >> 4) * 10 + (game->score_bcd[i] & 0x0F));
    }
    uint8_t score_bonus = 0;
    if (score >= 1000000) score_bonus = 5;
    else if (score >= 800000) score_bonus = 4;
    else if (score >= 600000) score_bonus = 3;
    else if (score >= 400000) score_bonus = 2;
    else if (score >= 200000) score_bonus = 1;
    gdv = MjkGame_AddAToGDV(gdv, score_bonus);

    // 4. Bonus de final completado (prg.asm:7423): +3 puntos al alcanzar el final
    if (ending_type >= 0) {
        gdv = MjkGame_AddAToGDV(gdv, 3);
    }

    return gdv;
}

uint8_t MjkGame_EvaluateEnding(const MjkGameState* game) {
    if (!game) return 0;
    // prg.asm:7377-7386
    // Ending 3 (Mejor final): requiere 5 Monedas Secretas y 2 Crystal Balls
    if (game->secret_coins_collected >= 5 && game->crystal_balls_collected >= 2) {
        return 3;
    }
    // Ending 2 (Gran final): requiere 5 Monedas Secretas
    if (game->secret_coins_collected >= 5) {
        return 2;
    }
    // Ending 1 (Buen final): requiere 2 Crystal Balls
    if (game->crystal_balls_collected >= 2) {
        return 1;
    }
    // Ending 0 (Peor final por defecto)
    return 0;
}

void MjkGame_TriggerTortureRoom(MjkGameState* game) {
    if (!game) return;
    game->mighty_coins = 0;
    game->jack.mighty_coins = 0;
    game->checkpoint_room_id = game->current_room;
    game->torture_scene_flag = 1;
    game->torture_message_timer = 192;
    game->torture_jumps_left = 50;
    game->greed_jump_counter = 0;
    MjkGame_ChangeState(game, GAME_STATE_7_TORTURE_ROOM);
}

