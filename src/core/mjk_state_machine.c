/**
 * @file mjk_state_machine.c
 * @brief Implementación de la máquina de 13 estados de Mighty Bomb Jack.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: GameHandler (prg.asm:302) y research/contracts/state_machine.md
 */

#include "mjk_game.h"
#include "mjk_camera.h"
#include "mjk_player.h"
#include "mjk_room_decoder.h"
#include "mjk_hud.h"
#include <string.h>

void MjkGame_ChangeState(MjkGameState* game, GameStateId new_state) {
    if (!game) return;
    game->current_state = new_state;
    game->sub_state = 0;
    game->state_timer = 0;
}

static void State_0_Init(MjkGameState* game) {
    game->lives_count = 3;
    game->current_round = 1;
    game->current_room = 1;
    game->entry_door_type = 0x0F; // Puerta canónica $0F (prg.asm:353-354)
    memset(game->score_bcd, 0, sizeof(game->score_bcd));
    game->mighty_coins = 0;
    game->gdv_value = 0x47; // PlayerGDV = $47 canónico (prg.asm:357-358)
    game->collected_1up_this_round = 0;
    MjkRoom_InitSection(&game->room_data_ram, 1, 0);
    
    // Transición directa a la introducción de la ronda
    MjkGame_ChangeState(game, GAME_STATE_1_ROUND_INTRO);
}

static void State_1_RoundIntro(MjkGameState* game, const MjkInputState* inputs) {
    (void)inputs;
    if (game->sub_state == 0) {
        // Duración canónica: exactamente 96 frames ($60 en NES, ~1.6 segundos a 60 Hz)
        // En NES (prg.asm:7671, 7690), en el primer fotograma se inicializa a $60 (96)
        // y se decrementa inmediatamente a 95 ($5F).
        game->state_timer = 95;
        game->sub_state = 1;
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
        }
        if (game->state_timer == 0) {
            MjkGame_ChangeState(game, GAME_STATE_2_LOAD_ROOM);
        }
    }
}

static uint8_t GetRoomSectionBombRole(uint8_t room_id) {
    if (room_id == 0 || room_id >= 0x90) return 0;
    for (uint8_t s = 0; s < MJK_SECTIONS_COUNT; ++s) {
        const uint8_t* list = g_section_rooms_table[s];
        if (!list) continue;
        uint8_t count = 0;
        while (list[count] != 0 && count < MJK_ROOMDATA_SECTION_SLOTS) count++;
        if (count > 0) {
            if (list[0] == room_id) return 1; // Primera sala de sección (starter)
            if (list[count - 1] == room_id) return 2; // Última sala de sección (final)
        }
    }
    return 0;
}

