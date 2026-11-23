/**
 * @file mjk_state_machine.c
 * @brief Máquina de estados completa `GameHandler` 0..C (REV_US).
 */

#include "mjk_game.h"
#include <string.h>

static void queue_sound(MjkGame* game, MjkSoundId snd) {
    game->last_sound_event = snd;
    if (game->sound_queue_count < MJK_GAME_SOUND_QUEUE_CAP) {
        game->sound_queue[game->sound_queue_count++] = snd;
    }
}

static uint8_t bcd_sub1(uint8_t bcd) {
    uint8_t lo = (uint8_t)(bcd & 0x0Fu);
    uint8_t hi = (uint8_t)((bcd >> 4u) & 0x0Fu);
    if (lo > 0u) {
        return (uint8_t)((hi << 4u) | (lo - 1u));
    }
    if (hi > 0u) {
        return (uint8_t)(((hi - 1u) << 4u) | 9u);
    }
    return 0u;
}

static int compare_score_bcd(const uint8_t a[4], const uint8_t b[4]) {
    for (int i = 3; i >= 0; --i) {
        if (a[i] > b[i]) { return 1; }
        if (a[i] < b[i]) { return -1; }
    }
    return 0;
}

uint8_t MjkGame_AddGdvBcd(uint8_t current_gdv_bcd, uint8_t delta) {
    uint8_t cur = MjkBcd_ToBinary(current_gdv_bcd);
    unsigned sum = (unsigned)cur + (unsigned)delta;
    if (sum > 99u) {
        sum = 99u;
    }
    return MjkBcd_FromBinary((uint8_t)sum);
}

void MjkGame_AddScore(MjkGame* game, MjkScoreId score_id) {
    if (!game || !game->pkg) {
        return;
    }
    uint8_t idx = (uint8_t)score_id;
    if (idx < 8u && game->score_multiplier > 0u && game->room.is_bomb_room) {
        const uint8_t* mult_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_MULTIPLIED_SCORE, 32u);
        if (mult_tbl) {
            uint8_t m_off = (uint8_t)((idx * 4u) + ((game->score_multiplier - 1u) & 3u));
            idx = mult_tbl[m_off];
        }
    }
    const uint8_t* add_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_SCORE_ADD_TABLE, 64u);
    if (!add_tbl || idx >= 32u) {
        return;
    }
    uint8_t byte_idx = add_tbl[idx * 2u];
    uint8_t bcd_val = add_tbl[idx * 2u + 1u];
    if (byte_idx >= 4u) {
        return;
    }
    uint8_t carry = 0u;
    game->score_bcd[byte_idx] = MjkBcd_AddByte(game->score_bcd[byte_idx], bcd_val, 0u, &carry);
    for (uint8_t i = (uint8_t)(byte_idx + 1u); i < 4u && carry != 0u; ++i) {
        game->score_bcd[i] = MjkBcd_AddByte(game->score_bcd[i], 0u, carry, &carry);
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

static void step_round_clear(MjkGame* game) {
    static const uint8_t k_fire_bomb_bonus[4] = { 0x10u, 0x20u, 0x30u, 0x50u };
    switch (game->sub_state) {
        case 0u: {
            game->timer_status |= 0x40u;
            game->state_timer = 0x40u;
            MjkPpu_ClearOam(&game->ppu);
            MjkPpu_ClearNametable(&game->ppu, 0x24u);
            MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_ROUND_CLEAR);
            MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_TIME_BONUS);
            game->stage_timer_copy = game->stage_timer_bcd;
            uint8_t rnum = MjkBcd_FromBinary((uint8_t)((game->last_bomb_room_cleared & 0x0Fu) + 1u));
            MjkPpu_DrawBcdByte(&game->ppu, rnum, 0x84u, 0x44u, true, 0u);

            if (game->fire_bombs_collected >= 20u) {
                uint8_t idx = (uint8_t)(game->fire_bombs_collected - 20u);
                if (idx > 3u) { idx = 3u; }
                game->fire_bomb_bonus_bcd = k_fire_bomb_bonus[idx];
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_YOUVE_GOTTEN);
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_FIRE_BOMBS);
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_SPECIAL_BONUS);
                game->gs8_status |= 2u;
            }
            queue_sound(game, MJK_MUS_ROUND_CLEAR);
            game->sub_state = 1u;
            break;
        }
        case 1u:
            if (game->state_timer > 0u) {
                game->state_timer--;
                break;
            }
            if (game->stage_timer_copy > 0u) {
                game->stage_timer_copy = bcd_sub1(game->stage_timer_copy);
                MjkGame_AddScore(game, MJK_SCORE_100);
                game->state_timer = 6u;
            } else {
                game->state_timer = 64u;
                game->sub_state = ((game->gs8_status & 2u) != 0u) ? 2u : 3u;
            }
            break;
        case 2u:
            if (game->state_timer > 0u) {
                game->state_timer--;
                break;
            }
            if (game->fire_bomb_bonus_bcd > 0u) {
                game->fire_bomb_bonus_bcd = bcd_sub1(game->fire_bomb_bonus_bcd);
                MjkGame_AddScore(game, MJK_SCORE_1000);
                game->state_timer = 6u;
            } else {
                game->state_timer = 64u;
                game->sub_state = 3u;
            }
            break;
        default:
            if (game->state_timer > 0u) {
                game->state_timer--;
                break;
            }
            game->collected_1up = 0u;
            game->score_multiplier = 0u;
            game->timer_status &= 0xBFu;
            queue_sound(game, MJK_SND_SILENCE);
            if (game->fire_bombs_collected == 1u && game->last_bomb_room_cleared != 0x9Fu) {
                game->current_room_id = (uint8_t)(game->last_bomb_room_cleared + 1u);
            }
            game->state = MJK_GS_LOAD_ROOM;
            game->sub_state = 0u;
            break;
    }
}

