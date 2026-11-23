/**
 * @file test_game_flow.c
 * @brief Tests de regresión C11 (Capa 1 / CTest) para puertas, Bomb Rooms, scroll vertical y flow.
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

static void warp_to_main(MjkGame* g, const MjkAssetPackage* pkg, uint8_t room_id) {
    MjkGame_Init(g, pkg);
    MjkGame_WarpToRoom(g, room_id);
    MjkInputState in;
    memset(&in, 0, sizeof(in));
    for (int f = 0; f < 300 && g->state != MJK_GS_MAIN; ++f) {
        MjkGame_StepFrame(g, &in);
    }
}

static void test_room06_exit_to_bomb_room_90(const MjkAssetPackage* pkg) {
    MjkGame g;
    warp_to_main(&g, pkg, 0x06u);
    if (g.state != MJK_GS_MAIN) {
        fprintf(stderr, "[FAIL] test_room06: no llego a MJK_GS_MAIN (state=%d)\n", g.state);
        g_failures++;
        return;
    }

    /* Posicionar a Jack frente a puerta $8A (0x2F/0x3F) en pared derecha */
    g.player.pos_x = MjkUQ8_FromInt(230u);
    g.player.pos_y = MjkUQ8_FromInt(0x28u);
    g.player.facing_attr = 0x04u;
    g.ppu.scroll_x = 16u;

    MjkInputState in;
    memset(&in, 0, sizeof(in));
    in.held = MJK_BTN_RIGHT;
    MjkGame_StepFrame(&g, &in);
    if (g.state != MJK_GS_EXIT_ROOM) {
        fprintf(stderr, "[FAIL] test_room06: no entro en puerta $8A (state=%d)\n", g.state);
        g_failures++;
        return;
    }
    in.held = 0u;
    MjkGame_StepFrame(&g, &in); /* EXIT_ROOM -> LOAD_ROOM */
    if (g.state != MJK_GS_LOAD_ROOM || g.current_room_id != 0x90u || g.entry_door_type != 0x0Eu) {
        fprintf(stderr, "[FAIL] test_room06: state=%d room=0x%02X door=0x%02X\n",
                g.state, g.current_room_id, g.entry_door_type);
        g_failures++;
        return;
    }
    MjkGame_StepFrame(&g, &in); /* LOAD_ROOM -> DRAW_SCREEN */
    MjkGame_StepFrame(&g, &in); /* DRAW_SCREEN -> ENTER_ROOM (scroll_x=0) */
    if (g.ppu.scroll_x != 0u) {
        fprintf(stderr, "[FAIL] test_room06: ppu.scroll_x no reseteo (%u)\n", g.ppu.scroll_x);
        g_failures++;
    }
}

