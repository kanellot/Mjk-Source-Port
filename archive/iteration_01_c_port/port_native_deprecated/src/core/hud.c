/**
 * @file hud.c
 * @brief HUD BCD Arithmetic, Torture Room Penalty, GDV Calculation, and Ending Evaluations.
 * Bit-exact C11 implementation matching NES PRG $8B69-$8D5E, $8487-$8764, and RES-009.
 */

#include "mbj/hud.h"
#include <string.h>
#include <stdio.h>

/* BCD Decrement with decimal carry */
uint8_t BCD_Sub1(uint8_t a) {
    uint8_t lo = a & 0x0F;
    uint8_t hi = (a >> 4) & 0x0F;
    if (lo == 0) {
        lo = 9;
        if (hi > 0) {
            hi--;
        } else {
            hi = 9;
        }
    } else {
        lo--;
    }
    return (uint8_t)((hi << 4) | lo);
}

/* BCD Addition with decimal carry */
uint8_t BCD_Add(uint8_t a, uint8_t b) {
    uint8_t lo = (a & 0x0F) + (b & 0x0F);
    uint8_t hi = ((a >> 4) & 0x0F) + ((b >> 4) & 0x0F);
    if (lo >= 10) {
        lo -= 10;
        hi++;
    }
    if (hi >= 10) {
        hi -= 10;
    }
    return (uint8_t)((hi << 4) | lo);
}

uint32_t BCD_ToUint32(const uint8_t *bcd_bytes, int num_bytes) {
    if (!bcd_bytes || num_bytes <= 0) return 0;
    uint32_t result = 0;
    for (int i = num_bytes - 1; i >= 0; --i) {
        uint8_t hi = (bcd_bytes[i] >> 4) & 0x0F;
        uint8_t lo = bcd_bytes[i] & 0x0F;
        result = result * 100 + (hi * 10 + lo);
    }
    return result;
}

MBJ_EndingType Ending_Evaluate(uint8_t crystal_balls, uint8_t secret_coins) {
    if (crystal_balls == 0) {
        return ENDING_TYPE_WORST;
    } else if (crystal_balls == 1) {
        return ENDING_TYPE_GOOD;
    } else if (crystal_balls >= 2 && secret_coins < 5) {
        return ENDING_TYPE_GREAT;
    } else {
        return ENDING_TYPE_BEST;
    }
}

int Ending_GetPyramidDebrisBlocks(MBJ_EndingType type) {
    switch (type) {
        case ENDING_TYPE_WORST: return 12;
        case ENDING_TYPE_GOOD:  return 30;
        case ENDING_TYPE_GREAT: return 56;
        case ENDING_TYPE_BEST:  return 90;
        default: return 12;
    }
}

uint8_t GDV_Calculate(const NES_RAM *ram, uint8_t last_bomb_room, uint8_t extra_lives, uint32_t total_score) {
    if (!ram) return 0;

    uint8_t gdv = 0;

    /* In-game collectibles */
    gdv = BCD_Add(gdv, ram->player.secret_coins);
    
    /* End game bonus (3 fixed points) */
    gdv = BCD_Add(gdv, 0x03);

    /* Rounds completed: (last_bomb_room & 0x0F) >> 1 */
    uint8_t rounds_bonus = (uint8_t)((last_bomb_room & 0x0F) >> 1);
    gdv = BCD_Add(gdv, rounds_bonus);

    /* Extra lives found: extra_lives >> 1 */
    uint8_t lives_bonus = (uint8_t)(extra_lives >> 1);
    gdv = BCD_Add(gdv, lives_bonus);

    /* Score threshold: 1 point per 200,000 pts (max 5) */
    uint8_t score_bonus = (uint8_t)(total_score / 200000);
    if (score_bonus > 5) score_bonus = 5;
    gdv = BCD_Add(gdv, score_bonus);

    return gdv;
}

