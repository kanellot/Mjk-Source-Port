/**
 * @file mjk_hud.h
 * @brief Aritmética BCD pura, cálculo de GDV y evaluación de finales.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: hud_scoring_contract.md (asm/src/prg.asm:1038-1118 y 7376-7450)
 */

#ifndef MJK_HUD_H
#define MJK_HUD_H

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * COORDENADAS CANÓNICAS DEL HUD (Fila 2 de tiles: Y = 16)
 * FUENTE DE VERDAD: prg.asm (DisplayStatusBar) y nes_ground_truth_spec.md
 * ======================================================================== */
#define MJK_HUD_ROW_Y           16

// Glifo 'T' (tile $1D)
#define MJK_HUD_TIMER_ICON_X    56
#define MJK_HUD_TIMER_ICON_Y    16

// Temporizador BCD (2 dígitos: tens, units)
#define MJK_HUD_TIMER_TENS_X    64
#define MJK_HUD_TIMER_UNITS_X   72
#define MJK_HUD_TIMER_Y         16

// Score BCD (8 dígitos, X = 88..144 con paso 8px)
#define MJK_HUD_SCORE_X_START   88
#define MJK_HUD_SCORE_X_END     144
#define MJK_HUD_SCORE_STEP_X    8
#define MJK_HUD_SCORE_Y         16

// Glifo 'M' (tile $16)
#define MJK_HUD_MIGHTY_ICON_X   184
#define MJK_HUD_MIGHTY_ICON_Y   16

// Mighty Coins (1 dígito)
#define MJK_HUD_MIGHTY_COINS_X  192
#define MJK_HUD_MIGHTY_COINS_Y  16

/**
 * @brief Suma dos bytes en formato BCD empaquetado (0x00 a 0x99).
 */
uint8_t MjkBCD_AddByte(uint8_t a, uint8_t b, uint8_t* carry);

/**
 * @brief Resta 1 a un byte en formato BCD (con borrow entre dígitos).
 */
uint8_t MjkBCD_Sub1(uint8_t a);

/**
 * @brief Suma puntos binarios al marcador de 8 dígitos BCD (4 bytes).
 * @param score_bcd Matriz de 4 bytes BCD [d7d6, d5d4, d3d2, d1d0].
 * @param points Puntos a sumar (por ejemplo 100, 200, 500, 10000).
 */
void MjkBCD_AddScore(uint8_t score_bcd[4], uint32_t points);

/**
 * @brief Convierte los 4 bytes BCD a un entero estándar uint32_t.
 */
uint32_t MjkBCD_ToInteger(const uint8_t score_bcd[4]);

/**
 * @brief Modifica el Game Determination Value (GDV) en aritmética BCD.
 */
void MjkHUD_UpdateGDV(uint8_t* gdv, int8_t delta_bcd);

/**
 * @brief Evalúa cuál de los 4 finales cinemáticos corresponde al GDV y secretos alcanzados.
 * @return 0 = Final Pobre, 1 = Final Bueno, 2 = Final Excelente, 3 = Final Supremo.
 */
uint8_t MjkHUD_EvaluateEnding(uint8_t gdv, bool has_all_secrets);

/**
 * @brief Genera la representación en texto del HUD superior (Score, Timer/Saltos, Round).
 */
void MjkHUD_FormatString(const uint8_t score_bcd[4], uint16_t timer_val, uint8_t round_val, char out_str[64]);

#endif // MJK_HUD_H