static void test_bomb_room_90_complete_to_round2(const MjkAssetPackage* pkg) {
    MjkGame g;
    warp_to_main(&g, pkg, 0x90u);

    uint8_t bomb_count = 0u;
    for (uint8_t i = 0u; i < g.room.object_count; ++i) {
        if ((g.room.objects[i].ram_state & 0x0Fu) == 0x05u) {
            bomb_count++;
        }
    }
    if (bomb_count != 24u) {
        fprintf(stderr, "[FAIL] test_bomb_room_90: bombas decodificadas=%u (esperado 24)\n", bomb_count);
        g_failures++;
        return;
    }

    MjkInputState in;
    memset(&in, 0, sizeof(in));
    for (uint8_t i = 0u; i < g.room.object_count; ++i) {
        if ((g.room.objects[i].ram_state & 0x0Fu) == 0x05u) {
            uint8_t yx = g.room.objects[i].yx;
            g.player.pos_x = MjkUQ8_FromInt((uint8_t)(((yx & 0x0Fu) << 4u) + 8u));
            g.player.pos_y = MjkUQ8_FromInt((uint8_t)((yx & 0xF0u) + 8u));
            MjkObjects_Update(&g.objects, &g, &in);
        }
    }

    if (g.state != MJK_GS_ANIMATE_DOOR_OPEN) {
        fprintf(stderr, "[FAIL] test_bomb_room_90: no disparo ANIMATE_DOOR_OPEN (state=%d)\n", g.state);
        g_failures++;
        return;
    }

    const MjkRoomObject* exit_door = NULL;
    for (uint8_t i = 0u; i < g.room.object_count; ++i) {
        if ((g.room.objects[i].opcode & 0xF0u) == 0x80u &&
            (g.room.objects[i].opcode & 0x0Fu) == 0x0Au) {
            exit_door = &g.room.objects[i];
            break;
        }
    }
    if (!exit_door || (exit_door->ram_state & 0x40u) == 0u ||
        g.room.metatiles[exit_door->yx >> 4u][exit_door->yx & 0x0Fu] != 0x8Du ||
        g.room.metatiles[exit_door->yx2 >> 4u][exit_door->yx2 & 0x0Fu] != 0x8Eu) {
        fprintf(stderr, "[FAIL] test_bomb_room_90: puerta salida $8A no abierta/pintada\n");
        g_failures++;
        return;
    }

    for (int f = 0; f < 80 && g.state != MJK_GS_MAIN; ++f) {
        MjkGame_StepFrame(&g, &in);
    }
    g.player.pos_x = MjkUQ8_FromInt((uint8_t)(((exit_door->yx & 0x0Fu) << 4u) + 8u));
    g.player.pos_y = MjkUQ8_FromInt((uint8_t)((exit_door->yx & 0xF0u) + 8u));
    MjkDoors_CheckPlayerEntry(&g);
    MjkGame_StepFrame(&g, &in); /* EXIT_ROOM -> ROUND_CLEAR */
    if (g.state != MJK_GS_ROUND_CLEAR) {
        fprintf(stderr, "[FAIL] test_bomb_room_90: no entro a ROUND_CLEAR (state=%d)\n", g.state);
        g_failures++;
        return;
    }

    for (int f = 0; f < 1000 && g.state == MJK_GS_ROUND_CLEAR; ++f) {
        MjkGame_StepFrame(&g, &in);
    }
    if (g.current_room_id != 0x07u || g.entry_door_type != 0x0Eu) {
        fprintf(stderr, "[FAIL] test_bomb_room_90: destino=0x%02X door=0x%02X (esperado 0x07/0x0E)\n",
                g.current_room_id, g.entry_door_type);
        g_failures++;
        return;
    }

    MjkGame_StepFrame(&g, &in); /* LOAD_ROOM 0x07 */
    if ((g.camera.room_flags & MJK_RF_SINGLE_SCREEN) != 0u || g.camera.neighbor_room_id != 0x08u) {
        fprintf(stderr, "[FAIL] test_bomb_room_90: Ronda 2 camara flags=0x%02X neighbor=0x%02X\n",
                g.camera.room_flags, g.camera.neighbor_room_id);
        g_failures++;
    }
}

static void test_right_room_door_uses_right_room_adjacency(const MjkAssetPackage* pkg) {
    MjkGame g;
    warp_to_main(&g, pkg, 0x05u);
    /* En juego real se entra por sala $01 (cabecera de seccion $01..$06) */
    MjkRoom_PreloadSection(pkg, 0x01u, &g.persist, 0u);
    MjkRoom_DecodeObjects(pkg, 0x06u, &g.persist, &g.right_room);
    if (g.camera.neighbor_room_id != 0x06u) {
        fprintf(stderr, "[FAIL] test_right_room_door: neighbor esperado 0x06 obtuvo 0x%02X\n",
                g.camera.neighbor_room_id);
        g_failures++;
        return;
    }

    g.camera.scroll_x_fixed = 0xF000; /* scroll_x = 240 */
    g.player.pos_x = MjkUQ8_FromInt(246u); /* wx = 486 -> +10 = 496 (right_room col 15) */
    g.player.pos_y = MjkUQ8_FromInt(0x28u);
    g.player.facing_attr = 0x04u;

    MjkActionResult res = MjkDoors_CheckPlayerEntry(&g);
    if (res != MJK_ACTION_ROOM_CHANGED || g.exit_source_room_id != 0x06u) {
        fprintf(stderr, "[FAIL] test_right_room_door: res=%d src=0x%02X\n",
                res, g.exit_source_room_id);
        g_failures++;
        return;
    }
    MjkGame_ExitRoom(&g, (uint8_t)(g.entry_door_type >> 2u));
    if (g.current_room_id != 0x90u) {
        fprintf(stderr, "[FAIL] test_right_room_door: destino=0x%02X (esperado 0x90)\n",
                g.current_room_id);
        g_failures++;
    }
}

