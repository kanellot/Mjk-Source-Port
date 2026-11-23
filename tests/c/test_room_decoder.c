/**
 * @file test_room_decoder.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para las 256 salas (0x00..0xFF) y objetos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_rom_extractor.h"
#include "mjk_room_decoder.h"

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

int main(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando tests de 256 salas.\n");
        return 0;
    }

    /* 1. Decodificar las 256 salas (0x00..0xFF) sin índices fuera de rango */
    MjkRoomState room;
    for (unsigned r = 0u; r < 256u; ++r) {
        if (!MjkRoom_DecodeGeometry(&pkg, (uint8_t)r, &room)) {
            fprintf(stderr, "[FAIL] MjkRoom_DecodeGeometry fallo en sala 0x%02X\n", r);
            g_failures++;
        }
    }

    /* 2. Validar Sala 0 (Title Screen) */
    if (!MjkRoom_DecodeGeometry(&pkg, 0x00u, &room) || room.adj.flags != 1u || room.is_horizontal) {
        fprintf(stderr, "[FAIL] Sala 0x00 metadata inesperada (flags=%u)\n", room.adj.flags);
        g_failures++;
    }

    /* 3. Validar Sala 1 y Sala 2 (Sección 1: bombas y cofres) */
    MjkRoomPersistRam persist;
    memset(&persist, 0, sizeof(persist));
    if (!MjkRoom_PreloadSection(&pkg, 0x01u, &persist, 0u)) {
        fprintf(stderr, "[FAIL] PreloadSection(0x01) fallo\n");
        g_failures++;
    }
    if (persist.slots[0].room_id != 0x01u ||
        persist.slots[0].objects[0] != 0x12u ||
        persist.slots[0].objects[1] != 0x85u ||
        persist.slots[0].objects[2] != 0x05u ||
        persist.slots[0].objects[6] != 0x10u ||
        persist.slots[0].objects[8] != 0xFFu) {
        fprintf(stderr, "[FAIL] Slot 0 (Sala 1) persistencia inesperada\n");
        g_failures++;
    }
    if (persist.slots[1].room_id != 0x02u ||
        persist.slots[1].objects[0] != 0x05u ||
        persist.slots[1].objects[6] != 0x10u) {
        fprintf(stderr, "[FAIL] Slot 1 (Sala 2) persistencia inesperada\n");
        g_failures++;
    }

    MjkRoom_DecodeGeometry(&pkg, 0x01u, &room);
    if (!MjkRoom_DecodeObjects(&pkg, 0x01u, &persist, &room)) {
        fprintf(stderr, "[FAIL] DecodeObjects(0x01) fallo\n");
        g_failures++;
    }
    /* Bombas ($27) en $85,$87,$39,$6B,$6D y cofres ($28) en $C6,$CC */
    if (room.metatiles[8][5] != 0x27u || room.metatiles[8][7] != 0x27u ||
        room.metatiles[3][9] != 0x27u || room.metatiles[6][11] != 0x27u ||
        room.metatiles[6][13] != 0x27u ||
        room.metatiles[12][6] != 0x28u || room.metatiles[12][12] != 0x28u) {
        fprintf(stderr, "[FAIL] Metatiles de objetos de Sala 1 no coinciden\n");
        g_failures++;
    }

    /* 4. Bomb Room $90, Torture $AC y Endings $A0..$A3 */
    const uint8_t special_rooms[6] = { 0x90u, 0xACu, 0xA0u, 0xA1u, 0xA2u, 0xA3u };
    for (size_t i = 0u; i < 6u; ++i) {
        uint8_t rid = special_rooms[i];
        if (!MjkRoom_DecodeGeometry(&pkg, rid, &room) ||
            !MjkRoom_PreloadSection(&pkg, rid, &persist, 0u) ||
            !MjkRoom_DecodeObjects(&pkg, rid, &persist, &room)) {
            fprintf(stderr, "[FAIL] Sala especial 0x%02X fallo al decodificar\n", rid);
            g_failures++;
        }
    }

    MjkAssets_Unload(&pkg);

    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_room_decoder: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_room_decoder: 256 salas (0x00..0xFF), Sala 1/2, Bomb Room $90, Torture $AC y Endings $A0..$A3 OK.\n");
    return 0;
}
