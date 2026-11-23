/**
 * @file mjk_enemy.h
 * @brief Enemigos, spawns diferidos, 6 arquetipos, Power Coin 320f y coleccionables (T-22, T-23).
 */

#ifndef MJK_ENEMY_H
#define MJK_ENEMY_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_ppu.h"
#include "mjk_types.h"

struct MjkGame;

#define MJK_ENEMY_MAX_SLOTS    8u
#define MJK_ENEMY_MAX_ALIVE    4u
#define MJK_POWER_COIN_FRAMES  320u

typedef struct {
    bool        active;
    bool        transmuted;
    bool        facing_left;
    uint8_t     type;
    uint8_t     status;
    uint8_t     anim_timer;
    uint8_t     sprite_id;
    uint8_t     attr_base;
    uint8_t     spawn_flash_timer;
    uint8_t     transform_timer;
    mjk_uq8_8_t pos_x;
    mjk_uq8_8_t pos_y;
    mjk_q8_8_t  vel_x;
    mjk_q8_8_t  vel_y;
    uint16_t    jump_accum;
    uint8_t     ai_timer;
    uint8_t     ai_dir;
    bool        falling;
    bool        edge_turned;
} MjkEnemy;

typedef struct {
    MjkEnemy slots[MJK_ENEMY_MAX_SLOTS];
    uint8_t  alive_count;
    uint8_t  spawn_timer;
    uint8_t  spawn_reload;
    uint8_t  transform_reload;
    uint8_t  speed_tier;
    uint8_t  spawn_cursor;
    uint16_t transmute_timer;
    uint8_t  transmute_streak;
    uint8_t  post_transmute_grace;
} MjkEnemyManager;

MJK_CORE_API void MjkEnemy_InitForRoom(MjkEnemyManager* mgr,
                                       const MjkAssetPackage* pkg,
                                       uint8_t last_bomb_cleared,
                                       bool is_bomb_room);

MJK_CORE_API void MjkEnemy_ActivatePowerCoin(MjkEnemyManager* mgr);

MJK_CORE_API void MjkEnemy_ActivatePowerCoinEx(MjkEnemyManager* mgr, struct MjkGame* game);

MJK_CORE_API bool MjkEnemy_SpawnPowerCoinBombRoom(MjkEnemyManager* mgr, const struct MjkGame* game);

MJK_CORE_API bool MjkEnemy_SpawnCollectible(MjkEnemyManager* mgr,
                                            MjkEnemyType type,
                                            uint8_t x,
                                            uint8_t y);

MJK_CORE_API MjkActionResult MjkEnemy_UpdateAll(MjkEnemyManager* mgr,
                                                struct MjkGame* game);

MJK_CORE_API uint8_t MjkEnemy_RenderOam(const MjkEnemyManager* mgr,
                                        const MjkAssetPackage* pkg,
                                        MjkPpu* ppu,
                                        uint8_t start_slot);

#endif /* MJK_ENEMY_H */
