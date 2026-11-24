/**
 * @file test_items_mighty.c
 * @brief Tests de regresión C11 (Capa 1 / CTest) para los 6 bloques de ítems, cofres, Mighty 0..3 y Power Coin.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
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

static void init_room_main(MjkGame* g, const MjkAssetPackage* pkg, uint8_t room_id) {
    MjkGame_Init(g, pkg);
    MjkGame_WarpToRoom(g, room_id);
    MjkInputState in;
    memset(&in, 0, sizeof(in));
    for (int f = 0; f < 175; ++f) {
        MjkInput_Update(&in, 0u);
        MjkGame_StepFrame(g, &in);
    }
    g->sound_queue_count = 0u;
}

static bool has_sound(const MjkGame* g, MjkSoundId snd) {
    for (uint8_t i = 0u; i < g->sound_queue_count; ++i) {
        if (g->sound_queue[i] == snd) { return true; }
    }
    return false;
}

static void test_item_and_bomb_feedback(const MjkAssetPackage* pkg) {
    MjkGame g;
    init_room_main(&g, pkg, 0x01u);
    MjkInputState in;
    memset(&in, 0, sizeof(in));

    /* Test BG items SFX, GDV, and NO floating popups (Bug 2) */
    static const struct {
        uint8_t item;
        MjkSoundId sfx;
    } k_cases[] = {
        { MJK_ITEM_100_BAG,        MJK_SND_BOMB_LIT },
        { MJK_ITEM_300_BAG,        MJK_SND_BOMB_LIT },
        { MJK_ITEM_1000_BAG,       MJK_SND_BOMB_LIT },
        { MJK_ITEM_GOLD_COIN,      MJK_SND_COIN_COLLECTED },
        { MJK_ITEM_MIGHTY_DRINK,   MJK_SND_TIME_PICKUP },
        { MJK_ITEM_SPHINX,         MJK_SND_DOOR },
        { MJK_ITEM_TECMO_PLATE,    MJK_SND_1UP_CAPTIVE },
        { MJK_ITEM_MIGHTY_COIN,    MJK_SND_COIN_COLLECTED },
        { MJK_ITEM_CRYSTAL_BALL_1, MJK_SND_CRYSTAL_BALL },
        { MJK_ITEM_BEELZEBUT_MASK, MJK_MUS_COLLECTED_FULL_FAMILY }
    };

    for (size_t i = 0u; i < sizeof(k_cases) / sizeof(k_cases[0]); ++i) {
        memset(g.popups, 0, sizeof(g.popups));
        g.sound_queue_count = 0u;
        g.mighty_coins = 0u;
        g.stage_timer_bcd = 0x50u;
        g.persist.sphinx_flags[0] = 0x92u;
        g.room.objects[0].ram_state = k_cases[i].item;
        g.room.objects[0].yx = 0x55u;
        g.player.pos_x = MjkUQ8_FromInt(0x58u);
        g.player.pos_y = MjkUQ8_FromInt(0x58u);
        g.camera.scroll_x_fixed = 0u;
        g.camera.scroll_y_fixed = 0u;
        MjkObjects_Update(&g.objects, &g, &in);
        if (!has_sound(&g, k_cases[i].sfx)) {
            fprintf(stderr, "[FAIL] Item %u no encolo SFX 0x%02X\n", k_cases[i].item, k_cases[i].sfx);
            g_failures++;
        }
        for (size_t p = 0u; p < MJK_FLOAT_SCORE_SLOTS; ++p) {
            if (g.popups[p].active) {
                fprintf(stderr, "[FAIL] Item BG %u activo popup flotante indebido\n", k_cases[i].item);
                g_failures++;
                break;
            }
        }
    }
    if (g.gdv_bcd != 0x01u) {
        fprintf(stderr, "[FAIL] Crystal Ball no incremento GDV (0x%02X)\n", g.gdv_bcd);
        g_failures++;
    }

    /* Test collecting 2 bombs in < 32f overwrites puff slot (sub_A270/sub_A29C) and shows Bomb Room mult_tile */
    init_room_main(&g, pkg, 0x01u);
    memset(g.popups, 0, sizeof(g.popups));
    g.room.is_bomb_room = true;
    g.score_multiplier = 1u;
    g.room.objects[0].ram_state = 0x45u;
    g.room.objects[0].yx = 0x55u;
    g.room.objects[1].ram_state = 0x05u;
    g.room.objects[1].yx = 0x57u;
    g.player.pos_x = MjkUQ8_FromInt(0x58u);
    g.player.pos_y = MjkUQ8_FromInt(0x58u);
    MjkObjects_Update(&g.objects, &g, &in);
    g.player.pos_x = MjkUQ8_FromInt(0x78u);
    MjkObjects_Update(&g.objects, &g, &in);
    if (!g.objects.puff.active || g.objects.puff.phase != 0u ||
        MjkUQ8_Int(g.objects.puff.x) != 0x78u || g.popups[0].active) {
        fprintf(stderr, "[FAIL] Bomba 2 no sobrescribio el puff de Bomba 1 (active=%d phase=%u x=0x%02X pop=%d)\n",
                g.objects.puff.active ? 1 : 0, g.objects.puff.phase,
                MjkUQ8_Int(g.objects.puff.x), g.popups[0].active ? 1 : 0);
        g_failures++;
    }
    for (int f = 0; f < 32; ++f) {
        MjkObjects_Update(&g.objects, &g, &in);
    }
    if (!g.objects.puff.active || g.objects.puff.phase != 1u || g.objects.puff.mult_tile != 0x3Cu) {
        fprintf(stderr, "[FAIL] Bomba 2 no paso a fase popup x2 tras 32f (active=%d phase=%u mult=0x%02X)\n",
                g.objects.puff.active ? 1 : 0, g.objects.puff.phase, g.objects.puff.mult_tile);
        g_failures++;
    }
    for (uint8_t m = 1u; m <= 4u; ++m) {
        memset(g.popups, 0, sizeof(g.popups));
        MjkHud_SpawnFloatingScore(g.popups, MJK_FLOAT_SCORE_SLOTS, 80u, 80u, MJK_SCORE_100, m, true);
        if (g.popups[0].br_tile != (uint8_t)(0x3Bu + m)) {
            fprintf(stderr, "[FAIL] br_tile mult=%u esperado 0x%02X obtuvo 0x%02X\n",
                    m, (unsigned)(0x3Bu + m), g.popups[0].br_tile);
            g_failures++;
        }
    }
}

