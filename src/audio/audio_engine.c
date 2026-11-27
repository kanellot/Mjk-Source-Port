/**
 * @file audio_engine.c
 * @brief Secuenciador de 8 canales (4 BGM + 4 SFX) y acumulador NTSC (Ticket R-15).
 */

#include "mjk/audio.h"
#include <string.h>

enum {
    AUDIO_NOTE_LEN_ADDR = 0xF39Eu,
    AUDIO_VOL_TBL_ADDR  = 0xF3BEu,
    AUDIO_MAX_OPS_FRAME = 64u
};

static const uint8_t* resolve_audio_ptr(const MjkAudioBank* bank, uint16_t cpu_addr, size_t min_bytes) {
    if (cpu_addr >= MJK_CPU_NOTE_FREQ_ADDR &&
        (size_t)(cpu_addr - MJK_CPU_NOTE_FREQ_ADDR) + min_bytes <= MJK_NOTE_FREQ_SIZE) {
        return bank->note_freq_table + (cpu_addr - MJK_CPU_NOTE_FREQ_ADDR);
    }
    if (cpu_addr >= MJK_CPU_MUSIC_DATA_ADDR &&
        (size_t)(cpu_addr - MJK_CPU_MUSIC_DATA_ADDR) + min_bytes <= MJK_MUSIC_DATA_SIZE) {
        return bank->music_data + (cpu_addr - MJK_CPU_MUSIC_DATA_ADDR);
    }
    return NULL;
}

static bool read_audio_u16(const MjkAudioBank* bank, uint16_t cpu_addr, uint16_t* out_val) {
    const uint8_t* p = resolve_audio_ptr(bank, cpu_addr, 2u);
    if (!p) { return false; }
    *out_val = (uint16_t)(((uint16_t)p[0]) | ((uint16_t)p[1] << 8));
    return true;
}

static bool is_bgm_inhibited(const MjkAudioEngine* a, uint8_t ch_idx) {
    return (ch_idx < 4u) && a->channels[ch_idx | 4u].active;
}

static void apply_hw_freq_regs(MjkAudioEngine* a, uint8_t ch_idx) {
    if (is_bgm_inhibited(a, ch_idx)) { return; }
    uint8_t r = (uint8_t)(ch_idx & 3u);
    const MjkAudioChannel* ch = &a->channels[ch_idx];
    a->voices[r].vol  |= 0x20u;
    a->voices[r].sweep = ch->sweep_reg;
    a->voices[r].lo    = ch->freq_lo;
    a->voices[r].hi    = ch->freq_hi;
}

static void reset_vol_envelope(MjkAudioEngine* a, const MjkAudioBank* bank, uint8_t ch_idx) {
    MjkAudioChannel* ch = &a->channels[ch_idx];
    if (ch->vol_table_addr == 0u) { return; }
    const uint8_t* e = resolve_audio_ptr(bank, (uint16_t)(ch->vol_table_addr + ch->vol_offset), 2u);
    if (!e) { return; }
    ch->vol_offset = (uint8_t)(ch->vol_offset + 2u);
    ch->vol_delay  = e[1];

    uint8_t combined = (uint8_t)(e[0] | (ch->vol_reg & 0xC0u));
    if ((combined & 0x10u) != 0u) {
        int vol = (int)(combined & 0x0Fu) - (int)ch->vol_atten;
        if (vol < 0) { vol = 0; }
        combined = (uint8_t)((combined & 0xD0u) | ((uint8_t)vol & 0x0Fu));
    }
    ch->vol_reg = combined;
    if (!is_bgm_inhibited(a, ch_idx)) {
        a->voices[ch_idx & 3u].vol = ch->vol_reg;
    }
}

static void drain_queue(MjkAudioEngine* a, const MjkAudioBank* bank) {
    for (uint8_t q = 0u; q < a->queue_count; ++q) {
        MjkSoundId id = a->queue[q];
        if (id == SOUND_SILENCE) {
            for (uint8_t i = 0u; i < MJK_AUDIO_CHANNEL_COUNT; ++i) { a->channels[i].active = false; }
            a->enable_mask = 0u;
            continue;
        }
        if ((unsigned)id >= (unsigned)SOUND_COUNT || bank->sound_cpu_addrs[id] == 0u) { continue; }
        uint16_t hdr = bank->sound_cpu_addrs[id];
        for (uint16_t off = 0u; off < 64u; off = (uint16_t)(off + 3u)) {
            const uint8_t* p_ch = resolve_audio_ptr(bank, (uint16_t)(hdr + off), 1u);
            if (!p_ch || *p_ch == 0xFFu) { break; }
            uint16_t target = 0u;
            if (!read_audio_u16(bank, (uint16_t)(hdr + off + 1u), &target)) { break; }
            if (*p_ch < MJK_AUDIO_CHANNEL_COUNT) {
                MjkAudioChannel* ch = &a->channels[*p_ch];
                memset(ch, 0, sizeof(*ch));
                ch->active           = true;
                ch->duration_counter = 1u;
                ch->pc               = target;
            }
        }
    }
    a->queue_count = 0u;
}

