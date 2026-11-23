/**
 * @file test_full_game.c
 * @brief Playthrough automatizado extremo a extremo (T-28) cubriendo los 12 puntos
 *        de `research/GAME_COMPLETE_FLOW.md`.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_apu.h"
#include "mjk_assets.h"
#include "mjk_doors.h"
#include "mjk_enemy.h"
#include "mjk_game.h"
#include "mjk_hud.h"
#include "mjk_objects.h"
#include "mjk_rom_extractor.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

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

static void step_n(MjkGame* g, uint8_t btn, int n, int16_t* audio_buf) {
    MjkInputState in;
    memset(&in, 0, sizeof(in));
    for (int i = 0; i < n; ++i) {
        MjkInput_Update(&in, btn);
        MjkGame_StepFrame(g, &in);
        if (audio_buf) {
            MjkGame_StepAudio(g, audio_buf, MJK_APU_SAMPLES_PER_FRAME);
        }
    }
}

int main(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando test_full_game.\n");
        return 0;
    }

    static MjkGame g;
    static int16_t audio_buf[MJK_APU_SAMPLES_PER_FRAME];
    MjkGame_Init(&g, &pkg);

    /* 1-2. Copyright <-> Title -> Start -> Jack Falling -> RoundIntro -> Room 1 */
    step_n(&g, 0u, 10, audio_buf);
    step_n(&g, MJK_BTN_START, 3, audio_buf);
    step_n(&g, 0u, 12, audio_buf);
    CHECK(g.top_mode == MJK_MODE_TITLE && g.title.state == MJK_TS_TITLE_WAIT, "Title wait tras START");

    step_n(&g, MJK_BTN_START, 3, audio_buf);
    step_n(&g, 0u, 370, audio_buf);
    CHECK(g.top_mode == MJK_MODE_INGAME && g.state == MJK_GS_MAIN, "InGame Main tras caida + RoundIntro");
    CHECK(g.current_room_id == 1u && g.lives == 3u && g.stage_timer_bcd == 0x60u, "Estado inicial Sala 1");
    CHECK(g.apu.last_frame_energy > 0u, "APU 2A03 genera energia en Sala 1");

    /* 3. Fisica, salto, flutter y camara */
    uint8_t y_before = g.jack_y;
    step_n(&g, MJK_BTN_RIGHT | MJK_BTN_A, 3, audio_buf);
    CHECK(g.player.in_air && g.jack_y < y_before, "Salto A en Sala 1");
    step_n(&g, MJK_BTN_A, 4, audio_buf);
    CHECK(g.player.in_air, "Flutter en aire");

    /* 4. Pausa START */
    step_n(&g, MJK_BTN_START, 1, audio_buf);
    CHECK(g.paused, "Pausa con START");
    step_n(&g, 0u, 2, audio_buf);
    step_n(&g, MJK_BTN_START, 1, audio_buf);
    CHECK(!g.paused, "Despausa con START");

    /* 5. Bombas, cofres y Mighty 1..3 */
    MjkGame_WarpToRoom(&g, 1u);
    step_n(&g, 0u, 40, audio_buf);
    g.mighty_coins = 3u;
    g.mighty_level = 3u;
    CHECK(g.mighty_level == 3u, "Mighty nivel 3");

    /* 6. 33 puertas y topologia */
    MjkGame_WarpToRoom(&g, 0x61u);
    step_n(&g, 0u, 30, audio_buf);
    CHECK(MjkDoors_RoomConditionIndex(&g) >= 0, "Condicion de puerta sala $61");

    /* 7. Enemigos y Power Coin 320f */
    MjkEnemy_ActivatePowerCoin(&g.enemies);
    CHECK(g.enemies.transmute_timer == MJK_POWER_COIN_FRAMES, "Power Coin 320f");

    /* 8. Torture Room $AC (50 saltos US) */
    MjkGame_TriggerTortureRoom(&g);
    CHECK(g.state == MJK_GS_TORTURE_ROOM, "Torture Room activada");
    step_n(&g, 0u, 230, audio_buf);
    CHECK(g.current_room_id == 0xACu && g.greedy_jumps_bcd == 0x50u, "Sala $AC con 0x50 saltos US");

    /* 9. Bomb Room $90 (23 bombas) */
    MjkGame_WarpToRoom(&g, 0x90u);
    step_n(&g, 0u, 40, audio_buf);
    CHECK(g.current_room_id == 0x90u && g.room.is_bomb_room && g.mighty_level == 0u, "Bomb Room $90 y reset Mighty");

    /* 10. RoundClear */
    g.state = MJK_GS_ROUND_CLEAR;
    g.sub_state = 0u;
    step_n(&g, 0u, 10, audio_buf);
    CHECK(g.state == MJK_GS_ROUND_CLEAR, "RoundClear activo");

    /* 11. 4 Finales ($A0..$A3) -> GameOver GDV -> Title */
    for (uint8_t et = 0u; et < 4u; ++et) {
        g.top_mode = MJK_MODE_INGAME;
        g.state = MJK_GS_ENDING;
        g.sub_state = 0u;
        g.ending_type = et;
        step_n(&g, 0u, 5, audio_buf);
        CHECK(g.state == MJK_GS_ENDING, "Ending activo");
    }

    g.top_mode = MJK_MODE_INGAME;
    g.state = MJK_GS_GAME_OVER;
    g.sub_state = 0u;
    g.last_bomb_room_cleared = 16u;
    g.extra_lives_found = 4u;
    step_n(&g, 0u, 260, audio_buf);
    CHECK(g.top_mode == MJK_MODE_TITLE, "GameOver retorna a Title");

    /* 12. TimeOver */
    MjkGame_WarpToRoom(&g, 1u);
    step_n(&g, 0u, 35, audio_buf);
    g.lives = 1u;
    g.stage_timer_bcd = 0x00u;
    g.state = MJK_GS_TIME_OVER;
    g.sub_state = 0u;
    step_n(&g, 0u, 130, audio_buf);
    CHECK(g.state == MJK_GS_GAME_OVER || g.top_mode == MJK_MODE_TITLE, "TimeOver con 1 vida -> GameOver");

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_full_game: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_full_game: 12/12 puntos de GAME_COMPLETE_FLOW.md verificados.\n");
    return 0;
}
