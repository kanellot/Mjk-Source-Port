/**
 * @file test_assets.c
 * @brief Unit tests exhaustivos para SHA-1, BYOR v2 y decodificacion de 256 salas (R-03, R-04).
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "golden_reference.h"
#include "mjk/assets.h"
#include "mjk/rom_loader.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

static uint32_t fnv1a_32(const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint32_t h = 2166136261u;
    for (size_t i = 0u; i < len; ++i) {
        h ^= (uint32_t)p[i];
        h *= 16777619u;
    }
    return h;
}

static void test_sha1_and_crc32_vectors(void) {
    uint8_t digest[MJK_SHA1_DIGEST_LEN];
    char hex[MJK_SHA1_HEX_LEN + 1u];

    MjkHash_Sha1("abc", 3u, digest);
    MjkHash_Sha1Hex(digest, hex);
    CHECK(strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d") == 0, "SHA-1 vector 'abc'");

    MjkHash_Sha1("", 0u, digest);
    MjkHash_Sha1Hex(digest, hex);
    CHECK(strcmp(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709") == 0, "SHA-1 vector vacio");

    uint32_t crc = MjkHash_Crc32("123456789", 9u);
    CHECK(crc == 0xCBF43926u, "CRC-32 vector '123456789'");

    /* Validacion de errores en cabecera iNES / tamano */
    uint8_t dummy[16] = {0};
    CHECK(MjkRom_ValidateInes(dummy, sizeof(dummy), NULL) == ASSET_ERR_BAD_SIZE, "ROM tamano erroneo");
}

int main(void) {
    test_sha1_and_crc32_vectors();

    char rom_path[512];
    MjkTestRomStatus st = MjkRom_ResolveTestPath(MJK_SOURCE_DIR, MJK_TEST_ROM_PATH, rom_path, sizeof(rom_path));
    if (st == TEST_ROM_EXPLICIT_MISSING_FAIL) {
        fprintf(stderr, "[FAIL] ROM explicita no encontrada: %s\n", rom_path);
        return 1;
    }
    if (st == TEST_ROM_ABSENT_SKIP) {
        printf("[SKIP] ROM local ausente; omitiendo pruebas con ROM (CTest code 77).\n");
        return (g_failures > 0) ? 1 : MJK_TEST_SKIP_CODE;
    }

    /* 1. Round-trip BYOR v2 y deteccion de CRC-32 corrupto */
    FILE* f = fopen(rom_path, "rb");
    CHECK(f != NULL, "Abrir ROM");
    if (!f) { return 1; }
    uint8_t* rom = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    size_t got = fread(rom, 1u, MJK_ROM_EXPECTED_SIZE, f);
    fclose(f);
    CHECK(got == MJK_ROM_EXPECTED_SIZE, "Leer 65552 bytes de ROM");

    uint8_t* dat = NULL;
    size_t dat_sz = 0u;
    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    MjkAssetError err = MjkRom_BuildAssetsDat(rom, MJK_ROM_EXPECTED_SIZE, &dat, &dat_sz, sha1_hex);
    CHECK(err == ASSET_OK, "BuildAssetsDat OK");
    CHECK(strcmp(sha1_hex, MJK_ROM_EXPECTED_SHA1) == 0, "SHA-1 canonico REV_US");

    /* Corromper 1 byte de la ROM -> debe fallar SHA-1 */
    rom[100] ^= 0xFFu;
    CHECK(MjkRom_ValidateInes(rom, MJK_ROM_EXPECTED_SIZE, NULL) == ASSET_ERR_BAD_ROM_SHA1, "ROM corrupta detectada");
    free(rom);

    /* Corromper 1 byte de assets.dat -> debe fallar CRC-32 */
    static MjkAssets assets;
    dat[MJK_ASSETS_HEADER_SIZE + 10u] ^= 0x01u;
    CHECK(MjkAssets_LoadFromAssetsMemory(dat, dat_sz, &assets) == ASSET_ERR_BAD_CRC, "CRC-32 corrupto detectado");
    dat[MJK_ASSETS_HEADER_SIZE + 10u] ^= 0x01u;

    /* 2. Carga limpia y validacion de las 256 salas contra Golden Vectors */
    CHECK(MjkAssets_LoadFromAssetsMemory(dat, dat_sz, &assets) == ASSET_OK, "LoadFromAssetsMemory OK");
    free(dat);

    int room_mismatches = 0;
    for (int r = 0; r < MJK_ROOM_COUNT; ++r) {
        const MjkRoomTemplate* rt = &assets.rooms[r];
        const GoldenRoomRef* ref = &k_golden_rooms[r];
        uint32_t mt_hash = fnv1a_32(rt->static_metatiles, sizeof(rt->static_metatiles));
        if (mt_hash != ref->pure_mt_hash ||
            rt->is_horizontal != (ref->is_horizontal != 0u) ||
            rt->is_bomb_room  != (ref->is_bomb_room != 0u) ||
            rt->palette_set   != ref->palette_set ||
            rt->music_index   != ref->music_id ||
            rt->section_id    != ref->section_id ||
            rt->object_count  != ref->object_count) {
            if (room_mismatches < 5) {
                fprintf(stderr, "[FAIL] Sala 0x%02X no coincide (mt=0x%08X vs 0x%08X, objs=%u vs %u)\n",
                        r, mt_hash, ref->pure_mt_hash, rt->object_count, ref->object_count);
            }
            room_mismatches++;
        }
    }
    CHECK(room_mismatches == 0, "Todas las 256 salas coinciden con Golden Vectors");

    /* 3. Tablas nativas decodificadas */
    CHECK(assets.jump_rate[0] > 0.9f && assets.jump_rate[0] < 1.0f, "jump_rate[0] neutro ~0.9375");
    CHECK(assets.jump_rate[1] > assets.jump_rate[0], "jump_rate[1] abajo > neutro");
    CHECK(assets.jump_rate[2] < assets.jump_rate[0], "jump_rate[2] arriba < neutro");
    CHECK(assets.score_values[SCORE_10] == 10u, "SCORE_10 == 10");
    CHECK(assets.score_values[SCORE_100] == 100u, "SCORE_100 == 100");
    CHECK(assets.score_values[SCORE_10000] == 10000u, "SCORE_10000 == 10000");
    CHECK(assets.mighty_a_presses[0] == 1u && assets.mighty_a_presses[1] == 20u &&
          assets.mighty_a_presses[2] == 30u && assets.mighty_a_presses[3] == 35u,
          "Mighty A presses {1,20,30,35}");
    CHECK(assets.strings[STR_PUSH_START_BUTTON].length > 0u, "Cadena PUSH START decodificada");
    CHECK(assets.strings[STR_NINTENDO_OF_AMERICA].length > 0u, "Cadena REV_US Nintendo decodificada");

    /* Verificar que los tiles pre-decodificados solo contienen indices 0..3 */
    bool tiles_valid = true;
    for (int t = 0; t < MJK_TILE_COUNT && tiles_valid; ++t) {
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                if (assets.tiles[t][y][x] > 3u) {
                    tiles_valid = false;
                }
            }
        }
    }
    CHECK(tiles_valid, "512 tiles pre-decodificados con indices validos 0..3");

    MjkAssets_Unload(&assets);
    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_assets fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_assets superado (SHA-1, BYOR v2, 256 salas y 512 tiles).\n");
    return 0;
}