static void AppendRoomObjects(MjkGameState* game, uint8_t room_id) {
    if (!game || room_id == 0 || room_id >= MJK_TOTAL_ROOMS_COUNT) return;
    const uint8_t* rdata = g_room_data_table[room_id];
    if (!rdata) return;

    size_t rom_pos = 0;
    uint8_t byte_AC = 0;
    uint8_t bomb_role = GetRoomSectionBombRole(room_id);
    uint8_t room_bombs_added = 0;
    uint8_t ram_obj_idx = 1;

    int slot_idx = MjkRoom_FindSlot(&game->room_data_ram, room_id);
    const uint8_t* ram_raw = NULL;
    if (slot_idx >= 0) {
        ram_raw = (slot_idx < MJK_ROOMDATA_SECTION_SLOTS)
                ? game->room_data_ram.slots[slot_idx].raw
                : game->room_data_ram.scratch.raw;
    }

    while (rom_pos < 512) {
        uint8_t b = rdata[rom_pos];
        if (b == 0xFF) break;

        // Cabecera escénica $Ex: 3 bytes contiguos (RES-020: 1.1)
        if ((b & 0xF0) == 0xE0) {
            rom_pos += 3;
            continue;
        }

        if (byte_AC != 0) {
            byte_AC--;
            uint8_t yx = rdata[rom_pos++];
            ram_obj_idx++;
            if (game->bomb_count < 24) {
                uint8_t b_state = MJK_BOMB_STATE_UNLIT;
                if (bomb_role == 1 && room_bombs_added == 0) {
                    b_state = MJK_BOMB_STATE_STARTER_OR_END; // 0x85
                }
                if (ram_raw && ram_obj_idx < MJK_ROOMDATA_SLOT_SIZE) {
                    uint8_t r_state = ram_raw[ram_obj_idx];
                    if (r_state == MJK_OBJ_STATE_EMPTY) {
                        b_state = MJK_BOMB_STATE_EMPTY;
                    }
                }
                game->bombs[game->bomb_count].tile_y = (uint8_t)(yx >> 4);
                game->bombs[game->bomb_count].tile_x = (uint8_t)(yx & 0x0F);
                game->bombs[game->bomb_count].collected = (b_state == MJK_BOMB_STATE_EMPTY);
                game->bombs[game->bomb_count].room_id = room_id;
                game->bombs[game->bomb_count].state = b_state;
                game->bombs[game->bomb_count].ram_offset = ram_obj_idx;
                game->bomb_count++;
                room_bombs_added++;
            }
            continue;
        }

        rom_pos++;
        uint8_t hi = (uint8_t)(b >> 4);
        uint8_t lo = (uint8_t)(b & 0x0F);

        if (hi < 0x0B) {
            ram_obj_idx++;
        }

        if (hi == 0) {
            // Comando de bombas: byte conteo seguido de coordenadas consecutivas (RES-020: 2)
            byte_AC = (lo == 0) ? 0 : (uint8_t)(lo - 1);
            if (game->is_bomb_room) {
                byte_AC = 0x18; // 24 bombas en Bomb Room
            }
            uint8_t yx = rdata[rom_pos++];
            if (game->bomb_count < 24) {
                uint8_t b_state = MJK_BOMB_STATE_UNLIT;
                if (bomb_role == 1 && room_bombs_added == 0) {
                    b_state = MJK_BOMB_STATE_STARTER_OR_END; // 0x85
                }
                if (ram_raw && ram_obj_idx < MJK_ROOMDATA_SLOT_SIZE) {
                    uint8_t r_state = ram_raw[ram_obj_idx];
                    if (r_state == MJK_OBJ_STATE_EMPTY) {
                        b_state = MJK_BOMB_STATE_EMPTY;
                    }
                }
                game->bombs[game->bomb_count].tile_y = (uint8_t)(yx >> 4);
                game->bombs[game->bomb_count].tile_x = (uint8_t)(yx & 0x0F);
                game->bombs[game->bomb_count].collected = (b_state == MJK_BOMB_STATE_EMPTY);
                game->bombs[game->bomb_count].room_id = room_id;
                game->bombs[game->bomb_count].state = b_state;
                game->bombs[game->bomb_count].ram_offset = ram_obj_idx;
                game->bomb_count++;
                room_bombs_added++;
            }
        } else if (hi == 1 || hi == 2 || hi == 3 || hi == 4 || hi == 9 || hi == 0x0A) {
            // Cofres ($10..$1F visibles, $20..$4F/$90..$AF ocultos / especiales) (RES-020: 3)
            uint8_t yx = rdata[rom_pos++];
            if (game->chest_count < 16) {
                MjkChestEntity* ch = &game->chests[game->chest_count];
                ch->tile_y = (uint8_t)(yx >> 4);
                ch->tile_x = (uint8_t)(yx & 0x0F);
                ch->chest_type = (hi == 3 || hi == 4 || hi == 0x0A) ? 1 : 0; // 0=Rojo, 1=Naranja
                ch->is_hidden = ((b & 0x80) != 0); // bit 7 en bytecode -> $9x, $Ax = oculto ($20)
                ch->jumps_needed = ch->is_hidden ? 2 : 1;
                ch->is_open = false;
                ch->reward_spawned = false;
                ch->collected = false;
                ch->room_id = room_id;
                ch->item_id = lo;
                ch->original_cmd = b;
                ch->ram_slot_idx = ram_obj_idx;

                if (ram_raw && ram_obj_idx < MJK_ROOMDATA_SLOT_SIZE) {
                    uint8_t r_state = ram_raw[ram_obj_idx];
                    if ((r_state & 0x20) == 0 && (r_state == 0x10 || r_state == 0x11)) {
                        ch->is_hidden = false;
                        ch->jumps_needed = 1;
                    } else if (r_state < 0x0F) {
                        ch->is_hidden = false;
                        ch->is_open = true;
                        ch->reward_spawned = true;
                        ch->item_id = r_state;
                    } else if (r_state == 0x0F) {
                        ch->is_hidden = false;
                        ch->is_open = true;
                        ch->reward_spawned = true;
                        ch->collected = true;
                    }
                }

                game->chest_count++;
            }
            if (lo == 6) {
                rom_pos++; // Sphinx extra byte
            }
        } else if (hi == 5 || hi == 6) {
            // Bloque secreto ($5x, $6x)
            uint8_t yx = rdata[rom_pos++];
            if (game->secret_block_count < 16) {
                MjkSecretBlockEntity* sb = &game->secret_blocks[game->secret_block_count];
                sb->tile_y = (uint8_t)(yx >> 4);
                sb->tile_x = (uint8_t)(yx & 0x0F);
                sb->hits_required = (lo == 0) ? 1 : lo;
                sb->hits_left = sb->hits_required;
                sb->revealed = false;
                sb->room_id = room_id;
                sb->original_cmd = b;
                sb->ram_slot_idx = ram_obj_idx;

                if (ram_raw && ram_obj_idx < MJK_ROOMDATA_SLOT_SIZE) {
                    uint8_t r_state = ram_raw[ram_obj_idx];
                    if (r_state == 0x0F) {
                        sb->revealed = true;
                        sb->hits_left = 0;
                    }
                }

                game->secret_block_count++;
            }
        } else if (hi < 0x0B) {
            rom_pos++; // Salta coordenada o argumento de puertas ($8x), ítems ($7x)
        }
    }

    if (bomb_role == 2 && room_bombs_added > 0 && game->bomb_count > 0) {
        game->bombs[game->bomb_count - 1].state = MJK_BOMB_STATE_STARTER_OR_END;
    }
}

