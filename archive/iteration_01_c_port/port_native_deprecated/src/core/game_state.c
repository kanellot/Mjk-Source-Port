/**
 * @file game_state.c
 * @brief Core simulation lifecycle and state machine implementation
 *        for Mighty Bomb Jack (NES) Native Port.
 * 
 * Conforms to:
 *   - ADR-004: Golden Standard US (REV_US)
 *   - RES-004: State Machine & 60Hz Dispatch
 *   - RES-005: RAM State Initialization
 *   - RES-006: Physics Integration & Collision
 */

#include "mbj/game_state.h"
#include "mbj/physics.h"
#include "mbj/collision.h"
#include <string.h>

void Game_Init(GameState *game) {
    if (!game) return;
    memset(game, 0, sizeof(GameState));

    game->in_game_flag = LOGIC_MODE_INGAME;
    game->state = STATE_5_MAIN;
    game->current_room = 1;
    game->checkpoint_room = 1;

    /* US canonical baseline settings (ADR-004) */
    game->player.lives = 3;
    game->player.facing = 1; /* Facing Right */
    game->player.x = SUBPIXEL_FROM_PIXEL(32);
    game->player.y = SUBPIXEL_FROM_PIXEL(176);
    game->player.mighty_level = 0;
    game->player.mighty_coins = 0;

    game->stage_timer_bcd = 0x60; /* 60 seconds */
    game->torture_jumps_left = 0x50; /* 50 jumps in US version */

    /* Synchronize authentic RAM image ($0000-$07FF) */
    game->ram.in_game_flag = 1;
    game->ram.game_state = STATE_5_MAIN;
    game->ram.stage_timer_bcd = 0x60;
    game->ram.torture_jumps_bcd = 0x50;
    game->ram.player.lives = 3;
    game->ram.player.facing_attr = 0x08; /* Facing Right */
    game->ram.player.x_pos_hi = 32;
    game->ram.player.y_pos_hi = 176;

    /* Clear virtual screen buffer to authentic NES background dark blue (#000000 / #0F) */
    for (int i = 0; i < MBJ_NES_WIDTH * MBJ_NES_HEIGHT; i++) {
        game->screen_buffer[i] = 0xFF181008; /* Dark Blue-Gray ABGR */
    }
}

void Game_Reset(GameState *game) {
    Game_Init(game);
}

void Game_Update(GameState *game, const GameInput *input) {
    if (!game) return;
    game->frame_count++;

    /* In game state machine dispatch (prg.asm:302) */
    switch (game->state) {
        case STATE_0_INIT:
            game->state = STATE_1_ROUND_INTRO;
            game->state_timer = 96; /* 96 frames for round intro */
            break;

        case STATE_1_ROUND_INTRO:
            if (game->state_timer > 0) {
                game->state_timer--;
            } else {
                game->state = STATE_5_MAIN;
            }
            break;

        case STATE_5_MAIN:
            /* Main gameplay logic: tick player movement */
            if (input) {
                /* 1. Horizontal Movement (MainSub_1, loc_89EF) */
                if (GameInput_IsDown(input, JP_BTN_LEFT)) {
                    /* -1.50 px/frame: -2 hi, +$80 lo */
                    game->ram.player.x_vel_hi = -2;
                    game->ram.player.x_vel_lo = 0x80;
                    game->ram.player.facing_attr = 0x00; /* Left */
                    game->player.facing = 0;
                } else if (GameInput_IsDown(input, JP_BTN_RIGHT)) {
                    /* +1.50 px/frame: +1 hi, +$80 lo */
                    game->ram.player.x_vel_hi = 1;
                    game->ram.player.x_vel_lo = 0x80;
                    game->ram.player.facing_attr = 0x08; /* Right */
                    game->player.facing = 1;
                } else {
                    game->ram.player.x_vel_hi = 0;
                    game->ram.player.x_vel_lo = 0;
                }

                /* 2. Parabolic Jump & Flutter (DoSomethingWithObjYVel, sub_8AE8) */
                bool just_jumped = GameInput_JustPressed(input, JP_BTN_A);
                Physics_UpdateJackJump(&game->ram.player, input->buttons, just_jumped);

                /* 3. Subpixel Position Integration */
                Physics_IntegratePosition(&game->ram.player);

                /* 4. Screen floor clamp (until room geometry is loaded in TASK-IMPL-004) */
                if (game->ram.player.y_pos_hi >= 200) {
                    game->ram.player.y_pos_hi = 200;
                    game->ram.player.y_pos_lo = 0;
                    game->ram.player.y_vel_hi = 0;
                    game->ram.player.y_vel_lo = 0;
                    game->ram.player.status &= ~0x02;        /* Clear in-air flag (grounded) */
                    game->ram.player.collision_flag |= 0x04; /* Grounded */
                }

                /* Horizontal Boundaries */
                if (game->ram.player.x_pos_hi < 8) {
                    game->ram.player.x_pos_hi = 8;
                    game->ram.player.x_pos_lo = 0;
                }
                if (game->ram.player.x_pos_hi > 240) {
                    game->ram.player.x_pos_hi = 240;
                    game->ram.player.x_pos_lo = 0;
                }

                /* Synchronize high-level player position helpers */
                game->player.x = ((subpixel_t)game->ram.player.x_pos_hi << 8) | game->ram.player.x_pos_lo;
                game->player.y = ((subpixel_t)game->ram.player.y_pos_hi << 8) | game->ram.player.y_pos_lo;
            }
            break;

        default:
            break;
    }
}
