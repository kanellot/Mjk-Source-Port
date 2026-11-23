/**
 * @file mjk_rom_extractor.c
 * @brief Extractor Clean-Room BYOR v2 en C11 (`mbj-us.nes` -> `assets/assets.dat`).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Verifica la cabecera iNES y el digest SHA-1 de la ROM legal del usuario antes
 * de extraer únicamente las secciones de datos necesarias (CHR-ROM, audio 2A03,
 * cadenas PPU, tablas globales y datos de las 257 salas), protegiendo el paquete
 * con un CRC-32 de payload e independencia de endianness del host.
 */

#include "mjk_rom_extractor.h"
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

MjkAssetError MjkRom_ValidateInesBuffer(const uint8_t* rom_data,
                                        size_t rom_size,
                                        char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]) {
    if (out_sha1_hex) {
        out_sha1_hex[0] = '\0';
    }
    if (!rom_data) {
        return MJK_ASSET_ERR_IO;
    }
    if (rom_size != MJK_ROM_EXPECTED_SIZE) {
        return MJK_ASSET_ERR_BAD_SIZE;
    }

    /* Cabecera iNES: "NES\x1A", 2 bancos PRG (32 KB), 4 bancos CHR (32 KB), Mapper 3 CNROM */
    if (rom_data[0] != 0x4Eu || rom_data[1] != 0x45u ||
        rom_data[2] != 0x53u || rom_data[3] != 0x1Au) {
        return MJK_ASSET_ERR_BAD_ROM_HEADER;
    }
    if (rom_data[4] != 2u || rom_data[5] != 4u) {
        return MJK_ASSET_ERR_BAD_ROM_HEADER;
    }
    uint8_t mapper = (uint8_t)((rom_data[6] >> 4) | (rom_data[7] & 0xF0u));
    if (mapper != 3u) {
        return MJK_ASSET_ERR_BAD_ROM_HEADER;
    }

    uint8_t digest[MJK_SHA1_DIGEST_SIZE];
    char hex[MJK_SHA1_HEX_LEN + 1u];
    MjkSha1_Compute(rom_data, rom_size, digest);
    MjkSha1_FormatHex(digest, hex);
    if (out_sha1_hex) {
        memcpy(out_sha1_hex, hex, sizeof(hex));
    }

    if (strcmp(hex, MJK_ROM_EXPECTED_SHA1) != 0) {
        return MJK_ASSET_ERR_BAD_ROM_SHA1;
    }

    return MJK_ASSET_OK;
}