static void step_game_over(MjkGame* game) {
    if (game->sub_state == 0u) {
        MjkPpu_ClearOam(&game->ppu);
        MjkPpu_ClearNametable(&game->ppu, 0x24u);
        MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
        MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_GAME_OVER);
        MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_YOUR_GDV);
        queue_sound(game, MJK_MUS_GAME_OVER);

        game->gdv_bcd = MjkGame_AddGdvBcd(game->gdv_bcd, (uint8_t)((game->last_bomb_room_cleared & 0x0Fu) >> 1u));
        game->gdv_bcd = MjkGame_AddGdvBcd(game->gdv_bcd, (uint8_t)(game->extra_lives_found >> 1u));
        uint8_t s = (game->score_bcd[3] != 0u) ? 0xA0u : game->score_bcd[2];
        game->gdv_bcd = MjkGame_AddGdvBcd(game->gdv_bcd, (uint8_t)(s >> 5u));

        if (game->gdv_bcd >= game->high_gdv_bcd) {
            game->high_gdv_bcd = game->gdv_bcd;
        }
        if (compare_score_bcd(game->score_bcd, game->high_score_bcd) > 0) {
            memcpy(game->high_score_bcd, game->score_bcd, 4u);
        }
        MjkPpu_DrawBcdByte(&game->ppu, game->gdv_bcd, 0xA4u, 0x84u, false, 0u);
        game->state_timer = 256u;
        game->sub_state = 1u;
        return;
    }
    if (game->state_timer > 0u) {
        game->state_timer--;
    }
    if (game->state_timer == 0u) {
        game->top_mode = MJK_MODE_TITLE;
        MjkTitle_Init(&game->title);
        MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
    }
}

