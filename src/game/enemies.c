/**
 * @file enemies.c
 * @brief IA de enemigos (Momia + 6 arquetipos), Power Coin (320f) y coleccionables (R-11, R-12).
 */

#include "mjk/enemies.h"
#include "mjk/game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { EF_RIGHT = 0x01u, EF_LEFT = 0x02u, EF_FLOOR = 0x04u, EF_CEIL = 0x08u };

static bool is_monster_kind(MjkEnemyKind k) {
    return (unsigned)k <= (unsigned)ENEMY_HORUS || k == ENEMY_MUMMY1 || k == ENEMY_MUMMY2;
}

static uint8_t archetype_sprite(MjkEnemyKind k, uint8_t anim) {
    uint8_t step6 = (uint8_t)(anim / 6u);
    uint8_t phase = (uint8_t)(step6 & 1u);
    static const uint8_t k_hanezo[3] = { 0x12u, 0x13u, 0x13u };
    static const uint8_t k_extra[3]  = { 0x1Bu, 0x1Cu, 0x1Du };
    static const uint8_t k_bonus[3]  = { 0x19u, 0x1Au, 0x1Du };
    switch (k) {
        case ENEMY_MUMMY:
        case ENEMY_MUMMY1:
        case ENEMY_MUMMY2:     return (uint8_t)(0x0Eu + phase);
        case ENEMY_HANEZO:     return k_hanezo[step6 % 3u];
        case ENEMY_GEJISHOGUN: return (uint8_t)(0x0Cu + phase);
        case ENEMY_GAMEIDO:    return (uint8_t)(0x16u + (step6 % 3u));
        case ENEMY_DOKURON:    return (uint8_t)(0x14u + phase);
        case ENEMY_DESUFA:     return 0x07u;
        case ENEMY_HORUS:      return (uint8_t)(0x08u + phase);
        case ENEMY_POWER_COIN: return 0x20u;
        case ENEMY_EXTRA_COIN: return k_extra[step6 % 3u];
        case ENEMY_SECRET_COIN:return 0x1Fu;
        case ENEMY_CAPTIVE:
        case ENEMY_BROTHER:    return 0x23u;
        case ENEMY_BALLOON:    return 0x24u;
        case ENEMY_BONUS_COIN: return k_bonus[step6 % 3u];
        default:               return 0x0Eu;
    }
}

static uint8_t archetype_palette(MjkEnemyKind k) {
    switch (k) {
        case ENEMY_HANEZO:
        case ENEMY_GEJISHOGUN: return 2u;
        case ENEMY_DESUFA:
        case ENEMY_EXTRA_COIN:
        case ENEMY_SECRET_COIN:
        case ENEMY_BALLOON:
        case ENEMY_BONUS_COIN: return 0u;
        case ENEMY_POWER_COIN:
        case ENEMY_BROTHER:    return 1u;
        default:               return 3u;
    }
}

static uint8_t enemy_collision_flags(const MjkWorld* world, float ex, float ey) {
    int wx = (int)floorf(ex) + (int)floorf(world->camera.scroll_x);
    int wy = (int)floorf(ey) + (int)floorf(world->camera.scroll_y);
    uint8_t cf = 0u;
    if (MjkWorld_IsEnemyPointSolid(world, wx + 8, wy - 4) || MjkWorld_IsEnemyPointSolid(world, wx + 8, wy + 4)) { cf |= EF_RIGHT; }
    if (MjkWorld_IsEnemyPointSolid(world, wx - 8, wy - 4) || MjkWorld_IsEnemyPointSolid(world, wx - 8, wy + 4)) { cf |= EF_LEFT; }
    if (MjkWorld_IsEnemyPointSolid(world, wx - 4, wy + 8) || MjkWorld_IsEnemyPointSolid(world, wx + 4, wy + 8)) { cf |= EF_FLOOR; }
    if (MjkWorld_IsEnemyPointSolid(world, wx - 4, wy - 8) || MjkWorld_IsEnemyPointSolid(world, wx + 4, wy - 8)) { cf |= EF_CEIL; }
    return cf;
}