static void test_chest_lateral_open_mighty_levels(const MjkAssetPackage* pkg) {
    MjkGame g;
    init_room_main(&g, pkg, 0x01u);
    MjkInputState in;
    memset(&in, 0, sizeof(in));

    g.camera.scroll_x_fixed = 0u;
    g.camera.scroll_y_fixed = 0u;
    g.player.facing_attr = 0x04u;
    g.player.pos_x = MjkUQ8_FromInt(0x4Fu);
    g.player.pos_y = MjkUQ8_FromInt(0x58u);
    g.room.object_count = 2u;
    g.room.objects[0].yx = 0x55u;
    g.room.objects[0].ram_state = 0x10u; /* Rojo visible */
    g.room.objects[0].opcode = 0x00u;
    g.room.objects[1].yx = 0x57u;
    g.room.objects[1].ram_state = 0x11u; /* Naranja visible */
    g.room.objects[1].opcode = 0x01u;

    /* Level 0 & 1: no lateral open */
    g.mighty_level = 0u;
    MjkObjects_Update(&g.objects, &g, &in);
    g.mighty_level = 1u;
    MjkObjects_Update(&g.objects, &g, &in);
    if (g.room.objects[0].ram_state != 0x10u || g.room.objects[1].ram_state != 0x11u) {
        fprintf(stderr, "[FAIL] Cofres abrieron lateralmente en Mighty < 2\n");
        g_failures++;
    }

    /* Level 2: red & orange open laterally */
    g.mighty_level = 2u;
    MjkObjects_Update(&g.objects, &g, &in);
    if (g.room.objects[0].ram_state != 0x00u) {
        fprintf(stderr, "[FAIL] Cofre rojo no abrio lateralmente en Mighty 2 (st=0x%02X)\n",
                g.room.objects[0].ram_state);
        g_failures++;
    }
    g.player.pos_x = MjkUQ8_FromInt(0x6Fu);
    MjkObjects_Update(&g.objects, &g, &in);
    if (g.room.objects[1].ram_state != 0x01u) {
        fprintf(stderr, "[FAIL] Cofre naranja no abrio lateralmente en Mighty 2 (st=0x%02X)\n",
                g.room.objects[1].ram_state);
        g_failures++;
    }
}

