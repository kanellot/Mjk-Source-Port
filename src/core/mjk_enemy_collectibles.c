/**
 * @file mjk_enemy_collectibles.c
 * @brief Power Coin (320f, transmutación) y coleccionables OAM (SecretCoin, Captive, Balloon, Bonus/Extra/Brother).
 */

#include "mjk_enemy_internal.h"
#include "mjk_game.h"
#include "mjk_hud.h"
#include <string.h>

void MjkEnemy_ActivatePowerCoin(MjkEnemyManager* mgr) {
    if (!mgr) { return; }
    mgr->transmute_timer = MJK_POWER_COIN_FRAMES;
    mgr->transmute_streak = 0u;
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (mgr->slots[i].active && MjkEnemyInternal_IsMonsterType(mgr->slots[i].type)) {
            mgr->slots[i].transmuted = true;
        }
    }
}

void MjkEnemy_ActivatePowerCoinEx(MjkEnemyManager* mgr, MjkGame* game) {
    MjkEnemy_ActivatePowerCoin(mgr);
    if (game) {
        game->ppu.palette_ram[0x19u] = 0x08u;
        game->ppu.palette_ram[0x1Au] = 0x26u;
        game->ppu.palette_ram[0x1Bu] = 0x30u;
        MjkGame_QueueSound(game, MJK_SND_POWER_COIN_MUSIC);
    }
}

bool MjkEnemy_SpawnPowerCoinBombRoom(MjkEnemyManager* mgr, const MjkGame* game) {
    if (!mgr || !game) { return false; }
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (!mgr->slots[i].active) {
            MjkEnemy* e = &mgr->slots[i];
            memset(e, 0, sizeof(*e));
            e->active = true;
            e->type = MJK_ENEMY_POWER_COIN;
            e->pos_x = MjkUQ8_FromInt(0x80u);
            e->pos_y = MjkUQ8_FromInt(0x78u);
            mjk_q8_8_t spd = MjkEnemyInternal_SpeedQ8(game->pkg, 5u);
            e->vel_x = spd;
            e->vel_y = spd;
            e->ai_dir = 1u;
            e->sprite_id = MjkEnemyInternal_ArchetypeSpriteId(e->type, 0u, false);
            e->attr_base = MjkEnemyInternal_ArchetypePalette(e->type);
            return true;
        }
    }
    return false;
}

bool MjkEnemy_SpawnCollectible(MjkEnemyManager* mgr, MjkEnemyType type, uint8_t x, uint8_t y) {
    if (!mgr) { return false; }
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        if (!mgr->slots[i].active) {
            MjkEnemy* e = &mgr->slots[i];
            memset(e, 0, sizeof(*e));
            e->active = true;
            e->pos_x = MjkUQ8_FromInt(x);
            e->pos_y = MjkUQ8_FromInt(y);
            if (type == MJK_ENEMY_MUMMY || type == MJK_ENEMY_MUMMY1 || type == MJK_ENEMY_MUMMY2) {
                e->type = MJK_ENEMY_MUMMY;
                e->spawn_flash_timer = 0x20u;
                MjkEnemyInternal_InitMummyFall(e, mgr->transform_reload);
                mgr->alive_count++;
            } else {
                e->type = (uint8_t)type;
                if (type == MJK_ENEMY_POWER_COIN) {
                    e->vel_y = MjkQ8_FromInt(-1);
                    e->ai_timer = 16u;
                    e->ai_dir = 0u;
                } else if (type == MJK_ENEMY_BALLOON) {
                    e->vel_y = MjkQ8_FromInt(-1);
                } else if (type == MJK_ENEMY_BONUS_COIN || type == MJK_ENEMY_EXTRA_COIN) {
                    e->falling = true;
                    e->jump_accum = 0x4000u;
                }
            }
            e->sprite_id = MjkEnemyInternal_ArchetypeSpriteId(e->type, 0u, false);
            e->attr_base = MjkEnemyInternal_ArchetypePalette(e->type);
            return true;
        }
    }
    return false;
}