static void LoadRoomEntities(MjkGameState* game, uint8_t room_id) {
    if (!game) return;
    game->bomb_count = 0;
    game->chest_count = 0;
    game->secret_block_count = 0;
    game->secret_block_hits_left = 0;
    game->secret_block_active_coord = 0;
    game->chest_target_coord = 0;
    game->chest_spawning_cmd = 0;
    game->is_bomb_room = (room_id >= 0x90 && room_id < 0xA0);

    // 1. Cargar objetos de la sala principal
    AppendRoomObjects(game, room_id);

    // 2. Si es pasillo con scroll continuo hacia la derecha, precargar objetos de la sala contigua
    if (room_id < MJK_TOTAL_ROOMS_COUNT) {
        const MjkAdjacentRoom* adj = &g_adjacent_rooms[room_id];
        if (adj->right != 0) {
            AppendRoomObjects(game, adj->right);
        }
    }

    game->active_fire_bomb_index = 0;
    game->chain_active = false;
    game->fire_bombs_collected = 0;
    game->fire_bombs_collected_total = 0;
    game->total_bombs_collected = 0;
    game->maybe_bomb_thing = 0;
    game->room_bomb_picked_flag = 0;

    game->bomb_fuse.spark_sprite_y = 0xF8;
    game->bomb_fuse.spark_sprite_tile = 0x0D;
    game->bomb_fuse.spark_sprite_attr = 0x12;
    game->bomb_fuse.spark_sprite_x = 0;
    game->bomb_fuse.spark_anim_timer = 3;
    game->bomb_fuse.spark_visible = false;
}