static bool exec_fx(MjkAudioEngine* a, const MjkAudioBank* bank, uint8_t ch_idx, uint8_t cmd) {
    MjkAudioChannel* ch = &a->channels[ch_idx];
    switch (cmd & 7u) {
        case 0u: { /* JMP */
            uint16_t t = 0u;
            if (!read_audio_u16(bank, ch->pc, &t)) { ch->active = false; return true; }
            ch->pc = t; return false;
        }
        case 1u: { /* Loop Start */
            const uint8_t* p = resolve_audio_ptr(bank, ch->pc, 1u);
            if (!p) { ch->active = false; return true; }
            ch->loop_count = *p; ch->pc++; ch->loop_addr = ch->pc; return false;
        }
        case 2u: /* Loop End */
            if (ch->loop_count > 1u) { ch->loop_count--; ch->pc = ch->loop_addr; }
            else { ch->loop_count = 0u; }
            return false;
        case 3u: { /* JSR */
            uint16_t t = 0u;
            if (!read_audio_u16(bank, ch->pc, &t)) { ch->active = false; return true; }
            if (ch->stack_sp < MJK_AUDIO_CALL_STACK_DEPTH) {
                ch->call_stack[ch->stack_sp++] = (uint16_t)(ch->pc + 2u);
            }
            ch->pc = t; return false;
        }
        case 4u: /* RTS */
            if (ch->stack_sp > 0u) { ch->pc = ch->call_stack[--ch->stack_sp]; return false; }
            ch->active = false; return true;
        case 5u: { /* Hardware Sweep */
            const uint8_t* p = resolve_audio_ptr(bank, ch->pc, 1u);
            if (!p) { ch->active = false; return true; }
            ch->sweep_reg = *p; ch->pc++; return false;
        }
        default: /* Stop (+ restaurar canal BGM hermano si era SFX) */
            ch->active = false;
            if ((ch_idx & 4u) != 0u) {
                uint8_t bgm = (uint8_t)(ch_idx & 3u);
                apply_hw_freq_regs(a, bgm);
                a->voices[bgm].vol = (uint8_t)(a->channels[bgm].vol_reg | 0x20u);
            }
            return true;
    }
}

static bool exec_cmd(MjkAudioEngine* a, const MjkAudioBank* bank, uint8_t ch_idx, uint8_t cmd) {
    MjkAudioChannel* ch = &a->channels[ch_idx];
    switch ((cmd >> 4) & 7u) {
        case 0u: case 1u: { /* Note Length */
            const uint8_t* p = resolve_audio_ptr(bank, (uint16_t)(AUDIO_NOTE_LEN_ADDR + (cmd & 0x1Fu)), 1u);
            if (p) { ch->duration_counter = *p; ch->duration_reload = *p; }
            return false;
        }
        case 2u: case 3u: { /* Volume Envelope Table */
            uint16_t addr = 0u;
            if (read_audio_u16(bank, (uint16_t)(AUDIO_VOL_TBL_ADDR + (uint16_t)(cmd & 0x1Fu) * 2u), &addr)) {
                ch->vol_table_addr = addr;
                ch->vol_offset     = 0u;
                reset_vol_envelope(a, bank, ch_idx);
            }
            return false;
        }
        case 4u: /* Direct Noise/Freq */
            ch->freq_lo = (uint8_t)(cmd & 0x0Fu);
            ch->freq_hi = 0x08u;
            apply_hw_freq_regs(a, ch_idx);
            return true;
        case 5u: /* Duty / Pitch Bend */
            if ((cmd & 0x0Fu) < 8u) {
                uint8_t v = (uint8_t)(cmd & 0x0Fu);
                uint8_t c0 = (uint8_t)(v & 1u);
                v = (uint8_t)((v >> 2) | (c0 << 7));
                ch->vol_reg = (uint8_t)((ch->vol_reg & 0x3Fu) | v);
            } else {
                static const int8_t k_bend[4] = { 1, 2, -1, -2 };
                int8_t st = k_bend[cmd & 3u];
                if (st != ch->pitch_bend_step) { ch->pitch_bend_step = st; ch->pitch_bend_acc = 0u; }
            }
            return false;
        case 6u: /* Volume Attenuation */
            ch->vol_atten = (uint8_t)(cmd & 0x0Fu);
            return false;
        default:
            return exec_fx(a, bank, ch_idx, cmd);
    }
}

