/**
 * @file platform_sdl.c
 * @brief Capa de plataforma limpia sobre SDL2 (ventana 256x240, audio 44.1 kHz, mando/teclado).
 */

#include "mjk/platform.h"
#include "mjk/input.h"
#include "mjk/types.h"
#include <SDL.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define AUDIO_PREBUFFER_BYTES   4096u
#define AUDIO_MAX_LATENCY_BYTES ((MJK_AUDIO_SAMPLE_RATE * (uint32_t)sizeof(int16_t) * 12u) / 100u)

static SDL_Window*         g_window        = NULL;
static SDL_Renderer*       g_renderer      = NULL;
static SDL_Texture*        g_texture       = NULL;
static SDL_AudioDeviceID   g_audio_dev     = 0;
static SDL_GameController* g_controller    = NULL;
static bool                g_initialized   = false;
static bool                g_audio_started = false;
static bool                g_is_running    = false;
static bool                g_is_fullscreen = false;

static void open_first_controller(void) {
    if (g_controller) { return; }
    int count = SDL_NumJoysticks();
    for (int i = 0; i < count; ++i) {
        if (SDL_IsGameController(i)) {
            g_controller = SDL_GameControllerOpen(i);
            if (g_controller) { break; }
        }
    }
}

bool MjkPlatform_Init(const MjkPlatformConfig* config) {
    if (g_initialized) { return true; }

    g_is_fullscreen = false;
    g_audio_started = false;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "[ERROR SDL2]: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    const char* title = (config && config->window_title)
        ? config->window_title
        : "Mighty Bomb Jack (NES 1987) - C11";
    int scale = (config && config->window_scale > 0) ? config->window_scale : 3;

    g_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                MJK_SCREEN_WIDTH * scale, MJK_SCREEN_HEIGHT * scale,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!g_window) {
        fprintf(stderr, "[ERROR SDL2]: %s\n", SDL_GetError());
        MjkPlatform_Shutdown();
        return false;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!g_renderer) {
        fprintf(stderr, "[ERROR SDL2]: %s\n", SDL_GetError());
        MjkPlatform_Shutdown();
        return false;
    }

    SDL_RenderSetLogicalSize(g_renderer, MJK_SCREEN_WIDTH, MJK_SCREEN_HEIGHT);
    g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  MJK_SCREEN_WIDTH, MJK_SCREEN_HEIGHT);
    if (!g_texture) {
        fprintf(stderr, "[ERROR SDL2]: %s\n", SDL_GetError());
        MjkPlatform_Shutdown();
        return false;
    }

    SDL_AudioSpec wanted;
    SDL_zero(wanted);
    wanted.freq     = (int)MJK_AUDIO_SAMPLE_RATE;
    wanted.format   = AUDIO_S16SYS;
    wanted.channels = 1;
    wanted.samples  = 1024;
    g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
    if (g_audio_dev > 0) {
        SDL_PauseAudioDevice(g_audio_dev, 1);
    }

    open_first_controller();
    g_initialized = true;
    g_is_running  = true;
    return true;
}

void MjkPlatform_Shutdown(void) {
    if (g_controller)  { SDL_GameControllerClose(g_controller); g_controller = NULL; }
    if (g_audio_dev)   { SDL_CloseAudioDevice(g_audio_dev);     g_audio_dev = 0; }
    if (g_texture)     { SDL_DestroyTexture(g_texture);         g_texture = NULL; }
    if (g_renderer)    { SDL_DestroyRenderer(g_renderer);       g_renderer = NULL; }
    if (g_window)      { SDL_DestroyWindow(g_window);           g_window = NULL; }
    if (g_initialized) { SDL_Quit();                            g_initialized = false; }
    g_audio_started = false;
    g_is_running    = false;
}

bool MjkPlatform_IsRunning(void) {
    return g_is_running;
}

uint64_t MjkPlatform_GetTimeMicroseconds(void) {
    static uint64_t freq = 0u;
    if (freq == 0u) { freq = SDL_GetPerformanceFrequency(); }
    if (freq == 0u) { return (uint64_t)SDL_GetTicks64() * 1000ULL; }
    uint64_t c = SDL_GetPerformanceCounter();
    return (c / freq) * 1000000ULL + ((c % freq) * 1000000ULL) / freq;
}

void MjkPlatform_SleepMilliseconds(uint32_t ms) {
    SDL_Delay((Uint32)ms);
}

