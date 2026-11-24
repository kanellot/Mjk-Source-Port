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

static void render_round_clear_oam(MjkGame* game) {
    MjkPpu_ClearOam(&game->ppu);
    uint8_t slot = MjkPpu_DrawHudScore(&game->ppu, game->score_bcd, 0u);
    /* DrawRoundIntroSprites (prg.asm:804, 7682-7700): Jack x Lives */
    slot = MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0u, 0x70u, 0x60u, 0x01u, slot);
    MjkPpu_SetOam(&game->ppu, slot++, 0x80u, 0x5Cu, 0x0Cu, 0u);
    slot = MjkPpu_DrawBcdByte(&game->ppu, MjkBcd_FromBinary(game->lives), 0x94u, 0x60u, true, slot);
    /* Número de ronda "ROUND X CLEAR" (prg.asm:820-833) */
    uint8_t rnum = MjkBcd_FromBinary((uint8_t)((game->last_bomb_room_cleared & 0x0Fu) + 1u));
    slot = MjkPpu_DrawBcdByte(&game->ppu, rnum, 0x84u, 0x44u, true, slot);
    /* Time Bonus: dígitos BCD + "00" (prg.asm:816-819, 1008-1018) */
    slot = MjkPpu_DrawBcdByte(&game->ppu, game->stage_timer_copy, 0xACu, 0x7Cu, true, slot);
    MjkPpu_SetOam(&game->ppu, slot++, 0xB0u, 0x78u, 0x00u, 0u);
    MjkPpu_SetOam(&game->ppu, slot++, 0xB8u, 0x78u, 0x00u, 0u);
    /* Special Bonus si >= 20 Fire Bombs (prg.asm:835-866, 1022-1032) */
    if ((game->gs8_status & 2u) != 0u) {
        uint8_t fb_bcd = MjkBcd_FromBinary(game->fire_bombs_collected);
        slot = MjkPpu_DrawBcdByte(&game->ppu, fb_bcd, 0x54u, 0xACu, true, slot);
        slot = MjkPpu_DrawBcdByte(&game->ppu, game->fire_bomb_bonus_bcd, 0xB4u, 0xC4u, true, slot);
        MjkPpu_SetOam(&game->ppu, slot++, 0xB8u, 0xC0u, 0x00u, 0u);
        MjkPpu_SetOam(&game->ppu, slot++, 0xC0u, 0xC0u, 0x00u, 0u);
        MjkPpu_SetOam(&game->ppu, slot++, 0xC8u, 0xC0u, 0x00u, 0u);
    }
}