static float advance_fall_speed(float* progress, float rate, const MjkAssets* assets) {
    float next = *progress + rate;
    if (next >= 128.0f) {
        next = 127.0f + (next - floorf(next));
    }
    *progress = next;
    int hi = (int)floorf(next);
    int idx = (hi < 64) ? hi : ((hi ^ 0xFF) & 0x3F);
    float mag = assets->jump_speed[idx];
    return (hi < 64) ? -mag : mag;
}

static void init_mummy_fall(MjkEnemy* e, int transform_reload) {
    e->falling         = true;
    e->fall_progress   = 64.0f;
    e->vx              = 0.0f;
    e->vy              = 0.0f;
    e->edge_turned     = false;
    e->transform_timer = transform_reload;
}

static void update_gameido_vel(MjkEnemy* e, const MjkGame* game, const MjkEnemySystem* sys, uint8_t cf) {
    uint8_t div = game->assets->gameido_div[sys->speed_tier & 7u];
    if (div == 0u) { div = 0x4Cu; }
    float dx = floorf(game->player.x) - floorf(e->x);
    float dy = floorf(game->player.y) - floorf(e->y);
    float vx = fabsf(dx) / (float)div;
    float vy = fabsf(dy) / (float)div;
    bool neg_x = (dx >= 0.0f) ? ((cf & EF_RIGHT) != 0u) : ((cf & EF_LEFT) == 0u);
    bool neg_y = (dy >= 0.0f) ? ((cf & EF_FLOOR) != 0u) : ((cf & EF_CEIL) == 0u);
    e->vx = neg_x ? -vx : vx;
    e->vy = neg_y ? -vy : vy;
}

static void transform_mummy(MjkEnemy* e, const MjkGame* game, const MjkEnemySystem* sys) {
    e->kind              = (MjkEnemyKind)((game->player.jump_press_count % 6u) + 2u);
    e->spawn_flash_timer = 32;
    e->palette_idx       = archetype_palette(e->kind);
    e->ai_timer          = 0u;
    e->ai_dir            = EF_RIGHT;
    e->falling           = false;
    e->edge_turned       = false;

    float spd = game->assets->enemy_speed[sys->speed_tier & 7u];
    switch (e->kind) {
        case ENEMY_HANEZO:     e->vx = spd; e->vy = spd; break;
        case ENEMY_GEJISHOGUN: e->vx = (game->player.x < e->x) ? -spd : spd;
                               e->vy = (game->player.y < e->y) ? -spd : spd; break;
        case ENEMY_GAMEIDO:    e->ai_dir = EF_LEFT; update_gameido_vel(e, game, sys, 0u); break;
        case ENEMY_DOKURON:    e->vx = (game->player.x < e->x) ? -spd : spd; e->vy = spd; break;
        case ENEMY_DESUFA:     e->vx = spd; e->vy = (game->player.y < e->y) ? -spd : spd; break;
        case ENEMY_HORUS:      e->vx = spd; e->vy = 0.0f; break;
        default: break;
    }
}

static void step_mummy(MjkEnemy* e, const MjkGame* game, const MjkEnemySystem* sys, uint8_t cf) {
    if ((cf & EF_FLOOR) != 0u) {
        if (e->falling) {
            e->falling = false;
            e->vy      = 0.0f;
            e->y       = (float)(((int)floorf(e->y)) & ~7);
            if ((game->world.camera.room_flags & (ROOM_FLAG_SINGLE_SCREEN | ROOM_FLAG_SCROLL_STOP)) != 0u &&
                e->y >= 224.0f) {
                e->y = 216.0f;
            }
            e->vx          = (e->x >= game->player.x) ? -0.5f : 0.5f;
            e->edge_turned = false;
            if (game->world.current.is_bomb_room && e->y >= 216.0f) {
                transform_mummy(e, game, sys);
                return;
            }
        }
        if (e->transform_timer > 0) {
            e->transform_timer--;
        } else if (!game->world.current.is_bomb_room) {
            transform_mummy(e, game, sys);
            return;
        }
        if ((cf & (EF_RIGHT | EF_LEFT)) != 0u) {
            if (!e->edge_turned) { e->edge_turned = true; e->vx = -e->vx; }
        } else {
            e->edge_turned = false;
        }
    } else if (e->falling) {
        e->vy = advance_fall_speed(&e->fall_progress, 1.375f, game->assets);
    } else {
        if (e->transform_timer > 0) {
            e->transform_timer--;
            if (!e->edge_turned) { e->edge_turned = true; e->vx = -e->vx; }
        } else if (game->world.current.is_bomb_room) {
            init_mummy_fall(e, sys->transform_reload);
        } else {
            transform_mummy(e, game, sys);
        }
    }
}

