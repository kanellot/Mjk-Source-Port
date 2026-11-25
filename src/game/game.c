/**
 * @file game.c
 * @brief Maquina de estados principal de Mighty Bomb Jack (`MjkGame`, R-13).
 *
 * Orquesta de forma lineal y legible el ciclo completo:
 *   TITLE -> INIT -> ROUND_INTRO -> LOAD_ROOM -> DRAW_SCREEN -> ENTER_ROOM ->
 *   PLAYING -> (TIME_OVER / TORTURE_ROOM / ROUND_CLEAR / EXIT_ROOM / ANIMATE_DOOR_OPEN /
 *               ENDING / GAME_OVER).
 */

#include "mjk/game.h"
#include <string.h>

void MjkGame_Init(MjkGame* game, const MjkAssets* assets) {
    if (!game) {
        return;
    }
    memset(game, 0, sizeof(*game));
    game->assets      = assets;
    game->mode        = SCREEN_TITLE;
    game->state       = GS_INIT;
    game->title.state = TITLE_COPYRIGHT_INIT;
}

void MjkGame_QueueSound(MjkGame* game, MjkSoundId sound) {
    if (!game) {
        return;
    }
    game->last_sound = sound;
    if (game->sound_queue_count < MJK_SOUND_QUEUE_CAP) {
        game->sound_queue[game->sound_queue_count++] = sound;
    }
}

void MjkGame_AddScore(MjkGame* game, MjkScoreId score_id) {
    if (!game || !game->assets) {
        return;
    }
    MjkScoreEvents ev;
    MjkScoring_AddPoints(&game->scoring, game->assets, score_id,
                         game->world.current.is_bomb_room, &ev);
    if (ev.spawn_bonus_coin || ev.spawn_extra_coin) {
        for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
            if (game->enemies.slots[i].active &&
                (game->enemies.slots[i].kind == ENEMY_BONUS_COIN ||
                 game->enemies.slots[i].kind == ENEMY_EXTRA_COIN)) {
                return;
            }
        }
        MjkEnemyKind kind = ev.spawn_extra_coin ? ENEMY_EXTRA_COIN : ENEMY_BONUS_COIN;
        if (MjkEnemies_Spawn(&game->enemies, kind, 128.0f, 64.0f)) {
            MjkGame_QueueSound(game, SOUND_BONUS_COIN_SPAWN);
        }
    }
}

MjkActionResult MjkGame_TriggerTortureRoom(MjkGame* game) {
    if (!game) {
        return ACTION_CONTINUE;
    }
    game->mighty_coins    = 0u;
    game->in_torture_mode = false;
    game->state           = GS_TORTURE_ROOM;
    game->sub_state       = 0u;
    return ACTION_TORTURE_TRIGGERED;
}

void MjkGame_OnTortureJump(MjkGame* game) {
    if (!game || !game->in_torture_mode) {
        return;
    }
    if (game->greedy_jumps_left > 0) {
        game->greedy_jumps_left--;
    }
    if (game->greedy_jumps_left == 0) {
        game->in_torture_mode      = false;
        game->scoring.timer_frozen = false;
        game->checkpoint_room_id   = game->torture_return_room_id;
        game->entry_door_backup    = game->torture_return_door_type;
        game->current_room_id      = game->checkpoint_room_id;
        game->entry_door_type      = game->entry_door_backup;
        game->state                = GS_LOAD_ROOM;
        game->sub_state            = 0u;
    }
}

void MjkGame_LoseLife(MjkGame* game) {
    if (!game) {
        return;
    }
    game->scoring.timer_frozen      = false;
    game->scoring.timer_seconds     = 0;
    game->mighty_level              = 0u;
    game->mighty_presses_left       = 0u;
    game->in_torture_mode           = false;
    game->power_coin_palette_active = false;

    if (game->scoring.lives > 0) {
        game->scoring.lives--;
    }
    if (game->scoring.lives == 0) {
        game->state     = GS_GAME_OVER;
        game->sub_state = 0u;
        return;
    }
    game->current_room_id = game->checkpoint_room_id;
    game->entry_door_type = game->entry_door_backup;
    game->state           = GS_ROUND_INTRO;
    game->sub_state       = 0u;
}