bool TortureRoom_CheckGreed(const NES_RAM *ram) {
    if (!ram) return false;
    /* Greed conditions: Timer >= 100 or Mighty Coins >= 10 */
    if (ram->player.mighty_coins >= 10) return true;
    if (ram->stage_timer_bcd >= 0x99) return true;
    return false;
}

void TortureRoom_Trigger(NES_RAM *ram) {
    if (!ram) return;
    /* USA Version: exactly 50 jumps in BCD (ADR-004) */
    ram->torture_jumps_bcd = 0x50;
    ram->player.mighty_coins = 0;
}

bool TortureRoom_OnJump(NES_RAM *ram) {
    if (!ram) return false;
    if (ram->torture_jumps_bcd > 0) {
        ram->torture_jumps_bcd = BCD_Sub1(ram->torture_jumps_bcd);
        if (ram->torture_jumps_bcd == 0) {
            return true; /* Penalty fulfilled! */
        }
    }
    return false;
}

/* Helper to render a 1-byte BCD number (2 digits) */
static void RenderBCDDigits(uint32_t *screen_buffer, int x, int y, uint8_t bcd, const CHRBank *chr, const uint8_t *pal) {
    uint8_t hi = (bcd >> 4) & 0x0F;
    uint8_t lo = bcd & 0x0F;
    /* Digits 0..9 are at CHR tile 0x30..0x39 in ASCII/NES font */
    Renderer_BlitTile(screen_buffer, x, y, chr, (uint8_t)(0x30 + hi), pal, false, false, true);
    Renderer_BlitTile(screen_buffer, x + 8, y, chr, (uint8_t)(0x30 + lo), pal, false, false, true);
}

void HUD_Render(uint32_t *screen_buffer, const NES_RAM *ram, const CHRBank *chr, uint32_t frame_count) {
    if (!screen_buffer || !ram || !chr) return;

    /* Palette: White/Cyan text */
    const uint8_t hud_pal[4] = { 0x0F, 0x30, 0x12, 0x0F };

    /* 1. Score: 8 Digits at X = 16..79, Y = 8 */
    bool zero_suppress = true;
    int cur_x = 16;
    for (int i = 3; i >= 0; --i) {
        uint8_t byte = ram->player_score_bcd[i];
        uint8_t hi = (byte >> 4) & 0x0F;
        uint8_t lo = byte & 0x0F;

        if (zero_suppress && hi == 0 && i > 0) {
            /* Blank space */
        } else {
            zero_suppress = false;
            Renderer_BlitTile(screen_buffer, cur_x, 8, chr, (uint8_t)(0x30 + hi), hud_pal, false, false, true);
        }
        cur_x += 8;

        if (zero_suppress && lo == 0 && i > 0) {
            /* Blank space */
        } else {
            zero_suppress = false;
            Renderer_BlitTile(screen_buffer, cur_x, 8, chr, (uint8_t)(0x30 + lo), hud_pal, false, false, true);
        }
        cur_x += 8;
    }

    /* 2. Timer: 'T' + 2 BCD digits at X = 120, Y = 8 */
    Renderer_BlitTile(screen_buffer, 112, 8, chr, 'T', hud_pal, false, false, true);
    RenderBCDDigits(screen_buffer, 120, 8, ram->stage_timer_bcd, chr, hud_pal);

    /* 3. Mighty Coins: 'M' + count at X = 184, Y = 8 */
    Renderer_BlitTile(screen_buffer, 184, 8, chr, 'M', hud_pal, false, false, true);
    Renderer_BlitTile(screen_buffer, 192, 8, chr, (uint8_t)(0x30 + (ram->player.mighty_coins & 0x0F)), hud_pal, false, false, true);

    /* 4. Torture Room Big Counter: if in torture room, render centered */
    if (ram->torture_jumps_bcd > 0 && ram->zp_scratch.byte_10 == 0xAC) {
        /* Big jump countdown display */
        if ((frame_count & 16) == 0) {
            RenderBCDDigits(screen_buffer, 120, 64, ram->torture_jumps_bcd, chr, hud_pal);
        }
    }
}
