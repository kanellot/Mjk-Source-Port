/**
 * @file test_renderer.c
 * @brief Unit tests del renderizador solo-lectura optimizado + Golden Frame Checks (R-14).
 */

#include <stdio.h>
#include <string.h>
#include "golden_reference.h"
#include "mjk/assets.h"
#include "mjk/game.h"
#include "mjk/input.h"
#include "mjk/renderer.h"
#include "mjk/rom_loader.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

static uint32_t fnv1a_32(const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint32_t h = 2166136261u;
    for (size_t i = 0u; i < len; ++i) {
        h ^= (uint32_t)p[i];
        h *= 16777619u;
    }
    return h;
}

static void step_n(MjkGame* g, MjkInputLatch* latch, uint8_t btn, int n) {
    for (int i = 0; i < n; ++i) {
        MjkInputLatch_Poll(latch, btn);
        MjkInputStep step;
        MjkInputLatch_ConsumeStep(latch, &step);
        MjkGame_StepFrame(g, &step);
    }
}

int main(void) {
    char rom_path[512];
    MjkTestRomStatus st = MjkRom_ResolveTestPath(MJK_SOURCE_DIR, MJK_TEST_ROM_PATH, rom_path, sizeof(rom_path));
    if (st == TEST_ROM_EXPLICIT_MISSING_FAIL) { return 1; }
    if (st == TEST_ROM_ABSENT_SKIP) { return MJK_TEST_SKIP_CODE; }

    static MjkAssets assets;
    CHECK(MjkAssets_LoadFromRomFile(rom_path, &assets) == ASSET_OK, "Cargar MjkAssets");

    static MjkGame g;
    static uint32_t fb[MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT];
    MjkInputLatch latch;
    MjkInputLatch_Init(&latch);
    MjkGame_Init(&g, &assets);

    /* 1. Golden Frame Check: Copyright screen (frame 10) */
    step_n(&g, &latch, 0u, 10);
    MjkRenderer_DrawFrame(&g, &assets, fb);
    uint32_t h_copy = fnv1a_32(fb, sizeof(fb));
    CHECK(h_copy == MJK_GOLDEN_FRAME_COPYRIGHT, "Golden Frame Check: Copyright exacto");

    /* 2. Golden Frame Check: Title screen wait */
    step_n(&g, &latch, MJK_INPUT_START, 3);
    step_n(&g, &latch, 0u, 12);
    MjkRenderer_DrawFrame(&g, &assets, fb);
    uint32_t h_title = fnv1a_32(fb, sizeof(fb));
    CHECK(h_title == MJK_GOLDEN_FRAME_TITLE, "Golden Frame Check: Title Screen exacto");

    /* 3. Sala 1 Playing: framebuffer valido y distinto de Title */
    step_n(&g, &latch, MJK_INPUT_START, 3);
    step_n(&g, &latch, 0u, 370);
    MjkRenderer_DrawFrame(&g, &assets, fb);
    uint32_t h_room1 = fnv1a_32(fb, sizeof(fb));
    CHECK(h_room1 != h_title && h_room1 != 0u, "Sala 1 renderiza frame valido");

    /* 4. Cambio de color de capa de Jack con Mighty 0 -> 3 cambia el framebuffer */
    g.mighty_level = 3u;
    MjkRenderer_DrawFrame(&g, &assets, fb);
    uint32_t h_mighty3 = fnv1a_32(fb, sizeof(fb));
    CHECK(h_mighty3 != h_room1, "Mighty nivel 3 altera paleta de capa de Jack");

    /* 5. Scroll horizontal cambia el framebuffer */
    g.world.camera.scroll_x = 40.0f;
    MjkRenderer_DrawFrame(&g, &assets, fb);
    uint32_t h_scroll = fnv1a_32(fb, sizeof(fb));
    CHECK(h_scroll != h_mighty3, "Scroll horizontal desplaza fondo correctamente");

    /* 6. Bomb Room $90 y Torture Room $AC renderizan sin fallos */
    MjkWorld_LoadRoom(&g.world, &assets, 0x90u, 0u);
    MjkObjects_InitForRoom(&g.objects, &g);
    MjkRenderer_DrawFrame(&g, &assets, fb);
    uint32_t h_bomb = fnv1a_32(fb, sizeof(fb));
    CHECK(h_bomb != h_room1 && h_bomb != 0u, "Bomb Room $90 renderiza correctamente");

    MjkWorld_LoadRoom(&g.world, &assets, 0xACu, 0u);
    g.in_torture_mode = true;
    g.greedy_jumps_left = 50;
    MjkRenderer_DrawFrame(&g, &assets, fb);
    uint32_t h_tort = fnv1a_32(fb, sizeof(fb));
    CHECK(h_tort != h_bomb && h_tort != 0u, "Torture Room $AC renderiza correctamente");

    /* Verificar que todos los pixeles tienen alpha opaco 0xFF */
    bool all_opaque = true;
    for (int i = 0; i < MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT; ++i) {
        if ((fb[i] & 0xFF000000u) != 0xFF000000u) {
            all_opaque = false;
            break;
        }
    }
    CHECK(all_opaque, "Todos los pixeles del framebuffer ARGB8888 son opacos");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_renderer fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_renderer superado (Golden Frame Checks + Mighty + Scroll + Bomb/Torture).\n");
    return 0;
}
