/**
 * @file test_assets.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para extractor BYOR v2 y cargador de assets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_hash.h"
#include "mjk_rom_extractor.h"

static int g_failures = 0;

static void write_le32(uint8_t* dst, uint32_t val) {
    dst[0] = (uint8_t)(val & 0xFFu);
    dst[1] = (uint8_t)((val >> 8) & 0xFFu);
    dst[2] = (uint8_t)((val >> 16) & 0xFFu);
    dst[3] = (uint8_t)((val >> 24) & 0xFFu);
}

static void build_synthetic_v2_header(uint8_t* buf) {
    memset(buf, 0, MJK_ASSETS_TOTAL_SIZE_V2);
    memcpy(buf, MJK_ASSETS_MAGIC, 8u);
    write_le32(buf + 0x08u, MJK_ASSETS_VERSION_V2);
    write_le32(buf + 0x0Cu, MJK_ASSETS_TOTAL_SIZE_V2);

    uint32_t off = MJK_ASSETS_HEADER_SIZE;
    write_le32(buf + 0x14u, off);
    write_le32(buf + 0x18u, MJK_ROM_CHR_SIZE);
    off += MJK_ROM_CHR_SIZE;

    write_le32(buf + 0x1Cu, off);
    write_le32(buf + 0x20u, MJK_AUDIO_SECTION_SIZE);
    off += MJK_AUDIO_SECTION_SIZE;

    write_le32(buf + 0x24u, off);
    write_le32(buf + 0x28u, MJK_TEXT_SIZE);
    off += MJK_TEXT_SIZE;

    write_le32(buf + 0x2Cu, off);
    write_le32(buf + 0x30u, MJK_GAME_TABLES_SIZE);
    off += MJK_GAME_TABLES_SIZE;

    write_le32(buf + 0x34u, off);
    write_le32(buf + 0x38u, MJK_WORLD_ROOMS_SIZE);

    uint32_t crc = MjkCrc32_Compute(buf + MJK_ASSETS_HEADER_SIZE, MJK_ASSETS_PAYLOAD_SIZE_V2);
    write_le32(buf + 0x10u, crc);
}

static void test_negative_without_rom(void) {
    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    uint8_t small_rom[32];
    memset(small_rom, 0, sizeof(small_rom));

    MjkAssetError err = MjkRom_ValidateInesBuffer(small_rom, sizeof(small_rom), sha1_hex);
    if (err != MJK_ASSET_ERR_BAD_SIZE) {
        fprintf(stderr, "[FAIL] ROM tamano erroneo devolvio %d (esperado %d)\n",
                (int)err, (int)MJK_ASSET_ERR_BAD_SIZE);
        g_failures++;
    }

    uint8_t* fake_rom = (uint8_t*)calloc(1u, MJK_ROM_EXPECTED_SIZE);
    if (!fake_rom) {
        fprintf(stderr, "[FAIL] Sin memoria para fake_rom\n");
        g_failures++;
        return;
    }

    err = MjkRom_ValidateInesBuffer(fake_rom, MJK_ROM_EXPECTED_SIZE, sha1_hex);
    if (err != MJK_ASSET_ERR_BAD_ROM_HEADER) {
        fprintf(stderr, "[FAIL] ROM sin cabecera iNES devolvio %d\n", (int)err);
        g_failures++;
    }

    fake_rom[0] = 'N';
    fake_rom[1] = 'E';
    fake_rom[2] = 'S';
    fake_rom[3] = 0x1Au;
    fake_rom[4] = 2u;
    fake_rom[5] = 4u;
    fake_rom[6] = 0x30u;
    err = MjkRom_ValidateInesBuffer(fake_rom, MJK_ROM_EXPECTED_SIZE, sha1_hex);
    if (err != MJK_ASSET_ERR_BAD_ROM_SHA1) {
        fprintf(stderr, "[FAIL] ROM falsa devolvio %d (esperado BAD_ROM_SHA1)\n", (int)err);
        g_failures++;
    }
    if (strlen(sha1_hex) != MJK_SHA1_HEX_LEN || strcmp(sha1_hex, MJK_ROM_EXPECTED_SHA1) == 0) {
        fprintf(stderr, "[FAIL] SHA-1 de ROM falsa invalido: %s\n", sha1_hex);
        g_failures++;
    }
    free(fake_rom);

    uint8_t* syn = (uint8_t*)malloc(MJK_ASSETS_TOTAL_SIZE_V2);
    if (!syn) {
        fprintf(stderr, "[FAIL] Sin memoria para buffer sintetico\n");
        g_failures++;
        return;
    }

    MjkAssetPackage pkg;
    build_synthetic_v2_header(syn);
    syn[0] = 'X';
    if (MjkAssets_LoadFromMemory(syn, MJK_ASSETS_TOTAL_SIZE_V2, &pkg) != MJK_ASSET_ERR_BAD_MAGIC) {
        fprintf(stderr, "[FAIL] Magic invalido no fue rechazado\n");
        g_failures++;
    }

    build_synthetic_v2_header(syn);
    write_le32(syn + 0x08u, 0x00010000u);
    if (MjkAssets_LoadFromMemory(syn, MJK_ASSETS_TOTAL_SIZE_V2, &pkg) != MJK_ASSET_ERR_BAD_VERSION) {
        fprintf(stderr, "[FAIL] Version v1 obsoleta no fue rechazada\n");
        g_failures++;
    }

    build_synthetic_v2_header(syn);
    syn[MJK_ASSETS_HEADER_SIZE + 10u] ^= 0x01u;
    if (MjkAssets_LoadFromMemory(syn, MJK_ASSETS_TOTAL_SIZE_V2, &pkg) != MJK_ASSET_ERR_BAD_CRC) {
        fprintf(stderr, "[FAIL] CRC-32 corrupto no fue rechazado\n");
        g_failures++;
    }

    free(syn);
}

static void test_with_local_rom(void) {
#ifdef MJK_SOURCE_DIR
    char rom_path[512];
    snprintf(rom_path, sizeof(rom_path), "%s/asm/bin/mbj-us.nes", MJK_SOURCE_DIR);
    FILE* f = fopen(rom_path, "rb");
    if (!f) {
        printf("[INFO] ROM local no encontrada en %s; saltando tests de extraccion real.\n", rom_path);
        return;
    }

    uint8_t* rom = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom) {
        fclose(f);
        fprintf(stderr, "[FAIL] Sin memoria para leer mbj-us.nes\n");
        g_failures++;
        return;
    }
    size_t got = fread(rom, 1u, MJK_ROM_EXPECTED_SIZE, f);
    fclose(f);
    if (got != MJK_ROM_EXPECTED_SIZE) {
        free(rom);
        fprintf(stderr, "[FAIL] Lectura incompleta de mbj-us.nes (%u B)\n", (unsigned)got);
        g_failures++;
        return;
    }

    uint8_t* v2_buf = NULL;
    size_t v2_size = 0u;
    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    MjkAssetError err = MjkRom_BuildAssetsV2Buffer(rom, MJK_ROM_EXPECTED_SIZE, &v2_buf, &v2_size, sha1_hex);
    free(rom);

    if (err != MJK_ASSET_OK || v2_size != 49255u || !v2_buf) {
        fprintf(stderr, "[FAIL] MjkRom_BuildAssetsV2Buffer fallo (err=%d, size=%u)\n",
                (int)err, (unsigned)v2_size);
        free(v2_buf);
        g_failures++;
        return;
    }

    /* Guardar y recargar desde scratch/test_assets_v2.dat */
    char out_dat_path[512];
    snprintf(out_dat_path, sizeof(out_dat_path), "%s/scratch/test_assets_v2.dat", MJK_SOURCE_DIR);
    FILE* out_f = fopen(out_dat_path, "wb");
    if (!out_f) {
        fprintf(stderr, "[FAIL] No se pudo escribir %s\n", out_dat_path);
        free(v2_buf);
        g_failures++;
        return;
    }
    size_t written = fwrite(v2_buf, 1u, v2_size, out_f);
    fclose(out_f);
    if (written != v2_size) {
        fprintf(stderr, "[FAIL] Escritura incompleta en %s\n", out_dat_path);
        remove(out_dat_path);
        free(v2_buf);
        g_failures++;
        return;
    }

    MjkAssetPackage pkg;
    err = MjkAssets_LoadFromFile(out_dat_path, &pkg);
    remove(out_dat_path);
    if (err != MJK_ASSET_OK) {
        fprintf(stderr, "[FAIL] MjkAssets_LoadFromFile(%s) fallo: %s\n",
                out_dat_path, MjkAssets_ErrorString(err));
        free(v2_buf);
        g_failures++;
        return;
    }

    /* Alterar 1 bit del payload -> MJK_ASSET_ERR_BAD_CRC */
    v2_buf[MJK_ASSETS_HEADER_SIZE + 123u] ^= 0x80u;
    MjkAssetPackage bad_pkg;
    err = MjkAssets_LoadFromMemory(v2_buf, v2_size, &bad_pkg);
    free(v2_buf);
    if (err != MJK_ASSET_ERR_BAD_CRC) {
        fprintf(stderr, "[FAIL] Bit corrupto en payload devolvio %d (esperado BAD_CRC)\n", (int)err);
        MjkAssets_Unload(&bad_pkg);
        MjkAssets_Unload(&pkg);
        g_failures++;
        return;
    }

    /* 4 bancos CHR no-cero (32768 B) */
    if (pkg.chr_size != MJK_ROM_CHR_SIZE) {
        fprintf(stderr, "[FAIL] chr_size=%u (esperado 32768)\n", (unsigned)pkg.chr_size);
        g_failures++;
    }
    for (uint8_t b = 0u; b < MJK_ROM_CHR_BANK_COUNT; ++b) {
        const uint8_t* bank = MjkAssets_GetChrBank(&pkg, b);
        if (!bank) {
            fprintf(stderr, "[FAIL] CHR bank %u NULL\n", (unsigned)b);
            g_failures++;
            continue;
        }
        bool any_nonzero = false;
        for (size_t i = 0u; i < MJK_ROM_CHR_BANK_SIZE; ++i) {
            if (bank[i] != 0u) {
                any_nonzero = true;
                break;
            }
        }
        if (!any_nonzero) {
            fprintf(stderr, "[FAIL] CHR bank %u completamente cero\n", (unsigned)b);
            g_failures++;
        }
    }

    /* 32 cadenas US (0x00..0x1F) */
    for (unsigned s = 0u; s < (unsigned)MJK_STR_COUNT; ++s) {
        uint16_t ppu_off = 0u;
        const uint8_t* tiles = NULL;
        size_t len = 0u;
        if (!MjkAssets_GetTextString(&pkg, (MjkStringId)s, &ppu_off, &tiles, &len) || !tiles || len == 0u) {
            fprintf(stderr, "[FAIL] Cadena US 0x%02X invalida\n", s);
            g_failures++;
        }
    }
    {
        uint16_t ppu_off = 0u;
        const uint8_t* tiles = NULL;
        size_t len = 0u;
        if (!MjkAssets_GetTextString(&pkg, MJK_STR_PUSH_START_BUTTON, &ppu_off, &tiles, &len) ||
            ppu_off != 0x01C8u || len != 17u) {
            fprintf(stderr, "[FAIL] PUSH_START_BUTTON ppu=0x%04X len=%u\n",
                    (unsigned)ppu_off, (unsigned)len);
            g_failures++;
        }
        if (!MjkAssets_GetTextString(&pkg, MJK_STR_NINTENDO_OF_AMERICA, &ppu_off, &tiles, &len) ||
            ppu_off != 0x0204u) {
            fprintf(stderr, "[FAIL] NINTENDO_OF_AMERICA ppu=0x%04X\n", (unsigned)ppu_off);
            g_failures++;
        }
    }

    /* 255 RoomDataPointers (Room 0 == NULL, Room 1 == 0xE10E, Room 255 == 0xF195) */
    uint16_t addr = 0u;
    if (MjkAssets_GetRoomDataPtr(&pkg, 0u, &addr) != NULL) {
        fprintf(stderr, "[FAIL] Room 0 deberia ser NULL\n");
        g_failures++;
    }
    for (unsigned r = 1u; r <= 255u; ++r) {
        const uint8_t* rptr = MjkAssets_GetRoomDataPtr(&pkg, (uint8_t)r, &addr);
        if (!rptr) {
            fprintf(stderr, "[FAIL] Room %u NULL\n", r);
            g_failures++;
        } else if (r == 1u && addr != 0xE10Eu) {
            fprintf(stderr, "[FAIL] Room 1 addr=0x%04X (esperado 0xE10E)\n", (unsigned)addr);
            g_failures++;
        } else if (r == 255u && addr != 0xF195u) {
            fprintf(stderr, "[FAIL] Room 255 addr=0x%04X (esperado 0xF195)\n", (unsigned)addr);
            g_failures++;
        }
    }

    /* MainPalette ($C038[0] == 0x21) */
    const uint8_t* pal = MjkAssets_ResolveCpuPtr(&pkg, MJK_ADDR_MAIN_PALETTE, 16u);
    if (!pal || pal[0] != 0x21u) {
        fprintf(stderr, "[FAIL] MainPalette[0]=0x%02X (esperado 0x21)\n", pal ? pal[0] : 0u);
        g_failures++;
    }

    /* NoteFrequencyTable ($B3FD == 0x0591) */
    uint16_t note0 = 0u;
    if (!MjkAssets_ReadCpuU16(&pkg, MJK_ADDR_NOTE_FREQ_TABLE, &note0) || note0 != 0x0591u) {
        fprintf(stderr, "[FAIL] NoteFrequencyTable[0]=0x%04X (esperado 0x0591)\n", (unsigned)note0);
        g_failures++;
    }

    /* 35 SoundPointers (1..0x23) */
    for (unsigned snd = 1u; snd <= (unsigned)MJK_MUS_COLLECTED_FULL_FAMILY; ++snd) {
        const uint8_t* sptr = MjkAssets_GetSoundDataPtr(&pkg, (MjkSoundId)snd, &addr);
        if (!sptr) {
            fprintf(stderr, "[FAIL] SoundId 0x%02X NULL\n", snd);
            g_failures++;
        }
    }

    MjkAssets_Unload(&pkg);
#endif
}

int main(void) {
    test_negative_without_rom();
    test_with_local_rom();

    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_assets: %d fallo(s).\n", g_failures);
        return 1;
    }

    printf("[OK] test_assets: extractor BYOR v2, CRC-32, 32 textos, 255 salas y 35 sonidos OK.\n");
    return 0;
}
