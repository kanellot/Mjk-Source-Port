/**
 * @file mjk_apu.c
 * @brief Máquina virtual bytecode de 8 canales (4 BGM + 4 SFX) sobre `MjkAssetPackage`.
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Reimplementación limpia de `asm/src/music-engine.asm` sin código de Tecmo en el repo:
 * - Canales `0..3`: BGM (`Mus0..3` -> Pulse 1, Pulse 2, Triangle, Noise).
 * - Canales `4..7`: SFX (`Sfx0..3`), con prioridad sobre el canal BGM hermano.
 * - Al terminar un SFX (`$F6/$F7/$FF`), restaura los registros hardware del BGM
 *   sin haber detenido su reloj interno, evitando desfases o notas colgadas.
 */

#include "mjk_apu.h"
#include <string.h>

static bool is_bgm_inhibited(const MjkApu* apu, uint8_t ch_idx) {
    if (ch_idx >= 4u) {
        return false;
    }
    return (apu->channels[ch_idx | 4u].active & 1u) != 0u;
}

static void apply_hw_freq_regs(MjkApu* apu, uint8_t ch_idx) {
    if (is_bgm_inhibited(apu, ch_idx)) {
        return;
    }
    uint8_t r = (uint8_t)(ch_idx & 3u);
    const MjkApuChannel* ch = &apu->channels[ch_idx];
    apu->regs.voice[r].vol |= 0x20u;
    apu->regs.voice[r].sweep = ch->sweep_reg;
    apu->regs.voice[r].lo = ch->freq_lo;
    apu->regs.voice[r].hi = ch->freq_hi;
}

static void maybe_reset_vol_delay(MjkApu* apu, const MjkAssetPackage* pkg, uint8_t ch_idx) {
    MjkApuChannel* ch = &apu->channels[ch_idx];
    if (ch->vol_table_addr == 0u) {
        return;
    }
    uint16_t addr = (uint16_t)(ch->vol_table_addr + (uint16_t)ch->vol_offset);
    const uint8_t* entry = MjkAssets_ResolveCpuPtr(pkg, addr, 2u);
    if (!entry) {
        return;
    }
    uint8_t val = entry[0];
    uint8_t delay = entry[1];
    ch->vol_offset = (uint8_t)(ch->vol_offset + 2u);
    ch->vol_delay = delay;

    uint8_t combined = (uint8_t)(val | (ch->vol_reg & 0xC0u));
    if ((combined & 0x10u) != 0u) {
        int vol = (int)(combined & 0x0Fu) - (int)ch->vol_atten;
        if (vol < 0) {
            vol = 0;
        }
        combined = (uint8_t)((combined & 0xD0u) | ((uint8_t)vol & 0x0Fu));
    }
    ch->vol_reg = combined;

    if (!is_bgm_inhibited(apu, ch_idx)) {
        apu->regs.voice[ch_idx & 3u].vol = ch->vol_reg;
    }
}

static void maybe_update_vol_delay(MjkApu* apu, const MjkAssetPackage* pkg, uint8_t ch_idx) {
    MjkApuChannel* ch = &apu->channels[ch_idx];
    if (ch->vol_delay == 0u) {
        maybe_reset_vol_delay(apu, pkg, ch_idx);
    } else {
        ch->vol_delay--;
    }
}

static void process_sound_queue(MjkApu* apu, const MjkAssetPackage* pkg) {
    for (uint8_t q = 0u; q < apu->queue_count; ++q) {
        MjkSoundId id = apu->queue[q];
        if (id == MJK_SND_SILENCE) {
            for (uint8_t i = 0u; i < MJK_APU_CHANNEL_COUNT; ++i) {
                apu->channels[i].active = 0u;
            }
            apu->regs.snd_chn = 0u;
            continue;
        }

        uint16_t hdr_addr = 0u;
        if (!MjkAssets_GetSoundDataPtr(pkg, id, &hdr_addr)) {
            continue;
        }

        for (uint16_t off = 0u; off < 64u; off = (uint16_t)(off + 3u)) {
            const uint8_t* p_ch = MjkAssets_ResolveCpuPtr(pkg, (uint16_t)(hdr_addr + off), 1u);
            if (!p_ch || *p_ch == 0xFFu) {
                break;
            }
            uint8_t ch_idx = *p_ch;
            const uint8_t* ptr_bytes = MjkAssets_ResolveCpuPtr(pkg, (uint16_t)(hdr_addr + off + 1u), 2u);
            if (!ptr_bytes) {
                break;
            }
            if (ch_idx < MJK_APU_CHANNEL_COUNT) {
                MjkApuChannel* ch = &apu->channels[ch_idx];
                memset(ch, 0, sizeof(*ch));
                ch->active = 1u;
                ch->duration_counter = 1u;
                ch->pc = (uint16_t)(ptr_bytes[0] | ((uint16_t)ptr_bytes[1] << 8));
            }
        }
    }
    apu->queue_count = 0u;
}