uint8_t MjkPlatform_PollRawButtons(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_is_running = false;
        } else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            if (e.key.keysym.sym == SDLK_F11) {
                g_is_fullscreen = !g_is_fullscreen;
                SDL_SetWindowFullscreen(g_window, g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0u);
            } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                g_is_running = false;
            }
        } else if (e.type == SDL_CONTROLLERDEVICEADDED && !g_controller) {
            open_first_controller();
        } else if (e.type == SDL_CONTROLLERDEVICEREMOVED && g_controller) {
            SDL_Joystick* joy = SDL_GameControllerGetJoystick(g_controller);
            if (joy && SDL_JoystickInstanceID(joy) == e.cdevice.which) {
                SDL_GameControllerClose(g_controller);
                g_controller = NULL;
                open_first_controller();
            }
        }
    }

    uint8_t buttons = 0u;
    const Uint8* ks = SDL_GetKeyboardState(NULL);
    if (ks[SDL_SCANCODE_RIGHT]  || ks[SDL_SCANCODE_D])      { buttons |= MJK_INPUT_RIGHT; }
    if (ks[SDL_SCANCODE_LEFT]   || ks[SDL_SCANCODE_A])      { buttons |= MJK_INPUT_LEFT; }
    if (ks[SDL_SCANCODE_DOWN]   || ks[SDL_SCANCODE_S])      { buttons |= MJK_INPUT_DOWN; }
    if (ks[SDL_SCANCODE_UP]     || ks[SDL_SCANCODE_W])      { buttons |= MJK_INPUT_UP; }
    if (ks[SDL_SCANCODE_RETURN] || ks[SDL_SCANCODE_KP_ENTER]) { buttons |= MJK_INPUT_START; }
    if (ks[SDL_SCANCODE_RSHIFT] || ks[SDL_SCANCODE_TAB])    { buttons |= MJK_INPUT_SELECT; }
    if (ks[SDL_SCANCODE_Z]      || ks[SDL_SCANCODE_J] || ks[SDL_SCANCODE_SPACE]) { buttons |= MJK_INPUT_A; }
    if (ks[SDL_SCANCODE_X]      || ks[SDL_SCANCODE_K])      { buttons |= MJK_INPUT_B; }

    if (g_controller) {
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
            SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX) > 12000)  { buttons |= MJK_INPUT_RIGHT; }
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
            SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX) < -12000) { buttons |= MJK_INPUT_LEFT; }
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
            SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY) > 12000)  { buttons |= MJK_INPUT_DOWN; }
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
            SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY) < -12000) { buttons |= MJK_INPUT_UP; }
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_START))      { buttons |= MJK_INPUT_START; }
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_BACK))       { buttons |= MJK_INPUT_SELECT; }
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_A))          { buttons |= MJK_INPUT_A; }
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_B) ||
            SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_X))          { buttons |= MJK_INPUT_B; }
    }
    return buttons;
}

void MjkPlatform_PresentFramebuffer(const uint32_t* fb_argb8888) {
    if (!g_renderer || !g_texture || !fb_argb8888) { return; }
    SDL_UpdateTexture(g_texture, NULL, fb_argb8888, MJK_SCREEN_WIDTH * (int)sizeof(uint32_t));
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

void MjkPlatform_QueueAudioSamples(const int16_t* samples, uint32_t sample_count) {
    if (g_audio_dev == 0 || !samples || sample_count == 0u) { return; }
    Uint32 queued = SDL_GetQueuedAudioSize(g_audio_dev);
    if (queued > AUDIO_MAX_LATENCY_BYTES) {
        return;
    }
    SDL_QueueAudio(g_audio_dev, samples, sample_count * (uint32_t)sizeof(int16_t));
    if (!g_audio_started && (queued + sample_count * sizeof(int16_t)) >= AUDIO_PREBUFFER_BYTES) {
        SDL_PauseAudioDevice(g_audio_dev, 0);
        g_audio_started = true;
    }
}

bool MjkPlatform_GetExecutableDir(char* out_buf, size_t buf_size) {
    if (!out_buf || buf_size == 0u) { return false; }
    char* base = SDL_GetBasePath();
    if (!base) { out_buf[0] = '\0'; return false; }
    size_t len = strlen(base);
    if (len >= buf_size) { SDL_free(base); out_buf[0] = '\0'; return false; }
    memcpy(out_buf, base, len + 1u);
    SDL_free(base);
    return true;
}

bool MjkPlatform_EnsureDir(const char* dir_path) {
    if (!dir_path || dir_path[0] == '\0') { return false; }
    char buf[1024];
    size_t len = strlen(dir_path);
    if (len >= sizeof(buf)) { return false; }
    memcpy(buf, dir_path, len + 1u);

    for (size_t i = 1u; i <= len; ++i) {
        if (buf[i] == '/' || buf[i] == '\\' || buf[i] == '\0') {
            char saved = buf[i];
            buf[i] = '\0';
            if (!(i == 2u && buf[1] == ':')) {
#ifdef _WIN32
                int rc = _mkdir(buf);
#else
                int rc = mkdir(buf, 0755);
#endif
                if (rc != 0 && errno != EEXIST) {
                    buf[i] = saved;
                    return false;
                }
            }
            buf[i] = saved;
        }
    }
    return true;
}

void MjkPlatform_ShowMessageBox(const char* title, const char* message, bool is_error) {
    Uint32 flags = is_error ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_INFORMATION;
    SDL_ShowSimpleMessageBox(flags, title ? title : "Mighty Bomb Jack",
                             message ? message : "", g_window);
}
