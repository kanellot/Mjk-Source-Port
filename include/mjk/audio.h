/**
 * @file audio.h
 * @brief Sintetizador NES 2A03 optimizado con LUTs y motor de audio NTSC (Ticket R-15).
 *
 * Incluye dos mejoras clave respecto al codigo original:
 *  1. Tablas de consulta precalculadas (`pulse_lut[31]`, `tnd_lut[16][16]`) para el
 *     mezclador no lineal NTSC, eliminando divisiones en coma flotante a 44.1 kHz.
 *  2. Acumulador fraccional de muestras (`44100 / 60.0988139 ≈ 733.7915`) que alterna
 *     entre 733 y 734 muestras/frame para eliminar la deriva y los cortes en SDL2.
 */

#ifndef MJK_ENGINE_AUDIO_H
#define MJK_ENGINE_AUDIO_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk/assets.h"
#include "mjk/types.h"

#define MJK_AUDIO_CPU_CLOCK        1789772.5
#define MJK_AUDIO_CHANNEL_COUNT    8u
#define MJK_AUDIO_CALL_STACK_DEPTH 8u
#define MJK_AUDIO_QUEUE_CAP        16u
#define MJK_AUDIO_MAX_SAMPLES_STEP 768u

/** @brief Registros espejo de una voz del chip 2A03. */
typedef struct {
    uint8_t vol;
    uint8_t sweep;
    uint8_t lo;
    uint8_t hi;
} MjkVoiceRegs;

/** @brief Estado hardware del sintetizador 2A03 y sus tablas LUT. */
typedef struct {
    double   pulse_phase[2];
    uint8_t  pulse_step[2];
    uint8_t  sweep_div[2];
    uint16_t pulse_period[2];
    bool     pulse_sweep_reload[2];

    double   tri_phase;
    uint8_t  tri_step;
    uint8_t  tri_linear_counter;
    bool     tri_linear_reload;

    double   noise_phase;
    uint16_t noise_lfsr;

    float    hp_prev_x;
    float    hp_prev_y;

    /* Tablas LUT precalculadas al inicializar (0 divisiones por muestra) */
    float    pulse_lut[31];
    float    tnd_lut[16][16];
} MjkSynth2A03;

/** @brief Canal virtual del secuenciador (0..3 = BGM, 4..7 = SFX). */
typedef struct {
    bool     active;
    uint8_t  duration_counter;
    uint8_t  duration_reload;
    uint16_t pc;
    uint16_t vol_table_addr;
    uint8_t  vol_offset;
    uint16_t loop_addr;
    uint8_t  stack_sp;
    uint16_t call_stack[MJK_AUDIO_CALL_STACK_DEPTH];
    uint8_t  vol_atten;
    uint8_t  vol_delay;
    uint8_t  vol_reg;
    uint8_t  sweep_reg;
    uint8_t  freq_lo;
    uint8_t  freq_hi;
    uint8_t  loop_count;
    int8_t   pitch_bend_step;
    uint8_t  pitch_bend_acc;
} MjkAudioChannel;

/** @brief Motor de audio completo (secuenciador 8 canales + sintetizador 2A03 + acumulador NTSC). */
typedef struct {
    MjkAudioChannel channels[MJK_AUDIO_CHANNEL_COUNT];
    MjkVoiceRegs    voices[4];
    uint8_t         enable_mask;
    MjkSynth2A03    synth;
    MjkSoundId      queue[MJK_AUDIO_QUEUE_CAP];
    uint8_t         queue_count;
    double          ntsc_sample_accum; /**< Acumulador fraccional 733/734 muestras/frame. */
    uint32_t        last_frame_energy;
} MjkAudioEngine;

/** @brief Inicializa el motor de audio y precalcula las tablas LUT del mezclador 2A03. */
void MjkAudio_Init(MjkAudioEngine* audio);

/** @brief Encola un sonido (SFX o BGM) para reproducirse en el siguiente frame. */
void MjkAudio_Play(MjkAudioEngine* audio, MjkSoundId sound);

/**
 * @brief Calcula cuantas muestras PCM producir en el siguiente frame NTSC (733 o 734)
 * manteniendo el error acumulado <= 1 muestra respecto a 44.100 Hz.
 */
uint32_t MjkAudio_NextFrameSampleCount(MjkAudioEngine* audio);

/** @brief Avanza un frame de musica/SFX y sintetiza `sample_count` muestras PCM 16-bit mono. */
void MjkAudio_StepFrame(MjkAudioEngine* audio,
                        const MjkAssets* assets,
                        int16_t* out_samples,
                        uint32_t sample_count);

/* Helpers del sintetizador (usados por audio_engine.c y test_audio.c) */
void MjkSynth_Init(MjkSynth2A03* s);
void MjkSynth_OnRegWrite(MjkSynth2A03* s, const MjkVoiceRegs voices[4], uint8_t voice_idx, bool reload_hi);
void MjkSynth_ClockQuarterFrame(MjkSynth2A03* s, const MjkVoiceRegs voices[4]);
void MjkSynth_ClockSweep(MjkSynth2A03* s, const MjkVoiceRegs voices[4]);
void MjkSynth_Render(MjkSynth2A03* s, const MjkVoiceRegs voices[4], uint8_t enable_mask,
                     int16_t* out_samples, uint32_t sample_count);

#endif /* MJK_ENGINE_AUDIO_H */
