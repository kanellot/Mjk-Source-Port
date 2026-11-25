/**
 * @file player.c
 * @brief Implementacion limpia de la fisica de Jack en `float` y colisiones 9-probe.
 */

#include "mjk/player.h"
#include <math.h>
#include <string.h>

/* 9 sondas relativas al centro de Jack (0=centro, 1-2=techo, 3-4=suelo, 5-6=izq, 7-8=der) */
static const int k_probe_dx[9] = { 0, -4,  4, -4, 4, -8, -8,  8, 8 };
static const int k_probe_dy[9] = { 0, -8, -8,  8, 8, -4,  4, -4, 4 };

static float advance_jump_curve(float* jump_progress, float rate, const MjkAssets* assets) {
    float next = *jump_progress + rate;
    if (next >= 128.0f) {
        next = 127.0f + (next - floorf(next));
    }
    *jump_progress = next;

    int step = (int)floorf(next);
    int tbl_idx = (step < 64) ? step : ((step ^ 0xFF) & 0x3F);
    float mag = assets->jump_speed[tbl_idx];
    return (step < 64) ? -mag : mag;
}

static float clamp_screen_y(int world_y, int scroll_y) {
    int sy = world_y - scroll_y;
    if (sy < 0)   { sy = 0; }
    if (sy > 255) { sy = 255; }
    return (float)sy;
}

void MjkPlayer_Init(MjkPlayer* player, float x, float y) {
    if (!player) {
        return;
    }
    /* `jump_press_count` solo se reinicia al empezar partida nueva, nunca al cambiar de sala. */
    uint8_t saved_presses = player->jump_press_count;
    memset(player, 0, sizeof(*player));
    player->jump_press_count = saved_presses;
    player->x      = x;
    player->y      = y;
    player->facing = FACING_RIGHT;
}

void MjkPlayer_Kill(MjkPlayer* player, uint8_t fire_tile) {
    if (!player || player->is_dying) {
        return;
    }
    player->is_dying          = true;
    player->death_grounded    = false;
    player->death_timer       = 0;
    player->vx                = 0.0f;
    player->fire_tile_died_to = fire_tile;
}

static MjkActionResult step_death_sequence(MjkPlayer* p,
                                           const MjkWorld* world,
                                           const MjkAssets* assets) {
    p->vx = 0.0f;
    if (p->death_grounded) {
        p->vy        = 0.0f;
        p->sprite_id = 0x2Au;
        p->death_timer++;
        return (p->death_timer >= 64) ? ACTION_PLAYER_DIED : ACTION_CONTINUE;
    }

    int scroll_x = (int)floorf(world->camera.scroll_x);
    int scroll_y = (int)floorf(world->camera.scroll_y);
    int cx = (int)floorf(p->x) + scroll_x;
    int cy = (int)floorf(p->y) + scroll_y;

    uint8_t ft0 = 0u, ft1 = 0u;
    bool g0 = MjkWorld_IsPointSolid(world, cx + k_probe_dx[3], cy + k_probe_dy[3], true, &ft0) || (ft0 != 0u);
    bool g1 = MjkWorld_IsPointSolid(world, cx + k_probe_dx[4], cy + k_probe_dy[4], true, &ft1) || (ft1 != 0u);
    if ((floorf(p->y) >= MJK_JACK_FLOOR_Y || g0 || g1) && p->vy >= 0.0f) {
        p->death_grounded = true;
        p->death_timer    = 0;
        p->in_air         = false;
        p->vy             = 0.0f;
        p->sprite_id      = 0x2Au;
        return ACTION_CONTINUE;
    }

    p->sprite_id = ((p->death_timer / 6) & 1) ? 0x29u : 0x28u;
    p->death_timer++;

    if (!p->in_air) {
        p->in_air        = true;
        p->jump_progress = 64.0f;
    }
    p->vy = advance_jump_curve(&p->jump_progress, assets->jump_rate[0], assets);
    p->y += p->vy;
    cy = (int)floorf(p->y) + scroll_y;

    if (p->vy < 0.0f) {
        if (MjkWorld_IsPointSolid(world, cx + k_probe_dx[1], cy + k_probe_dy[1], false, NULL) ||
            MjkWorld_IsPointSolid(world, cx + k_probe_dx[2], cy + k_probe_dy[2], false, NULL)) {
            p->jump_progress = 64.0f + (p->jump_progress - floorf(p->jump_progress));
            int snap_wy = ((cy + k_probe_dy[1]) & ~7) + 16;
            p->y  = clamp_screen_y(snap_wy, scroll_y);
            p->vy = 0.0f;
        }
    } else if (floorf(p->y) >= MJK_JACK_FLOOR_Y) {
        p->y              = MJK_JACK_FLOOR_Y;
        p->death_grounded = true;
        p->death_timer    = 0;
        p->in_air         = false;
        p->vy             = 0.0f;
        p->sprite_id      = 0x2Au;
    } else if (MjkWorld_IsPointSolid(world, cx + k_probe_dx[3], cy + k_probe_dy[3], true, NULL) ||
               MjkWorld_IsPointSolid(world, cx + k_probe_dx[4], cy + k_probe_dy[4], true, NULL)) {
        int snap_wy = ((cy + k_probe_dy[3]) & ~7) - 8;
        p->y              = clamp_screen_y(snap_wy, scroll_y);
        p->death_grounded = true;
        p->death_timer    = 0;
        p->in_air         = false;
        p->vy             = 0.0f;
        p->sprite_id      = 0x2Au;
    }
    return ACTION_CONTINUE;
}

