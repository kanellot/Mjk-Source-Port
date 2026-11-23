/**
 * @file rom_loader.c
 * @brief Universal NES ROM loader and asset extractor for Mighty Bomb Jack.
 * Reads iNES ROM (mbj-us.nes), extracts 32KB CHR planar tiles and 32KB PRG audio data.
 * Zero copyrighted assets in the source code.
 */

#include "mbj/rom_loader.h"
#include <stdio.h>
#include <string.h>

static const char *s_candidate_rom_paths[] = {
    "mbj-us.nes",
    "assets/mbj-us.nes",
    "assets/rom/mbj-us.nes",
    "../assets/mbj-us.nes",
    "../../assets/mbj-us.nes",
    "asm/bin/mbj-us.nes",
    "../asm/bin/mbj-us.nes",
    "../../asm/bin/mbj-us.nes",
    "../../../asm/bin/mbj-us.nes",
    NULL
};

static const char *s_candidate_chr_paths[] = {
    "assets/chr/mbj-us.chr",
    "../assets/chr/mbj-us.chr",
    "../../assets/chr/mbj-us.chr",
    "../../../port_native/assets/chr/mbj-us.chr",
    NULL
};

bool ROM_LoadFromDisk(const char *preferred_path, MBJ_RomData *out_rom) {
    if (!out_rom) return false;
    memset(out_rom, 0, sizeof(MBJ_RomData));

    FILE *f = NULL;
    const char *loaded_path = NULL;

    /* 1. Try preferred path first if given */
    if (preferred_path && preferred_path[0] != '\0') {
        f = fopen(preferred_path, "rb");
        if (f) {
            loaded_path = preferred_path;
        }
    }

    /* 2. Try candidate ROM paths */
    if (!f) {
        for (int i = 0; s_candidate_rom_paths[i] != NULL; ++i) {
            f = fopen(s_candidate_rom_paths[i], "rb");
            if (f) {
                loaded_path = s_candidate_rom_paths[i];
                break;
            }
        }
    }

    /* 3. If ROM file found, parse iNES header and extract PRG + CHR */
    if (f) {
        uint8_t header[16];
        if (fread(header, 1, 16, f) == 16) {
            /* Verify iNES magic: 'N', 'E', 'S', 0x1A */
            if (header[0] == 'N' && header[1] == 'E' && header[2] == 'S' && header[3] == 0x1A) {
                size_t prg_size = header[4] * 16384;
                size_t chr_size = header[5] * 8192;
                (void)chr_size;

                /* Read 32KB PRG ROM */
                size_t read_prg = fread(out_rom->prg_rom, 1, MBJ_PRG_ROM_SIZE, f);
                
                /* If PRG was larger than 32KB, skip extra PRG to reach CHR */
                if (prg_size > MBJ_PRG_ROM_SIZE) {
                    fseek(f, (long)(16 + prg_size), SEEK_SET);
                }

                /* Read 32KB CHR ROM */
                size_t read_chr = fread(out_rom->chr_rom, 1, MBJ_CHR_ROM_SIZE, f);
                fclose(f);

                if (read_prg > 0 && read_chr > 0) {
                    /* Decode all 512 8x8 CHR tiles */
                    CHR_LoadFromMemory(out_rom->chr_rom, read_chr, &out_rom->chr_bank);
                    out_rom->loaded = true;
                    printf("[ROM Loader] Successfully loaded ROM: %s (PRG: %zu KB, CHR: %zu KB)\n",
                           loaded_path, read_prg / 1024, read_chr / 1024);
                    return true;
                }
            } else {
                fclose(f);
            }
        } else {
            fclose(f);
        }
    }

    /* 4. Fallback: try loading extracted CHR file if ROM was not found */
    for (int i = 0; s_candidate_chr_paths[i] != NULL; ++i) {
        if (CHR_LoadFromFile(s_candidate_chr_paths[i], &out_rom->chr_bank)) {
            printf("[ROM Loader] Loaded graphics from external CHR file: %s\n", s_candidate_chr_paths[i]);
            out_rom->loaded = false; /* Graphics loaded, but no PRG audio */
            return true;
        }
    }

    printf("[ROM Loader] Warning: Could not find 'mbj-us.nes' or 'mbj-us.chr' in search paths.\n");
    return false;
}
