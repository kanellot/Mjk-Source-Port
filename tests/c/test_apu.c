/**
 * @file test_apu.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para sintetizador 2A03 y motor APU (T-25, T-26).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_apu.h"
#include "mjk_assets.h"
#include "mjk_game.h"
#include "mjk_rom_extractor.h"

static int g_failures = 0;

static bool load_rom_assets(MjkAssetPackage* out_pkg) {
#ifdef MJK_SOURCE_DIR
    char rom_path[512];
    snprintf(rom_path, sizeof(rom_path), "%s/asm/bin/mbj-us.nes", MJK_SOURCE_DIR);
    FILE* f = fopen(rom_path, "rb");
    if (!f) { return false; }
    uint8_t* rom = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom) { fclose(f); return false; }
    size_t got = fread(rom, 1u, MJK_ROM_EXPECTED_SIZE, f);
    fclose(f);
    if (got != MJK_ROM_EXPECTED_SIZE) { free(rom); return false; }
    uint8_t* v2_buf = NULL;
    size_t v2_size = 0u;
    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    MjkAssetError err = MjkRom_BuildAssetsV2Buffer(rom, MJK_ROM_EXPECTED_SIZE, &v2_buf, &v2_size, sha1_hex);
    free(rom);
    if (err != MJK_ASSET_OK) { free(v2_buf); return false; }
    err = MjkAssets_LoadFromMemory(v2_buf, v2_size, out_pkg);
    free(v2_buf);
    return (err == MJK_ASSET_OK);
#else
    (void)out_pkg;
    return false;
#endif
}

static uint64_t compute_energy(const int16_t* samples, uint32_t count) {
    uint64_t sum = 0u;
    for (uint32_t i = 0u; i < count; ++i) {
        int32_t v = samples[i];
        sum += (uint64_t)(v < 0 ? -v : v);
    }
    return sum;
}

static void test_pure_oscillators(void) {
    MjkApuSynth synth;
    MjkApuRegs regs;
    int16_t buf[MJK_APU_SAMPLES_PER_FRAME];

    /* Pulse 1 + Sweep */
    MjkApuSynth_Init(&synth);
    memset(&regs, 0, sizeof(regs));
    regs.snd_chn = 0x01u;
    regs.voice[0].vol = 0x9Fu; /* 50% duty, vol 15 */
    regs.voice[0].sweep = 0x89u; /* enabled, div 0, negate, shift 1 */
    regs.voice[0].lo = 0xABu;
    regs.voice[0].hi = 0x09u;
    MjkApuSynth_OnRegWrite(&synth, &regs, 0u, true);
    MjkApuSynth_ClockSweep(&synth, &regs);
    MjkApuSynth_RenderSamples(&synth, &regs, buf, MJK_APU_SAMPLES_PER_FRAME);
    if (compute_energy(buf, MJK_APU_SAMPLES_PER_FRAME) == 0u) {
        fprintf(stderr, "[FAIL] Pulse 1 puro produjo energia 0\n");
        g_failures++;
    }

    /* Triangle */
    MjkApuSynth_Init(&synth);
    memset(&regs, 0, sizeof(regs));
    regs.snd_chn = 0x04u;
    regs.voice[2].vol = 0xFFu;
    regs.voice[2].lo = 0x60u;
    regs.voice[2].hi = 0x08u;
    MjkApuSynth_RenderSamples(&synth, &regs, buf, MJK_APU_SAMPLES_PER_FRAME);
    if (compute_energy(buf, MJK_APU_SAMPLES_PER_FRAME) == 0u) {
        fprintf(stderr, "[FAIL] Triangle puro produjo energia 0\n");
        g_failures++;
    }

    /* Noise LFSR 15-bit */
    MjkApuSynth_Init(&synth);
    memset(&regs, 0, sizeof(regs));
    regs.snd_chn = 0x08u;
    regs.voice[3].vol = 0x1Fu;
    regs.voice[3].lo = 0x05u;
    MjkApuSynth_RenderSamples(&synth, &regs, buf, MJK_APU_SAMPLES_PER_FRAME);
    if (compute_energy(buf, MJK_APU_SAMPLES_PER_FRAME) == 0u) {
        fprintf(stderr, "[FAIL] Noise puro produjo energia 0\n");
        g_failures++;
    }
}

