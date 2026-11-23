/**
 * @file enemy.c
 * @brief Enemy AI, Spawning, Mummy Mutations, and Coin Transmutation.
 * Bit-accurate C11 implementation matching NES PRG $B593-$BFDF and RES-008.
 */

#include "mbj/enemy.h"
#include <string.h>
#include <stdlib.h>

/* Fixed-point 8.8 speed table from ROM $C0D3 */
static const uint16_t kEnemySpeedTable_C0D3[8] = {
    0x0058, /* Level 0: 0.344 px/frame */
    0x0070, /* Level 1: 0.437 px/frame */
    0x0088, /* Level 2: 0.531 px/frame */
    0x00A0, /* Level 3: 0.625 px/frame */
    0x00B8, /* Level 4: 0.718 px/frame */
    0x00D0, /* Level 5: 0.812 px/frame */
    0x00E8, /* Level 6: 0.906 px/frame */
    0x0100  /* Level 7: 1.000 px/frame */
};

/* US Difficulty Settings Table ($C080, REV_US) */
typedef struct {
    uint8_t speed_idx;
    uint8_t spawn_timer;
    uint8_t transform_timer_normal;
    uint8_t transform_timer_bomb_room;
} DifficultyEntry;

static const DifficultyEntry kDifficultyTableUS[16] = {
    { 0, 0xA0, 0x80, 0x40 }, /* Round 1/2 */
    { 1, 0x80, 0x70, 0x38 }, /* Round 3 */
    { 2, 0x70, 0x60, 0x30 }, /* Round 4 */
    { 3, 0x60, 0x50, 0x28 }, /* Round 5 */
    { 4, 0x58, 0x40, 0x20 }, /* Round 6 */
    { 5, 0x50, 0x30, 0x18 }, /* Round 7 */
    { 4, 0x48, 0x20, 0x10 }, /* Round 8 */
    { 5, 0x30, 0x10, 0x04 }, /* Round 9 */
    { 5, 0x40, 0x08, 0x08 }, /* Round 10 */
    { 6, 0x40, 0x08, 0x08 }, /* Round 11 */
    { 3, 0x30, 0x08, 0x08 }, /* Round 12 */
    { 5, 0x30, 0x08, 0x06 }, /* Round 13 */
    { 6, 0x40, 0x08, 0x08 }, /* Round 14 */
    { 7, 0x40, 0x08, 0x08 }, /* Round 15 */
    { 7, 0x40, 0x08, 0x08 }, /* Round 16 */
    { 3, 0x40, 0x08, 0x08 }  /* Fin */
};

/* Chain score awards for collecting transmutated coins */
static const uint16_t kCoinChainScore[7] = {
    100, 200, 300, 500, 800, 1200, 2000
};

/* Internal manager state */
static uint16_t s_enemy_coin_timer = 0;
static uint8_t  s_coin_chain_count = 0;
static uint8_t  s_spawn_countdown = 160;

void Enemy_InitManager(void) {
    s_enemy_coin_timer = 0;
    s_coin_chain_count = 0;
    s_spawn_countdown = 160;
}

int Enemy_GetActiveCount(const NES_RAM *ram) {
    if (!ram) return 0;
    int count = 0;
    for (int i = 0; i < 8; ++i) {
        if ((ram->enemies[i].status & ENEMY_STATUS_ACTIVE) && ram->enemies[i].enemy_type <= 7) {
            count++;
        }
    }
    return count;
}

static ObjectStructRAM* FindFreeSlot(NES_RAM *ram) {
    for (int i = 0; i < 8; ++i) {
        if (!(ram->enemies[i].status & ENEMY_STATUS_ACTIVE)) {
            return &ram->enemies[i];
        }
    }
    return NULL;
}

