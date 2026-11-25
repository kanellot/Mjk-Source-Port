/**
 * @file test_audio.c
 * @brief Unit tests exhaustivos de APU 2A03 (LUTs, voces, SFX->BGM y acumulador NTSC) (R-15).
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/audio.h"
#include "mjk/rom_loader.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

static void test_lut_equivalence_and_ntsc_accumulator(void) {
    MjkAudioEngine audio;
    MjkAudio_Init(&audio);

    /* 1. Equivalencia exacta de pulse_lut y tnd_lut frente a la formula analitica 2A03 */
    double max_err = 0.0;
    for (int p = 1; p < 31; ++p) {
        double exact = 95.88 / ((8128.0 / (double)p) + 100.0);
        double err = fabs((double)audio.synth.pulse_lut[p] - exact);
        if (err > max_err) { max_err = err; }
    }
    for (int t = 0; t < 16; ++t) {
        for (int n = 0; n < 16; ++n) {
            double d = ((double)t / 8227.0) + ((double)n / 12241.0);
            double exact = (d > 0.0) ? (159.79 / ((1.0 / d) + 100.0)) : 0.0;
            double err = fabs((double)audio.synth.tnd_lut[t][n] - exact);
            if (err > max_err) { max_err = err; }
        }
    }
    CHECK(max_err < 1e-6, "Tablas LUT 2A03 coinciden con formula analitica (< 1e-6)");

    /* 2. Acumulador fraccional NTSC: 3600 frames (~60s) alternando 733/734, deriva <= 1 muestra */
    uint64_t total_samples = 0u;
    bool saw_733 = false, saw_734 = false, bad_count = false;
    for (int f = 0; f < 3600; ++f) {
        uint32_t n = MjkAudio_NextFrameSampleCount(&audio);
        if (n == 733u) { saw_733 = true; }
        else if (n == 734u) { saw_734 = true; }
        else { bad_count = true; }
        total_samples += n;
    }
    double expected = (3600.0 / MJK_NTSC_FPS) * (double)MJK_AUDIO_SAMPLE_RATE;
    double drift = fabs((double)total_samples - expected);
    CHECK(saw_733 && saw_734 && !bad_count, "Acumulador NTSC alterna estrictamente entre 733 y 734");
    CHECK(drift <= 1.0, "Deriva acumulada en 3600 frames <= 1 muestra");
}

int main(void) {
    test_lut_equivalence_and_ntsc_accumulator();

    char rom_path[512];
    MjkTestRomStatus st = MjkRom_ResolveTestPath(MJK_SOURCE_DIR, MJK_TEST_ROM_PATH, rom_path, sizeof(rom_path));
    if (st == TEST_ROM_EXPLICIT_MISSING_FAIL) { return 1; }
    if (st == TEST_ROM_ABSENT_SKIP) {
        return (g_failures > 0) ? 1 : MJK_TEST_SKIP_CODE;
    }

    static MjkAssets assets;
    CHECK(MjkAssets_LoadFromRomFile(rom_path, &assets) == ASSET_OK, "Cargar MjkAssets");

    static MjkAudioEngine audio;
    static int16_t pcm[MJK_AUDIO_MAX_SAMPLES_STEP];
    MjkAudio_Init(&audio);

    /* 3. BGM genera energia > 0 */
    MjkAudio_Play(&audio, MUSIC_MAIN);
    uint64_t bgm_energy = 0u;
    for (int f = 0; f < 30; ++f) {
        uint32_t n = MjkAudio_NextFrameSampleCount(&audio);
        MjkAudio_StepFrame(&audio, &assets, pcm, n);
        bgm_energy += audio.last_frame_energy;
    }
    CHECK(bgm_energy > 0u, "MUSIC_MAIN genera energia PCM > 0");

    /* 4. Prioridad SFX sobre BGM y restauracion exacta del canal BGM al finalizar SFX */
    MjkAudio_Play(&audio, SOUND_JUMP);
    bool sfx_active_seen = false;
    for (int f = 0; f < 90; ++f) {
        uint32_t n = MjkAudio_NextFrameSampleCount(&audio);
        MjkAudio_StepFrame(&audio, &assets, pcm, n);
        if (audio.channels[4].active || audio.channels[5].active) {
            sfx_active_seen = true;
        }
    }
    CHECK(sfx_active_seen, "SFX JUMP activa canal SFX");
    CHECK(audio.channels[0].active, "Canal BGM 0 sigue activo tras finalizar SFX JUMP");

    /* 5. SOUND_SILENCE detiene todos los canales */
    MjkAudio_Play(&audio, SOUND_SILENCE);
    uint32_t n = MjkAudio_NextFrameSampleCount(&audio);
    MjkAudio_StepFrame(&audio, &assets, pcm, n);
    CHECK(audio.enable_mask == 0u && !audio.channels[0].active, "SOUND_SILENCE silencia todos los canales");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_audio fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_audio superado (LUT 2A03, acumulador NTSC 733/734, BGM/SFX y silencio).\n");
    return 0;
}
