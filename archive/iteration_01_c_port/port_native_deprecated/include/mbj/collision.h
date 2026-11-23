/**
 * @file collision.h
 * @brief 9-Point spatial probe collision engine, quadrant solidness masks,
 *        and metatile landing/snapping for Mighty Bomb Jack (NES).
 * 
 * Based directly on:
 *   - RES-006: Motor Polimórfico de Colisión de 9 Sondas
 *   - prg.asm:5863-5883 (byte_A3D8, byte_A3EA)
 *   - prg.asm:4480-4610 (sub_9B6A)
 */

#ifndef MBJ_COLLISION_H
#define MBJ_COLLISION_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/types.h"
#include "mbj/ram_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COLLISION_PROBE_COUNT 9

typedef struct {
    int8_t dx;
    int8_t dy;
} ProbeOffset;

extern const ProbeOffset kCollisionProbes_A3D8[COLLISION_PROBE_COUNT];
extern const uint8_t kPartialMetatileMasks_A3EA[10];

/* ============================================================================
 * SOLIDITY AND PROBE APIS
 * ============================================================================ */

bool Collision_IsMetatileSolid(uint8_t metatile_id, uint8_t px, uint8_t py, int16_t vy);

uint8_t Collision_Run9PointProbes(const uint8_t metatiles[15][16],
                                 uint8_t x_px, uint8_t y_px, int16_t vy);

void Collision_ResolvePlayer(PlayerStructRAM *player, const uint8_t metatiles[15][16]);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_COLLISION_H */
