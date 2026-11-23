/**
 * @file collision.c
 * @brief 9-Point spatial probe collision engine, quadrant solidness masks,
 *        ceiling bonk, and floor landing/snapping for Mighty Bomb Jack (NES).
 * 
 * Directly conforms to:
 *   - RES-006: Motor Polimórfico de Colisión de 9 Sondas
 *   - prg.asm:5863-5883 (byte_A3D8, byte_A3EA)
 *   - prg.asm:4480-4610 (sub_9B6A)
 *   - prg.asm:5529-5609 (sub_A1B9, sub_A1EB)
 */

#include "mbj/collision.h"
#include "mbj/physics.h"

/* ============================================================================
 * 9 DISCRETE PROBE OFFSETS (prg.asm:5863-5872, byte_A3D8)
 * ============================================================================ */

const ProbeOffset kCollisionProbes_A3D8[COLLISION_PROBE_COUNT] = {
    {  0,  0 }, /* Probe 0: Center (objects, items, bombs, lethal fire) */
    { -4, -8 }, /* Probe 1: Ceiling Left */
    {  4, -8 }, /* Probe 2: Ceiling Right */
    { -4,  8 }, /* Probe 3: Floor Left */
    {  4,  8 }, /* Probe 4: Floor Right */
    { -8, -4 }, /* Probe 5: Wall Left Top */
    { -8,  4 }, /* Probe 6: Wall Left Bottom */
    {  8, -4 }, /* Probe 7: Wall Right Top */
    {  8,  4 }  /* Probe 8: Wall Right Bottom */
};

/* ============================================================================
 * 10 METATILE QUADRANT MASKS (prg.asm:5873-5883, byte_A3EA)
 * ============================================================================ */

const uint8_t kPartialMetatileMasks_A3EA[10] = {
    0xCC, /* 0: $38 (Platform %1100), $39 (Platform %1100) */
    0xFC, /* 1: $3A (Full Block %1111), $3B (Platform %1100) */
    0x48, /* 2: $3C (Corner %0100), $3D (Corner %1000) */
    0x5A, /* 3: $3E (Vertical %0101), $3F (Vertical %1010) */
    0x7B, /* 4: $40 (Slanted %0111), $41 (Slanted %1011) */
    0xDE, /* 5: $42 (Slanted %1101), $43 (Slanted %1110) */
    0x21, /* 6: $44 (Corner %0010), $45 (Corner %0001) */
    0x35, /* 7: $46 (Bottom half %0011), $47 (Vertical %0101) */
    0xAC, /* 8: $48 (Vertical %1010), $49 (Platform %1100) */
    0x3C  /* 9: $4A (Bottom half %0011), $4B (Platform %1100) */
};

/* ============================================================================
 * METATILE SOLIDNESS EVALUATION
 * ============================================================================ */

bool Collision_IsMetatileSolid(uint8_t metatile_id, uint8_t px, uint8_t py, int16_t vy) {
    /* 1. Passable / empty tiles ($00, $2F, $24, $27, $28, $34) */
    if (metatile_id == 0x00 || metatile_id == 0x2F || metatile_id == 0x24 ||
        metatile_id == 0x27 || metatile_id == 0x28 || metatile_id == 0x34) {
        return false;
    }

    /* 2. Open doors ($95..$98) */
    if (metatile_id >= 0x95 && metatile_id <= 0x98) {
        return false;
    }

    /* 3. Partial solid and one-way platforms ($38..$4B) */
    if (metatile_id >= 0x38 && metatile_id <= 0x4B) {
        /* One-way platforms: pass upward freely if moving up (vy < 0) */
        bool is_platform = (metatile_id == 0x38 || metatile_id == 0x39 ||
                            metatile_id == 0x3B || metatile_id == 0x49 ||
                            metatile_id == 0x4B);
        if (is_platform && vy < 0) {
            return false;
        }

        uint8_t idx = (uint8_t)((metatile_id - 0x38) >> 1);
        uint8_t mask_byte = kPartialMetatileMasks_A3EA[idx];
        uint8_t nibble = ((metatile_id - 0x38) & 1) ? (mask_byte & 0x0F) : (mask_byte >> 4);

        /* Determine 8x8 quadrant: Bit 3=TL, 2=TR, 1=BL, 0=BR */
        bool is_bottom = (py & 8) != 0;
        bool is_right  = (px & 8) != 0;
        uint8_t shift = (is_bottom ? 0 : 2) + (is_right ? 0 : 1);
        uint8_t quad_bit = (uint8_t)(1 << shift);

        return (nibble & quad_bit) != 0;
    }

    /* 4. Full solid block (default for standard brick/stone tiles) */
    return true;
}

