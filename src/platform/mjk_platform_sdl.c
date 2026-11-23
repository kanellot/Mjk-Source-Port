/**
 * @file mjk_platform_sdl.c
 * @brief Implementación HAL multiplataforma con SDL2 (Windows y Linux).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Aísla completamente SDL2 del núcleo C11 (`src/core/`). Provee ventana con
 * relación lógica 256x240, audio PCM mono 44.1 kHz con pre-buffering, soporte
 * de teclado/mando con hotplug, resolución de rutas portables (`SDL_GetBasePath`)
 * y modo headless para pruebas deterministas (`ctest --smoke-test`).
 */

#include "mjk_platform.h"
#include <SDL.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define MJK_AUDIO_PREBUFFER_BYTES    4096u
#define MJK_AUDIO_MAX_LATENCY_BYTES  ((MJK_AUDIO_SAMPLE_RATE * (uint32_t)sizeof(int16_t) * 12u) / 100u)

static SDL_Window*         g_window = NULL;
static SDL_Renderer*       g_renderer = NULL;
static SDL_Texture*        g_texture = NULL;
static SDL_AudioDeviceID   g_audio_dev = 0;
static SDL_GameController* g_controller = NULL;

static bool g_initialized   = false;
static bool g_headless      = false;
static bool g_audio_started = false;
static bool g_is_running    = false;
static bool g_is_fullscreen = false;

static void platform_open_first_controller(void) {
    if (g_controller) {
        return;
    }
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; ++i) {
        if (SDL_IsGameController(i)) {
            g_controller = SDL_GameControllerOpen(i);
            if (g_controller) {
                printf("[SDL2]: Mando detectado: %s\n", SDL_GameControllerName(g_controller));
                break;
            }
        }
    }
}

bool MjkPlatform_Init(const MjkPlatformConfig* config) {
    if (g_initialized) {
        return true;
    }

    g_headless = (config && config->headless);
    g_is_fullscreen = false;
    g_audio_started = false;

    if (g_headless) {
        if (SDL_Init(SDL_INIT_TIMER) < 0) {
            fprintf(stderr, "[ERROR SDL2]: Fallo al inicializar SDL_INIT_TIMER: %s\n", SDL_GetError());
            return false;
        }
        g_initialized = true;
        g_is_running = true;
        return true;
    }

    const bool want_audio = (!config || config->enable_audio);
    Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | (want_audio ? SDL_INIT_AUDIO : 0u);
    if (SDL_Init(sdl_flags) < 0) {
        fprintf(stderr, "[ERROR SDL2]: Fallo al inicializar SDL2: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    const char* title = (config && config->window_title)
        ? config->window_title
        : "Mighty Bomb Jack (NES 1987) - Source Port C11";
    int scale = (config && config->window_scale > 0) ? config->window_scale : 3;

    g_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        MJK_SCREEN_WIDTH * scale,
        MJK_SCREEN_HEIGHT * scale,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!g_window) {
        fprintf(stderr, "[ERROR SDL2]: No se pudo crear la ventana: %s\n", SDL_GetError());
        MjkPlatform_Shutdown();
        return false;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!g_renderer) {
        fprintf(stderr, "[ERROR SDL2]: No se pudo crear el renderizador: %s\n", SDL_GetError());
        MjkPlatform_Shutdown();
        return false;
    }

    SDL_RenderSetLogicalSize(g_renderer, MJK_SCREEN_WIDTH, MJK_SCREEN_HEIGHT);

    g_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        MJK_SCREEN_WIDTH,
        MJK_SCREEN_HEIGHT
    );
    if (!g_texture) {
        fprintf(stderr, "[ERROR SDL2]: Fallo al crear textura 256x240: %s\n", SDL_GetError());
        MjkPlatform_Shutdown();
        return false;
    }

    if (want_audio) {
        SDL_AudioSpec wanted;
        SDL_zero(wanted);
        wanted.freq = MJK_AUDIO_SAMPLE_RATE;
        wanted.format = AUDIO_S16SYS;
        wanted.channels = 1;
        wanted.samples = MJK_AUDIO_BUFFER_SAMPLES;

        g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
        if (g_audio_dev > 0) {
            SDL_PauseAudioDevice(g_audio_dev, 1);
        } else {
            fprintf(stderr, "[AVISO SDL2]: Sin dispositivo de audio (%s). Modo silencioso.\n", SDL_GetError());
        }
    }

    platform_open_first_controller();

    g_initialized = true;
    g_is_running = true;
    return true;
}

