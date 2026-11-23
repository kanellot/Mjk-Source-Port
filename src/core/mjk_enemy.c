/**
 * @file mjk_enemy.c
 * @brief Implementación de IA de enemigos, colisiones, mutación y Power Coin (asm/src/prg.asm:8171-10159).
 */

#include "mjk_enemy_internal.h"
#include "mjk_game.h"
#include "mjk_hud.h"
#include <string.h>

enum { EF_RIGHT = 0x01u, EF_LEFT = 0x02u, EF_FLOOR = 0x04u, EF_CEIL = 0x08u };

static const uint8_t k_enemy_partial_masks[10] = {
    0xCCu, 0xFCu, 0x48u, 0x5Au, 0x7Bu, 0xDEu, 0x21u, 0x35u, 0xACu, 0x3Cu
};

bool MjkEnemyInternal_IsMonsterType(uint8_t t) {
    return t <= 7u || t == MJK_ENEMY_MUMMY1 || t == MJK_ENEMY_MUMMY2;
}

mjk_q8_8_t MjkEnemyInternal_SpeedQ8(const MjkAssetPackage* pkg, uint8_t speed_tier) {
    uint16_t raw = 0x0058u;
    uint8_t idx = (speed_tier < 8u) ? speed_tier : 7u;
    if (pkg) {
        MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_ENEMY_SPEED_TABLE + (uint16_t)idx * 2u), &raw);
    }
    return (mjk_q8_8_t)raw;
}

static uint8_t enemy_gameido_div(const MjkAssetPackage* pkg, uint8_t speed_tier) {
    uint8_t idx = (speed_tier < 8u) ? speed_tier : 7u;
    const uint8_t* tbl = pkg ? MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_GAMEIDO_DIV_TABLE, 8u) : NULL;
    return (tbl && tbl[idx] != 0u) ? tbl[idx] : 0x4Cu;
}

uint8_t MjkEnemyInternal_ArchetypeSpriteId(uint8_t t, uint8_t anim, bool facing_left) {
    uint8_t phase = (uint8_t)((anim >> 3u) & 1u);
    switch (t) {
        case MJK_ENEMY_MUMMY:
        case MJK_ENEMY_MUMMY1:
        case MJK_ENEMY_MUMMY2:     return (uint8_t)(0x0Eu + phase);
        case MJK_ENEMY_HANEZO:     return (uint8_t)(0x12u + phase);
        case MJK_ENEMY_GEJISHOGUN: return (uint8_t)(0x0Cu + phase);
        case MJK_ENEMY_GAMEIDO:    return (uint8_t)(0x16u + phase);
        case MJK_ENEMY_DOKURON:    return (uint8_t)(0x14u + phase);
        case MJK_ENEMY_DESUFA:     return 0x07u;
        case MJK_ENEMY_HORUS:      return facing_left ? 0x08u : 0x09u;
        case MJK_ENEMY_POWER_COIN: return 0x20u;
        case MJK_ENEMY_EXTRA_COIN: return 0x1Bu;
        case MJK_ENEMY_SECRET_COIN:return 0x1Fu;
        case MJK_ENEMY_CAPTIVE:
        case MJK_ENEMY_BROTHER:    return 0x23u;
        case MJK_ENEMY_BALLOON:    return 0x24u;
        case MJK_ENEMY_BONUS_COIN: return 0x19u;
        default:                   return 0x0Eu;
    }
}

uint8_t MjkEnemyInternal_ArchetypePalette(uint8_t t) {
    switch (t) {
        case MJK_ENEMY_HANEZO:
        case MJK_ENEMY_GEJISHOGUN: return 2u;
        case MJK_ENEMY_DESUFA:
        case MJK_ENEMY_EXTRA_COIN:
        case MJK_ENEMY_SECRET_COIN:
        case MJK_ENEMY_BALLOON:
        case MJK_ENEMY_BONUS_COIN: return 0u;
        case MJK_ENEMY_POWER_COIN: return 1u;
        default:                   return 3u;
    }
}

