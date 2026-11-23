/**
 * @file mjk_end_screens.c
 * @brief Estados finales de ronda/partida: RoundClear (8), GameOver (A) y Ending (C).
 */

#include "mjk_game.h"
#include <string.h>

static int compare_score_bcd(const uint8_t a[4], const uint8_t b[4]) {
    for (int i = 3; i >= 0; --i) {
        if (a[i] > b[i]) { return 1; }
        if (a[i] < b[i]) { return -1; }
    }
    return 0;
}

void MjkState_StepRoundClear(MjkGame* game) {
    static const uint8_t k_fire_bomb_bonus[4] = { 0x10u, 0x20u, 0x30u, 0x50u };
    switch (game->sub_state) {
        case 0u: {
            game->ppu.scroll_x = 0u;
            game->ppu.scroll_y = 0u;
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
            MjkGame_QueueSound(game, MJK_MUS_ROUND_CLEAR);
            game->sub_state = 1u;
            break;
        }
        case 1u:
            if (game->state_timer > 0u) {
                game->state_timer--;
                break;
            }
            if (game->stage_timer_copy > 0u) {
                game->stage_timer_copy = MjkBcd_Sub1(game->stage_timer_copy);
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
                game->fire_bomb_bonus_bcd = MjkBcd_Sub1(game->fire_bomb_bonus_bcd);
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
            game->door_1e_deaths = 0u;
            game->timer_status &= 0xBFu;
            MjkGame_QueueSound(game, MJK_SND_SILENCE);
            if (game->fire_bombs_collected == 1u && game->last_bomb_room_cleared != 0x9Fu) {
                uint8_t next = (uint8_t)(game->last_bomb_room_cleared + 1u);
                game->current_room_id = next;
                const uint8_t* entry_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_MAYBE_ENTRY_TYPE, 32u);
                if (entry_tbl) {
                    game->entry_door_type = entry_tbl[(next & 0x0Fu) * 2u];
                }
                game->timer_status = 0u;
            }
            game->state = MJK_GS_LOAD_ROOM;
            game->sub_state = 0u;
            break;
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

static void resolve_entry_door(MjkGame* game, uint8_t next_room, uint8_t exit_door) {
    uint8_t target = (uint8_t)(exit_door ^ 0x04u);
    uint8_t alt = (uint8_t)(exit_door ^ 0x06u);
    game->entry_door_type = target;
    game->entry_door_already_open = false;
    if (next_room == 0u) {
        return;
    }
    MjkRoom_PreloadSection(game->pkg, next_room, &game->persist, game->crystal_balls_mask);
    MjkRoomState tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.is_bomb_room = (next_room >= 0x90u && next_room <= 0x9Fu);
    if (!MjkRoom_DecodeObjects(game->pkg, next_room, &game->persist, &tmp)) {
        return;
    }
    const MjkRoomObject* found = NULL;
    for (uint8_t i = 0u; i < tmp.object_count; ++i) {
        const MjkRoomObject* ro = &tmp.objects[i];
        if ((ro->opcode & 0xF0u) != 0x80u) {
            continue;
        }
        uint8_t d = (uint8_t)(ro->opcode & 0x0Fu);
        if (d == target) {
            found = ro;
            break;
        }
        if (d == alt && !found) {
            found = ro;
        }
    }
    if (found) {
        game->entry_door_type = (uint8_t)(found->opcode & 0x0Fu);
        game->entry_door_already_open = ((found->ram_state & 0x40u) != 0u);
    }
}

void MjkGame_ExitRoom(MjkGame* game, uint8_t exit_door_dir) {
    if (!game || !game->pkg) {
        return;
    }
    uint8_t prev_room = (game->exit_source_room_id != 0u) ? game->exit_source_room_id : game->current_room_id;
    game->exit_source_room_id = 0u;
    const uint8_t* adj_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_ADJACENT_ROOMS, 256u * 5u);
    uint8_t next_room = prev_room;
    if (adj_tbl) {
        const uint8_t* entry = adj_tbl + ((size_t)prev_room * 5u);
        next_room = entry[1u + (exit_door_dir & 3u)];
    }

    uint8_t exit_door = (uint8_t)(game->entry_door_type & 0x0Fu);
    game->current_room_id = next_room;
    resolve_entry_door(game, next_room, exit_door);
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

void MjkState_StepGameOver(MjkGame* game) {
    if (game->sub_state == 0u) {
        game->ppu.scroll_x = 0u;
        game->ppu.scroll_y = 0u;
        MjkPpu_ClearOam(&game->ppu);
        MjkPpu_ClearNametable(&game->ppu, 0x24u);
        MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
        MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_GAME_OVER);
        MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_YOUR_GDV);
        MjkGame_QueueSound(game, MJK_MUS_GAME_OVER);

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
        game->ppu.scroll_x = 0u;
        game->ppu.scroll_y = 0u;
        game->top_mode = MJK_MODE_TITLE;
        MjkTitle_Init(&game->title);
        MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
    }
}

void MjkState_StepEnding(MjkGame* game, const MjkInputState* input) {
    static const uint8_t k_destroy_depth[4] = { 12u, 30u, 56u, 90u };
    static const MjkStringId k_end_str_start[4] = {
        MJK_STR_ENDING_1A, MJK_STR_ENDING_2A, MJK_STR_ENDING_3A, MJK_STR_ENDING_4A
    };
    static const uint8_t k_end_str_count[4] = { 3u, 3u, 4u, 4u };

    switch (game->ending_state) {
        case 0u:
            game->ppu.scroll_x = 0u;
            game->ppu.scroll_y = 0u;
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
                MjkGame_QueueSound(game, MJK_SND_BOMB_BONUS);
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
            MjkGame_QueueSound(game, MJK_MUS_ENDING);
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