void MjkPlatform_Shutdown(void) {
    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = NULL;
    }
    if (g_audio_dev > 0) {
        SDL_CloseAudioDevice(g_audio_dev);
        g_audio_dev = 0;
    }
    if (g_texture) {
        SDL_DestroyTexture(g_texture);
        g_texture = NULL;
    }
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    if (g_initialized) {
        SDL_Quit();
        g_initialized = false;
    }
    g_headless = false;
    g_audio_started = false;
    g_is_running = false;
}

bool MjkPlatform_IsRunning(void) {
    return g_is_running;
}

void MjkPlatform_RequestQuit(void) {
    g_is_running = false;
}

uint64_t MjkPlatform_GetTimeMicroseconds(void) {
    static uint64_t freq = 0u;
    if (freq == 0u) {
        freq = SDL_GetPerformanceFrequency();
    }
    if (freq == 0u) {
        return (uint64_t)SDL_GetTicks64() * 1000ULL;
    }
    uint64_t counter = SDL_GetPerformanceCounter();
    return (counter / freq) * 1000000ULL + ((counter % freq) * 1000000ULL) / freq;
}

void MjkPlatform_SleepMilliseconds(uint32_t ms) {
    SDL_Delay((Uint32)ms);
}

static uint32_t platform_map_debug_key(SDL_Keycode sym) {
    switch (sym) {
        case SDLK_F1:           return MJK_DBG_TOGGLE_OVERLAY;
        case SDLK_F2:           return MJK_DBG_TOGGLE_WARP;
        case SDLK_F3:           return MJK_DBG_CYCLE_CHEATS;
        case SDLK_F4:           return MJK_DBG_TOGGLE_PAUSE;
        case SDLK_F5:           return MJK_DBG_STEP_FRAME;
        case SDLK_LEFTBRACKET:  return MJK_DBG_WARP_PREV;
        case SDLK_RIGHTBRACKET: return MJK_DBG_WARP_NEXT;
        case SDLK_F6:           return MJK_DBG_WARP_CONFIRM;
        default:                return MJK_DBG_NONE;
    }
}

void MjkPlatform_PollInputs(MjkInputState* out_inputs, uint32_t* out_debug_keys) {
    uint32_t debug_keys = MJK_DBG_NONE;

    if (g_headless) {
        if (out_inputs) {
            MjkInput_Update(out_inputs, 0u);
        }
        if (out_debug_keys) {
            *out_debug_keys = MJK_DBG_NONE;
        }
        return;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                g_is_running = false;
                break;

            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_F11 && !e.key.repeat) {
                    g_is_fullscreen = !g_is_fullscreen;
                    SDL_SetWindowFullscreen(g_window, g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0u);
                } else if (e.key.keysym.sym == SDLK_ESCAPE && !e.key.repeat) {
                    g_is_running = false;
                } else if (!e.key.repeat) {
                    debug_keys |= platform_map_debug_key(e.key.keysym.sym);
                }
                break;

            case SDL_CONTROLLERDEVICEADDED:
                if (!g_controller) {
                    g_controller = SDL_GameControllerOpen(e.cdevice.which);
                }
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                if (g_controller) {
                    SDL_Joystick* joy = SDL_GameControllerGetJoystick(g_controller);
                    if (joy && e.cdevice.which == (Sint32)SDL_JoystickInstanceID(joy)) {
                        SDL_GameControllerClose(g_controller);
                        g_controller = NULL;
                    }
                }
                break;

            default:
                break;
        }
    }

    uint8_t buttons = 0u;
    const Uint8* kb = SDL_GetKeyboardState(NULL);
    if (kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D]) buttons |= MJK_BTN_RIGHT;
    if (kb[SDL_SCANCODE_LEFT]  || kb[SDL_SCANCODE_A]) buttons |= MJK_BTN_LEFT;
    if (kb[SDL_SCANCODE_DOWN]  || kb[SDL_SCANCODE_S]) buttons |= MJK_BTN_DOWN;
    if (kb[SDL_SCANCODE_UP]    || kb[SDL_SCANCODE_W]) buttons |= MJK_BTN_UP;
    if (kb[SDL_SCANCODE_RETURN] || kb[SDL_SCANCODE_KP_ENTER]) buttons |= MJK_BTN_START;
    if (kb[SDL_SCANCODE_RSHIFT] || kb[SDL_SCANCODE_TAB])      buttons |= MJK_BTN_SELECT;
    if (kb[SDL_SCANCODE_X]      || kb[SDL_SCANCODE_K])        buttons |= MJK_BTN_B;
    if (kb[SDL_SCANCODE_SPACE]  || kb[SDL_SCANCODE_Z] || kb[SDL_SCANCODE_J]) buttons |= MJK_BTN_A;

    if (g_controller) {
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) buttons |= MJK_BTN_RIGHT;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  buttons |= MJK_BTN_LEFT;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  buttons |= MJK_BTN_DOWN;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    buttons |= MJK_BTN_UP;

        Sint16 stick_x = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 stick_y = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (stick_x >  8000) buttons |= MJK_BTN_RIGHT;
        if (stick_x < -8000) buttons |= MJK_BTN_LEFT;
        if (stick_y >  8000) buttons |= MJK_BTN_DOWN;
        if (stick_y < -8000) buttons |= MJK_BTN_UP;

        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_START)) buttons |= MJK_BTN_START;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_BACK))  buttons |= MJK_BTN_SELECT;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_B))     buttons |= MJK_BTN_B;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_A))     buttons |= MJK_BTN_A;
    }

    if (out_inputs) {
        MjkInput_Update(out_inputs, buttons);
    }
    if (out_debug_keys) {
        *out_debug_keys = debug_keys;
    }
}

