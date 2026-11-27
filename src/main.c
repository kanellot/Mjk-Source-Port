/**
 * @file main.c
 * @brief Punto de entrada limpio de Mighty Bomb Jack (C11 + SDL2).
 *
 * Orquesta unicamente:
 *  1. Argumentos CLI (`--rom <ruta>`, `--assets <ruta>`).
 *  2. Carga o auto-extraccion Clean-Room BYOR v2 de `assets/assets.dat`.
 *  3. Bucle principal a paso fijo NTSC (60.0988 Hz) con `MjkInputLatch` y
 *     acumulador fraccional de muestras de audio (733/734 muestras/frame).
 */

#include <SDL_main.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/audio.h"
#include "mjk/game.h"
#include "mjk/input.h"
#include "mjk/platform.h"
#include "mjk/renderer.h"
#include "mjk/rom_loader.h"

typedef struct {
    const char* rom_path;
    const char* assets_path;
} CliOptions;

static bool parse_cli(int argc, char* argv[], CliOptions* opts) {
    memset(opts, 0, sizeof(*opts));
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--rom") == 0) {
            if ((i + 1) >= argc || argv[i + 1][0] == '\0') {
                fprintf(stderr, "[ERROR CLI]: Falta una ruta valida tras --rom.\n");
                return false;
            }
            opts->rom_path = argv[++i];
        } else if (strcmp(argv[i], "--assets") == 0) {
            if ((i + 1) >= argc || argv[i + 1][0] == '\0') {
                fprintf(stderr, "[ERROR CLI]: Falta una ruta valida tras --assets.\n");
                return false;
            }
            opts->assets_path = argv[++i];
        } else {
            fprintf(stderr, "[ERROR CLI]: Opcion desconocida: %s\nUso: mjk [--rom <mbj-us.nes>] [--assets <assets.dat>]\n",
                    argv[i]);
            return false;
        }
    }
    return true;
}

static bool join_path(char* out, size_t out_sz, const char* dir, const char* rel) {
    size_t len = strlen(dir);
    bool sep = (len > 0u && (dir[len - 1u] == '/' || dir[len - 1u] == '\\'));
    int n = snprintf(out, out_sz, sep ? "%s%s" : "%s/%s", dir, rel);
    return n >= 0 && (size_t)n < out_sz;
}

static bool file_exists(const char* path) {
    if (!path || path[0] == '\0') { return false; }
    FILE* f = fopen(path, "rb");
    if (!f) { return false; }
    fclose(f);
    return true;
}

static bool ensure_parent_dir(const char* path) {
    const char* last_sep = NULL;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') { last_sep = p; }
    }
    if (!last_sep || last_sep == path) { return true; }
    size_t dlen = (size_t)(last_sep - path);
    char buf[1024];
    if (dlen >= sizeof(buf)) { return false; }
    memcpy(buf, path, dlen);
    buf[dlen] = '\0';
    return MjkPlatform_EnsureDir(buf);
}