static void step_horus(MjkEnemy* e, const MjkGame* game, float spd, uint8_t cf) {
    if (cf == 0u) {
        if (e->ai_timer > 0u) { e->ai_timer--; return; }
        e->ai_timer = 16u;
    }
    float dx = game->player.x - e->x;
    float dy = game->player.y - e->y;
    uint8_t px_d = (dx >= 0.0f) ? EF_RIGHT : EF_LEFT;
    uint8_t ox_d = (dx >= 0.0f) ? EF_LEFT  : EF_RIGHT;
    uint8_t py_d = (dy >= 0.0f) ? EF_FLOOR : EF_CEIL;
    uint8_t oy_d = (dy >= 0.0f) ? EF_CEIL  : EF_FLOOR;
    bool horiz = (fabsf(dx) > fabsf(dy));
    uint8_t pref[4] = { horiz ? px_d : py_d, horiz ? py_d : px_d, horiz ? ox_d : oy_d, horiz ? oy_d : ox_d };
    uint8_t opp = (e->ai_dir == EF_RIGHT) ? EF_LEFT : (e->ai_dir == EF_LEFT) ? EF_RIGHT :
                  (e->ai_dir == EF_FLOOR) ? EF_CEIL : EF_FLOOR;
    uint8_t chosen = opp;
    for (int k = 0; k < 4; ++k) {
        if ((pref[k] & cf) == 0u && pref[k] != opp) { chosen = pref[k]; break; }
    }
    e->ai_dir = chosen;
    e->vx = (chosen == EF_RIGHT) ? spd : (chosen == EF_LEFT) ? -spd : 0.0f;
    e->vy = (chosen == EF_FLOOR) ? spd : (chosen == EF_CEIL) ? -spd : 0.0f;
}

static void step_monster_ai(MjkEnemy* e, const MjkGame* game, const MjkEnemySystem* sys) {
    uint8_t raw_cf = enemy_collision_flags(&game->world, e->x, e->y);
    uint8_t cf = raw_cf;
    if (e->vx > 0.0f) { cf &= (uint8_t)~EF_LEFT; }  else if (e->vx < 0.0f) { cf &= (uint8_t)~EF_RIGHT; }
    if (e->vy > 0.0f) { cf &= (uint8_t)~EF_CEIL; }  else if (e->vy < 0.0f) { cf &= (uint8_t)~EF_FLOOR; }
    float spd = game->assets->enemy_speed[sys->speed_tier & 7u];

    switch (e->kind) {
        case ENEMY_MUMMY: step_mummy(e, game, sys, raw_cf); break;
        case ENEMY_HANEZO:
            if ((cf & EF_RIGHT) != 0u) { e->vx = -spd; } else if ((cf & EF_LEFT) != 0u) { e->vx = spd; }
            if ((cf & EF_FLOOR) != 0u) { e->vy = -spd; } else if ((cf & EF_CEIL) != 0u) { e->vy = spd; }
            break;
        case ENEMY_GEJISHOGUN:
            e->vx = (game->player.x < e->x) ? (((cf & EF_LEFT) != 0u) ? spd : -spd)
                                            : (((cf & EF_RIGHT) != 0u) ? -spd : spd);
            if (e->ai_timer > 0u) { e->ai_timer--; if (cf == 0u) { break; } }
            e->vy = (game->player.y < e->y) ? (((cf & EF_CEIL) != 0u) ? spd : -spd)
                                            : (((cf & EF_FLOOR) != 0u) ? -spd : spd);
            if (cf != 0u) { e->ai_timer = 48u; }
            break;
        case ENEMY_GAMEIDO:
            if (cf != 0u) { update_gameido_vel(e, game, sys, cf); }
            break;
        case ENEMY_DOKURON: {
            e->ai_timer++;
            float tx = ((e->ai_timer & 0x20u) != 0u) ? (game->player.x - 16.0f) : (game->player.x + 16.0f);
            e->vx = (tx < e->x) ? (((cf & EF_LEFT) != 0u) ? spd : -spd) : (((cf & EF_RIGHT) != 0u) ? -spd : spd);
            if ((cf & EF_FLOOR) != 0u) { e->vy = -spd; } else if ((cf & EF_CEIL) != 0u) { e->vy = spd; }
            break;
        }
        case ENEMY_DESUFA: {
            e->ai_timer++;
            float ty = ((e->ai_timer & 0x20u) != 0u) ? (game->player.y - 16.0f) : (game->player.y + 16.0f);
            e->vy = (ty < e->y) ? (((cf & EF_CEIL) != 0u) ? spd : -spd) : (((cf & EF_FLOOR) != 0u) ? -spd : spd);
            if ((cf & EF_RIGHT) != 0u) { e->vx = -spd; } else if ((cf & EF_LEFT) != 0u) { e->vx = spd; }
            break;
        }
        case ENEMY_HORUS: step_horus(e, game, spd, cf); break;
        default: break;
    }
    if (e->vx != 0.0f) { e->facing_left = (e->vx < 0.0f); }
}

