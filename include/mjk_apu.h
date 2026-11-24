/**
 * @file mjk_apu.h
 * @brief Sintetizador hardware 2A03 (44.1 kHz) e intérprete bytecode de 8 canales.
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Separa la síntesis física del chip NES 2A03 (`mjk_apu_synth.c`: Pulse 1/2 con
 * sweep, Triangle de 32 pasos, Noise LFSR de 15 bits y mezclador no lineal NTSC)
 * del secuenciador bytecode de 8 canales (`mjk_apu.c`: 4 BGM + 4 SFX con prioridad
 * y restauración limpia al terminar SFX, basado en `asm/src/music-engine.asm`).
 */

#ifndef MJK_APU_H
#define MJK_APU_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_types.h"

#define MJK_APU_CPU_CLOCK         1789772.5
#define MJK_APU_SAMPLE_RATE       44100u
#define MJK_APU_SAMPLES_PER_FRAME 735u
#define MJK_APU_CHANNEL_COUNT     8u
#define MJK_APU_CALL_STACK_DEPTH  8u
#define MJK_APU_QUEUE_CAP         16u
#define MJK_APU_MAX_OPS_PER_FRAME 64u

/** @brief Registros espejo de una voz hardware 2A03 (`$4000..$400F`). */
typedef struct {
    uint8_t vol;
    uint8_t sweep;
    uint8_t lo;
    uint8_t hi;
} MjkApuVoiceReg;

/** @brief Banco completo de registros hardware 2A03 usados por el motor. */
typedef struct {
    MjkApuVoiceReg voice[4]; /* 0=Pulse1, 1=Pulse2, 2=Triangle, 3=Noise */
    uint8_t        snd_chn;  /* $4015 bits 0..3: P1/P2/Tri/Noise enable */
} MjkApuRegs;

/** @brief Estado de osciladores, sweep, LFSR y filtro DC del sintetizador 2A03. */
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

    double   hp_prev_x;
    double   hp_prev_y;
} MjkApuSynth;

/** @brief Estado de un canal virtual del motor bytecode (`channels[0..3]=BGM, [4..7]=SFX`). */
typedef struct {
    uint8_t  active;
    uint8_t  duration_counter;
    uint8_t  duration_reload;
    uint16_t pc;
    uint16_t vol_table_addr;
    uint8_t  vol_offset;
    uint16_t loop_addr;
    uint8_t  stack_sp;
    uint16_t call_stack[MJK_APU_CALL_STACK_DEPTH];
    uint8_t  vol_atten;
    uint8_t  vol_delay;
    uint8_t  vol_reg;
    uint8_t  sweep_reg;
    uint8_t  freq_lo;
    uint8_t  freq_hi;
    uint8_t  loop_count;
    int8_t   pitch_bend_step;
    uint8_t  pitch_bend_acc;
} MjkApuChannel;

/** @brief Estado completo de la APU (secuenciador 8 canales + sintetizador 2A03). */
typedef struct {
    MjkApuChannel channels[MJK_APU_CHANNEL_COUNT];
    MjkApuRegs    regs;
    MjkApuSynth   synth;
    MjkSoundId    queue[MJK_APU_QUEUE_CAP];
    uint8_t       queue_count;
    uint32_t      last_frame_energy;
} MjkApu;

MJK_CORE_API void MjkApuSynth_Init(MjkApuSynth* synth);
MJK_CORE_API void MjkApuSynth_OnRegWrite(MjkApuSynth* synth,
                                         const MjkApuRegs* regs,
                                         uint8_t voice_idx,
                                         bool reload_hi);
MJK_CORE_API void MjkApuSynth_ClockQuarterFrame(MjkApuSynth* synth, const MjkApuRegs* regs);
MJK_CORE_API void MjkApuSynth_ClockSweep(MjkApuSynth* synth, const MjkApuRegs* regs);
MJK_CORE_API void MjkApuSynth_RenderSamples(MjkApuSynth* synth,
                                            const MjkApuRegs* regs,
                                            int16_t* out_samples,
                                            uint32_t sample_count);

MJK_CORE_API void MjkApu_Init(MjkApu* apu);
MJK_CORE_API void MjkApu_QueueSound(MjkApu* apu, MjkSoundId id);
MJK_CORE_API void MjkApu_StepFrame(MjkApu* apu,
                                   const MjkAssetPackage* pkg,
                                   int16_t* out_samples,
                                   uint32_t sample_count);

#endif /* MJK_APU_H */