void MjkState_StepRoundClear(MjkGame* game) {
    static const uint8_t k_fire_bomb_bonus[4] = { 0x10u, 0x20u, 0x30u, 0x50u };
    switch (game->sub_state) {
        case 0u: {
            /* RoundClear_0 (prg.asm:794-800): limpiar RF_BombRoom antes de contar bonus. */
            game->room.is_bomb_room = false;
            game->ppu.scroll_x = 0u;
            game->ppu.scroll_y = 0u;
            game->timer_status |= 0x40u;
            game->state_timer = 0x40u;
            MjkPpu_RestoreDefaultPalettes(&game->ppu, game->pkg);
            MjkPpu_ClearNametable(&game->ppu, 0x24u);
            MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_ROUND_CLEAR);
            MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_TIME_BONUS);
            game->stage_timer_copy = game->stage_timer_bcd;

            if (game->fire_bombs_collected >= 20u) {
                uint8_t idx = (uint8_t)(game->fire_bombs_collected - 20u);
                if (idx > 3u) { idx = 3u; }
                game->fire_bomb_bonus_bcd = k_fire_bomb_bonus[idx];
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_YOUVE_GOTTEN);
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_FIRE_BOMBS);
                MjkPpu_WriteString(&game->ppu, game->pkg, MJK_STR_SPECIAL_BONUS);
                game->gs8_status |= 2u;
            }
            render_round_clear_oam(game);
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
                render_round_clear_oam(game);
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
                render_round_clear_oam(game);
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
        MjkPpu_DrawBcdByte(&game->ppu, game->gdv_bcd, 0xA4u, 0x84u, true, 0u);
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
    static const uint8_t k_end_jack_y[4] = { 0x90u, 0xA0u, 0xB0u, 0xC0u };
    static const MjkStringId k_end_str_start[4] = {
        MJK_STR_ENDING_1A, MJK_STR_ENDING_2A, MJK_STR_ENDING_3A, MJK_STR_ENDING_4A
    };
    static const uint8_t k_end_str_count[4] = { 3u, 3u, 4u, 4u };
    uint8_t et = game->ending_type & 3u;

    switch (game->ending_state) {
        case 0u:
            game->ppu.scroll_x = 0u;
            game->ppu.scroll_y = 0u;
            MjkPpu_ClearOam(&game->ppu);
            if (MjkRoom_DecodeGeometry(game->pkg, 0x00u, &game->room)) {
                MjkRoom_BlitToPpu(&game->room, game->pkg, &game->ppu, 0u);
            }
            /* EndingState_0 (prg.asm:7719-7760): limpiar las 16 filas superiores (logo de título). */
            for (uint8_t row = 0u; row < 16u; ++row) {
                for (uint8_t col = 0u; col < MJK_PPU_NT_COLS; ++col) {
                    MjkPpu_SetTile(&game->ppu, 0u, col, row, 0x24u);
                }
            }
            game->jack_x = 0x78u;
            game->jack_y = 0xE8u;
            game->state_timer = 0x18u;
            game->ending_state = 1u;
            break;
        case 1u:
            /* EndingState_1: Jack sube saltando hacia la pirámide. */
            if (game->jack_y > 0x90u) {
                game->jack_y = (uint8_t)(game->jack_y - 2u);
            }
            MjkPpu_ClearOam(&game->ppu);
            MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0x01u, game->jack_x, game->jack_y, 0x01u, 0u);
            if (game->state_timer > 0u) {
                game->state_timer--;
                break;
            }
            game->ending_depth = k_destroy_depth[et];
            game->state_timer = 8u;
            game->ending_state = 2u;
            break;
        case 2u: {
            uint8_t total = k_destroy_depth[et];
            uint8_t done = (total >= game->ending_depth) ? (uint8_t)(total - game->ending_depth) : 0u;
            uint8_t carve_row = (uint8_t)(16u + (done / 10u));
            uint8_t jack_y = (uint8_t)(0x88u + (done / 2u));
            if (jack_y > k_end_jack_y[et]) {
                jack_y = k_end_jack_y[et];
            }
            game->jack_y = jack_y;
            MjkPpu_ClearOam(&game->ppu);
            uint8_t slot = MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0x01u, game->jack_x, game->jack_y, 0x01u, 0u);
            uint8_t debris_off = (uint8_t)((8u - game->state_timer) * 3u);
            slot = MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0x25u,
                                          (uint8_t)(0x68u - debris_off), (uint8_t)(game->jack_y + debris_off), 0x00u, slot);
            MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0x25u,
                                   (uint8_t)(0x88u + debris_off), (uint8_t)(game->jack_y + debris_off), 0x00u, slot);
            if (game->state_timer > 0u) {
                game->state_timer--;
                break;
            }
            if (game->ending_depth > 0u) {
                if (carve_row < MJK_PPU_NT_ROWS) {
                    uint8_t half_w = (uint8_t)(2u + (done / 8u));
                    if (half_w > 14u) { half_w = 14u; }
                    uint8_t c_start = (uint8_t)(16u - half_w);
                    uint8_t c_end = (uint8_t)(16u + half_w);
                    for (uint8_t c = c_start; c < c_end && c < MJK_PPU_NT_COLS; ++c) {
                        MjkPpu_SetTile(&game->ppu, 0u, c, carve_row, 0x24u);
                    }
                }
                game->ending_depth--;
                MjkGame_QueueSound(game, MJK_SND_BOMB_BONUS);
                game->state_timer = 8u;
            } else {
                game->ending_state = 3u;
            }
            break;
        }
        case 3u: {
            for (uint8_t i = 0u; i < k_end_str_count[et]; ++i) {
                MjkPpu_WriteString(&game->ppu, game->pkg, (MjkStringId)(k_end_str_start[et] + i));
            }
            game->jack_y = k_end_jack_y[et];
            MjkPpu_ClearOam(&game->ppu);
            MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0x00u, game->jack_x, game->jack_y, 0x01u, 0u);
            MjkGame_QueueSound(game, MJK_MUS_ENDING);
            game->ending_state = 4u;
            break;
        }
        default:
            MjkPpu_ClearOam(&game->ppu);
            MjkPpu_WriteMetasprite(&game->ppu, game->pkg, 0x00u, game->jack_x, k_end_jack_y[et], 0x01u, 0u);
            if (input && (input->just_pressed & MJK_BTN_START) != 0u) {
                MjkTitle_Init(&game->title);
                game->state = MJK_GS_GAME_OVER;
                game->sub_state = 0u;
            }
            break;
    }
}
