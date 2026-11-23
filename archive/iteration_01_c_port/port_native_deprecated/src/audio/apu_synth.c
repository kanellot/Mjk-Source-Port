/**
 * @file apu_synth.c
 * @brief 2A03 APU Software Synthesizer (Pulse 1, Pulse 2, Triangle, Noise)
 * Approximates 2A03 hardware with non-linear DAC mixing curves.
 */

#include "mbj/audio.h"
#include <string.h>

/* NES Pulse duty cycles: 8 steps per waveform */
static const uint8_t kDutyCycles[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 }, /* 12.5% */
    { 0, 1, 1, 0, 0, 0, 0, 0 }, /* 25.0% */
    { 0, 1, 1, 1, 1, 0, 0, 0 }, /* 50.0% */
    { 1, 0, 0, 1, 1, 1, 1, 1 }  /* 75.0% */
};

/* Triangle channel 32-step waveform */
static const uint8_t kTriangleWave[32] = {
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

/* NES APU CPU clock frequency (NTSC): 1.789773 MHz */
#define NES_CPU_FREQ 1789773.0

void APU_Init(APU_SynthState *apu) {
    if (!apu) return;
    memset(apu, 0, sizeof(APU_SynthState));

    apu->pulse[0].duty = 2; /* 50% default */
    apu->pulse[1].duty = 2;
    apu->noise.shift_reg = 1;
}

void APU_GenerateSamples(APU_SynthState *apu, float *out_buffer, int num_samples, int sample_rate) {
    if (!apu || !out_buffer || num_samples <= 0 || sample_rate <= 0) return;

    /* Number of NES APU CPU clock cycles elapsed per output audio sample */
    float apu_ticks_per_sample = (float)(NES_CPU_FREQ / (double)sample_rate);

    for (int s = 0; s < num_samples; ++s) {
        /* Advance Pulse 1: 8 steps per waveform, each step = 2 * (period + 1) CPU cycles */
        int p0 = 0;
        if (apu->pulse[0].enabled && apu->pulse[0].period >= 8) {
            apu->pulse[0].timer += apu_ticks_per_sample;
            float step_period = (float)((apu->pulse[0].period + 1) * 2);
            if (step_period > 0.0f) {
                while (apu->pulse[0].timer >= step_period) {
                    apu->pulse[0].timer -= step_period;
                    apu->pulse[0].step = (uint8_t)((apu->pulse[0].step + 1) & 7);
                }
                if (kDutyCycles[apu->pulse[0].duty & 3][apu->pulse[0].step]) {
                    p0 = apu->pulse[0].volume;
                }
            }
        }

        /* Advance Pulse 2: 8 steps per waveform, each step = 2 * (period + 1) CPU cycles */
        int p1 = 0;
        if (apu->pulse[1].enabled && apu->pulse[1].period >= 8) {
            apu->pulse[1].timer += apu_ticks_per_sample;
            float step_period = (float)((apu->pulse[1].period + 1) * 2);
            if (step_period > 0.0f) {
                while (apu->pulse[1].timer >= step_period) {
                    apu->pulse[1].timer -= step_period;
                    apu->pulse[1].step = (uint8_t)((apu->pulse[1].step + 1) & 7);
                }
                if (kDutyCycles[apu->pulse[1].duty & 3][apu->pulse[1].step]) {
                    p1 = apu->pulse[1].volume;
                }
            }
        }

        /* Advance Triangle: 32 steps per waveform, each step = (period + 1) CPU cycles */
        int t = 0;
        if (apu->triangle.enabled && apu->triangle.period > 0) {
            apu->triangle.timer += apu_ticks_per_sample;
            float step_period = (float)(apu->triangle.period + 1);
            if (step_period > 0.0f) {
                while (apu->triangle.timer >= step_period) {
                    apu->triangle.timer -= step_period;
                    apu->triangle.step = (uint8_t)((apu->triangle.step + 1) & 31);
                }
                t = kTriangleWave[apu->triangle.step];
            }
        }

        /* Advance Noise: 15-bit LFSR clocked by noise period */
        int n = 0;
        if (apu->noise.enabled && apu->noise.period > 0) {
            apu->noise.timer += apu_ticks_per_sample;
            float step_period = (float)(apu->noise.period * 2);
            while (apu->noise.timer >= step_period) {
                apu->noise.timer -= step_period;
                uint16_t feedback = (apu->noise.shift_reg & 1) ^
                    (apu->noise.mode ? ((apu->noise.shift_reg >> 6) & 1) : ((apu->noise.shift_reg >> 1) & 1));
                apu->noise.shift_reg = (uint16_t)((apu->noise.shift_reg >> 1) | (feedback << 14));
            }
            if (!(apu->noise.shift_reg & 1)) {
                n = apu->noise.volume;
            }
        }

        /* Blargg non-linear DAC mix approximation */
        float pulse_out = 0.0f;
        if (p0 + p1 > 0) {
            pulse_out = 95.88f / ((8128.0f / (float)(p0 + p1)) + 100.0f);
        }

        float tnd_out = 0.0f;
        float t_val = (float)t / 8227.0f;
        float n_val = (float)n / 12241.0f;
        if (t_val + n_val > 0.0f) {
            tnd_out = 159.79f / ((1.0f / (t_val + n_val)) + 100.0f);
        }

        float sample = (pulse_out + tnd_out) * 1.8f;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        out_buffer[s] = sample;
    }
}
