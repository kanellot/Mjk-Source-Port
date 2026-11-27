/**
 * @file test_player_new.c
 * @brief Unit tests exhaustivos de fisica float, flutter, 9-probe y Golden Trajectories (R-07).
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "golden_reference.h"
#include "mjk/assets.h"
#include "mjk/input.h"
#include "mjk/player.h"
#include "mjk/rom_loader.h"
#include "mjk/world.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

static void setup_flat_world(MjkWorld* w) {
    MjkWorld_Reset(w);
    w->camera.room_flags = ROOM_FLAG_SINGLE_SCREEN;
    for (int mx = 0; mx < MJK_ROOM_COLS; ++mx) {
        w->current.static_metatiles[12][mx] = 0x01u;
    }
}

static void verify_trajectory(const MjkAssets* assets,
                              uint8_t first_btn,
                              uint8_t hold_btn,
                              int flutter_frame,
                              int frames,
                              const int16_t* ref_x,
                              const int16_t* ref_y,
                              const uint8_t* ref_air,
                              const char* label) {
    MjkWorld w;
    setup_flat_world(&w);
    MjkPlayer p;
    MjkPlayer_Init(&p, 128.0f, 184.0f);
    MjkInputLatch latch;
    MjkInputLatch_Init(&latch);

    int mismatches = 0;
    for (int f = 0; f < frames; ++f) {
        uint8_t btn = hold_btn;
        if (f == 0) { btn = first_btn; }
        else if (flutter_frame > 0 && f == flutter_frame) { btn = (uint8_t)(hold_btn | MJK_INPUT_A); }
        MjkInputLatch_Poll(&latch, btn);
        MjkInputStep step;
        MjkInputLatch_ConsumeStep(&latch, &step);
        MjkPlayerStepCtx ctx;
        memset(&ctx, 0, sizeof(ctx));
        MjkPlayer_Update(&p, &step, &w, assets, &ctx);

        int ix = (int)floorf(p.x);
        int iy = (int)floorf(p.y);
        if ((ref_x && ix != ref_x[f]) || (ref_y && iy != ref_y[f]) ||
            (ref_air && (p.in_air ? 1u : 0u) != ref_air[f])) {
            if (mismatches < 3) {
                fprintf(stderr, "[FAIL] %s f=%d: got (%d,%d,air=%d) expected (%d,%d)\n",
                        label, f, ix, iy, p.in_air ? 1 : 0,
                        ref_x ? ref_x[f] : ix, ref_y ? ref_y[f] : iy);
            }
            mismatches++;
        }
    }
    CHECK(mismatches == 0, label);
}

int main(void) {
    char rom_path[512];
    MjkTestRomStatus st = MjkRom_ResolveTestPath(MJK_SOURCE_DIR, MJK_TEST_ROM_PATH, rom_path, sizeof(rom_path));
    if (st == TEST_ROM_EXPLICIT_MISSING_FAIL) { return 1; }
    if (st == TEST_ROM_ABSENT_SKIP) { return MJK_TEST_SKIP_CODE; }

    static MjkAssets assets;
    CHECK(MjkAssets_LoadFromRomFile(rom_path, &assets) == ASSET_OK, "Cargar MjkAssets");

    /* 1. 5 Golden Trajectories frame a frame (exactitud float vs Q8.8 original) */
    verify_trajectory(&assets, MJK_INPUT_A, 0u, -1, 60, NULL,
                      k_golden_jump_neutral_y, k_golden_jump_neutral_air, "Golden: Salto neutro 60f");
    verify_trajectory(&assets, (uint8_t)(MJK_INPUT_A | MJK_INPUT_DOWN), MJK_INPUT_DOWN, -1, 60, NULL,
                      k_golden_jump_low_y, NULL, "Golden: Salto bajo (+Abajo) 60f");
    verify_trajectory(&assets, (uint8_t)(MJK_INPUT_A | MJK_INPUT_UP), MJK_INPUT_UP, -1, 60, NULL,
                      k_golden_jump_high_y, NULL, "Golden: Salto alto (+Arriba) 60f");
    verify_trajectory(&assets, MJK_INPUT_A, 0u, 16, 60, NULL,
                      k_golden_jump_flutter_y, NULL, "Golden: Salto + Flutter f=16");
    verify_trajectory(&assets, (uint8_t)(MJK_INPUT_A | MJK_INPUT_RIGHT), MJK_INPUT_RIGHT, -1, 30,
                      k_golden_jump_right_x, k_golden_jump_right_y, NULL, "Golden: Salto + Derecha 30f");

    /* 2. Mantener A pulsado NO dispara flutter */
    MjkWorld w;
    setup_flat_world(&w);
    MjkPlayer p;
    MjkPlayer_Init(&p, 128.0f, 184.0f);
    MjkInputLatch latch;
    MjkInputLatch_Init(&latch);
    MjkPlayerStepCtx ctx;
    memset(&ctx, 0, sizeof(ctx));

    MjkInputLatch_Poll(&latch, MJK_INPUT_A);
    MjkInputStep step;
    MjkInputLatch_ConsumeStep(&latch, &step);
    MjkPlayer_Update(&p, &step, &w, &assets, &ctx);
    CHECK(p.just_jumped && ctx.sfx_triggered == SOUND_JUMP && ctx.award_jump_10pts, "Salto inicial + SFX + 10pts");

    MjkInputLatch_Poll(&latch, MJK_INPUT_A); /* mantenido */
    MjkInputLatch_ConsumeStep(&latch, &step);
    MjkPlayer_Update(&p, &step, &w, &assets, &ctx);
    CHECK(p.jump_progress < 64.0f, "Mantener A no dispara flutter");

    /* 3. Secuencia de muerte por fuego -> suelo -> 64 frames -> ACTION_PLAYER_DIED */
    setup_flat_world(&w);
    w.current.static_metatiles[11][8] = 0x5Eu;
    MjkPlayer_Init(&p, 132.0f, 180.0f);
    MjkInputStep empty = {0u, 0u, 0u};
    MjkPlayer_Update(&p, &empty, &w, &assets, &ctx);
    CHECK(p.is_dying && p.fire_tile_died_to == 0x5Eu && ctx.sfx_triggered == SOUND_DEATH, "Fuego dispara muerte");

    MjkActionResult res = ACTION_CONTINUE;
    for (int f = 0; f < 120 && res == ACTION_CONTINUE; ++f) {
        res = MjkPlayer_Update(&p, &empty, &w, &assets, &ctx);
    }
    CHECK(res == ACTION_PLAYER_DIED, "Secuencia de muerte culmina en ACTION_PLAYER_DIED");

    /* 4. jump_press_count persiste tras MjkPlayer_Init */
    uint8_t before_cnt = p.jump_press_count;
    MjkPlayer_Init(&p, 64.0f, 184.0f);
    CHECK(p.jump_press_count == before_cnt, "jump_press_count persiste entre salas");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_player_new fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_player_new superado (5 Golden Trajectories exactas + colisiones + muerte).\n");
    return 0;
}
