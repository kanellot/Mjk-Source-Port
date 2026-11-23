/**
 * @file mjk_enemy.c
 * @brief Implementación de IA de enemigos, mutación y Power Coin (asm/src/prg.asm:8171-9575).
 */

#include "mjk_enemy.h"
#include "mjk_game.h"
#include "mjk_hud.h"
#include <string.h>

static bool is_monster_type(uint8_t t) {
    return t <= 7u || t == MJK_ENEMY_MUMMY1 || t == MJK_ENEMY_MUMMY2;
}

static uint8_t archetype_sprite_id(uint8_t t, uint8_t anim, bool facing_left) {
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
        case MJK_ENEMY_CAPTIVE:    return 0x23u;
        case MJK_ENEMY_BALLOON:    return 0x24u;
        case MJK_ENEMY_BONUS_COIN: return 0x19u;
        default:                   return 0x0Eu;
    }
}

static uint8_t archetype_palette(uint8_t t) {
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

void MjkEnemy_InitForRoom(MjkEnemyManager* mgr,
                          const MjkAssetPackage* pkg,
                          uint8_t last_bomb_cleared,
                          bool is_bomb_room) {
    if (!mgr) {
        return;
    }
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

void MjkEnemy_ActivatePowerCoin(MjkEnemyManager* mgr) {
    if (!mgr) {
        return;
    }
    mgr->transmute_timer = MJK_POWER_COIN_FRAMES;
    mgr->transmute_streak = 0u;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (mgr->slots[i].active && is_monster_type(mgr->slots[i].type)) {
            mgr->slots[i].transmuted = true;
        }
    }
}

bool MjkEnemy_SpawnCollectible(MjkEnemyManager* mgr,
                               MjkEnemyType type,
                               uint8_t x,
                               uint8_t y) {
    if (!mgr) {
        return false;
    }
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (!mgr->slots[i].active) {
            MjkEnemy* e = &mgr->slots[i];
            memset(e, 0, sizeof(*e));
            e->active = true;
            e->type = (uint8_t)type;
            e->pos_x = MjkUQ8_FromInt(x);
            e->pos_y = MjkUQ8_FromInt(y);
            e->sprite_id = archetype_sprite_id(e->type, 0u, false);
            e->attr_base = archetype_palette(e->type);
            return true;
        }
    }
    return false;
}

static void spawn_mummy(MjkEnemyManager* mgr, const MjkAssetPackage* pkg) {
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (!mgr->slots[i].active) {
            MjkEnemy* e = &mgr->slots[i];
            memset(e, 0, sizeof(*e));
            e->active = true;
            e->type = MJK_ENEMY_MUMMY;
            e->spawn_flash_timer = 0x20u;
            e->transform_timer = mgr->transform_reload;
            e->sprite_id = 0x0Eu;
            e->attr_base = 3u;

            uint8_t sx = 0x80u;
            uint8_t sy = 0x30u;
            const uint8_t* sp = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_ENEMY_SPAWN_POS, 14u);
            if (sp) {
                uint8_t idx = (uint8_t)(4u + ((mgr->spawn_cursor++ % 5u) * 2u));
                uint8_t yx = sp[idx];
                if (yx != 0u) {
                    sx = (uint8_t)(((yx & 0x0Fu) << 4u) + 8u);
                    sy = (uint8_t)((yx & 0xF0u) + 8u);
                }
            }
            e->pos_x = MjkUQ8_FromInt(sx);
            e->pos_y = MjkUQ8_FromInt(sy);
            e->vel_x = (mjk_q8_8_t)(0x00C0 + (mgr->speed_tier * 0x0020));
            mgr->alive_count++;
            return;
        }
    }
}

static void step_enemy_motion(MjkEnemy* e, const MjkGame* game) {
    if (e->type == MJK_ENEMY_DESUFA) {
        int16_t dx = (int16_t)game->jack_x - (int16_t)MjkUQ8_Int(e->pos_x);
        int16_t dy = (int16_t)game->jack_y - (int16_t)MjkUQ8_Int(e->pos_y);
        e->vel_x = (dx >= 0) ? (mjk_q8_8_t)0x00C0 : (mjk_q8_8_t)(-0x00C0);
        e->vel_y = (dy >= 0) ? (mjk_q8_8_t)0x00A0 : (mjk_q8_8_t)(-0x00A0);
    } else if (e->type >= MJK_ENEMY_HANEZO && e->type <= MJK_ENEMY_HORUS && e->vel_y == 0) {
        e->vel_y = (mjk_q8_8_t)0x00A0;
    }

    e->pos_x = MjkUQ8_AddSigned(e->pos_x, e->vel_x);
    e->pos_y = MjkUQ8_AddSigned(e->pos_y, e->vel_y);

    uint8_t px = MjkUQ8_Int(e->pos_x);
    uint8_t py = MjkUQ8_Int(e->pos_y);
    if (px < 16u || px > 240u) {
        e->vel_x = (mjk_q8_8_t)(-e->vel_x);
        e->facing_left = (e->vel_x < 0);
    }
    if (py < 24u || py > 216u) {
        e->vel_y = (mjk_q8_8_t)(-e->vel_y);
    }
}

