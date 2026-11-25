/**
 * @file scoring.c
 * @brief Implementacion de puntuacion en enteros estandar, GDV, StageTimer y popups.
 */

#include "mjk/scoring.h"
#include <string.h>

void MjkScoring_ResetNewGame(MjkScoring* sc) {
    if (!sc) {
        return;
    }
    int saved_high_gdv = sc->high_gdv;
    uint32_t saved_high_score = sc->high_score;
    memset(sc, 0, sizeof(*sc));
    sc->high_gdv      = saved_high_gdv;
    sc->high_score    = saved_high_score;
    sc->lives         = 3;
    sc->gdv           = 47;
    sc->timer_seconds = 60;
    sc->timer_frames  = MJK_TIMER_FRAMES_STEP;
}

void MjkScoring_ResetStageTimer(MjkScoring* sc, int start_seconds) {
    if (!sc) {
        return;
    }
    if (start_seconds < 0) { start_seconds = 0; }
    if (start_seconds > 99) { start_seconds = 99; }
    sc->timer_seconds = start_seconds;
    sc->timer_frames  = MJK_TIMER_FRAMES_STEP;
}

void MjkScoring_AddPoints(MjkScoring* sc,
                          const MjkAssets* assets,
                          MjkScoreId score_id,
                          bool is_bomb_room,
                          MjkScoreEvents* out_events) {
    if (out_events) {
        out_events->spawn_bonus_coin = false;
        out_events->spawn_extra_coin = false;
    }
    if (!sc || !assets) {
        return;
    }

    uint8_t idx = (uint8_t)score_id;
    if (idx < 8u && sc->score_multiplier > 0u && is_bomb_room) {
        uint8_t m_off = (uint8_t)((idx * 4u) + ((sc->score_multiplier - 1u) & 3u));
        idx = assets->multiplied_score[m_off];
    }
    if (idx >= 32u) {
        return;
    }

    uint32_t delta = assets->score_values[idx];
    if (sc->score > MJK_MAX_SCORE - delta) {
        sc->score = MJK_MAX_SCORE;
    } else {
        sc->score += delta;
    }
    if (sc->score > sc->high_score) {
        sc->high_score = sc->score;
    }

    /* Cada 5.000 puntos en Bomb Room aparece Bonus Coin (o Extra Coin si gold_coins >= 40). */
    uint32_t mark = sc->score / 5000u;
    if (mark > sc->bonus_5000_mark) {
        sc->bonus_5000_mark = mark;
        if (is_bomb_room && sc->score_multiplier < 4u) {
            if (sc->gold_coins >= 40u) {
                sc->gold_coins = 0u;
                if (out_events) { out_events->spawn_extra_coin = true; }
            } else if (out_events) {
                out_events->spawn_bonus_coin = true;
            }
        }
    }
}

int MjkScoring_AddGdv(MjkScoring* sc, int delta) {
    if (!sc) {
        return 0;
    }
    int next = sc->gdv + delta;
    if (next < 0) { next = 0; }
    if (next > MJK_MAX_GDV) { next = MJK_MAX_GDV; }
    sc->gdv = next;
    if (sc->gdv > sc->high_gdv) {
        sc->high_gdv = sc->gdv;
    }
    return sc->gdv;
}

bool MjkScoring_TickTimer(MjkScoring* sc) {
    if (!sc || sc->timer_frozen || sc->timer_seconds <= 0) {
        return false;
    }
    if (sc->timer_frames > 0) {
        sc->timer_frames--;
    }
    if (sc->timer_frames == 0) {
        sc->timer_frames = MJK_TIMER_FRAMES_STEP;
        sc->timer_seconds--;
        return (sc->timer_seconds == 0);
    }
    return false;
}

void MjkScoring_SpawnPopup(MjkScoring* sc,
                           float x,
                           float y,
                           MjkScoreId score_id,
                           uint8_t mult,
                           bool is_bomb_room) {
    /* Tiles PT0 0x30..0x3F para puntuaciones flotantes */
    static const uint8_t k_hi_tile[32] = {
        0x36u, 0x31u, 0x32u, 0x33u, 0x34u, 0x35u, 0x37u, 0x38u,
        0x31u, 0x36u, 0x36u, 0x3Au, 0x38u, 0x39u, 0x3Au, 0x3Au,
        0x34u, 0x35u, 0x35u, 0x31u, 0x31u, 0x32u, 0x31u, 0x32u,
        0x33u, 0x34u, 0x33u, 0x34u, 0x35u, 0x38u, 0x31u, 0x31u
    };
    if (!sc) {
        return;
    }
    size_t chosen = 0u;
    for (size_t i = 0u; i < MJK_FLOAT_POPUP_SLOTS; ++i) {
        if (!sc->popups[i].active) {
            chosen = i;
            break;
        }
        if (sc->popups[i].timer < sc->popups[chosen].timer) {
            chosen = i;
        }
    }

    uint8_t idx = (uint8_t)score_id;
    MjkFloatingPopup* p = &sc->popups[chosen];
    p->active  = true;
    p->timer   = MJK_FLOAT_POPUP_FRAMES;
    p->x       = x;
    p->y       = y;
    p->tl_tile = (idx < 32u) ? k_hi_tile[idx] : 0x31u;
    p->tr_tile = (idx >= 8u) ? 0x3Bu : 0x30u;
    p->br_tile = (is_bomb_room && mult >= 1u && mult <= 4u) ? (uint8_t)(0x3Bu + mult) : 0x0Fu;
}

void MjkScoring_TickPopups(MjkScoring* sc, float delta_cam_x, float delta_cam_y) {
    if (!sc) {
        return;
    }
    for (size_t i = 0u; i < MJK_FLOAT_POPUP_SLOTS; ++i) {
        MjkFloatingPopup* p = &sc->popups[i];
        if (!p->active) {
            continue;
        }
        p->x += delta_cam_x;
        p->y += delta_cam_y;
        if (p->timer > 0) {
            p->timer--;
        }
        if (p->timer <= 0) {
            p->active = false;
        }
    }
}
