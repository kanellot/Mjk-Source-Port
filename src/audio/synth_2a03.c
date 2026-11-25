/**
 * @file synth_2a03.c
 * @brief Sintetizador hardware NES 2A03 a 44.1 kHz con tablas LUT de mezcla (Ticket R-15).
 */

#include "mjk/audio.h"
#include <math.h>
#include <string.h>

static const uint8_t k_pulse_duty[4][8] = {
    { 0u, 1u, 0u, 0u, 0u, 0u, 0u, 0u }, /* 12.5% */
    { 0u, 1u, 1u, 0u, 0u, 0u, 0u, 0u }, /* 25.0% */
    { 0u, 1u, 1u, 1u, 1u, 0u, 0u, 0u }, /* 50.0% */
    { 1u, 0u, 0u, 1u, 1u, 1u, 1u, 1u }  /* 75.0% */
};

static const uint8_t k_triangle_seq[32] = {
    15u, 14u, 13u, 12u, 11u, 10u,  9u,  8u,
     7u,  6u,  5u,  4u,  3u,  2u,  1u,  0u,
     0u,  1u,  2u,  3u,  4u,  5u,  6u,  7u,
     8u,  9u, 10u, 11u, 12u, 13u, 14u, 15u
};

static const uint16_t k_noise_periods[16] = {
    4u, 8u, 16u, 32u, 64u, 96u, 128u, 160u,
    202u, 254u, 380u, 508u, 762u, 1016u, 2034u, 4068u
};

void MjkSynth_Init(MjkSynth2A03* s) {
    if (!s) { return; }
    memset(s, 0, sizeof(*s));
    s->noise_lfsr         = 1u;
    s->tri_linear_counter = 0x7Fu;

    /* Precalculo exacto del mezclador no lineal NTSC 2A03 (0 divisiones en tiempo real) */
    s->pulse_lut[0] = 0.0f;
    for (int p = 1; p < 31; ++p) {
        s->pulse_lut[p] = (float)(95.88 / ((8128.0 / (double)p) + 100.0));
    }
    for (int t = 0; t < 16; ++t) {
        for (int n = 0; n < 16; ++n) {
            double denom = ((double)t / 8227.0) + ((double)n / 12241.0);
            s->tnd_lut[t][n] = (denom > 0.0) ? (float)(159.79 / ((1.0 / denom) + 100.0)) : 0.0f;
        }
    }
}

void MjkSynth_OnRegWrite(MjkSynth2A03* s, const MjkVoiceRegs voices[4], uint8_t voice_idx, bool reload_hi) {
    if (!s || !voices || !reload_hi) { return; }
    if (voice_idx < 2u) {
        uint16_t lo = voices[voice_idx].lo;
        uint16_t hi = (uint16_t)(voices[voice_idx].hi & 0x07u);
        s->pulse_period[voice_idx]       = (uint16_t)(lo | (hi << 8));
        s->pulse_step[voice_idx]         = 0u;
        s->pulse_sweep_reload[voice_idx] = true;
    } else if (voice_idx == 2u) {
        s->tri_linear_reload  = true;
        s->tri_linear_counter = (uint8_t)(voices[2].vol & 0x7Fu);
    }
}

void MjkSynth_ClockQuarterFrame(MjkSynth2A03* s, const MjkVoiceRegs voices[4]) {
    if (!s || !voices) { return; }
    uint8_t ctrl = voices[2].vol;
    if (s->tri_linear_reload) {
        s->tri_linear_counter = (uint8_t)(ctrl & 0x7Fu);
    } else if (s->tri_linear_counter > 0u) {
        s->tri_linear_counter--;
    }
    if ((ctrl & 0x80u) == 0u) {
        s->tri_linear_reload = false;
    }
}

void MjkSynth_ClockSweep(MjkSynth2A03* s, const MjkVoiceRegs voices[4]) {
    if (!s || !voices) { return; }
    for (uint8_t ch = 0u; ch < 2u; ++ch) {
        uint8_t sw      = voices[ch].sweep;
        bool enabled    = (sw & 0x80u) != 0u;
        uint8_t div_per = (uint8_t)((sw >> 4) & 0x07u);
        bool negate     = (sw & 0x08u) != 0u;
        uint8_t shift   = (uint8_t)(sw & 0x07u);
        uint16_t cur    = s->pulse_period[ch];

        if (s->sweep_div[ch] == 0u && enabled && shift > 0u && cur >= 8u) {
            uint16_t delta = (uint16_t)(cur >> shift);
            int32_t target = negate
                ? ((int32_t)cur - (int32_t)delta - ((ch == 0u) ? 1 : 0))
                : ((int32_t)cur + (int32_t)delta);
            if (target >= 8 && target <= 0x7FF) {
                s->pulse_period[ch] = (uint16_t)target;
            }
        }
        if (s->sweep_div[ch] == 0u || s->pulse_sweep_reload[ch]) {
            s->sweep_div[ch]          = div_per;
            s->pulse_sweep_reload[ch] = false;
        } else {
            s->sweep_div[ch]--;
        }
    }
}