static void test_jack_mighty_palette_upgrade_and_degrade(const MjkAssetPackage* pkg) {
    MjkGame g;
    init_room_main(&g, pkg, 0x01u);
    static const uint8_t k_exp[4] = { 0x16u, 0x12u, 0x26u, 0x2Au };

    if (g.ppu.palette_ram[0x17u] != k_exp[0]) {
        fprintf(stderr, "[FAIL] Palette inicial != 0x16 (0x%02X)\n", g.ppu.palette_ram[0x17u]);
        g_failures++;
    }
    g.mighty_coins = 3u;
    for (uint8_t lvl = 1u; lvl <= 3u; ++lvl) {
        bool trans = false;
        MjkObjects_UseMightyCoin(&g, &trans);
        if (g.mighty_level != lvl || g.ppu.palette_ram[0x17u] != k_exp[lvl]) {
            fprintf(stderr, "[FAIL] Upgrade nivel %u paleta 0x%02X != 0x%02X\n",
                    lvl, g.ppu.palette_ram[0x17u], k_exp[lvl]);
            g_failures++;
        }
    }

    /* Degrade 3 -> 2 -> 1 -> 0 by pressing A */
    MjkInputState in;
    memset(&in, 0, sizeof(in));
    uint8_t prev = g.mighty_level;
    for (int f = 0; f < 100 && g.mighty_level > 0u; ++f) {
        MjkInput_Update(&in, (f & 1) ? MJK_BTN_A : 0u);
        MjkGame_StepFrame(&g, &in);
        if (g.mighty_level != prev) {
            prev = g.mighty_level;
            if (g.ppu.palette_ram[0x17u] != k_exp[prev]) {
                fprintf(stderr, "[FAIL] Degradacion nivel %u paleta 0x%02X != 0x%02X\n",
                        prev, g.ppu.palette_ram[0x17u], k_exp[prev]);
                g_failures++;
            }
        }
    }
    if (g.mighty_level != 0u || g.ppu.palette_ram[0x17u] != 0x16u) {
        fprintf(stderr, "[FAIL] No degrado hasta nivel 0 / 0x16\n");
        g_failures++;
    }
}

