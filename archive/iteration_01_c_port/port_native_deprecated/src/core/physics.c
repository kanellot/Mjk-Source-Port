/**
 * @file physics.c
 * @brief Jack Physics Engine, parabolic jump trajectory (word_C0E3/word_C163),
 *        aerodynamic flutter glide, and subpixel integration for Mighty Bomb Jack.
 * 
 * Directly conforms to:
 *   - RES-006: Cinemática de Salto y Aleteo
 *   - data.asm:187-257 (word_C0E3, word_C163)
 *   - prg.asm:1658-1709 (DoSomethingWithObjYVel, sub_8AE8)
 */

#include "mbj/physics.h"

/* ============================================================================
 * 6502 AUTHENTIC DATA TABLES FROM src/data.asm
 * ============================================================================ */

/* 64-entry parabolic velocity magnitude table word_C0E3 (data.asm:187-251) */
const uint16_t kParabolicYVelTable_C0E3[PHYSICS_JUMP_TABLE_SIZE] = {
    0x0380, 0x037C, 0x0378, 0x0374, 0x0370, 0x0368, 0x0360, 0x0358,
    0x0350, 0x0348, 0x0340, 0x0330, 0x0320, 0x0310, 0x0300, 0x02F0,
    0x02E0, 0x02D0, 0x02C0, 0x02B0, 0x02A0, 0x0290, 0x0280, 0x0270,
    0x0260, 0x0250, 0x0240, 0x0230, 0x0220, 0x0210, 0x0200, 0x01F0,
    0x01E0, 0x01D0, 0x01C0, 0x01B0, 0x01A0, 0x0190, 0x0180, 0x0170,
    0x0160, 0x0150, 0x0140, 0x0130, 0x0120, 0x0110, 0x0100, 0x00F0,
    0x00E0, 0x00D0, 0x00C0, 0x00B0, 0x00A0, 0x0090, 0x0080, 0x0070,
    0x005E, 0x004C, 0x003A, 0x0028, 0x0010, 0x0008, 0x0002, 0x0001
};

/* 4-entry trajectory step increment table word_C163 (data.asm:252-256) */
const uint16_t kJumpPhaseStepTable_C163[4] = {
    0x00F0, /* 0: Neutral -> +240 subpixels/frame (Standard Jump) */
    0x0160, /* 1: Down    -> +352 subpixels/frame (Short Jump / Fast Fall) */
    0x00A0, /* 2: Up      -> +160 subpixels/frame (High Float Jump) */
    0x00D0  /* 3: Up+Down -> +208 subpixels/frame */
};

/* ============================================================================
 * TRAJECTORY LOOKUP & STEP
 * ============================================================================ */

int16_t Physics_SampleParabolicYVel(uint8_t phase_hi) {
    /* 6502 sub_8AE8 (prg.asm:1689-1709) */
    if (phase_hi < PHYSICS_APEX_PHASE) {
        /* Rising phase: negative velocity (moving upward) */
        uint8_t idx = phase_hi;
        if (idx >= PHYSICS_JUMP_TABLE_SIZE) idx = PHYSICS_JUMP_TABLE_SIZE - 1;
        return (int16_t)(-(int32_t)kParabolicYVelTable_C0E3[idx]);
    } else {
        /* Falling phase: mirrored table index (phase ^ 0xFF) & 0x3F = 0x7F - phase */
        uint8_t idx = (phase_hi <= 0x7F) ? (uint8_t)(0x7F - phase_hi) : 0;
        if (idx >= PHYSICS_JUMP_TABLE_SIZE) idx = 0;
        return (int16_t)kParabolicYVelTable_C0E3[idx];
    }
}

void Physics_UpdateJackJump(PlayerStructRAM *player, uint8_t buttons, bool just_jumped) {
    if (!player) return;

    /* 1. Ground Launch (Jump button pressed on solid ground) */
    if (just_jumped && (player->status & 0x02) == 0) {
        player->status |= 0x02;        /* Set in-air flag */
        player->jump_table_step = 0;   /* Reset sub-step */
        player->jump_flutter_state = 0;/* Start at phase 0 (max launch velocity $0380) */
        player->collision_flag &= ~0x04; /* Clear on-ground flag */
    }

    /* 2. In-Air Trajectory Advance */
    if ((player->status & 0x02) != 0) {
        /* Flutter Glide: If player is falling (phase >= $40) and holds Button A */
        if ((player->jump_flutter_state >= PHYSICS_APEX_PHASE) && (buttons & JP_BTN_A) != 0) {
            /* Authentic NES flutter glide: minimal drop rate (1 subpixel per frame) */
            player->y_vel_hi = 0;
            player->y_vel_lo = 0x01;
            return;
        }

        /* Standard parabolic trajectory step */
        uint8_t step_idx = 0;
        if (buttons & JP_BTN_DOWN) step_idx |= 1;
        if (buttons & JP_BTN_UP)   step_idx |= 2;

        uint16_t step = kJumpPhaseStepTable_C163[step_idx];

        uint16_t phase = ((uint16_t)player->jump_flutter_state << 8) | (uint16_t)player->jump_table_step;
        phase += step;

        player->jump_table_step = (uint8_t)(phase & 0xFF);
        uint8_t phase_hi = (uint8_t)(phase >> 8);

        /* Terminal falling clamp at phase 127 ($7F) */
        if (phase_hi > 0x7F) {
            phase_hi = 0x7F;
        }
        player->jump_flutter_state = phase_hi;

        int16_t vy = Physics_SampleParabolicYVel(phase_hi);
        player->y_vel_lo = (uint8_t)((uint16_t)vy & 0xFF);
        player->y_vel_hi = (int8_t)(((uint16_t)vy >> 8) & 0xFF);
    }
}

void Physics_IntegratePosition(PlayerStructRAM *player) {
    if (!player) return;

    /* Integrate X: 16-bit addition with velocity */
    int16_t vx = ((int16_t)player->x_vel_hi << 8) | (uint16_t)player->x_vel_lo;
    uint16_t px = ((uint16_t)player->x_pos_hi << 8) | (uint16_t)player->x_pos_lo;
    px += (uint16_t)vx;
    player->x_pos_lo = (uint8_t)(px & 0xFF);
    player->x_pos_hi = (uint8_t)(px >> 8);

    /* Integrate Y: 16-bit addition with velocity */
    int16_t vy = ((int16_t)player->y_vel_hi << 8) | (uint16_t)player->y_vel_lo;
    uint16_t py = ((uint16_t)player->y_pos_hi << 8) | (uint16_t)player->y_pos_lo;
    py += (uint16_t)vy;
    player->y_pos_lo = (uint8_t)(py & 0xFF);
    player->y_pos_hi = (uint8_t)(py >> 8);
}