static bool load_or_extract_assets(const CliOptions* opts, const char* exe_dir, MjkAssets* out_assets) {
    char exe_buf[1024];
    MjkAssetError err = ASSET_ERR_IO;

    if (opts->assets_path) {
        err = MjkAssets_LoadFromAssetsFile(opts->assets_path, out_assets);
        if (err == ASSET_OK) { return true; }
    } else {
        err = MjkAssets_LoadFromAssetsFile("assets/assets.dat", out_assets);
        if (err == ASSET_OK) { return true; }
        if (exe_dir && join_path(exe_buf, sizeof(exe_buf), exe_dir, "assets/assets.dat")) {
            err = MjkAssets_LoadFromAssetsFile(exe_buf, out_assets);
            if (err == ASSET_OK) { return true; }
        }
    }

    const char* found_rom = NULL;
    char exe_rom[1024];
    if (opts->rom_path) {
        if (file_exists(opts->rom_path)) {
            found_rom = opts->rom_path;
        }
    } else if (file_exists("mbj-us.nes")) {
        found_rom = "mbj-us.nes";
    } else if (exe_dir && join_path(exe_rom, sizeof(exe_rom), exe_dir, "mbj-us.nes") && file_exists(exe_rom)) {
        found_rom = exe_rom;
    }

    if (!found_rom) {
        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "No se encontro assets.dat valido (%s) ni la ROM mbj-us.nes.\n"
                 "Indica --rom <ruta_mbj-us.nes> o coloca mbj-us.nes junto al ejecutable.\n"
                 "SHA-1 esperado: %s",
                 MjkRom_ErrorString(err), MJK_ROM_EXPECTED_SHA1);
        fprintf(stderr, "[ERROR BYOR]: %s\n", msg);
        MjkPlatform_ShowMessageBox("Mighty Bomb Jack - Falta ROM", msg, true);
        return false;
    }

    const char* target = opts->assets_path ? opts->assets_path : "assets/assets.dat";
    if (!ensure_parent_dir(target)) {
        MjkPlatform_ShowMessageBox("Mighty Bomb Jack", "No se pudo crear el directorio assets/.", true);
        return false;
    }

    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    MjkAssetError ext_err = MjkRom_ExtractFile(found_rom, target, sha1_hex);
    if (ext_err == ASSET_OK) {
        ext_err = MjkAssets_LoadFromAssetsFile(target, out_assets);
    }
    if (ext_err != ASSET_OK) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "Error BYOR al procesar '%s': %s.\nSHA-1 esperado: %s",
                 found_rom, MjkRom_ErrorString(ext_err), MJK_ROM_EXPECTED_SHA1);
        fprintf(stderr, "[ERROR BYOR]: %s\n", msg);
        MjkPlatform_ShowMessageBox("Mighty Bomb Jack - Error BYOR", msg, true);
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    CliOptions opts;
    if (!parse_cli(argc, argv, &opts)) {
        return 1;
    }

    MjkPlatformConfig cfg;
    cfg.window_title = "Mighty Bomb Jack (NES 1987) - Source Port C11";
    cfg.window_scale = 3;
    if (!MjkPlatform_Init(&cfg)) {
        return 1;
    }

    char exe_dir[512];
    bool has_exe_dir = MjkPlatform_GetExecutableDir(exe_dir, sizeof(exe_dir));

    static MjkAssets assets;
    if (!load_or_extract_assets(&opts, has_exe_dir ? exe_dir : NULL, &assets)) {
        MjkPlatform_Shutdown();
        return 1;
    }

    static MjkGame        game;
    static MjkAudioEngine audio;
    static uint32_t       framebuffer[MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT];
    static int16_t        audio_samples[MJK_AUDIO_MAX_SAMPLES_STEP];

    MjkGame_Init(&game, &assets);
    MjkAudio_Init(&audio);

    MjkInputLatch input_latch;
    MjkInputLatch_Init(&input_latch);

    const double step_seconds = 1.0 / MJK_NTSC_FPS;
    double accumulator        = 0.0;
    uint64_t prev_us          = MjkPlatform_GetTimeMicroseconds();

    while (MjkPlatform_IsRunning()) {
        uint64_t now_us = MjkPlatform_GetTimeMicroseconds();
        double dt = (double)(now_us - prev_us) / 1000000.0;
        prev_us = now_us;
        if (dt > 0.1) { dt = 0.1; }
        accumulator += dt;

        /* Acumular flancos de entrada sin perderlos ni duplicarlos entre pasos fijos */
        MjkInputLatch_Poll(&input_latch, MjkPlatform_PollRawButtons());

        bool stepped = false;
        while (accumulator >= step_seconds) {
            MjkInputStep step_in;
            MjkInputLatch_ConsumeStep(&input_latch, &step_in);
            MjkGame_StepFrame(&game, &step_in);

            for (uint8_t i = 0u; i < game.sound_queue_count; ++i) {
                MjkAudio_Play(&audio, game.sound_queue[i]);
            }
            uint32_t n_samples = MjkAudio_NextFrameSampleCount(&audio);
            MjkAudio_StepFrame(&audio, &assets, audio_samples, n_samples);
            MjkPlatform_QueueAudioSamples(audio_samples, n_samples);

            accumulator -= step_seconds;
            stepped = true;
        }

        if (stepped) {
            MjkRenderer_DrawFrame(&game, &assets, framebuffer);
            MjkPlatform_PresentFramebuffer(framebuffer);
        } else {
            MjkPlatform_SleepMilliseconds(1u);
        }
    }

    MjkAssets_Unload(&assets);
    MjkPlatform_Shutdown();
    return 0;
}
