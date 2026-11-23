/**
 * @file audio.h
 * @brief 2A03 APU Software Synthesizer, Sound Driver, and Sound Data for Mighty Bomb Jack.
 * Bit-exact C11 implementation matching NES PRG $B100-$B592, music-engine.asm, and RES-010.
 */

#ifndef MBJ_AUDIO_H
#define MBJ_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sound IDs matching RES-010 and SoundPointers in music-data.asm */
typedef enum {
    SOUND_NONE                  = 0x00,
    SOUND_JUMP                  = 0x01,
    SOUND_DEATH                 = 0x02,
    SOUND_BONUS_COIN_SPAWN      = 0x03,
    SOUND_COIN_COLLECTED        = 0x04,
    SOUND_POWER_COIN_SPAWN      = 0x05,
    SOUND_POWER_COIN_MUSIC      = 0x06,
    SOUND_BOMB_COLLECTED_UNLIT  = 0x07,
    SOUND_BOMB_COLLECTED_LIT    = 0x08,
    SOUND_YOU_ARE_GREEDY        = 0x09,
    SOUND_1UP_CAPTIVE           = 0x0A,
    SOUND_CRYSTAL_BALL          = 0x0B,
    SOUND_BONUS_STATUE          = 0x0D,
    SOUND_SECRET_BLOCK          = 0x0E,
    SOUND_DOOR                  = 0x0F,
    MUSIC_MAIN                  = 0x10,
    MUSIC_TREASURE_ROOM         = 0x11,
    MUSIC_SIDE_ROOM             = 0x12,
    MUSIC_LABYRINTH             = 0x13,
    MUSIC_OUTSIDE1              = 0x15,
    MUSIC_MAIN_WITH_INTRO       = 0x16,
    MUSIC_GAME_OVER             = 0x17,
    MUSIC_ROUND_CLEAR           = 0x18,
    SOUND_BOMB_CHEST_BONUS      = 0x19,
    SOUND_PAUSE                 = 0x1B,
    SOUND_UNPAUSE               = 0x1C,
    SOUND_USED_MIGHTY_COIN      = 0x1D,
    MUSIC_OUTSIDE2              = 0x1E,
    MUSIC_TORTURE_ROOM          = 0x1F,
    SOUND_BOMB_BONUS            = 0x20,
    SOUND_TIME_PICKUP           = 0x21
} MBJ_SoundID;

/* APU Channel Synthesis State */
typedef struct {
    /* Pulse 1 & Pulse 2 */
    struct {
        uint16_t period;
        float    timer;
        uint8_t  duty;       /* 0=12.5%, 1=25%, 2=50%, 3=75% */
        uint8_t  step;       /* 0..7 */
        uint8_t  volume;     /* 0..15 */
        bool     enabled;
    } pulse[2];

    /* Triangle Channel */
    struct {
        uint16_t period;
        float    timer;
        uint8_t  step;       /* 0..31 */
        bool     enabled;
    } triangle;

    /* Noise Channel (15-bit LFSR) */
    struct {
        uint16_t period;
        float    timer;
        uint16_t shift_reg;  /* 15-bit LFSR */
        uint8_t  volume;     /* 0..15 */
        bool     mode;       /* false=15-bit, true=6-bit */
        bool     enabled;
    } noise;
} APU_SynthState;

/**
 * @brief Initialize the APU software synthesizer.
 */
void APU_Init(APU_SynthState *apu);

/**
 * @brief Generates audio PCM samples into a float buffer (single-channel or interleaved stereo).
 * Uses Blargg's non-linear DAC mix approximation.
 */
void APU_GenerateSamples(APU_SynthState *apu, float *out_buffer, int num_samples, int sample_rate);

typedef struct MBJ_RomData MBJ_RomData;

/**
 * @brief Initializes the sound driver and loads music bytecode tables.
 */
void SoundDriver_Init(void);

/**
 * @brief Initializes the sound driver using extracted ROM bytecode and tables.
 */
void SoundDriver_InitWithROM(const MBJ_RomData *rom);

/**
 * @brief Advances the sound engine by 1 tick (called at 60 Hz).
 */
void SoundDriver_Update60Hz(APU_SynthState *apu);

/**
 * @brief Queues a sound effect or changes background music.
 */
void SoundDriver_Play(MBJ_SoundID sound_id);

/**
 * @brief Stops all active audio channels.
 */
void SoundDriver_StopAll(APU_SynthState *apu);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_AUDIO_H */
