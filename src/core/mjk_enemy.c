/**
 * @file mjk_enemy.c
 * @brief Implementación canónica de la IA de enemigos, spawning diferido, mutación determinista y Power Coins.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: RES-025 y RES-028 (asm/src/prg.asm:8150-8650, 9208-9575)
 *                   asm/src/data.asm:116-141 (DifficultyTable), 1963-2220 (EnemySpawnPositionTable)
 */

#include "mjk_enemy.h"
#include <string.h>
#include <stdlib.h>

/*
 * Tabla canónica de dificultad según ronda (data.asm:131-141)
 * Round 1/2:  speed=0, spawn=$A0 (160), tf_norm=$80 (128), tf_bomb=$40 (64)
 * Round 3:    speed=1, spawn=$80 (128), tf_norm=$70 (112), tf_bomb=$38 (56)
 * Round 4:    speed=2, spawn=$70 (112), tf_norm=$60 (96),  tf_bomb=$30 (48)
 * Round 5:    speed=3, spawn=$60 (96),  tf_norm=$50 (80),  tf_bomb=$28 (40)
 * Round 6:    speed=4, spawn=$58 (88),  tf_norm=$40 (64),  tf_bomb=$20 (32)
 * Round 7:    speed=5, spawn=$50 (80),  tf_norm=$30 (48),  tf_bomb=$18 (24)
 * Round 8+:   speed=4, spawn=$48 (72),  tf_norm=$20 (32),  tf_bomb=$10 (16)
 */
static const MjkDifficultySetting k_difficulty_table[7] = {
    { 0, 0xA0, 0x80, 0x40 },
    { 1, 0x80, 0x70, 0x38 },
    { 2, 0x70, 0x60, 0x30 },
    { 3, 0x60, 0x50, 0x28 },
    { 4, 0x58, 0x40, 0x20 },
    { 5, 0x50, 0x30, 0x18 },
    { 4, 0x48, 0x20, 0x10 },
};

/*
 * Velocidades canónicas en punto fijo Q8.8 para enemigos mutados (data.asm:178-186 word_C0D3)
 */
static const int16_t k_enemy_speeds[8] = {
    0x0058, 0x0070, 0x0088, 0x00A0, 0x00B8, 0x00D0, 0x00E8, 0x0100
};

/*
 * Puntos de generación canónicos por sala para las 256 salas (asm/src/data.asm:1963-2220 EnemySpawnPositionTable)
 * Coordenadas nibble $YX (Y = nibble alto * 16, X = nibble bajo * 16)
 * Si el primer byte es 0x00, la sala aborta el spawning (prg.asm:8704-8710)
 */
