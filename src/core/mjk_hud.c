/**
 * @file mjk_hud.c
 * @brief Implementación de puntuación BCD, StageTimer y HUD OAM (asm/src/prg.asm:1191-1237, 1783-1933, 2436-2573).
 */

#include "mjk_hud.h"
#include "mjk_game.h"
#include <string.h>

uint8_t MjkBcd_Sub1(uint8_t bcd) {
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

static uint32_t score_in_units_of_10(const uint8_t s[4]) {
    uint32_t v = 0u;
    for (int i = 3; i >= 0; --i) {
        v = v * 100u + (uint32_t)MjkBcd_ToBinary(s[i]);
    }
    return v; /* cada unidad = 10 pts (el dígito de unidades siempre es 0) */
}

void MjkHud_AddScore(MjkGame* game,
                     MjkScoreId score_id,
                     bool* out_spawn_bonus_coin,
                     bool* out_spawn_extra_coin) {
    if (out_spawn_bonus_coin) { *out_spawn_bonus_coin = false; }
    if (out_spawn_extra_coin) { *out_spawn_extra_coin = false; }
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

    /* Cada 5000 pts en Bomb Room: Bonus Coin (o Extra Coin con >= 40 Gold Coins). */
    uint32_t mark = score_in_units_of_10(game->score_bcd) / 500u;
    if (mark > game->score_5000_mark) {
        game->score_5000_mark = mark;
        if (game->room.is_bomb_room && game->score_multiplier < 4u) {
            if (game->gold_coins >= 40u) {
                if (out_spawn_extra_coin) { *out_spawn_extra_coin = true; }
            } else if (out_spawn_bonus_coin) {
                *out_spawn_bonus_coin = true;
            }
        }
    }
}

bool MjkHud_TickStageTimer(MjkGame* game) {
    if (!game || (game->timer_status & 0x40u) != 0u || game->stage_timer_bcd == 0u) {
        return false;
    }
    if (game->timer_frames > 0u) {
        game->timer_frames--;
    }
    if (game->timer_frames == 0u) {
        game->timer_frames = 256u;
        game->stage_timer_bcd = MjkBcd_Sub1(game->stage_timer_bcd);
        return game->stage_timer_bcd == 0u;
    }
    return false;
}

void MjkHud_SpawnFloatingScore(MjkFloatingScore* popups,
                               size_t max_popups,
                               uint8_t x,
                               uint8_t y,
                               MjkScoreId score_id,
                               uint8_t mult,
                               bool is_bomb_room) {
    /* Tiles PT0 0x30..0x3F (asm/src/prg.asm:2551-2573, RES-034). */
    static const uint8_t k_hi_tile[32] = {
        0x36u, 0x31u, 0x32u, 0x33u, 0x34u, 0x35u, 0x37u, 0x38u,
        0x31u, 0x36u, 0x36u, 0x3Au, 0x38u, 0x39u, 0x3Au, 0x3Au,
        0x34u, 0x35u, 0x35u, 0x31u, 0x31u, 0x32u, 0x31u, 0x32u,
        0x33u, 0x34u, 0x33u, 0x34u, 0x35u, 0x38u, 0x31u, 0x31u
    };
    if (!popups || max_popups == 0u) {
        return;
    }
    size_t chosen = 0u;
    for (size_t i = 0u; i < max_popups; ++i) {
        if (!popups[i].active) {
            chosen = i;
            break;
        }
        if (popups[i].timer < popups[chosen].timer) {
            chosen = i;
        }
    }
    popups[chosen].active = true;
    popups[chosen].timer = MJK_FLOAT_SCORE_FRAMES;
    popups[chosen].tl_tile = k_hi_tile[(uint8_t)score_id & 31u];
    popups[chosen].tr_tile = 0x30u;
    popups[chosen].br_tile = (is_bomb_room && mult >= 1u && mult <= 4u)
        ? (uint8_t)(0x3Bu + mult) : 0x0Fu;
    popups[chosen].x = MjkUQ8_FromInt(x);
    popups[chosen].y = MjkUQ8_FromInt(y);
}

void MjkHud_TickFloating(MjkFloatingScore* popups,
                         size_t max_popups,
                         int16_t delta_cam_x,
                         int16_t delta_cam_y) {
    if (!popups) {
        return;
    }
    for (size_t i = 0u; i < max_popups; ++i) {
        if (!popups[i].active) {
            continue;
        }
        int32_t nx = (int32_t)popups[i].x + (int32_t)delta_cam_x * 256;
        int32_t ny = (int32_t)popups[i].y + (int32_t)delta_cam_y * 256;
        if (nx < 0 || nx >= 0x10000 || ny < 0 || ny >= 0x10000) {
            popups[i].active = false;
            continue;
        }
        popups[i].x = (mjk_uq8_8_t)nx;
        popups[i].y = (mjk_uq8_8_t)ny;
        if (popups[i].timer > 0u) {
            popups[i].timer--;
        }
        if (popups[i].timer == 0u) {
            popups[i].active = false;
        }
    }
}

uint8_t MjkHud_RenderOam(MjkGame* game, uint8_t start_slot) {
    if (!game) {
        return start_slot;
    }
    uint8_t slot = MjkPpu_DrawHudScore(&game->ppu, game->score_bcd, start_slot);
    slot = MjkPpu_DrawHudTimer(&game->ppu,
                               (game->torture_scene_flag != 0u) ? game->greedy_jumps_bcd
                                                                : game->stage_timer_bcd,
                               slot);
    slot = MjkPpu_DrawHudMightyOrMult(&game->ppu, game->room.is_bomb_room,
                                      game->room.is_bomb_room ? game->score_multiplier
                                                              : game->mighty_coins,
                                      slot);
    for (size_t i = 0u; i < MJK_FLOAT_SCORE_SLOTS; ++i) {
        const MjkFloatingScore* p = &game->popups[i];
        if (!p->active || (size_t)slot + 4u > MJK_PPU_OAM_COUNT) {
            continue;
        }
        uint8_t px = MjkUQ8_Int(p->x);
        uint8_t py = MjkUQ8_Int(p->y);
        MjkPpu_SetOam(&game->ppu, slot++, px, py, p->tl_tile, 0u);
        MjkPpu_SetOam(&game->ppu, slot++, (uint8_t)(px + 8u), py, p->tr_tile, 0u);
        MjkPpu_SetOam(&game->ppu, slot++, px, (uint8_t)(py + 8u), 0x0Fu, 0u);
        MjkPpu_SetOam(&game->ppu, slot++, (uint8_t)(px + 8u), (uint8_t)(py + 8u), p->br_tile, 0u);
    }
    return slot;
}
