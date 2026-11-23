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
    MjkInputState in;
    memset(&in, 0, sizeof(in));
    for (int f = 0; f < 30; ++f) {
        MjkInput_Update(&in, 0u);
        MjkGame_StepFrame(&g, &in);
    }

    /* 1. DifficultyTable y spawn diferido hasta máximo 4 monstruos vivos */
    for (uint8_t my = 0u; my < MJK_ROOM_MT_ROWS; ++my) {
        g.room.static_metatiles[my][0] = 0x01u;
        g.room.static_metatiles[my][MJK_ROOM_MT_COLS - 1u] = 0x01u;
    }
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

    /* 5. Regresión Problema 2a: velocidad R1=0x0058, Momia cae con gravedad, camina ±0x0080 y rebota en paredes */
    uint16_t r1_spd = 0u;
    MjkAssets_ReadCpuU16(&pkg, MJK_ADDR_ENEMY_SPEED_TABLE, &r1_spd);
    const uint8_t* g_div = MjkAssets_ResolveCpuPtr(&pkg, MJK_ADDR_GAMEIDO_DIV_TABLE, 8u);
    if (r1_spd != 0x0058u || !g_div || g_div[0] != 0x4Cu) {
        fprintf(stderr, "[FAIL] Tablas velocidad/Gameido R1 inesperadas (spd=0x%04X div=0x%02X)\n",
                r1_spd, g_div ? g_div[0] : 0u);
        g_failures++;
    }
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, false);
    memset(&g.room, 0, sizeof(g.room));
    g.room.room_id = 0x02u;
    for (uint8_t mx = 0u; mx < MJK_ROOM_MT_COLS; ++mx) {
        g.room.static_metatiles[10][mx] = 0x01u; /* Suelo sólido en fila 10 (y=160) */
    }
    g.room.static_metatiles[9][10] = 0x01u; /* Pared sólida en col 10 (x=160) */
    g.jack_x = 200u;
    g.jack_y = 152u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_MUMMY, 120u, 80u);
    MjkEnemy* m = &g.enemies.slots[0];
    m->spawn_flash_timer = 0u;
    if (!m->falling || m->vel_x != 0) {
        fprintf(stderr, "[FAIL] Momia no inicia cayendo con vel_x=0\n");
        g_failures++;
    }
    for (int f = 0; f < 60 && m->falling; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (m->falling || m->vel_x != 0x0080) {
        fprintf(stderr, "[FAIL] Momia no aterrizo o no camino a +0x0080 (falling=%d vx=0x%04X)\n",
                m->falling, (uint16_t)m->vel_x);
        g_failures++;
    }
    for (int f = 0; f < 100 && m->vel_x > 0; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (m->vel_x >= 0 || MjkUQ8_Int(m->pos_x) >= 160u) {
        fprintf(stderr, "[FAIL] Momia atraveso pared o no reboto (x=%u vx=%d)\n",
                MjkUQ8_Int(m->pos_x), m->vel_x);
        g_failures++;
    }

    /* 6. Regresión Problema 2b: arquetipos 2..7, Gameido div, despawn por scroll y hitbox < 8px */
    for (uint8_t ap = 0u; ap < 6u; ++ap) {
        MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, false);
        memset(&g.room, 0, sizeof(g.room));
        g.room.room_id = 0x02u;
        g.room.static_metatiles[10][8] = 0x01u;
        g.jack_x = 40u;
        g.jack_y = 40u;
        g.player.a_press_counter = ap;
        MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_MUMMY, 128u, 152u);
        g.enemies.slots[0].spawn_flash_timer = 0u;
        g.enemies.slots[0].falling = false;
        g.enemies.slots[0].transform_timer = 0u;
        MjkEnemy_UpdateAll(&g.enemies, &g);
        MjkEnemy* e = &g.enemies.slots[0];
        uint8_t expected_type = (uint8_t)(ap + 2u);
        if (e->type != expected_type) {
            fprintf(stderr, "[FAIL] Mutacion ap=%u dio tipo=%u (esperado %u)\n",
                    ap, e->type, expected_type);
            g_failures++;
        }
        if (expected_type == MJK_ENEMY_GAMEIDO) {
            int16_t exp_vx = -(int16_t)(((128u - 40u) << 8u) / 0x4Cu);
            if (e->vel_x != exp_vx) {
                fprintf(stderr, "[FAIL] Gameido vel_x=%d (esperado %d)\n", e->vel_x, exp_vx);
                g_failures++;
            }
        } else {
            int16_t avx = (e->vel_x < 0) ? (int16_t)(-e->vel_x) : e->vel_x;
            int16_t avy = (e->vel_y < 0) ? (int16_t)(-e->vel_y) : e->vel_y;
            if (avx != 0x0058 && avy != 0x0058) {
                fprintf(stderr, "[FAIL] Arquetipo %u no usa speed R1 0x0058 (vx=%d vy=%d)\n",
                        expected_type, e->vel_x, e->vel_y);
                g_failures++;
            }
        }
    }
    /* Despawn por scroll offscreen y hitbox < 8 px */
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, false);
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_MUMMY, 10u, 100u);
    g.camera.delta_cam_x = -20;
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (g.enemies.slots[0].active) {
        fprintf(stderr, "[FAIL] Enemigo no despawneo al salir por scroll (x=%u)\n",
                MjkUQ8_Int(g.enemies.slots[0].pos_x));
        g_failures++;
    }
    g.camera.delta_cam_x = 0;
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, false);
    g.player.is_dying = false;
    g.jack_x = 128u;
    g.jack_y = 128u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_MUMMY, 137u, 128u); /* dx = 9 -> no mata */
    g.enemies.slots[0].spawn_flash_timer = 0u;
    g.enemies.slots[0].falling = false;
    g.enemies.slots[0].vel_x = 0;
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (g.player.is_dying) {
        fprintf(stderr, "[FAIL] Hitbox mato a Jack con dx=9 (>= 8)\n");
        g_failures++;
    }
    g.enemies.slots[0].pos_x = MjkUQ8_FromInt(135u); /* dx = 7 -> sí mata */
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (!g.player.is_dying) {
        fprintf(stderr, "[FAIL] Hitbox no mato a Jack con dx=7 (< 8)\n");
        g_failures++;
    }

    /* 7. Regresión Problema 2c: Bomb Room muta al aterrizar en Y >= 0xD8 */
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, true);
    memset(&g.room, 0, sizeof(g.room));
    g.room.room_id = 0x10u;
    g.room.is_bomb_room = true;
    for (uint8_t mx = 0u; mx < MJK_ROOM_MT_COLS; ++mx) {
        g.room.static_metatiles[14][mx] = 0x01u; /* Fondo en y=224 (0xE0) -> centro aterrizaje 0xD8 */
    }
    g.player.is_dying = false;
    g.jack_x = 200u;
    g.jack_y = 40u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_MUMMY, 100u, 190u);
    g.enemies.slots[0].spawn_flash_timer = 0u;
    for (int f = 0; f < 40 && g.enemies.slots[0].type == MJK_ENEMY_MUMMY; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (g.enemies.slots[0].type < 2u || g.enemies.slots[0].type > 7u) {
        fprintf(stderr, "[FAIL] Bomb Room Momia no muto al aterrizar en Y>=0xD8 (type=%u y=0x%02X)\n",
                g.enemies.slots[0].type, MjkUQ8_Int(g.enemies.slots[0].pos_y));
        g_failures++;
    }

    /* 8. Regresión Bug 3: Enemigos no transmutados matan a Jack incluso en Mighty 3 */
    MjkEnemy_InitForRoom(&g.enemies, &pkg, 0u, false);
    g.mighty_level = 3u;
    g.enemies.transmute_timer = 0u;
    g.enemies.post_transmute_grace = 0u;
    g.player.is_dying = false;
    g.jack_x = 128u;
    g.jack_y = 128u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_MUMMY, 128u, 128u);
    g.enemies.slots[0].spawn_flash_timer = 0u;
    g.enemies.slots[0].falling = false;
    g.enemies.slots[0].vel_x = 0;
    g.enemies.slots[0].vel_y = 0;
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (!g.player.is_dying || !g.enemies.slots[0].active) {
        fprintf(stderr, "[FAIL] Bug 3: enemigo no transmutado no mato a Jack en Mighty 3 (dying=%d active=%d)\n",
                g.player.is_dying ? 1 : 0, g.enemies.slots[0].active ? 1 : 0);
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
