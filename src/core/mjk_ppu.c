/**
 * @file mjk_ppu.c
 * @brief Renderizador PPU 2C02 limpio en C11 (paleta NTSC, 2bpp planar, BG + OAM).
 */

#include "mjk_ppu.h"
#include "mjk_platform.h"
#include <string.h>

/* Paleta maestra NTSC 2C02 de 64 colores en formato ARGB8888 (0xFFRRGGBB). */
static const uint32_t k_nes_palette_2c02[64] = {
    0xFF666666u, 0xFF002A88u, 0xFF1412A7u, 0xFF3B00A4u,
    0xFF5C007Eu, 0xFF6E0040u, 0xFF6C0600u, 0xFF561D00u,
    0xFF333500u, 0xFF0B4800u, 0xFF005200u, 0xFF004F08u,
    0xFF00404Du, 0xFF000000u, 0xFF000000u, 0xFF000000u,

    0xFFADADADu, 0xFF155FD9u, 0xFF4240FFu, 0xFF7527FEu,
    0xFFA01ACCu, 0xFFB71E7Bu, 0xFFB53120u, 0xFF994E00u,
    0xFF6B6D00u, 0xFF388700u, 0xFF0C9300u, 0xFF008F32u,
    0xFF007C8Du, 0xFF000000u, 0xFF000000u, 0xFF000000u,

    0xFFFFFEFFu, 0xFF64B0FFu, 0xFF9290FFu, 0xFFC676FFu,
    0xFFF36AFFu, 0xFFFE6ECCu, 0xFFFE8170u, 0xFFEA9E22u,
    0xFFBCBE00u, 0xFF88D800u, 0xFF5CE430u, 0xFF45E082u,
    0xFF48CDDEu, 0xFF4F4F4Fu, 0xFF000000u, 0xFF000000u,

    0xFFFFFEFFu, 0xFFC0DFFFu, 0xFFD3D2FFu, 0xFFE8C8FFu,
    0xFFFBC2FFu, 0xFFFEC4EAu, 0xFFFECCC5u, 0xFFF7D8A5u,
    0xFFE4E594u, 0xFFCFEF96u, 0xFFBDF4ABu, 0xFFB3F3CCu,
    0xFFB5EBF2u, 0xFFB8B8B8u, 0xFF000000u, 0xFF000000u
};

static uint8_t decode_chr_pixel(const uint8_t* bank1, uint16_t table_base,
                                uint8_t tile_id, uint8_t row, uint8_t col) {
    size_t off = (size_t)table_base + ((size_t)tile_id * 16u) + (size_t)(row & 7u);
    uint8_t p0 = bank1[off];
    uint8_t p1 = bank1[off + 8u];
    uint8_t bit = (uint8_t)(7u - (col & 7u));
    return (uint8_t)(((p0 >> bit) & 1u) | (((p1 >> bit) & 1u) << 1u));
}

uint32_t MjkPpu_GetMasterColor(uint8_t nes_color_index) {
    return k_nes_palette_2c02[nes_color_index & 0x3Fu];
}

void MjkPpu_Init(MjkPpu* ppu, const MjkAssetPackage* pkg) {
    if (!ppu) {
        return;
    }
    memset(ppu, 0, sizeof(*ppu));
    MjkPpu_ClearNametable(ppu, 0x24u);
    MjkPpu_ClearOam(ppu);
    if (pkg) {
        MjkPpu_RestoreDefaultPalettes(ppu, pkg);
    }
}

void MjkPpu_RestoreDefaultPalettes(MjkPpu* ppu, const MjkAssetPackage* pkg) {
    if (!ppu || !pkg) {
        return;
    }
    const uint8_t* pal = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_MAIN_PALETTE, MJK_PPU_PALETTE_SIZE);
    if (pal) {
        memcpy(ppu->palette_ram, pal, MJK_PPU_PALETTE_SIZE);
    }
}

