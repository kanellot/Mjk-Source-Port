/**
 * @file mjk_verify_api.h
 * @brief API C11 limpia de inspección headless para Capa 2 (`tools/verify_scenarios.py`).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Expone un contexto opaco `MjkVerifyCtx` sobre `mjk_core.dll` / `libmjk_core.so`
 * sin dependencias de SDL ni del SO, permitiendo avanzar frames deterministas,
 * leer telemetría estructurada, inspeccionar el framebuffer ARGB8888 y medir
 * la energía de audio 2A03 por frame.
 */

#ifndef MJK_VERIFY_API_H
#define MJK_VERIFY_API_H

#include <stdint.h>
#include "mjk_build_info.h"

typedef struct {
    uint8_t  top_mode;
    uint8_t  game_state;
    uint8_t  title_state;
    uint8_t  room_id;
    uint8_t  lives;
    uint8_t  gdv_bcd;
    uint8_t  high_gdv_bcd;
    uint8_t  score_bcd[4];
    uint8_t  jack_x;
    uint8_t  jack_y;
    uint8_t  in_air;
    uint8_t  is_dying;
    uint8_t  mighty_level;
    uint8_t  mighty_coins;
    uint8_t  timer_bcd;
    uint8_t  greedy_jumps_bcd;
    uint8_t  fire_bombs_collected;
    uint8_t  alive_enemies;
    uint8_t  last_sound_id;
    uint8_t  oam_count;
    uint16_t scroll_x;
    uint16_t scroll_y;
    uint32_t frame_count;
} MjkVerifySnapshot;

typedef struct MjkVerifyCtx MjkVerifyCtx;

MJK_CORE_API MjkVerifyCtx*   MjkVerify_Create(const char* assets_dat_path);
MJK_CORE_API void            MjkVerify_Destroy(MjkVerifyCtx* ctx);
MJK_CORE_API void            MjkVerify_Reset(MjkVerifyCtx* ctx);
MJK_CORE_API void            MjkVerify_StepFrame(MjkVerifyCtx* ctx, uint8_t buttons_mask);
MJK_CORE_API void            MjkVerify_WarpRoom(MjkVerifyCtx* ctx, uint8_t room_id);
MJK_CORE_API void            MjkVerify_GetSnapshot(const MjkVerifyCtx* ctx, MjkVerifySnapshot* out_snap);
MJK_CORE_API const uint32_t* MjkVerify_GetFramebuffer(const MjkVerifyCtx* ctx);
MJK_CORE_API uint32_t        MjkVerify_GetLastAudioEnergy(const MjkVerifyCtx* ctx);

#endif /* MJK_VERIFY_API_H */