static void step_ending(MjkGame* game, const MjkInputState* input) {
    static const uint8_t k_destroy_depth[4] = { 12u, 30u, 56u, 90u };
    static const MjkStringId k_end_str_start[4] = {
        MJK_STR_ENDING_1A, MJK_STR_ENDING_2A, MJK_STR_ENDING_3A, MJK_STR_ENDING_4A
    };
    static const uint8_t k_end_str_count[4] = { 3u, 3u, 4u, 4u };

    switch (game->ending_state) {
        case 0u:
            MjkPpu_ClearOam(&game->ppu);
            if (MjkRoom_DecodeGeometry(game->pkg, 0x00u, &game->room)) {
                MjkRoom_BlitToPpu(&game->room, game->pkg, &game->ppu, 0u);
            }
            game->jack_x = 0x78u;
            game->jack_y = 0xE8u;
            game->ending_state = 1u;
            break;
        case 1u:
            game->ending_depth = k_destroy_depth[game->ending_type & 3u];
            game->state_timer = 8u;
            game->ending_state = 2u;
            break;
        case 2u:
            if (game->state_timer > 0u) {
                game->state_timer--;
                break;
            }
            if (game->ending_depth > 0u) {
                game->ending_depth--;
                queue_sound(game, MJK_SND_BOMB_BONUS);
                game->state_timer = 8u;
            } else {
                game->ending_state = 3u;
            }
            break;
        case 3u: {
            uint8_t et = game->ending_type & 3u;
            for (uint8_t i = 0u; i < k_end_str_count[et]; ++i) {
                MjkPpu_WriteString(&game->ppu, game->pkg, (MjkStringId)(k_end_str_start[et] + i));
            }
            queue_sound(game, MJK_MUS_ENDING);
            game->ending_state = 4u;
            break;
        }
        default:
            if (input && (input->just_pressed & MJK_BTN_START) != 0u) {
                MjkTitle_Init(&game->title);
                game->state = MJK_GS_GAME_OVER;
                game->sub_state = 0u;
            }
            break;
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
                uint8_t slot = MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0u, 0x70u, 0x60u, 0x00u, 0u);
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
            game->state = MJK_GS_ENTER_ROOM;
            game->sub_state = 0u;
            break;

        case MJK_GS_ENTER_ROOM:
            if (game->sub_state == 0u) {
                MjkPpu_LoadRoomPalette(&game->ppu, game->pkg, game->room.palette_set);
                const uint8_t* pos_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_DOOR_ENTRY_XY, 32u);
                uint8_t idx = (uint8_t)(game->entry_door_type & 0x0Fu);
                game->jack_x = pos_tbl ? pos_tbl[idx * 2u] : 0x80u;
                game->jack_y = pos_tbl ? pos_tbl[idx * 2u + 1u] : 0x78u;
                game->state_timer = 0x18u; /* 24f */
                game->sub_state = 1u;
            } else {
                const uint8_t* vec_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_VECTOR_TABLE, 8u);
                uint8_t dir = (uint8_t)(game->entry_door_type & 3u);
                if (vec_tbl) {
                    game->jack_x = (uint8_t)(game->jack_x + (int8_t)vec_tbl[dir * 2u]);
                    game->jack_y = (uint8_t)(game->jack_y + (int8_t)vec_tbl[dir * 2u + 1u]);
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
            if ((game->timer_status & 0x40u) == 0u && game->stage_timer_bcd > 0u) {
                if (game->timer_frames > 0u) {
                    game->timer_frames--;
                }
                if (game->timer_frames == 0u) {
                    game->timer_frames = 256u;
                    game->stage_timer_bcd = bcd_sub1(game->stage_timer_bcd);
                    if (game->stage_timer_bcd == 0u) {
                        game->state = MJK_GS_TIME_OVER;
                        game->sub_state = 0u;
                        break;
                    }
                }
            }
            uint8_t slot = MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0u, game->jack_x, game->jack_y, 0u, 0u);
            slot = MjkPpu_DrawHudScore(&game->ppu, game->score_bcd, slot);
            if (game->torture_scene_flag != 0u) {
                slot = MjkPpu_DrawBcdByte(&game->ppu, game->greedy_jumps_bcd, 0x84u, 0x44u, true, slot);
            } else {
                slot = MjkPpu_DrawHudTimer(&game->ppu, game->stage_timer_bcd, slot);
            }
            MjkPpu_DrawHudMightyOrMult(&game->ppu, game->room.is_bomb_room,
                                       game->room.is_bomb_room ? game->score_multiplier : game->mighty_coins, slot);
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
            step_round_clear(game);
            break;

        case MJK_GS_EXIT_ROOM:
            MjkGame_ExitRoom(game, (uint8_t)(game->entry_door_type >> 2u));
            break;

        case MJK_GS_GAME_OVER:
            step_game_over(game);
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
            step_ending(game, input);
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
