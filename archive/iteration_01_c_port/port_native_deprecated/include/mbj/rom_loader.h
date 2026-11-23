/**
 * @file rom_loader.h
 * @brief Universal NES ROM loader and asset extractor for Mighty Bomb Jack.
 * Reads iNES ROM (mbj-us.nes), extracts 32KB CHR planar tiles and 32KB PRG audio data.
 * The source code contains ZERO copyrighted assets.
 */

#ifndef MBJ_ROM_LOADER_H
#define MBJ_ROM_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/render.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MBJ_PRG_ROM_SIZE 32768
#define MBJ_CHR_ROM_SIZE 32768

/* Known 6502 CPU Addresses in PRG-ROM for REV_US */
#define MBJ_US_ADDR_NOTE_LENGTH 0xF39E
#define MBJ_US_ADDR_SOUND_VOL   0xF3BE
#define MBJ_US_ADDR_SOUND_PTRS  0xF45C

typedef struct MBJ_RomData {
    bool     loaded;
    uint8_t  prg_rom[MBJ_PRG_ROM_SIZE]; /* 32KB PRG ($8000-$FFFF) */
    uint8_t  chr_rom[MBJ_CHR_ROM_SIZE]; /* 32KB CHR */
    CHRBank  chr_bank;                  /* 512 8x8 tiles decoded */
} MBJ_RomData;

/**
 * @brief Attempts to locate and load the NES ROM file from candidate locations.
 * @param preferred_path Optional custom path passed by user (or NULL).
 * @param out_rom Target ROM structure to populate.
 * @return True if ROM was located, validated, and loaded successfully.
 */
bool ROM_LoadFromDisk(const char *preferred_path, MBJ_RomData *out_rom);

/**
 * @brief Converts a 6502 CPU memory address ($8000-$FFFF) to a pointer within PRG ROM.
 */
static inline const uint8_t* ROM_GetPRGPointer(const MBJ_RomData *rom, uint16_t addr_6502) {
    if (!rom || !rom->loaded || addr_6502 < 0x8000) return 0;
    uint32_t offset = (uint32_t)(addr_6502 - 0x8000);
    if (offset >= MBJ_PRG_ROM_SIZE) return 0;
    return &rom->prg_rom[offset];
}

#ifdef __cplusplus
}
#endif

#endif /* MBJ_ROM_LOADER_H */
