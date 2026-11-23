/**
 * @file mjk_assets.c
 * @brief Cargador, validador CRC-32 y resolutor de punteros de `assets.dat` v2.
 */

#include "mjk_assets.h"
#include "mjk_hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_le32(const uint8_t* src) {
    return ((uint32_t)src[0])       |
           ((uint32_t)src[1] << 8)  |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

static uint16_t read_le16(const uint8_t* src) {
    return (uint16_t)(((uint16_t)src[0]) | ((uint16_t)src[1] << 8));
}

static bool range_valid(uint32_t offset, uint32_t len, uint32_t expected_len, size_t total_size) {
    if (len != expected_len) {
        return false;
    }
    if (offset < MJK_ASSETS_HEADER_SIZE) {
        return false;
    }
    if ((size_t)offset > total_size || (size_t)len > total_size - (size_t)offset) {
        return false;
    }
    return true;
}

MjkAssetError MjkAssets_LoadFromMemory(const uint8_t* data, size_t size, MjkAssetPackage* out_pkg) {
    if (!out_pkg) {
        return MJK_ASSET_ERR_IO;
    }
    memset(out_pkg, 0, sizeof(*out_pkg));

    if (!data || size < MJK_ASSETS_HEADER_SIZE) {
        return MJK_ASSET_ERR_BAD_SIZE;
    }
    if (memcmp(data, MJK_ASSETS_MAGIC, 8u) != 0) {
        return MJK_ASSET_ERR_BAD_MAGIC;
    }

    uint32_t version = read_le32(data + 0x08u);
    if (version != MJK_ASSETS_VERSION_V2) {
        return MJK_ASSET_ERR_BAD_VERSION;
    }

    uint32_t total_size = read_le32(data + 0x0Cu);
    if ((size_t)total_size != size || total_size < MJK_ASSETS_TOTAL_SIZE_V2) {
        return MJK_ASSET_ERR_BAD_SIZE;
    }

    uint32_t expected_crc  = read_le32(data + 0x10u);
    uint32_t chr_offset    = read_le32(data + 0x14u);
    uint32_t chr_size      = read_le32(data + 0x18u);
    uint32_t audio_offset  = read_le32(data + 0x1Cu);
    uint32_t audio_size    = read_le32(data + 0x20u);
    uint32_t text_offset   = read_le32(data + 0x24u);
    uint32_t text_size     = read_le32(data + 0x28u);
    uint32_t tables_offset = read_le32(data + 0x2Cu);
    uint32_t tables_size   = read_le32(data + 0x30u);
    uint32_t rooms_offset  = read_le32(data + 0x34u);
    uint32_t rooms_size    = read_le32(data + 0x38u);

    if (!range_valid(chr_offset, chr_size, MJK_ROM_CHR_SIZE, size) ||
        !range_valid(audio_offset, audio_size, MJK_AUDIO_SECTION_SIZE, size) ||
        !range_valid(text_offset, text_size, MJK_TEXT_SIZE, size) ||
        !range_valid(tables_offset, tables_size, MJK_GAME_TABLES_SIZE, size) ||
        !range_valid(rooms_offset, rooms_size, MJK_WORLD_ROOMS_SIZE, size)) {
        return MJK_ASSET_ERR_BAD_SIZE;
    }

    uint32_t actual_crc = MjkCrc32_Compute(data + MJK_ASSETS_HEADER_SIZE,
                                           size - MJK_ASSETS_HEADER_SIZE);
    if (actual_crc != expected_crc) {
        return MJK_ASSET_ERR_BAD_CRC;
    }

    uint8_t* copy = (uint8_t*)malloc(size);
    if (!copy) {
        return MJK_ASSET_ERR_OUT_OF_MEMORY;
    }
    memcpy(copy, data, size);

    out_pkg->raw_buffer       = copy;
    out_pkg->raw_size         = size;
    out_pkg->chr_rom          = copy + chr_offset;
    out_pkg->chr_size         = chr_size;
    out_pkg->note_freq_table  = copy + audio_offset;
    out_pkg->music_data       = copy + audio_offset + MJK_NOTE_FREQ_SIZE;
    out_pkg->music_size       = MJK_MUSIC_DATA_SIZE;
    out_pkg->text_data        = copy + text_offset;
    out_pkg->text_size        = text_size;
    out_pkg->game_tables      = copy + tables_offset;
    out_pkg->game_tables_size = tables_size;
    out_pkg->world_rooms      = copy + rooms_offset;
    out_pkg->world_rooms_size = rooms_size;
    out_pkg->payload_crc32    = expected_crc;
    return MJK_ASSET_OK;
}