static bool step_collectible_motion(MjkEnemy* e, const MjkGame* game) {
    if (e->kind == ENEMY_POWER_COIN) {
        if (e->ai_dir == 0u) {
            if (e->ai_timer > 0u) {
                if (e->y + e->vy >= 0.0f) { e->y += e->vy; }
                if (--e->ai_timer == 0u) { e->vy = 0.0f; }
            }
        } else {
            uint8_t cf = enemy_collision_flags(&game->world, e->x, e->y);
            float spd = game->assets->enemy_speed[5];
            if ((cf & EF_RIGHT) != 0u) { e->vx = -spd; } else if ((cf & EF_LEFT) != 0u) { e->vx = spd; }
            if ((cf & EF_FLOOR) != 0u) { e->vy = -spd; } else if ((cf & EF_CEIL) != 0u) { e->vy = spd; }
            e->x += e->vx;
            e->y += e->vy;
            if (e->x < 0.0f || e->x >= 256.0f || e->y < 0.0f || e->y >= 240.0f) {
                e->active = false;
                return false;
            }
        }
    } else if (e->kind == ENEMY_BALLOON) {
        e->y += e->vy;
        if (e->y < 0.0f) { e->active = false; return false; }
    } else if ((e->kind == ENEMY_BONUS_COIN || e->kind == ENEMY_EXTRA_COIN) &&
               game->world.current.is_bomb_room) {
        uint8_t cf = enemy_collision_flags(&game->world, e->x, e->y);
        float spd = game->assets->enemy_speed[5];
        if ((cf & EF_FLOOR) != 0u) {
            if (e->falling) {
                e->falling = false;
                e->vy      = 0.0f;
                e->y       = (float)(((int)floorf(e->y)) & ~7);
                e->vx      = (e->x >= game->player.x) ? -0.5f : 0.5f;
            } else if ((cf & EF_RIGHT) != 0u) { e->vx = -spd; }
            else if ((cf & EF_LEFT) != 0u)    { e->vx = spd; }
        } else {
            if (!e->falling) { e->falling = true; e->fall_progress = 64.0f; e->vx = 0.0f; }
            e->vy = advance_fall_speed(&e->fall_progress, 0.9375f, game->assets);
        }
        e->x += e->vx;
        e->y += e->vy;
        if (e->x < 0.0f || e->x >= 256.0f || e->y < 0.0f || e->y >= 240.0f) {
            e->active = false;
            return false;
        }
    }
    return true;
}

void MjkEnemies_InitForRoom(MjkEnemySystem* sys,
                            const MjkAssets* assets,
                            uint8_t last_bomb_cleared,
                            bool is_bomb_room) {
    if (!sys) { return; }
    memset(sys, 0, sizeof(*sys));
    sys->spawn_reload     = 0xA0;
    sys->transform_reload = is_bomb_room ? 0x80 : 0x40;
    if (assets) {
        uint8_t tier = (uint8_t)(last_bomb_cleared & 0x0Fu);
        sys->speed_tier       = assets->difficulty_table[tier][0];
        sys->spawn_reload     = assets->difficulty_table[tier][1];
        sys->transform_reload = is_bomb_room ? assets->difficulty_table[tier][2]
                                             : assets->difficulty_table[tier][3];
    }
    sys->spawn_timer = sys->spawn_reload;
}

