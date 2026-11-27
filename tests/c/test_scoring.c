/**
 * @file test_scoring.c
 * @brief Unit tests exhaustivos para puntuacion, GDV, StageTimer y popups (R-05).
 */

#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/rom_loader.h"
#include "mjk/scoring.h"

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
    if (st == TEST_ROM_EXPLICIT_MISSING_FAIL) {
        fprintf(stderr, "[FAIL] ROM explicita no encontrada: %s\n", rom_path);
        return 1;
    }
    if (st == TEST_ROM_ABSENT_SKIP) {
        return MJK_TEST_SKIP_CODE;
    }

    static MjkAssets assets;
    CHECK(MjkAssets_LoadFromRomFile(rom_path, &assets) == ASSET_OK, "Cargar MjkAssets");

    MjkScoring sc;
    memset(&sc, 0, sizeof(sc));
    MjkScoring_ResetNewGame(&sc);
    CHECK(sc.lives == 3 && sc.gdv == 47 && sc.timer_seconds == 60 && sc.score == 0u,
          "Estado inicial nueva partida (3 vidas, GDV 47, timer 60)");

    /* 1. Puntuacion basica + multiplicador x2 en Bomb Room */
    MjkScoreEvents ev;
    sc.score_multiplier = 1u; /* x2 */
    MjkScoring_AddPoints(&sc, &assets, SCORE_100, true, &ev);
    CHECK(sc.score == 200u && !ev.spawn_bonus_coin && !ev.spawn_extra_coin, "100 x2 = 200 en Bomb Room");

    /* 2. Cruzar 5.000 pts -> Bonus Coin */
    MjkScoring_AddPoints(&sc, &assets, SCORE_10000, true, &ev);
    CHECK(sc.score == 10200u && ev.spawn_bonus_coin && !ev.spawn_extra_coin,
          "Cruzar 5000 pts dispara Bonus Coin");

    /* 3. Con 40 Gold Coins -> Extra Coin y reinicio de gold_coins */
    sc.gold_coins = 40u;
    MjkScoring_AddPoints(&sc, &assets, SCORE_10000, true, &ev);
    CHECK(ev.spawn_extra_coin && !ev.spawn_bonus_coin && sc.gold_coins == 0u,
          "40 Gold Coins dispara Extra Coin y resetea gold_coins");

    /* 4. En mult == 4 (x5) NO dispara mas Bonus Coins */
    sc.score_multiplier = 4u;
    MjkScoring_AddPoints(&sc, &assets, SCORE_10000, true, &ev);
    CHECK(!ev.spawn_bonus_coin && !ev.spawn_extra_coin, "Mult x5 no dispara Bonus Coin");

    /* 5. Saturacion de score maximo */
    sc.score = MJK_MAX_SCORE - 50u;
    MjkScoring_AddPoints(&sc, &assets, SCORE_100, false, &ev);
    CHECK(sc.score == MJK_MAX_SCORE, "Score satura en 99.999.999");

    /* 6. GDV y saturacion en 99 + preservacion de high_gdv */
    CHECK(MjkScoring_AddGdv(&sc, 10) == 57, "GDV 47 + 10 = 57");
    CHECK(MjkScoring_AddGdv(&sc, 80) == 99 && sc.high_gdv == 99, "GDV satura en 99 y actualiza high_gdv");
    MjkScoring_ResetNewGame(&sc);
    CHECK(sc.gdv == 47 && sc.high_gdv == 99, "ResetNewGame preserva high_gdv");

    /* 7. StageTimer: divisor exacto de 256 frames + congelamiento */
    MjkScoring_ResetStageTimer(&sc, 1);
    sc.timer_frozen = true;
    for (int i = 0; i < 300; ++i) {
        CHECK(!MjkScoring_TickTimer(&sc), "Timer congelado no expira");
    }
    CHECK(sc.timer_seconds == 1, "Timer congelado mantiene valor");
    sc.timer_frozen = false;
    for (int i = 0; i < 255; ++i) {
        CHECK(!MjkScoring_TickTimer(&sc), "Timer no expira antes de 256f");
    }
    CHECK(!MjkScoring_TickTimer(&sc) && sc.timer_seconds == 0,
          "Frame 256 baja timer a 0 dejando ventana de 1 frame (prg.asm:1216)");
    CHECK(MjkScoring_TickTimer(&sc) && sc.timer_seconds == 0,
          "Frame 257 dispara Time Over");

    /* 8. Popups flotantes */
    MjkScoring_SpawnPopup(&sc, 80.0f, 100.0f, SCORE_200, 2u, true);
    CHECK(sc.popups[0].active && sc.popups[0].tl_tile == 0x32u &&
          sc.popups[0].tr_tile == 0x30u && sc.popups[0].br_tile == 0x3Du,
          "Popup flotante tiles canonicos 0x30..0x3F");
    MjkScoring_TickPopups(&sc, -2.0f, 0.0f);
    CHECK(sc.popups[0].x == 78.0f && sc.popups[0].timer == 31, "Popup sigue delta de camara");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_scoring fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_scoring superado.\n");
    return 0;
}