MjkAssetError MjkAssets_LoadFromFile(const char* path, MjkAssetPackage* out_pkg) {
    if (!out_pkg) {
        return MJK_ASSET_ERR_IO;
    }
    memset(out_pkg, 0, sizeof(*out_pkg));
    if (!path) {
        return MJK_ASSET_ERR_IO;
    }

    FILE* f = fopen(path, "rb");
    if (!f) {
        return MJK_ASSET_ERR_IO;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return MJK_ASSET_ERR_IO;
    }
    long file_len = ftell(f);
    if (file_len <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return MJK_ASSET_ERR_IO;
    }

    size_t sz = (size_t)file_len;
    if (sz > (1024u * 1024u)) {
        fclose(f);
        return MJK_ASSET_ERR_BAD_SIZE;
    }

    uint8_t* buf = (uint8_t*)malloc(sz);
    if (!buf) {
        fclose(f);
        return MJK_ASSET_ERR_OUT_OF_MEMORY;
    }

    size_t got = fread(buf, 1u, sz, f);
    fclose(f);
    if (got != sz) {
        free(buf);
        return MJK_ASSET_ERR_IO;
    }

    MjkAssetError err = MjkAssets_LoadFromMemory(buf, sz, out_pkg);
    free(buf);
    return err;
}

void MjkAssets_Unload(MjkAssetPackage* pkg) {
    if (!pkg) {
        return;
    }
    free(pkg->raw_buffer);
    memset(pkg, 0, sizeof(*pkg));
}

const char* MjkAssets_ErrorString(MjkAssetError err) {
    switch (err) {
        case MJK_ASSET_OK:             return "OK";
        case MJK_ASSET_ERR_IO:         return "Error de E/S al abrir o escribir archivo";
        case MJK_ASSET_ERR_BAD_MAGIC:  return "Cabecera MJKASSET inválida";
        case MJK_ASSET_ERR_BAD_VERSION:return "Versión de assets.dat obsoleta o incompatible (se requiere v2)";
        case MJK_ASSET_ERR_BAD_SIZE:   return "Tamaño de archivo o sección incorrecto";
        case MJK_ASSET_ERR_BAD_CRC:    return "CRC-32 de payload corrupto en assets.dat";
        case MJK_ASSET_ERR_BAD_ROM_HEADER: return "Cabecera iNES incompatible (se requiere NES USA Mapper 3 CNROM)";
        case MJK_ASSET_ERR_BAD_ROM_SHA1:   return "El SHA-1 de la ROM no coincide con mbj-us.nes (REV_US)";
        case MJK_ASSET_ERR_OUT_OF_MEMORY:  return "Memoria insuficiente";
        default:                       return "Error desconocido";
    }
}

const uint8_t* MjkAssets_ResolveCpuPtr(const MjkAssetPackage* pkg,
                                       uint16_t cpu_addr,
                                       size_t min_bytes) {
    if (!pkg || !pkg->raw_buffer || min_bytes == 0u) {
        return NULL;
    }

    const struct {
        uint16_t       base_addr;
        size_t         len;
        const uint8_t* ptr;
    } regions[] = {
        { MJK_CPU_NOTE_FREQ_ADDR,   MJK_NOTE_FREQ_SIZE,   pkg->note_freq_table },
        { MJK_CPU_GAME_TABLES_ADDR, pkg->game_tables_size, pkg->game_tables },
        { MJK_CPU_TEXT_ADDR,        pkg->text_size,        pkg->text_data },
        { MJK_CPU_WORLD_ROOMS_ADDR, pkg->world_rooms_size, pkg->world_rooms },
        { MJK_CPU_MUSIC_DATA_ADDR,  pkg->music_size,       pkg->music_data }
    };

    for (size_t i = 0u; i < (sizeof(regions) / sizeof(regions[0])); ++i) {
        uint16_t base = regions[i].base_addr;
        size_t len = regions[i].len;
        if (cpu_addr >= base && regions[i].ptr) {
            size_t rel = (size_t)(cpu_addr - base);
            if (rel < len && min_bytes <= (len - rel)) {
                return regions[i].ptr + rel;
            }
        }
    }

    return NULL;
}

