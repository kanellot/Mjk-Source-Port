/**
 * @file test_world.c
 * @brief Unit tests exhaustivos de mundo, camara en float, persistencia y colisiones (R-06).
 */

#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/rom_loader.h"
#include "mjk/world.h"

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

    static MjkWorld world;
    MjkWorld_Reset(&world);

    /* 1. Carga de Sala 1 (pasillo horizontal con sala vecina) */
    CHECK(MjkWorld_LoadRoom(&world, &assets, 0x01u, 0u), "Cargar Sala 1");
    CHECK(world.current.room_id == 0x01u && world.current.is_horizontal && !world.current.is_bomb_room,
          "Sala 1 es pasillo horizontal");
    CHECK(world.has_neighbor && world.camera.neighbor_room_id != 0u, "Sala 1 tiene sala vecina");

    /* 2. Scroll horizontal y cruce de costura derecha */
    float px = 128.0f, py = 180.0f;
    uint8_t first_room = world.camera.current_room_id;
    uint8_t expected_next = world.camera.neighbor_room_id;
    bool crossed = false;
    for (int f = 0; f < 220; ++f) {
        px += 1.5f;
        MjkWorld_UpdateCamera(&world, &assets, &px, &py, 1.5f, 0.0f, 0u);
        if (world.camera.room_crossed) {
            crossed = true;
            break;
        }
    }
    CHECK(crossed && world.camera.current_room_id == expected_next && world.current.room_id == expected_next,
          "Cruce de costura horizontal avanza a sala vecina");
    CHECK(world.camera.current_room_id != first_room, "ID de sala actualizado");

    /* 3. Persistencia de objetos dentro de seccion */
    CHECK(MjkWorld_LoadRoom(&world, &assets, 0x01u, 0u), "Recargar Sala 1");
    int bomb_idx = -1;
    for (uint8_t i = 0u; i < world.current.object_count; ++i) {
        if (world.current.objects[i].kind == OBJ_KIND_BOMB) {
            bomb_idx = (int)i;
            break;
        }
    }
    CHECK(bomb_idx >= 0, "Sala 1 tiene bombas");
    if (bomb_idx >= 0) {
        world.current.objects[bomb_idx].is_collected = true;
        MjkWorld_SaveObjectState(&world, &world.current, (uint8_t)bomb_idx);
        /* Simular re-entrada en una sala de la misma seccion (sin reiniciar seccion) */
        MjkPersistSlot* sl = MjkWorld_FindPersistSlot(&world, 0x01u);
        CHECK(sl != NULL && sl->objects[world.current.objects[bomb_idx].persist_index].is_collected,
              "Estado de bomba recogida persiste en slot de seccion");
    }

    /* 4. Bomb Room $90 es pantalla unica sin scroll */
    CHECK(MjkWorld_LoadRoom(&world, &assets, 0x90u, 0u), "Cargar Bomb Room $90");
    CHECK(world.current.is_bomb_room && (world.camera.room_flags & ROOM_FLAG_SINGLE_SCREEN) != 0u,
          "Bomb Room $90 es single-screen");
    px = 2.0f;
    MjkWorld_UpdateCamera(&world, &assets, &px, &py, -1.5f, 0.0f, 0u);
    CHECK(px == 8.0f, "Clamping izquierdo en pantalla unica (x=8)");

    /* 5. Colisiones semanticas: parciales 8x8, fuego 0x5C..0x62, cofres visibles vs ocultos */
    memset(&world, 0, sizeof(world));
    /* Metatile parcial 0x38: nibble alto 0xC = 1100 (fila superior solida, inferior libre) */
    world.current.static_metatiles[5][4] = 0x38u;
    CHECK(MjkWorld_IsPointSolid(&world, 4 * 16 + 2, 5 * 16 + 2, false, NULL), "Parcial 0x38 cuadrante TL solido");
    CHECK(!MjkWorld_IsPointSolid(&world, 4 * 16 + 2, 5 * 16 + 12, false, NULL), "Parcial 0x38 cuadrante BL libre");

    /* Fuego 0x5C..0x62: no es solido pero reporta fire_tile */
    for (uint8_t ft = 0x5Cu; ft <= 0x62u; ++ft) {
        world.current.static_metatiles[6][4] = ft;
        uint8_t detected = 0u;
        bool solid = MjkWorld_IsPointSolid(&world, 4 * 16 + 8, 6 * 16 + 8, false, &detected);
        CHECK(!solid && detected == ft, "Tile de fuego detectado sin bloquear paso");
    }

    /* Cofre oculto: libre lateralmente, solido desde arriba */
    world.current.object_count = 1u;
    world.current.objects[0].kind = OBJ_KIND_RED_CHEST;
    world.current.objects[0].grid_x = 7u;
    world.current.objects[0].grid_y = 7u;
    world.current.objects[0].is_hidden = true;
    CHECK(!MjkWorld_IsPointSolid(&world, 7 * 16 + 8, 7 * 16 + 8, false, NULL), "Cofre oculto libre lateral");
    CHECK(MjkWorld_IsPointSolid(&world, 7 * 16 + 8, 7 * 16 + 8, true, NULL), "Cofre oculto solido desde arriba");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_world fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_world superado.\n");
    return 0;
}
