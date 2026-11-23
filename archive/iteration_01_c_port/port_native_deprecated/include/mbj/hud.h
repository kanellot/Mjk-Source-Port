/**
 * @file hud.h
 * @brief HUD BCD Arithmetic, Torture Room Penalty, GDV Calculation, and Ending Evaluations.
 * Bit-exact C11 implementation matching NES PRG $8B69-$8D5E, $8487-$8764, and RES-009.
 */

#ifndef MBJ_HUD_H
#define MBJ_HUD_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/ram_map.h"
#include "mbj/render.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BCD Utility Functions */
uint8_t BCD_Sub1(uint8_t bcd_val);
uint8_t BCD_Add(uint8_t a, uint8_t b);
uint32_t BCD_ToUint32(const uint8_t *bcd_bytes, int num_bytes);

/* Ending Types */
typedef enum {
    ENDING_TYPE_WORST = 0, /* 0 crystal balls (12 blocks) */
    ENDING_TYPE_GOOD  = 1, /* 1 crystal ball (30 blocks) */
    ENDING_TYPE_GREAT = 2, /* 2 crystal balls, < 5 secret coins (56 blocks) */
    ENDING_TYPE_BEST  = 3  /* 2 crystal balls, >= 5 secret coins (90 blocks) */
} MBJ_EndingType;

/**
 * @brief Evaluates which of the 4 endings is unlocked.
 */
MBJ_EndingType Ending_Evaluate(uint8_t crystal_balls, uint8_t secret_coins);

/**
 * @brief Returns the pyramid debris depth count for the specified ending.
 */
int Ending_GetPyramidDebrisBlocks(MBJ_EndingType type);

/**
 * @brief Computes final Game Degree Value (GDV) in BCD based on original ROM formulas.
 */
uint8_t GDV_Calculate(const NES_RAM *ram, uint8_t last_bomb_room, uint8_t extra_lives, uint32_t total_score);

/**
 * @brief Checks if player has triggered Greed conditions (Timer >= 100 BCD or 10 Mighty Coins).
 */
bool TortureRoom_CheckGreed(const NES_RAM *ram);

/**
 * @brief Triggers the Torture Room penalty (50 jumps in US version, room $AC).
 */
void TortureRoom_Trigger(NES_RAM *ram);

/**
 * @brief Steps a jump inside the torture room.
 * @return True if penalty was fulfilled and player is released.
 */
bool TortureRoom_OnJump(NES_RAM *ram);

/**
 * @brief Renders the HUD top bar (Score, Stage Timer / Torture jumps, Mighty Coins) into the virtual screen.
 */
void HUD_Render(uint32_t *screen_buffer, const NES_RAM *ram, const CHRBank *chr, uint32_t frame_count);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_HUD_H */
