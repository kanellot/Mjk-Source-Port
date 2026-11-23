/**
 * @file mjk_platform.h
 * @brief Capa de Abstracción de Hardware (HAL) y contratos de plataforma.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FILOSOFÍA: Desacoplamiento total entre el núcleo de simulación y el frontend (SDL2/SDL3/Win32)
 */

#ifndef MJK_PLATFORM_H
#define MJK_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#define MJK_SCREEN_WIDTH          256
#define MJK_SCREEN_HEIGHT         240
#define MJK_AUDIO_SAMPLE_RATE     44100
#define MJK_AUDIO_BUFFER_SAMPLES  1024

/* ============================================================================
 * 1. ENTRADA DE MANDOS Y TECLADO (JOYPAD MAPPING)
 * ============================================================================
 * [CONCEPTO EDUCATIVO]:
 * La NES lee los mandos de forma serie a través del registro $4016.
 * En cada frame, el juego lee los 8 bits en este orden exacto:
 * A, B, Select, Start, Up, Down, Left, Right.
 */

typedef enum {
    MJK_BTN_A      = (1 << 0),  ///< Botón A (Salto / Aleteo continuo)
    MJK_BTN_B      = (1 << 1),  ///< Botón B (Acción / Abrir cofres)
    MJK_BTN_SELECT = (1 << 2),  ///< Botón Select
    MJK_BTN_START  = (1 << 3),  ///< Botón Start (Pausa / Comenzar partida)
    MJK_BTN_UP     = (1 << 4),  ///< D-Pad Arriba
    MJK_BTN_DOWN   = (1 << 5),  ///< D-Pad Abajo (Agacharse)
    MJK_BTN_LEFT   = (1 << 6),  ///< D-Pad Izquierda
    MJK_BTN_RIGHT  = (1 << 7)   ///< D-Pad Derecha
} MjkButtonMask;

typedef struct {
    uint8_t held;               ///< Botones actualmente mantenidos presionados
    uint8_t just_pressed;       ///< Flanco de subida (presionado en este frame exacto)
    uint8_t just_released;      ///< Flanco de bajada (soltado en este frame exacto)
} MjkInputState;

/* ============================================================================
 * 2. SALIDA DEL FRAME DE SIMULACIÓN (FRAME BUFFER & AUDIO SAMPLES)
 * ============================================================================
 * En cada ciclo a 60.0988 Hz, el núcleo produce:
 * 1. Un buffer de imagen RGBA32 de 256x240 píxeles.
 * 2. Un lote de muestras de audio PCM sintetizadas a 44.1 kHz.
 */

typedef struct {
    uint32_t framebuffer[MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT];
    int16_t  audio_samples[MJK_AUDIO_BUFFER_SAMPLES];
    uint32_t audio_sample_count;
} MjkFrameOutput;

/* ============================================================================
 * 3. FUNCIONES DE INTERFAZ HAL (FRONTEND PLATFORM API)
 * ============================================================================
 */

typedef struct {
    /**
     * @brief Obtiene la marca de tiempo de alta resolución en microsegundos.
     */
    uint64_t (*get_time_microseconds)(void);

    /**
     * @brief Consulta el estado actual de los periféricos (mando Xbox Series / teclado).
     */
    void (*poll_inputs)(MjkInputState* out_inputs);

    /**
     * @brief Envía el buffer de vídeo de 256x240 a la GPU para su escalado 4:3.
     */
    void (*present_framebuffer)(const uint32_t* framebuffer);

    /**
     * @brief Encola las muestras PCM en el dispositivo de salida de sonido.
     */
    void (*queue_audio_samples)(const int16_t* samples, uint32_t count);
} MjkPlatformInterface;

#endif // MJK_PLATFORM_H
