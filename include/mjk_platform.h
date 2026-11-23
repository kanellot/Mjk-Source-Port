/**
 * @file mjk_platform.h
 * @brief Capa de Abstracción de Hardware (HAL) multiplataforma (sin dependencias de SDL/SO).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * El código en `src/core/` usa únicamente este contrato C11 para leer entradas,
 * emitir el framebuffer 256x240 y generar muestras PCM a 44.1 kHz, mientras que
 * `src/platform/mjk_platform_sdl.c` implementa la capa SDL2 real para Windows y Linux.
 */

#ifndef MJK_PLATFORM_H
#define MJK_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MJK_SCREEN_WIDTH          256
#define MJK_SCREEN_HEIGHT         240
#define MJK_NTSC_FPS              60.0988138974405
#define MJK_AUDIO_SAMPLE_RATE     44100
#define MJK_AUDIO_BUFFER_SAMPLES  1024

/* ============================================================================
 * 1. MÁSCARA DE MANDO NES (asm/src/defs.asm:43-50)
 * ============================================================================
 * Al desplazarse por el registro serie $4016 (con `ROR` en `ReadJoypad`), los
 * 8 botones quedan exactamente en este orden de bit dentro del byte de entrada:
 *   bit 0 = Right, 1 = Left, 2 = Down, 3 = Up, 4 = Start, 5 = Select, 6 = B, 7 = A.
 */

typedef enum {
    MJK_BTN_RIGHT  = 0x01u,
    MJK_BTN_LEFT   = 0x02u,
    MJK_BTN_DOWN   = 0x04u,
    MJK_BTN_UP     = 0x08u,
    MJK_BTN_START  = 0x10u,
    MJK_BTN_SELECT = 0x20u,
    MJK_BTN_B      = 0x40u,
    MJK_BTN_A      = 0x80u
} MjkButtonMask;

typedef struct {
    uint8_t held;           /**< Botones mantenidos en este frame. */
    uint8_t just_pressed;   /**< Flanco de subida (0 -> 1 en este frame). */
    uint8_t just_released;  /**< Flanco de bajada (1 -> 0 en este frame). */
} MjkInputState;

/** @brief Actualiza estado mantenido y flancos a partir de la lectura cruda de botones. */
static inline void MjkInput_Update(MjkInputState* state, uint8_t raw_buttons) {
    if (!state) {
        return;
    }
    uint8_t prev = state->held;
    state->just_pressed  = (uint8_t)(raw_buttons & (uint8_t)(~prev));
    state->just_released = (uint8_t)((uint8_t)(~raw_buttons) & prev);
    state->held          = raw_buttons;
}

/** @brief Teclas de Capa 3 (Debug Overlay SDL2), separadas del mando NES. */
typedef enum {
    MJK_DBG_NONE           = 0x00u,
    MJK_DBG_TOGGLE_OVERLAY = 0x01u, /**< F1: Mostrar/ocultar telemetría y hitboxes 9-probe. */
    MJK_DBG_TOGGLE_WARP    = 0x02u, /**< F2: Selector directo de sala/Bomb/Torture/Final. */
    MJK_DBG_CYCLE_CHEATS   = 0x04u, /**< F3: Modificadores de prueba (invencibilidad / Mighty). */
    MJK_DBG_TOGGLE_PAUSE   = 0x08u, /**< F4: Pausa / reanudar ejecución paso a paso. */
    MJK_DBG_STEP_FRAME     = 0x10u, /**< F5: Avanzar 1 frame en pausa. */
    MJK_DBG_WARP_PREV      = 0x20u, /**< '[': Sala destino anterior en selector Warp. */
    MJK_DBG_WARP_NEXT      = 0x40u, /**< ']': Sala destino siguiente en selector Warp. */
    MJK_DBG_WARP_CONFIRM   = 0x80u  /**< F6: Ejecutar salto inmediato a la sala elegida. */
} MjkDebugKeyMask;

/* ============================================================================
 * 2. SALIDA DE FRAME E INICIALIZACIÓN HAL
 * ============================================================================ */

typedef struct {
    uint32_t framebuffer[MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT]; /**< ARGB8888 (0xAARRGGBB). */
    int16_t  audio_samples[MJK_AUDIO_BUFFER_SAMPLES];           /**< PCM 16-bit mono 44.1 kHz. */
    uint32_t audio_sample_count;
} MjkFrameOutput;

typedef struct {
    const char* window_title;
    int         window_scale;
    bool        headless;
    bool        enable_audio;
} MjkPlatformConfig;

bool     MjkPlatform_Init(const MjkPlatformConfig* config);
void     MjkPlatform_Shutdown(void);
bool     MjkPlatform_IsRunning(void);
void     MjkPlatform_RequestQuit(void);

uint64_t MjkPlatform_GetTimeMicroseconds(void);
void     MjkPlatform_SleepMilliseconds(uint32_t ms);

void     MjkPlatform_PollInputs(MjkInputState* out_inputs, uint32_t* out_debug_keys);
void     MjkPlatform_PresentFramebuffer(const uint32_t* framebuffer);
void     MjkPlatform_QueueAudioSamples(const int16_t* samples, uint32_t count);

bool     MjkPlatform_GetExecutableDir(char* out_buf, size_t buf_size);
void     MjkPlatform_ShowMessageBox(const char* title, const char* message, bool is_error);

#endif /* MJK_PLATFORM_H */