MjkActionResult MjkPlayer_Update(MjkPlayer* p,
                                 const MjkInputStep* input,
                                 const MjkWorld* world,
                                 const MjkAssets* assets,
                                 MjkPlayerStepCtx* ctx) {
    if (!p || !world || !assets) {
        return ACTION_CONTINUE;
    }
    if (ctx) {
        ctx->sfx_triggered    = SOUND_SILENCE;
        ctx->award_jump_10pts = false;
    }
    p->just_jumped = false;

    if (p->is_dying) {
        return step_death_sequence(p, world, assets);
    }

    const uint8_t held    = input ? input->held : 0u;
    const uint8_t pressed = input ? input->pressed : 0u;

    /* 1. Movimiento horizontal instantaneo (±1.5 px/frame, prioridad Derecha) */
    p->vx = 0.0f;
    if ((held & MJK_INPUT_RIGHT) != 0u) {
        p->vx     = MJK_JACK_RUN_SPEED;
        p->facing = FACING_RIGHT;
    } else if ((held & MJK_INPUT_LEFT) != 0u) {
        p->vx     = -MJK_JACK_RUN_SPEED;
        p->facing = FACING_LEFT;
    }

    /* 2. Salto y Flutter (estrictamente por flanco de subida) */
    uint8_t jump_mask = world->current.is_bomb_room
        ? (uint8_t)(MJK_INPUT_A | MJK_INPUT_B)
        : (uint8_t)MJK_INPUT_A;

    if ((pressed & jump_mask) != 0u) {
        p->jump_press_count++;
        if (ctx && ctx->mighty_level && ctx->mighty_presses_left &&
            *ctx->mighty_level > 0u && *ctx->mighty_presses_left > 0u) {
            (*ctx->mighty_presses_left)--;
            uint8_t threshold = assets->mighty_a_presses[(*ctx->mighty_level - 1u) & 3u];
            if (*ctx->mighty_presses_left < threshold) {
                (*ctx->mighty_level)--;
            }
        }
        if (!p->in_air) {
            p->in_air        = true;
            p->jump_progress = 0.0f;
            p->just_jumped   = true;
            if (ctx) {
                ctx->sfx_triggered    = SOUND_JUMP;
                ctx->award_jump_10pts = !ctx->in_torture_room;
            }
        } else {
            /* Flutter: reinicia el progreso al apice de la parabola (paso 64) */
            p->jump_progress = 64.0f;
        }
    }

    /* 3. Curva vertical de 64 pasos */
    if (p->in_air) {
        uint8_t v_idx = (uint8_t)((held & (MJK_INPUT_UP | MJK_INPUT_DOWN)) >> 2u);
        p->vy = advance_jump_curve(&p->jump_progress, assets->jump_rate[v_idx & 3u], assets);
    } else {
        p->vy = 0.0f;
    }

    /* 4. Integracion y colisiones 9-probe */
    int scroll_x = (int)floorf(world->camera.scroll_x);
    int scroll_y = (int)floorf(world->camera.scroll_y);
    p->collision_flags = 0u;

    p->x += p->vx;
    int cx = (int)floorf(p->x) + scroll_x;
    int cy = (int)floorf(p->y) + scroll_y;

    if (p->vx > 0.0f) {
        if (MjkWorld_IsPointSolid(world, cx + k_probe_dx[7], cy + k_probe_dy[7], false, NULL) ||
            MjkWorld_IsPointSolid(world, cx + k_probe_dx[8], cy + k_probe_dy[8], false, NULL)) {
            p->collision_flags |= MJK_COLL_RIGHT_WALL;
            p->x -= p->vx;
            p->vx = 0.0f;
        }
    } else if (p->vx < 0.0f) {
        if (MjkWorld_IsPointSolid(world, cx + k_probe_dx[5], cy + k_probe_dy[5], false, NULL) ||
            MjkWorld_IsPointSolid(world, cx + k_probe_dx[6], cy + k_probe_dy[6], false, NULL)) {
            p->collision_flags |= MJK_COLL_LEFT_WALL;
            p->x -= p->vx;
            p->vx = 0.0f;
        }
    }

    p->y += p->vy;
    cx = (int)floorf(p->x) + scroll_x;
    cy = (int)floorf(p->y) + scroll_y;

    if (p->vy < 0.0f) {
        if (MjkWorld_IsPointSolid(world, cx + k_probe_dx[1], cy + k_probe_dy[1], false, NULL) ||
            MjkWorld_IsPointSolid(world, cx + k_probe_dx[2], cy + k_probe_dy[2], false, NULL)) {
            p->collision_flags |= MJK_COLL_CEILING;
            p->jump_progress = 64.0f + (p->jump_progress - floorf(p->jump_progress));
            int snap_wy = ((cy + k_probe_dy[1]) & ~7) + 16;
            p->y  = clamp_screen_y(snap_wy, scroll_y);
            p->vy = 0.0f;
        }
    } else {
        uint8_t rf = world->camera.room_flags;
        bool clamp_bottom = ((rf & (ROOM_FLAG_SINGLE_SCREEN | ROOM_FLAG_HORIZONTAL | ROOM_FLAG_AT_SCROLL_EDGE)) != 0u) &&
                            (floorf(p->y) >= MJK_JACK_FLOOR_Y);
        if (clamp_bottom) {
            p->collision_flags |= MJK_COLL_GROUND;
            p->in_air = false;
            p->y      = MJK_JACK_FLOOR_Y;
            p->vy     = 0.0f;
        } else if (MjkWorld_IsPointSolid(world, cx + k_probe_dx[3], cy + k_probe_dy[3], true, NULL) ||
                   MjkWorld_IsPointSolid(world, cx + k_probe_dx[4], cy + k_probe_dy[4], true, NULL)) {
            p->collision_flags |= MJK_COLL_GROUND;
            p->in_air = false;
            int snap_wy = ((cy + k_probe_dy[3]) & ~7) - 8;
            p->y  = clamp_screen_y(snap_wy, scroll_y);
            p->vy = 0.0f;
        } else if (!p->in_air) {
            p->in_air        = true;
            p->jump_progress = 64.0f;
        }
    }

    /* Sonda central (0): muerte por fuego ($5C..$62) */
    uint8_t fire_tile = 0u;
    MjkWorld_IsPointSolid(world, (int)floorf(p->x) + scroll_x, (int)floorf(p->y) + scroll_y, false, &fire_tile);
    if (fire_tile != 0u) {
        MjkPlayer_Kill(p, fire_tile);
        if (ctx) { ctx->sfx_triggered = SOUND_DEATH; }
    }

    /* 5. Seleccion de sprite */
    bool pushing_h = ((held & MJK_INPUT_RIGHT) != 0u) != ((held & MJK_INPUT_LEFT) != 0u);
    if (!p->in_air) {
        if (p->vx != 0.0f || pushing_h) {
            p->walk_timer++;
            p->sprite_id = ((p->walk_timer & 4u) != 0u) ? 0x05u : 0x04u;
        } else {
            p->sprite_id = 0x00u;
        }
    } else if (p->vy < 0.0f) {
        p->sprite_id = (p->vx != 0.0f || pushing_h) ? 0x04u : 0x01u;
    } else {
        p->sprite_id = (p->vx != 0.0f || pushing_h) ? 0x06u : 0x02u;
    }
    return ACTION_CONTINUE;
}
