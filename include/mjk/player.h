/**
 * @file player.h
 * @brief Fisica de Jack en `float`, curva parabolica de 64 pasos, flutter y colisiones.
 *
 * Todas las posiciones y velocidades se expresan en pixeles y pixeles/frame (`float`).
 * Como las constantes originales de Mighty Bomb Jack son multiplos exactos de 1/256
 * (p. ej. 1.5 px/f, 0.9375, 1.375, 0.625), IEEE-754 reproduce la fisica original sin
 * errores de redondeo y con codigo completamente legible.
 */

#ifndef MJK_ENGINE_PLAYER_H
#define MJK_ENGINE_PLAYER_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk/assets.h"
#include "mjk/input.h"
#include "mjk/types.h"
#include "mjk/world.h"

#define MJK_COLL_RIGHT_WALL  0x01u
#define MJK_COLL_LEFT_WALL   0x02u
#define MJK_COLL_CEILING     0x04u
#define MJK_COLL_GROUND      0x08u

#define MJK_JACK_RUN_SPEED   1.5f
#define MJK_JACK_FLOOR_Y     224.0f

/** @brief Estado completo del jugador (Jack). */
typedef struct {
    float     x;
    float     y;
    float     vx;
    float     vy;
    float     jump_progress;    /**< Indice continuo [0..127.99] sobre la curva de salto. */
    MjkFacing facing;
    uint8_t   collision_flags;
    uint8_t   sprite_id;
    uint8_t   walk_timer;
    bool      in_air;
    bool      just_jumped;
    bool      is_dying;
    bool      death_grounded;
    int       death_timer;
    uint8_t   fire_tile_died_to;
    uint8_t   jump_press_count; /**< Persiste entre salas; gobierna mutacion de enemigos. */
} MjkPlayer;

/** @brief Contexto y salidas de un paso de `MjkPlayer_Update` (reemplaza 14 parametros). */
typedef struct {
    bool       in_torture_room;
    uint8_t*   mighty_level;
    uint8_t*   mighty_presses_left;
    MjkSoundId sfx_triggered;
    bool       award_jump_10pts;
} MjkPlayerStepCtx;

/** @brief Posiciona a Jack al entrar en una sala conservando `jump_press_count`. */
void MjkPlayer_Init(MjkPlayer* player, float x, float y);

/** @brief Inicia la secuencia de muerte de Jack (volteretas en aire -> 64f en suelo). */
void MjkPlayer_Kill(MjkPlayer* player, uint8_t fire_tile);

/** @brief Avanza un frame de fisica, flutter, colisiones 9-probe y animacion de Jack. */
MjkActionResult MjkPlayer_Update(MjkPlayer* player,
                                 const MjkInputStep* input,
                                 const MjkWorld* world,
                                 const MjkAssets* assets,
                                 MjkPlayerStepCtx* ctx);

#endif /* MJK_ENGINE_PLAYER_H */
