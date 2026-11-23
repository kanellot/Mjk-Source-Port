/**
 * @file physics.h
 * @brief Jack Physics Engine, parabolic jump trajectory (word_C0E3/word_C163),
 *        aerodynamic flutter glide, and subpixel integration for Mighty Bomb Jack.
 * 
 * Based directly on:
 *   - RES-006: Cinemática de Salto y Aleteo
 *   - data.asm:187-257 (word_C0E3, word_C163)
 *   - prg.asm:1658-1709 (DoSomethingWithObjYVel, sub_8AE8)
 */

#ifndef MBJ_PHYSICS_H
#define MBJ_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/types.h"
#include "mbj/ram_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PHYSICS_JUMP_TABLE_SIZE     64
#define PHYSICS_RUN_SPEED_SUB       384   /* 1.50 px/frame ($0180 subpixels) */
#define PHYSICS_APEX_PHASE          0x40  /* Phase index 64 = apex of jump */

extern const uint16_t kParabolicYVelTable_C0E3[PHYSICS_JUMP_TABLE_SIZE];
extern const uint16_t kJumpPhaseStepTable_C163[4];

/* ============================================================================
 * PHYSICS INTERACTION APIS
 * ============================================================================ */

int16_t Physics_SampleParabolicYVel(uint8_t phase_hi);

void Physics_UpdateJackJump(PlayerStructRAM *player, uint8_t buttons, bool just_jumped);

void Physics_IntegratePosition(PlayerStructRAM *player);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_PHYSICS_H */