static bool exec_fx_command(MjkApu* apu, const MjkAssetPackage* pkg, uint8_t ch_idx, uint8_t cmd) {
    MjkApuChannel* ch = &apu->channels[ch_idx];
    switch (cmd & 0x07u) {
        case 0u: { /* 0xF0 <lo> <hi>: JMP */
            uint16_t target = 0u;
            if (!MjkAssets_ReadCpuU16(pkg, ch->pc, &target)) {
                ch->active = 0u;
                return true;
            }
            ch->pc = target;
            return false;
        }
        case 1u: { /* 0xF1 <count>: Loop Start */
            const uint8_t* p_cnt = MjkAssets_ResolveCpuPtr(pkg, ch->pc, 1u);
            if (!p_cnt) {
                ch->active = 0u;
                return true;
            }
            ch->loop_count = *p_cnt;
            ch->pc++;
            ch->loop_addr = ch->pc;
            return false;
        }
        case 2u: /* 0xF2: Loop End */
            if (ch->loop_count > 1u) {
                ch->loop_count--;
                ch->pc = ch->loop_addr;
            } else {
                ch->loop_count = 0u;
            }
            return false;
        case 3u: { /* 0xF3 <lo> <hi>: JSR */
            uint16_t target = 0u;
            if (!MjkAssets_ReadCpuU16(pkg, ch->pc, &target)) {
                ch->active = 0u;
                return true;
            }
            uint16_t ret_addr = (uint16_t)(ch->pc + 2u);
            if (ch->stack_sp < MJK_APU_CALL_STACK_DEPTH) {
                ch->call_stack[ch->stack_sp++] = ret_addr;
            }
            ch->pc = target;
            return false;
        }
        case 4u: /* 0xF4: RTS */
            if (ch->stack_sp > 0u) {
                ch->pc = ch->call_stack[--ch->stack_sp];
                return false;
            }
            ch->active = 0u;
            return true;
        case 5u: { /* 0xF5 <byte>: Hardware Sweep */
            const uint8_t* p_sw = MjkAssets_ResolveCpuPtr(pkg, ch->pc, 1u);
            if (!p_sw) {
                ch->active = 0u;
                return true;
            }
            ch->sweep_reg = *p_sw;
            ch->pc++;
            return false;
        }
        default: /* 0xF6..0xFF: Stop Channel (+ restore BGM sibling if SFX) */
            ch->active = 0u;
            if ((ch_idx & 4u) != 0u) {
                uint8_t bgm_idx = (uint8_t)(ch_idx & 3u);
                apply_hw_freq_regs(apu, bgm_idx);
                apu->regs.voice[bgm_idx].vol = (uint8_t)(apu->channels[bgm_idx].vol_reg | 0x20u);
            }
            return true;
    }
}

static bool exec_command(MjkApu* apu, const MjkAssetPackage* pkg, uint8_t ch_idx, uint8_t cmd) {
    MjkApuChannel* ch = &apu->channels[ch_idx];
    switch ((cmd >> 4) & 0x07u) {
        case 0u:
        case 1u: { /* 0x80..0x9F: Note Length */
            const uint8_t* p_len = MjkAssets_ResolveCpuPtr(
                pkg, (uint16_t)(MJK_ADDR_NOTE_LENGTH_TABLE + (uint16_t)(cmd & 0x1Fu)), 1u);
            if (p_len) {
                ch->duration_counter = *p_len;
                ch->duration_reload = *p_len;
            }
            return false;
        }
        case 2u:
        case 3u: { /* 0xA0..0xBF: Volume Envelope Table */
            uint16_t vol_addr = 0u;
            if (MjkAssets_ReadCpuU16(
                    pkg, (uint16_t)(MJK_ADDR_SOUND_VOL_TABLE + ((uint16_t)(cmd & 0x1Fu) * 2u)), &vol_addr)) {
                ch->vol_table_addr = vol_addr;
                ch->vol_offset = 0u;
                maybe_reset_vol_delay(apu, pkg, ch_idx);
            }
            return false;
        }
        case 4u: /* 0xC0..0xCF: Direct Noise/Freq Period + End Frame */
            ch->freq_lo = (uint8_t)(cmd & 0x0Fu);
            ch->freq_hi = 0x08u;
            apply_hw_freq_regs(apu, ch_idx);
            return true;
        case 5u: /* 0xD0..0xDF: Duty/Ctrl or Pitch Bend */
            if ((cmd & 0x0Fu) < 8u) {
                uint8_t a = (uint8_t)(cmd & 0x0Fu);
                uint8_t c0 = (uint8_t)(a & 1u);
                a = (uint8_t)((a >> 2) | (c0 << 7));
                ch->vol_reg = (uint8_t)((ch->vol_reg & 0x3Fu) | a);
            } else {
                static const int8_t k_pitch_bend[4] = { 1, 2, -1, -2 };
                int8_t step = k_pitch_bend[cmd & 3u];
                if (step != ch->pitch_bend_step) {
                    ch->pitch_bend_step = step;
                    ch->pitch_bend_acc = 0u;
                }
            }
            return false;
        case 6u: /* 0xE0..0xEF: Volume Attenuation */
            ch->vol_atten = (uint8_t)(cmd & 0x0Fu);
            return false;
        default: /* 0xF0..0xFF */
            return exec_fx_command(apu, pkg, ch_idx, cmd);
    }
}