static bool ConfigureRoomDoorFromBytecode(MjkGameState* game, uint8_t room_id) {
    if (!game || room_id == 0 || room_id >= MJK_TOTAL_ROOMS_COUNT) return false;
    const uint8_t* rdata = g_room_data_table[room_id];
    if (!rdata) return false;

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
    bool found_door = false;

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
            if (ram_raw && ram_obj_idx < MJK_ROOMDATA_SLOT_SIZE &&
                ram_raw[ram_obj_idx] != 0 && ram_raw[ram_obj_idx] != 0xFF) {
                state = ram_raw[ram_obj_idx];
            }

            bool is_door_open = (state & MJK_OBJ_STATE_DOOR_OPEN) != 0;
            if (is_door_open || game->door_condition_active || game->is_bomb_room) {
                if (lo < 16) {
                    const MjkDoorPositionTile* dpos = &g_door_position_tile_table[lo];
                    game->door_tile_x = (uint8_t)(dpos->y_tile & 0x0F);
                    game->door_tile_y = (uint8_t)(dpos->y_tile >> 4);
                    game->door_is_open = is_door_open;
                    game->entry_door_type = lo;
                    found_door = true;

                    uint8_t mt = is_door_open ? METATILE_DOOR_OPEN : METATILE_DOOR_CLOSED;
                    uint8_t y1 = (uint8_t)(dpos->y_tile >> 4);
                    uint8_t x1 = (uint8_t)(dpos->y_tile & 0x0F);
                    uint8_t y2 = (uint8_t)(dpos->x_tile >> 4);
                    uint8_t x2 = (uint8_t)(dpos->x_tile & 0x0F);
                    if (y1 < MJK_ROOM_TILES_H && x1 < MJK_ROOM_TILES_W) {
                        game->room_metatiles[y1 * MJK_ROOM_TILES_W + x1] = mt;
                    }
                    if (y2 < MJK_ROOM_TILES_H && x2 < MJK_ROOM_TILES_W) {
                        game->room_metatiles[y2 * MJK_ROOM_TILES_W + x2] = mt;
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
    return found_door;
}

static void State_2_LoadRoom(MjkGameState* game) {
    uint8_t room_id = game->current_room ? game->current_room : 1;

    // 1. Decodificar geometría 4-tier de la sala actual
    uint8_t metatiles_2d[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W];
    bool is_horizontal = false;
    MjkRoom_DecodeGeometry(room_id, metatiles_2d, &is_horizontal);

    // Inicializar cinemática de cámara para la sala activa (TASK-F6-024 / RES-019)
    camera_reset(&game->camera, room_id);
    game->is_horizontal_room = (game->camera.mode == CAMERA_MODE_HORIZONTAL_CORRIDOR);
    game->camera_x = (int16_t)(game->camera.scroll_x_fixed >> 8);
    game->camera_y = (int16_t)(game->camera.scroll_y_fixed >> 8);

    for (int r = 0; r < MJK_ROOM_TILES_H; ++r) {
        for (int c = 0; c < MJK_ROOM_TILES_W; ++c) {
            game->room_metatiles[r * MJK_ROOM_TILES_W + c] = metatiles_2d[r][c];
        }
    }

    // 2. Extraer condición canónica de apertura desde la cabecera de la sala (RES-017)
    bool has_cond = false;
    game->door_condition_id = MjkRoom_ExtractDoorCondition(room_id, &has_cond);
    game->door_condition_active = has_cond;
    game->door_is_open = false;
    game->door_is_animating = false;
    game->is_bomb_room = (room_id >= 0x90 && room_id < 0xA0);

    // 3. Posición de puerta y metatile en la sala según bytecode canónico (RES-029)
    bool found_door = ConfigureRoomDoorFromBytecode(game, room_id);
    if (!found_door) {
        game->door_condition_active = false;
        game->door_condition_id = 0;
        game->door_tile_x = 0xFF;
        game->door_tile_y = 0xFF;
    }

    // 4. Configurar posición de Jack (spawn canónico de DoorEntryXYPositionTable si procede)
    if (room_id == 1 && game->entry_door_type == 8) {
        // Spawn inicial por defecto para Sala 1 al comenzar partida
        game->jack.pos_x = FIXED_FROM_INT(40);
        game->jack.pos_y = FIXED_FROM_INT(192);
        game->jack.facing_left = false;
    } else {
        uint8_t dtype = (uint8_t)(game->entry_door_type & 0x0F);
        uint8_t sy = g_door_entry_xy_table[dtype].y;
        uint8_t sx = g_door_entry_xy_table[dtype].x;
        if (sy > 0 || sx > 0) {
            game->jack.pos_y = FIXED_FROM_INT(sy);
            game->jack.pos_x = FIXED_FROM_INT(sx);
        } else {
            game->jack.pos_x = FIXED_FROM_INT(40);
            game->jack.pos_y = FIXED_FROM_INT(192);
        }
        game->jack.facing_left = (sx > 128);
    }
    game->jack.vel_x = 0;
    game->jack.vel_y = 0;
    game->jack.subpixel_vx = 0;
    game->jack.subpixel_vy = 0;
    game->jack.state = JACK_STATE_ON_GROUND;
    game->jack.is_alive = true;
    game->stage_timer_seconds = 45;
    game->greed_jump_counter = 0;
    game->transmute_timer = 0;
    game->transmute_streak = 0;

    // 5. Inicializar Bombas y Cofres canónicos de la sala (TASK-F6-029 / RES-020)
    LoadRoomEntities(game, room_id);

    // 7. Purga de entidades anteriores e inicialización de enemigos (TASK-F6-026)
    MjkEnemy_Init(&game->enemy_mgr, game->current_round, room_id, game->is_bomb_room);
    game->active_enemies_count = 0;
    for (int i = 0; i < MJK_MAX_ENEMIES; ++i) {
        game->enemies[i].active = false;
    }
    for (int i = 0; i < MJK_MAX_FLOATING_SCORES; ++i) {
        game->floating_scores[i].active = false;
    }

    MjkGame_ChangeState(game, GAME_STATE_3_DRAW_SCREEN);
}

static void State_3_DrawScreen(MjkGameState* game) {
    /* En NES (prg.asm:379-440 GameState_3_DrawScreen y prg.asm:2724 DrawFullScreen),
     * la transferencia completa de nametable y atributos a VRAM con NMI deshabilitado
     * se extiende durante exactamente 8 fotogramas PPU (frames 442-449) antes de
     * avanzar a GameState 4 en prg.asm:439. */
    if (game->sub_state == 0) {
        // Purga total de entidades dinámicas y flags canónicos de sala (prg.asm:379-440)
        game->active_enemies_count = 0;
        for (int i = 0; i < MJK_MAX_ENEMIES; ++i) {
            game->enemies[i].active = false;
        }
        for (int i = 0; i < MJK_MAX_FLOATING_SCORES; ++i) {
            game->floating_scores[i].active = false;
        }
        game->door_check_flag_19_1a = 0;
        game->door_is_animating = false;
        game->state_timer = 0;
        game->sub_state = 1;
        return;
    }

    game->state_timer++;
    if (game->state_timer >= 7) {
        MjkGame_ChangeState(game, GAME_STATE_4_ENTER_ROOM);
    }
}

static void State_4_EnterRoom(MjkGameState* game) {
    /* En NES (prg.asm:445-623 GameState_4_EnterRoom y prg.asm:7219-7290 AnimateDoor):
     * La secuencia completa de entrada a la sala consta de:
     * 1. Animación de apertura de puerta (AnimateDoorOpen): 72 frames (32+8+32).
     * 2. Marcha de Jack fuera de la puerta (byte_341 = #$18): 24 frames.
     * 3. Animación de cierre de puerta (AnimateDoorClose): 72 frames (32+8+32).
     * Duración total canónica en NES: exactamente 167 frames (frames 450 a 616),
     * pasando a GameState 5 (GAME_STATE_5_MAIN_LOOP) en el frame 617. */
    if (game->sub_state == 0) {
        game->state_timer = 166;
        game->sub_state = 1;
        MjkAPU_PlaySFX(&game->apu, SFX_DOOR_OPEN);
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
            // Marcha de Jack durante la fase intermedia (frames 72 a 96 restantes)
            if (game->state_timer >= 72 && game->state_timer < 96) {
                if (game->jack.facing_left) {
                    game->jack.pos_x -= FIXED_FROM_INT(1);
                } else {
                    game->jack.pos_x += FIXED_FROM_INT(1);
                }
            }
        }
        if (game->state_timer == 0) {
            MjkAPU_PlaySFX(&game->apu, SFX_DOOR_OPEN);
            MjkGame_ChangeState(game, GAME_STATE_5_MAIN_LOOP);
        }
    }
}

static void State_5_MainLoop(MjkGameState* game, const MjkInputState* inputs) {
    if (!game) return;

    // 1. Salto de Jack y SFX (TASK-F6-019 / TASK-F7-003)
    if (inputs && (inputs->just_pressed & MJK_BTN_A)) {
        game->a_press_counter++;
        game->jack.a_press_counter = game->a_press_counter;
        if (game->jack.on_ground) {
            MjkAPU_PlaySFX(&game->apu, SFX_JUMP);
            game->greed_jump_counter++;
        }
    }

    // 1b. Consumo de Moneda Mighty con botón B (prg.asm:1448-1458 loc_895A)
    if (inputs && (inputs->just_pressed & MJK_BTN_B)) {
        mjk_game_use_mighty_coin(game);
    }

    // 2. Disparador de Codicia (Greed Detector - TASK-F6-028)
    if (game->greed_jump_counter >= 35) {
        MjkGame_TriggerTortureRoom(game);
        return;
    }

    // 3. Cinemática, físicas de 64 pasos y colisiones 9-probe multi-sala (TASK-F6-024B / RES-019B)
    game->jack.room_id = game->current_room;
    game->jack.scroll_x = (uint8_t)(game->camera.scroll_x_fixed >> 8);
    game->jack.scroll_y = (uint8_t)(game->camera.scroll_y_fixed >> 8);
    game->jack.room_flags = game->camera.room_flags;
    Jack_Update(&game->jack, inputs, game->room_metatiles);
    if (game->jack.queued_sfx != 0) {
        MjkAPU_PlaySFX(&game->apu, game->jack.queued_sfx);
        game->jack.queued_sfx = 0;
    }

    int jx = FIXED_TO_INT(game->jack.pos_x);
    int jy = FIXED_TO_INT(game->jack.pos_y);

    // 4. Temporizador de Transmutación de Power Coin (TASK-F6-024 / RES-024)
    mjk_game_tick_power_coin(game);

    // 5. Interacción canónica con Bombas y Cadena de Mechas (TASK-F6-022B / RES-022)
    for (uint8_t b = 0; b < game->bomb_count; ++b) {
        MjkBombEntity* bomb = &game->bombs[b];
        if (bomb->collected) continue;

        // Bounding box AABB Jack vs Bomba en coordenadas de mundo continuas (RES-020)
        if (Jack_CheckBombCollision(&game->jack, bomb->room_id, bomb->tile_x, bomb->tile_y, game->camera_x)) {
            MjkRoom_CollectBombCanonical(game, b);
        }
    }

    // 6. Interacción con Cofres del Tesoro y Bloques Secretos (TASK-F6-023 / RES-023)
    MjkGame_UpdateChestsAndSecrets(game);

    // 7. Evaluación de Puertas y Transición (TASK-F6-021 / RES-017 / RES-022 / RES-029)
    if (!game->door_is_open) {
        if (game->door_tile_x != 0xFF && (game->door_condition_active || game->is_bomb_room)) {
            bool should_open = false;
            if (game->is_bomb_room) {
                if (game->total_bombs_collected >= 23) {
                    should_open = true;
                }
            } else if (game->door_condition_active) {
                if (MjkRoom_CheckDoorCondition(game->door_condition_id, &game->jack, game)) {
                    should_open = true;
                }
            }

            if (should_open) {
                if (game->door_condition_id == 0x0C && (game->maybe_bomb_thing & MJK_BOMB_THING_PERFECT_BONUS)) {
                    game->maybe_bomb_thing &= ~MJK_BOMB_THING_PERFECT_BONUS; // Consume bit 7 canónico (prg.asm:6875)
                }
                MjkRoom_HandleOpenDoorMaybe(game);
                return;
            }
        }
    } else {
        if (game->door_tile_x != 0xFF && game->door_tile_y != 0xFF) {
            int dx = game->door_tile_x * 16 - game->camera_x;
            int dy = game->door_tile_y * 16 - game->camera_y;
            if (jx + 12 >= dx && jx + 4 <= dx + 16 && jy + 16 >= dy && jy <= dy + 32) {
                game->jack.state = JACK_STATE_ENTERING_DOOR;
                MjkGame_ChangeState(game, GAME_STATE_9_EXIT_ROOM);
                return;
            }
        }
    }

    // 8. Actualizar enemigos con gestor canónico (TASK-F6-026 / RES-025 / TASK-F7-010)
    game->enemy_mgr.transmute_timer = game->transmute_timer;
    game->enemy_mgr.transmute_streak = game->transmute_streak;
    MjkEnemy_UpdateAll(&game->enemy_mgr, &game->jack, game->room_metatiles, game->a_press_counter);

    int32_t col = MjkEnemy_CheckJackCollisions(&game->enemy_mgr, &game->jack);
    if (col > 0) {
        MjkBCD_AddScore(game->score_bcd, (uint16_t)col);
        MjkGame_SpawnFloatingScore(game, (int16_t)jx, (int16_t)jy, (uint16_t)col);
        MjkAPU_PlaySfx(&game->apu, (game->enemy_mgr.transmute_timer > 0) ? SFX_COLLECT_BOMB : SFX_OPEN_CHEST);
        game->transmute_streak = game->enemy_mgr.transmute_streak;
    } else if (col == -1) {
        game->jack.is_alive = false;
        MjkAPU_PlaySfx(&game->apu, SFX_PLAYER_DIE);
    }
    memcpy(game->enemies, game->enemy_mgr.enemies, sizeof(game->enemies));
    game->active_enemies_count = game->enemy_mgr.alive_enemy_count;

    // 9. Actualizar marcadores de puntos flotantes (TASK-F6-027 / RES-026)
    MjkGame_UpdateFloatingScores(game);

    // 10. Cinemática de Cámara y Scroll de Escenario (TASK-F6-024 / RES-019)
    int16_t prev_cam_x = game->camera_x;
    camera_update_kinematics(&game->camera, g_adjacent_rooms, &game->jack, inputs);
    game->camera_x = (int16_t)(game->camera.scroll_x_fixed >> 8);
    game->camera_y = (int16_t)(game->camera.scroll_y_fixed >> 8);
    int16_t cam_dx = (int16_t)(game->camera_x - prev_cam_x);
    if (cam_dx != 0) {
        // Compensación de scroll para efectos visuales (sub_A32B / prg.asm:5751)
        for (int p = 0; p < MJK_MAX_BOMB_PUFFS; ++p) {
            if (game->bomb_puffs[p].active) {
                game->bomb_puffs[p].x -= cam_dx;
            }
        }
        for (int f = 0; f < MJK_MAX_FLOATING_SCORES; ++f) {
            if (game->floating_scores[f].active) {
                game->floating_scores[f].x -= cam_dx;
            }
        }
    }

    // Transición limpia de sala (loc_A55E / loc_A4DD / RES-019B Section 5)
    if (game->camera.current_room_id != game->current_room && game->camera.current_room_id != 0) {
        uint8_t new_room = game->camera.current_room_id;
        game->current_room = new_room;
        game->jack.room_id = new_room;
        game->enemy_mgr.current_room = new_room;

        // Decodificar geometría de la nueva sala sin tirones ni reseteos bruscos
        uint8_t metatiles_2d[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W];
        bool is_h = false;
        MjkRoom_DecodeGeometry(new_room, metatiles_2d, &is_h);
        game->is_horizontal_room = is_h;

        for (int r = 0; r < MJK_ROOM_TILES_H; ++r) {
            for (int c = 0; c < MJK_ROOM_TILES_W; ++c) {
                game->room_metatiles[r * MJK_ROOM_TILES_W + c] = metatiles_2d[r][c];
            }
        }

        // Cargar condición y puerta de la nueva sala (RES-029)
        bool has_cond = false;
        game->door_condition_id = MjkRoom_ExtractDoorCondition(new_room, &has_cond);
        game->door_condition_active = has_cond;
        game->door_is_open = false;
        game->is_bomb_room = (new_room >= 0x90 && new_room < 0xA0);

        bool found_door = ConfigureRoomDoorFromBytecode(game, new_room);
        if (!found_door) {
            game->door_condition_active = false;
            game->door_condition_id = 0;
            game->door_tile_x = 0xFF;
            game->door_tile_y = 0xFF;
        }

        const MjkAdjacentRoom* adj = &g_adjacent_rooms[new_room];

        // Asegurar que los objetos de la nueva sala y su vecina derecha estén disponibles (RES-020)
        bool has_new_room_bombs = false;
        for (uint8_t b = 0; b < game->bomb_count; ++b) {
            if (game->bombs[b].room_id == new_room) {
                has_new_room_bombs = true;
                break;
            }
        }
        if (!has_new_room_bombs) {
            LoadRoomEntities(game, new_room);
        } else if (adj->right != 0) {
            bool has_next_bombs = false;
            for (uint8_t b = 0; b < game->bomb_count; ++b) {
                if (game->bombs[b].room_id == adj->right) {
                    has_next_bombs = true;
                    break;
                }
            }
            if (!has_next_bombs) {
                AppendRoomObjects(game, adj->right);
            }
        }
    }

    // Aplicar delta inverso de cámara a entidades activas que residen en pantalla (RES-019 / TASK-F7-010)
    for (int i = 0; i < MJK_MAX_ENEMIES; ++i) {
        if (game->enemy_mgr.enemies[i].active) {
            camera_apply_entity_delta_fixed(&game->camera, &game->enemy_mgr.enemies[i].pos_x, &game->enemy_mgr.enemies[i].pos_y);
        }
    }
    memcpy(game->enemies, game->enemy_mgr.enemies, sizeof(game->enemies));
    for (int i = 0; i < MJK_MAX_FLOATING_SCORES; ++i) {
        if (game->floating_scores[i].active) {
            camera_apply_entity_delta(&game->camera, &game->floating_scores[i].x, &game->floating_scores[i].y);
        }
    }

    // 11. Temporizador de sala regresivo
    if (game->state_timer % 60 == 0 && game->stage_timer_seconds > 0) {
        game->stage_timer_seconds--;
        if (game->stage_timer_seconds == 0) {
            MjkGame_ChangeState(game, GAME_STATE_6_TIME_OVER);
            return;
        }
    }
    game->state_timer++;

    // 12. Comprobación de muerte de Jack
    if (!game->jack.is_alive) {
        if (game->lives_count > 0) {
            game->lives_count--;
            MjkGame_ChangeState(game, GAME_STATE_2_LOAD_ROOM);
        } else {
            MjkGame_ChangeState(game, GAME_STATE_A_GAME_OVER);
        }
    }
}

static void State_6_TimeOver(MjkGameState* game) {
    if (game->sub_state == 0) {
        game->state_timer = 60; // 1 segundo de aviso
        game->sub_state = 1;
        MjkAPU_PlaySFX(&game->apu, SFX_DEATH); // Sound_Death $02 (prg.asm:1322)
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
        } else {
            MjkGame_ChangeState(game, GAME_STATE_7_TORTURE_ROOM);
        }
    }
}

static void State_7_TortureRoom(MjkGameState* game, const MjkInputState* inputs) {
    if (game->sub_state == 0) {
        // En la versión USA (REV_US), la penalización son exactamente 50 saltos (0x50 BCD)
        game->torture_message_timer = 192;
        game->torture_jumps_left = 50;
        game->torture_scene_flag = 1;
        game->current_room = 0xAC;
        game->jack.pos_x = FIXED_FROM_INT(128);
        game->jack.pos_y = FIXED_FROM_INT(180);
        game->jack.vel_x = 0;
        game->jack.vel_y = 0;
        game->jack.state = JACK_STATE_ON_GROUND;
        game->sub_state = 1;
    } else if (game->sub_state == 1) {
        if (game->torture_message_timer > 0) {
            game->torture_message_timer--;
        }
        Jack_Update(&game->jack, inputs, game->room_metatiles);

        // Cada pulsación de botón A decrementa el contador de saltos restantes
        // En la NES original solo emite SFX_JUMP si el salto se inicia desde el suelo (prg.asm:743, 1422-1430)
        if (inputs && (inputs->just_pressed & MJK_BTN_A)) {
            if (game->jack.on_ground) {
                MjkAPU_PlaySFX(&game->apu, SFX_JUMP);
            }
            if (game->torture_jumps_left > 0) {
                game->torture_jumps_left--;
            }
        }
        
        // Al completar los 50 saltos, escapa de la tortura y vuelve a la sala
        if (game->torture_jumps_left == 0) {
            game->torture_scene_flag = 0;
            if (game->checkpoint_room_id != 0 && game->checkpoint_room_id != 0xAC) {
                game->current_room = game->checkpoint_room_id;
            } else {
                game->current_room = 1;
            }
            MjkGame_ChangeState(game, GAME_STATE_2_LOAD_ROOM);
        }
    }
}

static void State_8_RoundClear(MjkGameState* game) {
    if (game->sub_state == 0) {
        game->state_timer = 90; // Recuento de bonificaciones
        game->sub_state = 1;
        MjkAPU_PlayMusic(&game->apu, MUSIC_ROUND_CLEAR); // Music_RoundClear $18 (prg.asm:868-869)
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
        } else {
            game->current_round++;
            game->collected_1up_this_round = 0;
            if (game->current_round > 16) {
                MjkGame_ChangeState(game, GAME_STATE_C_ENDING);
            } else {
                MjkGame_ChangeState(game, GAME_STATE_1_ROUND_INTRO);
            }
        }
    }
}

