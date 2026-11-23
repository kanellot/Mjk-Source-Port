/**
 * @file mjk_ppu.c
 * @brief Implementación del PPU Ricoh 2C02, decodificador 2bpp y blitter nativo NES.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: Hardware 2C02 y research/audit/nes_ground_truth_spec.md
 */

#include "mjk_ppu.h"
#include <string.h>

// Tabla maestra canónica RGB de 64 colores de la NES 2C02 (NTSC PPU Ricoh)
const MjkNesRGB NES_2C02_Palette[64] = {
    {0x66, 0x66, 0x66}, {0x00, 0x2A, 0x88}, {0x14, 0x12, 0xA7}, {0x3B, 0x00, 0xA4}, // 00-03
    {0x5C, 0x00, 0x7E}, {0x6E, 0x00, 0x40}, {0x6C, 0x06, 0x00}, {0x56, 0x1D, 0x00}, // 04-07
    {0x33, 0x35, 0x00}, {0x0B, 0x48, 0x00}, {0x00, 0x52, 0x00}, {0x00, 0x4F, 0x08}, // 08-0B
    {0x00, 0x40, 0x4D}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, // 0C-0F
    {0xAD, 0xAD, 0xAD}, {0x15, 0x5F, 0xD9}, {0x42, 0x40, 0xFF}, {0x75, 0x27, 0xFE}, // 10-13
    {0xA0, 0x1A, 0xCC}, {0xB7, 0x1E, 0x7B}, {0xB5, 0x31, 0x20}, {0x99, 0x4E, 0x00}, // 14-17
    {0x6B, 0x6D, 0x00}, {0x38, 0x87, 0x00}, {0x0C, 0x93, 0x00}, {0x00, 0x8F, 0x32}, // 18-1B
    {0x00, 0x7C, 0x8D}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, // 1C-1F
    {0xFF, 0xFF, 0xFF}, {0x64, 0xB0, 0xFF}, {0x92, 0x90, 0xFF}, {0xC6, 0x76, 0xFF}, // 20-23
    {0xF3, 0x6A, 0xFF}, {0xFE, 0x6E, 0xCC}, {0xFE, 0x81, 0x70}, {0xEA, 0x9E, 0x22}, // 24-27
    {0xBC, 0xBE, 0x00}, {0x88, 0xD8, 0x00}, {0x5C, 0xE4, 0x30}, {0x45, 0xE0, 0x82}, // 28-2B
    {0x48, 0xCD, 0xDE}, {0x4F, 0x4F, 0x4F}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, // 2C-2F
    {0xFF, 0xFF, 0xFF}, {0xC0, 0xDF, 0xFF}, {0xD3, 0xD2, 0xFF}, {0xE8, 0xC8, 0xFF}, // 30-33
    {0xFB, 0xC2, 0xFF}, {0xFE, 0xC4, 0xEA}, {0xFE, 0xCC, 0xC5}, {0xF7, 0xD8, 0xA5}, // 34-37
    {0xE4, 0xE5, 0x94}, {0xCF, 0xEF, 0x96}, {0xBD, 0xF4, 0xAB}, {0xB3, 0xF3, 0xCC}, // 38-3B
    {0xB5, 0xEB, 0xF2}, {0xB8, 0xB8, 0xB8}, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}  // 3C-3F
};

uint32_t MjkPPU_ColorToARGB(uint8_t nes_color_index) {
    uint8_t idx = nes_color_index & 0x3F;
    MjkNesRGB c = NES_2C02_Palette[idx];
    return 0xFF000000u | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.b;
}

void MjkPPU_LoadPaletteARGB(const uint8_t nes_pal_32[32], uint32_t out_pal_argb[32]) {
    if (!nes_pal_32 || !out_pal_argb) return;
    for (int i = 0; i < 32; ++i) {
        out_pal_argb[i] = MjkPPU_ColorToARGB(nes_pal_32[i]);
    }
}