static const uint8_t k_enemy_spawn_pos_table[256][2] = {
    /* Room   0-  7 */ { 0x00, 0x00 }, { 0x00, 0x00 }, { 0xB1, 0x4A }, { 0x44, 0x2B }, { 0x53, 0xBD }, { 0xCC, 0x00 }, { 0x62, 0xCB }, { 0x6C, 0xA3 },
    /* Room   8- 15 */ { 0x14, 0x9B }, { 0x4B, 0xD4 }, { 0x4B, 0xE3 }, { 0xB8, 0x4C }, { 0x66, 0x7D }, { 0x8A, 0x00 }, { 0xD4, 0x2B }, { 0x80, 0x66 },
    /* Room  16- 23 */ { 0x64, 0xB8 }, { 0xA3, 0x5A }, { 0xC7, 0xCC }, { 0x63, 0x6B }, { 0x63, 0x6B }, { 0x43, 0x69 }, { 0xC6, 0xCE }, { 0x66, 0x6C },
    /* Room  24- 31 */ { 0xC6, 0x18 }, { 0xC4, 0x74 }, { 0xA3, 0x4B }, { 0xDB, 0x45 }, { 0xDE, 0x45 }, { 0xB4, 0x7B }, { 0xA3, 0x8C }, { 0xA3, 0xDE },
    /* Room  32- 39 */ { 0x4B, 0xC5 }, { 0x74, 0xDB }, { 0xC7, 0xC9 }, { 0xCE, 0x00 }, { 0xC3, 0x7C }, { 0x9E, 0xA3 }, { 0x78, 0xDB }, { 0x1A, 0xC8 },
    /* Room  40- 47 */ { 0x95, 0x31 }, { 0xC8, 0x48 }, { 0x4B, 0x00 }, { 0xC8, 0x48 }, { 0xDE, 0x6E }, { 0xDE, 0x3E }, { 0x5A, 0xC3 }, { 0x6C, 0x62 },
    /* Room  48- 55 */ { 0x37, 0x61 }, { 0x6B, 0x61 }, { 0x98, 0x64 }, { 0x79, 0x16 }, { 0xBC, 0x5C }, { 0xC5, 0x5E }, { 0xDD, 0x6D }, { 0xBD, 0x5D },
    /* Room  56- 63 */ { 0x96, 0x32 }, { 0xDE, 0x6D }, { 0xCE, 0x64 }, { 0x6D, 0xC7 }, { 0x6C, 0x65 }, { 0x4F, 0xC6 }, { 0x45, 0x00 }, { 0x31, 0xC1 },
    /* Room  64- 71 */ { 0x2E, 0xA2 }, { 0x4B, 0xC7 }, { 0x58, 0x00 }, { 0x8A, 0x54 }, { 0xCF, 0x76 }, { 0x88, 0xC1 }, { 0x4C, 0x67 }, { 0x6C, 0x66 },
    /* Room  72- 79 */ { 0x1E, 0x13 }, { 0x74, 0x79 }, { 0xC8, 0x31 }, { 0xD7, 0x27 }, { 0x59, 0x95 }, { 0x15, 0x1C }, { 0x03, 0x97 }, { 0x17, 0x84 },
    /* Room  80- 87 */ { 0x17, 0x84 }, { 0x17, 0x84 }, { 0x64, 0xCC }, { 0x15, 0x1C }, { 0x1B, 0x33 }, { 0x1B, 0x33 }, { 0x1B, 0x33 }, { 0x1B, 0x33 },
    /* Room  88- 95 */ { 0x1B, 0x33 }, { 0x1B, 0x33 }, { 0x95, 0x3B }, { 0x98, 0x6D }, { 0x22, 0x9A }, { 0x63, 0x6D }, { 0x69, 0x4E }, { 0x23, 0x48 },
    /* Room  96-103 */ { 0xC3, 0x6B }, { 0x35, 0x3B }, { 0x35, 0x3D }, { 0x46, 0x3D }, { 0x35, 0x3B }, { 0xCE, 0x35 }, { 0x00, 0x00 }, { 0x45, 0xC8 },
    /* Room 104-111 */ { 0x2C, 0xC6 }, { 0xC3, 0x49 }, { 0x22, 0x2A }, { 0x68, 0x4F }, { 0x47, 0x5F }, { 0x43, 0x4A }, { 0x64, 0x3C }, { 0xDB, 0x7A },
    /* Room 112-119 */ { 0xDC, 0x78 }, { 0x79, 0xBD }, { 0x15, 0x1B }, { 0x03, 0x97 }, { 0x17, 0x84 }, { 0x17, 0x84 }, { 0x64, 0xCC }, { 0x15, 0x1B },
    /* Room 120-127 */ { 0x1B, 0x33 }, { 0x1B, 0x33 }, { 0x1B, 0x33 }, { 0x1B, 0x33 }, { 0x1B, 0x33 }, { 0xBD, 0x57 }, { 0xCC, 0x35 }, { 0x4E, 0x66 },
    /* Room 128-135 */ { 0x57, 0x94 }, { 0x37, 0xA6 }, { 0x3D, 0x36 }, { 0x7C, 0xC3 }, { 0xCC, 0x65 }, { 0x87, 0x45 }, { 0x4E, 0x98 }, { 0xC8, 0x32 },
    /* Room 136-143 */ { 0xA2, 0x32 }, { 0xC3, 0x4B }, { 0x43, 0x4B }, { 0xC2, 0xAE }, { 0x68, 0x4C }, { 0x65, 0x6B }, { 0x31, 0x2C }, { 0xA3, 0x87 },
    /* Room 144-151 */ { 0x45, 0x3A }, { 0x43, 0x4C }, { 0x35, 0x39 }, { 0x34, 0x3B }, { 0x54, 0x5B }, { 0x43, 0x6A }, { 0x55, 0x5A }, { 0x44, 0x4C },
    /* Room 152-159 */ { 0x46, 0x4A }, { 0x33, 0x3C }, { 0x26, 0x2A }, { 0x34, 0x3B }, { 0x43, 0x3C }, { 0x26, 0x7A }, { 0x36, 0x39 }, { 0x36, 0x3A },
    /* Room 160-167 */ { 0x63, 0x6C }, { 0x65, 0x6A }, { 0x64, 0x6B }, { 0x63, 0x6C }, { 0x65, 0x6C }, { 0x63, 0x6A }, { 0x00, 0x00 }, { 0x00, 0x00 },
    /* Room 168-175 */ { 0x33, 0x3C }, { 0x35, 0x3D }, { 0x45, 0xDB }, { 0xD8, 0xDE }, { 0x35, 0x39 }, { 0x19, 0x48 }, { 0x4C, 0x9C }, { 0xD3, 0xDB },
    /* Room 176-183 */ { 0xD2, 0xD8 }, { 0x24, 0xDC }, { 0x33, 0x3A }, { 0x00, 0x00 }, { 0xAE, 0xB1 }, { 0x19, 0xC7 }, { 0xD3, 0x7B }, { 0xD3, 0xDA },
    /* Room 184-191 */ { 0xD5, 0xDB }, { 0x34, 0x3A }, { 0x33, 0x7B }, { 0x45, 0x4A }, { 0xD3, 0x2B }, { 0x74, 0x3E }, { 0x35, 0xD9 }, { 0xD4, 0xD9 },
    /* Room 192-199 */ { 0x58, 0x93 }, { 0x4B, 0x51 }, { 0x6B, 0x00 }, { 0x68, 0x62 }, { 0x4A, 0xC2 }, { 0x4B, 0x31 }, { 0xBB, 0x18 }, { 0xA2, 0x6D },
    /* Room 200-207 */ { 0x64, 0xCD }, { 0x63, 0x7C }, { 0xC6, 0x2C }, { 0xE2, 0x37 }, { 0x62, 0x2C }, { 0x9C, 0x3C }, { 0xD4, 0xDB }, { 0x45, 0x4B },
    /* Room 208-215 */ { 0xD4, 0xDB }, { 0xD4, 0x3B }, { 0xD6, 0xDC }, { 0xD7, 0x3A }, { 0xD5, 0xDB }, { 0xD6, 0x48 }, { 0xD6, 0x3A }, { 0xD5, 0xDB },
    /* Room 216-223 */ { 0x76, 0x7C }, { 0xD5, 0xDB }, { 0x66, 0x6C }, { 0xD5, 0xDB }, { 0x56, 0x5C }, { 0x55, 0x5B }, { 0x76, 0x7C }, { 0x54, 0x5B },
    /* Room 224-231 */ { 0x64, 0x6C }, { 0x54, 0x5B }, { 0x53, 0x5A }, { 0xD5, 0xDB }, { 0xD5, 0xDB }, { 0xD4, 0xDB }, { 0x53, 0x59 }, { 0xD4, 0xDA },
    /* Room 232-239 */ { 0x53, 0x5A }, { 0xD5, 0xDB }, { 0x55, 0x5C }, { 0xD5, 0xDB }, { 0x55, 0x5C }, { 0x55, 0x5B }, { 0x54, 0x5A }, { 0xD3, 0xDA },
    /* Room 240-247 */ { 0x54, 0x5A }, { 0xD5, 0xDA }, { 0x54, 0xDA }, { 0x54, 0x5B }, { 0xD3, 0x3A }, { 0xD5, 0xD9 }, { 0xD5, 0x59 }, { 0xD6, 0xDA },
    /* Room 248-255 */ { 0xD5, 0xDA }, { 0x54, 0x5A }, { 0xC9, 0x64 }, { 0xCD, 0xC6 }, { 0x00, 0x00 }, { 0x00, 0x00 }, { 0x00, 0x00 }, { 0x00, 0x00 },
};