static bool probe_enemy_solid(const MjkGame* game, int32_t wx, int32_t wy) {
    if (wy < 16 || wy >= 240) { return true; }
    if (wx < 0 || wx >= 512) { return false; }
    if (wx >= 256 && game->camera.neighbor_room_id == 0u) { return false; }
    const MjkRoomState* src = (wx >= 256) ? &game->right_room : &game->room;
    uint8_t mx = (uint8_t)(((uint32_t)wx & 255u) >> 4u);
    uint8_t my = (uint8_t)((uint32_t)wy >> 4u);
    if (my >= MJK_ROOM_MT_ROWS) { return true; }
    uint8_t m = src->static_metatiles[my][mx];
    if (m >= 0x5Cu && m <= 0x62u) { return false; }
    if (m != 0x00u && m != 0x27u && m != 0x2Fu) {
        if (m >= 0x38u && m <= 0x4Bu) {
            uint8_t idx = (uint8_t)((m - 0x38u) >> 1u);
            uint8_t packed = k_enemy_partial_masks[idx];
            uint8_t nibble = ((m & 1u) == 0u) ? (uint8_t)(packed >> 4u) : (uint8_t)(packed & 0x0Fu);
            uint8_t bit = (uint8_t)(3u - (((uint8_t)((wy >> 3u) & 1u) * 2u) + (uint8_t)((wx >> 3u) & 1u)));
            return ((nibble >> bit) & 1u) != 0u;
        }
        return true;
    }
    uint8_t cell_yx = (uint8_t)((my << 4u) | mx);
    for (uint8_t i = 0u; i < src->object_count; ++i) {
        const MjkRoomObject* ro = &src->objects[i];
        if (ro->yx == cell_yx) {
            uint8_t kind = (uint8_t)(ro->ram_state & 0x1Fu);
            if ((kind == 0x10u || kind == 0x11u || kind == 0x13u) && (ro->ram_state & 0x20u) == 0u) {
                return true;
            }
        }
    }
    return false;
}

uint8_t MjkEnemyInternal_CalcCollisionFlags(const MjkGame* game, uint8_t ex, uint8_t ey) {
    int32_t wx = (int32_t)ex + (int32_t)(game->camera.scroll_x_fixed >> 8);
    int32_t wy = (int32_t)ey + (int32_t)(game->camera.scroll_y_fixed >> 8);
    uint8_t cf = 0u;
    if (probe_enemy_solid(game, wx + 8, wy - 4) || probe_enemy_solid(game, wx + 8, wy + 4)) { cf |= EF_RIGHT; }
    if (probe_enemy_solid(game, wx - 8, wy - 4) || probe_enemy_solid(game, wx - 8, wy + 4)) { cf |= EF_LEFT; }
    if (probe_enemy_solid(game, wx - 4, wy + 8) || probe_enemy_solid(game, wx + 4, wy + 8)) { cf |= EF_FLOOR; }
    if (probe_enemy_solid(game, wx - 4, wy - 8) || probe_enemy_solid(game, wx + 4, wy - 8)) { cf |= EF_CEIL; }
    return cf;
}

void MjkEnemyInternal_InitMummyFall(MjkEnemy* e, uint8_t transform_reload) {
    e->falling = true;
    e->jump_accum = 0x4000u;
    e->vel_x = 0;
    e->vel_y = 0;
    e->edge_turned = false;
    e->transform_timer = transform_reload;
}

