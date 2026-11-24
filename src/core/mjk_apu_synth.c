/**
 * @file mjk_apu_synth.c
 * @brief Sintetizador hardware NES 2A03 a 44.1 kHz (Pulse 1/2, Triangle, Noise).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Emula las 4 voces del chip 2A03 (NTSC 1.7897725 MHz) sin depender de SDL ni del SO:
 * - Pulse 1/2: 4 duty cycles (12.5%, 25%, 50%, 75%) y hardware sweep ($4001/$4005).
 * - Triangle: secuenciador fijo de 32 pasos (silencio si period < 2).
 * - Noise: LFSR de 15 bits (modo normal y corto) con tabla NTSC de 16 periodos.
 * - Mezclador no lineal NTSC + filtro pasa-altos DC para evitar offset y clipping.
 */

#include "mjk_apu.h"
#include <math.h>
#include <string.h>

static const uint8_t k_pulse_duty_table[4][8] = {
    { 0u, 1u, 0u, 0u, 0u, 0u, 0u, 0u }, /* 12.5% */
    { 0u, 1u, 1u, 0u, 0u, 0u, 0u, 0u }, /* 25.0% */
    { 0u, 1u, 1u, 1u, 1u, 0u, 0u, 0u }, /* 50.0% */
    { 1u, 0u, 0u, 1u, 1u, 1u, 1u, 1u }  /* 75.0% (25% invertido) */
};

static const uint8_t k_triangle_seq[32] = {
    15u, 14u, 13u, 12u, 11u, 10u,  9u,  8u,
     7u,  6u,  5u,  4u,  3u,  2u,  1u,  0u,
     0u,  1u,  2u,  3u,  4u,  5u,  6u,  7u,
     8u,  9u, 10u, 11u, 12u, 13u, 14u, 15u
};

static const uint16_t k_noise_periods_ntsc[16] = {
    4u, 8u, 16u, 32u, 64u, 96u, 128u, 160u,
    202u, 254u, 380u, 508u, 762u, 1016u, 2034u, 4068u
};

void MjkApuSynth_Init(MjkApuSynth* synth) {
    if (!synth) {
        return;
    }
    memset(synth, 0, sizeof(*synth));
    synth->noise_lfsr = 1u;
    synth->tri_linear_counter = 0x7Fu;
}

void MjkApuSynth_OnRegWrite(MjkApuSynth* synth,
                            const MjkApuRegs* regs,
                            uint8_t voice_idx,
                            bool reload_hi) {
    if (!synth || !regs) {
        return;
    }
    if (voice_idx < 2u && reload_hi) {
        uint16_t lo = regs->voice[voice_idx].lo;
        uint16_t hi = (uint16_t)(regs->voice[voice_idx].hi & 0x07u);
        synth->pulse_period[voice_idx] = (uint16_t)(lo | (hi << 8));
        synth->pulse_step[voice_idx] = 0u;
        synth->pulse_sweep_reload[voice_idx] = true;
    } else if (voice_idx == 2u && reload_hi) {
        synth->tri_linear_reload = true;
        synth->tri_linear_counter = (uint8_t)(regs->voice[2].vol & 0x7Fu);
    }
}

void MjkApuSynth_ClockQuarterFrame(MjkApuSynth* synth, const MjkApuRegs* regs) {
    if (!synth || !regs) {
        return;
    }
    uint8_t ctrl = regs->voice[2].vol;
    if (synth->tri_linear_reload) {
        synth->tri_linear_counter = (uint8_t)(ctrl & 0x7Fu);
    } else if (synth->tri_linear_counter > 0u) {
        synth->tri_linear_counter--;
    }
    if ((ctrl & 0x80u) == 0u) {
        synth->tri_linear_reload = false;
    }
}

void MjkApuSynth_ClockSweep(MjkApuSynth* synth, const MjkApuRegs* regs) {
    if (!synth || !regs) {
        return;
    }
    for (uint8_t ch = 0u; ch < 2u; ++ch) {
        uint8_t sw = regs->voice[ch].sweep;
        bool enabled = (sw & 0x80u) != 0u;
        uint8_t div_period = (uint8_t)((sw >> 4) & 0x07u);
        bool negate = (sw & 0x08u) != 0u;
        uint8_t shift = (uint8_t)(sw & 0x07u);
        uint16_t cur = synth->pulse_period[ch];

        if (synth->sweep_div[ch] == 0u && enabled && shift > 0u && cur >= 8u) {
            uint16_t delta = (uint16_t)(cur >> shift);
            int32_t target = negate
                ? ((int32_t)cur - (int32_t)delta - ((ch == 0u) ? 1 : 0))
                : ((int32_t)cur + (int32_t)delta);
            if (target >= 8 && target <= 0x7FF) {
                synth->pulse_period[ch] = (uint16_t)target;
            }
        }

        if (synth->sweep_div[ch] == 0u || synth->pulse_sweep_reload[ch]) {
            synth->sweep_div[ch] = div_period;
            synth->pulse_sweep_reload[ch] = false;
        } else {
            synth->sweep_div[ch]--;
        }
    }
}