static void step_channel(MjkApu* apu, const MjkAssetPackage* pkg, uint8_t ch_idx) {
    MjkApuChannel* ch = &apu->channels[ch_idx];
    maybe_update_vol_delay(apu, pkg, ch_idx);

    /* HandleSoundChannel (music-engine.asm:142-148): SBC #1 envuelve 0 -> 0xFF (256 frames para $80). */
    ch->duration_counter = (uint8_t)(ch->duration_counter - 1u);
    if (ch->duration_counter != 0u) {
        return;
    }

    ch->duration_counter = ch->duration_reload;
    ch->vol_offset = 0u;
    maybe_reset_vol_delay(apu, pkg, ch_idx);
    ch->sweep_reg = 0x08u;

    for (uint32_t op = 0u; op < MJK_APU_MAX_OPS_PER_FRAME; ++op) {
        const uint8_t* p_cmd = MjkAssets_ResolveCpuPtr(pkg, ch->pc, 1u);
        if (!p_cmd) {
            ch->active = 0u;
            return;
        }
        uint8_t cmd = *p_cmd;
        ch->pc++;

        if ((cmd & 0x80u) != 0u) {
            if (exec_command(apu, pkg, ch_idx, cmd)) {
                return;
            }
            continue;
        }

        uint8_t n = cmd;
        if (ch->pitch_bend_step != 0) {
            ch->pitch_bend_acc++;
            uint8_t count = ch->pitch_bend_acc;
            for (uint8_t k = 0u; k < count; ++k) {
                n = (uint8_t)(n + (uint8_t)ch->pitch_bend_step);
                if ((n & 0x0Fu) >= 0x0Bu) {
                    n = (uint8_t)(n + ((ch->pitch_bend_step > 0) ? 5 : -4));
                }
            }
        }

        uint8_t semi = (uint8_t)(n & 0x0Fu);
        uint8_t oct = (uint8_t)(n >> 4);
        uint16_t freq = 0u;
        if (!MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_NOTE_FREQ_TABLE + (uint16_t)(semi * 2u)), &freq)) {
            ch->active = 0u;
            return;
        }
        freq = (uint16_t)(freq >> oct);
        ch->freq_lo = (uint8_t)(freq & 0xFFu);
        ch->freq_hi = (uint8_t)(((freq >> 8) & 0xFFu) | 0x08u);
        apply_hw_freq_regs(apu, ch_idx);
        return;
    }
}

static void write_sound_registers(MjkApu* apu, uint8_t active_mask) {
    apu->regs.snd_chn = (uint8_t)(((active_mask >> 4) | active_mask) & 0x0Fu);
    for (uint8_t r = 0u; r < 4u; ++r) {
        bool reload_hi = (apu->regs.voice[r].vol & 0x20u) != 0u;
        apu->regs.voice[r].vol &= 0xDFu;
        MjkApuSynth_OnRegWrite(&apu->synth, &apu->regs, r, reload_hi);
    }
}

void MjkApu_Init(MjkApu* apu) {
    if (!apu) {
        return;
    }
    memset(apu, 0, sizeof(*apu));
    MjkApuSynth_Init(&apu->synth);
}

void MjkApu_QueueSound(MjkApu* apu, MjkSoundId id) {
    if (!apu) {
        return;
    }
    if (apu->queue_count < MJK_APU_QUEUE_CAP) {
        apu->queue[apu->queue_count++] = id;
    }
}

void MjkApu_StepFrame(MjkApu* apu,
                      const MjkAssetPackage* pkg,
                      int16_t* out_samples,
                      uint32_t sample_count) {
    if (!out_samples || sample_count == 0u) {
        return;
    }
    if (!apu) {
        memset(out_samples, 0, (size_t)sample_count * sizeof(int16_t));
        return;
    }

    if (pkg) {
        process_sound_queue(apu, pkg);
        uint8_t active_mask = 0u;
        for (uint8_t ch = 0u; ch < MJK_APU_CHANNEL_COUNT; ++ch) {
            if ((apu->channels[ch].active & 1u) != 0u) {
                active_mask |= (uint8_t)(1u << ch);
                step_channel(apu, pkg, ch);
            }
        }
        write_sound_registers(apu, active_mask);
    }

    uint32_t q = sample_count / 4u;
    uint32_t offset = 0u;
    for (uint8_t step = 0u; step < 4u; ++step) {
        uint32_t n = (step == 3u) ? (sample_count - offset) : q;
        MjkApuSynth_ClockQuarterFrame(&apu->synth, &apu->regs);
        if (step == 1u || step == 3u) {
            MjkApuSynth_ClockSweep(&apu->synth, &apu->regs);
        }
        if (n > 0u) {
            MjkApuSynth_RenderSamples(&apu->synth, &apu->regs, out_samples + offset, n);
            offset += n;
        }
    }

    uint32_t energy = 0u;
    for (uint32_t i = 0u; i < sample_count; ++i) {
        int32_t s = out_samples[i];
        energy += (uint32_t)(s < 0 ? -s : s);
    }
    apu->last_frame_energy = energy;
}