static void resolve_entry_door(MjkGame* game, uint8_t next_room, uint8_t exit_door) {
    uint8_t target = (uint8_t)(exit_door ^ 0x04u);
    uint8_t alt    = (uint8_t)(exit_door ^ 0x06u);
    game->entry_door_type         = target;
    game->entry_door_already_open = false;
    if (next_room == 0u || !game->assets) {
        return;
    }
    const MjkRoomTemplate* tmpl = &game->assets->rooms[next_room];
    MjkWorld_PreloadSection(&game->world, game->assets, next_room, game->crystal_balls_mask);
    MjkPersistSlot* slot = MjkWorld_FindPersistSlot(&game->world, next_room);

    int found_idx = -1;
    uint8_t p_idx = 0u;
    uint8_t found_p_idx = 0u;
    for (uint8_t i = 0u; i < tmpl->object_count; ++i) {
        const MjkRoomObjectDef* d = &tmpl->objects[i];
        if (d->kind == OBJ_KIND_DOOR) {
            if (d->door_slot == target) {
                found_idx   = (int)i;
                found_p_idx = p_idx;
                break;
            }
            if (d->door_slot == alt && found_idx < 0) {
                found_idx   = (int)i;
                found_p_idx = p_idx;
            }
        }
        if (d->is_persistent) { p_idx++; }
    }
    if (found_idx >= 0) {
        game->entry_door_type = tmpl->objects[found_idx].door_slot;
        if (slot && found_p_idx < slot->count) {
            game->entry_door_already_open = slot->objects[found_p_idx].is_open;
        }
    }
}

void MjkGame_ExitRoom(MjkGame* game, uint8_t exit_door_dir) {
    if (!game || !game->assets) {
        return;
    }
    uint8_t prev_room = (game->exit_source_room_id != 0u) ? game->exit_source_room_id : game->current_room_id;
    game->exit_source_room_id = 0u;

    const MjkRoomAdjacency* adj = &game->assets->rooms[prev_room].adj;
    uint8_t dirs[4] = { adj->up, adj->down, adj->right, adj->left };
    uint8_t next_room = dirs[exit_door_dir & 3u];
    uint8_t exit_door = (uint8_t)(game->entry_door_type & 0x0Fu);

    game->current_room_id = next_room;
    resolve_entry_door(game, next_room, exit_door);
    game->state     = GS_LOAD_ROOM;
    game->sub_state = 0u;

    if (prev_room >= 0x90u && prev_room < 0xA0u) {
        game->last_bomb_room_cleared = prev_room;
        game->state                  = GS_ROUND_CLEAR;
        return;
    }
    if (next_room == 0xACu) {
        MjkGame_TriggerTortureRoom(game);
        return;
    }
    if (prev_room >= 0xA0u && prev_room <= 0xA3u) {
        uint8_t etype = (uint8_t)(prev_room & 3u);
        bool match = (etype == 0u && game->crystal_balls_mask == 0u) ||
                     (etype == 1u && game->crystal_balls_mask != 3u) ||
                     (etype == 2u && game->secret_coins < 5u) ||
                     (etype == 3u);
        if (match) {
            game->ending_type  = etype;
            game->ending_state = 0u;
            MjkScoring_AddGdv(&game->scoring, 3);
            game->state     = GS_ENDING;
            game->sub_state = 0u;
        }
    }
}

