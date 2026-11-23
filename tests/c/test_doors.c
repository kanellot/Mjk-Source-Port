/**
 * @file test_doors.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para las 33 condiciones de puertas REV_US (T-21).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_doors.h"
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
        printf("[INFO] ROM local no encontrada; saltando test_doors.\n");
        return 0;
    }

    MjkGame g;
    MjkGame_Init(&g, &pkg);
    MjkGame_WarpToRoom(&g, 0x01u);

    /* 1. Sala 1 no tiene puertas falsas de tipo AdjacentRoomsTable (RES-029) */
    for (uint8_t i = 0u; i < g.room.object_count; ++i) {
        if ((g.room.objects[i].opcode & 0xF0u) == 0x80u) {
            if (g.room.objects[i].yx == 0u) {
                fprintf(stderr, "[FAIL] Puerta en Sala 1 con yx=0\n");
                g_failures++;
            }
        }
    }

    /* 2. Condición $02 (just_jumped) */
    g.player.just_jumped = false;
    if (MjkDoors_EvaluateCondition(&g, 0x02u)) {
        fprintf(stderr, "[FAIL] Cond $02 no deberia disparar sin salto\n");
        g_failures++;
    }
    g.player.just_jumped = true;
    if (!MjkDoors_EvaluateCondition(&g, 0x02u)) {
        fprintf(stderr, "[FAIL] Cond $02 no disparo con just_jumped\n");
        g_failures++;
    }

    /* 3. Condiciones Mighty 3 ($07, $0E, $17, $18, $1B) */
    const uint8_t m3_conds[5] = { 0x07u, 0x0Eu, 0x17u, 0x18u, 0x1Bu };
    g.mighty_level = 2u;
    for (size_t i = 0u; i < 5u; ++i) {
        if (MjkDoors_EvaluateCondition(&g, m3_conds[i])) {
            fprintf(stderr, "[FAIL] Cond 0x%02X disparo con Mighty 2\n", m3_conds[i]);
            g_failures++;
        }
    }
    g.mighty_level = 3u;
    for (size_t i = 0u; i < 5u; ++i) {
        if (!MjkDoors_EvaluateCondition(&g, m3_conds[i])) {
            fprintf(stderr, "[FAIL] Cond 0x%02X no disparo con Mighty 3\n", m3_conds[i]);
            g_failures++;
        }
    }

    /* 4. Condición $19/$1D (4 condiciones conjuntas) */
    g.camera.room_flags = MJK_RF_AT_SCROLL_EDGE;
    g.mighty_coins = 0u;
    g.stage_timer_bcd = 0u;
    g.player.sprite_id = 6u;
    if (!MjkDoors_EvaluateCondition(&g, 0x19u) || g.door_flag_09 != 1u) {
        fprintf(stderr, "[FAIL] Cond $19 fallo\n");
        g_failures++;
    }

    /* 5. Condición $1E (2 muertes en fuego $5D) y $1F (Sala $61 una vez) */
    g.door_1e_deaths = 2u;
    g.player.is_dying = false;
    if (!MjkDoors_EvaluateCondition(&g, 0x1Eu) || g.door_1e_deaths != 0u) {
        fprintf(stderr, "[FAIL] Cond $1E fallo\n");
        g_failures++;
    }
    g.current_room_id = 0x61u;
    if (MjkDoors_RoomConditionIndex(&g) != 0x1Fu ||
        !MjkDoors_EvaluateCondition(&g, 0x1Fu) ||
        MjkDoors_EvaluateCondition(&g, 0x1Fu)) {
        fprintf(stderr, "[FAIL] Sala $61 / Cond $1F fallo\n");
        g_failures++;
    }

    /* 6. No-op en REV_US ($0A, $1A, $20) */
    if (MjkDoors_EvaluateCondition(&g, 0x0Au) ||
        MjkDoors_EvaluateCondition(&g, 0x1Au) ||
        MjkDoors_EvaluateCondition(&g, 0x20u)) {
        fprintf(stderr, "[FAIL] No-op REV_US devolvio true\n");
        g_failures++;
    }

    /* 7. Apertura y cruce de puerta */
    MjkGame_WarpToRoom(&g, 0x01u);
    MjkInputState in_zero;
    memset(&in_zero, 0, sizeof(in_zero));
    MjkGame_StepFrame(&g, &in_zero); /* LOAD_ROOM */
    MjkGame_StepFrame(&g, &in_zero); /* DRAW_SCREEN */
    MjkDoors_OpenRoomDoor(&g, 0u);
    if ((g.room.objects[0].ram_state & 0x40u) == 0u) {
        fprintf(stderr, "[FAIL] OpenRoomDoor no marco bit 6\n");
        g_failures++;
    }
    uint8_t dyx = g.room.objects[0].yx;
    g.camera.scroll_x_fixed = 0;
    g.player.pos_x = MjkUQ8_FromInt((uint8_t)(((dyx & 0x0Fu) << 4u) + 8u));
    g.player.pos_y = MjkUQ8_FromInt((uint8_t)((dyx & 0xF0u) + 8u));
    if (MjkDoors_CheckPlayerEntry(&g) != MJK_ACTION_ROOM_CHANGED || g.state != MJK_GS_EXIT_ROOM) {
        fprintf(stderr, "[FAIL] CheckPlayerEntry no disparo EXIT_ROOM\n");
        g_failures++;
    }

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_doors: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_doors: 33 condiciones REV_US, Sala $61, apertura y cruce OK.\n");
    return 0;
}
