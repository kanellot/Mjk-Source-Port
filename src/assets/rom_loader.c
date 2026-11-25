/**
 * @file rom_loader.c
 * @brief Extractor Clean-Room BYOR v2 (`mbj-us.nes` -> `assets/assets.dat`).
 */

#include "mjk/rom_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_le32(uint8_t* dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8)  & 0xFFu);
    dst[2] = (uint8_t)((v >> 16) & 0xFFu);
    dst[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static size_t cpu_to_rom_offset(uint16_t cpu_addr) {
    return (size_t)MJK_ROM_PRG_OFFSET + (size_t)(cpu_addr - 0x8000u);
}

static bool file_readable(const char* path) {
    if (!path || path[0] == '\0') {
        return false;
    }
    FILE* f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}

const char* MjkRom_ErrorString(MjkAssetError err) {
    switch (err) {
        case ASSET_OK:                 return "OK";
        case ASSET_ERR_IO:             return "Error de E/S al leer o escribir archivo";
        case ASSET_ERR_BAD_MAGIC:      return "Cabecera MJKASSET invalida";
        case ASSET_ERR_BAD_VERSION:    return "Version de assets.dat incompatible (se requiere v2)";
        case ASSET_ERR_BAD_SIZE:       return "Tamano de archivo o seccion incorrecto";
        case ASSET_ERR_BAD_CRC:        return "CRC-32 de payload corrupto en assets.dat";
        case ASSET_ERR_BAD_ROM_HEADER: return "Cabecera iNES incompatible (se requiere Mapper 3 CNROM)";
        case ASSET_ERR_BAD_ROM_SHA1:   return "El SHA-1 no coincide con mbj-us.nes (REV_US)";
        case ASSET_ERR_OUT_OF_MEMORY:  return "Memoria insuficiente";
        default:                       return "Error desconocido";
    }
}

MjkAssetError MjkRom_ValidateInes(const uint8_t* rom_data,
                                  size_t rom_size,
                                  char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]) {
    if (out_sha1_hex) {
        out_sha1_hex[0] = '\0';
    }
    if (!rom_data) {
        return ASSET_ERR_IO;
    }
    if (rom_size != MJK_ROM_EXPECTED_SIZE) {
        return ASSET_ERR_BAD_SIZE;
    }

    /* Cabecera iNES: "NES\x1A", 2 bancos PRG (32 KB), 4 bancos CHR (32 KB), Mapper 3 CNROM */
    if (rom_data[0] != 0x4Eu || rom_data[1] != 0x45u ||
        rom_data[2] != 0x53u || rom_data[3] != 0x1Au) {
        return ASSET_ERR_BAD_ROM_HEADER;
    }
    if (rom_data[4] != 2u || rom_data[5] != 4u) {
        return ASSET_ERR_BAD_ROM_HEADER;
    }
    uint8_t mapper = (uint8_t)((rom_data[6] >> 4) | (rom_data[7] & 0xF0u));
    if (mapper != 3u) {
        return ASSET_ERR_BAD_ROM_HEADER;
    }

    uint8_t digest[MJK_SHA1_DIGEST_LEN];
    char hex[MJK_SHA1_HEX_LEN + 1u];
    MjkHash_Sha1(rom_data, rom_size, digest);
    MjkHash_Sha1Hex(digest, hex);
    if (out_sha1_hex) {
        memcpy(out_sha1_hex, hex, sizeof(hex));
    }
    if (strcmp(hex, MJK_ROM_EXPECTED_SHA1) != 0) {
        return ASSET_ERR_BAD_ROM_SHA1;
    }
    return ASSET_OK;
}