static void step_load_room(MjkGame* game) {
    MjkWorld_LoadRoom(&game->world, game->assets, game->current_room_id, game->crystal_balls_mask);
    MjkObjects_InitForRoom(&game->objects, game);
    MjkEnemies_InitForRoom(&game->enemies, game->assets,
                           game->last_bomb_room_cleared, game->world.current.is_bomb_room);
    memset(game->scoring.popups, 0, sizeof(game->scoring.popups));
    game->one_up_timer              = 0;
    game->power_coin_palette_active = false;

    const MjkRoomTemplate* tmpl = &game->assets->rooms[game->current_room_id];
    if (tmpl->has_header && (tmpl->section_raw & 0x40u) != 0u) {
        /* Sala del Hermano: 2 Cautivos + Hermano (o Cautivo si ya se obtuvo 1UP) */
        MjkEnemies_Spawn(&game->enemies, ENEMY_CAPTIVE, 104.0f, 216.0f);
        MjkEnemies_Spawn(&game->enemies, ENEMY_CAPTIVE, 168.0f, 216.0f);
        MjkEnemies_Spawn(&game->enemies,
                         game->collected_1up ? ENEMY_CAPTIVE : ENEMY_BROTHER, 136.0f, 216.0f);
    }
    if (tmpl->has_header && tmpl->section_raw != 0u && (tmpl->section_raw & 0x40u) == 0u &&
        game->current_room_id != 0xFCu) {
        game->checkpoint_room_id = game->current_room_id;
        game->entry_door_backup  = game->entry_door_type;
        if ((tmpl->section_raw & 0x80u) != 0u) {
            game->scoring.timer_seconds    = 0;
            game->scoring.timer_frozen     = false;
            game->torture_return_room_id   = game->current_room_id;
            game->torture_return_door_type = game->entry_door_type;
        }
    }
    game->state     = GS_DRAW_SCREEN;
    game->sub_state = 0u;
}

static void enter_playing_state(MjkGame* game) {
    if (game->scoring.timer_seconds == 0) {
        MjkScoring_ResetStageTimer(&game->scoring, 60);
    }
    MjkGame_QueueSound(game, (MjkSoundId)game->assets->music_options[game->world.current.music_index & 7u]);
    MjkPlayer_Init(&game->player, game->cinematic_jack_x, game->cinematic_jack_y);
    game->state     = GS_PLAYING;
    game->sub_state = 0u;
}

static void step_enter_room(MjkGame* game) {
    uint8_t x0 = 0u, y0 = 0u, x1 = 0u, y1 = 0u, closed_mt = 0u;
    MjkAssets_GetDoorLayout(game->entry_door_type, &x0, &y0, &x1, &y1, &closed_mt, NULL);

    if (game->sub_state == 0u) {
        uint8_t idx = (uint8_t)(game->entry_door_type & 0x0Fu);
        game->cinematic_jack_y = (float)game->assets->door_entry_xy[idx][0];
        game->cinematic_jack_x = (float)game->assets->door_entry_xy[idx][1];
        if (game->entry_door_already_open) {
            game->state_timer = 24;
            game->sub_state   = 2u;
        } else {
            MjkGame_QueueSound(game, SOUND_DOOR);
            game->state_timer = 72;
            game->sub_state   = 1u;
        }
    } else if (game->sub_state == 1u) {
        if (game->state_timer > 0) { game->state_timer--; }
        if (game->state_timer == 40) {
            MjkWorld_SetVisualMetatile(&game->world.current, game->assets, x1, y1, (uint8_t)(closed_mt + 3u));
        } else if (game->state_timer == 32) {
            MjkWorld_SetVisualMetatile(&game->world.current, game->assets, x0, y0, (uint8_t)(closed_mt + 2u));
        }
        if (game->state_timer == 0) {
            game->state_timer = 24;
            game->sub_state   = 2u;
        }
    } else if (game->sub_state == 2u) {
        uint8_t dir = (uint8_t)((game->entry_door_type >> 2u) & 3u);
        game->cinematic_jack_y += (float)game->assets->door_walk_vec[dir][0];
        game->cinematic_jack_x += (float)game->assets->door_walk_vec[dir][1];
        if (game->state_timer > 0) { game->state_timer--; }
        if (game->state_timer == 0) {
            if (game->entry_door_already_open) {
                enter_playing_state(game);
            } else {
                MjkGame_QueueSound(game, SOUND_DOOR);
                game->state_timer = 72;
                game->sub_state   = 3u;
            }
        }
    } else {
        if (game->state_timer > 0) { game->state_timer--; }
        if (game->state_timer == 40) {
            MjkWorld_SetVisualMetatile(&game->world.current, game->assets, x0, y0, closed_mt);
        } else if (game->state_timer == 32) {
            MjkWorld_SetVisualMetatile(&game->world.current, game->assets, x1, y1, (uint8_t)(closed_mt + 1u));
        }
        if (game->state_timer == 0) {
            enter_playing_state(game);
        }
    }
}