MjkActionResult MjkEnemy_UpdateAll(MjkEnemyManager* mgr, MjkGame* game) {
    if (!mgr || !game || !game->pkg) {
        return MJK_ACTION_CONTINUE;
    }

    if (mgr->transmute_timer > 0u) {
        mgr->transmute_timer--;
        if (mgr->transmute_timer == 0u) {
            for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
                mgr->slots[i].transmuted = false;
            }
        }
    } else if (mgr->alive_count < MJK_ENEMY_MAX_ALIVE) {
        if (mgr->spawn_timer > 0u) {
            mgr->spawn_timer--;
        }
        if (mgr->spawn_timer == 0u) {
            mgr->spawn_timer = mgr->spawn_reload;
            spawn_mummy(mgr, game->pkg);
        }
    }

    mgr->alive_count = 0u;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        MjkEnemy* e = &mgr->slots[i];
        if (!e->active) {
            continue;
        }
        e->pos_x = (mjk_uq8_8_t)(e->pos_x + (uint16_t)(game->camera.delta_cam_x * 256));
        e->pos_y = (mjk_uq8_8_t)(e->pos_y + (uint16_t)(game->camera.delta_cam_y * 256));
        e->anim_timer++;

        if (is_monster_type(e->type)) {
            mgr->alive_count++;
            if (e->spawn_flash_timer > 0u) {
                e->spawn_flash_timer--;
            } else {
                if (e->type == MJK_ENEMY_MUMMY && !e->transmuted) {
                    if (e->transform_timer > 0u) {
                        e->transform_timer--;
                    }
                    if (e->transform_timer == 0u) {
                        e->type = (uint8_t)((game->player.a_press_counter % 6u) + 2u);
                        e->spawn_flash_timer = 0x20u;
                        e->attr_base = archetype_palette(e->type);
                    }
                }
                step_enemy_motion(e, game);
            }
        }

        int16_t dx = (int16_t)MjkUQ8_Int(e->pos_x) - (int16_t)game->jack_x;
        int16_t dy = (int16_t)MjkUQ8_Int(e->pos_y) - (int16_t)game->jack_y;
        if (dx > -12 && dx < 12 && dy > -12 && dy < 12) {
            if (e->transmuted) {
                static const MjkScoreId k_chain[7] = {
                    MJK_SCORE_100, MJK_SCORE_200, MJK_SCORE_300, MJK_SCORE_500,
                    MJK_SCORE_800, MJK_SCORE_1200, MJK_SCORE_2000
                };
                uint8_t s_idx = (mgr->transmute_streak < 7u) ? mgr->transmute_streak : 6u;
                MjkGame_AddScore(game, k_chain[s_idx]);
                if (mgr->transmute_streak < 6u) { mgr->transmute_streak++; }
                e->active = false;
                if (mgr->alive_count > 0u) { mgr->alive_count--; }
            } else if (!is_monster_type(e->type)) {
                if (e->type == MJK_ENEMY_POWER_COIN) {
                    MjkEnemy_ActivatePowerCoin(mgr);
                } else if (e->type == MJK_ENEMY_EXTRA_COIN) {
                    game->lives++;
                    game->extra_lives_found++;
                    MjkGame_QueueSound(game, MJK_SND_1UP_CAPTIVE);
                } else if (e->type == MJK_ENEMY_SECRET_COIN) {
                    game->secret_coins++;
                    MjkGame_AddScore(game, MJK_SCORE_50000);
                } else if (e->type == MJK_ENEMY_BONUS_COIN && game->score_multiplier < 4u) {
                    game->score_multiplier++;
                }
                e->active = false;
            } else if (game->mighty_level == 3u) {
                MjkGame_AddScore(game, MJK_SCORE_500);
                e->active = false;
                if (mgr->alive_count > 0u) { mgr->alive_count--; }
            } else if (!game->player.is_dying && e->spawn_flash_timer == 0u) {
                game->player.is_dying = true;
                game->player.death_timer = 0u;
                MjkGame_QueueSound(game, MJK_SND_DEATH);
            }
        }

        if (e->active) {
            e->sprite_id = archetype_sprite_id(e->type, e->anim_timer, e->facing_left);
        }
    }
    return MJK_ACTION_CONTINUE;
}

uint8_t MjkEnemy_RenderOam(const MjkEnemyManager* mgr,
                           const MjkAssetPackage* pkg,
                           MjkPpu* ppu,
                           uint8_t start_slot) {
    if (!mgr || !pkg || !ppu) {
        return start_slot;
    }
    uint8_t slot = start_slot;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS && slot < MJK_PPU_OAM_COUNT; ++i) {
        const MjkEnemy* e = &mgr->slots[i];
        if (!e->active) {
            continue;
        }
        uint8_t ex = MjkUQ8_Int(e->pos_x);
        uint8_t ey = MjkUQ8_Int(e->pos_y);
        if (ey >= 240u) {
            continue;
        }
        if (e->transmuted) {
            if (mgr->transmute_timer < 64u && ((mgr->transmute_timer >> 2u) & 1u) != 0u) {
                continue;
            }
            slot = MjkPpu_WriteMetasprite(ppu, pkg, 0x21u, ex, ey, 0x02u, slot);
        } else if (is_monster_type(e->type) && e->spawn_flash_timer > 16u) {
            slot = MjkPpu_WriteMetasprite(ppu, pkg, 0x25u, ex, ey, 0x00u, slot);
        } else {
            uint8_t attr = (uint8_t)((e->attr_base & 3u) | (e->facing_left ? 0x00u : 0x04u));
            slot = MjkPpu_WriteMetasprite(ppu, pkg, e->sprite_id, ex, ey, attr, slot);
        }
    }
    return slot;
}