void MjkEnemies_ActivatePowerCoin(MjkEnemySystem* sys, MjkGame* game) {
    if (!sys) { return; }
    sys->transmute_timer  = MJK_POWER_COIN_FRAMES;
    sys->transmute_streak = 0u;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (sys->slots[i].active && is_monster_kind(sys->slots[i].kind)) {
            sys->slots[i].transmuted = true;
        }
    }
    if (game) {
        game->power_coin_palette_active = true;
        MjkGame_QueueSound(game, SOUND_POWER_COIN_MUSIC);
    }
}

bool MjkEnemies_SpawnPowerCoinBombRoom(MjkEnemySystem* sys, const MjkAssets* assets) {
    if (!sys || !assets) { return false; }
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (!sys->slots[i].active) {
            MjkEnemy* e = &sys->slots[i];
            memset(e, 0, sizeof(*e));
            e->active      = true;
            e->kind        = ENEMY_POWER_COIN;
            e->x           = 128.0f;
            e->y           = 120.0f;
            float spd      = assets->enemy_speed[5];
            e->vx          = spd;
            e->vy          = spd;
            e->ai_dir      = 1u;
            e->sprite_id   = archetype_sprite(e->kind, 0u);
            e->palette_idx = archetype_palette(e->kind);
            return true;
        }
    }
    return false;
}

bool MjkEnemies_Spawn(MjkEnemySystem* sys, MjkEnemyKind kind, float x, float y) {
    if (!sys) { return false; }
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (!sys->slots[i].active) {
            MjkEnemy* e = &sys->slots[i];
            memset(e, 0, sizeof(*e));
            e->active = true;
            e->x      = x;
            e->y      = y;
            if (kind == ENEMY_MUMMY || kind == ENEMY_MUMMY1 || kind == ENEMY_MUMMY2) {
                e->kind              = ENEMY_MUMMY;
                e->spawn_flash_timer = 32;
                init_mummy_fall(e, sys->transform_reload);
                if (sys->transmute_timer > 0) { e->transmuted = true; }
                sys->alive_count++;
            } else {
                e->kind = kind;
                if (kind == ENEMY_POWER_COIN) {
                    e->vy = -1.0f; e->ai_timer = 8u; e->ai_dir = 0u;
                } else if (kind == ENEMY_BALLOON) {
                    e->vy = -1.0f; e->spawn_flash_timer = 12;
                } else if (kind == ENEMY_BONUS_COIN || kind == ENEMY_EXTRA_COIN) {
                    e->falling = true; e->fall_progress = 64.0f;
                }
            }
            e->sprite_id   = archetype_sprite(e->kind, 0u);
            e->palette_idx = archetype_palette(e->kind);
            return true;
        }
    }
    return false;
}

static bool resolve_spawn_candidate(uint8_t yx, uint8_t room_flags, bool is_horiz,
                                    int scroll_x, int scroll_y, float* out_x, float* out_y) {
    if (yx == 0u) { return false; }
    if ((room_flags & (ROOM_FLAG_SINGLE_SCREEN | ROOM_FLAG_AT_SCROLL_EDGE)) == 0u) {
        uint8_t coord_hi  = is_horiz ? (uint8_t)((yx & 0x0Fu) << 4u) : (uint8_t)(yx & 0xF0u);
        uint8_t scroll_hi = is_horiz ? (uint8_t)(scroll_x & 0xFF) : (uint8_t)(scroll_y & 0xFF);
        if (((coord_hi ^ scroll_hi) & 0x80u) == 0u) { return false; }
    }
    uint8_t sy_sc = (uint8_t)(scroll_y & 0xFF);
    if (sy_sc >= 0x78u) { sy_sc = (uint8_t)(sy_sc + 0x10u); }
    uint8_t sy = (uint8_t)(((yx & 0xF0u) | 8u) - sy_sc);
    uint8_t sx = (uint8_t)((((yx & 0x0Fu) << 4u) | 8u) - (uint8_t)(scroll_x & 0xFF));
    if (sx == 0u || sy == 0u || sy >= 240u) { return false; }
    *out_x = (float)sx;
    *out_y = (float)sy;
    return true;
}