bool MjkPpu_LoadRoomPalette(MjkPpu* ppu, const MjkAssetPackage* pkg, uint8_t set_idx) {
    if (!ppu || !pkg || set_idx >= 9u) {
        return false;
    }
    const uint8_t* sets = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_BG_PALETTE_SETS, 9u * 12u);
    if (!sets) {
        return false;
    }
    const uint8_t* src = sets + ((size_t)set_idx * 12u);
    memcpy(ppu->palette_ram + 4u, src, 12u);
    uint8_t backdrop = src[0];
    for (size_t i = 0u; i < MJK_PPU_PALETTE_SIZE; i += 4u) {
        ppu->palette_ram[i] = backdrop;
    }
    return true;
}

void MjkPpu_SetJackMightyColor(MjkPpu* ppu, const MjkAssetPackage* pkg, uint8_t mighty_level) {
    if (!ppu || !pkg) {
        return;
    }
    const uint8_t* colors = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_MIGHTY_COLORS, 4u);
    if (colors) {
        ppu->palette_ram[0x17u] = colors[mighty_level & 3u];
    }
}

void MjkPpu_ClearNametable(MjkPpu* ppu, uint8_t fill_tile) {
    if (!ppu) {
        return;
    }
    memset(ppu->nt, fill_tile, sizeof(ppu->nt));
    memset(ppu->attr, 0, sizeof(ppu->attr));
}

void MjkPpu_ClearOam(MjkPpu* ppu) {
    if (!ppu) {
        return;
    }
    for (size_t i = 0u; i < MJK_PPU_OAM_COUNT; ++i) {
        ppu->oam[i].y = 0xF8u;
        ppu->oam[i].tile = 0u;
        ppu->oam[i].attr = 0u;
        ppu->oam[i].x = 0u;
    }
}

void MjkPpu_SetTile(MjkPpu* ppu, uint8_t nt_idx, uint8_t col, uint8_t row, uint8_t tile_id) {
    if (!ppu || nt_idx >= 2u || col >= MJK_PPU_NT_COLS || row >= MJK_PPU_NT_ROWS) {
        return;
    }
    ppu->nt[nt_idx][row][col] = tile_id;
}

void MjkPpu_SetMetatile(MjkPpu* ppu,
                        const MjkAssetPackage* pkg,
                        uint8_t nt_idx,
                        uint8_t mx,
                        uint8_t my,
                        uint8_t metatile_id,
                        uint8_t palette_attr) {
    if (!ppu || !pkg || nt_idx >= 2u || mx >= MJK_PPU_MT_COLS || my >= MJK_PPU_MT_ROWS) {
        return;
    }
    const uint8_t* defs = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_METATILE_DEFS, 153u * 4u);
    if (!defs || metatile_id >= 153u) {
        return;
    }
    const uint8_t* m = defs + ((size_t)metatile_id * 4u);
    uint8_t col = (uint8_t)(mx * 2u);
    uint8_t row = (uint8_t)(my * 2u);
    ppu->nt[nt_idx][row][col]           = m[0];
    ppu->nt[nt_idx][row][col + 1u]      = m[1];
    ppu->nt[nt_idx][row + 1u][col]      = m[2];
    ppu->nt[nt_idx][row + 1u][col + 1u] = m[3];
    ppu->attr[nt_idx][my][mx]           = (uint8_t)(palette_attr & 3u);
}

void MjkPpu_SetOam(MjkPpu* ppu,
                   uint8_t slot,
                   uint8_t x,
                   uint8_t y,
                   uint8_t tile,
                   uint8_t attr) {
    if (!ppu || slot >= MJK_PPU_OAM_COUNT) {
        return;
    }
    ppu->oam[slot].y = y;
    ppu->oam[slot].tile = tile;
    ppu->oam[slot].attr = attr;
    ppu->oam[slot].x = x;
}

