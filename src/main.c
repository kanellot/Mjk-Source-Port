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
#include "mjk_assets.h"
#include "mjk_build_info.h"
#include "mjk_debug_overlay.h"
#include "mjk_game.h"
#include "mjk_platform.h"
#include "mjk_rom_extractor.h"

#define MJK_SAMPLES_PER_NTSC_FRAME 735u

typedef struct {
    const char* rom_path;
    const char* assets_path;
    bool        headless;
    bool        smoke_test;
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
    opts->assets_path = NULL;
    opts->headless = false;
    opts->smoke_test = false;
    opts->max_frames = 0u;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--headless") == 0) {
            opts->headless = true;
        } else if (strcmp(argv[i], "--smoke-test") == 0) {
            opts->headless = true;
            opts->smoke_test = true;
            opts->max_frames = 5u;
        } else if (strcmp(argv[i], "--rom") == 0) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "[ERROR CLI]: Falta ruta tras --rom.\n");
                return false;
            }
            opts->rom_path = argv[++i];
        } else if (strcmp(argv[i], "--assets") == 0) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "[ERROR CLI]: Falta ruta tras --assets.\n");
                return false;
            }
            opts->assets_path = argv[++i];
        } else if (strcmp(argv[i], "--frames") == 0) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "[ERROR CLI]: Falta número tras --frames.\n");
                return false;
            }
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

static bool join_path(char* out, size_t out_size, const char* dir, const char* rel) {
    if (!out || out_size == 0u || !dir || !rel) {
        return false;
    }
    size_t dir_len = strlen(dir);
    bool has_sep = (dir_len > 0u && (dir[dir_len - 1u] == '/' || dir[dir_len - 1u] == '\\'));
    int n = snprintf(out, out_size, has_sep ? "%s%s" : "%s/%s", dir, rel);
    return n >= 0 && (size_t)n < out_size;
}

static bool file_exists(const char* path) {
    if (!path || path[0] == '\0') {
        return false;
    }
    FILE* f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}

static bool ensure_parent_dir(const char* file_path) {
    if (!file_path) {
        return false;
    }
    const char* last_sep = NULL;
    for (const char* p = file_path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            last_sep = p;
        }
    }
    if (!last_sep) {
        return true;
    }
    size_t dir_len = (size_t)(last_sep - file_path);
    if (dir_len == 0u) {
        return true;
    }
    char dir_buf[1024];
    if (dir_len >= sizeof(dir_buf)) {
        return false;
    }
    memcpy(dir_buf, file_path, dir_len);
    dir_buf[dir_len] = '\0';
    return MjkPlatform_EnsureDir(dir_buf);
}

static bool load_or_extract_assets(const MjkCliOptions* opts,
                                   const char* exe_dir,
                                   MjkAssetPackage* out_assets) {
    char exe_assets_path[1024];
    MjkAssetError load_err = MJK_ASSET_ERR_IO;

    if (opts->assets_path) {
        load_err = MjkAssets_LoadFromFile(opts->assets_path, out_assets);
        if (load_err == MJK_ASSET_OK) {
            printf("  Assets   : %s (CRC-32 0x%08X)\n",
                   opts->assets_path, (unsigned)out_assets->payload_crc32);
            return true;
        }
    } else {
        load_err = MjkAssets_LoadFromFile("assets/assets.dat", out_assets);
        if (load_err == MJK_ASSET_OK) {
            printf("  Assets   : assets/assets.dat (CRC-32 0x%08X)\n",
                   (unsigned)out_assets->payload_crc32);
            return true;
        }
        if (exe_dir && join_path(exe_assets_path, sizeof(exe_assets_path), exe_dir, "assets/assets.dat")) {
            load_err = MjkAssets_LoadFromFile(exe_assets_path, out_assets);
            if (load_err == MJK_ASSET_OK) {
                printf("  Assets   : %s (CRC-32 0x%08X)\n",
                       exe_assets_path, (unsigned)out_assets->payload_crc32);
                return true;
            }
        }
    }

    const char* found_rom = NULL;
    char exe_rom_path[1024];
    if (opts->rom_path) {
        if (file_exists(opts->rom_path)) {
            found_rom = opts->rom_path;
        }
    } else if (file_exists("mbj-us.nes")) {
        found_rom = "mbj-us.nes";
    } else if (exe_dir && join_path(exe_rom_path, sizeof(exe_rom_path), exe_dir, "mbj-us.nes") &&
               file_exists(exe_rom_path)) {
        found_rom = exe_rom_path;
    }

    if (!found_rom) {
        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "No se encontró assets.dat válido (%s) ni la ROM mbj-us.nes.\n"
                 "Proporciona --rom <ruta_mbj-us.nes> o coloca mbj-us.nes junto al ejecutable.\n"
                 "SHA-1 esperado: %s",
                 MjkAssets_ErrorString(load_err), MJK_ROM_EXPECTED_SHA1);
        fprintf(stderr, "[ERROR BYOR]: %s\n", msg);
        MjkPlatform_ShowMessageBox("Mighty Bomb Jack - Falta ROM", msg, true);
        return false;
    }

    const char* target_assets = opts->assets_path ? opts->assets_path : "assets/assets.dat";
    if (!ensure_parent_dir(target_assets)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "No se pudo crear el directorio padre para '%s'.", target_assets);
        fprintf(stderr, "[ERROR BYOR]: %s\n", msg);
        MjkPlatform_ShowMessageBox("Mighty Bomb Jack - Error de Directorio", msg, true);
        return false;
    }

    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    memset(sha1_hex, 0, sizeof(sha1_hex));
    MjkAssetError ext_err = MjkRom_ExtractFileToAssetsDat(found_rom, target_assets, sha1_hex);
    if (ext_err == MJK_ASSET_OK) {
        ext_err = MjkAssets_LoadFromFile(target_assets, out_assets);
    }

    if (ext_err != MJK_ASSET_OK) {
        char msg[1024];
        if (sha1_hex[0] != '\0' && strcmp(sha1_hex, MJK_ROM_EXPECTED_SHA1) != 0) {
            snprintf(msg, sizeof(msg),
                     "Error al extraer '%s': %s.\n"
                     "SHA-1 esperado: %s\n"
                     "SHA-1 obtenido: %s",
                     found_rom, MjkAssets_ErrorString(ext_err),
                     MJK_ROM_EXPECTED_SHA1, sha1_hex);
        } else {
            snprintf(msg, sizeof(msg),
                     "Error al extraer '%s' hacia '%s': %s.\n"
                     "SHA-1 esperado: %s",
                     found_rom, target_assets, MjkAssets_ErrorString(ext_err),
                     MJK_ROM_EXPECTED_SHA1);
        }
        fprintf(stderr, "[ERROR BYOR]: %s\n", msg);
        MjkPlatform_ShowMessageBox("Mighty Bomb Jack - Error BYOR", msg, true);
        return false;
    }

    printf("  BYOR v2  : Extraído %s <- %s (SHA-1 %s)\n",
           target_assets, found_rom, sha1_hex);
    return true;
}

