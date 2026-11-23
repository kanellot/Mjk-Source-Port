/**
 * @file test_state_machine.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para Hito 4 (Title + GameState 0..C).
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
    if (!f) {
        return false;
    }
    uint8_t* rom = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom) {
        fclose(f);
        return false;
    }
    size_t got = fread(rom, 1u, MJK_ROM_EXPECTED_SIZE, f);
    fclose(f);
    if (got != MJK_ROM_EXPECTED_SIZE) {
        free(rom);
        return false;
    }
    uint8_t* v2_buf = NULL;
    size_t v2_size = 0u;
    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    MjkAssetError err = MjkRom_BuildAssetsV2Buffer(rom, MJK_ROM_EXPECTED_SIZE, &v2_buf, &v2_size, sha1_hex);
    free(rom);
    if (err != MJK_ASSET_OK) {
        free(v2_buf);
        return false;
    }
    err = MjkAssets_LoadFromMemory(v2_buf, v2_size, out_pkg);
    free(v2_buf);
    return (err == MJK_ASSET_OK);
#else
    (void)out_pkg;
    return false;
#endif
}

static void step_n(MjkGame* g, int frames, uint8_t raw_btn) {
    MjkInputState in;
    memset(&in, 0, sizeof(in));
    for (int i = 0; i < frames; ++i) {
        MjkInput_Update(&in, raw_btn);
        MjkGame_StepFrame(g, &in);
    }
}

int main(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando test_state_machine.\n");
        return 0;
    }

    MjkGame g;
    MjkGame_Init(&g, &pkg);

    /* 1. Copyright (1 init + 256 wait) <-> Title (1 init + 512 wait) */
    step_n(&g, 258, 0u);
    if (g.top_mode != MJK_MODE_TITLE || g.title.state != MJK_TS_TITLE_WAIT) {
        fprintf(stderr, "[FAIL] Copyright->Title fallo (state=%d)\n", (int)g.title.state);
        g_failures++;
    }
    step_n(&g, 512, 0u);
    if (g.title.state != MJK_TS_US_COPYRIGHT_INIT) {
        fprintf(stderr, "[FAIL] Title timeout->Copyright fallo (state=%d)\n", (int)g.title.state);
        g_failures++;
    }

    /* 2. Copyright + START -> Title -> START -> Jack Falling (~73f) -> Init -> RoundIntro -> Main */
    step_n(&g, 2, MJK_BTN_START);
    if (g.title.state != MJK_TS_TITLE_INIT && g.title.state != MJK_TS_TITLE_WAIT) {
        fprintf(stderr, "[FAIL] Skip Copyright con START fallo (state=%d)\n", (int)g.title.state);
        g_failures++;
    }
    step_n(&g, 2, 0u);
    step_n(&g, 1, MJK_BTN_START);
    if (g.title.state != MJK_TS_JACK_FALLING) {
        fprintf(stderr, "[FAIL] Title+START no paso a JACK_FALLING (state=%d)\n", (int)g.title.state);
        g_failures++;
    }
    int fall_frames = 0;
    while (g.top_mode == MJK_MODE_TITLE && fall_frames < 120) {
        step_n(&g, 1, 0u);
        fall_frames++;
    }
    if (g.top_mode != MJK_MODE_INGAME || fall_frames < 65 || fall_frames > 85) {
        fprintf(stderr, "[FAIL] Jack Falling tardo %d frames\n", fall_frames);
        g_failures++;
    }
    step_n(&g, 1, 0u); /* Init */
    if (g.state != MJK_GS_ROUND_INTRO || g.current_room_id != 1u || g.lives != 3u || g.gdv_bcd != 0x47u) {
        fprintf(stderr, "[FAIL] Init valores incorrectos (st=%d rm=%u lv=%u gdv=0x%02X)\n",
                (int)g.state, g.current_room_id, g.lives, g.gdv_bcd);
        g_failures++;
    }
    step_n(&g, 97, 0u); /* RoundIntro (96f) -> LoadRoom */
    step_n(&g, 28, 0u); /* Load + Draw + Enter (24f) -> Main */
    if (g.state != MJK_GS_MAIN) {
        fprintf(stderr, "[FAIL] No llego a MJK_GS_MAIN (st=%d)\n", (int)g.state);
        g_failures++;
    }

    /* 3. Pausa START */
    uint16_t tf_before = g.timer_frames;
    step_n(&g, 1, MJK_BTN_START);
    if (!g.paused || g.last_sound_event != MJK_SND_PAUSE) {
        fprintf(stderr, "[FAIL] Pausa START fallo\n");
        g_failures++;
    }
    step_n(&g, 5, 0u);
    if (g.timer_frames != tf_before) {
        fprintf(stderr, "[FAIL] timer_frames avanzo en pausa\n");
        g_failures++;
    }
    step_n(&g, 1, MJK_BTN_START);
    if (g.paused || g.last_sound_event != MJK_SND_UNPAUSE) {
        fprintf(stderr, "[FAIL] Unpause START fallo\n");
        g_failures++;
    }

    /* 4. ExitRoom topologico y RoundClear */
    g.entry_door_type = 0x08u; /* Right (dir=2) */
    MjkGame_ExitRoom(&g, 2u);
    if (g.current_room_id != 2u || g.entry_door_type != 0x0Cu) {
        fprintf(stderr, "[FAIL] ExitRoom 1->2 fallo (rm=%u door=0x%02X)\n",
                g.current_room_id, g.entry_door_type);
        g_failures++;
    }
    g.current_room_id = 0x90u;
    g.fire_bombs_collected = 21u;
    g.stage_timer_bcd = 0x02u;
    MjkGame_ExitRoom(&g, 0u);
    if (g.state != MJK_GS_ROUND_CLEAR) {
        fprintf(stderr, "[FAIL] ExitRoom $90 no entro en ROUND_CLEAR\n");
        g_failures++;
    }
    step_n(&g, 350, 0u);
    if (g.state != MJK_GS_LOAD_ROOM || g.score_bcd[2] == 0u) {
        fprintf(stderr, "[FAIL] RoundClear no completo bonus/transicion (st=%d)\n", (int)g.state);
        g_failures++;
    }

    /* 5. TimeOver 128f -> LoseLife -> checkpoint */
    g.lives = 2u;
    g.checkpoint_room_id = 0x01u;
    g.state = MJK_GS_TIME_OVER;
    g.sub_state = 0u;
    step_n(&g, 130, 0u);
    if (g.lives != 1u || g.state != MJK_GS_ROUND_INTRO || g.current_room_id != 0x01u) {
        fprintf(stderr, "[FAIL] TimeOver->LoseLife fallo (lv=%u st=%d rm=%u)\n",
                g.lives, (int)g.state, g.current_room_id);
        g_failures++;
    }

    /* 6. TortureRoom (192f + 50 saltos BCD) */
    g.mighty_coins = 9u;
    g.torture_return_room_id = 0x01u;
    g.torture_return_door_type = 0x0Fu;
    MjkGame_TriggerTortureRoom(&g);
    step_n(&g, 194, 0u);
    if (g.mighty_coins != 0u || g.current_room_id != 0xACu || g.greedy_jumps_bcd != 0x50u) {
        fprintf(stderr, "[FAIL] TortureRoom init fallo (rm=0x%02X jumps=0x%02X)\n",
                g.current_room_id, g.greedy_jumps_bcd);
        g_failures++;
    }
    for (int j = 0; j < 50; ++j) {
        MjkGame_OnTortureJump(&g);
    }
    if (g.torture_scene_flag != 0u || g.current_room_id != 0x01u || g.state != MJK_GS_LOAD_ROOM) {
        fprintf(stderr, "[FAIL] TortureRoom 50 saltos retorno fallo\n");
        g_failures++;
    }

    /* 7. 4 Endings ($A0..$A3) */
    const uint8_t end_rooms[4] = { 0xA0u, 0xA1u, 0xA2u, 0xA3u };
    for (int e = 0; e < 4; ++e) {
        g.state = MJK_GS_MAIN;
        g.current_room_id = end_rooms[e];
        g.crystal_balls_mask = (e == 0) ? 0u : (e == 1 ? 1u : 3u);
        g.secret_coins = (e == 2) ? 0u : 5u;
        uint8_t prev_gdv = g.gdv_bcd;
        MjkGame_ExitRoom(&g, 0u);
        if (g.state != MJK_GS_ENDING || g.ending_type != (uint8_t)e ||
            g.gdv_bcd != MjkGame_AddGdvBcd(prev_gdv, 3u)) {
            fprintf(stderr, "[FAIL] Ending %d ($%02X) fallo\n", e, end_rooms[e]);
            g_failures++;
        }
    }

    /* 8. GameOver (GDV BCD + 256f -> Title) */
    step_n(&g, 900, 0u); /* avanzar hasta ending_state 4 */
    step_n(&g, 1, MJK_BTN_START);
    if (g.state != MJK_GS_GAME_OVER) {
        fprintf(stderr, "[FAIL] Ending+START no paso a GAME_OVER (st=%d)\n", (int)g.state);
        g_failures++;
    }
    step_n(&g, 258, 0u);
    if (g.top_mode != MJK_MODE_TITLE || g.high_gdv_bcd == 0u) {
        fprintf(stderr, "[FAIL] GameOver->Title fallo (mode=%d high_gdv=0x%02X)\n",
                (int)g.top_mode, g.high_gdv_bcd);
        g_failures++;
    }

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_state_machine: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_state_machine: Copyright, Title, Rounds, Torture (50 BCD), 4 Endings y GameOver OK.\n");
    return 0;
}
