/**
 * @file test_hud.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para BCD LE, StageTimer 256f y HUD OAM (T-24).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_game.h"
#include "mjk_hud.h"
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
        printf("[INFO] ROM local no encontrada; saltando test_hud.\n");
        return 0;
    }

    MjkGame g;
    MjkGame_Init(&g, &pkg);
    g.room.is_bomb_room = true;
    g.score_multiplier = 1u; /* x2 */

    /* 1. AddScore BCD LE + MultipliedScoreTable + Bonus Coin cada 5000 pts */
    bool bonus = false, extra = false;
    MjkHud_AddScore(&g, MJK_SCORE_100, &bonus, &extra); /* 100 * 2 = 200 */
    if (g.score_bcd[1] != 0x02u || bonus || extra) {
        fprintf(stderr, "[FAIL] AddScore x2 fallo (s1=0x%02X)\n", g.score_bcd[1]);
        g_failures++;
    }
    MjkHud_AddScore(&g, MJK_SCORE_10000, &bonus, &extra); /* > 5000 -> spawn Bonus Coin */
    if (!bonus || extra) {
        fprintf(stderr, "[FAIL] No disparo Bonus Coin al cruzar 5000 pts\n");
        g_failures++;
    }

    /* 2. StageTimer divisor 256f */
    g.stage_timer_bcd = 0x01u;
    g.timer_frames = 256u;
    for (int f = 0; f < 255; ++f) {
        if (MjkHud_TickStageTimer(&g)) {
            fprintf(stderr, "[FAIL] StageTimer expiro antes de 256f\n");
            g_failures++;
        }
    }
    if (!MjkHud_TickStageTimer(&g) || g.stage_timer_bcd != 0x00u) {
        fprintf(stderr, "[FAIL] StageTimer no expiro en el frame 256\n");
        g_failures++;
    }

    /* 3. Puntuación flotante canónica tiles 0x30..0x3F (RES-034) */
    MjkHud_SpawnFloatingScore(g.popups, MJK_FLOAT_SCORE_SLOTS, 80u, 100u, MJK_SCORE_200, 2u, true);
    if (!g.popups[0].active || g.popups[0].tl_tile != 0x32u ||
        g.popups[0].tr_tile != 0x30u || g.popups[0].br_tile != 0x3Cu) {
        fprintf(stderr, "[FAIL] FloatingScore tiles incorrectos (0x%02X,0x%02X,0x%02X)\n",
                g.popups[0].tl_tile, g.popups[0].tr_tile, g.popups[0].br_tile);
        g_failures++;
    }

    MjkPpu_ClearOam(&g.ppu);
    uint8_t next = MjkHud_RenderOam(&g, 0u);
    if (next < 17u) {
        fprintf(stderr, "[FAIL] MjkHud_RenderOam escribio pocos sprites (%u)\n", next);
        g_failures++;
    }

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_hud: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_hud: BCD LE, Bonus Coin 5000 pts, StageTimer 256f y FloatingScore 0x30..0x3F OK.\n");
    return 0;
}
