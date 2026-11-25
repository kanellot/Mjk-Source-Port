/**
 * @file test_objects_new.c
 * @brief Unit tests exhaustivos de bombas, mechas, cofres, secretos, items y Mighty (R-08, R-09).
 */

#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/game.h"
#include "mjk/objects.h"
#include "mjk/rom_loader.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

static void load_room_for_test(MjkGame* g, const MjkAssets* assets, uint8_t room_id) {
    MjkGame_Init(g, assets);
    g->mode = SCREEN_INGAME;
    g->state = GS_PLAYING;
    MjkScoring_ResetNewGame(&g->scoring);
    g->current_room_id = room_id;
    MjkWorld_LoadRoom(&g->world, assets, room_id, 0u);
    MjkObjects_InitForRoom(&g->objects, g);
    MjkEnemies_InitForRoom(&g->enemies, assets, 0u, g->world.current.is_bomb_room);
}

int main(void) {
    char rom_path[512];
    MjkTestRomStatus st = MjkRom_ResolveTestPath(MJK_SOURCE_DIR, MJK_TEST_ROM_PATH, rom_path, sizeof(rom_path));
    if (st == TEST_ROM_EXPLICIT_MISSING_FAIL) { return 1; }
    if (st == TEST_ROM_ABSENT_SKIP) { return MJK_TEST_SKIP_CODE; }

    static MjkAssets assets;
    CHECK(MjkAssets_LoadFromRomFile(rom_path, &assets) == ASSET_OK, "Cargar MjkAssets");

    static MjkGame g;
    MjkInputStep empty = {0u, 0u, 0u};

    /* 1. Bomb Room $90: bomba encendida (200), avance de chispa, puff 2 fases (32f+32f) */
    load_room_for_test(&g, &assets, 0x90u);
    CHECK(g.objects.spark.active, "Bomb Room $90 arranca con chispa activa");
    g.player.x = (float)(g.objects.spark.grid_x * 16 + 8);
    g.player.y = (float)(g.objects.spark.grid_y * 16 + 8);
    MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(g.scoring.score == 200u && g.fire_bombs_collected == 1u, "Bomba encendida suma 200 y fire_bombs=1");
    CHECK(g.objects.puff.active && g.objects.puff.phase == 0u && g.objects.puff.timer == 32,
          "Puff fase 0 (32f) iniciada");
    for (int f = 0; f < 32; ++f) {
        g.player.x = 16.0f; g.player.y = 16.0f;
        MjkObjects_Update(&g.objects, &g, &empty);
    }
    CHECK(g.objects.puff.active && g.objects.puff.phase == 1u, "Puff pasa a fase 1 tras 32f");

    /* Recoger las 22 bombas restantes -> debe abrir puerta de salida */
    for (uint8_t i = 0u; i < g.world.current.object_count; ++i) {
        MjkRoomObj* o = &g.world.current.objects[i];
        if (o->kind == OBJ_KIND_BOMB && !o->is_collected) {
            g.player.x = (float)(o->grid_x * 16 + 8);
            g.player.y = (float)(o->grid_y * 16 + 8);
            MjkObjects_Update(&g.objects, &g, &empty);
        }
    }
    CHECK(g.state == GS_ANIMATE_DOOR_OPEN && !g.objects.spark.active,
          "Ultima bomba en Bomb Room abre puerta de salida");

    /* 2. Bonus de orden perfecto en pasillo (10.200 pts) y descalificacion */
    load_room_for_test(&g, &assets, 0x01u);
    g.world.current.object_count = 2u;
    memset(g.world.current.objects, 0, sizeof(g.world.current.objects));
    g.world.current.objects[0].kind = OBJ_KIND_BOMB;
    g.world.current.objects[0].grid_x = 3u; g.world.current.objects[0].grid_y = 5u;
    g.world.current.objects[0].is_order_marker = true;
    g.world.current.objects[1].kind = OBJ_KIND_BOMB;
    g.world.current.objects[1].grid_x = 5u; g.world.current.objects[1].grid_y = 5u;
    g.world.current.objects[1].is_order_marker = true;
    g.player.x = 3 * 16 + 8.0f; g.player.y = 5 * 16 + 8.0f;
    MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(g.objects.bomb_order == BOMB_ORDER_ACTIVE && g.world.current.objects[1].is_lit,
          "Primera bomba marcada activa cadena y enciende siguiente");
    g.player.x = 5 * 16 + 8.0f; g.player.y = 5 * 16 + 8.0f;
    MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(g.objects.bomb_order == BOMB_ORDER_COMPLETED && g.scoring.score == 10300u,
          "Ultima bomba marcada otorga 10.200 pts y completa orden");

    /* 3. Uso de Mighty Coin: niveles 1 -> 2 -> 3 (transmuta) y prohibido en Bomb Room */
    load_room_for_test(&g, &assets, 0x01u);
    g.mighty_coins = 3u;
    bool transmute = false;
    CHECK(MjkObjects_UseMightyCoin(&g, &transmute) && g.mighty_level == 1u && g.mighty_presses_left == 20u && !transmute,
          "Mighty nivel 1 (20 pulsaciones)");
    CHECK(MjkObjects_UseMightyCoin(&g, &transmute) && g.mighty_level == 2u && g.mighty_presses_left == 30u && !transmute,
          "Mighty nivel 2 (30 pulsaciones)");
    CHECK(MjkObjects_UseMightyCoin(&g, &transmute) && g.mighty_level == 3u && g.mighty_presses_left == 35u && transmute,
          "Mighty nivel 3 (35 pulsaciones + transmutacion)");
    g.world.current.is_bomb_room = true;
    g.mighty_level = 0u; g.mighty_coins = 1u;
    CHECK(!MjkObjects_UseMightyCoin(&g, &transmute), "Mighty prohibido en Bomb Room");

    /* 4. Cofres rojo/naranja, bloques secretos e items (Mighty Coin x10 -> Torture Room) */
    load_room_for_test(&g, &assets, 0x01u);
    g.world.current.object_count = 3u;
    memset(g.world.current.objects, 0, sizeof(g.world.current.objects));
    /* Cofre naranja: bloqueado en Mighty 0, abierto en Mighty 1 */
    g.world.current.objects[0].kind = OBJ_KIND_ORANGE_CHEST;
    g.world.current.objects[0].grid_x = 4u; g.world.current.objects[0].grid_y = 6u;
    g.world.current.objects[0].item_kind = ITEM_BAG_1000;
    /* Bloque secreto (1 golpe) */
    g.world.current.objects[1].kind = OBJ_KIND_SECRET_BLOCK;
    g.world.current.objects[1].grid_x = 6u; g.world.current.objects[1].grid_y = 6u;
    g.world.current.objects[1].raw_opcode = 0x51u;
    g.world.current.objects[1].is_hidden = true;
    /* Item Mighty Coin #10 -> Torture Room */
    g.world.current.objects[2].kind = OBJ_KIND_LOOSE_ITEM;
    g.world.current.objects[2].grid_x = 8u; g.world.current.objects[2].grid_y = 6u;
    g.world.current.objects[2].item_kind = ITEM_MIGHTY_COIN;

    /* Probar cofre naranja con Mighty 0 -> no se abre */
    g.mighty_level = 0u;
    g.player.x = 4 * 16 + 8.0f; g.player.y = 6 * 16 - 4.0f; g.player.in_air = false;
    MjkObjects_Update(&g.objects, &g, &empty);
    g.player.just_jumped = true;
    MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(!g.world.current.objects[0].is_revealed, "Cofre naranja bloqueado con Mighty 0");

    /* Con Mighty 1 -> se abre y revela bolsa 1000 */
    g.mighty_level = 1u; g.player.just_jumped = false;
    MjkObjects_Update(&g.objects, &g, &empty);
    g.player.just_jumped = true;
    MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(g.world.current.objects[0].is_revealed, "Cofre naranja abierto con Mighty 1");

    /* Recoger la bolsa 1000 */
    g.player.just_jumped = false;
    g.player.x = 4 * 16 + 8.0f; g.player.y = 6 * 16 + 8.0f;
    MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(g.scoring.score == 1000u && g.world.current.objects[0].is_collected, "Bolsa 1000 recogida");

    /* Bloque secreto: saltar encima lo rompe */
    g.player.x = 6 * 16 + 8.0f; g.player.y = 6 * 16 - 4.0f; g.player.in_air = false;
    MjkObjects_Update(&g.objects, &g, &empty);
    g.player.just_jumped = true;
    MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(g.world.current.objects[1].is_collected && g.last_sound == SOUND_SECRET_BLOCK_REVEALED,
          "Bloque secreto revelado al saltar encima");

    /* 10.ª Mighty Coin dispara Torture Room */
    g.mighty_coins = 9u; g.player.just_jumped = false;
    g.player.x = 8 * 16 + 8.0f; g.player.y = 6 * 16 + 8.0f;
    MjkActionResult tres = MjkObjects_Update(&g.objects, &g, &empty);
    CHECK(tres == ACTION_TORTURE_TRIGGERED && g.state == GS_TORTURE_ROOM,
          "10 Mighty Coins disparan Torture Room");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_objects_new fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_objects_new superado.\n");
    return 0;
}
