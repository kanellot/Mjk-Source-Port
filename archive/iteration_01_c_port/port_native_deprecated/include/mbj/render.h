/**
 * @file render.h
 * @brief NES 2C02 PPU emulation rendering interface, CHR 2bpp decoding,
 *        palette lookup, and OAM sprite compositor for Mighty Bomb Jack (NES).
 * 
 * Conforms to:
 *   - ADR-005: Raylib Presentation, 256x240 Virtual Canvas & Fullscreen
 *   - RES-004: PPU Buffer & Palette Architecture
 */

#ifndef MBJ_RENDER_H
#define MBJ_RENDER_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHR_TILE_COUNT 512 /* 2 pattern tables x 256 tiles = 4096 bytes per 4KB bank */

typedef struct {
    uint8_t pixels[8][8]; /* 2bpp pixel index (0..3) */
} CHRTile;

typedef struct {
    CHRTile tiles[CHR_TILE_COUNT];
} CHRBank;

typedef struct {
    uint8_t r, g, b;
} PaletteColor;

/* NES 2C02 Master Palette (64 RGB colors) */
extern const PaletteColor g_nes_palette[64];

/* ============================================================================
 * CHR LOADING & TILE BLITTING APIS
 * ============================================================================ */

bool CHR_LoadFromFile(const char *path, CHRBank *bank);
bool CHR_LoadFromMemory(const uint8_t *raw_chr, size_t size, CHRBank *bank);

void Renderer_BlitTile(uint32_t *dest, int dst_x, int dst_y,
                       const CHRBank *bank, uint16_t tile_id,
                       const uint8_t *subpalette,
                       bool flip_x, bool flip_y, bool transparent_color_0);

void Renderer_RenderOAM(uint32_t *dest, const CHRBank *bank,
                        const uint8_t oam[256], const uint8_t sprite_palettes[16]);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_RENDER_H */