static void State_9_ExitRoom(MjkGameState* game) {
    if (game->sub_state == 0) {
        game->state_timer = 20; // Animación de salida y cierre de puerta
        game->sub_state = 1;
        MjkAPU_StopMusic(&game->apu); // prg.asm:5329-5330: Silencia audio al cruzar la puerta
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
        } else {
            // Desvío de salas de bombas a Round Clear
            if (game->is_bomb_room || (game->current_room >= 0x90 && game->current_room < 0xA0)) {
                MjkGame_ChangeState(game, GAME_STATE_8_ROUND_CLEAR);
                return;
            }
            // Desvío de salas finales
            if (game->current_room >= 0xA0 && game->current_room <= 0xA3) {
                MjkGame_ChangeState(game, GAME_STATE_C_ENDING);
                return;
            }

            // Consultar matriz topológica AdjacentRoomsTable
            uint8_t next_room = MjkRoom_GetAdjacentRoom(game->current_room, game->entry_door_type);
            if (next_room == 0xAC) {
                MjkGame_ChangeState(game, GAME_STATE_7_TORTURE_ROOM);
                return;
            }
            if (next_room == 0) {
                next_room = (uint8_t)(game->current_room + 1);
            }

            // Inversión canónica del eje de spawn: EntryDoorType ^ 4
            game->entry_door_type = (uint8_t)(game->entry_door_type ^ 4);
            game->current_room = next_room;

            MjkGame_ChangeState(game, GAME_STATE_2_LOAD_ROOM);
        }
    }
}