bool MjkEnemyInternal_StepCollectible(MjkEnemy* e, const MjkGame* game) {
    if (e->type == MJK_ENEMY_POWER_COIN) {
        if (e->ai_dir == 0u) {
            if (e->ai_timer > 0u) {
                int32_t pny = (int32_t)e->pos_y + (int32_t)e->vel_y;
                if (pny >= 0) { e->pos_y = (mjk_uq8_8_t)pny; }
                e->ai_timer--;
                if (e->ai_timer == 0u) { e->vel_y = 0; }
            }
        } else {
            uint8_t cf = MjkEnemyInternal_CalcCollisionFlags(game, MjkUQ8_Int(e->pos_x), MjkUQ8_Int(e->pos_y));
            mjk_q8_8_t spd = MjkEnemyInternal_SpeedQ8(game->pkg, 5u);
            if ((cf & 0x01u) != 0u) { e->vel_x = (mjk_q8_8_t)(-spd); }
            else if ((cf & 0x02u) != 0u) { e->vel_x = spd; }
            if ((cf & 0x04u) != 0u) { e->vel_y = (mjk_q8_8_t)(-spd); }
            else if ((cf & 0x08u) != 0u) { e->vel_y = spd; }
            int32_t nx = (int32_t)e->pos_x + (int32_t)e->vel_x;
            int32_t ny = (int32_t)e->pos_y + (int32_t)e->vel_y;
            if (nx < 0 || nx >= 0x10000 || ny < 0 || ny >= 0x10000) {
                e->active = false;
                return false;
            }
            e->pos_x = (mjk_uq8_8_t)nx;
            e->pos_y = (mjk_uq8_8_t)ny;
        }
    } else if (e->type == MJK_ENEMY_BALLOON) {
        int32_t bny = (int32_t)e->pos_y + (int32_t)e->vel_y;
        if (bny < 0) {
            e->active = false;
            return false;
        }
        e->pos_y = (mjk_uq8_8_t)bny;
    } else if ((e->type == MJK_ENEMY_BONUS_COIN || e->type == MJK_ENEMY_EXTRA_COIN) &&
               game && game->room.is_bomb_room) {
        /* ObjCode_9_E_ExtraBonusCoin (prg.asm:9144-9198): cae hasta suelo y camina a speed mod 5. */
        uint8_t ex = MjkUQ8_Int(e->pos_x);
        uint8_t ey = MjkUQ8_Int(e->pos_y);
        uint8_t cf = MjkEnemyInternal_CalcCollisionFlags(game, ex, ey);
        mjk_q8_8_t spd = MjkEnemyInternal_SpeedQ8(game->pkg, 5u);
        if ((cf & 0x04u) != 0u) {
            if (e->falling) {
                e->falling = false;
                e->vel_y = 0;
                e->pos_y = (mjk_uq8_8_t)(e->pos_y & 0xF800u);
                e->vel_x = (ex >= game->jack_x) ? (mjk_q8_8_t)(-spd) : spd;
            } else if ((cf & 0x01u) != 0u) {
                e->vel_x = (mjk_q8_8_t)(-spd);
            } else if ((cf & 0x02u) != 0u) {
                e->vel_x = spd;
            }
        } else {
            if (!e->falling) {
                e->falling = true;
                e->jump_accum = 0x4000u;
                e->vel_x = 0;
            }
            uint32_t acc = (uint32_t)e->jump_accum + 0x00F0u;
            if ((acc >> 8u) >= 0x80u) { acc = (0x7Fu << 8u) | (acc & 0xFFu); }
            e->jump_accum = (uint16_t)acc;
            uint8_t hi = (uint8_t)(e->jump_accum >> 8u);
            uint8_t idx = (hi < 0x40u) ? hi : (uint8_t)((hi ^ 0xFFu) & 0x3Fu);
            uint16_t mag = 0x0380u;
            MjkAssets_ReadCpuU16(game->pkg, (uint16_t)(MJK_ADDR_JUMP_TABLE_C0E3 + (uint16_t)idx * 2u), &mag);
            e->vel_y = (hi < 0x40u) ? (mjk_q8_8_t)(-(int16_t)mag) : (mjk_q8_8_t)mag;
        }
        int32_t nx = (int32_t)e->pos_x + (int32_t)e->vel_x;
        int32_t ny = (int32_t)e->pos_y + (int32_t)e->vel_y;
        if (nx < 0 || nx >= 0x10000 || ny < 0 || ny >= 0x10000) {
            e->active = false;
            return false;
        }
        e->pos_x = (mjk_uq8_8_t)nx;
        e->pos_y = (mjk_uq8_8_t)ny;
    }
    return true;
}

