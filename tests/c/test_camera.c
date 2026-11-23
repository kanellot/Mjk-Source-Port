/**
 * @file test_camera.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para scroll de pasillos y seams (T-18).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_camera.h"
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
        printf("[INFO] ROM local no encontrada; saltando test_camera.\n");
        return 0;
    }

    MjkRoomState r1;
    MjkRoom_DecodeGeometry(&pkg, 0x01u, &r1);
    MjkCamera cam;
    MjkCamera_InitForRoom(&cam, 0x01u, &r1.adj, false);

    MjkPlayer p;
    MjkPlayer_Init(&p, 128u, 184u);
    p.vel_x = (mjk_q8_8_t)0x0180;

    /* Desbloquear borde inicial y cruzar de Sala 1 -> Sala 2 */
    bool crossed_1_to_2 = false;
    for (int f = 0; f < 220; ++f) {
        MjkCamera_Update(&cam, &p, &pkg);
        if (cam.room_crossed && cam.current_room_id == 0x02u) {
            crossed_1_to_2 = true;
            break;
        }
    }
    if (!crossed_1_to_2) {
        fprintf(stderr, "[FAIL] Camara no cruzo Sala 1 -> Sala 2 (rm=%u)\n", cam.current_room_id);
        g_failures++;
    }

    /* Retroceder Sala 2 -> Sala 1 sin activar RF_SECTION_START (RES-035) */
    p.vel_x = (mjk_q8_8_t)(-0x0180);
    bool crossed_2_to_1 = false;
    for (int f = 0; f < 60; ++f) {
        MjkCamera_Update(&cam, &p, &pkg);
        if (cam.room_crossed && cam.current_room_id == 0x01u) {
            crossed_2_to_1 = true;
            break;
        }
    }
    if (!crossed_2_to_1 || (cam.room_flags & MJK_RF_SECTION_START) != 0u) {
        fprintf(stderr, "[FAIL] Retroceso Sala 2 -> 1 fallo (flags=0x%02X)\n", cam.room_flags);
        g_failures++;
    }

    /* Sala 6 (fin de pasillo antes de $90) no tiene neighbor_room_id hacia $90 (RES-033) */
    MjkRoomState r6;
    MjkRoom_DecodeGeometry(&pkg, 0x06u, &r6);
    MjkCamera_InitForRoom(&cam, 0x06u, &r6.adj, false);
    if (cam.neighbor_room_id != 0u || (cam.room_flags & MJK_RF_AT_SCROLL_EDGE) == 0u) {
        fprintf(stderr, "[FAIL] Sala 6 no clampo en borde derecho\n");
        g_failures++;
    }

    MjkAssets_Unload(&pkg);
    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_camera: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_camera: scroll pasillo 1<->2 y clamp Sala 6 OK.\n");
    return 0;
}