static void State_A_GameOver(MjkGameState* game, const MjkInputState* inputs) {
    if (game->sub_state == 0) {
        game->state_timer = 300; // 5 segundos en pantalla de Game Over
        game->gdv_value = MjkGame_CalculateFinalGDV(game, -1);
        if (game->gdv_value > game->high_gdv) {
            game->high_gdv = game->gdv_value;
        }
        game->sub_state = 1;
        MjkAPU_PlayMusic(&game->apu, MUSIC_GAME_OVER); // Music_GameOver $17 (prg.asm:1046-1047)
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
        }
        // Si pulsa Start o se agota el tiempo, regresa a la pantalla de título
        if ((inputs && (inputs->just_pressed & MJK_BTN_START)) || game->state_timer == 0) {
            game->in_game = false;
            MjkGame_ChangeState(game, GAME_STATE_0_INIT);
        }
    }
}

static void State_B_AnimateDoorOpen(MjkGameState* game) {
    if (game->sub_state == 0) {
        // Fase 0: 32 frames ($20 ticks)
        game->state_timer = 32;
        game->door_anim_phase = 0;
        game->sub_state = 1;
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
        } else {
            game->door_anim_phase++;
            if (game->door_anim_phase == 1) {
                // Fase 1: 8 frames ($08 ticks)
                game->state_timer = 8;
            } else if (game->door_anim_phase == 2) {
                // Fase 2: 32 frames ($20 ticks)
                game->state_timer = 32;
            } else {
                // Fase 3: Fin de animación -> MaybeDoorReturnTwiceFlag = 0
                game->door_is_animating = false;
                MjkGame_ChangeState(game, GAME_STATE_5_MAIN_LOOP);
            }
        }
    }
}

