/**
 * @file raylib_renderer.c
 * @brief NES 2C02 PPU tile rendering, 2bpp CHR decoder, palette mapping,
 *        and OAM hardware sprite priority compositor for Mighty Bomb Jack.
 * 
 * Conforms to:
 *   - ADR-005: Raylib Presentation & 256x240 Virtual Canvas
 *   - RES-004: PPU Buffer & Palette Architecture
 */

#include "mbj/render.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * NES 2C02 MASTER PALETTE (64 RGB COLORS)
 * ============================================================================ */

const PaletteColor g_nes_palette[64] = {
    { 0x66, 0x66, 0x66 }, { 0x00, 0x2A, 0x88 }, { 0x14, 0x12, 0xA7 }, { 0x3B, 0x00, 0xA4 },
    { 0x5C, 0x00, 0x7E }, { 0x6E, 0x00, 0x40 }, { 0x6C, 0x06, 0x00 }, { 0x56, 0x1D, 0x00 },
    { 0x33, 0x35, 0x00 }, { 0x0B, 0x48, 0x00 }, { 0x00, 0x52, 0x00 }, { 0x00, 0x4F, 0x08 },
    { 0x00, 0x40, 0x4D }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 },

    { 0xAD, 0xAD, 0xAD }, { 0x15, 0x5F, 0xD9 }, { 0x42, 0x40, 0xFF }, { 0x75, 0x27, 0xFE },
    { 0xA0, 0x1A, 0xCC }, { 0xB7, 0x1E, 0x7B }, { 0xB5, 0x31, 0x20 }, { 0x99, 0x4E, 0x00 },
    { 0x6B, 0x6D, 0x00 }, { 0x38, 0x87, 0x00 }, { 0x0C, 0x93, 0x00 }, { 0x00, 0x8F, 0x32 },
    { 0x00, 0x7C, 0x8D }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 },

    { 0xFF, 0xFE, 0xFF }, { 0x64, 0xB0, 0xFF }, { 0x92, 0x90, 0xFF }, { 0xC6, 0x76, 0xFF },
    { 0xF3, 0x6A, 0xFF }, { 0xFE, 0x6E, 0xCC }, { 0xFE, 0x81, 0x70 }, { 0xEA, 0x9E, 0x22 },
    { 0xBC, 0xBE, 0x00 }, { 0x88, 0xD8, 0x00 }, { 0x5C, 0xE4, 0x30 }, { 0x45, 0xE0, 0x82 },
    { 0x48, 0xCD, 0xDE }, { 0x4F, 0x4F, 0x4F }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 },

    { 0xFF, 0xFE, 0xFF }, { 0xC0, 0xDF, 0xFF }, { 0xD3, 0xD2, 0xFF }, { 0xE8, 0xC8, 0xFF },
    { 0xFB, 0xC2, 0xFF }, { 0xFE, 0xC4, 0xEA }, { 0xFE, 0xCC, 0xC5 }, { 0xF7, 0xD8, 0xA5 },
    { 0xE4, 0xE5, 0x94 }, { 0xCF, 0xEF, 0x96 }, { 0xBD, 0xF4, 0xAB }, { 0xB3, 0xF3, 0xCC },
    { 0xB5, 0xEB, 0xF2 }, { 0xB8, 0xB8, 0xB8 }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 }
};

/* ============================================================================
 * 2BPP CHR PLANAR DECODER (16 BYTES PER 8x8 TILE)
 * ============================================================================ */