static void update_gameido_vel(MjkEnemy* e, const MjkGame* game, const MjkEnemyManager* mgr, uint8_t cf) {
    uint8_t ex = MjkUQ8_Int(e->pos_x);
    uint8_t ey = MjkUQ8_Int(e->pos_y);
    uint8_t div = enemy_gameido_div(game->pkg, mgr->speed_tier);
    int16_t dx = (int16_t)game->jack_x - (int16_t)ex;
    int16_t dy = (int16_t)game->jack_y - (int16_t)ey;
    uint16_t ax = (uint16_t)(dx < 0 ? -dx : dx);
    uint16_t ay = (uint16_t)(dy < 0 ? -dy : dy);
    mjk_q8_8_t vx = (mjk_q8_8_t)((ax << 8u) / div);
    mjk_q8_8_t vy = (mjk_q8_8_t)((ay << 8u) / div);
    bool neg_x = (dx >= 0) ? ((cf & EF_RIGHT) != 0u) : ((cf & EF_LEFT) == 0u);
    bool neg_y = (dy >= 0) ? ((cf & EF_FLOOR) != 0u) : ((cf & EF_CEIL) == 0u);
    e->vel_x = neg_x ? (mjk_q8_8_t)(-vx) : vx;
    e->vel_y = neg_y ? (mjk_q8_8_t)(-vy) : vy;
}

static void init_archetype(MjkEnemy* e, const MjkGame* game, const MjkEnemyManager* mgr) {
    mjk_q8_8_t spd = MjkEnemyInternal_SpeedQ8(game->pkg, mgr->speed_tier);
    uint8_t ex = MjkUQ8_Int(e->pos_x);
    uint8_t ey = MjkUQ8_Int(e->pos_y);
    e->ai_timer = 0u;
    e->ai_dir = EF_RIGHT;
    e->falling = false;
    e->edge_turned = false;
    switch (e->type) {
        case MJK_ENEMY_HANEZO:     e->vel_x = spd; e->vel_y = spd; break;
        case MJK_ENEMY_GEJISHOGUN: e->vel_x = (game->jack_x < ex) ? (mjk_q8_8_t)(-spd) : spd;
                                   e->vel_y = (game->jack_y < ey) ? (mjk_q8_8_t)(-spd) : spd; break;
        case MJK_ENEMY_GAMEIDO:    update_gameido_vel(e, game, mgr, 0u); break;
        case MJK_ENEMY_DOKURON:    e->vel_x = (game->jack_x < ex) ? (mjk_q8_8_t)(-spd) : spd; e->vel_y = spd; break;
        case MJK_ENEMY_DESUFA:     e->vel_x = spd; e->vel_y = (game->jack_y < ey) ? (mjk_q8_8_t)(-spd) : spd; break;
        case MJK_ENEMY_HORUS:      e->vel_x = spd; e->vel_y = 0; break;
        default: break;
    }
}

static void transform_mummy(MjkEnemy* e, const MjkGame* game, const MjkEnemyManager* mgr) {
    e->type = (uint8_t)((game->player.a_press_counter % 6u) + 2u);
    e->spawn_flash_timer = 0x20u;
    e->attr_base = MjkEnemyInternal_ArchetypePalette(e->type);
    init_archetype(e, game, mgr);
}

static void step_mummy(MjkEnemy* e, const MjkGame* game, const MjkEnemyManager* mgr, uint8_t cf) {
    uint8_t ex = MjkUQ8_Int(e->pos_x);
    if ((cf & EF_FLOOR) != 0u) {
        if (e->falling) {
            e->falling = false;
            e->vel_y = 0;
            e->pos_y = (mjk_uq8_8_t)(e->pos_y & 0xF800u);
            e->vel_x = (ex >= game->jack_x) ? (mjk_q8_8_t)(-0x0080) : (mjk_q8_8_t)0x0080;
            e->edge_turned = false;
            if (game->room.is_bomb_room && MjkUQ8_Int(e->pos_y) >= 0xD8u) {
                transform_mummy(e, game, mgr);
                return;
            }
        }
        if (e->transform_timer > 0u) { e->transform_timer--; }
        else if (!game->room.is_bomb_room) { transform_mummy(e, game, mgr); return; }
        if ((cf & (EF_RIGHT | EF_LEFT)) != 0u) {
            if (!e->edge_turned) { e->edge_turned = true; e->vel_x = (mjk_q8_8_t)(-e->vel_x); }
        } else {
            e->edge_turned = false;
        }
    } else if (e->falling) {
        uint32_t acc = (uint32_t)e->jump_accum + 0x00F0u;
        if ((acc >> 8u) >= 0x80u) { acc = (0x7Fu << 8u) | (acc & 0xFFu); }
        e->jump_accum = (uint16_t)acc;
        uint8_t hi = (uint8_t)(e->jump_accum >> 8u);
        uint8_t idx = (hi < 0x40u) ? hi : (uint8_t)((hi ^ 0xFFu) & 0x3Fu);
        uint16_t mag = 0x0380u;
        MjkAssets_ReadCpuU16(game->pkg, (uint16_t)(MJK_ADDR_JUMP_TABLE_C0E3 + (uint16_t)idx * 2u), &mag);
        e->vel_y = (hi < 0x40u) ? (mjk_q8_8_t)(-(int16_t)mag) : (mjk_q8_8_t)mag;
    } else {
        if (e->transform_timer > 0u) {
            e->transform_timer--;
            if (!e->edge_turned) { e->edge_turned = true; e->vel_x = (mjk_q8_8_t)(-e->vel_x); }
        } else if (game->room.is_bomb_room) {
            MjkEnemyInternal_InitMummyFall(e, mgr->transform_reload);
        } else {
            transform_mummy(e, game, mgr);
        }
    }
}

