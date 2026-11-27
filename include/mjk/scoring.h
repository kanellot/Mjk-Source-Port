/**
 * @file scoring.h
 * @brief Sistema de puntuacion, GDV, vidas, temporizador y popups flotantes.
 *
 * A diferencia del juego original de 1987 (que usaba BCD empaquetado para evitar
 * divisiones en el 6502), aqui todos los marcadores son enteros C estandar
 * (`uint32_t score`, `int gdv`, `int timer_seconds`, `int lives`). La conversion
 * a digitos solo ocurre en el renderizador al dibujar el HUD.
 */

#ifndef MJK_ENGINE_SCORING_H
#define MJK_ENGINE_SCORING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk/assets.h"
#include "mjk/types.h"

#define MJK_FLOAT_POPUP_SLOTS   4u
#define MJK_FLOAT_POPUP_FRAMES  32
#define MJK_TIMER_FRAMES_STEP   256
#define MJK_MAX_SCORE           99999999u
#define MJK_MAX_GDV             99

/** @brief Popup de puntuacion flotante (100, 200, x2..x5) que sube 32 frames. */
typedef struct {
    bool    active;
    int     timer;
    uint8_t tl_tile;
    uint8_t tr_tile;
    uint8_t br_tile;
    float   x;
    float   y;
} MjkFloatingPopup;

/** @brief Eventos generados al sumar puntos (aparicion de Bonus/Extra Coin). */
typedef struct {
    bool spawn_bonus_coin;
    bool spawn_extra_coin;
} MjkScoreEvents;

/** @brief Estado completo de marcador, temporizador y vidas de la partida. */
typedef struct {
    uint32_t         score;
    uint32_t         high_score;
    uint32_t         bonus_5000_mark;
    int              gdv;
    int              high_gdv;
    int              lives;
    int              timer_seconds;
    int              timer_frames;
    bool             timer_frozen;
    uint8_t          score_multiplier; /**< 0..4 => x1..x5 en Bomb Room. */
    uint8_t          gold_coins;       /**< >= 40 convierte Bonus Coin en Extra Coin. */
    MjkFloatingPopup popups[MJK_FLOAT_POPUP_SLOTS];
} MjkScoring;

/** @brief Inicializa el marcador para una partida nueva (3 vidas, GDV 47, preserva high_gdv). */
void MjkScoring_ResetNewGame(MjkScoring* sc);

/** @brief Reinicia el temporizador de sala a 60 segundos (divisor exacto de 256 frames). */
void MjkScoring_ResetStageTimer(MjkScoring* sc, int start_seconds);

/**
 * @brief Suma `score_id` al marcador aplicando `multiplied_score` en Bomb Room.
 * Si se cruza un multiplo de 5.000 pts en Bomb Room (con mult < x5), indica en
 * `out_events` si debe aparecer una Bonus Coin o una Extra Coin.
 */
void MjkScoring_AddPoints(MjkScoring* sc,
                          const MjkAssets* assets,
                          MjkScoreId score_id,
                          bool is_bomb_room,
                          MjkScoreEvents* out_events);

/** @brief Incrementa el GDV en `delta`, saturando en 99 y actualizando `high_gdv`. */
int MjkScoring_AddGdv(MjkScoring* sc, int delta);

/**
 * @brief Avanza un frame del temporizador de sala (divisor de 256 frames).
 * @return `true` en el frame exacto en que `timer_seconds` llega a 0 (Time Over).
 */
bool MjkScoring_TickTimer(MjkScoring* sc);

/** @brief Crea un popup flotante en `(x, y)` usando los tiles canonicos 0x30..0x3F. */
void MjkScoring_SpawnPopup(MjkScoring* sc,
                           float x,
                           float y,
                           MjkScoreId score_id,
                           uint8_t mult,
                           bool is_bomb_room);

/** @brief Actualiza los popups activos aplicando el desplazamiento de camara. */
void MjkScoring_TickPopups(MjkScoring* sc, float delta_cam_x, float delta_cam_y);

#endif /* MJK_ENGINE_SCORING_H */
