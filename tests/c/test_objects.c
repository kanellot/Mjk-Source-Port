/**
 * @file test_objects.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para bombas, cofres, ítems y Mighty (T-19, T-20).
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

int main(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando test_objects.\n");
        return 0;
    }

    MjkGame g;
    MjkGame_Init(&g, &pkg);
    MjkGame_WarpToRoom(&g, 0x01u);
    /* Avanzar LoadRoom -> DrawScreen -> EnterRoom (169f) -> Main */
    MjkInputState in;
    memset(&in, 0, sizeof(in));
    for (int f = 0; f < 175; ++f) {
        MjkInput_Update(&in, 0u);
        MjkGame_StepFrame(&g, &in);
    }

    /* 1. Recoger 1ª bomba de Sala 1 ($85) -> puff 32f (fase 0) + popup 32f (fase 1) en slot único */
    g.player.pos_x = MjkUQ8_FromInt(0x54u);
    g.player.pos_y = MjkUQ8_FromInt(0x84u);
    MjkObjects_Update(&g.objects, &g, &in);
    if (!g.objects.puff.active || g.objects.puff.phase != 0u || g.objects.puff.timer != 32u) {
        fprintf(stderr, "[FAIL] Puff fase 0 no inicio tras recoger bomba\n");
        g_failures++;
    }
    for (int f = 0; f < 32; ++f) {
        MjkObjects_Update(&g.objects, &g, &in);
    }
    if (!g.objects.puff.active || g.objects.puff.phase != 1u || g.objects.puff.timer != 32u ||
        g.popups[0].active) {
        fprintf(stderr, "[FAIL] Puff no paso a fase 1 (popup) en slot unico tras 32f\n");
        g_failures++;
    }
    for (int f = 0; f < 32; ++f) {
        MjkObjects_Update(&g.objects, &g, &in);
    }
    if (g.objects.puff.active) {
        fprintf(stderr, "[FAIL] Puff fase 1 no termino tras 32f\n");
        g_failures++;
    }

    /* 2. Botón B Mighty 1..3 y degradación por A */
    g.mighty_coins = 3u;
    bool transmute = false;
    if (!MjkObjects_UseMightyCoin(&g, &transmute) || g.mighty_level != 1u || transmute) {
        fprintf(stderr, "[FAIL] Mighty nivel 1 fallo\n");
        g_failures++;
    }
    MjkObjects_UseMightyCoin(&g, &transmute);
    MjkObjects_UseMightyCoin(&g, &transmute);
    if (g.mighty_level != 3u || !transmute || g.mighty_coins != 0u) {
        fprintf(stderr, "[FAIL] Mighty nivel 3 / transmute fallo\n");
        g_failures++;
    }

    /* 3. 10 Mighty Coins -> TortureRoom */
    g.room.objects[0].yx = 0x44u;
    g.room.objects[0].ram_state = MJK_ITEM_MIGHTY_COIN;
    g.mighty_coins = 9u;
    g.player.pos_x = MjkUQ8_FromInt(0x44u);
    g.player.pos_y = MjkUQ8_FromInt(0x44u);
    MjkActionResult res = MjkObjects_Update(&g.objects, &g, &in);
    if (res != MJK_ACTION_TORTURE_TRIGGERED || g.state != MJK_GS_TORTURE_ROOM) {
        fprintf(stderr, "[FAIL] 10 Mighty Coins no disparo TortureRoom\n");
        g_failures++;
    }

    /* 4. Regresión Bug 3: bomba en right_room (wx >= 256) */
    g.state = MJK_GS_MAIN;
    g.current_room_id = 0x02u;
    g.camera.neighbor_room_id = 0x03u;
    g.camera.scroll_x_fixed = (mjk_uq8_8_t)(160u << 8);
    memset(&g.right_room, 0, sizeof(g.right_room));
    g.right_room.room_id = 0x03u;
    g.right_room.object_count = 1u;
    g.right_room.objects[0].yx = 0x42u; /* y=0x40..0x4F, x=0x20..0x2F (wx=288 -> 0x20) */
    g.right_room.objects[0].ram_state = 0x05u;
    g.right_room.objects[0].ram_index = 0u;
    g.player.pos_x = MjkUQ8_FromInt(128u);
    g.player.pos_y = MjkUQ8_FromInt(0x44u);
    MjkInput_Update(&in, 0u);
    MjkObjects_Update(&g.objects, &g, &in);
    if (g.right_room.objects[0].ram_state != 0x0Fu || g.right_room.metatiles[4][2] != 0x2Fu) {
        fprintf(stderr, "[FAIL] Bomba en right_room no recogida (st=0x%02X mt=0x%02X)\n",
                g.right_room.objects[0].ram_state, g.right_room.metatiles[4][2]);
        g_failures++;
    }

    /* 5. Regresión Problema 1: saltar encima de un cofre rojo en Sala 1 lo abre en el frame del salto */
    MjkGame_WarpToRoom(&g, 0x01u);
    for (int f = 0; f < 175; ++f) {
        MjkInput_Update(&in, 0u);
        MjkGame_StepFrame(&g, &in);
    }
    uint8_t chest_idx = g.room.object_count;
    if (chest_idx < MJK_ROOM_MAX_OBJECTS) {
        g.room.object_count++;
        g.room.objects[chest_idx].yx = 0xA4u;
        g.room.objects[chest_idx].ram_state = 0x10u; /* Cofre rojo visible */
        g.room.objects[chest_idx].opcode = 0x00u;    /* Item 0 (100 bag -> metatile != 0x28) */
        g.room.objects[chest_idx].ram_index = chest_idx;
        g.room.static_metatiles[0x0A][0x04] = 0x00u;
        MjkObjects_SetMetatile(&g, 0xA4u, 0x28u);
        g.player.pos_x = MjkUQ8_FromInt(0x48u);
        g.player.pos_y = MjkUQ8_FromInt(0x98u);
        g.player.vel_x = 0;
        g.player.vel_y = 0;
        g.player.in_air = false;
        g.player.just_jumped = false;

        /* 1 frame de pie -> latch */
        MjkInput_Update(&in, 0u);
        MjkGame_StepFrame(&g, &in);
        if (g.objects.chest_target_yx != 0xA4u) {
            fprintf(stderr, "[FAIL] Cofre bajo los pies no latcheado (target=0x%02X)\n",
                    g.objects.chest_target_yx);
            g_failures++;
        }

        /* Pulsar A -> debe abrirse en el mismo frame del salto */
        MjkInput_Update(&in, MJK_BTN_A);
        MjkGame_StepFrame(&g, &in);
        uint8_t nt_tile = g.ppu.nt[0][0x0Au * 2u][0x04u * 2u];
        if (g.room.objects[chest_idx].ram_state != 0x00u ||
            g.room.metatiles[0x0A][0x04] == 0x28u ||
            nt_tile == 0u) {
            fprintf(stderr, "[FAIL] Cofre no abrio al saltar (st=0x%02X mt=0x%02X)\n",
                    g.room.objects[chest_idx].ram_state, g.room.metatiles[0x0A][0x04]);
            g_failures++;
        }
    }

    /* 6. Regresión Royal Room: chispa de mecha usa tile $0D/$0F y no se mueve al coger bomba apagada */
    memset(&g.room, 0, sizeof(g.room));
    g.room.room_id = 0x10u;
    g.room.is_bomb_room = true;
    g.room.object_count = 3u;
    g.room.objects[0].yx = 0x42u; g.room.objects[0].ram_state = 0x05u; g.room.objects[0].ram_index = 0u;
    g.room.objects[1].yx = 0x44u; g.room.objects[1].ram_state = 0x05u; g.room.objects[1].ram_index = 1u;
    g.room.objects[2].yx = 0x46u; g.room.objects[2].ram_state = 0x05u; g.room.objects[2].ram_index = 2u;
    g.camera.neighbor_room_id = 0u;
    g.camera.scroll_x_fixed = 0u;
    g.camera.scroll_y_fixed = 0u;
    g.score_multiplier = 0u;
    MjkObjects_InitForRoom(&g.objects, &g);

    /* Recoger 1.ª bomba -> debe encender la 2.ª con tile_id == 0x0D */
    g.player.pos_x = MjkUQ8_FromInt(0x28u);
    g.player.pos_y = MjkUQ8_FromInt(0x48u);
    MjkInput_Update(&in, 0u);
    MjkObjects_Update(&g.objects, &g, &in);
    if (!g.objects.spark.active || g.objects.spark.yx != 0x44u || g.objects.spark.tile_id != 0x0Du) {
        fprintf(stderr, "[FAIL] Chispa Royal Room mal inicializada (active=%d yx=0x%02X tile=0x%02X)\n",
                g.objects.spark.active, g.objects.spark.yx, g.objects.spark.tile_id);
        g_failures++;
    }
    /* Tras 3 ticks, la chispa alterna 0x0D -> 0x0F (nunca dígitos 0x00/0x02) */
    g.player.pos_x = MjkUQ8_FromInt(0x88u);
    for (int f = 0; f < 3; ++f) {
        MjkObjects_Update(&g.objects, &g, &in);
    }
    if (g.objects.spark.tile_id != 0x0Fu) {
        fprintf(stderr, "[FAIL] Chispa no alterno a 0x0F tras 3f (tile=0x%02X)\n", g.objects.spark.tile_id);
        g_failures++;
    }
    /* Recoger la 3.ª bomba (apagada) NO debe mover la chispa de la 2.ª (0x44) y debe sobrescribir el puff anterior */
    g.player.pos_x = MjkUQ8_FromInt(0x68u);
    MjkObjects_Update(&g.objects, &g, &in);
    if (!g.objects.spark.active || g.objects.spark.yx != 0x44u) {
        fprintf(stderr, "[FAIL] Recoger bomba apagada movio la chispa (active=%d yx=0x%02X)\n",
                g.objects.spark.active, g.objects.spark.yx);
        g_failures++;
    }
    if (!g.objects.puff.active || g.objects.puff.phase != 0u || g.objects.puff.timer != 32u ||
        MjkUQ8_Int(g.objects.puff.x) != 0x68u || g.popups[0].active) {
        fprintf(stderr, "[FAIL] Recoger 2.a bomba no sobrescribio el puff sin crear popup anterior\n");
        g_failures++;
    }

    /* 7. Regresión cofre secreto (0x30) dentro de terreno sólido: al abrirlo se limpia static_metatiles */
    memset(&g.room, 0, sizeof(g.room));
    g.room.room_id = 0x01u;
    g.room.is_horizontal = true;
    g.room.object_count = 1u;
    g.room.objects[0].yx = 0x84u;
    g.room.objects[0].ram_state = 0x30u; /* Cofre secreto oculto */
    g.room.objects[0].opcode = 0x90u;    /* Item 0 (bolsa 100) */
    g.room.objects[0].ram_index = 0u;
    g.room.static_metatiles[8][4] = 0x01u; /* Terreno sólido debajo del cofre secreto */
    g.camera.neighbor_room_id = 0u;
    g.camera.scroll_x_fixed = 0u;
    g.camera.scroll_y_fixed = 0u;
    MjkObjects_InitForRoom(&g.objects, &g);
    g.player.pos_x = MjkUQ8_FromInt(0x48u);
    g.player.pos_y = MjkUQ8_FromInt(0x78u);
    g.player.in_air = false;
    g.player.just_jumped = false;
    /* 1.º salto: revela cofre (0x30 -> 0x10) */
    MjkInput_Update(&in, 0u);
    MjkObjects_Update(&g.objects, &g, &in);
    g.player.just_jumped = true;
    MjkObjects_Update(&g.objects, &g, &in);
    /* 2.º salto: abre cofre (0x10 -> 0x00) y debe limpiar static_metatiles[8][4] */
    g.player.in_air = false;
    g.player.just_jumped = false;
    MjkObjects_Update(&g.objects, &g, &in);
    g.player.just_jumped = true;
    MjkObjects_Update(&g.objects, &g, &in);
    if (g.room.objects[0].ram_state != 0x00u || g.room.static_metatiles[8][4] != 0x2Fu) {
        fprintf(stderr, "[FAIL] Cofre secreto abierto conservo colision solida (st=0x%02X static=0x%02X)\n",
                g.room.objects[0].ram_state, g.room.static_metatiles[8][4]);
        g_failures++;
    }

    /* 8. Regresión O-01: bloque secreto (0x33) se rompe saltando encima (feet_yx + just_jumped) */
    memset(&g.room, 0, sizeof(g.room));
    g.room.room_id = 0x01u;
    g.room.is_horizontal = true;
    g.room.object_count = 1u;
    g.room.objects[0].yx = 0x84u;
    g.room.objects[0].ram_state = 0x33u;
    g.room.objects[0].opcode = 0x62u; /* 2 saltos */
    g.room.objects[0].ram_index = 0u;
    g.room.static_metatiles[8][4] = 0x01u;
    MjkObjects_InitForRoom(&g.objects, &g);
    g.player.pos_x = MjkUQ8_FromInt(0x48u);
    g.player.pos_y = MjkUQ8_FromInt(0x78u);
    g.player.in_air = false;
    g.player.just_jumped = false;
    for (int j = 0; j < 2; ++j) {
        MjkObjects_Update(&g.objects, &g, &in); /* latch */
        g.player.just_jumped = true;
        MjkObjects_Update(&g.objects, &g, &in); /* jump hit */
        g.player.just_jumped = false;
    }
    if (g.room.objects[0].ram_state != 0x0Fu || g.room.static_metatiles[8][4] != 0x2Fu) {
        fprintf(stderr, "[FAIL] O-01: bloque secreto 0x33 no rompio tras 2 saltos (st=0x%02X static=0x%02X)\n",
                g.room.objects[0].ram_state, g.room.static_metatiles[8][4]);
        g_failures++;
    }

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_objects: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_objects: bombas, puff 32+32f, Mighty B 1..3 y Torture trigger OK.\n");
    return 0;
}