static void step_horus(MjkEnemy* e, const MjkGame* game, mjk_q8_8_t spd, uint8_t cf) {
    if (cf == 0u) {
        if (e->ai_timer > 0u) { e->ai_timer--; return; }
        e->ai_timer = 0x10u;
    }
    int16_t dx = (int16_t)game->jack_x - (int16_t)MjkUQ8_Int(e->pos_x);
    int16_t dy = (int16_t)game->jack_y - (int16_t)MjkUQ8_Int(e->pos_y);
    uint16_t ax = (uint16_t)(dx < 0 ? -dx : dx);
    uint16_t ay = (uint16_t)(dy < 0 ? -dy : dy);
    uint8_t px_d = (dx >= 0) ? EF_RIGHT : EF_LEFT, ox_d = (dx >= 0) ? EF_LEFT : EF_RIGHT;
    uint8_t py_d = (dy >= 0) ? EF_FLOOR : EF_CEIL, oy_d = (dy >= 0) ? EF_CEIL : EF_FLOOR;
    uint8_t pref[4] = {
        (ax > ay) ? px_d : py_d, (ax > ay) ? py_d : px_d,
        (ax > ay) ? ox_d : oy_d, (ax > ay) ? oy_d : ox_d
    };
    uint8_t opp = (e->ai_dir == EF_RIGHT) ? EF_LEFT : (e->ai_dir == EF_LEFT) ? EF_RIGHT :
                  (e->ai_dir == EF_FLOOR) ? EF_CEIL : EF_FLOOR;
    uint8_t chosen = opp;
    for (size_t k = 0u; k < 4u; ++k) {
        if ((pref[k] & cf) == 0u && pref[k] != opp) { chosen = pref[k]; break; }
    }
    e->ai_dir = chosen;
    e->vel_x = (chosen == EF_RIGHT) ? spd : (chosen == EF_LEFT) ? (mjk_q8_8_t)(-spd) : 0;
    e->vel_y = (chosen == EF_FLOOR) ? spd : (chosen == EF_CEIL) ? (mjk_q8_8_t)(-spd) : 0;
}