MjkAssetError MjkRom_BuildAssetsV2Buffer(const uint8_t* rom_data,
                                         size_t rom_size,
                                         uint8_t** out_buf,
                                         size_t* out_size,
                                         char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]) {
    if (out_buf) {
        *out_buf = NULL;
    }
    if (out_size) {
        *out_size = 0u;
    }
    if (!out_buf || !out_size) {
        return MJK_ASSET_ERR_IO;
    }

    MjkAssetError val_err = MjkRom_ValidateInesBuffer(rom_data, rom_size, out_sha1_hex);
    if (val_err != MJK_ASSET_OK) {
        return val_err;
    }

    const uint32_t chr_offset    = MJK_ASSETS_HEADER_SIZE;
    const uint32_t audio_offset  = chr_offset + MJK_ROM_CHR_SIZE;
    const uint32_t text_offset   = audio_offset + MJK_AUDIO_SECTION_SIZE;
    const uint32_t tables_offset = text_offset + MJK_TEXT_SIZE;
    const uint32_t rooms_offset  = tables_offset + MJK_GAME_TABLES_SIZE;
    const uint32_t total_size    = rooms_offset + MJK_WORLD_ROOMS_SIZE;

    uint8_t* pkg = (uint8_t*)malloc(total_size);
    if (!pkg) {
        return MJK_ASSET_ERR_OUT_OF_MEMORY;
    }

    /* Copiar payload estructurado por secciones */
    memcpy(pkg + chr_offset,
           rom_data + MJK_ROM_CHR_OFFSET,
           MJK_ROM_CHR_SIZE);

    memcpy(pkg + audio_offset,
           rom_data + cpu_to_rom_offset(MJK_CPU_NOTE_FREQ_ADDR),
           MJK_NOTE_FREQ_SIZE);
    memcpy(pkg + audio_offset + MJK_NOTE_FREQ_SIZE,
           rom_data + cpu_to_rom_offset(MJK_CPU_MUSIC_DATA_ADDR),
           MJK_MUSIC_DATA_SIZE);

    memcpy(pkg + text_offset,
           rom_data + cpu_to_rom_offset(MJK_CPU_TEXT_ADDR),
           MJK_TEXT_SIZE);

    memcpy(pkg + tables_offset,
           rom_data + cpu_to_rom_offset(MJK_CPU_GAME_TABLES_ADDR),
           MJK_GAME_TABLES_SIZE);

    memcpy(pkg + rooms_offset,
           rom_data + cpu_to_rom_offset(MJK_CPU_WORLD_ROOMS_ADDR),
           MJK_WORLD_ROOMS_SIZE);

    uint32_t payload_crc = MjkCrc32_Compute(pkg + MJK_ASSETS_HEADER_SIZE,
                                            total_size - MJK_ASSETS_HEADER_SIZE);

    /* Serializar cabecera de 64 bytes en little-endian */
    memcpy(pkg + 0x00u, MJK_ASSETS_MAGIC, 8u);
    write_le32(pkg + 0x08u, MJK_ASSETS_VERSION_V2);
    write_le32(pkg + 0x0Cu, total_size);
    write_le32(pkg + 0x10u, payload_crc);
    write_le32(pkg + 0x14u, chr_offset);
    write_le32(pkg + 0x18u, MJK_ROM_CHR_SIZE);
    write_le32(pkg + 0x1Cu, audio_offset);
    write_le32(pkg + 0x20u, MJK_AUDIO_SECTION_SIZE);
    write_le32(pkg + 0x24u, text_offset);
    write_le32(pkg + 0x28u, MJK_TEXT_SIZE);
    write_le32(pkg + 0x2Cu, tables_offset);
    write_le32(pkg + 0x30u, MJK_GAME_TABLES_SIZE);
    write_le32(pkg + 0x34u, rooms_offset);
    write_le32(pkg + 0x38u, MJK_WORLD_ROOMS_SIZE);
    write_le32(pkg + 0x3Cu, 0u);

    *out_buf = pkg;
    *out_size = (size_t)total_size;
    return MJK_ASSET_OK;
}

MjkAssetError MjkRom_ExtractFileToAssetsDat(const char* rom_path,
                                            const char* out_assets_path,
                                            char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]) {
    if (out_sha1_hex) {
        out_sha1_hex[0] = '\0';
    }
    if (!rom_path || !out_assets_path) {
        return MJK_ASSET_ERR_IO;
    }

    FILE* in = fopen(rom_path, "rb");
    if (!in) {
        return MJK_ASSET_ERR_IO;
    }

    if (fseek(in, 0, SEEK_END) != 0) {
        fclose(in);
        return MJK_ASSET_ERR_IO;
    }
    long raw_len = ftell(in);
    if (raw_len < 0) {
        fclose(in);
        return MJK_ASSET_ERR_IO;
    }
    if (fseek(in, 0, SEEK_SET) != 0) {
        fclose(in);
        return MJK_ASSET_ERR_IO;
    }

    if ((size_t)raw_len != MJK_ROM_EXPECTED_SIZE) {
        fclose(in);
        return MJK_ASSET_ERR_BAD_SIZE;
    }

    uint8_t* rom_buf = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom_buf) {
        fclose(in);
        return MJK_ASSET_ERR_OUT_OF_MEMORY;
    }

    size_t read_bytes = fread(rom_buf, 1u, MJK_ROM_EXPECTED_SIZE, in);
    fclose(in);
    if (read_bytes != MJK_ROM_EXPECTED_SIZE) {
        free(rom_buf);
        return MJK_ASSET_ERR_IO;
    }

    uint8_t* out_pkg = NULL;
    size_t out_size = 0u;
    MjkAssetError err = MjkRom_BuildAssetsV2Buffer(rom_buf,
                                                   MJK_ROM_EXPECTED_SIZE,
                                                   &out_pkg,
                                                   &out_size,
                                                   out_sha1_hex);
    free(rom_buf);
    if (err != MJK_ASSET_OK) {
        return err;
    }

    FILE* out = fopen(out_assets_path, "wb");
    if (!out) {
        free(out_pkg);
        return MJK_ASSET_ERR_IO;
    }

    size_t written = fwrite(out_pkg, 1u, out_size, out);
    int close_rc = fclose(out);
    free(out_pkg);

    if (written != out_size || close_rc != 0) {
        return MJK_ASSET_ERR_IO;
    }

    return MJK_ASSET_OK;
}
