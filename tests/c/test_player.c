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

    /* 4. Regresión Bug 2: Cofre rojo cerrado (0x10) sólido y cofre oculto (0x30) solo sólido por arriba */
    memset(&room, 0, sizeof(room));
    room.object_count = 1u;
    room.objects[0].yx = 0x84u; /* my=8 (y=128..143), mx=4 (x=64..79) */
    room.objects[0].ram_state = 0x10u;
    MjkPlayer_Init(&p, 0x48u, 0x75u);
    p.in_air = true;
    p.jump_accum = 0x4000u;
    MjkInput_Update(&in, 0u);
    for (int f = 0; f < 15; ++f) {
        MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    }
    if (p.in_air || (p.collision_flags & MJK_CF_GROUND) == 0u) {
        fprintf(stderr, "[FAIL] Jack no aterrizo sobre cofre cerrado 0x10 (in_air=%d cf=0x%02X)\n",
                p.in_air, p.collision_flags);
        g_failures++;
    }
    /* Cofre oculto (0x30) NO debe bloquear pared lateral */
    room.objects[0].ram_state = 0x30u;
    MjkPlayer_Init(&p, 0x36u, 0x88u);
    MjkInput_Update(&in, MJK_BTN_RIGHT);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if ((p.collision_flags & MJK_CF_RIGHT_WALL) != 0u) {
        fprintf(stderr, "[FAIL] Cofre oculto 0x30 bloqueo pared lateral\n");
        g_failures++;
    }

    /* 5. Regresión muerte en suelo vs muerte en aire (trayectoria estrictamente vertical, prg.asm:1304-1366) */
    memset(&room, 0, sizeof(room));
    for (uint8_t x = 0u; x < MJK_ROOM_MT_COLS; ++x) {
        room.static_metatiles[13][x] = 0x01u; /* Suelo en y=208 */
    }
    /* 5a. Muerte en el suelo -> directamente tendido (0x2A), sin volteretas */
    MjkPlayer_Init(&p, 100u, 200u);
    p.is_dying = true;
    p.death_grounded = false;
    p.death_timer = 0u;
    MjkInput_Update(&in, MJK_BTN_RIGHT);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (!p.death_grounded || p.sprite_id != 0x2Au || MjkUQ8_Int(p.pos_x) != 100u) {
        fprintf(stderr, "[FAIL] Muerte en suelo no tendio a Jack inmediatamente (g=%d spr=0x%02X x=%u)\n",
                p.death_grounded ? 1 : 0, p.sprite_id, MjkUQ8_Int(p.pos_x));
        g_failures++;
    }

    /* 5b. Muerte en el aire con input RIGHT y vel_x previa -> volteretas verticales (X constante) hasta suelo */
    MjkPlayer_Init(&p, 100u, 120u);
    p.in_air = true;
    p.jump_accum = 0x4000u;
    p.vel_x = (mjk_q8_8_t)0x0180;
    p.is_dying = true;
    p.death_grounded = false;
    p.death_timer = 0u;
    bool saw_tumble = false;
    for (int f = 0; f < 60 && !p.death_grounded; ++f) {
        MjkInput_Update(&in, MJK_BTN_RIGHT);
        MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
        if (MjkUQ8_Int(p.pos_x) != 100u || p.vel_x != 0) {
            fprintf(stderr, "[FAIL] Muerte en aire movio X horizontalmente (x=%u vx=%d)\n",
                    MjkUQ8_Int(p.pos_x), (int)p.vel_x);
            g_failures++;
            break;
        }
        if (p.sprite_id == 0x28u || p.sprite_id == 0x29u) {
            saw_tumble = true;
        }
    }
    if (!saw_tumble || !p.death_grounded || p.sprite_id != 0x2Au || MjkUQ8_Int(p.pos_y) < 192u) {
        fprintf(stderr, "[FAIL] Muerte en aire no dio volteretas verticales hasta el suelo (tumble=%d g=%d spr=0x%02X y=%u)\n",
                saw_tumble ? 1 : 0, p.death_grounded ? 1 : 0, p.sprite_id, MjkUQ8_Int(p.pos_y));
        g_failures++;
    }

    /* 6. Regresión P-01 (salir por borde resetea jump_accum a 0x4000) y P-03 (suelo $E0) */
    memset(&room, 0, sizeof(room));
    MjkPlayer_Init(&p, 100u, 100u);
    p.in_air = false;
    p.jump_accum = 0x7F00u; /* Velocidad terminal residual de una caída previa */
    MjkInput_Update(&in, 0u);
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (!p.in_air || p.jump_accum != 0x4000u) {
        fprintf(stderr, "[FAIL] P-01: salir por borde no reseteo jump_accum (in_air=%d acc=0x%04X)\n",
                p.in_air ? 1 : 0, p.jump_accum);
        g_failures++;
    }
    MjkPlayer_Init(&p, 100u, 0xDFu);
    p.in_air = true;
    p.jump_accum = 0x7000u;
    MjkPlayer_Update(&p, &in, &pkg, &room, NULL, MJK_RF_SINGLE_SCREEN, 0u, 0u, false, NULL, NULL, NULL, &sfx, &add10);
    if (p.in_air || MjkUQ8_Int(p.pos_y) != 0xE0u) {
        fprintf(stderr, "[FAIL] P-03: suelo de seguridad $E0 fallo (in_air=%d y=0x%02X)\n",
                p.in_air ? 1 : 0, MjkUQ8_Int(p.pos_y));
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
