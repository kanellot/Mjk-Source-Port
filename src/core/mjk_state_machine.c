/**
 * @file mjk_state_machine.c
 * @brief Máquina de estados completa `GameHandler` 0..C (REV_US).
 */

#include "mjk_game.h"
#include <string.h>

void MjkGame_QueueSound(MjkGame* game, MjkSoundId snd) {
    if (!game) {
        return;
    }
    game->last_sound_event = snd;
    if (game->sound_queue_count < MJK_GAME_SOUND_QUEUE_CAP) {
        game->sound_queue[game->sound_queue_count++] = snd;
    }
}

#define queue_sound MjkGame_QueueSound
#define bcd_sub1    MjkBcd_Sub1

uint8_t MjkGame_AddGdvBcd(uint8_t current_gdv_bcd, uint8_t delta) {
    uint8_t cur = MjkBcd_ToBinary(current_gdv_bcd);
    unsigned sum = (unsigned)cur + (unsigned)delta;
    if (sum > 99u) {
        sum = 99u;
    }
    return MjkBcd_FromBinary((uint8_t)sum);
}

void MjkGame_AddScore(MjkGame* game, MjkScoreId score_id) {
    bool spawn_bonus = false;
    bool spawn_extra = false;
    MjkHud_AddScore(game, score_id, &spawn_bonus, &spawn_extra);
    if (spawn_extra) {
        MjkEnemy_SpawnCollectible(&game->enemies, MJK_ENEMY_EXTRA_COIN, 0x80u, 0x40u);
    } else if (spawn_bonus) {
        MjkEnemy_SpawnCollectible(&game->enemies, MJK_ENEMY_BONUS_COIN, 0x80u, 0x40u);
    }
}

void MjkGame_Init(MjkGame* game, const MjkAssetPackage* pkg) {
    if (!game) {
        return;
    }
    memset(game, 0, sizeof(*game));
    game->pkg = pkg;
    game->top_mode = MJK_MODE_TITLE;
    game->state = MJK_GS_INIT;
    game->high_gdv_bcd = 0x00u;
    MjkTitle_Init(&game->title);
    MjkPpu_Init(&game->ppu, pkg);
    MjkApu_Init(&game->apu);
}

void MjkGame_StepAudio(MjkGame* game, int16_t* out_samples, uint32_t sample_count) {
    if (!game) {
        if (out_samples && sample_count > 0u) {
            memset(out_samples, 0, (size_t)sample_count * sizeof(int16_t));
        }
        return;
    }
    for (uint8_t i = 0u; i < game->sound_queue_count; ++i) {
        MjkApu_QueueSound(&game->apu, game->sound_queue[i]);
    }
    game->sound_queue_count = 0u;
    MjkApu_StepFrame(&game->apu, game->pkg, out_samples, sample_count);
}

void MjkGame_WarpToRoom(MjkGame* game, uint8_t room_id) {
    if (!game || !game->pkg) {
        return;
    }
    game->top_mode = MJK_MODE_INGAME;
    game->paused = false;
    game->torture_scene_flag = 0u;
    game->current_room_id = room_id;
    game->checkpoint_room_id = room_id;
    game->entry_door_type = 0x0Fu;
    game->entry_door_backup = 0x0Fu;
    if (game->lives == 0u) {
        game->lives = 3u;
    }
    game->state = (room_id == 0u) ? MJK_GS_DRAW_SCREEN : MJK_GS_LOAD_ROOM;
    game->sub_state = 0u;
    if (room_id == 0u) {
        MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
        MjkRoom_DecodeGeometry(game->pkg, 0x00u, &game->room);
        MjkRoom_BlitToPpu(&game->room, game->pkg, &game->ppu, 0u);
        MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_PUSH_START_BUTTON);
        game->state = MJK_GS_MAIN;
    }
}

void MjkGame_LoseLife(MjkGame* game) {
    if (!game) {
        return;
    }
    game->timer_status = 0u;
    if (game->lives > 0u) {
        game->lives--;
    }
    if (game->lives == 0u) {
        game->state = MJK_GS_GAME_OVER;
        game->sub_state = 0u;
        return;
    }
    game->current_room_id = game->checkpoint_room_id;
    game->entry_door_type = game->entry_door_backup;
    game->mighty_level = 0u;
    game->mighty_presses_left = 0u;
    game->torture_scene_flag = 0u;
    game->state = MJK_GS_ROUND_INTRO;
    game->sub_state = 0u;
}

