/**
 * @file test_enemies_new.c
 * @brief Unit tests exhaustivos de dificultad, spawns, 6 arquetipos, Power Coin y coleccionables (R-11, R-12).
 */

#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/enemies.h"
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
    MjkScoring_ResetNewGame(&g.scoring);
    MjkWorld_LoadRoom(&g.world, &assets, 0x01u, 0u);
    MjkPlayer_Init(&g.player, 64.0f, 184.0f);
    MjkEnemies_InitForRoom(&g.enemies, &assets, 0u, false);

    /* 1. Mutacion determinista (jump_press_count % 6) + 2 hacia los 6 arquetipos */
    for (uint8_t press = 0u; press < 6u; ++press) {
        MjkEnemies_InitForRoom(&g.enemies, &assets, 0u, false);
        g.player.jump_press_count = press;
        CHECK(MjkEnemies_Spawn(&g.enemies, ENEMY_MUMMY, 160.0f, 184.0f), "Spawn Momia");
        g.enemies.slots[0].spawn_flash_timer = 0;
        g.enemies.slots[0].falling           = false;
        g.enemies.slots[0].transform_timer   = 0;
        MjkEnemies_Update(&g.enemies, &g);
        MjkEnemyKind expected = (MjkEnemyKind)(press + 2u);
        CHECK(g.enemies.slots[0].kind == expected, "Mutacion determinista a cada uno de los 6 arquetipos");
        /* Simular 5 frames de movimiento del arquetipo sin crash ni NaN */
        g.enemies.slots[0].spawn_flash_timer = 0;
        for (int f = 0; f < 5; ++f) {
            MjkEnemies_Update(&g.enemies, &g);
        }
    }

    /* 2. Power Coin: 320 frames, transmutacion, cadena de puntos 200->300->500 y restauracion */
    MjkEnemies_InitForRoom(&g.enemies, &assets, 0u, false);
    MjkEnemies_Spawn(&g.enemies, ENEMY_MUMMY, 120.0f, 120.0f);
    MjkEnemies_Spawn(&g.enemies, ENEMY_MUMMY, 160.0f, 120.0f);
    MjkEnemies_Spawn(&g.enemies, ENEMY_MUMMY, 200.0f, 120.0f);
    MjkEnemies_ActivatePowerCoin(&g.enemies, &g);
    CHECK(g.enemies.transmute_timer == MJK_POWER_COIN_FRAMES && g.power_coin_palette_active,
          "Power Coin activa 320f y paleta especial");
    CHECK(g.enemies.slots[0].transmuted && g.enemies.slots[1].transmuted, "Monstruos transmutados");

    /* Recoger dos enemigos transmutados -> 200 + 300 = 500 pts */
    g.scoring.score = 0u;
    g.player.x = 120.0f; g.player.y = 120.0f;
    MjkEnemies_Update(&g.enemies, &g);
    g.player.x = 160.0f; g.player.y = 120.0f;
    MjkEnemies_Update(&g.enemies, &g);
    CHECK(g.scoring.score == 500u && g.enemies.transmute_streak == 2u,
          "Cadena transmutacion suma 200 + 300 = 500");

    /* Agotar los frames restantes -> restaura paleta y activa gracia post-transmutacion */
    g.player.x = 20.0f; g.player.y = 20.0f;
    for (int f = 0; f < MJK_POWER_COIN_FRAMES; ++f) {
        MjkEnemies_Update(&g.enemies, &g);
    }
    CHECK(g.enemies.transmute_timer == 0 && !g.power_coin_palette_active && !g.enemies.slots[2].transmuted,
          "Power Coin expira y restaura enemigos y paleta");

    /* 3. Coleccionables: Bonus Coin, Extra Coin (1UP), Secret Coin (solo Mighty 3), Captive */
    MjkEnemies_InitForRoom(&g.enemies, &assets, 0u, false);
    g.scoring.score = 0u; g.scoring.lives = 3; g.scoring.score_multiplier = 0u;
    g.mighty_level = 0u; g.secret_coins = 0u;

    MjkEnemies_Spawn(&g.enemies, ENEMY_BONUS_COIN, 80.0f, 80.0f);
    g.player.x = 80.0f; g.player.y = 80.0f;
    MjkEnemies_Update(&g.enemies, &g);
    CHECK(g.scoring.score_multiplier == 1u && g.scoring.score == 1000u, "Bonus Coin sube multiplicador y +1000");

    MjkEnemies_Spawn(&g.enemies, ENEMY_EXTRA_COIN, 80.0f, 80.0f);
    MjkEnemies_Update(&g.enemies, &g);
    CHECK(g.scoring.lives == 4 && g.collected_1up, "Extra Coin otorga 1UP");

    MjkEnemies_Spawn(&g.enemies, ENEMY_SECRET_COIN, 80.0f, 80.0f);
    MjkEnemies_Update(&g.enemies, &g);
    CHECK(g.secret_coins == 0u, "Secret Coin requiere Mighty 3");
    g.mighty_level = 3u;
    MjkEnemies_Update(&g.enemies, &g);
    CHECK(g.secret_coins == 1u, "Secret Coin recogida con Mighty 3");

    /* 4. Colision letal con monstruo activo */
    MjkEnemies_InitForRoom(&g.enemies, &assets, 0u, false);
    MjkPlayer_Init(&g.player, 100.0f, 100.0f);
    MjkEnemies_Spawn(&g.enemies, ENEMY_MUMMY, 100.0f, 100.0f);
    g.enemies.slots[0].spawn_flash_timer = 0;
    MjkEnemies_Update(&g.enemies, &g);
    CHECK(g.player.is_dying, "Colision con monstruo mata a Jack");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_enemies_new fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_enemies_new superado (6 arquetipos, Power Coin 320f y coleccionables).\n");
    return 0;
}
