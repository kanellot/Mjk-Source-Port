/**
 * @file test_player.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para física Q8.8, flutter y 9-probe (T-16, T-17).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_player.h"
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
        printf("[INFO] ROM local no encontrada; saltando test_player.\n");
        return 0;
    }

    MjkRoomState room;
    memset(&room, 0, sizeof(room));
    /* Suelo sólido en fila 12 (y=192..207) y plataforma en fila 9 (y=144..159) */
    for (int mx = 0; mx < 16; ++mx) {
        room.static_metatiles[12][mx] = 0x01u;
    }
    room.static_metatiles[9][8] = 0x01u;

    MjkPlayer p;
    MjkPlayer_Init(&p, 128u, 184u);
    MjkInputState in;
    memset(&in, 0, sizeof(in));

    /* 1. Velocidad horizontal instantánea ±0x0180 */
    MjkSoundId sfx = MJK_SND_SILENCE;
    bool add10 = false;
    MjkInput_Update(&in, MJK_BTN_RIGHT);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (p.vel_x != (mjk_q8_8_t)0x0180 || p.facing_attr != 0x04u) {
        fprintf(stderr, "[FAIL] vel_x derecha=%d (esperado 384)\n", p.vel_x);
        g_failures++;
    }
    MjkInput_Update(&in, 0u);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (p.vel_x != 0) {
        fprintf(stderr, "[FAIL] Parada instantanea fallo (vel_x=%d)\n", p.vel_x);
        g_failures++;
    }

    /* 2. Salto A + 10 pts + Flutter edge-triggered (0x4000) */
    MjkInput_Update(&in, MJK_BTN_A);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (!p.in_air || !p.just_jumped || sfx != MJK_SND_JUMP || !add10) {
        fprintf(stderr, "[FAIL] Salto inicial fallo\n");
        g_failures++;
    }
    MjkInput_Update(&in, 0u);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    MjkInput_Update(&in, MJK_BTN_A);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (p.jump_accum < 0x4000u) {
        fprintf(stderr, "[FAIL] Flutter no fijo jump_accum >= 0x4000 (0x%04X)\n", p.jump_accum);
        g_failures++;
    }

    /* 3. Sonda 0 muerte por fuego ($5C) */
    room.static_metatiles[11][8] = 0x5Cu;
    MjkPlayer_Init(&p, 132u, 180u);
    MjkInput_Update(&in, 0u);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (!p.is_dying || p.fire_tile_died_to != 0x5Cu) {
        fprintf(stderr, "[FAIL] Fuego $5C no disparo muerte\n");
        g_failures++;
    }

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_player: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_player: fisica Q8.8, flutter y 9-probe OK.\n");
    return 0;
}