static inline bool IsMetatileSolid(uint8_t tile_id) {
    if (tile_id == 0x00 || tile_id == 0x27 || tile_id == 0x28 || tile_id == 0x2F ||
        tile_id == 0x34 || (tile_id >= 0x47 && tile_id <= 0x50) ||
        tile_id == 0x63 || tile_id == 0x8F || tile_id == 0x90 || tile_id == 0x91 ||
        (tile_id >= 0x81 && tile_id <= 0x98)) {
        return false;
    }
    return true;
}

const MjkDifficultySetting* MjkEnemy_GetDifficulty(uint8_t round) {
    if (round == 0) round = 1;
    uint8_t idx = 0;
    if (round <= 2) idx = 0;
    else if (round == 3) idx = 1;
    else if (round == 4) idx = 2;
    else if (round == 5) idx = 3;
    else if (round == 6) idx = 4;
    else if (round == 7) idx = 5;
    else idx = 6;
    return &k_difficulty_table[idx];
}

void MjkEnemy_Init(MjkEnemyManager* mgr, uint8_t round, uint8_t room, bool is_bomb_room) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(MjkEnemyManager));

    mgr->current_round = round;
    mgr->current_room = room;
    mgr->is_bomb_room = is_bomb_room;

    const MjkDifficultySetting* diff = MjkEnemy_GetDifficulty(round);
    mgr->speed_modifier = diff->speed_modifier;
    mgr->spawn_timer = diff->spawn_timer;
    mgr->transform_timer = is_bomb_room ? diff->transform_timer_bomb_room : diff->transform_timer_normal;
    mgr->alive_enemy_count = 0;
    mgr->spawn_point_toggle = 0;
}

