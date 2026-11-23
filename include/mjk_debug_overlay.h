/**
 * @file mjk_debug_overlay.h
 * @brief Capa 3: Debug Overlay SDL2 (telemetría F1, Warp F2/[/]/F6, Cheats F3, Pausa F4/F5).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Usa una fuente bitmap 5x7 propia (Clean-Room, 0 bytes de Tecmo) superpuesta
 * directamente sobre el framebuffer ARGB8888 de 256x240 para inspeccionar
 * cualquier sala 0x00..0xFF, hitboxes y estado interno sin alterar la PPU.
 */

#ifndef MJK_DEBUG_OVERLAY_H
#define MJK_DEBUG_OVERLAY_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_platform.h"

typedef struct {
    bool    show_telemetry;
    bool    show_warp;
    bool    paused;
    bool    step_requested;
    uint8_t cheat_mode;   /**< 0 = OFF, 1 = INVINCIBLE, 2 = MIGHTY3. */
    uint8_t warp_room_id; /**< Sala seleccionada en el selector Warp (0x00..0xFF). */
} MjkDebugOverlayState;

typedef struct {
    uint8_t  room_id;
    uint8_t  game_state;
    uint8_t  lives;
    uint8_t  mighty_level;
    uint8_t  timer_bcd;
    uint16_t camera_x;
    int16_t  jack_x;
    int16_t  jack_y;
    bool     has_jack;
    uint8_t  oam_count;
} MjkDebugTelemetry;

void MjkDebugOverlay_Init(MjkDebugOverlayState* st);
bool MjkDebugOverlay_HandleKeys(MjkDebugOverlayState* st,
                                uint32_t debug_keys,
                                uint8_t* out_warp_target);
void MjkDebugOverlay_Render(const MjkDebugOverlayState* st,
                            const MjkDebugTelemetry* tel,
                            uint32_t* fb_argb8888);

#endif /* MJK_DEBUG_OVERLAY_H */