bool MjkAssets_ReadCpuU16(const MjkAssetPackage* pkg, uint16_t cpu_addr, uint16_t* out_val) {
    if (!out_val) {
        return false;
    }
    const uint8_t* ptr = MjkAssets_ResolveCpuPtr(pkg, cpu_addr, 2u);
    if (!ptr) {
        return false;
    }
    *out_val = read_le16(ptr);
    return true;
}

const uint8_t* MjkAssets_GetChrBank(const MjkAssetPackage* pkg, uint8_t bank_index) {
    if (!pkg || !pkg->chr_rom || bank_index >= MJK_ROM_CHR_BANK_COUNT) {
        return NULL;
    }
    size_t offset = (size_t)bank_index * MJK_ROM_CHR_BANK_SIZE;
    if (offset + MJK_ROM_CHR_BANK_SIZE > pkg->chr_size) {
        return NULL;
    }
    return pkg->chr_rom + offset;
}

bool MjkAssets_GetTextString(const MjkAssetPackage* pkg,
                             MjkStringId id,
                             uint16_t* out_ppu_offset,
                             const uint8_t** out_tiles,
                             size_t* out_len) {
    if (!pkg || (unsigned)id >= (unsigned)MJK_STR_COUNT) {
        return false;
    }
    uint16_t ptr_addr = (uint16_t)(MJK_ADDR_STRING_PTR_TABLE + ((uint16_t)id * 2u));
    uint16_t str_cpu_addr = 0u;
    if (!MjkAssets_ReadCpuU16(pkg, ptr_addr, &str_cpu_addr)) {
        return false;
    }

    /* Cada entrada tiene 2 bytes de offset PPU ($2000 + offset) + tiles hasta 0xFF */
    const uint8_t* str_ptr = MjkAssets_ResolveCpuPtr(pkg, str_cpu_addr, 3u);
    if (!str_ptr) {
        return false;
    }

    size_t offset_in_text = (size_t)(str_cpu_addr - MJK_CPU_TEXT_ADDR);
    size_t max_bytes = pkg->text_size - offset_in_text;
    size_t len = 0u;
    while ((2u + len) < max_bytes && str_ptr[2u + len] != 0xFFu) {
        len++;
    }
    if ((2u + len) >= max_bytes) {
        return false;
    }

    if (out_ppu_offset) {
        *out_ppu_offset = read_le16(str_ptr);
    }
    if (out_tiles) {
        *out_tiles = str_ptr + 2u;
    }
    if (out_len) {
        *out_len = len;
    }
    return true;
}

const uint8_t* MjkAssets_GetRoomDataPtr(const MjkAssetPackage* pkg,
                                        uint8_t room_id,
                                        uint16_t* out_cpu_addr) {
    if (out_cpu_addr) {
        *out_cpu_addr = 0u;
    }
    if (!pkg || room_id == 0u) {
        return NULL;
    }
    uint16_t entry_addr = (uint16_t)(MJK_ADDR_ROOM_DATA_PTRS + ((uint16_t)(room_id - 1u) * 2u));
    uint16_t target_addr = 0u;
    if (!MjkAssets_ReadCpuU16(pkg, entry_addr, &target_addr)) {
        return NULL;
    }
    if (out_cpu_addr) {
        *out_cpu_addr = target_addr;
    }
    return MjkAssets_ResolveCpuPtr(pkg, target_addr, 1u);
}

const uint8_t* MjkAssets_GetSoundDataPtr(const MjkAssetPackage* pkg,
                                         MjkSoundId sound_id,
                                         uint16_t* out_cpu_addr) {
    if (out_cpu_addr) {
        *out_cpu_addr = 0u;
    }
    if (!pkg || sound_id == MJK_SND_SILENCE || (unsigned)sound_id > (unsigned)MJK_MUS_COLLECTED_FULL_FAMILY) {
        return NULL;
    }
    uint16_t entry_addr = (uint16_t)(MJK_ADDR_SOUND_POINTERS + ((uint16_t)((unsigned)sound_id - 1u) * 2u));
    uint16_t target_addr = 0u;
    if (!MjkAssets_ReadCpuU16(pkg, entry_addr, &target_addr)) {
        return NULL;
    }
    if (out_cpu_addr) {
        *out_cpu_addr = target_addr;
    }
    return MjkAssets_ResolveCpuPtr(pkg, target_addr, 1u);
}