static uint8_t step_pulse(MjkSynth2A03* s, const MjkVoiceRegs voices[4], uint8_t en,
                          uint8_t ch, double cps) {
    if ((en & (1u << ch)) == 0u) { return 0u; }
    uint16_t period = s->pulse_period[ch];
    if (period < 8u || period > 0x7FFu) { return 0u; }

    uint8_t sw = voices[ch].sweep;
    if ((sw & 0x80u) != 0u && (sw & 0x08u) == 0u && (sw & 0x07u) > 0u) {
        if ((uint16_t)(period + (period >> (sw & 0x07u))) > 0x7FFu) { return 0u; }
    }

    double step_c = 2.0 * (double)(period + 1u);
    s->pulse_phase[ch] += cps;
    while (s->pulse_phase[ch] >= step_c) {
        s->pulse_phase[ch] -= step_c;
        s->pulse_step[ch] = (uint8_t)((s->pulse_step[ch] + 1u) & 7u);
    }

    uint8_t duty = (uint8_t)((voices[ch].vol >> 6) & 3u);
    return k_pulse_duty[duty][s->pulse_step[ch]] ? (uint8_t)(voices[ch].vol & 0x0Fu) : 0u;
}

static uint8_t step_triangle(MjkSynth2A03* s, const MjkVoiceRegs voices[4], uint8_t en, double cps) {
    if ((en & 0x04u) == 0u || (voices[2].vol & 0x7Fu) == 0u || s->tri_linear_counter == 0u) {
        return 0u;
    }
    uint16_t period = (uint16_t)(voices[2].lo | ((uint16_t)(voices[2].hi & 0x07u) << 8));
    if (period < 2u) { return 0u; }

    double step_c = (double)(period + 1u);
    s->tri_phase += cps;
    while (s->tri_phase >= step_c) {
        s->tri_phase -= step_c;
        s->tri_step = (uint8_t)((s->tri_step + 1u) & 31u);
    }
    return k_triangle_seq[s->tri_step];
}

static uint8_t step_noise(MjkSynth2A03* s, const MjkVoiceRegs voices[4], uint8_t en, double cps) {
    double step_c   = (double)k_noise_periods[voices[3].lo & 0x0Fu];
    bool short_mode = (voices[3].lo & 0x80u) != 0u;

    s->noise_phase += cps;
    while (s->noise_phase >= step_c) {
        s->noise_phase -= step_c;
        uint16_t bit0 = s->noise_lfsr & 1u;
        uint16_t tap  = short_mode ? ((s->noise_lfsr >> 6) & 1u) : ((s->noise_lfsr >> 1) & 1u);
        s->noise_lfsr = (uint16_t)((s->noise_lfsr >> 1) | ((uint16_t)(bit0 ^ tap) << 14));
    }
    if ((en & 0x08u) == 0u || (s->noise_lfsr & 1u) != 0u) { return 0u; }
    return (uint8_t)(voices[3].vol & 0x0Fu);
}

void MjkSynth_Render(MjkSynth2A03* s, const MjkVoiceRegs voices[4], uint8_t enable_mask,
                     int16_t* out_samples, uint32_t sample_count) {
    if (!out_samples || sample_count == 0u) { return; }
    if (!s || !voices) {
        memset(out_samples, 0, (size_t)sample_count * sizeof(int16_t));
        return;
    }

    const double cps = MJK_AUDIO_CPU_CLOCK / (double)MJK_AUDIO_SAMPLE_RATE;
    for (uint32_t i = 0u; i < sample_count; ++i) {
        uint8_t p1 = step_pulse(s, voices, enable_mask, 0u, cps);
        uint8_t p2 = step_pulse(s, voices, enable_mask, 1u, cps);
        uint8_t tr = step_triangle(s, voices, enable_mask, cps);
        uint8_t ns = step_noise(s, voices, enable_mask, cps);

        float x = (s->pulse_lut[p1 + p2] + s->tnd_lut[tr][ns]) * 28000.0f;
        float y = x - s->hp_prev_x + (0.996f * s->hp_prev_y);
        s->hp_prev_x = x;
        s->hp_prev_y = y;

        if (!isfinite(y))      { y = 0.0f; }
        else if (y > 32767.0f) { y = 32767.0f; }
        else if (y < -32767.0f){ y = -32767.0f; }
        out_samples[i] = (int16_t)y;
    }
}