uint8_t MjkPpu_WriteMetasprite(MjkPpu* ppu,
                               const MjkAssetPackage* pkg,
                               uint8_t sprite_id,
                               uint8_t x,
                               uint8_t y,
                               uint8_t attr_flags,
                               uint8_t start_slot) {
    if (!ppu || !pkg || start_slot >= MJK_PPU_OAM_COUNT) {
        return start_slot;
    }

    if ((attr_flags & 0x10u) != 0u) {
        MjkPpu_SetOam(ppu, start_slot,
                      (uint8_t)(x - 4u), (uint8_t)(y - 4u),
                      sprite_id, (uint8_t)(attr_flags & 0xE3u));
        return (uint8_t)(start_slot + 1u);
    }

    const uint8_t* attr_tbl = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_SPRITE_ATTR_TABLE, 8u);
    const uint8_t* spr_tbl = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_SPRITES_TABLE,
                                                     ((size_t)sprite_id + 1u) * 5u);
    if (!attr_tbl || !spr_tbl) {
        return start_slot;
    }

    uint8_t idx = (uint8_t)((attr_flags & 0x0Cu) >> 1u);
    uint8_t flip_mask = attr_tbl[idx];
    uint8_t offset_mask = attr_tbl[idx + 1u];

    const uint8_t* entry = spr_tbl + ((size_t)sprite_id * 5u);
    uint8_t flip_bits = (uint8_t)(entry[0] ^ flip_mask);
    uint8_t base_attr = (uint8_t)(attr_flags & 0x23u);

    uint8_t slot = start_slot;
    for (uint8_t q = 0u; q < 4u && slot < MJK_PPU_OAM_COUNT; ++q) {
        uint8_t sy = (uint8_t)(y + ((offset_mask & 1u) ? (uint8_t)(-8) : 0u));
        offset_mask >>= 1u;
        uint8_t sx = (uint8_t)(x + ((offset_mask & 1u) ? (uint8_t)(-8) : 0u));
        offset_mask >>= 1u;

        uint8_t h_flip = (uint8_t)(flip_bits & 1u);
        uint8_t v_flip = (uint8_t)((flip_bits >> 1u) & 1u);
        flip_bits >>= 2u;

        uint8_t q_attr = (uint8_t)(base_attr | (h_flip << 6u) | (v_flip << 7u));
        MjkPpu_SetOam(ppu, slot++, sx, sy, entry[1u + q], q_attr);
    }
    return slot;
}

bool MjkPpu_WriteString(MjkPpu* ppu, const MjkAssetPackage* pkg, MjkStringId id) {
    if (!ppu || !pkg) {
        return false;
    }
    uint16_t ppu_offset = 0u;
    const uint8_t* tiles = NULL;
    size_t len = 0u;
    if (!MjkAssets_GetTextString(pkg, id, &ppu_offset, &tiles, &len)) {
        return false;
    }
    uint8_t nt_idx = (uint8_t)((ppu_offset >> 10u) & 1u);
    uint8_t row = (uint8_t)((ppu_offset >> 5u) & 31u);
    uint8_t col = (uint8_t)(ppu_offset & 31u);
    for (size_t i = 0u; i < len && (col + i) < MJK_PPU_NT_COLS; ++i) {
        if (row < MJK_PPU_NT_ROWS) {
            ppu->nt[nt_idx][row][col + i] = tiles[i];
        }
    }
    return true;
}