static void render_game_frame(const MjkGame* game,
                              const MjkDebugOverlayState* dbg,
                              MjkFrameOutput* out) {
    MjkGame_Render(game, out->framebuffer);
    MjkDebugTelemetry tel;
    memset(&tel, 0, sizeof(tel));
    tel.room_id = game->current_room_id;
    tel.game_state = (uint8_t)game->state;
    tel.lives = game->lives;
    tel.mighty_level = (dbg->cheat_mode == 2u) ? 3u : game->mighty_level;
    tel.timer_bcd = game->stage_timer_bcd;
    tel.camera_x = game->ppu.scroll_x;
    tel.jack_x = (int16_t)game->jack_x;
    tel.jack_y = (int16_t)game->jack_y;
    tel.has_jack = (game->top_mode == MJK_MODE_INGAME);
    MjkDebugOverlay_Render(dbg, &tel, out->framebuffer);
    out->audio_sample_count = MJK_SAMPLES_PER_NTSC_FRAME;
    memset(out->audio_samples, 0, MJK_SAMPLES_PER_NTSC_FRAME * sizeof(int16_t));
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
    bool exe_dir_ok = MjkPlatform_GetExecutableDir(exe_dir, sizeof(exe_dir));
    if (!exe_dir_ok) {
        snprintf(exe_dir, sizeof(exe_dir), "(desconocido)");
    }

    printf("Mighty Bomb Jack C11 Port (%s, ABI v%u)\n",
           MjkCore_Version(), (unsigned)MjkCore_AbiVersion());
    printf("  Dir base : %s\n", exe_dir);
    printf("  Modo     : %s | ROM CLI: %s\n",
           opts.headless ? "headless" : "SDL2",
           opts.rom_path ? opts.rom_path : "(auto)");

    MjkAssetPackage assets;
    memset(&assets, 0, sizeof(assets));
    bool has_assets = false;
    if (!opts.smoke_test) {
        if (!load_or_extract_assets(&opts, exe_dir_ok ? exe_dir : NULL, &assets)) {
            MjkPlatform_Shutdown();
            return 1;
        }
        has_assets = true;
    }

    static MjkGame game;
    MjkDebugOverlayState dbg;
    MjkDebugOverlay_Init(&dbg);
    if (has_assets) {
        MjkGame_Init(&game, &assets);
    }

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
            if (has_assets) {
                MjkGame_StepFrame(&game, &inputs);
                render_game_frame(&game, &dbg, &frame_output);
            } else {
                render_hal_test_pattern(&frame_output, simulated_frames);
            }
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
        if (has_assets) {
            uint8_t warp_target = 0u;
            if (MjkDebugOverlay_HandleKeys(&dbg, debug_keys, &warp_target)) {
                MjkGame_WarpToRoom(&game, warp_target);
            }
        }

        while (accumulator >= seconds_per_frame) {
            if (has_assets) {
                if (!dbg.paused || dbg.step_requested) {
                    MjkGame_StepFrame(&game, &inputs);
                    dbg.step_requested = false;
                    simulated_frames++;
                }
                render_game_frame(&game, &dbg, &frame_output);
            } else {
                render_hal_test_pattern(&frame_output, simulated_frames);
                simulated_frames++;
            }
            MjkPlatform_QueueAudioSamples(frame_output.audio_samples, frame_output.audio_sample_count);
            accumulator -= seconds_per_frame;

            if (opts.max_frames > 0u && simulated_frames >= opts.max_frames) {
                MjkPlatform_RequestQuit();
                break;
            }
        }

        MjkPlatform_PresentFramebuffer(frame_output.framebuffer);
        MjkPlatform_SleepMilliseconds(1u);
    }

    MjkAssets_Unload(&assets);
    MjkPlatform_Shutdown();
    printf("[OK] %u frame(s) a %.4f Hz completados.\n", (unsigned)simulated_frames, MJK_NTSC_FPS);
    return 0;
}
