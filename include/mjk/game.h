/**
 * @file game.h
 * @brief Estructura raiz `MjkGame` y ciclo completo de Mighty Bomb Jack (C11 puro).
 *
 * `MjkGame` contiene exclusivamente estado de simulacion y reglas de juego:
 *  - Cero dependencias de SDL2 o del sistema operativo.
 *  - Cero estructuras PPU/OAM (el renderizador lee `const MjkGame*` en `src/render/`).
 *  - Cero codigo de debug, cheats o verificacion externa.
 */

#ifndef MJK_ENGINE_GAME_H
#define MJK_ENGINE_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk/assets.h"
#include "mjk/doors.h"
#include "mjk/enemies.h"
#include "mjk/input.h"
#include "mjk/objects.h"
#include "mjk/player.h"
#include "mjk/scoring.h"
#include "mjk/types.h"
#include "mjk/world.h"

#define MJK_SOUND_QUEUE_CAP 8u

/** @brief Estado de la secuencia de inicio (Copyright <-> Title -> Jack Falling). */
typedef struct {
    MjkTitleState state;
    int           timer;
    float         jack_x;
    float         jack_y;
    float         jack_vx;
    float         jack_vy;
    float         grav_progress;
    uint8_t       jack_sprite;
} MjkTitleScreen;

/** @brief Estado completo de una partida de Mighty Bomb Jack. */
typedef struct MjkGame {
    const MjkAssets* assets;
    MjkScreenMode    mode;
    MjkGameState     state;
    MjkTitleScreen   title;

    MjkWorld         world;
    MjkPlayer        player;
    MjkScoring       scoring;
    MjkObjectSystem  objects;
    MjkEnemySystem   enemies;

    /* Navegacion entre salas y checkpoints */
    uint8_t current_room_id;
    uint8_t exit_source_room_id;
    uint8_t checkpoint_room_id;
    uint8_t torture_return_room_id;
    uint8_t entry_door_type;
    uint8_t entry_door_backup;
    uint8_t torture_return_door_type;
    bool    entry_door_already_open;

    /* Progresion, Mighty y coleccionables */
    uint8_t mighty_coins;
    uint8_t mighty_level;
    uint8_t mighty_presses_left;
    uint8_t last_bomb_room_cleared;
    uint8_t fire_bombs_collected;
    uint8_t extra_lives_found;
    uint8_t crystal_balls_mask;
    uint8_t secret_coins;
    bool    collected_1up;
    int     one_up_timer;

    /* Banderas de puertas especiales */
    uint8_t door_flag_09;
    uint8_t door_flag_1f;
    uint8_t door_1e_deaths;

    /* Control de subestados y pantallas */
    bool    paused;
    int     state_timer;
    uint8_t sub_state;
    bool    round_clear_has_special;
    int     round_clear_timer_copy;
    int     round_clear_fire_bonus;

    /* Sala de Tortura (50 saltos US) y 4 Finales */
    bool    in_torture_mode;
    int     greedy_jumps_left;
    uint8_t ending_type;
    uint8_t ending_state;
    uint8_t ending_depth;
    uint8_t ending_carved_rows;
    float   cinematic_jack_x;
    float   cinematic_jack_y;

    /* Indicador visual para el renderizador */
    bool    power_coin_palette_active;

    /* Cola de eventos de audio consumida por AudioEngine */
    MjkSoundId last_sound;
    MjkSoundId sound_queue[MJK_SOUND_QUEUE_CAP];
    uint8_t    sound_queue_count;
} MjkGame;

/** @brief Inicializa el juego en la pantalla de Copyright (REV_US). */
void MjkGame_Init(MjkGame* game, const MjkAssets* assets);

/** @brief Avanza exactamente un frame (1/60.0988 s) de logica de juego. */
void MjkGame_StepFrame(MjkGame* game, const MjkInputStep* input);

/** @brief Encola un sonido/musica para que el motor de audio lo reproduzca. */
void MjkGame_QueueSound(MjkGame* game, MjkSoundId sound);

/** @brief Suma puntos y genera Bonus/Extra Coin si se cruza un multiplo de 5.000 pts. */
void MjkGame_AddScore(MjkGame* game, MjkScoreId score_id);

/** @brief Dispara la Sala de Tortura (por exceso de Mighty Coins o tiempo >= 100). */
MjkActionResult MjkGame_TriggerTortureRoom(MjkGame* game);

/** @brief Descuenta un salto en la Sala de Tortura; al llegar a 0 retorna al checkpoint. */
void MjkGame_OnTortureJump(MjkGame* game);

/** @brief Procesa la perdida de una vida (respawn en checkpoint o Game Over). */
void MjkGame_LoseLife(MjkGame* game);

/** @brief Ejecuta la transicion al salir por una puerta abierta hacia la sala destino. */
void MjkGame_ExitRoom(MjkGame* game, uint8_t exit_door_dir);

/* Helpers internos de pantallas (implementados en src/game/screens.c) */
bool MjkScreens_StepTitle(MjkGame* game, const MjkInputStep* input);
void MjkScreens_StepRoundClear(MjkGame* game);
void MjkScreens_StepGameOver(MjkGame* game);
void MjkScreens_StepEnding(MjkGame* game, const MjkInputStep* input);

#endif /* MJK_ENGINE_GAME_H */
