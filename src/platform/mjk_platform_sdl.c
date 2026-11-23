/**
 * @file mjk_platform_sdl.c
 * @brief Implementación unificada multiplataforma con SDL2 (Windows & Linux).
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * COMPATIBILIDAD: Windows 10/11, Linux (X11/Wayland), macOS.
 */

#include "mjk_platform.h"
#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>

static SDL_Window*         g_window = NULL;
static SDL_Renderer*       g_renderer = NULL;
static SDL_Texture*        g_texture = NULL;
static SDL_AudioDeviceID   g_audio_dev = 0;
static bool                s_audio_started = false;
static SDL_GameController* g_controller = NULL;
static bool                g_is_running = true;
static bool                g_is_fullscreen = false;

bool Platform_Init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "[ERROR SDL2]: Fallo al inicializar SDL2: %s\n", SDL_GetError());
        return false;
    }

    // Escalar con aspecto retro nítido (Nearest Neighbor)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // Ventana a escala 3x (768x720) redimensionable
    g_window = SDL_CreateWindow(
        "Mighty Bomb Jack (NES 1987) - Source Port Multiplataforma",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        MJK_SCREEN_WIDTH * 3,
        MJK_SCREEN_HEIGHT * 3,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!g_window) {
        fprintf(stderr, "[ERROR SDL2]: No se pudo crear la ventana: %s\n", SDL_GetError());
        return false;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        // Fallback a software renderer si la GPU no responde
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!g_renderer) {
        fprintf(stderr, "[ERROR SDL2]: No se pudo crear el renderizador: %s\n", SDL_GetError());
        return false;
    }

    // Fijar resolución lógica 256x240 (SDL mantiene el aspect ratio 4:3 con bandas negras automáticamente)
    SDL_RenderSetLogicalSize(g_renderer, MJK_SCREEN_WIDTH, MJK_SCREEN_HEIGHT);

    // Textura RGBA32 de 256x240 en memoria de vídeo
    g_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        MJK_SCREEN_WIDTH,
        MJK_SCREEN_HEIGHT
    );

    if (!g_texture) {
        fprintf(stderr, "[ERROR SDL2]: Fallo al crear textura de vídeo: %s\n", SDL_GetError());
        return false;
    }

    // Inicializar dispositivo de audio PCM a 44.1 kHz Mono
    SDL_AudioSpec wanted;
    SDL_zero(wanted);
    wanted.freq = MJK_AUDIO_SAMPLE_RATE;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 1;
    wanted.samples = 1024;

    g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
    if (g_audio_dev > 0) {
        // Mantener en pausa hasta acumular colchón inicial (pre-buffering suave)
        SDL_PauseAudioDevice(g_audio_dev, 1);
        s_audio_started = false;
    } else {
        fprintf(stderr, "[AVISO SDL2]: No se pudo abrir audio: %s. Continuando en silencio.\n", SDL_GetError());
    }

    // Abrir primer mando conectado si existe (Xbox Series, PS4/PS5, Switch)
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            g_controller = SDL_GameControllerOpen(i);
            if (g_controller) {
                printf("[SDL2]: Mando detectado: %s\n", SDL_GameControllerName(g_controller));
                break;
            }
        }
    }

    g_is_running = true;
    return true;
}

void Platform_Shutdown(void) {
    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = NULL;
    }
    if (g_audio_dev > 0) {
        SDL_CloseAudioDevice(g_audio_dev);
        g_audio_dev = 0;
        s_audio_started = false;
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
    SDL_Quit();
}

uint64_t Platform_GetTimeMicroseconds(void) {
    static uint64_t freq = 0;
    if (freq == 0) {
        freq = SDL_GetPerformanceFrequency();
    }
    return (SDL_GetPerformanceCounter() * 1000000ULL) / freq;
}

