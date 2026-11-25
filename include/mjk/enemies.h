/**
 * @file enemies.h
 * @brief IA de enemigos (Momia + 6 arquetipos), Power Coin (320f) y coleccionables.
 */

#ifndef MJK_ENGINE_ENEMIES_H
#define MJK_ENGINE_ENEMIES_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk/assets.h"
#include "mjk/types.h"

struct MjkGame;

#define MJK_ENEMY_MAX_SLOTS    8u
#define MJK_ENEMY_MAX_ALIVE    4u
#define MJK_POWER_COIN_FRAMES  320

/** @brief Entidad activa de enemigo o moneda/cautivo coleccionable. */
typedef struct {
    bool         active;
    bool         transmuted;
    bool         facing_left;
    MjkEnemyKind kind;
    uint8_t      anim_timer;
    uint8_t      sprite_id;
    uint8_t      palette_idx;
    int          spawn_flash_timer;
    int          transform_timer;
    float        x;
    float        y;
    float        vx;
    float        vy;
    float        fall_progress;
    uint8_t      ai_timer;
    uint8_t      ai_dir;
    bool         falling;
    bool         edge_turned;
} MjkEnemy;

/** @brief Gestor de los 8 slots de enemigos y del temporizador de Power Coin. */
typedef struct {
    MjkEnemy slots[MJK_ENEMY_MAX_SLOTS];
    uint8_t  alive_count;
    int      spawn_timer;
    int      spawn_reload;
    int      transform_reload;
    uint8_t  speed_tier;
    int      transmute_timer;
    uint8_t  transmute_streak;
    int      post_transmute_grace;
} MjkEnemySystem;

/** @brief Configura dificultad, spawns y limpia slots al entrar en una sala. */
void MjkEnemies_InitForRoom(MjkEnemySystem* sys,
                            const MjkAssets* assets,
                            uint8_t last_bomb_cleared,
                            bool is_bomb_room);

/** @brief Activa la transmutacion Power Coin (320 frames) sobre todos los monstruos. */
void MjkEnemies_ActivatePowerCoin(MjkEnemySystem* sys, struct MjkGame* game);

/** @brief Genera una Power Coin movil en Bomb Room. */
bool MjkEnemies_SpawnPowerCoinBombRoom(MjkEnemySystem* sys, const MjkAssets* assets);

/** @brief Genera un enemigo o coleccionable (moneda, cautivo, globo) en `(x, y)`. */
bool MjkEnemies_Spawn(MjkEnemySystem* sys, MjkEnemyKind kind, float x, float y);

/** @brief Avanza un frame de spawns, IA de los 6 arquetipos, Power Coin y colisiones. */
MjkActionResult MjkEnemies_Update(MjkEnemySystem* sys, struct MjkGame* game);

#endif /* MJK_ENGINE_ENEMIES_H */
