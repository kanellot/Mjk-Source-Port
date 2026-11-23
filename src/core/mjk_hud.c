/**
 * @file mjk_hud.c
 * @brief Implementación de aritmética BCD, puntuación, GDV y finales.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: hud_scoring_contract.md (asm/src/prg.asm:1038-1118 y 7376-7450)
 */

#include "mjk_hud.h"
#include <stdio.h>
#include <string.h>

uint8_t MjkBCD_AddByte(uint8_t a, uint8_t b, uint8_t* carry) {
    uint8_t c_in = carry ? *carry : 0;
    uint8_t lo = (uint8_t)((a & 0x0F) + (b & 0x0F) + c_in);
    uint8_t hi = (uint8_t)((a >> 4) + (b >> 4));

    if (lo >= 10) {
        lo -= 10;
        hi += 1;
    }

    if (hi >= 10) {
        hi -= 10;
        if (carry) *carry = 1;
    } else {
        if (carry) *carry = 0;
    }

    return (uint8_t)((hi << 4) | (lo & 0x0F));
}

uint8_t MjkBCD_Sub1(uint8_t a) {
    uint8_t lo = a & 0x0F;
    uint8_t hi = (a >> 4) & 0x0F;

    if (lo > 0) {
        lo--;
    } else if (hi > 0) {
        lo = 9;
        hi--;
    } else {
        return 0; // Ya está en cero
    }

    return (uint8_t)((hi << 4) | lo);
}

void MjkBCD_AddScore(uint8_t score_bcd[4], uint32_t points) {
    if (!score_bcd || points == 0) return;

    // Convertir el número de puntos a 4 bytes BCD
    uint8_t pts_bcd[4] = {0, 0, 0, 0};
    uint32_t temp = points;
    
    // Extraer pares de dígitos de derecha a izquierda
    pts_bcd[3] = (uint8_t)(((temp % 100) / 10) << 4 | (temp % 10)); temp /= 100;
    pts_bcd[2] = (uint8_t)(((temp % 100) / 10) << 4 | (temp % 10)); temp /= 100;
    pts_bcd[1] = (uint8_t)(((temp % 100) / 10) << 4 | (temp % 10)); temp /= 100;
    pts_bcd[0] = (uint8_t)(((temp % 100) / 10) << 4 | (temp % 10));

    // Sumar con acarreo en cadena desde el byte menos significativo
    uint8_t carry = 0;
    score_bcd[3] = MjkBCD_AddByte(score_bcd[3], pts_bcd[3], &carry);
    score_bcd[2] = MjkBCD_AddByte(score_bcd[2], pts_bcd[2], &carry);
    score_bcd[1] = MjkBCD_AddByte(score_bcd[1], pts_bcd[1], &carry);
    score_bcd[0] = MjkBCD_AddByte(score_bcd[0], pts_bcd[0], &carry);
}

uint32_t MjkBCD_ToInteger(const uint8_t score_bcd[4]) {
    if (!score_bcd) return 0;
    uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t hi = (score_bcd[i] >> 4) & 0x0F;
        uint8_t lo = score_bcd[i] & 0x0F;
        result = result * 100 + (hi * 10 + lo);
    }
    return result;
}

void MjkHUD_UpdateGDV(uint8_t* gdv, int8_t delta_bcd) {
    if (!gdv) return;

    if (delta_bcd > 0) {
        uint8_t carry = 0;
        *gdv = MjkBCD_AddByte(*gdv, (uint8_t)delta_bcd, &carry);
        if (carry || *gdv > 0x99) *gdv = 0x99;
    } else if (delta_bcd < 0) {
        int count = -delta_bcd;
        for (int i = 0; i < count; ++i) {
            *gdv = MjkBCD_Sub1(*gdv);
        }
    }
}

uint8_t MjkHUD_EvaluateEnding(uint8_t gdv, bool has_all_secrets) {
    // Ending 3 (Best): GDV >= 90 y todos los secretos
    if (gdv >= 0x90 && has_all_secrets) {
        return 3;
    }
    // Ending 2 (Excelente): GDV >= 75
    if (gdv >= 0x75) {
        return 2;
    }
    // Ending 1 (Bueno): GDV >= 50
    if (gdv >= 0x50) {
        return 1;
    }
    // Ending 0 (Pobre): GDV < 50
    return 0;
}

void MjkHUD_FormatString(const uint8_t score_bcd[4], uint16_t timer_val, uint8_t round_val, char out_str[64]) {
    if (!out_str) return;

    uint32_t score_int = MjkBCD_ToInteger(score_bcd);
    snprintf(out_str, 64, "1UP %06u   TIME %02u   ROUND %02u", score_int, timer_val, round_val);
}
