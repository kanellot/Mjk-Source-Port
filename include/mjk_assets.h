/**
 * @file mjk_assets.h
 * @brief Paquete binario Clean-Room BYOR v2 (`assets/assets.dat`) y acceso por CPU addr.
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Para garantizar cero bytes con copyright de Tecmo en el repositorio o el código C,
 * todos los gráficos CHR-ROM, secuencias de audio, textos PPU, paletas, metatiles
 * y salas se extraen en tiempo de ejecución desde `mbj-us.nes` hacia `assets.dat` v2.
 * Cada sección conserva su dirección base original del bus CPU ($8000-$FFFF).
 */

#ifndef MJK_ASSETS_H
#define MJK_ASSETS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk_build_info.h"
#include "mjk_types.h"

/* Perfil físico y criptográfico de la ROM canónica REV_US (mbj-us.nes) */
#define MJK_ROM_EXPECTED_SIZE     65552u
#define MJK_ROM_INES_HEADER_SIZE  16u
#define MJK_ROM_PRG_OFFSET        16u
#define MJK_ROM_PRG_SIZE          32768u
#define MJK_ROM_CHR_OFFSET        32784u /* 0x8010 */
#define MJK_ROM_CHR_SIZE          32768u
#define MJK_ROM_CHR_BANK_SIZE     8192u
#define MJK_ROM_CHR_BANK_COUNT    4u
#define MJK_ROM_EXPECTED_SHA1     "a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7"

/* Formato binario serializado little-endian de assets/assets.dat v2 */
#define MJK_ASSETS_MAGIC          "MJKASSET"
#define MJK_ASSETS_VERSION_V2     0x00020000u
#define MJK_ASSETS_HEADER_SIZE    64u

/* Bloques PRG contiguos verificados sobre asm/tools/tmp_test.mlb y REV_US */
#define MJK_CPU_NOTE_FREQ_ADDR    0xB3FDu
#define MJK_NOTE_FREQ_SIZE        32u
#define MJK_CPU_GAME_TABLES_ADDR  0xBEAEu
#define MJK_GAME_TABLES_SIZE      852u   /* $BEAE..$C201 */
#define MJK_CPU_TEXT_ADDR         0xC202u
#define MJK_TEXT_SIZE             727u   /* $C202..$C4D8 */
#define MJK_CPU_WORLD_ROOMS_ADDR  0xC4D9u
#define MJK_WORLD_ROOMS_SIZE      11973u /* $C4D9..$F39D */
#define MJK_CPU_MUSIC_DATA_ADDR   0xF39Eu
#define MJK_MUSIC_DATA_SIZE       2839u  /* $F39E..$FEB4 */
#define MJK_AUDIO_SECTION_SIZE    (MJK_NOTE_FREQ_SIZE + MJK_MUSIC_DATA_SIZE) /* 2871 B */
#define MJK_ASSETS_PAYLOAD_SIZE_V2 \
    (MJK_ROM_CHR_SIZE + MJK_AUDIO_SECTION_SIZE + MJK_TEXT_SIZE + \
     MJK_GAME_TABLES_SIZE + MJK_WORLD_ROOMS_SIZE) /* 49191 B */
#define MJK_ASSETS_TOTAL_SIZE_V2  (MJK_ASSETS_HEADER_SIZE + MJK_ASSETS_PAYLOAD_SIZE_V2)