bool CHR_LoadFromMemory(const uint8_t *raw, size_t size, CHRBank *bank) {
    if (!raw || !bank || size < 16) return false;
    memset(bank, 0, sizeof(CHRBank));

    /* In US CNROM, Bank 1 (offset 8192) contains the active gameplay graphics */
    const uint8_t *src = raw;
    if (size >= 16384) {
        src = raw + 8192;
        size -= 8192;
    }

    size_t tile_count = size / 16;
    if (tile_count > CHR_TILE_COUNT) tile_count = CHR_TILE_COUNT;

    for (size_t t = 0; t < tile_count; t++) {
        const uint8_t *tile_data = &src[t * 16];
        for (int y = 0; y < 8; y++) {
            uint8_t plane0 = tile_data[y];
            uint8_t plane1 = tile_data[y + 8];
            for (int x = 0; x < 8; x++) {
                uint8_t shift = (uint8_t)(7 - x);
                uint8_t bit0 = (plane0 >> shift) & 1;
                uint8_t bit1 = (plane1 >> shift) & 1;
                bank->tiles[t].pixels[y][x] = (uint8_t)((bit1 << 1) | bit0);
            }
        }
    }
    return true;
}

bool CHR_LoadFromFile(const char *path, CHRBank *bank) {
    if (!path || !bank) return false;
    memset(bank, 0, sizeof(CHRBank));

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint8_t raw[32768];
    size_t read_bytes = fread(raw, 1, sizeof(raw), f);
    fclose(f);

    return CHR_LoadFromMemory(raw, read_bytes, bank);
}

/* ============================================================================
 * SOFTWARE BLITTER (8x8 TILE TO 256x240 CANVAS)
 * ============================================================================ */

void Renderer_BlitTile(uint32_t *dest, int dst_x, int dst_y,
                       const CHRBank *bank, uint16_t tile_id,
                       const uint8_t *subpalette,
                       bool flip_x, bool flip_y, bool transparent_color_0) {
    if (!dest || !bank || tile_id >= CHR_TILE_COUNT || !subpalette) return;

    const CHRTile *tile = &bank->tiles[tile_id];

    for (int y = 0; y < 8; y++) {
        int py = dst_y + (flip_y ? (7 - y) : y);
        if (py < 0 || py >= MBJ_NES_HEIGHT) continue;

        for (int x = 0; x < 8; x++) {
            int px = dst_x + (flip_x ? (7 - x) : x);
            if (px < 0 || px >= MBJ_NES_WIDTH) continue;

            uint8_t color_idx = tile->pixels[y][x];
            if (transparent_color_0 && color_idx == 0) continue;

            uint8_t nes_color = subpalette[color_idx] & 0x3F;
            PaletteColor rgb = g_nes_palette[nes_color];

            /* ABGR packed 32-bit pixel */
            dest[py * MBJ_NES_WIDTH + px] = (0xFFu << 24) |
                                            ((uint32_t)rgb.b << 16) |
                                            ((uint32_t)rgb.g << 8) |
                                            (uint32_t)rgb.r;
        }
    }
}

/* ============================================================================
 * NES OAM SPRITE COMPOSITOR (64 SPRITES, SPRITE 0 TOP PRIORITY)
 * ============================================================================ */

void Renderer_RenderOAM(uint32_t *dest, const CHRBank *bank,
                        const uint8_t oam[256], const uint8_t sprite_palettes[16]) {
    if (!dest || !bank || !oam || !sprite_palettes) return;

    /* Render backwards (63 down to 0) so sprite 0 draws over higher sprite indices */
    for (int i = 63; i >= 0; i--) {
        int spr_y = (int)oam[i * 4 + 0];
        uint8_t tile_id = oam[i * 4 + 1];
        uint8_t attr = oam[i * 4 + 2];
        int spr_x = (int)oam[i * 4 + 3];

        /* Off-screen sprite cull */
        if (spr_y >= 239 || spr_y == 0) continue;

        bool flip_h = (attr & 0x40) != 0;
        bool flip_v = (attr & 0x80) != 0;
        uint8_t subpal = (attr & 0x03);

        const uint8_t *subpalette = &sprite_palettes[subpal * 4];

        /* Sprites typically use Pattern Table 0 (tiles 0..255) */
        Renderer_BlitTile(dest, spr_x, spr_y, bank, tile_id, subpalette, flip_h, flip_v, true);
    }
}