void Platform_PollInputs(MjkInputState* out_inputs) {
    if (!out_inputs) return;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                g_is_running = false;
                break;

            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_F11) {
                    g_is_fullscreen = !g_is_fullscreen;
                    SDL_SetWindowFullscreen(g_window, g_is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    g_is_running = false;
                }
                break;

            case SDL_CONTROLLERDEVICEADDED:
                if (!g_controller) {
                    g_controller = SDL_GameControllerOpen(e.cdevice.which);
                }
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                if (g_controller && e.cdevice.which == (Sint32)SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(g_controller))) {
                    SDL_GameControllerClose(g_controller);
                    g_controller = NULL;
                }
                break;
        }
    }

    uint8_t buttons = 0;

    // 1. Sondeo de teclado
    const Uint8* kb = SDL_GetKeyboardState(NULL);
    if (kb[SDL_SCANCODE_UP]    || kb[SDL_SCANCODE_W]) buttons |= MJK_BTN_UP;
    if (kb[SDL_SCANCODE_DOWN]  || kb[SDL_SCANCODE_S]) buttons |= MJK_BTN_DOWN;
    if (kb[SDL_SCANCODE_LEFT]  || kb[SDL_SCANCODE_A]) buttons |= MJK_BTN_LEFT;
    if (kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D]) buttons |= MJK_BTN_RIGHT;

    if (kb[SDL_SCANCODE_SPACE]  || kb[SDL_SCANCODE_Z] || kb[SDL_SCANCODE_J]) buttons |= MJK_BTN_A;
    if (kb[SDL_SCANCODE_X]      || kb[SDL_SCANCODE_K])                       buttons |= MJK_BTN_B;
    if (kb[SDL_SCANCODE_RETURN] || kb[SDL_SCANCODE_KP_ENTER])                 buttons |= MJK_BTN_START;
    if (kb[SDL_SCANCODE_RSHIFT] || kb[SDL_SCANCODE_TAB])                      buttons |= MJK_BTN_SELECT;

    // 2. Sondeo de Gamepad (Xbox Series / PS4 / Switch)
    if (g_controller) {
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    buttons |= MJK_BTN_UP;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  buttons |= MJK_BTN_DOWN;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  buttons |= MJK_BTN_LEFT;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) buttons |= MJK_BTN_RIGHT;

        // Stick analógico izquierdo con zona muerta
        Sint16 stick_x = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 stick_y = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (stick_x < -8000) buttons |= MJK_BTN_LEFT;
        if (stick_x >  8000) buttons |= MJK_BTN_RIGHT;
        if (stick_y < -8000) buttons |= MJK_BTN_UP;
        if (stick_y >  8000) buttons |= MJK_BTN_DOWN;

        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_A))     buttons |= MJK_BTN_A;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_B))     buttons |= MJK_BTN_B;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_START)) buttons |= MJK_BTN_START;
        if (SDL_GameControllerGetButton(g_controller, SDL_CONTROLLER_BUTTON_BACK))  buttons |= MJK_BTN_SELECT;
    }

    out_inputs->held = buttons;
}

void Platform_PresentFramebuffer(const uint32_t* framebuffer) {
    if (!g_texture || !g_renderer || !framebuffer) return;

    SDL_UpdateTexture(g_texture, NULL, framebuffer, MJK_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

#define MJK_AUDIO_PREBUFFER_BYTES 4096 // Colchón inicial de 4096 bytes (~2048 muestras / ~46 ms a 44.1 kHz mono 16-bit)
#define MJK_AUDIO_MAX_LATENCY_BYTES ((MJK_AUDIO_SAMPLE_RATE * (uint32_t)sizeof(int16_t) * 12) / 100) // ~120 ms

void Platform_QueueAudioSamples(const int16_t* samples, uint32_t count) {
    if (g_audio_dev == 0 || !samples || count == 0) return;

    Uint32 queued_bytes = SDL_GetQueuedAudioSize(g_audio_dev);

    // Si la cola supera el límite de latencia admisible (~120 ms), descartar para evitar desincronización
    if (queued_bytes > MJK_AUDIO_MAX_LATENCY_BYTES) {
        return;
    }

    // Encolar las muestras simuladas del frame
    SDL_QueueAudio(g_audio_dev, samples, count * sizeof(int16_t));

    // Pre-buffering suave: al arrancar el dispositivo, mantenerlo en pausa
    // hasta haber acumulado el colchón inicial (4096 bytes / ~46 ms) y solo entonces despausar.
    // Esto asegura flujo continuo sin underrun inicial. Si se produce un underrun
    // posterior puntual, SDL2 reproduce silencio nativo sin inyectar bloques abruptos.
    if (!s_audio_started) {
        queued_bytes = SDL_GetQueuedAudioSize(g_audio_dev);
        if (queued_bytes >= MJK_AUDIO_PREBUFFER_BYTES) {
            SDL_PauseAudioDevice(g_audio_dev, 0);
            s_audio_started = true;
        }
    }
}

bool Platform_IsRunning(void) {
    return g_is_running;
}