static void State_C_Ending(MjkGameState* game, const MjkInputState* inputs) {
    if (game->sub_state == 0) {
        uint8_t ending_type = MjkGame_EvaluateEnding(game);
        game->gdv_value = MjkGame_CalculateFinalGDV(game, (int8_t)ending_type);
        if (game->gdv_value > game->high_gdv) {
            game->high_gdv = game->gdv_value;
        }
        game->state_timer = 600; // 10 segundos de cinemática
        game->sub_state = 1;
    } else if (game->sub_state == 1) {
        if (game->state_timer > 0) {
            game->state_timer--;
        }
        if ((inputs && (inputs->just_pressed & MJK_BTN_START)) || game->state_timer == 0) {
            game->in_game = false;
            MjkGame_ChangeState(game, GAME_STATE_0_INIT);
        }
    }
}

void MjkGame_ExecuteState(MjkGameState* game, const MjkInputState* inputs) {
    if (!game) return;

    switch (game->current_state) {
        case GAME_STATE_0_INIT:              State_0_Init(game); break;
        case GAME_STATE_1_ROUND_INTRO:       State_1_RoundIntro(game, inputs); break;
        case GAME_STATE_2_LOAD_ROOM:         State_2_LoadRoom(game); break;
        case GAME_STATE_3_DRAW_SCREEN:       State_3_DrawScreen(game); break;
        case GAME_STATE_4_ENTER_ROOM:        State_4_EnterRoom(game); break;
        case GAME_STATE_5_MAIN_LOOP:         State_5_MainLoop(game, inputs); break;
        case GAME_STATE_6_TIME_OVER:         State_6_TimeOver(game); break;
        case GAME_STATE_7_TORTURE_ROOM:      State_7_TortureRoom(game, inputs); break;
        case GAME_STATE_8_ROUND_CLEAR:       State_8_RoundClear(game); break;
        case GAME_STATE_9_EXIT_ROOM:         State_9_ExitRoom(game); break;
        case GAME_STATE_A_GAME_OVER:         State_A_GameOver(game, inputs); break;
        case GAME_STATE_B_ANIMATE_DOOR_OPEN: State_B_AnimateDoorOpen(game); break;
        case GAME_STATE_C_ENDING:            State_C_Ending(game, inputs); break;
        default:
            MjkGame_ChangeState(game, GAME_STATE_0_INIT);
            break;
    }
}