static void step_playing(MjkGame* game, const MjkInputStep* input) {
    if (input && (input->pressed & MJK_INPUT_START) != 0u) {
        game->paused = !game->paused;
        MjkGame_QueueSound(game, game->paused ? SOUND_PAUSE : SOUND_UNPAUSE);
    }
    if (game->paused) {
        return;
    }
    if (game->in_torture_mode) {
        game->scoring.timer_frozen = true;
    }

    MjkPlayerStepCtx pctx;
    memset(&pctx, 0, sizeof(pctx));
    pctx.in_torture_room     = game->in_torture_mode;
    pctx.mighty_level        = &game->mighty_level;
    pctx.mighty_presses_left = &game->mighty_presses_left;

    MjkActionResult pres = MjkPlayer_Update(&game->player, input, &game->world, game->assets, &pctx);
    if (pctx.sfx_triggered != SOUND_SILENCE) {
        if (pctx.sfx_triggered == SOUND_DEATH) { MjkGame_QueueSound(game, SOUND_SILENCE); }
        MjkGame_QueueSound(game, pctx.sfx_triggered);
    }
    if (pctx.award_jump_10pts) {
        MjkGame_AddScore(game, SCORE_10);
    }
    if (game->player.just_jumped && game->in_torture_mode) {
        MjkGame_OnTortureJump(game);
        if (game->state != GS_PLAYING) { return; }
    }
    if (pres == ACTION_PLAYER_DIED) {
        if (game->player.fire_tile_died_to == 0x5Du && game->current_room_id == 0xFCu) {
            game->door_1e_deaths++;
        }
        MjkGame_LoseLife(game);
        return;
    }

    bool allow_cam = !game->player.is_dying ||
                     (!game->world.current.is_horizontal && !game->world.current.is_bomb_room);
    if (allow_cam) {
        MjkWorld_UpdateCamera(&game->world, game->assets,
                              &game->player.x, &game->player.y,
                              game->player.vx, game->player.vy,
                              game->crystal_balls_mask);
        if (game->world.camera.room_crossed) {
            game->current_room_id = game->world.camera.current_room_id;
        }
    }

    if (!game->player.is_dying) {
        if (game->one_up_timer > 0) { game->one_up_timer--; }
        MjkScoring_TickPopups(&game->scoring, game->world.camera.delta_x, game->world.camera.delta_y);
        if (MjkObjects_Update(&game->objects, game, input) != ACTION_CONTINUE) { return; }
        if (MjkDoors_Update(game) || MjkDoors_CheckPlayerEntry(game) != ACTION_CONTINUE) { return; }
        if (MjkEnemies_Update(&game->enemies, game) != ACTION_CONTINUE) { return; }
        if (MjkScoring_TickTimer(&game->scoring)) {
            game->state     = GS_TIME_OVER;
            game->sub_state = 0u;
        }
    }
}

