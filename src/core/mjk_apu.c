/**
 * @file mjk_apu.c
 * @brief Implementación del sintetizador de audio chiptune APU 2A03 de la NES y motor de bytecode Tecmo.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: RES-030, RES-014, music-engine.asm y music-data.asm
 */

#include "mjk_apu.h"
#include <string.h>

// =========================================================================
// 1. Tablas Maestras del Sintetizador 2A03 (music-engine.asm / music-data.asm)
// =========================================================================

// Ciclos de trabajo canónicos de los canales Pulse 1 y Pulse 2
static const uint8_t DutyTable[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25.0%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50.0%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 75.0%
};

// Secuencia de 32 pasos simétrica del canal Triangle
static const uint8_t TriangleTable[32] = {
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

// Tabla de períodos de 11 bits canónicos (music-engine.asm:776-792)
static const uint16_t k_note_frequency_table[16] = {
    0x0591, 0x0541, 0x04F5, 0x04AE, 0x046B, 0x042B, 0x03EF, 0x03B7,
    0x0381, 0x034F, 0x031F, 0x02F2, 0x0000, 0x0000, 0x0000, 0x0000
};

// Tabla de duraciones en frames NTSC a 60 Hz (music-data.asm:5-37)
static const uint8_t k_note_lengths[32] = {
     0,  1,  2,  3,  4,  6,  8, 12, 16, 24, 32, 48, 64, 96, 128, 80,
     7, 14, 28,  5, 10, 60,  6,  9, 12, 18, 24, 36, 48, 72,  96, 144
};

// Modulación de tono Pitch Bend (music-engine.asm:774-775 byte_B52C)
static const int8_t k_pitch_bend_steps[4] = { 1, 2, -1, -2 };

// Períodos de temporizador para el canal de ruido 2A03 ($400E)
static const uint16_t NoisePeriodTable[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

// Envolventes de volumen maestras (music-data.asm:38-125)
static const uint8_t k_vol_envelope_0[] = {
    0x1F, 1, 0x1E, 1, 0x1D, 1, 0x1C, 1, 0x1B, 1, 0x1A, 1, 0x19, 1, 0x18, 1,
    0x17, 1, 0x16, 1, 0x15, 1, 0x14, 1, 0x13, 1, 0x12, 1, 0x11, 1, 0x10, 0xFF
};

static const uint8_t k_vol_envelope_1[] = {
    0x1D, 1, 0x1E, 1, 0x1F, 0xFF
};

static const uint8_t k_vol_envelope_2[] = {
    0x1F, 1, 0x1D, 1, 0x1B, 1, 0x19, 1, 0x17, 1, 0x15, 1, 0x13, 1, 0x01, 1, 0x10, 0xFF
};

static const uint8_t k_vol_envelope_3[] = {
    0x12, 1, 0x14, 1, 0x16, 1, 0x18, 1, 0x1A, 1, 0x1C, 1, 0x1E, 1, 0x1F, 0x30,
    0x1E, 1, 0x1D, 1, 0x1C, 1, 0x1B, 1, 0x1A, 1, 0x19, 1, 0x18, 1, 0x17, 1,
    0x16, 1, 0x15, 1, 0x14, 1, 0x13, 1, 0x12, 1, 0x11, 1, 0x10, 0xFF
};

static const uint8_t k_vol_envelope_4[] = {
    0x1F, 0xFF
};

static const uint8_t k_vol_envelope_5[] = {
    0x1F, 2, 0x10, 0xFF
};

static const uint8_t k_vol_envelope_6[] = {
    0x7F, 0xFF
};

static const uint8_t k_vol_envelope_7[] = {
    0x1F, 4, 0x1E, 2, 0x1D, 2, 0x1C, 2, 0x1B, 3, 0x1A, 3, 0x19, 4, 0x18, 4,
    0x17, 6, 0x16, 6, 0x15, 8, 0x14, 8, 0x13, 8, 0x12, 8, 0x11, 0x10, 0x10, 0xFF
};

static const uint8_t* const k_vol_envelopes[8] = {
    k_vol_envelope_0,
    k_vol_envelope_1,
    k_vol_envelope_2,
    k_vol_envelope_3,
    k_vol_envelope_4,
    k_vol_envelope_5,
    k_vol_envelope_6,
    k_vol_envelope_7
};

// =========================================================================
// 2. Bytecode Canónico de Temas BGM (music-data.asm)
// =========================================================================

// --- Tema Principal / Round 1 (SoundData_16_Music_MainIntro / SoundData_10) ---
#define OFF_P1_10_0    26
#define OFF_P1_10_0L   27
#define OFF_P1_10_0LA  105
#define OFF_P1_F7AB    142

static const uint8_t s_track_p1[] = {
    // SoundTrack_16_0 (Intro - 26 bytes)
    /* 00 */ 0xA0,
    /* 01 */ 0xD1,
    /* 02 */ 0xE4,
    /* 03 */ 0x92,
    /* 04 */ 0x37,
    /* 05 */ 0x90,
    /* 06 */ 0x37, 0x39, 0x37, 0x35,
    /* 10 */ 0x91,
    /* 11 */ 0x34, 0x32, 0x30, 0x2B, 0x27,
    /* 16 */ 0x0C,
    /* 17 */ 0x2B,
    /* 18 */ 0x0C,
    /* 19 */ 0x32,
    /* 20 */ 0x0C, 0x0C, 0x0C,
    /* 23 */ 0xF0, (uint8_t)(OFF_P1_10_0 & 0xFF), (uint8_t)(OFF_P1_10_0 >> 8),

    // SoundTrack_10_0 (Offset 26)
    /* 26 */ 0xA0,

    // SoundTrack_10_0L (Offset 27)
    /* 27 */ 0xD1,
    /* 28 */ 0xE9,
    /* 29 */ 0xF3, (uint8_t)(OFF_P1_10_0LA & 0xFF), (uint8_t)(OFF_P1_10_0LA >> 8),
    /* 32 */ 0x90,
    /* 33 */ 0x30, 0x2B, 0x29, 0x27, 0x32, 0x30, 0x2B, 0x29,
    /* 41 */ 0x34, 0x32, 0x30, 0x2B, 0x35, 0x34, 0x32, 0x2B,
    /* 49 */ 0xF3, (uint8_t)(OFF_P1_10_0LA & 0xFF), (uint8_t)(OFF_P1_10_0LA >> 8),
    /* 52 */ 0x30, 0x27,
    /* 54 */ 0x90,
    /* 55 */ 0x29, 0x2B, 0x32, 0x27,
    /* 59 */ 0x91,
    /* 60 */ 0x30,
    /* 61 */ 0x90,
    /* 62 */ 0x27, 0x27,
    /* 64 */ 0x91,
    /* 65 */ 0x30, 0x0C,
    /* 67 */ 0xF3, (uint8_t)(OFF_P1_F7AB & 0xFF), (uint8_t)(OFF_P1_F7AB >> 8),
    /* 70 */ 0x0C,
    /* 71 */ 0x90,
    /* 72 */ 0x35, 0x34,
    /* 74 */ 0x91,
    /* 75 */ 0x35,
    /* 76 */ 0x90,
    /* 77 */ 0x37,
    /* 78 */ 0x91,
    /* 79 */ 0x37,
    /* 80 */ 0x90,
    /* 81 */ 0x35, 0x34, 0x32, 0x34, 0x35, 0x37, 0x36, 0x37, 0x38,
    /* 90 */ 0xF3, (uint8_t)(OFF_P1_F7AB & 0xFF), (uint8_t)(OFF_P1_F7AB >> 8),
    /* 93 */ 0x39, 0x37, 0x35, 0x34, 0x32, 0x30, 0x2B, 0x30, 0x0C,
    /* 102 */ 0xF0, (uint8_t)(OFF_P1_10_0L & 0xFF), (uint8_t)(OFF_P1_10_0L >> 8),

    // SoundTrack_10_0La (Offset 105 - Subrutina Melódica A)
    /* 105 */ 0x90,
    /* 106 */ 0x34, 0x30, 0x34, 0x30,
    /* 110 */ 0x91,
    /* 111 */ 0x34, 0x37, 0x39, 0x37, 0x35, 0x34, 0x32, 0x27,
    /* 119 */ 0x90,
    /* 120 */ 0x27, 0x29, 0x2B, 0x30,
    /* 124 */ 0x91,
    /* 125 */ 0x32, 0x0C, 0x27, 0x0C,
    /* 129 */ 0x90,
    /* 130 */ 0x32, 0x2B, 0x32, 0x2B,
    /* 134 */ 0x91,
    /* 135 */ 0x32, 0x35, 0x37, 0x35, 0x34, 0x32,
    /* 141 */ 0xF4,

    // SoundTrack_F7AB (Offset 142 - Subrutina Melódica B)
    /* 142 */ 0x90,
    /* 143 */ 0x39, 0x38,
    /* 145 */ 0x91,
    /* 146 */ 0x39,
    /* 147 */ 0x90,
    /* 148 */ 0x3B,
    /* 149 */ 0x91,
    /* 150 */ 0x3B,
    /* 151 */ 0x90,
    /* 152 */ 0x39,
    /* 153 */ 0x91,
    /* 154 */ 0x37, 0x34, 0x30,
    /* 157 */ 0xF4
};

#define OFF_P2_10_1    26
#define OFF_P2_10_1L   27
#define OFF_P2_F80A    105
#define OFF_P2_F82F    142

static const uint8_t s_track_p2[] = {
    // SoundTrack_16_1 (Intro - 26 bytes)
    /* 00 */ 0xA0,
    /* 01 */ 0xD1,
    /* 02 */ 0xE4,
    /* 03 */ 0x92,
    /* 04 */ 0x30,
    /* 05 */ 0x90,
    /* 06 */ 0x34, 0x35, 0x34, 0x32,
    /* 10 */ 0x91,
    /* 11 */ 0x30, 0x24, 0x29, 0x27, 0x22,
    /* 16 */ 0x0C,
    /* 17 */ 0x27,
    /* 18 */ 0x0C,
    /* 19 */ 0x2B,
    /* 20 */ 0x0C, 0x0C, 0x0C,
    /* 23 */ 0xF0, (uint8_t)(OFF_P2_10_1 & 0xFF), (uint8_t)(OFF_P2_10_1 >> 8),

    // SoundTrack_10_1 (Offset 26)
    /* 26 */ 0xA0,

    // SoundTrack_10_1L (Offset 27)
    /* 27 */ 0xD1,
    /* 28 */ 0xE9,
    /* 29 */ 0xF3, (uint8_t)(OFF_P2_F80A & 0xFF), (uint8_t)(OFF_P2_F80A >> 8),
    /* 32 */ 0x90,
    /* 33 */ 0x29, 0x27, 0x25, 0x24, 0x2B, 0x29, 0x27, 0x25,
    /* 41 */ 0x30, 0x2B, 0x29, 0x27, 0x32, 0x30, 0x2B, 0x27,
    /* 49 */ 0xF3, (uint8_t)(OFF_P2_F80A & 0xFF), (uint8_t)(OFF_P2_F80A >> 8),
    /* 52 */ 0x27, 0x24,
    /* 54 */ 0x90,
    /* 55 */ 0x25, 0x27, 0x2B, 0x22,
    /* 59 */ 0x91,
    /* 60 */ 0x27,
    /* 61 */ 0x90,
    /* 62 */ 0x22, 0x22,
    /* 64 */ 0x91,
    /* 65 */ 0x27, 0x0C,
    /* 67 */ 0xF3, (uint8_t)(OFF_P2_F82F & 0xFF), (uint8_t)(OFF_P2_F82F >> 8),
    /* 70 */ 0x0C,
    /* 71 */ 0x90,
    /* 72 */ 0x32, 0x30,
    /* 74 */ 0x91,
    /* 75 */ 0x32,
    /* 76 */ 0x90,
    /* 77 */ 0x34,
    /* 78 */ 0x91,
    /* 79 */ 0x34,
    /* 80 */ 0x90,
    /* 81 */ 0x32, 0x30, 0x2B, 0x30, 0x32, 0x34, 0x33, 0x34, 0x34,
    /* 90 */ 0xF3, (uint8_t)(OFF_P2_F82F & 0xFF), (uint8_t)(OFF_P2_F82F >> 8),
    /* 93 */ 0x35, 0x34, 0x32, 0x30, 0x2B, 0x27, 0x27, 0x27, 0x0C,
    /* 102 */ 0xF0, (uint8_t)(OFF_P2_10_1L & 0xFF), (uint8_t)(OFF_P2_10_1L >> 8),

    // SoundTrack_F80A (Offset 105 - Armonía Frase A)
    /* 105 */ 0x90,
    /* 106 */ 0x30, 0x27, 0x30, 0x27,
    /* 110 */ 0x91,
    /* 111 */ 0x30, 0x34, 0x35, 0x34, 0x32, 0x30, 0x2B, 0x22,
    /* 119 */ 0x90,
    /* 120 */ 0x22, 0x25, 0x27, 0x29,
    /* 124 */ 0x91,
    /* 125 */ 0x2B, 0x0C, 0x22, 0x0C,
    /* 129 */ 0x90,
    /* 130 */ 0x2B, 0x27, 0x2B, 0x27,
    /* 134 */ 0x91,
    /* 135 */ 0x2B, 0x32, 0x34, 0x32, 0x30, 0x2B,
    /* 141 */ 0xF4,

    // SoundTrack_F82F (Offset 142 - Armonía Frase B)
    /* 142 */ 0x90,
    /* 143 */ 0x35, 0x34,
    /* 145 */ 0x91,
    /* 146 */ 0x35,
    /* 147 */ 0x90,
    /* 148 */ 0x37,
    /* 149 */ 0x91,
    /* 150 */ 0x37,
    /* 151 */ 0x90,
    /* 152 */ 0x35,
    /* 153 */ 0x91,
    /* 154 */ 0x34, 0x30, 0x27,
    /* 157 */ 0xF4
};

#define OFF_TRI_10_2   29
#define OFF_TRI_10_2L  30
#define OFF_TRI_F894   114

static const uint8_t s_track_tri[] = {
    // SoundTrack_16_2 (Intro - 29 bytes)
    /* 00 */ 0xA4,
    /* 01 */ 0x92,
    /* 02 */ 0x30,
    /* 03 */ 0x90,
    /* 04 */ 0x30, 0x2B, 0x30, 0x2B,
    /* 08 */ 0x91,
    /* 09 */ 0x30, 0x32, 0x34, 0x35, 0x37, 0x0C, 0x32, 0x2B,
    /* 17 */ 0x90,
    /* 18 */ 0x37, 0x0C, 0x35, 0x0C, 0x34, 0x0C, 0x32, 0x0C,
    /* 26 */ 0xF0, (uint8_t)(OFF_TRI_10_2 & 0xFF), (uint8_t)(OFF_TRI_10_2 >> 8),

    // SoundTrack_10_2 (Offset 29)
    /* 29 */ 0xA4,

    // SoundTrack_10_2L (Offset 30)
    /* 30 */ 0xF3, (uint8_t)(OFF_TRI_F894 & 0xFF), (uint8_t)(OFF_TRI_F894 >> 8),
    /* 33 */ 0x17, 0x22, 0x19, 0x24, 0x1B, 0x25, 0x1B, 0x27,
    /* 41 */ 0xF3, (uint8_t)(OFF_TRI_F894 & 0xFF), (uint8_t)(OFF_TRI_F894 >> 8),
    /* 44 */ 0x17, 0x20,
    /* 46 */ 0x90,
    /* 47 */ 0x25, 0x24, 0x22, 0x20, 0x1B, 0x17, 0x19, 0x1B,
    /* 55 */ 0x91,
    /* 56 */ 0x20, 0x0C,
    /* 58 */ 0x91,
    /* 59 */ 0x19, 0x20,
    /* 61 */ 0x90,
    /* 62 */ 0x25, 0x24, 0x22, 0x20,
    /* 66 */ 0x91,
    /* 67 */ 0x24, 0x20, 0x17, 0x24, 0x22, 0x19,
    /* 73 */ 0x90,
    /* 74 */ 0x25, 0x24,
    /* 76 */ 0x91,
    /* 77 */ 0x22, 0x20, 0x24, 0x27, 0x24, 0x19, 0x20,
    /* 84 */ 0x90,
    /* 85 */ 0x25, 0x24, 0x22, 0x20,
    /* 89 */ 0x91,
    /* 90 */ 0x24, 0x20,
    /* 92 */ 0x90,
    /* 93 */ 0x17, 0x20, 0x24, 0x20,
    /* 97 */ 0x90,
    /* 98 */ 0x19, 0x15, 0x1B, 0x17, 0x20, 0x19, 0x22, 0x1B,
    /* 106 */ 0x91,
    /* 107 */ 0x20, 0x17, 0x20, 0x0C,
    /* 111 */ 0xF0, (uint8_t)(OFF_TRI_10_2L & 0xFF), (uint8_t)(OFF_TRI_10_2L >> 8),

    // SoundTrack_F894 (Offset 114 - Subrutina Bajo Frase A)
    /* 114 */ 0x91,
    /* 115 */ 0x20, 0x27, 0x20, 0x27, 0x20, 0x27, 0x20, 0x27,
    /* 123 */ 0x17, 0x22, 0x17, 0x22, 0x17, 0x22,
    /* 129 */ 0x90,
    /* 130 */ 0x17, 0x19, 0x1B, 0x20,
    /* 134 */ 0x91,
    /* 135 */ 0x17, 0x22, 0x17, 0x22, 0x17, 0x22, 0x17, 0x22,
    /* 143 */ 0xF4
};

// --- Game Over ($17 - music-data.asm:1733-1788) ---
static const uint8_t s_track_17_gameover_p1[] = {
    0xA1, 0xD2, 0xE4, 0x88, 0x34, 0x30, 0x34, 0x30, 0x32, 0x2B, 0x32, 0x2B,
    0x8A, 0x29, 0x2B, 0x30, 0x0C, 0xFF
};
static const uint8_t s_track_17_gameover_p2[] = {
    0xA1, 0xD2, 0xE4, 0x88, 0x27, 0x20, 0x27, 0x20, 0x27, 0x1B, 0x27, 0x1B,
    0x8A, 0x25, 0x27, 0x27, 0x0C, 0xFF
};
static const uint8_t s_track_17_gameover_tri[] = {
    0xA6, 0x8A, 0x20, 0x20, 0x1B, 0x1B, 0x8A, 0x19, 0x1B, 0x1B, 0x20, 0x0C, 0xFF
};

// --- Round Clear ($18 - music-data.asm:1789-1845) ---
static const uint8_t s_track_18_roundclear_p1[] = {
    0xA7, 0xD1, 0xE4, 0x9A,
    0xF1, 4, 0x40, 0x47, 0x47, 0xF2,
    0xF1, 4, 0x42, 0x49, 0x49, 0xF2,
    0xF0, 4, 0x00
};
static const uint8_t s_track_18_roundclear_p2[] = {
    0xA7, 0xD1, 0xE4, 0x98, 0x0C, 0x9A,
    0xF1, 4, 0x44, 0x49, 0x44, 0xF2,
    0xF1, 4, 0x45, 0x4B, 0x45, 0xF2,
    0xF0, 6, 0x00
};
static const uint8_t s_track_18_roundclear_tri[] = {
    0xA6, 0x98,
    0xF1, 0x18, 0x20, 0xF2,
    0xF1, 0x18, 0x22, 0xF2,
    0xF0, 2, 0x00
};

// --- Torture Room ($1F - music-data.asm:2025-2045) ---
static const uint8_t s_track_1f_torture_p1[] = {
    0xA0, 0xD1, 0xE6, 0x98, 0x10, 0x15, 0x12, 0x13,
    0xF0, 3, 0x00
};
static const uint8_t s_track_1f_torture_tri[] = {
    0xA4, 0x98, 0x10, 0x15, 0x12, 0x13,
    0xF0, 1, 0x00
};

// =========================================================================
// 3. Bytecode Canónico de Todos los Efectos de Sonido (music-data.asm)
// =========================================================================

// $01: Jump (Canal 5 = SFX Pulse 2)
static const uint8_t s_sfx_01_jump_ch5[] = {
    0xA0, 0xD1, 0xF5, 0x96, 0x84, 0x32, 0xF5, 0x9D, 0x8A, 0x30, 0xFF
};

// $02: Death (Canal 0 = BGM Pulse 1, silencia 1, 2, 4, 5, 6)
static const uint8_t s_sfx_02_death_ch0[] = {
    0xA0, 0xD2, 0x84, 0xF1, 0x13, 0xF5, 0x9C, 0x40, 0xDB, 0xF2, 0xFF
};

// $03: BonusCoinSpawn (Canales 4 y 5)
static const uint8_t s_sfx_03_bonus_coin_ch4[] = {
    0xA0, 0xD1, 0x86, 0x57, 0x8A, 0x57, 0xFF
};
static const uint8_t s_sfx_03_bonus_coin_ch5[] = {
    0xA0, 0xD1, 0x86, 0x56, 0x8A, 0x56, 0xFF
};

// $04: CoinCollected (Canal 5)
static const uint8_t s_sfx_04_coin_collected_ch5[] = {
    0xA0, 0xD1, 0x81, 0x30, 0x34, 0x38, 0x3C, 0x8A, 0x40, 0xFF
};

// $05: PowerCoinSpawn (Canal 0, bucle offset 4)
static const uint8_t s_sfx_05_power_coin_spawn_ch0[] = {
    0xA0, 0xD1, 0xE8, 0x81,
    0xF1, 0x1F, 0x20, 0xD8, 0xF2, 0xF1, 0x1F, 0xDA, 0x47, 0xF2,
    0xF0, 0x04, 0x00
};

// $06: PowerCoinMusic (Canales 0, 1, 2)
static const uint8_t s_sfx_06_power_coin_ch0[] = {
    0xA1, 0xD1, 0xE4, 0x95, 0x37, 0x94, 0x0C, 0x37, 0x95, 0x34, 0x94, 0x0C, 0x34,
    0x95, 0x30, 0x94, 0x0C, 0x93, 0x2B, 0x30, 0x8F, 0x32, 0x93, 0x0C, 0xFF
};
static const uint8_t s_sfx_06_power_coin_ch1[] = {
    0xA1, 0xD1, 0xE4, 0x95, 0x30, 0x94, 0x0C, 0x30, 0x95, 0x29, 0x94, 0x0C, 0x29,
    0x95, 0x25, 0x94, 0x0C, 0x93, 0x24, 0x25, 0x8F, 0x27, 0x93, 0x0C, 0xFF
};
static const uint8_t s_sfx_06_power_coin_ch2[] = {
    0xA4, 0x93, 0x40, 0x0C, 0x40, 0x44, 0x47, 0x0C, 0x49, 0x0C, 0x50, 0x0C, 0x49,
    0x0C, 0x47, 0x0C, 0x44, 0x0C, 0x39, 0x0C, 0x39, 0x40, 0x44, 0x0C, 0x47, 0x0C,
    0x49, 0x0C, 0x47, 0x0C, 0x44, 0x0C, 0x40, 0x0C, 0x35, 0x0C, 0x35, 0x39, 0x40,
    0x0C, 0x45, 0x0C, 0x47, 0x0C, 0x45, 0x0C, 0x40, 0x0C, 0x39, 0x0C, 0x37, 0x0C,
    0x37, 0x3B, 0x42, 0x0C, 0x45, 0x0C, 0x47, 0x0C, 0x45, 0x0C, 0x42, 0x0C, 0x3B,
    0x0C, 0x37, 0xFF
};

// $07: BombCollectedUnlit (Canal 5)
static const uint8_t s_sfx_07_bomb_unlit_ch5[] = {
    0xA0, 0xD1, 0x82, 0x47, 0x0C, 0x48, 0xFF
};

// $08: BombCollectedLit (Canal 5)
static const uint8_t s_sfx_08_bomb_lit_ch5[] = {
    0xA0, 0xD1, 0x84, 0xF5, 0x9B, 0x34, 0xF5, 0x9B, 0x37, 0xFF
};

// $09: YouAreGreedy (Canales 0 y 1)
static const uint8_t s_sfx_09_greedy_ch0[] = {
    0xA1, 0xD1, 0x80, 0xF5, 0x97, 0x34, 0xFF
};
static const uint8_t s_sfx_09_greedy_ch1[] = {
    0xA1, 0xD1, 0x80, 0xF5, 0x97, 0x35, 0xFF
};

// $0A: 1UP_Captive (Canales 4, 5, 6)
static const uint8_t s_sfx_0a_1up_ch4[] = {
    0xA0, 0xD1, 0x84, 0x40, 0x37, 0x39, 0x34, 0x39, 0x3B, 0x8A, 0x40, 0xFF
};
static const uint8_t s_sfx_0a_1up_ch5[] = {
    0xA0, 0xD1, 0x84, 0x37, 0x34, 0x34, 0x30, 0x35, 0x37, 0x8A, 0x37, 0xFF
};
static const uint8_t s_sfx_0a_1up_ch6[] = {
    0x89, 0x0C, 0x8A, 0x0C, 0xFF
};

// $0B: CrystalBall (Canales 4, 5, 6)
static const uint8_t s_sfx_0b_crystal_ch4[] = {
    0xA1, 0xD1, 0x96, 0x34, 0x0C, 0x34, 0x37, 0x40, 0x0C, 0x37, 0x0C, 0x9A, 0x40, 0x0C, 0xFF
};
static const uint8_t s_sfx_0b_crystal_ch5[] = {
    0xA1, 0xD1, 0x96, 0x30, 0x0C, 0x30, 0x34, 0x37, 0x0C, 0x34, 0x0C, 0x9A, 0x37, 0x0C, 0xFF
};
static const uint8_t s_sfx_0b_crystal_ch6[] = {
    0xA4, 0x96, 0x27, 0x0C, 0x27, 0x30, 0x34, 0x0C, 0x30, 0x0C, 0x9A, 0x34, 0x0C, 0xFF
};

// $0D: BonusStatueCollected (Canal 5)
static const uint8_t s_sfx_0d_statue_ch5[] = {
    0xA0, 0xD1, 0x82, 0x30, 0x36, 0x33, 0x39, 0x36, 0x40, 0x39, 0x43, 0xFF
};

// $0E: SecretBlockRevealed (Canal 5)
static const uint8_t s_sfx_0e_secret_block_ch5[] = {
    0xA0, 0xD1, 0x81, 0x40, 0x36, 0x39, 0x33, 0x36, 0x30, 0x33, 0x29, 0x30, 0x26, 0x29, 0x23, 0x26, 0x20, 0xFF
};

// $0F: Door (Canales 4, 5, 6)
static const uint8_t s_sfx_0f_door_ch4[] = {
    0xA0, 0xD1, 0x82, 0xF1, 0x0B, 0x30, 0x32, 0xD8, 0xF2, 0xFF
};
static const uint8_t s_sfx_0f_door_ch5[] = {
    0xA0, 0xD1, 0x82, 0xF1, 0x0B, 0x27, 0x29, 0xD8, 0xF2, 0xFF
};
static const uint8_t s_sfx_0f_door_ch6[] = {
    0x99, 0x0C, 0xFF
};

// $19: BombChestBonus (Canales 4, 5, 6)
static const uint8_t s_sfx_19_bomb_chest_ch4[] = {
    0xA0, 0xD1, 0x84, 0x44, 0x40, 0x45, 0x42, 0x47, 0x44, 0x49, 0x45, 0x8A, 0x4B, 0xFF
};
static const uint8_t s_sfx_19_bomb_chest_ch5[] = {
    0xA0, 0xD1, 0x84, 0x40, 0x37, 0x42, 0x39, 0x44, 0x3B, 0x45, 0x40, 0x8A, 0x47, 0xFF
};
static const uint8_t s_sfx_19_bomb_chest_ch6[] = {
    0xA4, 0x8C, 0x0C, 0xFF
};

// $1A: Sound_1A (Canales 0 y 1)
static const uint8_t s_sfx_1a_ch0[] = {
    0xA0, 0xD1, 0x81, 0x37, 0x38, 0x39, 0x3A, 0xFF
};
static const uint8_t s_sfx_1a_ch1[] = {
    0xA0, 0xD2, 0x81, 0x36, 0x37, 0x38, 0x39, 0xFF
};

// $1C: Unpause (Canales 4, 5, 6)
static const uint8_t s_sfx_1c_unpause_ch4[] = {
    0xA0, 0xD2, 0x85, 0x40, 0x37, 0x40, 0x8A, 0x37, 0xFF
};
static const uint8_t s_sfx_1c_unpause_ch5[] = {
    0xA0, 0xD2, 0x85, 0x37, 0x30, 0x37, 0x8A, 0x30, 0xFF
};
static const uint8_t s_sfx_1c_unpause_ch6[] = {
    0x85, 0x0C, 0x0C, 0x0C, 0x8A, 0x0C, 0xFF
};

// $1D: UsedMightyCoin (Canales 4, 5, 6)
static const uint8_t s_sfx_1d_coin_ch4[] = {
    0xA0, 0xD1,
    0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xE3, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xE6, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xE9, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xEB, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0xFF
};
static const uint8_t s_sfx_1d_coin_ch5[] = {
    0xA0, 0xD2, 0xDA,
    0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xE3, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xE6, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xE9, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0x84, 0x0C,
    0xEB, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0xFF
};
static const uint8_t s_sfx_1d_coin_ch6[] = {
    0xA4, 0xDA, 0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0xFF
};

// $20: BombBonus (Canales 4, 5, 6, 7)
static const uint8_t s_sfx_20_bomb_bonus_ch4[] = {
    0xA0, 0xD1,
    0x81, 0x10, 0x13, 0x12, 0x0B, 0x11, 0x84, 0x0C,
    0xE1, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE2, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE3, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE4, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE6, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE8, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xEC, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11, 0xFF
};
static const uint8_t s_sfx_20_bomb_bonus_ch5[] = {
    0x82, 0x0C,
    0xA0, 0xD1,
    0x81, 0x10, 0x13, 0x12, 0x0B, 0x11, 0x84, 0x0C,
    0xE1, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE2, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE3, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE4, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE6, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xE8, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11,
    0xEC, 0x81, 0x10, 0x13, 0x12, 0x0B, 0x11, 0xFF
};
static const uint8_t s_sfx_20_bomb_bonus_ch6[] = {
    0xA6, 0x84, 0x20, 0x0C, 0x8E, 0x21, 0xFF
};
static const uint8_t s_sfx_20_bomb_bonus_ch7[] = {
    0xA5, 0x86, 0xCF, 0xA7, 0x8E, 0xCD, 0xFF
};

// $21: MightyDrink / TimePickup (Canales 4, 5, 6)
static const uint8_t s_sfx_21_mighty_drink_ch4[] = {
    0xA7, 0xD1, 0x85, 0x40, 0x44, 0x87, 0x47, 0x89, 0x40, 0xFF
};
static const uint8_t s_sfx_21_mighty_drink_ch5[] = {
    0xA7, 0xD1, 0x85, 0x37, 0x40, 0x87, 0x44, 0x89, 0x37, 0xFF
};
static const uint8_t s_sfx_21_mighty_drink_ch6[] = {
    0x8B, 0x0C, 0xFF
};

// =========================================================================
// 4. Intérprete de Bytecode y Máquina de Estados por Canal
// =========================================================================

static void InitChannel(MjkSoundChannel* ch, const uint8_t* stream, const uint8_t* base) {
    if (!ch) return;
    memset(ch, 0, sizeof(MjkSoundChannel));
    ch->status = 1;
    ch->duration_counter = 1; // Provoca la lectura de bytecode inmediatamente en el frame 0
    ch->duration_reload = 0;
    ch->data_ptr = stream;
    ch->track_base = base;
    ch->vol_delay_counter = 0;
    ch->vol_env_ptr = NULL;
    ch->vol_env_index = 0;
    ch->duty_and_vol = 0;
    ch->sweep_reg = 0x08;
    ch->sweep_divider = 0;
    ch->pitch_bend_step = 0;
    ch->pitch_bend_acc = 0;
    ch->phase = 0.0f;
}

static void Channel_Tick(MjkSoundChannel* ch, int ch_index) {
    if (!ch || !ch->status || !ch->data_ptr) return;

    // 1. Actualización de fase de volumen (MaybeUpdateVolDelay en 6502)
    if (ch->vol_delay_counter == 0) {
        if (ch->vol_env_ptr) {
            uint8_t vol_byte = ch->vol_env_ptr[ch->vol_env_index];
            uint8_t duration = ch->vol_env_ptr[ch->vol_env_index + 1];
            if (duration != 0xFF) {
                ch->vol_env_index += 2;
            }
            ch->vol_delay_counter = duration;

            uint8_t final_vol = vol_byte & 0x0F;
            if (vol_byte & 0x10) {
                if (final_vol >= ch->vol_attenuation) {
                    final_vol -= ch->vol_attenuation;
                } else {
                    final_vol = 0;
                }
            }
            ch->duty_and_vol = (uint8_t)((ch->duty_and_vol & 0xC0) | (final_vol & 0x0F));
        }
    } else {
        ch->vol_delay_counter--;
    }

    // 2. Hardware Sweep en canales Pulse (0, 1, 4, 5) a 60 Hz (RES-030 Secciones 2.2 y 5.1)
    if ((ch_index == 0 || ch_index == 1 || ch_index == 4 || ch_index == 5) &&
        (ch->sweep_reg & 0x80) != 0 && (ch->sweep_reg & 0x07) != 0 && ch->period >= 8) {
        uint16_t delta = ch->period >> (ch->sweep_reg & 7);
        if (ch->sweep_reg & 0x08) { // Negate (tono ascendente / período decreciente)
            ch->period = (ch->period > delta) ? (ch->period - delta) : 0;
        } else { // Positivo (tono descendente / período creciente)
            ch->period += delta;
        }
    }

    // 3. Decrementar duración de nota activa
    ch->duration_counter--;
    if (ch->duration_counter > 0) return;

    // 4. Recargar duración base y reiniciar estado de envolvente (MaybeResetVolDelay)
    ch->duration_counter = ch->duration_reload;
    ch->vol_env_index = 0;
    ch->vol_delay_counter = 0;
    ch->sweep_reg = 0x08;

    if (ch->vol_env_ptr) {
        uint8_t vol_byte = ch->vol_env_ptr[0];
        uint8_t duration = ch->vol_env_ptr[1];
        if (duration != 0xFF) {
            ch->vol_env_index = 2;
        }
        ch->vol_delay_counter = duration;

        uint8_t final_vol = vol_byte & 0x0F;
        if (vol_byte & 0x10) {
            if (final_vol >= ch->vol_attenuation) {
                final_vol -= ch->vol_attenuation;
            } else {
                final_vol = 0;
            }
        }
        ch->duty_and_vol = (uint8_t)((ch->duty_and_vol & 0xC0) | (final_vol & 0x0F));
    }

    // 5. Intérprete secuencial de bytecode Tecmo
    while (ch->status && ch->data_ptr) {
        uint8_t b = *ch->data_ptr++;
        if (b < 0x80) {
            // Emisión de nota o silencio
            if ((b & 0x0F) == 0x0C) {
                // Silencio / rest (semitono $0C)
                ch->period = 0;
            } else {
                uint8_t n = b;
                // Modulación Software Pitch Bend ($D8..$DF / loc_B214 en music-engine.asm)
                if (ch->pitch_bend_step != 0) {
                    uint8_t count = ++ch->pitch_bend_acc;
                    for (uint8_t i = 0; i < count; ++i) {
                        n = (uint8_t)(n + (uint8_t)ch->pitch_bend_step);
                        if ((n & 0x0F) >= 0x0B) {
                            if (ch->pitch_bend_step > 0) {
                                n = (uint8_t)(n + 5);
                            } else {
                                n = (uint8_t)(n - 4);
                            }
                        }
                    }
                }
                uint8_t semitone = n & 0x0F;
                uint8_t octave = (n >> 4) & 0x07;
                uint16_t period = (semitone < 12) ? k_note_frequency_table[semitone] : 0;
                if (octave > 0 && period > 0) {
                    period >>= octave;
                }
                ch->period = period;
            }
            break; // Nota emitida: suspende hasta que expire duration_counter
        }

        // Comandos de bytecode ($80..$FF)
        uint8_t cmd = (b & 0x7F) >> 4;
        switch (cmd) {
            case 0: // 0x80..0x8F
            case 1: // 0x90..0x9F: Set Duration (NoteLengthTable[cmd & 0x1F])
                ch->duration_reload = k_note_lengths[b & 0x1F];
                ch->duration_counter = ch->duration_reload;
                break;

            case 2: // 0xA0..0xAF
            case 3: { // 0xB0..0xBF: Set Volume Envelope
                uint8_t env_idx = b & 0x1F;
                if (env_idx < 8) {
                    ch->vol_env_ptr = k_vol_envelopes[env_idx];
                }
                ch->vol_env_index = 0;
                ch->vol_delay_counter = 0;
                if (ch->vol_env_ptr) {
                    uint8_t vol_byte = ch->vol_env_ptr[0];
                    uint8_t duration = ch->vol_env_ptr[1];
                    if (duration != 0xFF) {
                        ch->vol_env_index = 2;
                    }
                    ch->vol_delay_counter = duration;

                    uint8_t final_vol = vol_byte & 0x0F;
                    if (vol_byte & 0x10) {
                        if (final_vol >= ch->vol_attenuation) {
                            final_vol -= ch->vol_attenuation;
                        } else {
                            final_vol = 0;
                        }
                    }
                    ch->duty_and_vol = (uint8_t)((ch->duty_and_vol & 0xC0) | (final_vol & 0x0F));
                }
                break;
            }

            case 4: // 0xC0..0xCF: Direct Freq / Noise Timer
                ch->period = b & 0x0F;
                return; // Espera a que expire duration_counter

            case 5: // 0xD0..0xDF: Set Duty (<8) o Pitch Bend (>=8)
                if ((b & 0x0F) < 8) {
                    uint8_t duty = (b & 0x03) << 6;
                    ch->duty_and_vol = (uint8_t)((ch->duty_and_vol & 0x3F) | duty);
                } else {
                    int8_t step = k_pitch_bend_steps[b & 3];
                    if (step != ch->pitch_bend_step) {
                        ch->pitch_bend_step = step;
                        ch->pitch_bend_acc = 0;
                    }
                }
                break;

            case 6: // 0xE0..0xEF: Set Attenuation
                ch->vol_attenuation = b & 0x0F;
                break;

            case 7: // 0xF0..0xFF: Subcomandos Fx
                switch (b & 0x07) {
                    case 0: { // F0: JUMP (offset de 16 bits)
                        uint8_t lo = *ch->data_ptr++;
                        uint8_t hi = *ch->data_ptr++;
                        uint16_t target = (uint16_t)(lo | ((uint16_t)hi << 8));
                        ch->data_ptr = ch->track_base + target;
                        break;
                    }
                    case 1: { // F1: LOOP START (count)
                        ch->loop_counter = *ch->data_ptr++;
                        ch->loop_ptr = ch->data_ptr;
                        break;
                    }
                    case 2: { // F2: LOOP END
                        if (ch->loop_counter > 1) {
                            ch->loop_counter--;
                            ch->data_ptr = ch->loop_ptr;
                        } else {
                            ch->loop_counter = 0;
                        }
                        break;
                    }
                    case 3: { // F3: CALL (subrutina)
                        uint8_t lo = *ch->data_ptr++;
                        uint8_t hi = *ch->data_ptr++;
                        uint16_t target = (uint16_t)(lo | ((uint16_t)hi << 8));
                        if (ch->stack_depth < MJK_APU_CALL_STACK_MAX) {
                            ch->call_stack[ch->stack_depth++] = ch->data_ptr;
                        }
                        ch->data_ptr = ch->track_base + target;
                        break;
                    }
                    case 4: { // F4: RET
                        if (ch->stack_depth > 0) {
                            ch->data_ptr = ch->call_stack[--ch->stack_depth];
                        }
                        break;
                    }
                    case 5: { // F5: SET SWEEP (Hardware Sweep $4001 / $4005)
                        ch->sweep_reg = *ch->data_ptr++;
                        ch->sweep_divider = (ch->sweep_reg >> 4) & 7;
                        break;
                    }
                    case 6:
                    case 7: { // F6/F7/FF: END TRACK
                        ch->status = 0;
                        ch->period = 0;
                        return;
                    }
                }
                break;
        }
    }
}

// =========================================================================
// 5. Implementación de la API del APU
// =========================================================================

void MjkAPU_Init(MjkApuState* apu) {
    if (!apu) return;
    memset(apu, 0, sizeof(MjkApuState));

    apu->noise_lfsr = 1; // El LFSR no puede inicializarse a 0
    apu->p1_duty = 2;    // 50% por defecto
    apu->p2_duty = 1;    // 25% por defecto
}

void MjkAPU_PlayMusic(MjkApuState* apu, MjkMusicId music) {
    if (!apu) return;
    if (music == MUSIC_NONE) {
        MjkAPU_StopMusic(apu);
        return;
    }

    apu->music_enabled = true;
    apu->current_music = music;
    apu->music_step_ticks = 0;
    apu->music_step_index = 0;
    apu->music_loop_length = 32;
    apu->noise_enabled = false;
    apu->noise_volume = 0;

    if (music == MUSIC_ROUND_1 || music == MUSIC_MAIN_WITH_INTRO) {
        InitChannel(&apu->channels[0], s_track_p1, s_track_p1);
        InitChannel(&apu->channels[1], s_track_p2, s_track_p2);
        InitChannel(&apu->channels[2], s_track_tri, s_track_tri);
        memset(&apu->channels[3], 0, sizeof(MjkSoundChannel));
    } else if (music == MUSIC_GAME_OVER) {
        InitChannel(&apu->channels[0], s_track_17_gameover_p1, s_track_17_gameover_p1);
        InitChannel(&apu->channels[1], s_track_17_gameover_p2, s_track_17_gameover_p2);
        InitChannel(&apu->channels[2], s_track_17_gameover_tri, s_track_17_gameover_tri);
        memset(&apu->channels[3], 0, sizeof(MjkSoundChannel));
    } else if (music == MUSIC_ROUND_CLEAR) {
        InitChannel(&apu->channels[0], s_track_18_roundclear_p1, s_track_18_roundclear_p1);
        InitChannel(&apu->channels[1], s_track_18_roundclear_p2, s_track_18_roundclear_p2);
        InitChannel(&apu->channels[2], s_track_18_roundclear_tri, s_track_18_roundclear_tri);
        memset(&apu->channels[3], 0, sizeof(MjkSoundChannel));
    } else if (music == MUSIC_TORTURE) {
        InitChannel(&apu->channels[0], s_track_1f_torture_p1, s_track_1f_torture_p1);
        memset(&apu->channels[1], 0, sizeof(MjkSoundChannel));
        InitChannel(&apu->channels[2], s_track_1f_torture_tri, s_track_1f_torture_tri);
        memset(&apu->channels[3], 0, sizeof(MjkSoundChannel));
    } else if (music == MUSIC_POWER_COIN) {
        InitChannel(&apu->channels[0], s_sfx_06_power_coin_ch0, s_sfx_06_power_coin_ch0);
        InitChannel(&apu->channels[1], s_sfx_06_power_coin_ch1, s_sfx_06_power_coin_ch1);
        InitChannel(&apu->channels[2], s_sfx_06_power_coin_ch2, s_sfx_06_power_coin_ch2);
        memset(&apu->channels[3], 0, sizeof(MjkSoundChannel));
    }
}

void MjkAPU_StopMusic(MjkApuState* apu) {
    if (!apu) return;
    apu->music_enabled = false;
    apu->current_music = MUSIC_NONE;

    for (int i = 0; i < 4; ++i) {
        apu->channels[i].status = 0;
        apu->channels[i].period = 0;
    }
}

void MjkAPU_PlaySFX(MjkApuState* apu, uint8_t sfx_id) {
    if (!apu || sfx_id == 0xFF) return;

    if (sfx_id == 0) {
        // Sound_Silence ($00): silencia todos los canales 0..7 (music-engine.asm:49-68)
        MjkAPU_StopMusic(apu);
        for (int i = 0; i < 8; ++i) {
            apu->channels[i].status = 0;
            apu->channels[i].period = 0;
        }
        apu->active_sfx = SFX_NONE;
        apu->sfx_frames_left = 0;
        apu->p1_enabled = false;
        apu->p2_enabled = false;
        apu->tri_enabled = false;
        apu->noise_enabled = false;
        return;
    }

    apu->active_sfx = (MjkSfxId)sfx_id;

    // Resetear canales virtuales de SFX (4..7) y silenciar ruido antes de armar el nuevo SFX
    for (int i = 4; i < 8; ++i) {
        apu->channels[i].status = 0;
        apu->channels[i].period = 0;
        apu->channels[i].duty_and_vol = 0;
    }
    apu->noise_enabled = false;
    apu->noise_volume = 0;

    switch (sfx_id) {
        case 0x01: // Sound_Jump (Canal 5 = SFX Pulse 2)
            InitChannel(&apu->channels[5], s_sfx_01_jump_ch5, s_sfx_01_jump_ch5);
            apu->sfx_frames_left = 18;
            apu->p2_enabled = true;
            apu->p2_period = 149;
            break;

        case 0x02: // Sound_Death (Canal 0 = BGM Pulse 1, silencia 1, 2, 4, 5, 6)
            apu->channels[1].status = 0; apu->channels[1].period = 0;
            apu->channels[2].status = 0; apu->channels[2].period = 0;
            apu->channels[4].status = 0; apu->channels[4].period = 0;
            apu->channels[5].status = 0; apu->channels[5].period = 0;
            apu->channels[6].status = 0; apu->channels[6].period = 0;
            InitChannel(&apu->channels[0], s_sfx_02_death_ch0, s_sfx_02_death_ch0);
            apu->sfx_frames_left = 45;
            break;

        case 0x03: // Sound_BonusCoinSpawn (Canales 4 y 5)
            InitChannel(&apu->channels[4], s_sfx_03_bonus_coin_ch4, s_sfx_03_bonus_coin_ch4);
            InitChannel(&apu->channels[5], s_sfx_03_bonus_coin_ch5, s_sfx_03_bonus_coin_ch5);
            apu->sfx_frames_left = 20;
            break;

        case 0x04: // Sound_CoinCollected (Canal 5)
            InitChannel(&apu->channels[5], s_sfx_04_coin_collected_ch5, s_sfx_04_coin_collected_ch5);
            apu->sfx_frames_left = 16;
            break;

        case 0x05: // Sound_PowerCoinSpawn (Canal 0, silencia 1 y 2)
            apu->channels[1].status = 0; apu->channels[1].period = 0;
            apu->channels[2].status = 0; apu->channels[2].period = 0;
            InitChannel(&apu->channels[0], s_sfx_05_power_coin_spawn_ch0, s_sfx_05_power_coin_spawn_ch0);
            apu->sfx_frames_left = 64;
            break;

        case 0x06: // Sound_PowerCoinMusic
            MjkAPU_PlayMusic(apu, MUSIC_POWER_COIN);
            break;

        case 0x07: // Sound_BombCollectedUnlit (Canal 5)
            InitChannel(&apu->channels[5], s_sfx_07_bomb_unlit_ch5, s_sfx_07_bomb_unlit_ch5);
            apu->sfx_frames_left = 12;
            break;

        case 0x08: // Sound_BombCollectedLit (Canal 5)
            InitChannel(&apu->channels[5], s_sfx_08_bomb_lit_ch5, s_sfx_08_bomb_lit_ch5);
            apu->sfx_frames_left = 16;
            break;

        case 0x09: // Sound_YouAreGreedy (Canales 0 y 1, silencia 2)
            apu->channels[2].status = 0; apu->channels[2].period = 0;
            InitChannel(&apu->channels[0], s_sfx_09_greedy_ch0, s_sfx_09_greedy_ch0);
            InitChannel(&apu->channels[1], s_sfx_09_greedy_ch1, s_sfx_09_greedy_ch1);
            apu->sfx_frames_left = 30;
            break;

        case 0x0A: // Sound_1UP_Captive (Canales 4, 5, 6)
            InitChannel(&apu->channels[4], s_sfx_0a_1up_ch4, s_sfx_0a_1up_ch4);
            InitChannel(&apu->channels[5], s_sfx_0a_1up_ch5, s_sfx_0a_1up_ch5);
            InitChannel(&apu->channels[6], s_sfx_0a_1up_ch6, s_sfx_0a_1up_ch6);
            apu->sfx_frames_left = 40;
            break;

        case 0x0B: // Sound_CrystalBall (Canales 4, 5, 6)
            InitChannel(&apu->channels[4], s_sfx_0b_crystal_ch4, s_sfx_0b_crystal_ch4);
            InitChannel(&apu->channels[5], s_sfx_0b_crystal_ch5, s_sfx_0b_crystal_ch5);
            InitChannel(&apu->channels[6], s_sfx_0b_crystal_ch6, s_sfx_0b_crystal_ch6);
            apu->sfx_frames_left = 32;
            break;

        case 0x0D: // Sound_BonusStatueCollected (Canal 5)
            InitChannel(&apu->channels[5], s_sfx_0d_statue_ch5, s_sfx_0d_statue_ch5);
            apu->sfx_frames_left = 24;
            break;

        case 0x0E: // Sound_SecretBlockRevealed (Canal 5)
            InitChannel(&apu->channels[5], s_sfx_0e_secret_block_ch5, s_sfx_0e_secret_block_ch5);
            apu->sfx_frames_left = 28;
            break;

        case 0x0F: // Sound_Door (Canales 4, 5, 6)
            InitChannel(&apu->channels[4], s_sfx_0f_door_ch4, s_sfx_0f_door_ch4);
            InitChannel(&apu->channels[5], s_sfx_0f_door_ch5, s_sfx_0f_door_ch5);
            InitChannel(&apu->channels[6], s_sfx_0f_door_ch6, s_sfx_0f_door_ch6);
            apu->sfx_frames_left = 32;
            break;

        case 0x10: // Music_Main
            MjkAPU_PlayMusic(apu, MUSIC_ROUND_1);
            break;

        case 0x16: // Music_MainWithIntro
            MjkAPU_PlayMusic(apu, MUSIC_MAIN_WITH_INTRO);
            break;

        case 0x17: // Music_GameOver
            MjkAPU_PlayMusic(apu, MUSIC_GAME_OVER);
            break;

        case 0x18: // Music_RoundClear
            MjkAPU_PlayMusic(apu, MUSIC_ROUND_CLEAR);
            break;

        case 0x19: // Sound_BombChestBonus (Canales 4, 5, 6)
            InitChannel(&apu->channels[4], s_sfx_19_bomb_chest_ch4, s_sfx_19_bomb_chest_ch4);
            InitChannel(&apu->channels[5], s_sfx_19_bomb_chest_ch5, s_sfx_19_bomb_chest_ch5);
            InitChannel(&apu->channels[6], s_sfx_19_bomb_chest_ch6, s_sfx_19_bomb_chest_ch6);
            apu->sfx_frames_left = 32;
            break;

        case 0x1A: // Sound_1A
            InitChannel(&apu->channels[0], s_sfx_1a_ch0, s_sfx_1a_ch0);
            InitChannel(&apu->channels[1], s_sfx_1a_ch1, s_sfx_1a_ch1);
            apu->channels[2].status = 0; apu->channels[2].period = 0;
            apu->sfx_frames_left = 20;
            break;

        case 0x1C: // Sound_Unpause
            InitChannel(&apu->channels[4], s_sfx_1c_unpause_ch4, s_sfx_1c_unpause_ch4);
            InitChannel(&apu->channels[5], s_sfx_1c_unpause_ch5, s_sfx_1c_unpause_ch5);
            InitChannel(&apu->channels[6], s_sfx_1c_unpause_ch6, s_sfx_1c_unpause_ch6);
            apu->sfx_frames_left = 16;
            break;

        case 0x1D: // Sound_UsedMightyCoin
            InitChannel(&apu->channels[4], s_sfx_1d_coin_ch4, s_sfx_1d_coin_ch4);
            InitChannel(&apu->channels[5], s_sfx_1d_coin_ch5, s_sfx_1d_coin_ch5);
            InitChannel(&apu->channels[6], s_sfx_1d_coin_ch6, s_sfx_1d_coin_ch6);
            apu->sfx_frames_left = 40;
            break;

        case 0x1F: // Music_TortureRoom
            MjkAPU_PlayMusic(apu, MUSIC_TORTURE);
            break;

        case 0x20: // Sound_BombBonus (Canales 4, 5, 6, 7)
            InitChannel(&apu->channels[4], s_sfx_20_bomb_bonus_ch4, s_sfx_20_bomb_bonus_ch4);
            InitChannel(&apu->channels[5], s_sfx_20_bomb_bonus_ch5, s_sfx_20_bomb_bonus_ch5);
            InitChannel(&apu->channels[6], s_sfx_20_bomb_bonus_ch6, s_sfx_20_bomb_bonus_ch6);
            InitChannel(&apu->channels[7], s_sfx_20_bomb_bonus_ch7, s_sfx_20_bomb_bonus_ch7);
            apu->sfx_frames_left = 40;
            apu->noise_enabled = true;
            break;

        case 0x21: // Sound_TimePickup (Mighty Drink)
            InitChannel(&apu->channels[4], s_sfx_21_mighty_drink_ch4, s_sfx_21_mighty_drink_ch4);
            InitChannel(&apu->channels[5], s_sfx_21_mighty_drink_ch5, s_sfx_21_mighty_drink_ch5);
            InitChannel(&apu->channels[6], s_sfx_21_mighty_drink_ch6, s_sfx_21_mighty_drink_ch6);
            apu->sfx_frames_left = 24;
            break;

        default:
            break;
    }
}

void MjkAPU_PlaySfx(MjkApuState* apu, MjkSfxId sfx) {
    MjkAPU_PlaySFX(apu, (uint8_t)sfx);
}

void MjkAPU_UpdateFrame(MjkApuState* apu) {
    if (!apu) return;

    // 1. Tick independiente de los 8 canales virtuales canónicos (HandleSound en 6502)
    for (int i = 0; i < 8; ++i) {
        if (i < 4) {
            if (apu->music_enabled) {
                Channel_Tick(&apu->channels[i], i);
            }
        } else {
            Channel_Tick(&apu->channels[i], i);
        }
    }

    if (apu->music_enabled) {
        apu->music_step_ticks++;
    }

    // 2. Control de expiración de SFX y sincronización de canales
    if (apu->sfx_frames_left > 0) {
        apu->sfx_frames_left--;
        if (apu->sfx_frames_left == 0) {
            apu->active_sfx = SFX_NONE;
            for (int i = 4; i < 8; ++i) {
                apu->channels[i].status = 0;
            }
            if (!apu->music_enabled) {
                apu->channels[0].status = 0;
                apu->channels[1].status = 0;
                apu->channels[2].status = 0;
            }
        }
    } else {
        bool any_sfx = false;
        for (int i = 4; i < 8; ++i) {
            if (apu->channels[i].status != 0) {
                any_sfx = true;
                break;
            }
        }
        if (!apu->music_enabled && apu->channels[0].status != 0) {
            any_sfx = true;
        }
        if (!any_sfx) {
            apu->active_sfx = SFX_NONE;
        }
    }

    // 3. Multiplexación limpia de 8 canales virtuales a 4 canales de hardware 2A03:
    //    El canal SFX (i + 4) tiene prioridad estricta si está activo; si no, emite el canal BGM (i).

    // Voice 0: Pulse 1
    if (apu->channels[4].status != 0) {
        apu->p1_enabled = (apu->channels[4].period >= 8);
        apu->p1_period = apu->channels[4].period;
        apu->p1_volume = apu->channels[4].duty_and_vol & 0x0F;
        apu->p1_duty = (apu->channels[4].duty_and_vol >> 6) & 3;
    } else if (apu->music_enabled && apu->channels[0].status != 0) {
        apu->p1_enabled = (apu->channels[0].period >= 8);
        apu->p1_period = apu->channels[0].period;
        apu->p1_volume = apu->channels[0].duty_and_vol & 0x0F;
        apu->p1_duty = (apu->channels[0].duty_and_vol >> 6) & 3;
    } else {
        apu->p1_enabled = false;
        apu->p1_volume = 0;
    }

    // Voice 1: Pulse 2
    if (apu->channels[5].status != 0) {
        apu->p2_enabled = (apu->channels[5].period >= 8);
        apu->p2_period = apu->channels[5].period;
        apu->p2_volume = apu->channels[5].duty_and_vol & 0x0F;
        apu->p2_duty = (apu->channels[5].duty_and_vol >> 6) & 3;
    } else if (apu->music_enabled && apu->channels[1].status != 0) {
        apu->p2_enabled = (apu->channels[1].period >= 8);
        apu->p2_period = apu->channels[1].period;
        apu->p2_volume = apu->channels[1].duty_and_vol & 0x0F;
        apu->p2_duty = (apu->channels[1].duty_and_vol >> 6) & 3;
    } else {
        apu->p2_enabled = false;
        apu->p2_volume = 0;
    }

    // Voice 2: Triangle
    if (apu->channels[6].status != 0) {
        apu->tri_enabled = (apu->channels[6].period >= 1);
        apu->tri_period = apu->channels[6].period;
    } else if (apu->music_enabled && apu->channels[2].status != 0) {
        apu->tri_enabled = (apu->channels[2].period >= 1);
        apu->tri_period = apu->channels[2].period;
    } else {
        apu->tri_enabled = false;
    }

    // Voice 3: Noise
    if (apu->channels[7].status != 0) {
        apu->noise_enabled = true;
        apu->noise_period = apu->channels[7].period;
        apu->noise_volume = apu->channels[7].duty_and_vol & 0x0F;
    } else if (apu->music_enabled && apu->channels[3].status != 0) {
        apu->noise_enabled = true;
        apu->noise_period = apu->channels[3].period;
        apu->noise_volume = apu->channels[3].duty_and_vol & 0x0F;
    } else {
        apu->noise_enabled = false;
        apu->noise_volume = 0;
    }
}

void MjkAPU_GenerateFrameSamples(MjkApuState* apu, int16_t* out_samples, uint32_t sample_count) {
    if (!apu || !out_samples || sample_count == 0) return;

    // 1. Tick de 60 Hz del motor y multiplexación
    MjkAPU_UpdateFrame(apu);

    // 2. Parámetros de osciladores para este frame
    bool p1_active = apu->p1_enabled && apu->p1_volume > 0 && apu->p1_period >= 8;
    bool p2_active = apu->p2_enabled && apu->p2_volume > 0 && apu->p2_period >= 8;
    bool tri_active = apu->tri_enabled && apu->tri_period >= 1;
    bool noise_active = apu->noise_enabled && apu->noise_volume > 0;

    // 3. Incrementos de fase
    float p1_phase_inc = 0.0f;
    if (p1_active) {
        float f1 = 1789773.0f / (16.0f * (float)(apu->p1_period + 1));
        p1_phase_inc = f1 / 44100.0f;
    }

    float p2_phase_inc = 0.0f;
    if (p2_active) {
        float f2 = 1789773.0f / (16.0f * (float)(apu->p2_period + 1));
        p2_phase_inc = f2 / 44100.0f;
    }

    float tri_phase_inc = 0.0f;
    if (tri_active) {
        float ftri = 1789773.0f / (32.0f * (float)(apu->tri_period + 1));
        tri_phase_inc = ftri / 44100.0f;
    }

    uint16_t noise_period_clocks = 16;
    if (noise_active) {
        uint8_t idx = (uint8_t)(apu->noise_period & 0x0F);
        noise_period_clocks = NoisePeriodTable[idx];
    }

    // 4. Síntesis PCM 16-bit a 44.1 kHz
    for (uint32_t s = 0; s < sample_count; ++s) {
        float p1_sample = 0.0f;
        float p2_sample = 0.0f;
        float tri_sample = 0.0f;
        float noise_sample = 0.0f;

        // Pulse 1
        if (p1_active && p1_phase_inc > 0.0f) {
            apu->channels[0].phase += p1_phase_inc;
            if (apu->channels[0].phase >= 1.0f) {
                apu->channels[0].phase -= (float)((int)apu->channels[0].phase);
            }
            uint8_t step = (uint8_t)(apu->channels[0].phase * 8.0f) & 7;
            float pulse_val = DutyTable[apu->p1_duty & 3][step] ? 1.0f : -1.0f;
            p1_sample = pulse_val * ((float)apu->p1_volume / 15.0f) * 4000.0f;
        }

        // Pulse 2
        if (p2_active && p2_phase_inc > 0.0f) {
            apu->channels[1].phase += p2_phase_inc;
            if (apu->channels[1].phase >= 1.0f) {
                apu->channels[1].phase -= (float)((int)apu->channels[1].phase);
            }
            uint8_t step = (uint8_t)(apu->channels[1].phase * 8.0f) & 7;
            float pulse_val = DutyTable[apu->p2_duty & 3][step] ? 1.0f : -1.0f;
            p2_sample = pulse_val * ((float)apu->p2_volume / 15.0f) * 4000.0f;
        }

        // Triangle
        if (tri_active && tri_phase_inc > 0.0f) {
            apu->channels[2].phase += tri_phase_inc;
            if (apu->channels[2].phase >= 1.0f) {
                apu->channels[2].phase -= (float)((int)apu->channels[2].phase);
            }
            uint8_t step = (uint8_t)(apu->channels[2].phase * 32.0f) & 31;
            float tri_val = ((float)TriangleTable[step] - 7.5f) / 7.5f;
            tri_sample = tri_val * 5000.0f;
        }

        // Noise (LFSR con feedback auténtico de 15 bits)
        if (noise_active) {
            apu->noise_timer += 40; // ~40 clocks de CPU por muestra de audio a 44.1 kHz
            while (apu->noise_timer >= noise_period_clocks) {
                apu->noise_timer -= noise_period_clocks;
                uint16_t feedback = (apu->noise_lfsr ^ (apu->noise_lfsr >> 1)) & 1;
                apu->noise_lfsr = (apu->noise_lfsr >> 1) | (feedback << 14);
            }
            float noise_val = (apu->noise_lfsr & 1) ? 1.0f : -1.0f;
            noise_sample = noise_val * ((float)apu->noise_volume / 15.0f) * 3000.0f;
        }

        float mixed = p1_sample + p2_sample + tri_sample + noise_sample;
        out_samples[s] = (int16_t)mixed;
    }
}