uint8_t Collision_Run9PointProbes(const uint8_t metatiles[15][16],
                                 uint8_t x_px, uint8_t y_px, int16_t vy) {
    if (!metatiles) return 0;

    uint8_t collision_flags = 0;

    /* Probe evaluation helper */
    bool probe_results[COLLISION_PROBE_COUNT];
    for (int p = 0; p < COLLISION_PROBE_COUNT; p++) {
        int test_x = (int)x_px + kCollisionProbes_A3D8[p].dx;
        int test_y = (int)y_px + kCollisionProbes_A3D8[p].dy;

        /* Clamp to room bounds */
        if (test_x < 0) test_x = 0;
        if (test_x > 255) test_x = 255;
        if (test_y < 0) test_y = 0;
        if (test_y > 239) test_y = 239;

        int row = test_y / 16;
        int col = test_x / 16;
        if (row > 14) row = 14;
        if (col > 15) col = 15;

        uint8_t tile = metatiles[row][col];
        probe_results[p] = Collision_IsMetatileSolid(tile, (uint8_t)(test_x & 0x0F), (uint8_t)(test_y & 0x0F), vy);
    }

    /* Collapse probe pairs (sub_9965):
     * Bit 3: Ceiling (Probe 1 or Probe 2)
     * Bit 2: Floor   (Probe 3 or Probe 4)
     * Bit 1: Wall L  (Probe 5 or Probe 6)
     * Bit 0: Wall R  (Probe 7 or Probe 8)
     */
    if (probe_results[1] || probe_results[2]) collision_flags |= 0x08; /* Ceiling */
    if (probe_results[3] || probe_results[4]) collision_flags |= 0x04; /* Floor */
    if (probe_results[5] || probe_results[6]) collision_flags |= 0x02; /* Wall Left */
    if (probe_results[7] || probe_results[8]) collision_flags |= 0x01; /* Wall Right */

    return collision_flags;
}

void Collision_ResolvePlayer(PlayerStructRAM *player, const uint8_t metatiles[15][16]) {
    if (!player || !metatiles) return;

    int16_t vy = ((int16_t)player->y_vel_hi << 8) | (uint16_t)player->y_vel_lo;
    uint8_t flags = Collision_Run9PointProbes(metatiles, player->x_pos_hi, player->y_pos_hi, vy);
    player->collision_flag = flags;

    /* 1. Ceiling Hit (sub_A1B9: Ceiling Bonk) */
    if ((flags & 0x08) != 0 && vy < 0) {
        player->jump_flutter_state = PHYSICS_APEX_PHASE; /* Reset to apex = start falling */
        player->y_vel_hi = 0;
        player->y_vel_lo = 0;
        player->y_pos_hi = (uint8_t)(player->y_pos_hi | 7); /* Align Y */
    }

    /* 2. Floor Hit & Landing (sub_A1EB: Floor State Update) */
    if ((flags & 0x04) != 0) {
        /* Land Jack if moving downwards */
        if (vy >= 0) {
            player->status &= ~0x02;        /* Clear in-air flag */
            player->y_vel_hi = 0;
            player->y_vel_lo = 0;
            player->y_pos_hi = (uint8_t)(player->y_pos_hi & 0xF8); /* Snap to 8px floor grid */
        }
    } else {
        /* Walked off ledge: start falling immediately from apex */
        if ((player->status & 0x02) == 0) {
            player->status |= 0x02;
            player->jump_flutter_state = PHYSICS_APEX_PHASE;
        }
    }

    /* 3. Wall Blocks */
    if ((flags & 0x02) != 0 && player->x_vel_hi < 0) {
        player->x_vel_hi = 0;
        player->x_vel_lo = 0;
    }
    if ((flags & 0x01) != 0 && player->x_vel_hi > 0) {
        player->x_vel_hi = 0;
        player->x_vel_lo = 0;
    }
}