static void step_enemy_ai(MjkEnemy* e, const MjkGame* game, const MjkEnemyManager* mgr) {
    uint8_t ex = MjkUQ8_Int(e->pos_x), ey = MjkUQ8_Int(e->pos_y);
    uint8_t cf = MjkEnemyInternal_CalcCollisionFlags(game, ex, ey);
    mjk_q8_8_t spd = MjkEnemyInternal_SpeedQ8(game->pkg, mgr->speed_tier);

    switch (e->type) {
        case MJK_ENEMY_MUMMY: step_mummy(e, game, mgr, cf); break;
        case MJK_ENEMY_HANEZO:
            if ((cf & EF_RIGHT) != 0u) { e->vel_x = (mjk_q8_8_t)(-spd); }
            else if ((cf & EF_LEFT) != 0u) { e->vel_x = spd; }
            if ((cf & EF_FLOOR) != 0u) { e->vel_y = (mjk_q8_8_t)(-spd); }
            else if ((cf & EF_CEIL) != 0u) { e->vel_y = spd; }
            break;
        case MJK_ENEMY_GEJISHOGUN:
            e->vel_x = (game->jack_x < ex) ? (((cf & EF_LEFT) != 0u) ? spd : (mjk_q8_8_t)(-spd))
                                           : (((cf & EF_RIGHT) != 0u) ? (mjk_q8_8_t)(-spd) : spd);
            if (e->ai_timer > 0u) { e->ai_timer--; if (cf == 0u) { break; } }
            e->vel_y = (game->jack_y < ey) ? (((cf & EF_CEIL) != 0u) ? spd : (mjk_q8_8_t)(-spd))
                                           : (((cf & EF_FLOOR) != 0u) ? (mjk_q8_8_t)(-spd) : spd);
            if (cf != 0u) { e->ai_timer = 0x30u; }
            break;
        case MJK_ENEMY_GAMEIDO:
            if (cf != 0u) { update_gameido_vel(e, game, mgr, cf); }
            break;
        case MJK_ENEMY_DOKURON: {
            e->ai_timer++;
            int16_t tx = ((e->ai_timer & 0x20u) != 0u) ? ((int16_t)game->jack_x - 16) : ((int16_t)game->jack_x + 16);
            e->vel_x = (tx < (int16_t)ex) ? (((cf & EF_LEFT) != 0u) ? spd : (mjk_q8_8_t)(-spd))
                                          : (((cf & EF_RIGHT) != 0u) ? (mjk_q8_8_t)(-spd) : spd);
            if ((cf & EF_FLOOR) != 0u) { e->vel_y = (mjk_q8_8_t)(-spd); }
            else if ((cf & EF_CEIL) != 0u) { e->vel_y = spd; }
            break;
        }
        case MJK_ENEMY_DESUFA: {
            e->ai_timer++;
            int16_t ty = ((e->ai_timer & 0x20u) != 0u) ? ((int16_t)game->jack_y - 16) : ((int16_t)game->jack_y + 16);
            e->vel_y = (ty < (int16_t)ey) ? (((cf & EF_CEIL) != 0u) ? spd : (mjk_q8_8_t)(-spd))
                                          : (((cf & EF_FLOOR) != 0u) ? (mjk_q8_8_t)(-spd) : spd);
            if ((cf & EF_RIGHT) != 0u) { e->vel_x = (mjk_q8_8_t)(-spd); }
            else if ((cf & EF_LEFT) != 0u) { e->vel_x = spd; }
            break;
        }
        case MJK_ENEMY_HORUS: step_horus(e, game, spd, cf); break;
        default: break;
    }
    if (e->vel_x != 0) { e->facing_left = (e->vel_x < 0); }
}

void MjkEnemy_InitForRoom(MjkEnemyManager* mgr, const MjkAssetPackage* pkg,
                          uint8_t last_bomb_cleared, bool is_bomb_room) {
    if (!mgr) { return; }
    memset(mgr, 0, sizeof(*mgr));
    mgr->spawn_reload = 0xA0u;
    mgr->transform_reload = is_bomb_room ? 0x40u : 0x80u;
    if (pkg) {
        uint8_t tier = (uint8_t)(last_bomb_cleared & 0x0Fu);
        const uint8_t* diff = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_DIFFICULTY_TABLE, (size_t)(tier + 1u) * 4u);
        if (diff) {
            const uint8_t* row = diff + ((size_t)tier * 4u);
            mgr->speed_tier = row[0];
            mgr->spawn_reload = row[1];
            mgr->transform_reload = is_bomb_room ? row[3] : row[2];
        }
    }
    mgr->spawn_timer = mgr->spawn_reload;
}