void MjkEnemy_InitAll(MjkEnemyManager* mgr) {
    MjkEnemy_Init(mgr, 1, 1, false);
}

int MjkEnemy_Spawn(MjkEnemyManager* mgr, EnemyType type, int16_t x, int16_t y) {
    if (!mgr) return -1;

    for (int i = 0; i < MJK_MAX_ENEMIES; ++i) {
        if (!mgr->enemies[i].active) {
            EnemyEntity* e = &mgr->enemies[i];
            e->active = true;
            e->type = type;
            e->pos_x = FIXED_FROM_INT(x);
            e->pos_y = FIXED_FROM_INT(y);
            e->vel_x = (i % 2 == 0) ? 1 : -1;
            e->vel_y = 0;
            e->subpixel_vx = (e->vel_x < 0) ? -0x0080 : 0x0080;
            e->subpixel_vy = 0;
            e->facing_left = (e->vel_x < 0);
            e->state_timer = 0;
            e->anim_frame = 0;
            e->status = 0;
            e->flash_timer = 0;
            mgr->alive_enemy_count++;
            return i;
        }
    }
    return -1;
}

void MjkEnemy_TickSpawning(MjkEnemyManager* mgr) {
    if (!mgr) return;

    // Si los enemigos están transmutados en monedas, el spawn se detiene (prg.asm:8178-8182)
    if (mgr->transmute_timer > 0) return;

    if (mgr->spawn_timer > 0) {
        mgr->spawn_timer--;
    }

    if (mgr->spawn_timer == 0) {
        const MjkDifficultySetting* diff = MjkEnemy_GetDifficulty(mgr->current_round);
        mgr->spawn_timer = diff->spawn_timer;

        // Máximo 4 enemigos vivos simultáneamente (prg.asm:8203 CMP #4, BCC _SpawnEnemy)
        if (mgr->alive_enemy_count < MJK_MAX_ALIVE_ENEMIES) {
            uint8_t room = mgr->current_room;
            uint8_t raw_pos0 = k_enemy_spawn_pos_table[room][0];

            // Si el primer byte de spawn es 0x00, la sala aborta el spawn (prg.asm:8704-8710)
            if (raw_pos0 == 0x00) {
                return;
            }

            uint8_t coord_byte = k_enemy_spawn_pos_table[room][mgr->spawn_point_toggle];
            mgr->spawn_point_toggle ^= 1; // Alterna entre punto 0 y 1

            if (coord_byte == 0x00) {
                coord_byte = raw_pos0;
            }

            int16_t spawn_y = (int16_t)(coord_byte & 0xF0);
            int16_t spawn_x = (int16_t)((coord_byte & 0x0F) * 16);

            int slot = MjkEnemy_Spawn(mgr, ENEMY_TYPE_MUMMY, spawn_x, spawn_y);
            if (slot >= 0) {
                // Ciclo de 32 frames de spawn (16 frames invisible + 16 frames nube de humo)
                mgr->enemies[slot].status |= 0x08;
                mgr->enemies[slot].flash_timer = 0x20;
            }
        }
    }
}