void MjkPPU_DecodeTile8x8(const uint8_t* tile_16b, uint8_t out_pixels[64]) {
    if (!tile_16b || !out_pixels) return;
    for (int y = 0; y < 8; ++y) {
        uint8_t p0 = tile_16b[y];
        uint8_t p1 = tile_16b[y + 8];
        for (int x = 0; x < 8; ++x) {
            int shift = 7 - x;
            uint8_t bit0 = (p0 >> shift) & 1;
            uint8_t bit1 = (p1 >> shift) & 1;
            out_pixels[y * 8 + x] = (uint8_t)((bit1 << 1) | bit0);
        }
    }
}

void MjkPPU_BlitTile(uint32_t* framebuffer, int dst_x, int dst_y,
                     const uint8_t* tile_chr, const uint32_t* subpalette_argb,
                     bool transparent_zero, bool flip_h, bool flip_v) {
    if (!framebuffer || !tile_chr || !subpalette_argb) return;
    if (dst_x <= -8 || dst_x >= MJK_SCREEN_WIDTH || dst_y <= -8 || dst_y >= MJK_SCREEN_HEIGHT) return;

    for (int y = 0; y < 8; ++y) {
        int src_y = flip_v ? (7 - y) : y;
        int py = dst_y + y;
        if (py < 0 || py >= MJK_SCREEN_HEIGHT) continue;

        uint8_t p0 = tile_chr[src_y];
        uint8_t p1 = tile_chr[src_y + 8];
        uint32_t* row_fb = &framebuffer[py * MJK_SCREEN_WIDTH];

        for (int x = 0; x < 8; ++x) {
            int px = dst_x + x;
            if (px < 0 || px >= MJK_SCREEN_WIDTH) continue;

            int src_x = flip_h ? (7 - x) : x;
            int shift = 7 - src_x;
            uint8_t bit0 = (p0 >> shift) & 1;
            uint8_t bit1 = (p1 >> shift) & 1;
            uint8_t color_idx = (uint8_t)((bit1 << 1) | bit0);

            if (transparent_zero && color_idx == 0) continue;
            row_fb[px] = subpalette_argb[color_idx];
        }
    }
}

void MjkPPU_BlitMetatile(uint32_t* framebuffer, int dst_x, int dst_y,
                         const uint8_t* chr_bank_512, uint8_t tl, uint8_t tr,
                         uint8_t bl, uint8_t br, const uint32_t* subpalette_argb) {
    if (!framebuffer || !chr_bank_512 || !subpalette_argb) return;
    // Background tiles are in Pattern Table 1 ($1000 = offset 4096 in Bank 1)
    const uint8_t* bg_table = chr_bank_512 + 4096;
    MjkPPU_BlitTile(framebuffer, dst_x,     dst_y,     bg_table + (size_t)tl * 16, subpalette_argb, false, false, false);
    MjkPPU_BlitTile(framebuffer, dst_x + 8, dst_y,     bg_table + (size_t)tr * 16, subpalette_argb, false, false, false);
    MjkPPU_BlitTile(framebuffer, dst_x,     dst_y + 8, bg_table + (size_t)bl * 16, subpalette_argb, false, false, false);
    MjkPPU_BlitTile(framebuffer, dst_x + 8, dst_y + 8, bg_table + (size_t)br * 16, subpalette_argb, false, false, false);
}

void MjkPPU_BlitSprite(uint32_t* framebuffer, const uint8_t* chr_bank_512,
                       const MjkOamSprite* sprite, const uint32_t sprite_palette_argb[16]) {
    if (!framebuffer || !chr_bank_512 || !sprite || !sprite_palette_argb) return;
    // Sprites with Y >= 240 or $F8 are offscreen
    if (sprite->y >= MJK_SCREEN_HEIGHT || sprite->y >= 0xF8) return;

    // Sprite Pattern Table 0: $0000 (offset 0 in Bank 1)
    const uint8_t* sprite_tile_data = chr_bank_512 + (size_t)sprite->tile_id * 16;
    uint8_t subpal = sprite->attributes & 0x03;
    const uint32_t* subpalette_argb = &sprite_palette_argb[subpal * 4];
    bool flip_h = (sprite->attributes & 0x40) != 0;
    bool flip_v = (sprite->attributes & 0x80) != 0;

    MjkPPU_BlitTile(framebuffer, (int)sprite->x, (int)sprite->y, sprite_tile_data,
                    subpalette_argb, true, flip_h, flip_v);
}