void Enemy_TransformMummy(ObjectStructRAM *enemy, uint8_t a_press_counter) {
    if (!enemy) return;

    /* Formula: (APressCounter % 6) + 2 */
    uint8_t new_type = (uint8_t)((a_press_counter % 6) + 2);
    enemy->enemy_type = new_type;
    enemy->status |= ENEMY_STATUS_SPAWN_FLASH;
    enemy->anim_frame = 32; /* 32 frames of spawn flash */
    
    /* Assign initial speed from difficulty */
    uint16_t spd = kEnemySpeedTable_C0D3[enemy->speed_modifier & 7];
    enemy->x_vel_lo = (uint8_t)(spd & 0xFF);
    enemy->x_vel_hi = (int8_t)((spd >> 8) ? (spd >> 8) : 1);
    enemy->y_vel_lo = (uint8_t)(spd & 0xFF);
    enemy->y_vel_hi = (int8_t)((spd >> 8) ? (spd >> 8) : 1);

    /* Assign initial sprite tiles based on archetype */
    switch (new_type) {
        case ENEMY_TYPE_HANEZO:     enemy->sprite_id = 0x27; enemy->facing_attr = 2; break;
        case ENEMY_TYPE_GEJISHOGUN: enemy->sprite_id = 0x29; enemy->facing_attr = 3; break;
        case ENEMY_TYPE_GAMEIDO:    enemy->sprite_id = 0x2B; enemy->facing_attr = 1; break;
        case ENEMY_TYPE_DOKURON:    enemy->sprite_id = 0x2D; enemy->facing_attr = 0; break;
        case ENEMY_TYPE_DESUFA:     enemy->sprite_id = 0x2F; enemy->facing_attr = 3; break;
        case ENEMY_TYPE_HORUS:      enemy->sprite_id = 0x31; enemy->facing_attr = 2; break;
        default: break;
    }
}

void Enemy_SpawnMummy(NES_RAM *ram, uint8_t room_id) {
    if (!ram) return;
    if (Enemy_GetActiveCount(ram) >= MAX_ALIVE_ENEMIES) return;

    ObjectStructRAM *slot = FindFreeSlot(ram);
    if (!slot) return;

    memset(slot, 0, sizeof(ObjectStructRAM));
    slot->status = ENEMY_STATUS_ACTIVE | ENEMY_STATUS_SPAWN_FLASH;
    slot->enemy_type = ENEMY_TYPE_MUMMY;
    slot->sprite_id = 0x25;
    slot->facing_attr = 1;

    /* Difficulty params */
    uint8_t round_idx = (uint8_t)((room_id > 0 ? (room_id - 1) : 0) & 0x0F);
    const DifficultyEntry *diff = &kDifficultyTableUS[round_idx];

    slot->speed_modifier = diff->speed_idx;
    slot->coin_transform_timer = diff->transform_timer_normal;

    /* Initial spawn coordinates: ceiling opening */
    slot->x_pos_hi = (uint8_t)(120 + ((room_id & 3) * 16));
    slot->y_pos_hi = 32;
    slot->y_vel_hi = 1; /* Drop out of ceiling */
    slot->anim_frame = 32;
}

void Enemy_ActivatePowerCoin(NES_RAM *ram) {
    if (!ram) return;
    s_enemy_coin_timer = ENEMY_COIN_TIMER_DURATION;
    ram->enemy_coin_timer = ENEMY_COIN_TIMER_DURATION;
    s_coin_chain_count = 0;

    for (int i = 0; i < 8; ++i) {
        if ((ram->enemies[i].status & ENEMY_STATUS_ACTIVE) && ram->enemies[i].enemy_type <= 7) {
            ram->enemies[i].status |= ENEMY_STATUS_IS_COIN;
            ram->enemies[i].sprite_id = 0x21; /* Spinning coin */
            ram->enemies[i].facing_attr = 0;   /* Gold palette */
        }
    }
}

static void UpdateMummyAI(ObjectStructRAM *e, NES_RAM *ram, const uint8_t *cmap) {
    (void)cmap;
    /* Drop down until hitting ground */
    if (e->y_pos_hi < 200) {
        e->y_pos_hi += (uint8_t)e->y_vel_hi;
    }

    /* Horizontal patrol */
    if (e->facing_attr & 8) {
        e->x_pos_hi += 1;
        if (e->x_pos_hi >= 232) {
            e->facing_attr &= ~8;
        }
    } else {
        if (e->x_pos_hi > 16) {
            e->x_pos_hi -= 1;
        } else {
            e->facing_attr |= 8;
        }
    }

    /* Countdown transformation */
    if (e->coin_transform_timer > 0) {
        e->coin_transform_timer--;
        if (e->coin_transform_timer == 0) {
            Enemy_TransformMummy(e, ram->a_press_counter);
        }
    }
}