uint8_t MjkPpu_DrawHudScore(MjkPpu* ppu, const uint8_t score_bcd[4], uint8_t start_slot) {
    if (!ppu || !score_bcd) {
        return start_slot;
    }
    uint8_t slot = start_slot;
    uint8_t x = 0x58u; /* 88 */
    bool non_zero_seen = false;

    for (int i = 3; i >= 0; --i) {
        uint8_t byte_val = score_bcd[i];
        uint8_t nibbles[2] = {
            (uint8_t)((byte_val >> 4u) & 0x0Fu),
            (uint8_t)(byte_val & 0x0Fu)
        };
        for (int n = 0; n < 2; ++n) {
            uint8_t d = nibbles[n];
            bool is_units = (i == 0 && n == 1);
            uint8_t y = 0x10u; /* 16 */
            if (d != 0u || is_units || non_zero_seen) {
                non_zero_seen = true;
            } else {
                y = 0xF8u;
            }
            if (slot < MJK_PPU_OAM_COUNT) {
                MjkPpu_SetOam(ppu, slot++, x, y, d, 0u);
            }
            x = (uint8_t)(x + 8u);
        }
    }
    return slot;
}

uint8_t MjkPpu_DrawHudTimer(MjkPpu* ppu, uint8_t timer_bcd, uint8_t start_slot) {
    if (!ppu) {
        return start_slot;
    }
    uint8_t slot = start_slot;
    if (slot < MJK_PPU_OAM_COUNT) {
        MjkPpu_SetOam(ppu, slot++, 56u, 24u, 0x0Eu, 0u); /* 'T' */
    }
    uint8_t tens = (uint8_t)((timer_bcd >> 4u) & 0x0Fu);
    uint8_t ones = (uint8_t)(timer_bcd & 0x0Fu);
    if (slot < MJK_PPU_OAM_COUNT) {
        MjkPpu_SetOam(ppu, slot++, 64u, 24u, (tens == 0u) ? 0x0Fu : tens, 0u);
    }
    if (slot < MJK_PPU_OAM_COUNT) {
        MjkPpu_SetOam(ppu, slot++, 72u, 24u, ones, 0u);
    }
    return slot;
}

uint8_t MjkPpu_DrawHudMightyOrMult(MjkPpu* ppu,
                                    bool is_bomb_room,
                                    uint8_t value_0_9,
                                    uint8_t start_slot) {
    if (!ppu) {
        return start_slot;
    }
    uint8_t slot = start_slot;
    uint8_t icon = is_bomb_room ? 0x0Cu : 0x0Bu; /* 'x' o 'M' */
    uint8_t digit = (uint8_t)((value_0_9 % 10u) + (is_bomb_room ? 1u : 0u));
    if (slot < MJK_PPU_OAM_COUNT) {
        MjkPpu_SetOam(ppu, slot++, 184u, 24u, icon, 0u);
    }
    if (slot < MJK_PPU_OAM_COUNT) {
        MjkPpu_SetOam(ppu, slot++, 192u, 24u, digit, 0u);
    }
    return slot;
}

uint8_t MjkPpu_DrawBcdByte(MjkPpu* ppu,
                           uint8_t bcd_val,
                           uint8_t x,
                           uint8_t y,
                           bool hide_leading_zero,
                           uint8_t start_slot) {
    if (!ppu) {
        return start_slot;
    }
    uint8_t slot = start_slot;
    uint8_t tens = (uint8_t)((bcd_val >> 4u) & 0x0Fu);
    uint8_t ones = (uint8_t)(bcd_val & 0x0Fu);
    if (slot < MJK_PPU_OAM_COUNT) {
        uint8_t t_tile = (hide_leading_zero && tens == 0u) ? 0x0Fu : tens;
        MjkPpu_SetOam(ppu, slot++, (uint8_t)(x - 4u), (uint8_t)(y - 4u), t_tile, 0u);
    }
    if (slot < MJK_PPU_OAM_COUNT) {
        MjkPpu_SetOam(ppu, slot++, (uint8_t)(x + 4u), (uint8_t)(y - 4u), ones, 0u);
    }
    return slot;
}