static void step_channel(MjkAudioEngine* a, const MjkAudioBank* bank, uint8_t ch_idx) {
    MjkAudioChannel* ch = &a->channels[ch_idx];
    if (ch->vol_delay == 0u) { reset_vol_envelope(a, bank, ch_idx); }
    else { ch->vol_delay--; }

    ch->duration_counter = (uint8_t)(ch->duration_counter - 1u);
    if (ch->duration_counter != 0u) { return; }

    ch->duration_counter = ch->duration_reload;
    ch->vol_offset       = 0u;
    reset_vol_envelope(a, bank, ch_idx);
    ch->sweep_reg        = 0x08u;

    for (uint32_t op = 0u; op < AUDIO_MAX_OPS_FRAME; ++op) {
        const uint8_t* p = resolve_audio_ptr(bank, ch->pc, 1u);
        if (!p) { ch->active = false; return; }
        uint8_t cmd = *p;
        ch->pc++;

        if ((cmd & 0x80u) != 0u) {
            if (exec_cmd(a, bank, ch_idx, cmd)) { return; }
            continue;
        }

        uint8_t n = cmd;
        if (ch->pitch_bend_step != 0) {
            ch->pitch_bend_acc++;
            for (uint8_t k = 0u; k < ch->pitch_bend_acc; ++k) {
                n = (uint8_t)(n + (uint8_t)ch->pitch_bend_step);
                if ((n & 0x0Fu) >= 0x0Bu) {
                    n = (uint8_t)(n + ((ch->pitch_bend_step > 0) ? 5 : -4));
                }
            }
        }
        uint16_t freq = 0u;
        if (!read_audio_u16(bank, (uint16_t)(MJK_CPU_NOTE_FREQ_ADDR + (uint16_t)(n & 0x0Fu) * 2u), &freq)) {
            ch->active = false;
            return;
        }
        freq >>= (n >> 4);
        ch->freq_lo = (uint8_t)(freq & 0xFFu);
        ch->freq_hi = (uint8_t)(((freq >> 8) & 0xFFu) | 0x08u);
        apply_hw_freq_regs(a, ch_idx);
        return;
    }
}

void MjkAudio_Init(MjkAudioEngine* audio) {
    if (!audio) { return; }
    memset(audio, 0, sizeof(*audio));
    MjkSynth_Init(&audio->synth);
}

void MjkAudio_Play(MjkAudioEngine* audio, MjkSoundId sound) {
    if (!audio || audio->queue_count >= MJK_AUDIO_QUEUE_CAP) { return; }
    audio->queue[audio->queue_count++] = sound;
}

uint32_t MjkAudio_NextFrameSampleCount(MjkAudioEngine* audio) {
    if (!audio) { return 734u; }
    audio->ntsc_sample_accum += (double)MJK_AUDIO_SAMPLE_RATE / MJK_NTSC_FPS;
    uint32_t count = (uint32_t)audio->ntsc_sample_accum;
    audio->ntsc_sample_accum -= (double)count;
    return count;
}

void MjkAudio_StepFrame(MjkAudioEngine* audio,
                        const MjkAssets* assets,
                        int16_t* out_samples,
                        uint32_t sample_count) {
    if (!out_samples || sample_count == 0u) { return; }
    if (!audio) {
        memset(out_samples, 0, (size_t)sample_count * sizeof(int16_t));
        return;
    }

    if (assets) {
        drain_queue(audio, &assets->audio);
        uint8_t active_mask = 0u;
        for (uint8_t ch = 0u; ch < MJK_AUDIO_CHANNEL_COUNT; ++ch) {
            if (audio->channels[ch].active) {
                active_mask |= (uint8_t)(1u << ch);
                step_channel(audio, &assets->audio, ch);
            }
        }
        audio->enable_mask = (uint8_t)(((active_mask >> 4) | active_mask) & 0x0Fu);
        for (uint8_t r = 0u; r < 4u; ++r) {
            bool reload_hi = (audio->voices[r].vol & 0x20u) != 0u;
            audio->voices[r].vol &= 0xDFu;
            MjkSynth_OnRegWrite(&audio->synth, audio->voices, r, reload_hi);
        }
    }

    uint32_t q = sample_count / 4u;
    uint32_t off = 0u;
    for (uint8_t step = 0u; step < 4u; ++step) {
        uint32_t n = (step == 3u) ? (sample_count - off) : q;
        MjkSynth_ClockQuarterFrame(&audio->synth, audio->voices);
        if (step == 1u || step == 3u) {
            MjkSynth_ClockSweep(&audio->synth, audio->voices);
        }
        if (n > 0u) {
            MjkSynth_Render(&audio->synth, audio->voices, audio->enable_mask, out_samples + off, n);
            off += n;
        }
    }

    uint32_t energy = 0u;
    for (uint32_t i = 0u; i < sample_count; ++i) {
        int32_t v = out_samples[i];
        energy += (uint32_t)(v < 0 ? -v : v);
    }
    audio->last_frame_energy = energy;
}