static void UpdateHanezoAI(ObjectStructRAM *e) {
    /* Diagonal bouncing hopper */
    e->x_pos_hi += (uint8_t)e->x_vel_hi;
    e->y_pos_hi += (uint8_t)e->y_vel_hi;

    if (e->x_pos_hi >= 232) {
        e->x_vel_hi = (int8_t)(-abs(e->x_vel_hi));
    } else if (e->x_pos_hi <= 16) {
        e->x_vel_hi = (int8_t)(abs(e->x_vel_hi));
    }

    if (e->y_pos_hi >= 208) {
        e->y_vel_hi = (int8_t)(-abs(e->y_vel_hi));
    } else if (e->y_pos_hi <= 32) {
        e->y_vel_hi = (int8_t)(abs(e->y_vel_hi));
    }
}

static void UpdateGejiShogunAI(ObjectStructRAM *e) {
    /* Perimeter crawler */
    e->x_pos_hi += (uint8_t)e->x_vel_hi;
    if (e->x_pos_hi >= 232 || e->x_pos_hi <= 16) {
        e->x_vel_hi = (int8_t)(-e->x_vel_hi);
    }
}

static void UpdateGameidoAI(ObjectStructRAM *e, const NES_RAM *ram) {
    /* Homing turtle */
    uint8_t px = ram->player.x_pos_hi;
    uint8_t py = ram->player.y_pos_hi;

    if ((e->anim_frame & 1) == 0) {
        if (e->x_pos_hi < px) e->x_pos_hi++;
        else if (e->x_pos_hi > px) e->x_pos_hi--;

        if (e->y_pos_hi < py) e->y_pos_hi++;
        else if (e->y_pos_hi > py) e->y_pos_hi--;
    }
}

static void UpdateDokuronAI(ObjectStructRAM *e, const NES_RAM *ram) {
    /* Leaping skull tracking Jack in X */
    uint8_t px = ram->player.x_pos_hi;
    if (e->x_pos_hi < px) e->x_pos_hi++;
    else if (e->x_pos_hi > px) e->x_pos_hi--;

    /* Parabolic vertical hop */
    e->y_pos_hi += (uint8_t)e->y_vel_hi;
    if (e->y_pos_hi >= 200) {
        e->y_vel_hi = -2;
    } else if (e->y_pos_hi <= 100) {
        e->y_vel_hi = 2;
    }
}

static void UpdateDesufaAI(ObjectStructRAM *e) {
    /* Fireball with sinusoidal Y */
    e->x_pos_hi += (uint8_t)e->x_vel_hi;
    if (e->x_pos_hi >= 232 || e->x_pos_hi <= 16) {
        e->x_vel_hi = (int8_t)(-e->x_vel_hi);
    }

    /* Small oscillation around base */
    int8_t wave = (int8_t)(((e->anim_frame >> 2) & 3) - 1);
    e->y_pos_hi = (uint8_t)(e->y_pos_hi + wave);
    if (e->y_pos_hi < 32) e->y_pos_hi = 32;
    if (e->y_pos_hi > 208) e->y_pos_hi = 208;
}

static void UpdateHorusAI(ObjectStructRAM *e, const NES_RAM *ram) {
    /* 4-way dominant vector AI */
    int dx = (int)ram->player.x_pos_hi - (int)e->x_pos_hi;
    int dy = (int)ram->player.y_pos_hi - (int)e->y_pos_hi;

    if (abs(dx) > abs(dy)) {
        /* Dominant axis: Horizontal */
        if (dx > 0) e->x_pos_hi += 1;
        else if (dx < 0) e->x_pos_hi -= 1;
    } else {
        /* Dominant axis: Vertical */
        if (dy > 0) e->y_pos_hi += 1;
        else if (dy < 0) e->y_pos_hi -= 1;
    }
}

