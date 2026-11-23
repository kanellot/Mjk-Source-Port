/**
 * @file sound_data.c
 * @brief Frequency tables, durations, envelopes, and sound bytecode streams.
 * Matching NES music-data.asm and RES-010.
 */

#include "mbj/audio.h"

/* 12-note chromatic scale base frequencies (lowest octave) */
const uint16_t g_note_frequency_table[12] = {
    0x0591, /* 0: C  (Do)  */
    0x0541, /* 1: C# (Do#) */
    0x04F5, /* 2: D  (Re)  */
    0x04AE, /* 3: D# (Re#) */
    0x046B, /* 4: E  (Mi)  */
    0x042B, /* 5: F  (Fa)  */
    0x03EF, /* 6: F# (Fa#) */
    0x03B7, /* 7: G  (Sol) */
    0x0381, /* 8: G# (Sol#)*/
    0x034F, /* 9: A  (La)  */
    0x031F, /* A: A# (La#) */
    0x02F2  /* B: B  (Si)  */
};

/* Standard 32-step note durations (frames at 60 Hz) */
const uint8_t g_note_length_table[32] = {
    1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 255,
    5, 7, 9, 10, 11, 13, 14, 15, 18, 20, 22, 28, 36, 40, 56, 72
};

/* Bytecode stream for Sound_Jump (Pulse 2: upward pitch glide) */
const uint8_t g_sfx_jump[] = {
    0x81,             /* Duration = 2 frames */
    0xD2,             /* 50% duty */
    0x40, 0x44, 0x47, 0x4B, 0x52, /* Upward rapid notes */
    0xF6              /* End track */
};

/* Bytecode stream for Sound_CoinCollected (Pulse 2: high chime) */
const uint8_t g_sfx_coin[] = {
    0x83,             /* Duration = 4 frames */
    0xD1,             /* 25% duty */
    0x5B, 0x62,       /* B5, D6 */
    0xF6
};

/* Bytecode stream for Sound_BombCollectedLit (Triangle: deep pop) */
const uint8_t g_sfx_bomb[] = {
    0x82,             /* Duration = 3 frames */
    0x24, 0x20,       /* Low notes */
    0xF6
};

/* Bytecode stream for Music_Main (Chiptune melody loop) */
const uint8_t g_bgm_main_pulse1[] = {
    0x85,             /* Duration = 8 frames */
    0xD2,             /* 50% duty */
    0xF1, 0x02,       /* Loop start (2 iterations) */
    0x30, 0x34, 0x37, 0x3C,
    0x37, 0x34, 0x30, 0xC0,
    0xF2,             /* Loop end */
    0x32, 0x35, 0x39, 0x3E,
    0x39, 0x35, 0x32, 0xC0,
    0xF0, 0x00, 0x00  /* Loop forever */
};
