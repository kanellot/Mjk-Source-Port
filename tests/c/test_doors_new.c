/**
 * @file test_doors_new.c
 * @brief Unit tests exhaustivos de las 33 condiciones de puertas, Esfinge y cruce (R-10).
 */

#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/doors.h"
#include "mjk/game.h"
#include "mjk/rom_loader.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

int main(void) {
    char rom_path[512];
    MjkTestRomStatus st = MjkRom_ResolveTestPath(MJK_SOURCE_DIR, MJK_TEST_ROM_PATH, rom_path, sizeof(rom_path));
    if (st == TEST_ROM_EXPLICIT_MISSING_FAIL) { return 1; }
    if (st == TEST_ROM_ABSENT_SKIP) { return MJK_TEST_SKIP_CODE; }

    static MjkAssets assets;
    CHECK(MjkAssets_LoadFromRomFile(rom_path, &assets) == ASSET_OK, "Cargar MjkAssets");

    static MjkGame g;
    MjkGame_Init(&g, &assets);
    MjkWorld_LoadRoom(&g.world, &assets, 0x01u, 0u);

    /* 1. Evaluar las 33 condiciones (0..32) en estado base falso */
    for (uint8_t c = 0u; c < MJK_DOOR_COND_COUNT; ++c) {
        if (c == DOOR_COND_ROOM_61_SPECIAL) {
            g.door_flag_1f = 1u;
        }
        CHECK(!MjkDoors_EvaluateCondition(&g, c), "Condicion en estado base debe ser false");
    }

    /* 2. Probar caso verdadero de cada condicion activa REV_US */
    g.player.just_jumped = true;
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_JUMP_TOUCH), "Cond 0x02 (just_jumped)");
    g.player.just_jumped = false;

    g.mighty_level = 3u;
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_MIGHTY3_A), "Cond 0x07 (Mighty 3)");
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_MIGHTY3_B), "Cond 0x0E (Mighty 3)");
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_MIGHTY3_C), "Cond 0x17 (Mighty 3)");
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_MIGHTY3_D), "Cond 0x18 (Mighty 3)");
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_MIGHTY3_E), "Cond 0x1B (Mighty 3)");
    g.mighty_level = 0u;

    g.door_flag_09 = 1u;
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_FLAG_09), "Cond 0x09 dispara una vez");
    CHECK(!MjkDoors_EvaluateCondition(&g, DOOR_COND_FLAG_09), "Cond 0x09 no repite");

    g.objects.bomb_order = BOMB_ORDER_COMPLETED;
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_BOMB_ORDER), "Cond 0x0C (orden de bombas)");
    CHECK(!MjkDoors_EvaluateCondition(&g, DOOR_COND_BOMB_ORDER), "Cond 0x0C consumida");

    g.world.camera.room_flags = ROOM_FLAG_AT_SCROLL_EDGE;
    g.mighty_coins = 0u; g.scoring.timer_seconds = 0; g.player.sprite_id = 6u;
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_ZERO_TIMER_A), "Cond 0x19 (timer 0 + sprite 6)");
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_ZERO_TIMER_B), "Cond 0x1D (timer 0 + sprite 6)");

    g.door_1e_deaths = 2u; g.player.is_dying = false;
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_TWO_FIRE_DEATHS), "Cond 0x1E (2 muertes fuego)");
    CHECK(!MjkDoors_EvaluateCondition(&g, DOOR_COND_TWO_FIRE_DEATHS), "Cond 0x1E reseteada");

    g.door_flag_1f = 0u;
    CHECK(MjkDoors_EvaluateCondition(&g, DOOR_COND_ROOM_61_SPECIAL), "Cond 0x1F (Sala $61)");

    /* 3. Caso especial Sala $61 */
    MjkWorld_LoadRoom(&g.world, &assets, 0x61u, 0u);
    CHECK(MjkDoors_GetRoomCondition(&g) == DOOR_COND_ROOM_61_SPECIAL, "Sala $61 devuelve cond 0x1F");

    /* 4. Apertura de puerta, persistencia y cruce de Jack */
    MjkWorld_LoadRoom(&g.world, &assets, 0x01u, 0u);
    int door_idx = -1;
    for (uint8_t i = 0u; i < g.world.current.object_count; ++i) {
        if (g.world.current.objects[i].kind == OBJ_KIND_DOOR) {
            door_idx = (int)i;
            break;
        }
    }
    CHECK(door_idx >= 0, "Sala 1 contiene al menos una puerta");
    if (door_idx >= 0) {
        MjkRoomObj* d = &g.world.current.objects[door_idx];
        d->is_open = false; d->is_hidden = false;
        g.player.x = (float)(d->grid_x * 16 + 8);
        g.player.y = (float)(d->grid_y * 16 + 8);
        CHECK(MjkDoors_CheckPlayerEntry(&g) == ACTION_CONTINUE, "Puerta cerrada no permite cruce");

        MjkDoors_OpenInRoom(&g, &g.world.current, (uint8_t)door_idx);
        CHECK(d->is_open && !d->is_hidden, "Puerta abierta correctamente");
        CHECK(MjkDoors_CheckPlayerEntry(&g) == ACTION_ROOM_CHANGED && g.state == GS_EXIT_ROOM,
              "Puerta abierta dispara ACTION_ROOM_CHANGED");
    }

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_doors_new fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_doors_new superado (33 condiciones + Sala $61 + cruce).\n");
    return 0;
}