void MjkEnemyInternal_HandleTransmutedPickup(MjkEnemy* e, MjkEnemyManager* mgr,
                                             MjkGame* game, uint8_t ex, uint8_t ey) {
    static const MjkScoreId k_chain[7] = {
        MJK_SCORE_100, MJK_SCORE_200, MJK_SCORE_300, MJK_SCORE_500,
        MJK_SCORE_800, MJK_SCORE_1200, MJK_SCORE_2000
    };
    uint8_t s_idx = (mgr->transmute_streak < 7u) ? mgr->transmute_streak : 6u;
    uint8_t fx = (ex >= 8u) ? (uint8_t)(ex - 8u) : 0u;
    uint8_t fy = (ey >= 8u) ? (uint8_t)(ey - 8u) : 0u;
    MjkGame_AddScore(game, k_chain[s_idx]);
    MjkGame_QueueSound(game, MJK_SND_COIN_COLLECTED);
    game->gold_coins++;
    MjkHud_SpawnFloatingScore(game->popups, MJK_FLOAT_SCORE_SLOTS, fx, fy,
                              k_chain[s_idx], game->score_multiplier, game->room.is_bomb_room);
    if (mgr->transmute_streak < 6u) { mgr->transmute_streak++; }
    e->active = false;
    if (mgr->alive_count > 0u) { mgr->alive_count--; }
}

void MjkEnemyInternal_HandleCollectiblePickup(MjkEnemy* e, MjkEnemyManager* mgr,
                                              MjkGame* game, uint8_t ex, uint8_t ey) {
    uint8_t fx = (ex >= 8u) ? (uint8_t)(ex - 8u) : 0u;
    uint8_t fy = (ey >= 8u) ? (uint8_t)(ey - 8u) : 0u;
    switch (e->type) {
        case MJK_ENEMY_POWER_COIN:
            MjkEnemy_ActivatePowerCoinEx(mgr, game);
            e->active = false;
            break;
        case MJK_ENEMY_EXTRA_COIN:
        case MJK_ENEMY_BROTHER:
            game->lives++;
            game->extra_lives_found++;
            game->collected_1up = 1u;
            MjkGame_QueueSound(game, MJK_SND_1UP_CAPTIVE);
            e->active = false;
            break;
        case MJK_ENEMY_SECRET_COIN:
            if (game->mighty_level == 3u) {
                game->secret_coins++;
                game->gdv_bcd = MjkGame_AddGdvBcd(game->gdv_bcd, 1u);
                MjkGame_AddScore(game, MJK_SCORE_50000);
                MjkGame_QueueSound(game, MJK_SND_COIN_COLLECTED);
                MjkHud_SpawnFloatingScore(game->popups, MJK_FLOAT_SCORE_SLOTS, fx, fy,
                                          MJK_SCORE_50000, game->score_multiplier,
                                          game->room.is_bomb_room);
                e->active = false;
            }
            break;
        case MJK_ENEMY_CAPTIVE:
            MjkGame_AddScore(game, MJK_SCORE_2000);
            MjkGame_QueueSound(game, MJK_SND_BONUS_STATUE);
            MjkHud_SpawnFloatingScore(game->popups, MJK_FLOAT_SCORE_SLOTS, fx, fy,
                                      MJK_SCORE_2000, game->score_multiplier,
                                      game->room.is_bomb_room);
            e->active = false;
            break;
        case MJK_ENEMY_BALLOON:
            MjkGame_AddScore(game, MJK_SCORE_10000);
            MjkGame_QueueSound(game, MJK_SND_BOMB_LIT);
            MjkHud_SpawnFloatingScore(game->popups, MJK_FLOAT_SCORE_SLOTS, fx, fy,
                                      MJK_SCORE_10000, game->score_multiplier,
                                      game->room.is_bomb_room);
            e->active = false;
            break;
        case MJK_ENEMY_BONUS_COIN:
            if (game->score_multiplier < 4u) { game->score_multiplier++; }
            MjkGame_AddScore(game, MJK_SCORE_1000);
            MjkGame_QueueSound(game, MJK_SND_COIN_COLLECTED);
            MjkHud_SpawnFloatingScore(game->popups, MJK_FLOAT_SCORE_SLOTS, fx, fy,
                                      MJK_SCORE_1000, game->score_multiplier,
                                      game->room.is_bomb_room);
            e->active = false;
            break;
        default:
            e->active = false;
            break;
    }
}