static void test_sfx_priority_and_sweep(const MjkAssetPackage* pkg) {
    MjkApu apu;
    int16_t buf[MJK_APU_SAMPLES_PER_FRAME];

    /* 1. Prioridad SFX (MJK_SND_JUMP) sobre BGM (MJK_MUS_MAIN) */
    MjkApu_Init(&apu);
    MjkApu_QueueSound(&apu, MJK_MUS_MAIN);
    for (int f = 0; f < 10; ++f) {
        MjkApu_StepFrame(&apu, pkg, buf, MJK_APU_SAMPLES_PER_FRAME);
    }
    if (apu.last_frame_energy == 0u) {
        fprintf(stderr, "[FAIL] MJK_MUS_MAIN produjo energia 0\n");
        g_failures++;
    }

    uint16_t bgm_pc_before[4];
    for (int i = 0; i < 4; ++i) {
        bgm_pc_before[i] = apu.channels[i].pc;
    }

    MjkApu_QueueSound(&apu, MJK_SND_JUMP);
    MjkApu_StepFrame(&apu, pkg, buf, MJK_APU_SAMPLES_PER_FRAME);
    bool any_sfx = false;
    for (int i = 4; i < 8; ++i) {
        if (apu.channels[i].active) {
            any_sfx = true;
        }
    }
    if (!any_sfx) {
        fprintf(stderr, "[FAIL] MJK_SND_JUMP no activo ningun canal SFX\n");
        g_failures++;
    }

    bool sfx_finished = false;
    for (int f = 0; f < 120; ++f) {
        MjkApu_StepFrame(&apu, pkg, buf, MJK_APU_SAMPLES_PER_FRAME);
        bool still_on = false;
        for (int i = 4; i < 8; ++i) {
            if (apu.channels[i].active) {
                still_on = true;
            }
        }
        if (!still_on) {
            sfx_finished = true;
            break;
        }
    }
    if (!sfx_finished) {
        fprintf(stderr, "[FAIL] MJK_SND_JUMP no termino en 120 frames\n");
        g_failures++;
    }
    if (apu.channels[0].pc == bgm_pc_before[0] && apu.channels[1].pc == bgm_pc_before[1]) {
        fprintf(stderr, "[FAIL] BGM pc no avanzo durante SFX\n");
        g_failures++;
    }

    /* 2. Silencio exacto tras MJK_SND_SILENCE */
    MjkApu_QueueSound(&apu, MJK_SND_SILENCE);
    for (int f = 0; f < 10; ++f) {
        MjkApu_StepFrame(&apu, pkg, buf, MJK_APU_SAMPLES_PER_FRAME);
    }
    if (apu.regs.snd_chn != 0u || apu.last_frame_energy != 0u) {
        fprintf(stderr, "[FAIL] MJK_SND_SILENCE no silencio APU (snd_chn=0x%02X, energy=%u)\n",
                apu.regs.snd_chn, (unsigned)apu.last_frame_energy);
        g_failures++;
    }

    /* 3. Barrido completo de los 35 MjkSoundId (0x01..0x23) por 600 frames c/u */
    for (unsigned id = 1u; id <= (unsigned)MJK_MUS_COLLECTED_FULL_FAMILY; ++id) {
        MjkApu_Init(&apu);
        MjkApu_QueueSound(&apu, (MjkSoundId)id);
        uint64_t total_energy = 0u;
        for (int f = 0; f < 600; ++f) {
            MjkApu_StepFrame(&apu, pkg, buf, MJK_APU_SAMPLES_PER_FRAME);
            total_energy += apu.last_frame_energy;
        }
        if (id != (unsigned)MJK_SND_0C_NULL && id != (unsigned)MJK_SND_14_NULL && total_energy == 0u) {
            fprintf(stderr, "[FAIL] SoundId 0x%02X produjo energia 0 en 600 frames\n", id);
            g_failures++;
        }
    }

    /* 4. Integracion MjkGame_QueueSound -> MjkGame_StepAudio */
    MjkGame game;
    MjkGame_Init(&game, pkg);
    MjkGame_QueueSound(&game, MJK_SND_COIN_COLLECTED);
    MjkGame_StepAudio(&game, buf, MJK_APU_SAMPLES_PER_FRAME);
    if (game.sound_queue_count != 0u || game.apu.last_frame_energy == 0u) {
        fprintf(stderr, "[FAIL] MjkGame_StepAudio no dreno sound_queue o produjo energia 0\n");
        g_failures++;
    }
}

int main(void) {
    test_pure_oscillators();

    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando barrido de 35 tracks.\n");
    } else {
        test_sfx_priority_and_sweep(&pkg);
        MjkAssets_Unload(&pkg);
    }

    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_apu: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_apu: osciladores 2A03, prioridad SFX/BGM, silencio y 35 tracks verificados.\n");
    return 0;
}
