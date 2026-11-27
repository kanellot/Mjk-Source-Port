/**
 * @file screens.c
 * @brief Secuencias de pantallas: Titulo (REV_US), RoundClear, GameOver y 4 Finales (R-13).
 *
 * Este modulo gestiona unicamente transiciones y temporizadores de estado; no dibuja
 * pixeles ni manipula OAM/PPU (el renderizador en `src/render/renderer.c` lee este estado).
 */

#include "mjk/game.h"
#include <math.h>
#include <string.h>

static void init_title_screen(MjkTitleScreen* ts) {
    memset(ts, 0, sizeof(*ts));
    ts->state = TITLE_COPYRIGHT_INIT;
}

bool MjkScreens_StepTitle(MjkGame* game, const MjkInputStep* input) {
    if (!game || !game->assets) {
        return false;
    }
    MjkTitleScreen* ts = &game->title;
    const uint8_t held    = input ? input->held : 0u;
    const uint8_t pressed = input ? input->pressed : 0u;

    switch (ts->state) {
        case TITLE_COPYRIGHT_INIT:
            ts->timer = 256;
            ts->state = TITLE_COPYRIGHT_WAIT;
            break;

        case TITLE_COPYRIGHT_WAIT:
            if (ts->timer > 0) { ts->timer--; }
            if ((held & MJK_INPUT_START) != 0u || ts->timer == 0) {
                ts->state = TITLE_MAIN_INIT;
            }
            break;

        case TITLE_MAIN_INIT:
            ts->jack_x      = 40.0f;
            ts->jack_y      = 120.0f;
            ts->jack_vx     = 0.0f;
            ts->jack_vy     = 0.0f;
            ts->jack_sprite = 0u;
            ts->timer       = 512;
            ts->state       = TITLE_MAIN_WAIT;
            break;

        case TITLE_MAIN_WAIT:
            if ((pressed & MJK_INPUT_START) != 0u) {
                ts->jack_vx       = 0.5f;
                ts->jack_sprite   = 3u;
                ts->grav_progress = 64.0f;
                ts->state         = TITLE_JACK_FALLING;
                break;
            }
            if (ts->timer > 0) { ts->timer--; }
            if (ts->timer == 0) {
                ts->state = TITLE_COPYRIGHT_INIT;
            }
            break;

        case TITLE_JACK_FALLING: {
            float next = ts->grav_progress + game->assets->jump_rate[2];
            if (next >= 128.0f) {
                next = 127.0f + (next - floorf(next));
            }
            ts->grav_progress = next;
            int step = (int)floorf(next);
            int idx  = (step < 64) ? step : ((step ^ 0xFF) & 0x3F);
            float mag = game->assets->jump_speed[idx];
            ts->jack_vy = (step < 64) ? -mag : mag;
            ts->jack_x += ts->jack_vx;
            ts->jack_y += ts->jack_vy;

            if (floorf(ts->jack_y) >= 216.0f) {
                init_title_screen(ts);
                return true;
            }
            break;
        }

        default:
            init_title_screen(ts);
            break;
    }
    return false;
}

void MjkScreens_StepRoundClear(MjkGame* game) {
    static const int k_fire_bomb_bonus[4] = { 10, 20, 30, 50 };

    switch (game->sub_state) {
        case 0u:
            game->world.current.is_bomb_room = false;
            game->scoring.timer_frozen       = true;
            game->state_timer                = 64;
            game->round_clear_timer_copy     = game->scoring.timer_seconds;
            game->round_clear_has_special    = false;
            game->round_clear_fire_bonus     = 0;

            if (game->fire_bombs_collected >= 20u) {
                uint8_t idx = (uint8_t)(game->fire_bombs_collected - 20u);
                if (idx > 3u) { idx = 3u; }
                game->round_clear_fire_bonus  = k_fire_bomb_bonus[idx];
                game->round_clear_has_special = true;
            }
            MjkGame_QueueSound(game, MUSIC_ROUND_CLEAR);
            game->sub_state = 1u;
            break;

        case 1u:
            if (game->state_timer > 0) { game->state_timer--; break; }
            if (game->round_clear_timer_copy > 0) {
                game->round_clear_timer_copy--;
                MjkGame_AddScore(game, SCORE_100);
                game->state_timer = 6;
            } else {
                game->state_timer = 64;
                game->sub_state   = game->round_clear_has_special ? 2u : 3u;
            }
            break;

        case 2u:
            if (game->state_timer > 0) { game->state_timer--; break; }
            if (game->round_clear_fire_bonus > 0) {
                game->round_clear_fire_bonus--;
                MjkGame_AddScore(game, SCORE_1000);
                game->state_timer = 6;
            } else {
                game->state_timer = 64;
                game->sub_state   = 3u;
            }
            break;

        default:
            if (game->state_timer > 0) { game->state_timer--; break; }
            game->collected_1up            = false;
            game->scoring.score_multiplier = 0u;
            game->door_1e_deaths           = 0u;
            game->scoring.timer_frozen     = false;
            MjkGame_QueueSound(game, SOUND_SILENCE);
            if (game->fire_bombs_collected == 1u && game->last_bomb_room_cleared != 0x9Fu) {
                uint8_t next = (uint8_t)(game->last_bomb_room_cleared + 1u);
                game->current_room_id         = next;
                game->entry_door_type         = game->assets->bomb_room_entry_door[next & 0x0Fu];
                game->entry_door_already_open = false;
                MjkScoring_ResetStageTimer(&game->scoring, 60);
            }
            game->state     = GS_LOAD_ROOM;
            game->sub_state = 0u;
            break;
    }
}