/* Direcciones CPU simbólicas de tablas en PRG (asm/src/data.asm & music-*) */
#define MJK_ADDR_NOTE_FREQ_TABLE    0xB3FDu
#define MJK_ADDR_SPRITE_ANIM_TABLE  0xBEAEu
#define MJK_ADDR_ENEMY_INIT_STATUS  0xBF47u
#define MJK_ADDR_DIFFICULTY_TABLE   0xBF58u
#define MJK_ADDR_GAMEIDO_DIV_TABLE  0xBF98u
#define MJK_ADDR_ENEMY_SPEED_TABLE  0xBFA0u
#define MJK_ADDR_JUMP_TABLE_C0E3    0xBFB0u
#define MJK_ADDR_VEL_X_TABLE_C163   0xC030u
#define MJK_ADDR_MAIN_PALETTE       0xC038u
#define MJK_ADDR_BG_PALETTE_SETS    0xC058u
#define MJK_ADDR_MIGHTY_COLORS      0xC0C4u
#define MJK_ADDR_MIGHTY_A_PRESSES   0xC0C8u
#define MJK_ADDR_MUSIC_OPTIONS      0xC0CCu
#define MJK_ADDR_DOOR_ENTRY_XY      0xC0D4u
#define MJK_ADDR_VECTOR_TABLE       0xC0F4u
#define MJK_ADDR_SPRITES_TABLE      0xC11Cu
#define MJK_ADDR_STRING_PTR_TABLE   0xC202u
#define MJK_ADDR_SPRITE_ATTR_TABLE  0xC4D9u
#define MJK_ADDR_TILE_ATTR_TABLE    0xC4E1u
#define MJK_ADDR_SECTION_ROOMS_TBL  0xC509u
#define MJK_ADDR_FIRE_BOMB_BONUS    0xC646u
#define MJK_ADDR_MULTIPLIED_SCORE   0xC64Au
#define MJK_ADDR_SCORE_ADD_TABLE    0xC66Au
#define MJK_ADDR_ITEM_TO_TILE       0xC6AAu
#define MJK_ADDR_ROOM_HALF_SCREENS  0xC6CAu
#define MJK_ADDR_HALF_SCREEN_LAYTS  0xC8CAu
#define MJK_ADDR_LAYOUT_CHUNKS      0xD0EAu
#define MJK_ADDR_METATILE_DEFS      0xD7AAu
#define MJK_ADDR_ADJACENT_ROOMS     0xDA0Eu
#define MJK_ADDR_ENEMY_SPAWN_POS    0xDF0Eu
#define MJK_ADDR_ROOM_DATA_01       0xE10Eu
#define MJK_ADDR_ROOM_DATA_PTRS     0xF1A0u
#define MJK_ADDR_NOTE_LENGTH_TABLE  0xF39Eu
#define MJK_ADDR_SOUND_VOL_TABLE    0xF3BEu
#define MJK_ADDR_SOUND_POINTERS     0xF45Cu

typedef enum {
    MJK_ASSET_OK = 0,
    MJK_ASSET_ERR_IO,
    MJK_ASSET_ERR_BAD_MAGIC,
    MJK_ASSET_ERR_BAD_VERSION,
    MJK_ASSET_ERR_BAD_SIZE,
    MJK_ASSET_ERR_BAD_CRC,
    MJK_ASSET_ERR_BAD_ROM_HEADER,
    MJK_ASSET_ERR_BAD_ROM_SHA1,
    MJK_ASSET_ERR_OUT_OF_MEMORY
} MjkAssetError;

typedef struct {
    uint8_t*       raw_buffer;
    size_t         raw_size;
    const uint8_t* chr_rom;
    size_t         chr_size;
    const uint8_t* note_freq_table;
    const uint8_t* music_data;
    size_t         music_size;
    const uint8_t* text_data;
    size_t         text_size;
    const uint8_t* game_tables;
    size_t         game_tables_size;
    const uint8_t* world_rooms;
    size_t         world_rooms_size;
    uint32_t       payload_crc32;
} MjkAssetPackage;

MJK_CORE_API MjkAssetError MjkAssets_LoadFromMemory(const uint8_t* data,
                                                    size_t size,
                                                    MjkAssetPackage* out_pkg);
MJK_CORE_API MjkAssetError MjkAssets_LoadFromFile(const char* path,
                                                  MjkAssetPackage* out_pkg);
MJK_CORE_API void          MjkAssets_Unload(MjkAssetPackage* pkg);
MJK_CORE_API const char*   MjkAssets_ErrorString(MjkAssetError err);

MJK_CORE_API const uint8_t* MjkAssets_ResolveCpuPtr(const MjkAssetPackage* pkg,
                                                    uint16_t cpu_addr,
                                                    size_t min_bytes);
MJK_CORE_API bool MjkAssets_ReadCpuU16(const MjkAssetPackage* pkg,
                                       uint16_t cpu_addr,
                                       uint16_t* out_val);
MJK_CORE_API const uint8_t* MjkAssets_GetChrBank(const MjkAssetPackage* pkg,
                                                 uint8_t bank_index);
MJK_CORE_API bool MjkAssets_GetTextString(const MjkAssetPackage* pkg,
                                          MjkStringId id,
                                          uint16_t* out_ppu_offset,
                                          const uint8_t** out_tiles,
                                          size_t* out_len);
MJK_CORE_API const uint8_t* MjkAssets_GetRoomDataPtr(const MjkAssetPackage* pkg,
                                                     uint8_t room_id,
                                                     uint16_t* out_cpu_addr);
MJK_CORE_API const uint8_t* MjkAssets_GetSoundDataPtr(const MjkAssetPackage* pkg,
                                                      MjkSoundId sound_id,
                                                      uint16_t* out_cpu_addr);

#endif /* MJK_ASSETS_H */