MjkAssetError MjkRom_BuildAssetsDat(const uint8_t* rom_data,
                                    size_t rom_size,
                                    uint8_t** out_buf,
                                    size_t* out_size,
                                    char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]) {
    if (out_buf)  { *out_buf = NULL; }
    if (out_size) { *out_size = 0u; }
    if (!out_buf || !out_size) {
        return ASSET_ERR_IO;
    }

    MjkAssetError err = MjkRom_ValidateInes(rom_data, rom_size, out_sha1_hex);
    if (err != ASSET_OK) {
        return err;
    }

    const uint32_t chr_off    = MJK_ASSETS_HEADER_SIZE;
    const uint32_t audio_off  = chr_off + MJK_ROM_CHR_SIZE;
    const uint32_t text_off   = audio_off + MJK_AUDIO_SECTION_SIZE;
    const uint32_t tables_off = text_off + MJK_TEXT_SIZE;
    const uint32_t rooms_off  = tables_off + MJK_GAME_TABLES_SIZE;
    const uint32_t total_sz   = rooms_off + MJK_WORLD_ROOMS_SIZE;

    uint8_t* pkg = (uint8_t*)malloc(total_sz);
    if (!pkg) {
        return ASSET_ERR_OUT_OF_MEMORY;
    }

    memcpy(pkg + chr_off, rom_data + MJK_ROM_CHR_OFFSET, MJK_ROM_CHR_SIZE);
    memcpy(pkg + audio_off, rom_data + cpu_to_rom_offset(MJK_CPU_NOTE_FREQ_ADDR), MJK_NOTE_FREQ_SIZE);
    memcpy(pkg + audio_off + MJK_NOTE_FREQ_SIZE,
           rom_data + cpu_to_rom_offset(MJK_CPU_MUSIC_DATA_ADDR), MJK_MUSIC_DATA_SIZE);
    memcpy(pkg + text_off, rom_data + cpu_to_rom_offset(MJK_CPU_TEXT_ADDR), MJK_TEXT_SIZE);
    memcpy(pkg + tables_off, rom_data + cpu_to_rom_offset(MJK_CPU_GAME_TABLES_ADDR), MJK_GAME_TABLES_SIZE);
    memcpy(pkg + rooms_off, rom_data + cpu_to_rom_offset(MJK_CPU_WORLD_ROOMS_ADDR), MJK_WORLD_ROOMS_SIZE);

    uint32_t crc = MjkHash_Crc32(pkg + MJK_ASSETS_HEADER_SIZE, total_sz - MJK_ASSETS_HEADER_SIZE);

    memcpy(pkg + 0x00u, MJK_ASSETS_MAGIC, 8u);
    write_le32(pkg + 0x08u, MJK_ASSETS_VERSION_V2);
    write_le32(pkg + 0x0Cu, total_sz);
    write_le32(pkg + 0x10u, crc);
    write_le32(pkg + 0x14u, chr_off);
    write_le32(pkg + 0x18u, MJK_ROM_CHR_SIZE);
    write_le32(pkg + 0x1Cu, audio_off);
    write_le32(pkg + 0x20u, MJK_AUDIO_SECTION_SIZE);
    write_le32(pkg + 0x24u, text_off);
    write_le32(pkg + 0x28u, MJK_TEXT_SIZE);
    write_le32(pkg + 0x2Cu, tables_off);
    write_le32(pkg + 0x30u, MJK_GAME_TABLES_SIZE);
    write_le32(pkg + 0x34u, rooms_off);
    write_le32(pkg + 0x38u, MJK_WORLD_ROOMS_SIZE);
    write_le32(pkg + 0x3Cu, 0u);

    *out_buf = pkg;
    *out_size = (size_t)total_sz;
    return ASSET_OK;
}

MjkAssetError MjkRom_ExtractFile(const char* rom_path,
                                 const char* out_assets_path,
                                 char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]) {
    if (out_sha1_hex) {
        out_sha1_hex[0] = '\0';
    }
    if (!rom_path || !out_assets_path) {
        return ASSET_ERR_IO;
    }

    FILE* in = fopen(rom_path, "rb");
    if (!in) {
        return ASSET_ERR_IO;
    }
    if (fseek(in, 0, SEEK_END) != 0) {
        fclose(in);
        return ASSET_ERR_IO;
    }
    long raw_len = ftell(in);
    if (raw_len < 0 || fseek(in, 0, SEEK_SET) != 0) {
        fclose(in);
        return ASSET_ERR_IO;
    }
    if ((size_t)raw_len != MJK_ROM_EXPECTED_SIZE) {
        fclose(in);
        return ASSET_ERR_BAD_SIZE;
    }

    uint8_t* rom_buf = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom_buf) {
        fclose(in);
        return ASSET_ERR_OUT_OF_MEMORY;
    }
    size_t got = fread(rom_buf, 1u, MJK_ROM_EXPECTED_SIZE, in);
    fclose(in);
    if (got != MJK_ROM_EXPECTED_SIZE) {
        free(rom_buf);
        return ASSET_ERR_IO;
    }

    uint8_t* out_pkg = NULL;
    size_t out_size = 0u;
    MjkAssetError err = MjkRom_BuildAssetsDat(rom_buf, MJK_ROM_EXPECTED_SIZE,
                                              &out_pkg, &out_size, out_sha1_hex);
    free(rom_buf);
    if (err != ASSET_OK) {
        return err;
    }

    FILE* out = fopen(out_assets_path, "wb");
    if (!out) {
        free(out_pkg);
        return ASSET_ERR_IO;
    }
    size_t written = fwrite(out_pkg, 1u, out_size, out);
    int rc = fclose(out);
    free(out_pkg);
    return (written == out_size && rc == 0) ? ASSET_OK : ASSET_ERR_IO;
}

MjkTestRomStatus MjkRom_ResolveTestPath(const char* source_dir,
                                        const char* cmake_rom_path,
                                        char* out_path,
                                        size_t out_path_size) {
    if (!out_path || out_path_size == 0u) {
        return TEST_ROM_ABSENT_SKIP;
    }
    out_path[0] = '\0';

    const char* env_rom = getenv("MJK_ROM_PATH");
    if (env_rom && env_rom[0] != '\0') {
        snprintf(out_path, out_path_size, "%s", env_rom);
        return file_readable(out_path) ? TEST_ROM_FOUND : TEST_ROM_EXPLICIT_MISSING_FAIL;
    }
    if (cmake_rom_path && cmake_rom_path[0] != '\0') {
        snprintf(out_path, out_path_size, "%s", cmake_rom_path);
        return file_readable(out_path) ? TEST_ROM_FOUND : TEST_ROM_EXPLICIT_MISSING_FAIL;
    }
    if (source_dir && source_dir[0] != '\0') {
        snprintf(out_path, out_path_size, "%s/asm/bin/mbj-us.nes", source_dir);
        if (file_readable(out_path)) {
            return TEST_ROM_FOUND;
        }
    }
    out_path[0] = '\0';
    return TEST_ROM_ABSENT_SKIP;
}