static void spawn_mummy(MjkEnemySystem* sys, const MjkGame* game) {
    int scx = (int)floorf(game->world.camera.scroll_x);
    int scy = (int)floorf(game->world.camera.scroll_y);
    int wx  = (int)floorf(game->player.x) + scx;
    int wy  = (int)floorf(game->player.y) + scy;
    bool horiz = game->world.current.is_horizontal;
    bool in_nb = game->world.has_neighbor && (horiz ? (wx >= 256) : (wy >= 240));
    uint8_t rid = in_nb ? game->world.neighbor.room_id : game->world.current.room_id;

    uint8_t c0 = game->assets->enemy_spawn_pos[rid][0];
    uint8_t c1 = game->assets->enemy_spawn_pos[rid][1];
    if (c0 == 0u) { return; }

    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    bool v0 = resolve_spawn_candidate(c0, game->world.camera.room_flags, horiz, scx, scy, &x0, &y0);
    bool v1 = resolve_spawn_candidate(c1, game->world.camera.room_flags, horiz, scx, scy, &x1, &y1);
    if (!v0 && !v1) { return; }

    float sx = v0 ? x0 : x1, sy = v0 ? y0 : y1;
    if (v0 && v1) {
        float d0 = fabsf(x0 - game->player.x) + fabsf(y0 - game->player.y);
        float d1 = fabsf(x1 - game->player.x) + fabsf(y1 - game->player.y);
        bool pick1 = ((game->world.camera.room_flags & ROOM_FLAG_SINGLE_SCREEN) != 0u) ? (d1 > d0) : (d1 < d0);
        if (pick1) { sx = x1; sy = y1; }
    }
    MjkEnemies_Spawn(sys, ENEMY_MUMMY, sx, sy);
}

static void handle_pickup(MjkEnemy* e, MjkEnemySystem* sys, MjkGame* game) {
    if (e->transmuted) {
        static const MjkScoreId k_chain[6] = {
            SCORE_200, SCORE_300, SCORE_500, SCORE_800, SCORE_1200, SCORE_2000
        };
        uint8_t s_idx = (sys->transmute_streak < 6u) ? sys->transmute_streak : 5u;
        MjkGame_AddScore(game, k_chain[s_idx]);
        MjkGame_QueueSound(game, SOUND_COIN_COLLECTED);
        if (game->scoring.gold_coins < 40u) { game->scoring.gold_coins++; }
        if (sys->transmute_streak < 5u) { sys->transmute_streak++; }
        e->active = false;
        if (sys->alive_count > 0u) { sys->alive_count--; }
        return;
    }

    switch (e->kind) {
        case ENEMY_POWER_COIN:
            MjkEnemies_ActivatePowerCoin(sys, game);
            e->active = false;
            break;
        case ENEMY_EXTRA_COIN:
        case ENEMY_BROTHER:
            MjkGame_AddScore(game, (MjkScoreId)0x1E); /* 3000 pts + 1UP */
            game->scoring.lives++;
            game->extra_lives_found++;
            game->collected_1up = true;
            game->one_up_timer  = 64;
            MjkGame_QueueSound(game, SOUND_1UP_CAPTIVE);
            e->active = false;
            break;
        case ENEMY_SECRET_COIN:
            if (game->mighty_level == 3u) {
                game->secret_coins++;
                MjkScoring_AddGdv(&game->scoring, 1);
                MjkGame_AddScore(game, SCORE_50000);
                MjkGame_QueueSound(game, SOUND_COIN_COLLECTED);
                e->active = false;
            }
            break;
        case ENEMY_CAPTIVE:
            MjkGame_AddScore(game, SCORE_2000);
            MjkGame_QueueSound(game, SOUND_BONUS_STATUE);
            e->active = false;
            break;
        case ENEMY_BALLOON:
            MjkGame_AddScore(game, SCORE_10000);
            MjkGame_QueueSound(game, SOUND_BOMB_LIT);
            e->active = false;
            break;
        case ENEMY_BONUS_COIN:
            if (game->scoring.score_multiplier < 4u) { game->scoring.score_multiplier++; }
            MjkGame_AddScore(game, SCORE_1000);
            MjkGame_QueueSound(game, SOUND_COIN_COLLECTED);
            e->active = false;
            break;
        default:
            e->active = false;
            break;
    }
}

