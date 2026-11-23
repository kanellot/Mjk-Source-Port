/**
 * @file mjk_oracle_api.c
 * @brief Implementación de la API de introspección para el Oráculo de Paridad Jerárquica.
 */

#include "mjk_oracle_api.h"
#include "mjk_room_decoder.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    MjkAssetPackage assets;
    MjkGameState game;
    MjkFrameOutput output;
    uint32_t current_frame;
    uint8_t prev_held;
} MjkOracleContext;

MJK_API void* MjkOracle_Create(const char* asset_path) {
    MjkOracleContext* ctx = (MjkOracleContext*)calloc(1, sizeof(MjkOracleContext));
    if (!ctx) return NULL;

    const char* path = (asset_path && asset_path[0]) ? asset_path : "assets/assets.dat";
    if (!MjkAssets_Load(path, &ctx->assets)) {
        free(ctx);
        return NULL;
    }

    MjkGame_Init(&ctx->game, &ctx->assets);
    return ctx;
}

MJK_API void MjkOracle_Destroy(void* oracle) {
    if (!oracle) return;
    MjkOracleContext* ctx = (MjkOracleContext*)oracle;
    MjkAssets_Unload(&ctx->assets);
    free(ctx);
}

MJK_API void MjkOracle_Step(void* oracle, uint8_t input_mask) {
    if (!oracle) return;
    MjkOracleContext* ctx = (MjkOracleContext*)oracle;

    MjkInputState inputs;
    inputs.held = input_mask;
    inputs.just_pressed = (uint8_t)(input_mask & ~ctx->prev_held);
    inputs.just_released = (uint8_t)(ctx->prev_held & ~input_mask);
    ctx->prev_held = input_mask;

    MjkGame_SimulateFrame(&ctx->game, &inputs, &ctx->output);
    ctx->current_frame++;
}

MJK_API void MjkOracle_GetSnapshot(void* oracle, MjkCanonicalSnapshot* snap) {
    if (!oracle || !snap) return;
    MjkOracleContext* ctx = (MjkOracleContext*)oracle;
    MjkGameState* g = &ctx->game;
    memset(snap, 0, sizeof(MjkCanonicalSnapshot));

    // --- NIVEL 1: Macro-Flujo y Máquina de Estados ---
    snap->frame = ctx->current_frame;
    snap->game_state = (uint8_t)g->current_state;
    snap->title_screen_state = (uint8_t)g->title_screen_state;
    snap->in_game = g->in_game ? 1 : 0;
    snap->current_room = g->current_room;
    snap->state_timer = g->state_timer;
    snap->stage_timer_seconds = g->stage_timer_seconds;
    snap->greed_jump_counter = g->greed_jump_counter;
    snap->lives_count = g->lives_count;
    memcpy(snap->score_bcd, g->score_bcd, 4);

    // --- NIVEL 2: Objetos y Persistencia del Mundo ---
    snap->door_tile_x = g->door_tile_x;
    snap->door_tile_y = g->door_tile_y;
    snap->door_is_open = g->door_is_open ? 1 : 0;
    snap->door_condition_id = g->door_condition_id;
    snap->bomb_count = g->bomb_count;
    snap->active_fire_bomb_index = g->active_fire_bomb_index;
    snap->chest_count = g->chest_count;

    int slot_idx = MjkRoom_FindSlot(&g->room_data_ram, g->current_room);
    if (slot_idx >= 0 && slot_idx < MJK_ROOMDATA_SECTION_SLOTS) {
        memcpy(snap->room_data_slot, g->room_data_ram.slots[slot_idx].raw, 16);
    }

    // --- NIVEL 3: Actores y Ciclo de Vida de Enemigos ---
    snap->active_enemies_count = g->enemy_mgr.alive_enemy_count;
    for (int i = 0; i < 6 && i < MJK_MAX_ENEMIES; ++i) {
        snap->enemies[i].type = (uint8_t)g->enemy_mgr.enemies[i].type;
        snap->enemies[i].status = g->enemy_mgr.enemies[i].status;
        snap->enemies[i].x = (int16_t)(g->enemy_mgr.enemies[i].pos_x >> 8);
        snap->enemies[i].y = (int16_t)(g->enemy_mgr.enemies[i].pos_y >> 8);
        snap->enemies[i].timer = g->enemy_mgr.enemies[i].state_timer;
        snap->enemies[i].active = g->enemy_mgr.enemies[i].active ? 1 : 0;
    }
    snap->enemy_coin_timer = g->enemy_coin_timer;
    snap->spark_visible = g->bomb_fuse.spark_visible ? 1 : 0;
    snap->spark_x = g->bomb_fuse.spark_sprite_x;
    snap->spark_y = g->bomb_fuse.spark_sprite_y;

    // --- NIVEL 4: Cinemática, Físicas Continuas y Jack ---
    if (!g->in_game) {
        snap->jack_x = (g->title_screen_state == TITLE_STATE_5_JACK_FALLING) ? FIXED_TO_INT(g->title_jack_x) : 40;
        snap->jack_y = (g->title_screen_state == TITLE_STATE_5_JACK_FALLING) ? FIXED_TO_INT(g->title_jack_y) : 120;
    } else {
        snap->jack_x = FIXED_TO_INT(g->jack.pos_x);
        snap->jack_y = FIXED_TO_INT(g->jack.pos_y);
        snap->jack_sub_x = (uint8_t)(g->jack.pos_x & 0x0F);
        snap->jack_sub_y = (uint8_t)(g->jack.pos_y & 0x0F);
        snap->jack_vel_x = (int16_t)g->jack.vel_x;
        snap->jack_vel_y = (int16_t)g->jack.vel_y;
        snap->jack_state = (uint8_t)g->jack.state;
        snap->on_ground = g->jack.on_ground ? 1 : 0;
        snap->flutter_active = g->jack.flutter_active ? 1 : 0;
    }

    // --- NIVEL 5: Micro-Visual y Sprites OAM ---
    snap->backdrop_color = ctx->output.framebuffer[0];
}

MJK_API const uint32_t* MjkOracle_GetFramebuffer(void* oracle) {
    if (!oracle) return NULL;
    MjkOracleContext* ctx = (MjkOracleContext*)oracle;
    return ctx->output.framebuffer;
}