void MjkPlatform_PresentFramebuffer(const uint32_t* framebuffer) {
    if (g_headless || !g_texture || !g_renderer || !framebuffer) {
        return;
    }

    SDL_UpdateTexture(g_texture, NULL, framebuffer, MJK_SCREEN_WIDTH * (int)sizeof(uint32_t));
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

void MjkPlatform_QueueAudioSamples(const int16_t* samples, uint32_t count) {
    if (g_headless || g_audio_dev == 0 || !samples || count == 0u) {
        return;
    }

    Uint32 queued_bytes = SDL_GetQueuedAudioSize(g_audio_dev);
    if (queued_bytes > MJK_AUDIO_MAX_LATENCY_BYTES) {
        return;
    }

    SDL_QueueAudio(g_audio_dev, samples, (Uint32)(count * sizeof(int16_t)));

    if (!g_audio_started) {
        queued_bytes = SDL_GetQueuedAudioSize(g_audio_dev);
        if (queued_bytes >= MJK_AUDIO_PREBUFFER_BYTES) {
            SDL_PauseAudioDevice(g_audio_dev, 0);
            g_audio_started = true;
        }
    }
}

bool MjkPlatform_GetExecutableDir(char* out_buf, size_t buf_size) {
    if (!out_buf || buf_size == 0u) {
        return false;
    }
    out_buf[0] = '\0';

    char* base_path = SDL_GetBasePath();
    if (!base_path) {
        return false;
    }

    size_t len = strlen(base_path);
    bool fits = (len < buf_size);
    size_t copy_len = fits ? len : (buf_size - 1u);
    if (copy_len > 0u) {
        memcpy(out_buf, base_path, copy_len);
    }
    out_buf[copy_len] = '\0';
    SDL_free(base_path);
    return fits;
}

static bool platform_mkdir_single(const char* path) {
    if (!path || path[0] == '\0' || strcmp(path, ".") == 0) {
        return true;
    }
#ifdef _WIN32
    if (_mkdir(path) == 0 || errno == EEXIST) {
        return true;
    }
#else
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return true;
    }
#endif
    return false;
}

bool MjkPlatform_EnsureDir(const char* dir_path) {
    if (!dir_path || dir_path[0] == '\0' || strcmp(dir_path, ".") == 0) {
        return true;
    }

    char buf[1024];
    size_t len = strlen(dir_path);
    if (len >= sizeof(buf)) {
        return false;
    }
    memcpy(buf, dir_path, len + 1u);

    while (len > 1u && (buf[len - 1u] == '/' || buf[len - 1u] == '\\')) {
        if (len == 3u && buf[1] == ':') {
            break;
        }
        buf[--len] = '\0';
    }

    size_t start = 0u;
    if (len >= 2u && buf[1] == ':') {
        start = 2u;
        if (buf[start] == '/' || buf[start] == '\\') {
            start = 3u;
        }
    } else if (buf[0] == '/' || buf[0] == '\\') {
        start = 1u;
    }

    for (size_t i = start; i <= len; ++i) {
        if (buf[i] == '/' || buf[i] == '\\' || buf[i] == '\0') {
            char saved = buf[i];
            buf[i] = '\0';
            if (!platform_mkdir_single(buf)) {
                return false;
            }
            buf[i] = saved;
        }
    }
    return true;
}

void MjkPlatform_ShowMessageBox(const char* title, const char* message, bool is_error) {
    const char* safe_title = title ? title : "Mighty Bomb Jack";
    const char* safe_msg = message ? message : "";

    if (g_headless) {
        fprintf(stderr, "[%s] %s: %s\n", is_error ? "ERROR" : "INFO", safe_title, safe_msg);
        return;
    }

    Uint32 flags = is_error ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_INFORMATION;
    if (SDL_ShowSimpleMessageBox(flags, safe_title, safe_msg, g_window) != 0) {
        fprintf(stderr, "[%s] %s: %s\n", is_error ? "ERROR" : "INFO", safe_title, safe_msg);
    }
}