MjkActionResult MjkEnemies_Update(MjkEnemySystem* sys, MjkGame* game) {
    if (!sys || !game || !game->assets) {
        return ACTION_CONTINUE;
    }

    if (sys->post_transmute_grace > 0) { sys->post_transmute_grace--; }

    if (sys->transmute_timer > 0) {
        if (--sys->transmute_timer == 0) {
            for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) { sys->slots[i].transmuted = false; }
            game->power_coin_palette_active = false;
            game->objects.power_coin_armed  = false;
            game->objects.power_coin_acc    = 0u;
            uint8_t m_idx = (uint8_t)(game->world.current.music_index & 7u);
            if (m_idx == 0u) { m_idx = 1u; }
            MjkGame_QueueSound(game, (MjkSoundId)game->assets->music_options[m_idx]);
            sys->post_transmute_grace = 32;
        }
    } else {
        if (sys->spawn_timer > 0) { sys->spawn_timer--; }
        if (sys->spawn_timer == 0) {
            sys->spawn_timer = sys->spawn_reload;
            if (sys->alive_count < MJK_ENEMY_MAX_ALIVE) { spawn_mummy(sys, game); }
        }
    }

    sys->alive_count = 0u;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        MjkEnemy* e = &sys->slots[i];
        if (!e->active) { continue; }
        e->x += game->world.camera.delta_x;
        e->y += game->world.camera.delta_y;
        if (e->x < 0.0f || e->x >= 256.0f || e->y < 0.0f || e->y >= 240.0f) {
            e->active = false;
            continue;
        }
        e->anim_timer++;

        if (is_monster_kind(e->kind)) {
            sys->alive_count++;
            if (!e->transmuted) {
                if (e->spawn_flash_timer > 0) {
                    e->spawn_flash_timer--;
                } else {
                    step_monster_ai(e, game, sys);
                    e->x += e->vx;
                    e->y += e->vy;
                    if (e->x < 0.0f || e->x >= 256.0f || e->y < 0.0f || e->y >= 240.0f) {
                        e->active = false;
                        if (sys->alive_count > 0u) { sys->alive_count--; }
                        continue;
                    }
                }
            }
        } else {
            if (e->spawn_flash_timer > 0) { e->spawn_flash_timer--; }
            if (!step_collectible_motion(e, game)) { continue; }
        }

        float dx = floorf(e->x) - floorf(game->player.x);
        float dy = floorf(e->y) - floorf(game->player.y);
        if (dx > -8.0f && dx < 8.0f && dy > -8.0f && dy < 8.0f) {
            if (e->transmuted || !is_monster_kind(e->kind)) {
                if (e->kind != ENEMY_BALLOON || e->spawn_flash_timer == 0) {
                    handle_pickup(e, sys, game);
                }
            } else if (!game->player.is_dying && e->spawn_flash_timer == 0 && sys->post_transmute_grace == 0) {
                MjkPlayer_Kill(&game->player, 0u);
                MjkGame_QueueSound(game, SOUND_SILENCE);
                MjkGame_QueueSound(game, SOUND_DEATH);
            }
        }

        if (e->active) {
            uint8_t step6 = (uint8_t)(e->anim_timer / 6u);
            if (e->kind == ENEMY_MUMMY && e->falling) {
                e->sprite_id = (uint8_t)(0x10u + (step6 & 1u));
            } else if (e->kind == ENEMY_HORUS && e->vy != 0.0f && e->vx == 0.0f) {
                e->sprite_id = (uint8_t)(0x0Au + (step6 & 1u));
            } else {
                e->sprite_id = archetype_sprite(e->kind, e->anim_timer);
            }
        }
    }
    return ACTION_CONTINUE;
}