void MjkScreens_StepGameOver(MjkGame* game) {
    if (game->sub_state == 0u) {
        MjkGame_QueueSound(game, MUSIC_GAME_OVER);

        /* Bonificaciones finales de GDV: ronda alcanzada, vidas extra y marcador */
        int round_bonus = (int)((game->last_bomb_room_cleared & 0x0Fu) >> 1u);
        int lives_bonus = (int)(game->extra_lives_found >> 1u);
        uint32_t score_100k = game->scoring.score / 100000u;
        int score_bonus = (score_100k >= 10u) ? 10 : (int)score_100k;
        MjkScoring_AddGdv(&game->scoring, round_bonus + lives_bonus + score_bonus);

        game->state_timer = 256;
        game->sub_state   = 1u;
        return;
    }
    if (game->state_timer > 0) {
        game->state_timer--;
    }
    if (game->state_timer == 0) {
        game->mode = SCREEN_TITLE;
        init_title_screen(&game->title);
    }
}

void MjkScreens_StepEnding(MjkGame* game, const MjkInputStep* input) {
    static const uint8_t k_destroy_depth[4] = { 12u, 30u, 56u, 90u };
    static const float   k_end_jack_y[4]    = { 144.0f, 160.0f, 176.0f, 192.0f };
    uint8_t et = game->ending_type & 3u;

    switch (game->ending_state) {
        case 0u:
            game->cinematic_jack_x   = 120.0f;
            game->cinematic_jack_y   = 232.0f;
            game->ending_carved_rows = 0u;
            game->state_timer        = 24;
            game->ending_state       = 1u;
            break;

        case 1u:
            if (game->cinematic_jack_y > 144.0f) {
                game->cinematic_jack_y -= 2.0f;
            }
            if (game->state_timer > 0) {
                game->state_timer--;
                break;
            }
            game->ending_depth = k_destroy_depth[et];
            game->state_timer  = 8;
            game->ending_state = 2u;
            break;

        case 2u: {
            uint8_t total = k_destroy_depth[et];
            uint8_t done  = (total >= game->ending_depth) ? (uint8_t)(total - game->ending_depth) : 0u;
            float jy = 136.0f + (float)(done / 2u);
            if (jy > k_end_jack_y[et]) { jy = k_end_jack_y[et]; }
            game->cinematic_jack_y = jy;

            if (game->state_timer > 0) {
                game->state_timer--;
                break;
            }
            if (game->ending_depth > 0u) {
                game->ending_carved_rows = (uint8_t)(done / 10u + 1u);
                game->ending_depth--;
                MjkGame_QueueSound(game, SOUND_BOMB_BONUS);
                game->state_timer = 8;
            } else {
                game->ending_state = 3u;
            }
            break;
        }

        case 3u:
            game->cinematic_jack_y = k_end_jack_y[et];
            MjkGame_QueueSound(game, MUSIC_ENDING);
            game->ending_state = 4u;
            break;

        default:
            game->cinematic_jack_y = k_end_jack_y[et];
            if (input && (input->pressed & MJK_INPUT_START) != 0u) {
                init_title_screen(&game->title);
                game->state     = GS_GAME_OVER;
                game->sub_state = 0u;
            }
            break;
    }
}