static bool try_spawn_candidate(uint8_t yx, bool is_right_room, int32_t scroll_x,
                                uint8_t* out_sx, uint8_t* out_sy) {
    if (yx == 0x00u) { return false; }
    int32_t screen_x = (int32_t)(((yx & 0x0Fu) << 4u) + 8u) + (is_right_room ? 256 : 0) - scroll_x;
    if (screen_x < 0 || screen_x >= 256) { return false; }
    *out_sx = (uint8_t)screen_x;
    *out_sy = (uint8_t)((yx & 0xF0u) + 8u);
    return true;
}

static uint16_t manhattan_dist(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by) {
    int16_t dx = (int16_t)ax - (int16_t)bx, dy = (int16_t)ay - (int16_t)by;
    return (uint16_t)((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy));
}

static void spawn_mummy(MjkEnemyManager* mgr, const MjkGame* game) {
    int32_t scroll_x = (int32_t)(game->camera.scroll_x_fixed >> 8);
    uint16_t wx = (uint16_t)MjkUQ8_Int(game->player.pos_x) + (uint16_t)scroll_x;
    bool is_right = (wx >= 256u && game->camera.neighbor_room_id != 0u);
    uint8_t rid = is_right ? game->camera.neighbor_room_id : game->current_room_id;
    const uint8_t* sp = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_ENEMY_SPAWN_POS, 512u);
    if (!sp || sp[(size_t)rid * 2u] == 0x00u) { return; }

    uint8_t sx0 = 0u, sy0 = 0u, sx1 = 0u, sy1 = 0u;
    bool v0 = try_spawn_candidate(sp[(size_t)rid * 2u], is_right, scroll_x, &sx0, &sy0);
    bool v1 = try_spawn_candidate(sp[(size_t)rid * 2u + 1u], is_right, scroll_x, &sx1, &sy1);
    if (!v0 && !v1) { return; }

    uint8_t sx = v0 ? sx0 : sx1, sy = v0 ? sy0 : sy1;
    if (v0 && v1 && manhattan_dist(sx1, sy1, game->jack_x, game->jack_y) >
                    manhattan_dist(sx0, sy0, game->jack_x, game->jack_y)) {
        sx = sx1; sy = sy1;
    }
    MjkEnemy_SpawnCollectible(mgr, MJK_ENEMY_MUMMY, sx, sy);
}

