/**
 * @file mjk_verify_api.c
 * @brief Implementación C11 pura de la API de verificación semántica (Capa 2, T-27).
 */

#include "mjk_verify_api.h"
#include <stdlib.h>
#include <string.h>
#include "mjk_apu.h"
#include "mjk_assets.h"
#include "mjk_game.h"
#include "mjk_ppu.h"

struct MjkVerifyCtx {
    MjkAssetPackage pkg;
    MjkGame         game;
    uint32_t        framebuffer[MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT];
    int16_t         audio_buf[MJK_APU_SAMPLES_PER_FRAME];
    uint8_t         prev_buttons;
    uint32_t        frame_count;
};

MjkVerifyCtx* MjkVerify_Create(const char* assets_dat_path) {
    if (!assets_dat_path) {
        return NULL;
    }
    MjkVerifyCtx* ctx = (MjkVerifyCtx*)calloc(1u, sizeof(MjkVerifyCtx));
    if (!ctx) {
        return NULL;
    }
    if (MjkAssets_LoadFromFile(assets_dat_path, &ctx->pkg) != MJK_ASSET_OK) {
        free(ctx);
        return NULL;
    }
    MjkGame_Init(&ctx->game, &ctx->pkg);
    MjkGame_Render(&ctx->game, ctx->framebuffer);
    return ctx;
}

void MjkVerify_Destroy(MjkVerifyCtx* ctx) {
    if (!ctx) {
        return;
    }
    MjkAssets_Unload(&ctx->pkg);
    free(ctx);
}

void MjkVerify_Reset(MjkVerifyCtx* ctx) {
    if (!ctx) {
        return;
    }
    ctx->prev_buttons = 0u;
    ctx->frame_count = 0u;
    memset(ctx->audio_buf, 0, sizeof(ctx->audio_buf));
    MjkGame_Init(&ctx->game, &ctx->pkg);
    MjkGame_Render(&ctx->game, ctx->framebuffer);
}

void MjkVerify_StepFrame(MjkVerifyCtx* ctx, uint8_t buttons_mask) {
    if (!ctx) {
        return;
    }
    MjkInputState input;
    input.held = buttons_mask;
    input.just_pressed = (uint8_t)(buttons_mask & (uint8_t)(~ctx->prev_buttons));
    input.just_released = (uint8_t)((uint8_t)(~buttons_mask) & ctx->prev_buttons);
    ctx->prev_buttons = buttons_mask;

    MjkGame_StepFrame(&ctx->game, &input);
    MjkGame_StepAudio(&ctx->game, ctx->audio_buf, MJK_APU_SAMPLES_PER_FRAME);
    MjkGame_Render(&ctx->game, ctx->framebuffer);
    ctx->frame_count++;
}

void MjkVerify_WarpRoom(MjkVerifyCtx* ctx, uint8_t room_id) {
    if (!ctx) {
        return;
    }
    MjkGame_WarpToRoom(&ctx->game, room_id);
    if (room_id == 0xACu) {
        ctx->game.torture_scene_flag = 3u;
        ctx->game.greedy_jumps_bcd = 0x50u;
    }
    MjkGame_Render(&ctx->game, ctx->framebuffer);
}

void MjkVerify_GetSnapshot(const MjkVerifyCtx* ctx, MjkVerifySnapshot* out_snap) {
    if (!ctx || !out_snap) {
        return;
    }
    memset(out_snap, 0, sizeof(*out_snap));
    const MjkGame* g = &ctx->game;

    out_snap->top_mode = (uint8_t)g->top_mode;
    out_snap->game_state = (uint8_t)g->state;
    out_snap->title_state = (uint8_t)g->title.state;
    out_snap->room_id = g->current_room_id;
    out_snap->lives = g->lives;
    out_snap->gdv_bcd = g->gdv_bcd;
    out_snap->high_gdv_bcd = g->high_gdv_bcd;
    memcpy(out_snap->score_bcd, g->score_bcd, 4u);

    out_snap->jack_x = (g->top_mode == MJK_MODE_TITLE) ? MjkUQ8_Int(g->title.jack_x) : g->jack_x;
    out_snap->jack_y = (g->top_mode == MJK_MODE_TITLE) ? MjkUQ8_Int(g->title.jack_y) : g->jack_y;
    out_snap->in_air = g->player.in_air ? 1u : 0u;
    out_snap->is_dying = g->player.is_dying ? 1u : 0u;
    out_snap->mighty_level = g->mighty_level;
    out_snap->mighty_coins = g->mighty_coins;
    out_snap->timer_bcd = g->stage_timer_bcd;
    out_snap->greedy_jumps_bcd = g->greedy_jumps_bcd;
    out_snap->fire_bombs_collected = g->fire_bombs_collected;
    out_snap->alive_enemies = g->enemies.alive_count;
    out_snap->last_sound_id = (uint8_t)g->last_sound_event;

    uint8_t oam_cnt = 0u;
    size_t oam_cap = sizeof(g->ppu.oam) / sizeof(g->ppu.oam[0]);
    for (size_t i = 0u; i < oam_cap; ++i) {
        if (g->ppu.oam[i].y < 0xEFu) {
            oam_cnt++;
        }
    }
    out_snap->oam_count = oam_cnt;
    out_snap->scroll_x = g->ppu.scroll_x;
    out_snap->scroll_y = g->ppu.scroll_y;
    out_snap->frame_count = ctx->frame_count;
}

const uint32_t* MjkVerify_GetFramebuffer(const MjkVerifyCtx* ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->framebuffer;
}

uint32_t MjkVerify_GetLastAudioEnergy(const MjkVerifyCtx* ctx) {
    if (!ctx) {
        return 0u;
    }
    return ctx->game.apu.last_frame_energy;
}