static void test_asymmetric_door_match_and_open_door_preservation(const MjkAssetPackage* pkg) {
    MjkGame g;
    MjkGame_Init(&g, pkg);
    g.current_room_id = 0x91u;
    g.exit_source_room_id = 0x91u;
    g.entry_door_type = 0x09u;
    MjkGame_ExitRoom(&g, (uint8_t)(g.entry_door_type >> 2u));

    if (g.current_room_id != 0x0Cu || g.entry_door_type != 0x0Fu) {
        fprintf(stderr, "[FAIL] test_asymmetric: room=0x%02X door=0x%02X\n",
                g.current_room_id, g.entry_door_type);
        g_failures++;
        return;
    }

    /* Entrar a sala 0x06 por puerta 0x0A (nace abierta 0x52) y verificar que ENTER_ROOM no la cierra */
    MjkGame_Init(&g, pkg);
    MjkRoom_PreloadSection(pkg, 0x01u, &g.persist, 0u);
    g.current_room_id = 0x90u;
    g.exit_source_room_id = 0x90u;
    g.entry_door_type = 0x0Eu;
    MjkGame_ExitRoom(&g, (uint8_t)(g.entry_door_type >> 2u)); /* -> 0x06, door 0x0A, already_open=true */
    if (g.current_room_id != 0x06u || g.entry_door_type != 0x0Au || !g.entry_door_already_open) {
        fprintf(stderr, "[FAIL] test_asymmetric: 0x90->0x06 room=0x%02X door=0x%02X open=%d\n",
                g.current_room_id, g.entry_door_type, g.entry_door_already_open ? 1 : 0);
        g_failures++;
        return;
    }

    MjkInputState in;
    memset(&in, 0, sizeof(in));
    g.top_mode = MJK_MODE_INGAME;
    g.state = MJK_GS_LOAD_ROOM;
    g.sub_state = 0u;
    for (int f = 0; f < 100 && g.state != MJK_GS_MAIN; ++f) {
        MjkGame_StepFrame(&g, &in);
    }
    if (g.state != MJK_GS_MAIN) {
        fprintf(stderr, "[FAIL] test_asymmetric: no llego a MAIN (state=%d)\n", g.state);
        g_failures++;
        return;
    }
    const MjkRoomObject* entry_door = NULL;
    for (uint8_t i = 0u; i < g.room.object_count; ++i) {
        if ((g.room.objects[i].opcode & 0xF0u) == 0x80u &&
            (g.room.objects[i].opcode & 0x0Fu) == 0x0Au) {
            entry_door = &g.room.objects[i];
            break;
        }
    }
    if (!entry_door ||
        g.room.metatiles[entry_door->yx >> 4u][entry_door->yx & 0x0Fu] != 0x8Du) {
        fprintf(stderr, "[FAIL] test_asymmetric: ENTER_ROOM repinto puerta cerrada sobre puerta abierta\n");
        g_failures++;
    }
}

static void test_death_blocks_door_and_resets_scroll(const MjkAssetPackage* pkg) {
    MjkGame g;
    warp_to_main(&g, pkg, 0x06u);

    g.player.is_dying = true;
    g.player.death_timer = 10u;
    g.player.pos_x = MjkUQ8_FromInt(230u);
    g.player.pos_y = MjkUQ8_FromInt(0x28u);
    g.player.facing_attr = 0x04u;
    g.ppu.scroll_x = 42u;
    g.ppu.scroll_y = 12u;

    MjkInputState in;
    memset(&in, 0, sizeof(in));
    MjkGame_StepFrame(&g, &in);
    if (g.state != MJK_GS_MAIN) {
        fprintf(stderr, "[FAIL] test_death_blocks_door: entro por puerta estando muriendo (state=%d)\n",
                g.state);
        g_failures++;
        return;
    }

    g.player.death_timer = 95u;
    MjkGame_StepFrame(&g, &in); /* -> LoseLife -> ROUND_INTRO */
    MjkGame_StepFrame(&g, &in); /* ROUND_INTRO sub_state 0 -> reset scroll */
    if (g.ppu.scroll_x != 0u || g.ppu.scroll_y != 0u) {
        fprintf(stderr, "[FAIL] test_death_blocks_door: scroll no reseteo (%u,%u)\n",
                g.ppu.scroll_x, g.ppu.scroll_y);
        g_failures++;
    }
}

int main(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando test_game_flow.\n");
        return 0;
    }

    test_room06_exit_to_bomb_room_90(&pkg);
    test_bomb_room_90_complete_to_round2(&pkg);
    test_right_room_door_uses_right_room_adjacency(&pkg);
    test_asymmetric_door_match_and_open_door_preservation(&pkg);
    test_death_blocks_door_and_resets_scroll(&pkg);

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_game_flow: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_game_flow: 5/5 regresiones PASS.\n");
    return 0;
}
