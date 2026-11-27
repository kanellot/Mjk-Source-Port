/**
 * @file rom_loader.h
 * @brief Validacion criptografica (SHA-1 / CRC-32) y cargador Clean-Room BYOR v2.
 *
 * Esta es la frontera de entrada de archivos binarios del juego:
 *  - Valida la ROM legal del usuario (`mbj-us.nes`, 65.552 bytes, iNES Mapper 3 CNROM,
 *    SHA-1 `a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`).
 *  - Extrae y empaqueta `assets/assets.dat` v2 protegido por CRC-32.
 *  - Proporciona un helper para unit tests (`MjkRom_ResolveTestPath`) que distingue
 *    entre ROM ausente (CTest SKIP = 77) y ROM explicita invalida (CTest FAIL = 1).
 */

#ifndef MJK_ENGINE_ROM_LOADER_H
#define MJK_ENGINE_ROM_LOADER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MJK_SHA1_DIGEST_LEN       20u
#define MJK_SHA1_HEX_LEN          40u

#define MJK_ROM_EXPECTED_SIZE     65552u
#define MJK_ROM_PRG_OFFSET        16u
#define MJK_ROM_PRG_SIZE          32768u
#define MJK_ROM_CHR_OFFSET        32784u
#define MJK_ROM_CHR_SIZE          32768u
#define MJK_ROM_CHR_BANK_SIZE     8192u
#define MJK_ROM_EXPECTED_SHA1     "a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7"

/* Formato binario serializado little-endian de assets/assets.dat v2 */
#define MJK_ASSETS_MAGIC          "MJKASSET"
#define MJK_ASSETS_VERSION_V2     0x00020000u
#define MJK_ASSETS_HEADER_SIZE    64u

/* Secciones PRG contiguas extraidas de mbj-us.nes (REV_US) */
#define MJK_CPU_NOTE_FREQ_ADDR    0xB3FDu
#define MJK_NOTE_FREQ_SIZE        32u
#define MJK_CPU_GAME_TABLES_ADDR  0xBEAEu
#define MJK_GAME_TABLES_SIZE      852u
#define MJK_CPU_TEXT_ADDR         0xC202u
#define MJK_TEXT_SIZE             727u
#define MJK_CPU_WORLD_ROOMS_ADDR  0xC4D9u
#define MJK_WORLD_ROOMS_SIZE      11973u
#define MJK_CPU_MUSIC_DATA_ADDR   0xF39Eu
#define MJK_MUSIC_DATA_SIZE       2839u
#define MJK_AUDIO_SECTION_SIZE    (MJK_NOTE_FREQ_SIZE + MJK_MUSIC_DATA_SIZE)
#define MJK_ASSETS_PAYLOAD_SIZE   (MJK_ROM_CHR_SIZE + MJK_AUDIO_SECTION_SIZE + MJK_TEXT_SIZE + \
                                   MJK_GAME_TABLES_SIZE + MJK_WORLD_ROOMS_SIZE)
#define MJK_ASSETS_TOTAL_SIZE     (MJK_ASSETS_HEADER_SIZE + MJK_ASSETS_PAYLOAD_SIZE)

/** @brief Codigos de error de carga y validacion de ROM / assets.dat. */
typedef enum {
    ASSET_OK = 0,
    ASSET_ERR_IO,
    ASSET_ERR_BAD_MAGIC,
    ASSET_ERR_BAD_VERSION,
    ASSET_ERR_BAD_SIZE,
    ASSET_ERR_BAD_CRC,
    ASSET_ERR_BAD_ROM_HEADER,
    ASSET_ERR_BAD_ROM_SHA1,
    ASSET_ERR_OUT_OF_MEMORY
} MjkAssetError;

/** @brief Resultado de resolucion de ruta de ROM para unit tests. */
typedef enum {
    TEST_ROM_FOUND = 0,            /**< ROM encontrada y lista para cargar. */
    TEST_ROM_ABSENT_SKIP,          /**< No hay ROM local ni explicita: omitir test (codigo 77). */
    TEST_ROM_EXPLICIT_MISSING_FAIL /**< El usuario indico una ROM explicita pero no existe: fallar. */
} MjkTestRomStatus;

/* SHA-1 (RFC 3174) y CRC-32 IEEE 802.3 */
void     MjkHash_Sha1(const void* data, size_t len, uint8_t out_digest[MJK_SHA1_DIGEST_LEN]);
void     MjkHash_Sha1Hex(const uint8_t digest[MJK_SHA1_DIGEST_LEN], char out_hex[MJK_SHA1_HEX_LEN + 1u]);
uint32_t MjkHash_Crc32(const void* data, size_t len);

/* Validacion y extraccion BYOR v2 */
const char*   MjkRom_ErrorString(MjkAssetError err);
MjkAssetError MjkRom_ValidateInes(const uint8_t* rom_data,
                                  size_t rom_size,
                                  char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]);
MjkAssetError MjkRom_BuildAssetsDat(const uint8_t* rom_data,
                                    size_t rom_size,
                                    uint8_t** out_buf,
                                    size_t* out_size,
                                    char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]);
MjkAssetError MjkRom_ExtractFile(const char* rom_path,
                                 const char* out_assets_path,
                                 char out_sha1_hex[MJK_SHA1_HEX_LEN + 1u]);

/**
 * @brief Resuelve la ruta de la ROM para unit tests C11.
 * Orden de busqueda:
 *  1. Variable de entorno `MJK_ROM_PATH` (explicita: si falta, devuelve FAIL).
 *  2. `cmake_rom_path` si no esta vacio (explicita: si falta, devuelve FAIL).
 *  3. `<source_dir>/asm/bin/mbj-us.nes` (implicita: si falta, devuelve SKIP 77).
 */
MjkTestRomStatus MjkRom_ResolveTestPath(const char* source_dir,
                                        const char* cmake_rom_path,
                                        char* out_path,
                                        size_t out_path_size);

#endif /* MJK_ENGINE_ROM_LOADER_H */