void MjkPpu_RenderFrame(const MjkPpu* ppu,
                        const MjkAssetPackage* pkg,
                        uint32_t* out_fb_argb8888) {
    if (!ppu || !pkg || !out_fb_argb8888) {
        return;
    }
    const uint8_t* bank1 = MjkAssets_GetChrBank(pkg, MJK_PPU_CHR_BANK_INDEX);
    if (!bank1) {
        return;
    }

    const uint32_t backdrop_color = k_nes_palette_2c02[ppu->palette_ram[0] & 0x3Fu];
    uint8_t bg_opaque[MJK_SCREEN_HEIGHT][MJK_SCREEN_WIDTH];

    for (uint32_t y = 0u; y < (uint32_t)MJK_SCREEN_HEIGHT; ++y) {
        uint32_t wy = (y + (uint32_t)ppu->scroll_y) % 240u;
        uint8_t tile_row = (uint8_t)(wy >> 3u);
        uint8_t fine_y = (uint8_t)(wy & 7u);
        uint8_t my = (uint8_t)(tile_row >> 1u);

        for (uint32_t x = 0u; x < (uint32_t)MJK_SCREEN_WIDTH; ++x) {
            uint32_t wx = (x + (uint32_t)ppu->scroll_x) & 511u;
            uint8_t nt_idx = (uint8_t)((wx >> 8u) & 1u);
            uint8_t tile_col = (uint8_t)((wx >> 3u) & 31u);
            uint8_t fine_x = (uint8_t)(wx & 7u);
            uint8_t mx = (uint8_t)(tile_col >> 1u);

            uint8_t tile_id = ppu->nt[nt_idx][tile_row][tile_col];
            uint8_t px = decode_chr_pixel(bank1, MJK_PPU_BG_BASE, tile_id, fine_y, fine_x);

            uint32_t color = backdrop_color;
            if (px != 0u) {
                uint8_t pal_idx = (uint8_t)((ppu->attr[nt_idx][my][mx] & 3u) * 4u + px);
                color = k_nes_palette_2c02[ppu->palette_ram[pal_idx] & 0x3Fu];
            }
            bg_opaque[y][x] = (uint8_t)(px != 0u);
            out_fb_argb8888[y * (uint32_t)MJK_SCREEN_WIDTH + x] = color;
        }
    }

    /* OAM: prioridad por índice (0 delante), sin límite artificial de 8 por scanline. */
    for (int s = (int)MJK_PPU_OAM_COUNT - 1; s >= 0; --s) {
        const MjkOamEntry* e = &ppu->oam[s];
        if (e->y >= 0xF0u) {
            continue;
        }
        uint8_t pal_base = (uint8_t)(16u + ((e->attr & 3u) * 4u));
        bool behind_bg = ((e->attr & 0x20u) != 0u);
        bool h_flip = ((e->attr & 0x40u) != 0u);
        bool v_flip = ((e->attr & 0x80u) != 0u);

        for (uint8_t row = 0u; row < 8u; ++row) {
            uint32_t py = (uint32_t)e->y + 1u + (uint32_t)row;
            if (py >= (uint32_t)MJK_SCREEN_HEIGHT) {
                continue;
            }
            uint8_t src_row = v_flip ? (uint8_t)(7u - row) : row;
            for (uint8_t col = 0u; col < 8u; ++col) {
                uint32_t px_x = (uint32_t)e->x + (uint32_t)col;
                if (px_x >= (uint32_t)MJK_SCREEN_WIDTH) {
                    continue;
                }
                uint8_t src_col = h_flip ? (uint8_t)(7u - col) : col;
                uint8_t c = decode_chr_pixel(bank1, MJK_PPU_SPRITE_BASE, e->tile, src_row, src_col);
                if (c == 0u) {
                    continue;
                }
                if (behind_bg && bg_opaque[py][px_x]) {
                    continue;
                }
                uint8_t nes_c = ppu->palette_ram[pal_base + c] & 0x3Fu;
                out_fb_argb8888[py * (uint32_t)MJK_SCREEN_WIDTH + px_x] = k_nes_palette_2c02[nes_c];
            }
        }
    }
}