void MjkEnemy_MutateMummy(EnemyEntity* enemy, uint8_t a_press_counter) {
    if (!enemy || enemy->type != ENEMY_TYPE_MUMMY) return;

    /*
     * Fórmula 6502 determinista de prg.asm:9362-9375:
     * NewType = (A % 6) + 2
     * Mapea a: 2 (Hanezo), 3 (GejiShogun), 4 (Gameido), 5 (Dokuron), 6 (Desufa), 7 (Horus)
     */
    uint8_t new_type = (uint8_t)((a_press_counter % 6) + 2);
    enemy->type = (EnemyType)new_type;
    enemy->anim_frame = 0;
    enemy->state_timer = 0;
}

void MjkEnemy_ActivatePowerCoin(MjkEnemyManager* mgr) {
    if (!mgr) return;
    mgr->transmute_timer = MJK_POWER_COIN_DURATION_FRAMES;
    mgr->transmute_streak = 0;
}

void MjkEnemy_UpdateAll(MjkEnemyManager* mgr, const JackPlayer* jack, const uint8_t* room_metatiles, uint8_t a_press_counter) {
    if (!mgr) return;

    // Decrementar temporizador de transmutación
    if (mgr->transmute_timer > 0) {
        mgr->transmute_timer--;
    }

    // Gestionar el spawning continuo diferido
    MjkEnemy_TickSpawning(mgr);

    int16_t spd = k_enemy_speeds[mgr->speed_modifier & 0x07];
    int sx = jack ? jack->scroll_x : 0;
    int sy = jack ? jack->scroll_y : 0;

    for (int i = 0; i < MJK_MAX_ENEMIES; ++i) {
        EnemyEntity* e = &mgr->enemies[i];
        if (!e->active) continue;

        e->state_timer++;

        // Temporizador de 32 frames de spawn/mutación (Status & 0x08, EnemyStruct_12)
        if (e->status & 0x08) {
            if (e->flash_timer > 0) {
                e->flash_timer--;
            }
            if (e->flash_timer == 0) {
                e->status &= ~0x08;
            }
            continue; // Inmóvil e inofensivo durante humo/spawn
        }

        // Si están transmutados en monedas, permanecen inmóviles
        if (mgr->transmute_timer > 0) {
            continue;
        }

        int ex = FIXED_TO_INT(e->pos_x);
        int ey = FIXED_TO_INT(e->pos_y);
        int wx = ex + sx;
        int wy = ey + sy;

        switch (e->type) {
            case ENEMY_TYPE_MUMMY: {
                // Momia canónica: 0x0080 Q8.8 (0.5 px/f), patrulla de plataformas y caída vertical
                int16_t mummy_spd = 0x0080;
                if (e->vel_x == 0) {
                    e->vel_x = e->facing_left ? -1 : 1;
                }
                e->subpixel_vx = (e->vel_x < 0) ? -mummy_spd : mummy_spd;

                // Sondas inferiores canónicas: pie trasero y pie delantero (sub_9965 / loc_BBCF)
                int back_x = (e->vel_x > 0) ? (wx + 2) : (wx + 14);
                int front_x = (e->vel_x > 0) ? (wx + 14) : (wx + 2);
                int foot_y = wy + 16;
                int back_col = back_x / 16;
                int front_col = front_x / 16;
                int foot_row = foot_y / 16;

                bool back_on_ground = false;
                bool front_on_ground = false;

                if (room_metatiles && foot_row >= 0 && foot_row < 15) {
                    if (back_col >= 0 && back_col < 16 && IsMetatileSolid(room_metatiles[foot_row * 16 + back_col])) {
                        back_on_ground = true;
                    }
                    if (front_col >= 0 && front_col < 16 && IsMetatileSolid(room_metatiles[foot_row * 16 + front_col])) {
                        front_on_ground = true;
                    }
                }

                if (!back_on_ground && !front_on_ground && ey < 208) {
                    // Ambos pies en el aire: caída vertical por gravedad (sub_BB1C)
                    e->status |= 0x02;
                    e->pos_y += 0x0200;
                } else {
                    // Sustentado en plataforma
                    e->status &= ~0x02;

                    int next_wx = (e->vel_x < 0) ? (wx - 1) : (wx + 16);
                    int wall_col = next_wx / 16;
                    int wall_row = (wy + 8) / 16;
                    bool hit_wall = false;

                    if (ex <= 16 || ex >= 224) {
                        hit_wall = true;
                    }
                    if (room_metatiles && !hit_wall && wall_col >= 0 && wall_col < 16 && wall_row >= 0 && wall_row < 15) {
                        if (IsMetatileSolid(room_metatiles[wall_row * 16 + wall_col])) {
                            hit_wall = true;
                        }
                    }

                    if (hit_wall || (back_on_ground && !front_on_ground)) {
                        // Inversión ante pared o borde de plataforma (loc_BBCF)
                        e->vel_x = -e->vel_x;
                        e->facing_left = (e->vel_x < 0);
                        e->subpixel_vx = (e->vel_x < 0) ? -mummy_spd : mummy_spd;
                    } else {
                        // Preservar subpíxeles Q8.8
                        e->pos_x += e->subpixel_vx;
                    }
                }

                // Mutación tras cumplir el temporizador de transformación
                if (e->state_timer >= mgr->transform_timer) {
                    if (!mgr->is_bomb_room) {
                        MjkEnemy_MutateMummy(e, a_press_counter);
                        e->status |= 0x08;
                        e->flash_timer = 0x20; // 32 frames de animación de humo
                    } else {
                        e->state_timer = 0;
                    }
                }
                break;
            }

            case ENEMY_TYPE_HANEZO: {
                // Rebote elástico diagonal a velocidad canónica word_C0D3
                if (e->vel_x == 0) e->vel_x = 1;
                if (e->vel_y == 0) e->vel_y = 1;
                e->subpixel_vx = (e->vel_x < 0) ? -spd : spd;
                e->subpixel_vy = (e->vel_y < 0) ? -spd : spd;
                e->pos_x += e->subpixel_vx;
                e->pos_y += e->subpixel_vy;

                if (ex <= 16 || ex >= 224) {
                    e->vel_x = -e->vel_x;
                    e->facing_left = (e->vel_x < 0);
                }
                if (ey <= 24 || ey >= 208) {
                    e->vel_y = -e->vel_y;
                }
                break;
            }

            case ENEMY_TYPE_GEJI_SHOGUN: {
                // Persigue a Jack en X y reorienta en Y cada 48 frames
                if (jack) {
                    int jx = FIXED_TO_INT(jack->pos_x);
                    e->vel_x = (ex < jx) ? 1 : -1;
                    e->facing_left = (e->vel_x < 0);
                }
                if (jack && (e->state_timer % 48 == 0)) {
                    int jy = FIXED_TO_INT(jack->pos_y);
                    e->vel_y = (ey < jy) ? 1 : -1;
                }
                e->subpixel_vx = (e->vel_x < 0) ? -spd : spd;
                e->subpixel_vy = (e->vel_y < 0) ? -spd : spd;
                e->pos_x += e->subpixel_vx;
                e->pos_y += e->subpixel_vy;

                if (ex <= 16 || ex >= 224) {
                    e->vel_x = -e->vel_x;
                    e->facing_left = (e->vel_x < 0);
                }
                if (ey <= 24 || ey >= 208) {
                    e->vel_y = -e->vel_y;
                }
                break;
            }

            case ENEMY_TYPE_GAMEIDO: {
                // Rebote diagonal 45° a velocidad canónica
                if (e->vel_x == 0) e->vel_x = 1;
                if (e->vel_y == 0) e->vel_y = 1;
                e->subpixel_vx = (e->vel_x < 0) ? -spd : spd;
                e->subpixel_vy = (e->vel_y < 0) ? -spd : spd;
                e->pos_x += e->subpixel_vx;
                e->pos_y += e->subpixel_vy;

                if (ex <= 16 || ex >= 224) {
                    e->vel_x = -e->vel_x;
                    e->facing_left = (e->vel_x < 0);
                }
                if (ey <= 24 || ey >= 208) {
                    e->vel_y = -e->vel_y;
                }
                break;
            }

            case ENEMY_TYPE_DOKURON: {
                // Rebote vertical y persecución horizontal que alterna cada 32 frames
                if (jack && (e->state_timer % 32 == 0)) {
                    int jx = FIXED_TO_INT(jack->pos_x);
                    e->vel_x = (ex < jx) ? 1 : -1;
                    e->facing_left = (e->vel_x < 0);
                }
                if (e->vel_y == 0) e->vel_y = 1;
                e->subpixel_vx = (e->vel_x < 0) ? -spd : spd;
                e->subpixel_vy = (e->vel_y < 0) ? -spd : spd;
                e->pos_x += e->subpixel_vx;
                e->pos_y += e->subpixel_vy;

                if (ex <= 16 || ex >= 224) {
                    e->vel_x = -e->vel_x;
                    e->facing_left = (e->vel_x < 0);
                }
                if (ey <= 24 || ey >= 208) {
                    e->vel_y = -e->vel_y;
                }
                break;
            }

            case ENEMY_TYPE_DESUFA: {
                // Rebote horizontal y rastreo vertical de Jack cada 32 frames
                if (jack && (e->state_timer % 32 == 0)) {
                    int jy = FIXED_TO_INT(jack->pos_y);
                    e->vel_y = (ey < jy) ? 1 : -1;
                }
                if (e->vel_x == 0) e->vel_x = 1;
                e->subpixel_vx = (e->vel_x < 0) ? -spd : spd;
                e->subpixel_vy = (e->vel_y < 0) ? -spd : spd;
                e->pos_x += e->subpixel_vx;
                e->pos_y += e->subpixel_vy;

                if (ex <= 16 || ex >= 224) {
                    e->vel_x = -e->vel_x;
                    e->facing_left = (e->vel_x < 0);
                }
                if (ey <= 24 || ey >= 208) {
                    e->vel_y = -e->vel_y;
                }
                break;
            }

            case ENEMY_TYPE_HORUS: {
                // Vuelo cardinal que prioriza el eje mayor respecto a Jack cada 16 frames
                if (jack && (e->state_timer % 16 == 0)) {
                    int jx = FIXED_TO_INT(jack->pos_x);
                    int jy = FIXED_TO_INT(jack->pos_y);
                    int dx = abs(jx - ex);
                    int dy = abs(jy - ey);
                    if (dx >= dy) {
                        e->vel_x = (ex < jx) ? 1 : -1;
                        e->vel_y = 0;
                    } else {
                        e->vel_y = (ey < jy) ? 1 : -1;
                        e->vel_x = 0;
                    }
                    e->facing_left = (e->vel_x < 0);
                }
                e->subpixel_vx = (e->vel_x < 0) ? -spd : ((e->vel_x > 0) ? spd : 0);
                e->subpixel_vy = (e->vel_y < 0) ? -spd : ((e->vel_y > 0) ? spd : 0);
                e->pos_x += e->subpixel_vx;
                e->pos_y += e->subpixel_vy;

                if (ex <= 16 || ex >= 224) {
                    e->vel_x = -e->vel_x;
                    e->facing_left = (e->vel_x < 0);
                }
                if (ey <= 24 || ey >= 208) {
                    e->vel_y = -e->vel_y;
                }
                break;
            }

            default: {
                // Movimiento horizontal estándar
                if (e->vel_x == 0) e->vel_x = 1;
                e->subpixel_vx = (e->vel_x < 0) ? -spd : spd;
                e->pos_x += e->subpixel_vx;
                if (ex <= 16 || ex >= 224) {
                    e->vel_x = -e->vel_x;
                    e->facing_left = (e->vel_x < 0);
                }
                break;
            }
        }
    }
}

