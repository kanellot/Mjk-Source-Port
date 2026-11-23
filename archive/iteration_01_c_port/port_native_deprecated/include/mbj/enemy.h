/**
 * @file enemy.h
 * @brief Enemy AI, Spawning, Mummy Mutations, and Coin Transmutation.
 * Parity with NES PRG $B593-$BFDF and RES-008.
 */

#ifndef MBJ_ENEMY_H
#define MBJ_ENEMY_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/ram_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Enemy Types matching JumpTable at $8269 in prg.asm */
typedef enum {
    ENEMY_TYPE_MUMMY            = 0,
    ENEMY_TYPE_UNKNOWN_1        = 1,
    ENEMY_TYPE_HANEZO           = 2,    /* Bouncing bird/elastic hopper */
    ENEMY_TYPE_GEJISHOGUN       = 3,    /* Perimeter caterpillar crawler */
    ENEMY_TYPE_GAMEIDO          = 4,    /* Homing turtle with smoothed tracking */
    ENEMY_TYPE_DOKURON          = 5,    /* Leaping skull with horizontal chase */
    ENEMY_TYPE_DESUFA           = 6,    /* Floating fireball with sinusoidal Y */
    ENEMY_TYPE_HORUS            = 7,    /* Hunting hawk with 4-way dominant vector */
    ENEMY_TYPE_POWER_COIN       = 8,
    ENEMY_TYPE_EXTRA_COIN       = 9,
    ENEMY_TYPE_SECRET_COIN      = 10,
    ENEMY_TYPE_CAPTIVE          = 11,
    ENEMY_TYPE_BALLOON          = 12,
    ENEMY_TYPE_MUMMY_ALT1       = 13,
    ENEMY_TYPE_BONUS_COIN       = 14,
    ENEMY_TYPE_MUMMY_ALT2       = 15,
    ENEMY_TYPE_BROTHER          = 16
} MBJ_EnemyType;

/* Status flags for ObjectStructRAM (+0x00) */
#define ENEMY_STATUS_ACTIVE         0x01
#define ENEMY_STATUS_IN_AIR         0x02
#define ENEMY_STATUS_SPAWN_FLASH    0x08
#define ENEMY_STATUS_IS_COIN        0x20

/* Total power coin duration: 320 frames (5.33 seconds at 60 Hz) */
#define ENEMY_COIN_TIMER_DURATION   320

/* Maximum active enemies alive at once */
#define MAX_ALIVE_ENEMIES           4

/**
 * @brief Resets the enemy manager state.
 */
void Enemy_InitManager(void);

/**
 * @brief Updates all active enemies, spawning timer, AI machines, and coin timers for 1 frame.
 * @param ram Pointer to complete NES RAM map.
 * @param collision_map Pointer to current 256-byte room collision map ($0500-$05FF).
 */
void Enemy_UpdateAll(NES_RAM *ram, const uint8_t *collision_map);

/**
 * @brief Spawns a mummy at the designated room spawn coordinate if slots are available.
 */
void Enemy_SpawnMummy(NES_RAM *ram, uint8_t room_id);

/**
 * @brief Evolve a Mummy into types 2..7 using the deterministic formula: (APressCounter % 6) + 2.
 */
void Enemy_TransformMummy(ObjectStructRAM *enemy, uint8_t a_press_counter);

/**
 * @brief Activates the Power Coin effect: sets EnemyCoinTimer to 320 frames and turns enemies into coins.
 */
void Enemy_ActivatePowerCoin(NES_RAM *ram);

/**
 * @brief Check collision between Jack and an active enemy slot.
 * @param ram Pointer to NES RAM map.
 * @param enemy Pointer to the enemy object.
 * @param out_collected_as_coin Set to true if the enemy was a transmutated coin and collected.
 * @return True if Jack touched the enemy (damage or coin collect).
 */
bool Enemy_CheckPlayerCollision(NES_RAM *ram, ObjectStructRAM *enemy, bool *out_collected_as_coin);

/**
 * @brief Returns the count of active enemies alive (types 0..7).
 */
int Enemy_GetActiveCount(const NES_RAM *ram);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_ENEMY_H */