static double step_pulse_voice(MjkApuSynth* synth,
                               const MjkApuRegs* regs,
                               uint8_t ch,
                               double cycles_per_sample) {
    if ((regs->snd_chn & (1u << ch)) == 0u) {
        return 0.0;
    }
    uint16_t period = synth->pulse_period[ch];
    if (period < 8u || period > 0x7FFu) {
        return 0.0;
    }

    uint8_t sw = regs->voice[ch].sweep;
    if ((sw & 0x80u) != 0u && (sw & 0x08u) == 0u && (sw & 0x07u) > 0u) {
        uint16_t target = (uint16_t)(period + (period >> (sw & 0x07u)));
        if (target > 0x7FFu) {
            return 0.0;
        }
    }

    double step_cycles = 2.0 * (double)(period + 1u);
    synth->pulse_phase[ch] += cycles_per_sample;
    while (synth->pulse_phase[ch] >= step_cycles) {
        synth->pulse_phase[ch] -= step_cycles;
        synth->pulse_step[ch] = (uint8_t)((synth->pulse_step[ch] + 1u) & 7u);
    }

    uint8_t duty = (uint8_t)((regs->voice[ch].vol >> 6) & 0x03u);
    uint8_t vol = (uint8_t)(regs->voice[ch].vol & 0x0Fu);
    return k_pulse_duty_table[duty][synth->pulse_step[ch]] ? (double)vol : 0.0;
}

static double step_triangle_voice(MjkApuSynth* synth,
                                  const MjkApuRegs* regs,
                                  double cycles_per_sample) {
    if ((regs->snd_chn & 0x04u) == 0u ||
        (regs->voice[2].vol & 0x7Fu) == 0u ||
        synth->tri_linear_counter == 0u) {
        return 0.0;
    }
    uint16_t period = (uint16_t)(regs->voice[2].lo | ((uint16_t)(regs->voice[2].hi & 0x07u) << 8));
    if (period < 2u) {
        return 0.0;
    }

    double step_cycles = (double)(period + 1u);
    synth->tri_phase += cycles_per_sample;
    while (synth->tri_phase >= step_cycles) {
        synth->tri_phase -= step_cycles;
        synth->tri_step = (uint8_t)((synth->tri_step + 1u) & 31u);
    }

    return (double)k_triangle_seq[synth->tri_step];
}

static double step_noise_voice(MjkApuSynth* synth,
                               const MjkApuRegs* regs,
                               double cycles_per_sample) {
    uint16_t period = k_noise_periods_ntsc[regs->voice[3].lo & 0x0Fu];
    bool short_mode = (regs->voice[3].lo & 0x80u) != 0u;
    double step_cycles = (double)period;

    synth->noise_phase += cycles_per_sample;
    while (synth->noise_phase >= step_cycles) {
        synth->noise_phase -= step_cycles;
        uint16_t bit0 = synth->noise_lfsr & 1u;
        uint16_t tap = short_mode
            ? ((synth->noise_lfsr >> 6) & 1u)
            : ((synth->noise_lfsr >> 1) & 1u);
        uint16_t fb = (uint16_t)(bit0 ^ tap);
        synth->noise_lfsr = (uint16_t)((synth->noise_lfsr >> 1) | (fb << 14));
    }

    if ((regs->snd_chn & 0x08u) == 0u || (synth->noise_lfsr & 1u) != 0u) {
        return 0.0;
    }
    return (double)(regs->voice[3].vol & 0x0Fu);
}

void MjkApuSynth_RenderSamples(MjkApuSynth* synth,
                               const MjkApuRegs* regs,
                               int16_t* out_samples,
                               uint32_t sample_count) {
    if (!out_samples || sample_count == 0u) {
        return;
    }
    if (!synth || !regs) {
        memset(out_samples, 0, (size_t)sample_count * sizeof(int16_t));
        return;
    }

    const double cycles_per_sample = MJK_APU_CPU_CLOCK / (double)MJK_APU_SAMPLE_RATE;

    for (uint32_t i = 0u; i < sample_count; ++i) {
        double p1 = step_pulse_voice(synth, regs, 0u, cycles_per_sample);
        double p2 = step_pulse_voice(synth, regs, 1u, cycles_per_sample);
        double tri = step_triangle_voice(synth, regs, cycles_per_sample);
        double noise = step_noise_voice(synth, regs, cycles_per_sample);

        double p_sum = p1 + p2;
        double pulse_out = (p_sum > 0.0)
            ? (95.88 / ((8128.0 / p_sum) + 100.0))
            : 0.0;

        double tnd_denom = (tri / 8227.0) + (noise / 12241.0);
        double tnd_out = (tnd_denom > 0.0)
            ? (159.79 / ((1.0 / tnd_denom) + 100.0))
            : 0.0;

        double x = (pulse_out + tnd_out) * 28000.0;
        double y = x - synth->hp_prev_x + (0.996 * synth->hp_prev_y);
        synth->hp_prev_x = x;
        synth->hp_prev_y = y;

        if (!isfinite(y)) {
            y = 0.0;
        } else if (y > 32767.0) {
            y = 32767.0;
        } else if (y < -32767.0) {
            y = -32767.0;
        }
        out_samples[i] = (int16_t)y;
    }
}
