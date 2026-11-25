/**
 * @file test_game_flow_new.c
 * @brief Unit tests exhaustivos del flujo completo del juego y los 4 finales (R-13).
 */

#include <stdio.h>
#include <string.h>
#include "mjk/assets.h"
#include "mjk/game.h"
#include "mjk/input.h"
#include "mjk/rom_loader.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

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
    MjkInputLatch latch;
    MjkInputLatch_Init(&latch);
    MjkGame_Init(&g, &assets);

    /* 1. Copyright -> START -> Title Wait -> START -> Falling -> Round Intro -> Sala 1 Playing */
    step_n(&g, &latch, 0u, 10);
    CHECK(g.mode == SCREEN_TITLE && g.title.state == TITLE_COPYRIGHT_WAIT, "Copyright wait");

    step_n(&g, &latch, MJK_INPUT_START, 3);
    step_n(&g, &latch, 0u, 12);
    CHECK(g.mode == SCREEN_TITLE && g.title.state == TITLE_MAIN_WAIT, "Title wait tras START");

    step_n(&g, &latch, MJK_INPUT_START, 3);
    step_n(&g, &latch, 0u, 370);
    CHECK(g.mode == SCREEN_INGAME && g.state == GS_PLAYING, "InGame Playing tras caida + RoundIntro");
    CHECK(g.current_room_id == 1u && g.scoring.lives == 3 && g.scoring.timer_seconds == 60 && g.scoring.gdv == 47,
          "Estado inicial Sala 1 (3 vidas, timer 60, GDV 47)");

    /* 2. Pausa y despausa con START */
    step_n(&g, &latch, MJK_INPUT_START, 1);
    CHECK(g.paused && g.last_sound == SOUND_PAUSE, "Pausa activada");
    step_n(&g, &latch, 0u, 2);
    step_n(&g, &latch, MJK_INPUT_START, 1);
    CHECK(!g.paused && g.last_sound == SOUND_UNPAUSE, "Despausa activada");

    /* 3. Torture Room ($AC): 50 saltos US -> retorno a sala origen */
    g.torture_return_room_id   = 0x05u;
    g.torture_return_door_type = 0x0Fu;
    MjkGame_TriggerTortureRoom(&g);
    CHECK(g.state == GS_TORTURE_ROOM, "Trigger Torture Room");
    step_n(&g, &latch, 0u, 200);
    CHECK(g.current_room_id == 0xACu && g.greedy_jumps_left == 50, "Torture Room $AC con 50 saltos US");
    for (int j = 0; j < 49; ++j) {
        MjkGame_OnTortureJump(&g);
    }
    CHECK(g.greedy_jumps_left == 1 && g.in_torture_mode, "49 saltos: aun en Torture Room");
    MjkGame_OnTortureJump(&g);
    CHECK(g.greedy_jumps_left == 0 && !g.in_torture_mode && g.current_room_id == 0x05u && g.state == GS_LOAD_ROOM,
          "50.o salto sale de Torture Room a sala origen");

    /* 4. Round Clear desde Bomb Room $90 con >= 20 Fire Bombs -> Special Bonus */
    g.current_room_id      = 0x90u;
    g.exit_source_room_id  = 0x90u;
    g.fire_bombs_collected = 23u;
    g.scoring.timer_seconds = 10;
    g.scoring.score = 0u;
    MjkGame_ExitRoom(&g, 0u);
    CHECK(g.state == GS_ROUND_CLEAR && g.last_bomb_room_cleared == 0x90u, "Exit desde $90 entra a Round Clear");
    step_n(&g, &latch, 0u, 700);
    CHECK(g.state != GS_ROUND_CLEAR && g.scoring.score >= 51000u,
          "Round Clear otorga Time Bonus (1000) + Special Bonus (50000)");

    /* 5. Los 4 Finales ($A0..$A3) */
    for (uint8_t et = 0u; et < 4u; ++et) {
        MjkGame_Init(&g, &assets);
        g.mode = SCREEN_INGAME;
        MjkScoring_ResetNewGame(&g.scoring);
        g.crystal_balls_mask  = (et == 0u) ? 0u : (et == 1u) ? 1u : 3u;
        g.secret_coins        = (et == 2u) ? 2u : 5u;
        g.exit_source_room_id = (uint8_t)(0xA0u + et);
        g.current_room_id     = (uint8_t)(0xA0u + et);
        MjkGame_ExitRoom(&g, 0u);
        CHECK(g.state == GS_ENDING && g.ending_type == et, "Final disparado correctamente");
        step_n(&g, &latch, 0u, 900);
        CHECK(g.ending_state == 4u, "Secuencia de final completada hasta espera START");
        step_n(&g, &latch, MJK_INPUT_START, 1);
        CHECK(g.state == GS_GAME_OVER, "START en final transiciona a Game Over");
    }

    /* 6. Game Over -> bonus GDV por marcador (+1 por cada 200k pts) y vuelta a SCREEN_TITLE */
    g.scoring.gdv             = 47;
    g.scoring.score           = 800000u;
    g.last_bomb_room_cleared  = 0u;
    g.extra_lives_found       = 0u;
    g.scoring.lives           = 1;
    MjkGame_LoseLife(&g);
    CHECK(g.state == GS_GAME_OVER && g.scoring.lives == 0, "0 vidas dispara Game Over");
    step_n(&g, &latch, 0u, 1);
    CHECK(g.scoring.gdv == 51, "Game Over suma +4 GDV por 800.000 pts (47 -> 51)");
    step_n(&g, &latch, 0u, 260);
    CHECK(g.mode == SCREEN_TITLE, "Game Over retorna a pantalla de titulo");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_game_flow_new fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_game_flow_new superado (ciclo completo, Torture 50 saltos, RoundClear y 4 Finales).\n");
    return 0;
}