void MjkGame_StepFrame(MjkGame* game, const MjkInputStep* input) {
    if (!game || !game->assets) {
        return;
    }
    game->sound_queue_count = 0u;

    if (game->mode == SCREEN_TITLE) {
        if (MjkScreens_StepTitle(game, input)) {
            game->mode      = SCREEN_INGAME;
            game->state     = GS_INIT;
            game->sub_state = 0u;
        }
        return;
    }

    switch (game->state) {
        case GS_INIT:
            MjkScoring_ResetNewGame(&game->scoring);
            MjkWorld_Reset(&game->world);
            memset(&game->objects, 0, sizeof(game->objects));
            memset(&game->enemies, 0, sizeof(game->enemies));
            memset(&game->player, 0, sizeof(game->player));
            game->checkpoint_room_id        = 0u;
            game->crystal_balls_mask        = 0u;
            game->secret_coins              = 0u;
            game->in_torture_mode           = false;
            game->last_bomb_room_cleared    = 0u;
            game->extra_lives_found         = 0u;
            game->mighty_coins              = 0u;
            game->mighty_level              = 0u;
            game->mighty_presses_left       = 0u;
            game->collected_1up             = false;
            game->one_up_timer              = 0;
            game->door_flag_09              = 0u;
            game->door_flag_1f              = 0u;
            game->door_1e_deaths            = 0u;
            game->entry_door_already_open   = false;
            game->power_coin_palette_active = false;
            game->scoring.timer_seconds     = 0;
            MjkGame_QueueSound(game, SOUND_SILENCE);
            game->current_room_id = 1u;
            game->entry_door_type = 0x0Fu;
            game->state           = GS_ROUND_INTRO;
            game->sub_state       = 0u;
            break;

        case GS_ROUND_INTRO:
            if (game->sub_state == 0u) {
                game->state_timer = 96;
                game->sub_state   = 1u;
            } else if (--game->state_timer <= 0) {
                game->state     = GS_LOAD_ROOM;
                game->sub_state = 0u;
            }
            break;

        case GS_LOAD_ROOM:
            step_load_room(game);
            break;

        case GS_DRAW_SCREEN:
            if (game->world.current.is_bomb_room) {
                game->mighty_level        = 0u;
                game->mighty_presses_left = 0u;
            }
            game->state     = GS_ENTER_ROOM;
            game->sub_state = 0u;
            break;

        case GS_ENTER_ROOM:
            step_enter_room(game);
            break;

        case GS_PLAYING:
            step_playing(game, input);
            break;

        case GS_TIME_OVER:
            if (game->sub_state == 0u) {
                game->state_timer = 128;
                MjkGame_QueueSound(game, SOUND_DEATH);
                game->sub_state = 1u;
            } else if (--game->state_timer <= 0) {
                MjkGame_LoseLife(game);
            }
            break;

        case GS_TORTURE_ROOM:
            if (game->sub_state == 0u) {
                game->in_torture_mode = true;
                game->state_timer     = 192;
                MjkGame_QueueSound(game, SOUND_SILENCE);
                MjkGame_QueueSound(game, SOUND_YOU_ARE_GREEDY);
                game->sub_state = 1u;
            } else if (--game->state_timer <= 0) {
                game->current_room_id   = 0xACu;
                game->entry_door_type   = 0x00u;
                game->greedy_jumps_left = 50; /* 50 saltos en REV_US */
                game->state             = GS_LOAD_ROOM;
                game->sub_state         = 0u;
            }
            break;

        case GS_ROUND_CLEAR:
            MjkScreens_StepRoundClear(game);
            break;

        case GS_EXIT_ROOM:
            MjkGame_ExitRoom(game, (uint8_t)(game->entry_door_type >> 2u));
            break;

        case GS_GAME_OVER:
            MjkScreens_StepGameOver(game);
            break;

        case GS_ANIMATE_DOOR_OPEN:
            if (game->sub_state == 0u) {
                MjkGame_QueueSound(game, SOUND_DOOR);
                game->state_timer = 72;
                game->sub_state   = 1u;
            } else if (--game->state_timer <= 0) {
                game->state     = GS_PLAYING;
                game->sub_state = 0u;
            }
            break;

        case GS_ENDING:
            MjkScreens_StepEnding(game, input);
            break;

        default:
            game->state = GS_INIT;
            break;
    }
}