static void test_power_coin_transmute_color_freeze_music(const MjkAssetPackage* pkg) {
    MjkGame g;
    init_room_main(&g, pkg, 0x01u);

    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_MUMMY, 0x90u, 0x80u);
    g.enemies.slots[0].spawn_flash_timer = 0u;
    g.enemies.slots[0].vel_x = MjkQ8_FromInt(1);
    g.enemies.slots[0].vel_y = MjkQ8_FromInt(1);
    mjk_uq8_8_t init_x = g.enemies.slots[0].pos_x;
    mjk_uq8_8_t init_y = g.enemies.slots[0].pos_y;

    MjkEnemy_ActivatePowerCoinEx(&g.enemies, &g);
    if (g.ppu.palette_ram[0x19u] != 0x08u ||
        g.ppu.palette_ram[0x1Au] != 0x26u ||
        g.ppu.palette_ram[0x1Bu] != 0x30u) {
        fprintf(stderr, "[FAIL] Power Coin no puso paleta dorada\n");
        g_failures++;
    }
    if (!has_sound(&g, MJK_SND_POWER_COIN_MUSIC)) {
        fprintf(stderr, "[FAIL] Power Coin no encolo MJK_SND_POWER_COIN_MUSIC\n");
        g_failures++;
    }

    g.camera.delta_cam_x = 0;
    g.camera.delta_cam_y = 0;
    g.jack_x = 0x20u;
    g.jack_y = 0x20u;
    for (int f = 0; f < 10; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (g.enemies.slots[0].pos_x != init_x || g.enemies.slots[0].pos_y != init_y) {
        fprintf(stderr, "[FAIL] Moneda transmutada se movio durante congelamiento\n");
        g_failures++;
    }

    /* Collect transmuted coin */
    g.sound_queue_count = 0u;
    g.jack_x = 0x90u;
    g.jack_y = 0x80u;
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (g.gold_coins != 1u || !has_sound(&g, MJK_SND_COIN_COLLECTED)) {
        fprintf(stderr, "[FAIL] Recoger moneda transmutada no dio gold_coin/SFX\n");
        g_failures++;
    }

    /* Expire remaining 309 frames -> restore palette & room music */
    g.sound_queue_count = 0u;
    for (int f = 0; f < 309; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (g.enemies.transmute_timer != 0u ||
        g.ppu.palette_ram[0x1Au] != 0x16u ||
        g.sound_queue_count == 0u) {
        fprintf(stderr, "[FAIL] Expirar Power Coin no restauro paleta/musica (timer=%u, pal=0x%02X)\n",
                g.enemies.transmute_timer, g.ppu.palette_ram[0x1Au]);
        g_failures++;
    }
}

static void test_power_coin_rises_above_chest(const MjkAssetPackage* pkg) {
    MjkGame g;
    init_room_main(&g, pkg, 0x01u);
    MjkInputState in;
    memset(&in, 0, sizeof(in));

    g.camera.scroll_x_fixed = 0u;
    g.camera.scroll_y_fixed = 0u;
    g.mighty_level = 2u;
    g.objects.power_coin_armed = 1u;
    g.room.object_count = 1u;
    g.room.objects[0].yx = 0x76u; /* x=0x68, y=0x78 */
    g.room.objects[0].ram_state = 0x10u;
    g.room.objects[0].opcode = 0x00u;
    g.player.pos_x = MjkUQ8_FromInt(0x68u);
    g.player.pos_y = MjkUQ8_FromInt(0x78u);
    MjkObjects_Update(&g.objects, &g, &in);

    g.jack_x = 0x20u;
    g.jack_y = 0x20u;
    for (int f = 0; f < 16; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (!g.enemies.slots[0].active ||
        g.enemies.slots[0].type != MJK_ENEMY_POWER_COIN ||
        MjkUQ8_Int(g.enemies.slots[0].pos_y) != 0x70u) {
        fprintf(stderr, "[FAIL] Power Coin no subio 8 px sobre el cofre (y=0x%02X)\n",
                MjkUQ8_Int(g.enemies.slots[0].pos_y));
        g_failures++;
    }
}

static void test_collectibles_secret_captive_balloon_bonus(const MjkAssetPackage* pkg) {
    MjkGame g;
    init_room_main(&g, pkg, 0x01u);
    g.jack_x = 0x80u;
    g.jack_y = 0x80u;

    /* SecretCoin blocked below Mighty 3, collected at Mighty 3 */
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_SECRET_COIN, 0x80u, 0x80u);
    g.mighty_level = 2u;
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (!g.enemies.slots[0].active || g.secret_coins != 0u) {
        fprintf(stderr, "[FAIL] SecretCoin recogida en Mighty < 3\n");
        g_failures++;
    }
    g.mighty_level = 3u;
    g.sound_queue_count = 0u;
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (g.enemies.slots[0].active || g.secret_coins != 1u || g.gdv_bcd != 0x01u ||
        !has_sound(&g, MJK_SND_COIN_COLLECTED)) {
        fprintf(stderr, "[FAIL] SecretCoin fallo en Mighty 3\n");
        g_failures++;
    }

    /* Captive */
    g.sound_queue_count = 0u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_CAPTIVE, 0x80u, 0x80u);
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (!has_sound(&g, MJK_SND_BONUS_STATUE) || g.score_bcd[1] != 0x20u) {
        fprintf(stderr, "[FAIL] Captive no dio +2000 / SFX\n");
        g_failures++;
    }

    /* Balloon floats up (12f immunity) and gives +10000 + MJK_SND_BOMB_LIT */
    g.jack_x = 0x20u;
    g.jack_y = 0x20u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_BALLOON, 0x80u, 0x80u);
    for (int f = 0; f < 12; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (MjkUQ8_Int(g.enemies.slots[0].pos_y) != 0x74u) {
        fprintf(stderr, "[FAIL] Balloon no floto hacia arriba (y=0x%02X)\n",
                MjkUQ8_Int(g.enemies.slots[0].pos_y));
        g_failures++;
    }
    g.sound_queue_count = 0u;
    g.jack_x = 0x80u;
    g.jack_y = 0x74u;
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (!has_sound(&g, MJK_SND_BOMB_LIT) || g.score_bcd[2] != 0x06u) {
        fprintf(stderr, "[FAIL] Balloon no dio +10000 / SFX (s2=0x%02X)\n", g.score_bcd[2]);
        g_failures++;
    }

    /* BonusCoin */
    g.sound_queue_count = 0u;
    g.score_multiplier = 0u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_BONUS_COIN, 0x80u, 0x74u);
    MjkEnemy_UpdateAll(&g.enemies, &g);
    if (g.score_multiplier != 1u || !has_sound(&g, MJK_SND_COIN_COLLECTED)) {
        fprintf(stderr, "[FAIL] BonusCoin no incremento multiplicador / SFX\n");
        g_failures++;
    }

    /* BonusCoin en Bomb Room cae y camina horizontalmente (ObjCode_9_E_ExtraBonusCoin) */
    memset(&g.room, 0, sizeof(g.room));
    g.room.is_bomb_room = true;
    for (uint8_t x = 0u; x < MJK_ROOM_MT_COLS; ++x) {
        g.room.static_metatiles[13][x] = 0x01u;
    }
    g.jack_x = 0xC0u;
    g.jack_y = 0x20u;
    MjkEnemy_SpawnCollectible(&g.enemies, MJK_ENEMY_BONUS_COIN, 0x80u, 0xA0u);
    for (int f = 0; f < 60; ++f) {
        MjkEnemy_UpdateAll(&g.enemies, &g);
    }
    if (!g.enemies.slots[0].active ||
        MjkUQ8_Int(g.enemies.slots[0].pos_y) <= 0xA0u ||
        MjkUQ8_Int(g.enemies.slots[0].pos_x) <= 0x80u) {
        fprintf(stderr, "[FAIL] BonusCoin en Royal Room no cayo y camino (act=%d x=0x%02X y=0x%02X)\n",
                g.enemies.slots[0].active ? 1 : 0,
                MjkUQ8_Int(g.enemies.slots[0].pos_x),
                MjkUQ8_Int(g.enemies.slots[0].pos_y));
        g_failures++;
    }
}

int main(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando test_items_mighty.\n");
        return 0;
    }

    test_item_and_bomb_feedback(&pkg);
    test_chest_lateral_open_mighty_levels(&pkg);
    test_jack_mighty_palette_upgrade_and_degrade(&pkg);
    test_power_coin_transmute_color_freeze_music(&pkg);
    test_power_coin_rises_above_chest(&pkg);
    test_collectibles_secret_captive_balloon_bonus(&pkg);

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_items_mighty: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_items_mighty: 6 tests de regresion PASS.\n");
    return 0;
}