MjkActionResult MjkGame_TriggerTortureRoom(MjkGame* game) {
    if (!game) {
        return MJK_ACTION_CONTINUE;
    }
    game->mighty_coins = 0u;
    game->torture_scene_flag = 0u;
    game->state = MJK_GS_TORTURE_ROOM;
    game->sub_state = 0u;
    return MJK_ACTION_TORTURE_TRIGGERED;
}

void MjkGame_OnTortureJump(MjkGame* game) {
    if (!game || game->torture_scene_flag == 0u) {
        return;
    }
    game->greedy_jumps_bcd = bcd_sub1(game->greedy_jumps_bcd);
    if (game->greedy_jumps_bcd == 0u) {
        game->torture_scene_flag = 0u;
        game->timer_status &= 0xBFu;
        game->checkpoint_room_id = game->torture_return_room_id;
        game->entry_door_backup = game->torture_return_door_type;
        game->current_room_id = game->checkpoint_room_id;
        game->entry_door_type = game->entry_door_backup;
        game->state = MJK_GS_LOAD_ROOM;
        game->sub_state = 0u;
    }
}

static void trigger_ending(MjkGame* game, uint8_t ending_idx) {
    MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
    game->ending_type = (uint8_t)(ending_idx & 3u);
    game->ending_state = 0u;
    game->gdv_bcd = MjkGame_AddGdvBcd(game->gdv_bcd, 3u);
    game->state = MJK_GS_ENDING;
    game->sub_state = 0u;
}

void MjkGame_ExitRoom(MjkGame* game, uint8_t exit_door_dir) {
    if (!game || !game->pkg) {
        return;
    }
    uint8_t prev_room = game->current_room_id;
    const uint8_t* adj_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_ADJACENT_ROOMS, 256u * 5u);
    uint8_t next_room = prev_room;
    if (adj_tbl) {
        const uint8_t* entry = adj_tbl + ((size_t)prev_room * 5u);
        next_room = entry[1u + (exit_door_dir & 3u)];
    }

    game->current_room_id = next_room;
    game->entry_door_type ^= 0x04u;
    game->state = MJK_GS_LOAD_ROOM;
    game->sub_state = 0u;

    if (prev_room >= 0x90u && prev_room < 0xA0u) {
        game->last_bomb_room_cleared = prev_room;
        game->gs8_status = 0u;
        game->state = MJK_GS_ROUND_CLEAR;
        return;
    }
    if (next_room == 0xACu) {
        MjkGame_TriggerTortureRoom(game);
        return;
    }
    if (prev_room >= 0xA0u && prev_room <= 0xA3u) {
        uint8_t etype = (uint8_t)(prev_room & 3u);
        bool match = false;
        if (etype == 0u && game->crystal_balls_mask == 0u) {
            match = true;
        } else if (etype == 1u && game->crystal_balls_mask != 3u) {
            match = true;
        } else if (etype == 2u && game->secret_coins < 5u) {
            match = true;
        } else if (etype == 3u) {
            match = true;
        }
        if (match) {
            trigger_ending(game, etype);
        }
    }
}