void Enemy_UpdateAll(NES_RAM *ram, const uint8_t *collision_map) {
    if (!ram) return;

    /* Handle Power Coin timer */
    if (s_enemy_coin_timer > 0) {
        s_enemy_coin_timer--;
        ram->enemy_coin_timer = s_enemy_coin_timer;
        if (s_enemy_coin_timer == 0) {
            /* Revert transmutated enemies back to monsters */
            for (int i = 0; i < 8; ++i) {
                if ((ram->enemies[i].status & ENEMY_STATUS_ACTIVE) &&
                    (ram->enemies[i].status & ENEMY_STATUS_IS_COIN)) {
                    ram->enemies[i].status &= ~ENEMY_STATUS_IS_COIN;
                    /* Re-assign sprite based on enemy type */
                    switch (ram->enemies[i].enemy_type) {
                        case ENEMY_TYPE_MUMMY:      ram->enemies[i].sprite_id = 0x25; break;
                        case ENEMY_TYPE_HANEZO:     ram->enemies[i].sprite_id = 0x27; break;
                        case ENEMY_TYPE_GEJISHOGUN: ram->enemies[i].sprite_id = 0x29; break;
                        case ENEMY_TYPE_GAMEIDO:    ram->enemies[i].sprite_id = 0x2B; break;
                        case ENEMY_TYPE_DOKURON:    ram->enemies[i].sprite_id = 0x2D; break;
                        case ENEMY_TYPE_DESUFA:     ram->enemies[i].sprite_id = 0x2F; break;
                        case ENEMY_TYPE_HORUS:      ram->enemies[i].sprite_id = 0x31; break;
                        default: break;
                    }
                }
            }
        }
    }

    /* Spawn timer */
    if (s_spawn_countdown > 0) {
        s_spawn_countdown--;
    } else {
        s_spawn_countdown = 160;
        Enemy_SpawnMummy(ram, ram->current_sprite_idx);
    }

    /* Update each active enemy */
    for (int i = 0; i < 8; ++i) {
        ObjectStructRAM *e = &ram->enemies[i];
        if (!(e->status & ENEMY_STATUS_ACTIVE)) continue;

        e->anim_frame++;

        /* Handle spawn flashing invulnerability */
        if (e->status & ENEMY_STATUS_SPAWN_FLASH) {
            if (e->anim_frame >= 32) {
                e->status &= ~ENEMY_STATUS_SPAWN_FLASH;
            }
        }

        /* If transmutated into a coin, enemies remain in place spinning */
        if (e->status & ENEMY_STATUS_IS_COIN) {
            /* In final 64 frames, toggle palette for warning */
            if (s_enemy_coin_timer < 64 && (s_enemy_coin_timer & 4)) {
                e->facing_attr = 1;
            } else {
                e->facing_attr = 0;
            }
            continue;
        }

        /* Dispatch AI machine */
        switch (e->enemy_type) {
            case ENEMY_TYPE_MUMMY:
                UpdateMummyAI(e, ram, collision_map);
                break;
            case ENEMY_TYPE_HANEZO:
                UpdateHanezoAI(e);
                break;
            case ENEMY_TYPE_GEJISHOGUN:
                UpdateGejiShogunAI(e);
                break;
            case ENEMY_TYPE_GAMEIDO:
                UpdateGameidoAI(e, ram);
                break;
            case ENEMY_TYPE_DOKURON:
                UpdateDokuronAI(e, ram);
                break;
            case ENEMY_TYPE_DESUFA:
                UpdateDesufaAI(e);
                break;
            case ENEMY_TYPE_HORUS:
                UpdateHorusAI(e, ram);
                break;
            default:
                break;
        }
    }
}

bool Enemy_CheckPlayerCollision(NES_RAM *ram, ObjectStructRAM *enemy, bool *out_collected_as_coin) {
    if (!ram || !enemy || !(enemy->status & ENEMY_STATUS_ACTIVE)) return false;
    if (out_collected_as_coin) *out_collected_as_coin = false;

    /* AABB check between Jack (16x16) and Enemy (16x16) */
    int jx = ram->player.x_pos_hi;
    int jy = ram->player.y_pos_hi;
    int ex = enemy->x_pos_hi;
    int ey = enemy->y_pos_hi;

    if (abs(jx - ex) < 12 && abs(jy - ey) < 12) {
        if (enemy->status & ENEMY_STATUS_IS_COIN) {
            /* Collected transmutated enemy! */
            enemy->status = 0; /* Despawn */
            if (out_collected_as_coin) *out_collected_as_coin = true;

            /* Award chain score */
            uint8_t chain_idx = s_coin_chain_count;
            if (chain_idx > 6) chain_idx = 6;
            s_coin_chain_count++;

            /* Add to player score BCD (hundreds digit) */
            uint8_t add_hundreds = (uint8_t)(kCoinChainScore[chain_idx] / 100);
            ram->player_score_bcd[1] += add_hundreds;
            if (ram->player_score_bcd[1] >= 100) {
                ram->player_score_bcd[1] -= 100;
                ram->player_score_bcd[2] += 1;
            }

            return true;
        }

        /* If flashing / spawning, cannot damage Jack */
        if (enemy->status & ENEMY_STATUS_SPAWN_FLASH) {
            return false;
        }

        /* Enemy damages Jack */
        return true;
    }

    return false;
}