MjkActionResult MjkEnemy_UpdateAll(MjkEnemyManager* mgr, MjkGame* game) {
    if (!mgr || !game || !game->pkg) { return MJK_ACTION_CONTINUE; }

    if (mgr->post_transmute_grace > 0u) {
        mgr->post_transmute_grace--;
    }

    if (mgr->transmute_timer > 0u) {
        mgr->transmute_timer--;
        if (mgr->transmute_timer == 0u) {
            for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) { mgr->slots[i].transmuted = false; }
            MjkPpu_RestoreEnemyPalettes(&game->ppu, game->pkg);
            game->objects.power_coin_armed = 0u;
            game->objects.power_coin_acc = 0u;
            const uint8_t* mus_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_MUSIC_OPTIONS, 8u);
            if (mus_tbl) {
                uint8_t m_idx = (uint8_t)(game->room.music_id & 7u);
                if (m_idx == 0u) { m_idx = 1u; }
                MjkGame_QueueSound(game, (MjkSoundId)mus_tbl[m_idx]);
            }
            mgr->post_transmute_grace = 32u;
        }
    } else {
        if (mgr->spawn_timer > 0u) { mgr->spawn_timer--; }
        if (mgr->spawn_timer == 0u) {
            mgr->spawn_timer = mgr->spawn_reload;
            if (mgr->alive_count < MJK_ENEMY_MAX_ALIVE) { spawn_mummy(mgr, game); }
        }
    }

    mgr->alive_count = 0u;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        MjkEnemy* e = &mgr->slots[i];
        if (!e->active) { continue; }
        int32_t nx = (int32_t)e->pos_x + (int32_t)game->camera.delta_cam_x * 256;
        int32_t ny = (int32_t)e->pos_y + (int32_t)game->camera.delta_cam_y * 256;
        if (nx < 0 || nx >= 0x10000 || ny < 0 || ny >= 0x10000) { e->active = false; continue; }
        e->pos_x = (mjk_uq8_8_t)nx;
        e->pos_y = (mjk_uq8_8_t)ny;
        e->anim_timer++;

        if (MjkEnemyInternal_IsMonsterType(e->type)) {
            mgr->alive_count++;
            if (e->transmuted) {
                /* Transmutados en moneda quedan congelados en su posición (solo scroll). */
            } else if (e->spawn_flash_timer > 0u) {
                e->spawn_flash_timer--;
            } else {
                step_enemy_ai(e, game, mgr);
                nx = (int32_t)e->pos_x + (int32_t)e->vel_x;
                ny = (int32_t)e->pos_y + (int32_t)e->vel_y;
                if (nx < 0 || nx >= 0x10000 || ny < 0 || ny >= 0x10000) {
                    e->active = false;
                    if (mgr->alive_count > 0u) { mgr->alive_count--; }
                    continue;
                }
                e->pos_x = (mjk_uq8_8_t)nx;
                e->pos_y = (mjk_uq8_8_t)ny;
            }
        } else if (!MjkEnemyInternal_StepCollectible(e, game)) {
            continue;
        }

        uint8_t ex = MjkUQ8_Int(e->pos_x);
        uint8_t ey = MjkUQ8_Int(e->pos_y);
        int16_t dx = (int16_t)ex - (int16_t)game->jack_x;
        int16_t dy = (int16_t)ey - (int16_t)game->jack_y;
        if (dx > -8 && dx < 8 && dy > -8 && dy < 8) {
            if (e->transmuted) {
                MjkEnemyInternal_HandleTransmutedPickup(e, mgr, game, ex, ey);
            } else if (!MjkEnemyInternal_IsMonsterType(e->type)) {
                MjkEnemyInternal_HandleCollectiblePickup(e, mgr, game, ex, ey);
            } else if (game->mighty_level == 3u) {
                MjkGame_AddScore(game, MJK_SCORE_500);
                e->active = false;
                if (mgr->alive_count > 0u) { mgr->alive_count--; }
            } else if (!game->player.is_dying && e->spawn_flash_timer == 0u &&
                       mgr->post_transmute_grace == 0u) {
                game->player.is_dying = true;
                game->player.death_timer = 0u;
                MjkGame_QueueSound(game, MJK_SND_DEATH);
            }
        }
        if (e->active) { e->sprite_id = MjkEnemyInternal_ArchetypeSpriteId(e->type, e->anim_timer, e->facing_left); }
    }
    return MJK_ACTION_CONTINUE;
}

uint8_t MjkEnemy_RenderOam(const MjkEnemyManager* mgr, const MjkAssetPackage* pkg,
                           MjkPpu* ppu, uint8_t start_slot) {
    if (!mgr || !pkg || !ppu) { return start_slot; }
    uint8_t slot = start_slot;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS && slot < MJK_PPU_OAM_COUNT; ++i) {
        const MjkEnemy* e = &mgr->slots[i];
        if (!e->active) { continue; }
        uint8_t ex = MjkUQ8_Int(e->pos_x), ey = MjkUQ8_Int(e->pos_y);
        if (ey >= 240u) { continue; }
        if (e->transmuted) {
            uint8_t pal = (mgr->transmute_timer < 64u)
                ? (uint8_t)(mgr->transmute_timer & 3u)
                : 0x02u;
            slot = MjkPpu_WriteMetasprite(ppu, pkg, 0x21u, ex, ey, pal, slot);
        } else if (MjkEnemyInternal_IsMonsterType(e->type) && e->spawn_flash_timer > 16u) {
            slot = MjkPpu_WriteMetasprite(ppu, pkg, 0x25u, ex, ey, 0x00u, slot);
        } else {
            uint8_t attr = (uint8_t)((e->attr_base & 3u) | (e->facing_left ? 0x00u : 0x04u));
            slot = MjkPpu_WriteMetasprite(ppu, pkg, e->sprite_id, ex, ey, attr, slot);
        }
    }
    return slot;
}
