/**
 * @file platform.h
 * @brief Contrato minimo de la capa de plataforma SDL2 (Windows y Linux).
 *
 * Aisla por completo SDL2 y el sistema operativo del nucleo puro del motor.
 * Sin modo headless, sin overlays de debug ni teclas de cheats.
 */

#ifndef MJK_ENGINE_PLATFORM_H
#define MJK_ENGINE_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char* window_title;
    int         window_scale;
} MjkPlatformConfig;

bool     MjkPlatform_Init(const MjkPlatformConfig* config);
void     MjkPlatform_Shutdown(void);
bool     MjkPlatform_IsRunning(void);

uint64_t MjkPlatform_GetTimeMicroseconds(void);
void     MjkPlatform_SleepMilliseconds(uint32_t ms);

/** @brief Lee eventos SDL2 y devuelve la mascara cruda de botones (`MJK_INPUT_*`). */
uint8_t  MjkPlatform_PollRawButtons(void);

void     MjkPlatform_PresentFramebuffer(const uint32_t* fb_argb8888);
void     MjkPlatform_QueueAudioSamples(const int16_t* samples, uint32_t sample_count);

bool     MjkPlatform_GetExecutableDir(char* out_buf, size_t buf_size);
bool     MjkPlatform_EnsureDir(const char* dir_path);
void     MjkPlatform_ShowMessageBox(const char* title, const char* message, bool is_error);

#endif /* MJK_ENGINE_PLATFORM_H */