int32_t MjkEnemy_CheckJackCollisions(MjkEnemyManager* mgr, JackPlayer* jack) {
    if (!mgr || !jack || !jack->is_alive) return 0;

    int jx = FIXED_TO_INT(jack->pos_x);
    int jy = FIXED_TO_INT(jack->pos_y);

    // Caja de colisión de Jack (12x12 pixels centrada)
    int j_l = jx + 2;
    int j_r = jx + 14;
    int j_t = jy + 2;
    int j_b = jy + 14;

    for (int i = 0; i < MJK_MAX_ENEMIES; ++i) {
        EnemyEntity* e = &mgr->enemies[i];
        if (!e->active) continue;

        // Inmune y no letal durante el humo/spawn de 32 frames (prg.asm:3981-3982)
        if (e->status & 0x08) continue;

        int ex = FIXED_TO_INT(e->pos_x);
        int ey = FIXED_TO_INT(e->pos_y);

        // Caja de colisión del enemigo (12x12 pixels)
        int e_l = ex + 2;
        int e_r = ex + 14;
        int e_t = ey + 2;
        int e_b = ey + 14;

        // Comprobación de solapamiento AABB
        if (j_r >= e_l && j_l <= e_r && j_b >= e_t && j_t <= e_b) {
            // Caso 1: Enemigos transmutados en monedas (Power Coin activa)
            if (mgr->transmute_timer > 0) {
                e->active = false;
                if (mgr->alive_enemy_count > 0) {
                    mgr->alive_enemy_count--;
                }
                // Escala exponencial: 100, 200, 400, 800, 1000, 2000, 4000, 8000
                static const int32_t CoinBonusScale[8] = {100, 200, 400, 800, 1000, 2000, 4000, 8000};
                int idx = mgr->transmute_streak < 8 ? mgr->transmute_streak : 7;
                mgr->transmute_streak++;
                return CoinBonusScale[idx];
            }
            // Caso 2: Jack con nivel Mighty 3 destruye al enemigo
            else if (jack->power_level >= MIGHTY_POWER_GREEN) {
                e->active = false;
                if (mgr->alive_enemy_count > 0) {
                    mgr->alive_enemy_count--;
                }
                return 500;
            }
            // Caso 3: Colisión letal para Jack
            else {
                jack->is_alive = false;
                return -1; // Jack muere
            }
        }
    }
    return 0; // Sin colisión
}
