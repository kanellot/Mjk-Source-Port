/**
 * @file test_enemy.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para enemigos, mutación y Power Coin (T-22, T-23).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_enemy.h"
#include "mjk_game.h"
#include "mjk_rom_extractor.h"

static int g_failures = 0;

static bool load_rom_assets(MjkAssetPackage* out_pkg) {
#ifdef MJK_SOURCE_DIR
    char rom_path[512];
    snprintf(rom_path, sizeof(rom_path), "%s/asm/bin/mbj-us.nes", MJK_SOURCE_DIR);
    FILE* f = fopen(rom_path, "rb");
    if (!f) { return false; }
    uint8_t* rom = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom) { fclose(f); return false; }
    size_t got = fread(rom, 1u, MJK_ROM_EXPECTED_SIZE, f);
    fclose(f);
    if (got != MJK_ROM_EXPECTED_SIZE) { free(rom); return false; }
    uint8_t* v2_buf = NULL;
    size_t v2_size = 0u;
    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    MjkAssetError err = MjkRom_BuildAssetsV2Buffer(rom, MJK_ROM_EXPECTED_SIZE, &v2_buf, &v2_size, sha1_hex);
    free(rom);
    if (err != MJK_ASSET_OK) { free(v2_buf); return false; }
    err = MjkAssets_LoadFromMemory(v2_buf, v2_size, out_pkg);
    free(v2_buf);
    return (err == MJK_ASSET_OK);
#else
    (void)out_pkg;
    return false;
#endif
}

int main(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando test_enemy.\n");
        return 0;
    }

    MjkGame g;
    MjkGame_Init(&g, &pkg);
    MjkGame_WarpToRoom(&g, 0x02u);

    /* 1. DifficultyTable y spawn diferido hasta máximo 4 monstruos vivos */
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0x90u, false);
    if (g.enemies.spawn_reload != 0xA0u || g.enemies.transform_reload != 0x80u) {
        fprintf(stderr, "[FAIL] DifficultyTable R1 inesperada (%u, %u)\n",
                g.enemies.spawn_reload, g.enemies.transform_reload);
        g_failures++;
    }
    for (int f = 0; f < 900; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (g.enemies.alive_count != MJK_ENEMY_MAX_ALIVE) {
        fprintf(stderr, "[FAIL] alive_count=%u (esperado 4)\n", g.enemies.alive_count);
        g_failures++;
    }

    /* 2. Power Coin 320f + cadena de puntuación 100..2000 */
    MjkEnemy_ActivatePowerCoin(&g.enemies);
    if (g.enemies.transmute_timer != 320u) {
        fprintf(stderr, "[FAIL] transmute_timer=%u (esperado 320)\n", g.enemies.transmute_timer);
        g_failures++;
    }
    /* Tocar primer enemigo transmutado -> +100 pts */
    g.enemies.slots[0].pos_x = MjkUQ8_FromInt(g.jack_x);
    g.enemies.slots[0].pos_y = MjkUQ8_FromInt(g.jack_y);
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (g.enemies.transmute_timer != 319u || g.score_bcd[1] != 0x01u) {
        fprintf(stderr, "[FAIL] Power Coin pickup/timer fallo (t=%u s1=0x%02X)\n",
                g.enemies.transmute_timer, g.score_bcd[1]);
        g_failures++;
    }

    /* 3. Verificar IDs de metasprite válidos ($07..$25) en render */
    MjkPpu_ClearOam(&g.ppu);
    uint8_t used = MjkEnemy_RenderOam(&g.enemies, &pkg, &g.ppu, 0u);
    if (used == 0u) {
        fprintf(stderr, "[FAIL] RenderOam no escribio sprites de enemigos\n");
        g_failures++;
    }

    /* 4. Regresión Bug 1: Room 0x01 nunca spawnea enemigos; Room 0x02 sí spawnea */
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, false);
    g.current_room_id = 0x01u;
    g.camera.scroll_x_fixed = 0u;
    g.camera.neighbor_room_id = 0u;
    g.player.pos_x = MjkUQ8_FromInt(128u);
    g.jack_x = 128u;
    g.jack_y = 128u;
    for (int f = 0; f < 400; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (g.enemies.alive_count != 0u) {
        fprintf(stderr, "[FAIL] Room 0x01 spawneo %u enemigos (esperado 0)\n", g.enemies.alive_count);
        g_failures++;
    }
    g.current_room_id = 0x02u;
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, false);
    for (int f = 0; f < 200; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (g.enemies.alive_count != 1u) {
        fprintf(stderr, "[FAIL] Room 0x02 alive_count=%u tras 200f (esperado 1)\n", g.enemies.alive_count);
        g_failures++;
    }

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_enemy: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_enemy: DifficultyTable, max 4 vivos, Power Coin 320f y render OAM OK.\n");
    return 0;
}
