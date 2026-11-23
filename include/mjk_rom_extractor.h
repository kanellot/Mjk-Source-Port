/**
 * @file mjk_rom_extractor.h
 * @brief Extractor Clean-Room BYOR v2 integrado en C11 (`mbj-us.nes` -> `assets.dat`).
 */

#ifndef MJK_ROM_EXTRACTOR_H
#define MJK_ROM_EXTRACTOR_H

#include <stddef.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_build_info.h"
#include "mjk_hash.h"

MJK_CORE_API MjkAssetError MjkRom_ValidateInesBuffer(const uint8_t* rom_data,
                                                     size_t rom_size,
                                                     char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]);

MJK_CORE_API MjkAssetError MjkRom_BuildAssetsV2Buffer(const uint8_t* rom_data,
                                                      size_t rom_size,
                                                      uint8_t** out_buf,
                                                      size_t* out_size,
                                                      char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]);

MJK_CORE_API MjkAssetError MjkRom_ExtractFileToAssetsDat(const char* rom_path,
                                                         const char* out_assets_path,
                                                         char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]);

#endif /* MJK_ROM_EXTRACTOR_H */