void MjkGame_StepFrame(MjkGame* game, const MjkInputState* input) {
    if (!game || !game->pkg) {
        return;
    }
    game->sound_queue_count = 0u;

    if (game->top_mode == MJK_MODE_TITLE) {
        if (MjkTitle_Step(&game->title, input, game->pkg, &game->ppu, game->score_bcd, game->high_gdv_bcd)) {
            game->top_mode = MJK_MODE_INGAME;
            game->state = MJK_GS_INIT;
            game->sub_state = 0u;
        }
        return;
    }

    switch (game->state) {
        case MJK_GS_INIT:
            game->checkpoint_room_id = 0u;
            game->gold_coins = 0u;
            game->secret_coins = 0u;
            game->crystal_balls_mask = 0u;
            game->torture_scene_flag = 0u;
            game->last_bomb_room_cleared = 0u;
            game->extra_lives_found = 0u;
            game->mighty_coins = 0u;
            game->collected_1up = 0u;
            game->score_multiplier = 0u;
            game->timer_status = 0u;
            memset(game->score_bcd, 0, sizeof(game->score_bcd));
            queue_sound(game, MJK_SND_SILENCE);
            MjkPpu_ClearNametable(&game->ppu, 0x24u);
            game->current_room_id = 1u;
            game->entry_door_type = 0x0Fu;
            game->lives = 3u;
            game->gdv_bcd = 0x47u;
            game->state = MJK_GS_ROUND_INTRO;
            game->sub_state = 0u;
            break;

        case MJK_GS_ROUND_INTRO:
            if (game->sub_state == 0u) {
                MjkPpu_ClearOam(&game->ppu);
                MjkPpu_ClearNametable(&game->ppu, 0x24u);
                MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
                MjkPpu_ClearOam(&game->ppu);
            uint8_t slot = MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0u, 0x70u, 0x60u, 0x01u, 0u);
                MjkPpu_SetOam(&game->ppu, slot++, 0x80u, 0x5Cu, 0x0Cu, 0u);
                MjkPpu_DrawBcdByte(&game->ppu, MjkBcd_FromBinary(game->lives), 0x94u, 0x60u, true, slot);
                game->state_timer = 0x60u; /* 96f */
                game->sub_state = 1u;
            } else {
                if (game->state_timer > 0u) {
                    game->state_timer--;
                }
                if (game->state_timer == 0u) {
                    MjkPpu_ClearOam(&game->ppu);
                    game->state = MJK_GS_LOAD_ROOM;
                    game->sub_state = 0u;
                }
            }
            break;

        case MJK_GS_LOAD_ROOM: {
            MjkRoom_PreloadSection(game->pkg, game->current_room_id, &game->persist, game->crystal_balls_mask);
            MjkRoom_DecodeGeometry(game->pkg, game->current_room_id, &game->room);
            MjkRoom_DecodeObjects(game->pkg, game->current_room_id, &game->persist, &game->room);
            MjkCamera_InitForRoom(&game->camera, game->current_room_id, &game->room.adj, game->room.is_bomb_room);
            if (game->camera.neighbor_room_id != 0u) {
                MjkRoom_DecodeGeometry(game->pkg, game->camera.neighbor_room_id, &game->right_room);
                MjkRoom_DecodeObjects(game->pkg, game->camera.neighbor_room_id, &game->persist, &game->right_room);
            }
            MjkObjects_InitForRoom(&game->objects, game);
            MjkEnemy_InitForRoom(&game->enemies, game->pkg, game->last_bomb_room_cleared, game->room.is_bomb_room);
            memset(game->popups, 0, sizeof(game->popups));
            const uint8_t* rdata = MjkAssets_GetRoomDataPtr(game->pkg, game->current_room_id, NULL);
            if (rdata && (rdata[0] & 0xF0u) == 0xE0u && rdata[2] != 0u && (rdata[2] & 0x40u) == 0u &&
                game->current_room_id != 0xFCu) {
                game->checkpoint_room_id = game->current_room_id;
                game->entry_door_backup = game->entry_door_type;
                if ((rdata[2] & 0x80u) != 0u) {
                    game->timer_status = 0u;
                    game->torture_return_room_id = game->current_room_id;
                    game->torture_return_door_type = game->entry_door_type;
                }
            }
            game->state = MJK_GS_DRAW_SCREEN;
            game->sub_state = 0u;
            break;
        }

        case MJK_GS_DRAW_SCREEN:
            MjkPpu_ClearOam(&game->ppu);
            if (game->room.is_bomb_room) {
                game->mighty_level = 0u;
                game->mighty_presses_left = 0u;
                game->ppu.palette_ram[0x17u] = 0x16u;
            }
            MjkRoom_BlitToPpu(&game->room, game->pkg, &game->ppu, 0u);
            if (game->camera.neighbor_room_id != 0u) {
                MjkRoom_BlitToPpu(&game->right_room, game->pkg, &game->ppu, 1u);
            }
            game->state = MJK_GS_ENTER_ROOM;
            game->sub_state = 0u;
            break;

        case MJK_GS_ENTER_ROOM:
            if (game->sub_state == 0u) {
                MjkPpu_LoadRoomPalette(&game->ppu, game->pkg, game->room.palette_set);
                const uint8_t* pos_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_DOOR_ENTRY_XY, 32u);
                uint8_t idx = (uint8_t)(game->entry_door_type & 0x0Fu);
                game->jack_y = pos_tbl ? pos_tbl[idx * 2u] : 0x78u;
                game->jack_x = pos_tbl ? pos_tbl[idx * 2u + 1u] : 0x80u;
                game->state_timer = 0x18u; /* 24f */
                game->sub_state = 1u;
            } else {
                const uint8_t* vec_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_VECTOR_TABLE, 8u);
                uint8_t dir = (uint8_t)((game->entry_door_type >> 2u) & 3u);
                uint8_t vec_idx = (uint8_t)(3u - dir);
                if (vec_tbl) {
                    game->jack_x = (uint8_t)(game->jack_x + (int8_t)vec_tbl[vec_idx * 2u]);
                    game->jack_y = (uint8_t)(game->jack_y + (int8_t)vec_tbl[vec_idx * 2u + 1u]);
                }
                if (game->state_timer > 0u) {
                    game->state_timer--;
                }
                if (game->state_timer == 0u) {
                    if (game->timer_status == 0u) {
                        game->stage_timer_bcd = 0x60u;
                        game->timer_frames = 256u;
                    }
                    const uint8_t* mus_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_MUSIC_OPTIONS, 8u);
                    if (mus_tbl) {
                        queue_sound(game, (MjkSoundId)mus_tbl[game->room.music_id & 7u]);
                    }
                    MjkPlayer_Init(&game->player, game->jack_x, game->jack_y);
                    game->state = MJK_GS_MAIN;
                    game->sub_state = 0u;
                }
            }
            break;

        case MJK_GS_MAIN: {
            if (input && (input->just_pressed & MJK_BTN_START) != 0u) {
                game->paused = !game->paused;
                queue_sound(game, game->paused ? MJK_SND_PAUSE : MJK_SND_UNPAUSE);
            }
            if (game->paused) {
                break;
            }
            if (game->torture_scene_flag != 0u) {
                game->timer_status |= 0x40u;
            }
            MjkSoundId sfx = MJK_SND_SILENCE;
            bool add_10 = false;
            MjkActionResult p_res = MjkPlayer_Update(
                &game->player, input, game->pkg, &game->room,
                (game->camera.neighbor_room_id != 0u) ? &game->right_room : NULL,
                game->camera.room_flags,
                (uint16_t)(game->camera.scroll_x_fixed >> 8),
                (uint16_t)(game->camera.scroll_y_fixed >> 8),
                game->torture_scene_flag != 0u,
                &game->mighty_level, &game->mighty_presses_left, &game->mighty_coins,
                &sfx, &add_10);
            game->jack_x = MjkUQ8_Int(game->player.pos_x);
            game->jack_y = MjkUQ8_Int(game->player.pos_y);
            if (sfx != MJK_SND_SILENCE) {
                queue_sound(game, sfx);
            }
            if (add_10) {
                MjkGame_AddScore(game, MJK_SCORE_10);
            }
            if (game->player.just_jumped && game->torture_scene_flag != 0u) {
                MjkGame_OnTortureJump(game);
            }
            if (p_res == MJK_ACTION_PLAYER_DIED) {
                if (game->player.fire_tile_died_to == 0x5Du && game->current_room_id == 0xFCu) {
                    game->door_1e_deaths++;
                }
                MjkGame_LoseLife(game);
                break;
            }

            MjkCamera_Update(&game->camera, &game->player, game->pkg);
            game->jack_x = MjkUQ8_Int(game->player.pos_x);
            game->ppu.scroll_x = (uint16_t)(game->camera.scroll_x_fixed >> 8);
            if (game->camera.room_crossed) {
                game->current_room_id = game->camera.current_room_id;
                MjkRoom_DecodeGeometry(game->pkg, game->current_room_id, &game->room);
                MjkRoom_DecodeObjects(game->pkg, game->current_room_id, &game->persist, &game->room);
                MjkRoom_BlitToPpu(&game->room, game->pkg, &game->ppu, 0u);
                if (game->camera.neighbor_room_id != 0u) {
                    MjkRoom_DecodeGeometry(game->pkg, game->camera.neighbor_room_id, &game->right_room);
                    MjkRoom_DecodeObjects(game->pkg, game->camera.neighbor_room_id, &game->persist, &game->right_room);
                    MjkRoom_BlitToPpu(&game->right_room, game->pkg, &game->ppu, 1u);
                }
            }

            MjkHud_TickFloating(game->popups, MJK_FLOAT_SCORE_SLOTS,
                                game->camera.delta_cam_x, game->camera.delta_cam_y);
            if (MjkObjects_Update(&game->objects, game, input) != MJK_ACTION_CONTINUE) {
                break;
            }
            if (MjkDoors_Update(game) || MjkDoors_CheckPlayerEntry(game) != MJK_ACTION_CONTINUE) {
                break;
            }
            if (MjkEnemy_UpdateAll(&game->enemies, game) != MJK_ACTION_CONTINUE) {
                break;
            }
            if (MjkHud_TickStageTimer(game)) {
                game->state = MJK_GS_TIME_OVER;
                game->sub_state = 0u;
                break;
            }

            MjkPpu_ClearOam(&game->ppu);
            uint8_t slot = MjkPpu_WriteMetasprite(&game->ppu, game->pkg,
                                                  game->player.sprite_id,
                                                  game->jack_x, game->jack_y,
                                                  (uint8_t)(game->player.facing_attr | 0x01u), 0u);
            slot = MjkHud_RenderOam(game, slot);
            slot = MjkObjects_RenderOam(&game->objects, game->pkg, &game->ppu, slot);
            MjkEnemy_RenderOam(&game->enemies, game->pkg, &game->ppu, slot);
            break;
        }

        case MJK_GS_TIME_OVER:
            if (game->sub_state == 0u) {
                game->timer_status |= 0x80u;
                game->state_timer = 0x80u; /* 128f */
                MjkPpu_ClearNametable(&game->ppu, 0x24u);
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_TIME_OVER);
                queue_sound(game, MJK_SND_DEATH);
                game->sub_state = 1u;
            } else {
                if (game->state_timer > 0u) {
                    game->state_timer--;
                }
                if (game->state_timer == 0u) {
                    MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
                    MjkGame_LoseLife(game);
                }
            }
            break;

        case MJK_GS_TORTURE_ROOM:
            if (game->sub_state == 0u) {
                game->torture_scene_flag = 1u;
                MjkPpu_ClearOam(&game->ppu);
                MjkPpu_ClearNametable(&game->ppu, 0x24u);
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_YOU_ARE_GREEDY);
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_GO_TO_TORTURE_ROOM);
                game->state_timer = 0xC0u; /* 192f */
                queue_sound(game, MJK_SND_SILENCE);
                queue_sound(game, MJK_SND_YOU_ARE_GREEDY);
                game->sub_state = 1u;
            } else {
                if (game->state_timer > 0u) {
                    game->state_timer--;
                }
                if (game->state_timer == 0u) {
                    game->torture_scene_flag |= 2u;
                    game->current_room_id = 0xACu;
                    game->greedy_jumps_bcd = 0x50u;
                    game->state = MJK_GS_LOAD_ROOM;
                    game->sub_state = 0u;
                }
            }
            break;

        case MJK_GS_ROUND_CLEAR:
            MjkState_StepRoundClear(game);
            break;

        case MJK_GS_EXIT_ROOM:
            MjkGame_ExitRoom(game, (uint8_t)(game->entry_door_type >> 2u));
            break;

        case MJK_GS_GAME_OVER:
            MjkState_StepGameOver(game);
            break;

        case MJK_GS_ANIMATE_DOOR_OPEN:
            if (game->sub_state == 0u) {
                queue_sound(game, MJK_SND_DOOR);
                game->state_timer = 72u;
                game->sub_state = 1u;
            } else {
                if (game->state_timer > 0u) {
                    game->state_timer--;
                }
                if (game->state_timer == 0u) {
                    game->state = MJK_GS_MAIN;
                    game->sub_state = 0u;
                }
            }
            break;

        case MJK_GS_ENDING:
            MjkState_StepEnding(game, input);
            break;

        default:
            game->state = MJK_GS_INIT;
            break;
    }
}

void MjkGame_Render(const MjkGame* game, uint32_t* out_fb_argb8888) {
    if (!game || !game->pkg || !out_fb_argb8888) {
        return;
    }
    MjkPpu_RenderFrame(&game->ppu, game->pkg, out_fb_argb8888);
}
