/**
 * @file main.c
 * @brief Punto de entrada multiplataforma (Windows/Linux) del port Mighty Bomb Jack.
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Orquesta únicamente argumentos CLI, la capa HAL (`MjkPlatform_*`) y el bucle
 * principal de paso fijo a 60.0988 Hz (NTSC NES). Cero llamadas Win32/POSIX directas.
 */

#include <SDL_main.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_build_info.h"
#include "mjk_platform.h"

#define MJK_SAMPLES_PER_NTSC_FRAME 735u

typedef struct {
    const char* rom_path;
    bool        headless;
    uint32_t    max_frames;
} MjkCliOptions;

static void render_hal_test_pattern(MjkFrameOutput* out, uint32_t frame_index) {
    static const uint32_t k_bg_blue = 0xFF2038ECu;
    static const uint32_t k_bar_gold = 0xFFFCA044u;
    static const uint32_t k_border_white = 0xFFFCFCFCu;

    uint32_t bar_y = (frame_index * 2u) % (uint32_t)MJK_SCREEN_HEIGHT;
    for (int y = 0; y < MJK_SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < MJK_SCREEN_WIDTH; ++x) {
            bool is_border = (x == 0 || x == MJK_SCREEN_WIDTH - 1 ||
                              y == 0 || y == MJK_SCREEN_HEIGHT - 1);
            uint32_t color = k_bg_blue;
            if (is_border) {
                color = k_border_white;
            } else if ((uint32_t)y >= bar_y && (uint32_t)y < bar_y + 4u) {
                color = k_bar_gold;
            }
            out->framebuffer[y * MJK_SCREEN_WIDTH + x] = color;
        }
    }

    out->audio_sample_count = MJK_SAMPLES_PER_NTSC_FRAME;
    memset(out->audio_samples, 0, MJK_SAMPLES_PER_NTSC_FRAME * sizeof(int16_t));
}

static bool parse_cli_options(int argc, char* argv[], MjkCliOptions* opts) {
    opts->rom_path = NULL;
    opts->headless = false;
    opts->max_frames = 0u;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--headless") == 0) {
            opts->headless = true;
        } else if (strcmp(argv[i], "--smoke-test") == 0) {
            opts->headless = true;
            opts->max_frames = 5u;
        } else if (strcmp(argv[i], "--rom") == 0 && (i + 1) < argc) {
            opts->rom_path = argv[++i];
        } else if (strcmp(argv[i], "--frames") == 0 && (i + 1) < argc) {
            char* end = NULL;
            unsigned long parsed = strtoul(argv[++i], &end, 10);
            if (!end || *end != '\0') {
                fprintf(stderr, "[ERROR CLI]: Valor inválido para --frames.\n");
                return false;
            }
            opts->max_frames = (uint32_t)parsed;
        } else {
            fprintf(stderr, "[ERROR CLI]: Opción desconocida: %s\n", argv[i]);
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    MjkCliOptions opts;
    if (!parse_cli_options(argc, argv, &opts)) {
        return 1;
    }

    MjkPlatformConfig cfg;
    cfg.window_title = "Mighty Bomb Jack (NES 1987) - Source Port C11";
    cfg.window_scale = 3;
    cfg.headless = opts.headless;
    cfg.enable_audio = !opts.headless;

    if (!MjkPlatform_Init(&cfg)) {
        return 1;
    }

    char exe_dir[512];
    if (!MjkPlatform_GetExecutableDir(exe_dir, sizeof(exe_dir))) {
        snprintf(exe_dir, sizeof(exe_dir), "(desconocido)");
    }

    printf("Mighty Bomb Jack C11 Port (%s, ABI v%u)\n",
           MjkCore_Version(), (unsigned)MjkCore_AbiVersion());
    printf("  Dir base : %s\n", exe_dir);
    printf("  Modo     : %s | ROM CLI: %s\n",
           opts.headless ? "headless" : "SDL2",
           opts.rom_path ? opts.rom_path : "(auto)");

    static MjkFrameOutput frame_output;
    memset(&frame_output, 0, sizeof(frame_output));

    MjkInputState inputs = {0u, 0u, 0u};
    uint32_t debug_keys = MJK_DBG_NONE;
    uint32_t simulated_frames = 0u;

    const double seconds_per_frame = 1.0 / MJK_NTSC_FPS;
    double accumulator = 0.0;
    uint64_t previous_time = MjkPlatform_GetTimeMicroseconds();

    while (MjkPlatform_IsRunning()) {
        if (opts.headless) {
            MjkPlatform_PollInputs(&inputs, &debug_keys);
            render_hal_test_pattern(&frame_output, simulated_frames);
            simulated_frames++;
            if (opts.max_frames > 0u && simulated_frames >= opts.max_frames) {
                MjkPlatform_RequestQuit();
            }
            continue;
        }

        uint64_t current_time = MjkPlatform_GetTimeMicroseconds();
        double delta_time = (double)(current_time - previous_time) / 1000000.0;
        previous_time = current_time;
        if (delta_time > 0.1) {
            delta_time = 0.1;
        }
        accumulator += delta_time;

        MjkPlatform_PollInputs(&inputs, &debug_keys);

        while (accumulator >= seconds_per_frame) {
            render_hal_test_pattern(&frame_output, simulated_frames);
            MjkPlatform_QueueAudioSamples(frame_output.audio_samples, frame_output.audio_sample_count);
            accumulator -= seconds_per_frame;
            simulated_frames++;

            if (opts.max_frames > 0u && simulated_frames >= opts.max_frames) {
                MjkPlatform_RequestQuit();
                break;
            }
        }

        MjkPlatform_PresentFramebuffer(frame_output.framebuffer);
        MjkPlatform_SleepMilliseconds(1u);
    }

    MjkPlatform_Shutdown();
    printf("[OK] %u frame(s) a %.4f Hz completados.\n", (unsigned)simulated_frames, MJK_NTSC_FPS);
    return 0;
}
