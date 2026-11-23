/**
 * @file mjk_hud.h
 * @brief Puntuación BCD little-endian, StageTimer, HUD OAM y puntuación flotante (T-24).
 */

#ifndef MJK_HUD_H
#define MJK_HUD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_ppu.h"
#include "mjk_types.h"

struct MjkGame;

#define MJK_FLOAT_SCORE_SLOTS 4u
#define MJK_FLOAT_SCORE_FRAMES 32u

typedef struct {
    bool        active;
    uint8_t     timer;
    uint8_t     tl_tile;
    uint8_t     tr_tile;
    uint8_t     br_tile;
    mjk_uq8_8_t x;
    mjk_uq8_8_t y;
} MjkFloatingScore;

/** @brief Resta 1 a un byte BCD empaquetado (0x00 se queda en 0x00). */
MJK_CORE_API uint8_t MjkBcd_Sub1(uint8_t bcd);

/**
 * @brief Suma `score_id` a `score_bcd[0..3]` (AddScore, asm/src/prg.asm:2436-2490).
 * Aplica `MultipliedScoreTable` en Bomb Room y avisa cada 5000 pts (prg.asm:2493-2542).
 */
MJK_CORE_API void MjkHud_AddScore(struct MjkGame* game,
                                  MjkScoreId score_id,
                                  bool* out_spawn_bonus_coin,
                                  bool* out_spawn_extra_coin);

/** @brief Avanza StageTimer (divisor 256f). Devuelve `true` si llega a 0 (TimeOver). */
MJK_CORE_API bool MjkHud_TickStageTimer(struct MjkGame* game);

MJK_CORE_API void MjkHud_SpawnFloatingScore(MjkFloatingScore* popups,
                                            size_t max_popups,
                                            uint8_t x,
                                            uint8_t y,
                                            MjkScoreId score_id,
                                            uint8_t mult,
                                            bool is_bomb_room);

MJK_CORE_API void MjkHud_TickFloating(MjkFloatingScore* popups,
                                      size_t max_popups,
                                      int16_t delta_cam_x,
                                      int16_t delta_cam_y);

/** @brief Dibuja Score/Timer/Mighty (+ puntuaciones flotantes). Devuelve el siguiente slot OAM. */
MJK_CORE_API uint8_t MjkHud_RenderOam(struct MjkGame* game, uint8_t start_slot);

#endif /* MJK_HUD_H */
