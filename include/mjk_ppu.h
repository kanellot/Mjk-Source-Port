/**
 * @file mjk_ppu.h
 * @brief Renderizador PPU 2C02 limpio en C11 (sin SDL ni flicker artificial de 8 sprites).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Modela la PPU NES 2C02 de Mighty Bomb Jack (`PPUCTRL = $90` en asm/src/prg.asm:11-12):
 * - Sprites 8x8 desde CHR Bank 1 `$0000` (bytes 0..4095)
 * - Fondo (BG) 8x8 desde CHR Bank 1 `$1000` (bytes 4096..8191)
 * - 2 nametables lógicas de 32x30 tiles + atributos por metatile 16x16
 * - 64 entradas OAM sin límite artificial de 8 sprites por scanline
 */

#ifndef MJK_PPU_H
#define MJK_PPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_build_info.h"
#include "mjk_types.h"

#define MJK_PPU_CHR_BANK_INDEX   1u
#define MJK_PPU_SPRITE_BASE      0x0000u
#define MJK_PPU_BG_BASE          0x1000u
#define MJK_PPU_NT_COLS          32u
#define MJK_PPU_NT_ROWS          30u
#define MJK_PPU_MT_COLS          16u
#define MJK_PPU_MT_ROWS          15u
#define MJK_PPU_OAM_COUNT        64u
#define MJK_PPU_PALETTE_SIZE     32u

typedef struct {
    uint8_t y;    /**< Posición Y en pantalla (>= 0xF0 oculta el sprite). */
    uint8_t tile; /**< Índice de tile 8x8 en patrón de sprites ($0000). */
    uint8_t attr; /**< Bits 0-1: paleta, bit 5: detrás de BG, bit 6: H-flip, bit 7: V-flip. */
    uint8_t x;    /**< Posición X en pantalla. */
} MjkOamEntry;

typedef struct {
    uint8_t     nt[2][MJK_PPU_NT_ROWS][MJK_PPU_NT_COLS];
    uint8_t     attr[2][MJK_PPU_MT_ROWS][MJK_PPU_MT_COLS];
    uint8_t     palette_ram[MJK_PPU_PALETTE_SIZE];
    MjkOamEntry oam[MJK_PPU_OAM_COUNT];
    uint16_t    scroll_x; /**< Scroll horizontal 0..511 (wrap sobre 2 nametables). */
    uint8_t     scroll_y; /**< Scroll vertical 0..239. */
} MjkPpu;

/** @brief Paleta maestra NTSC 2C02 de 64 colores en formato ARGB8888 (0xFFRRGGBB). */
MJK_CORE_API uint32_t MjkPpu_GetMasterColor(uint8_t nes_color_index);

MJK_CORE_API void MjkPpu_Init(MjkPpu* ppu, const MjkAssetPackage* pkg);
MJK_CORE_API void MjkPpu_RestoreDefaultPalettes(MjkPpu* ppu, const MjkAssetPackage* pkg);
MJK_CORE_API bool MjkPpu_LoadRoomPalette(MjkPpu* ppu, const MjkAssetPackage* pkg, uint8_t set_idx);
MJK_CORE_API void MjkPpu_SetJackMightyColor(MjkPpu* ppu, const MjkAssetPackage* pkg, uint8_t mighty_level);

MJK_CORE_API void MjkPpu_ClearNametable(MjkPpu* ppu, uint8_t fill_tile);
MJK_CORE_API void MjkPpu_ClearOam(MjkPpu* ppu);
MJK_CORE_API void MjkPpu_SetTile(MjkPpu* ppu, uint8_t nt_idx, uint8_t col, uint8_t row, uint8_t tile_id);
MJK_CORE_API void MjkPpu_SetMetatile(MjkPpu* ppu,
                                     const MjkAssetPackage* pkg,
                                     uint8_t nt_idx,
                                     uint8_t mx,
                                     uint8_t my,
                                     uint8_t metatile_id,
                                     uint8_t palette_attr);

MJK_CORE_API void MjkPpu_SetOam(MjkPpu* ppu,
                                uint8_t slot,
                                uint8_t x,
                                uint8_t y,
                                uint8_t tile,
                                uint8_t attr);

/**
 * @brief Escribe un metasprite 2x2 (16x16) desde `SpritesTable` ($C11C) a OAM.
 * @return Siguiente slot OAM libre tras escribir los 4 sprites (o 1 si bit 4 de attr_flags).
 */
MJK_CORE_API uint8_t MjkPpu_WriteMetasprite(MjkPpu* ppu,
                                            const MjkAssetPackage* pkg,
                                            uint8_t sprite_id,
                                            uint8_t x,
                                            uint8_t y,
                                            uint8_t attr_flags,
                                            uint8_t start_slot);

/** @brief Escribe una cadena de texto oficial REV_US en la nametable (WriteStringToPPU). */
MJK_CORE_API bool MjkPpu_WriteString(MjkPpu* ppu, const MjkAssetPackage* pkg, MjkStringId id);

/** @brief Dibuja la puntuación BCD de 8 dígitos en OAM (Y=16, X=88..144) con supresión de ceros. */
MJK_CORE_API uint8_t MjkPpu_DrawHudScore(MjkPpu* ppu, const uint8_t score_bcd[4], uint8_t start_slot);

/** @brief Dibuja el temporizador BCD ('T' + 2 dígitos) en OAM (Y=24, X=56..72). */
MJK_CORE_API uint8_t MjkPpu_DrawHudTimer(MjkPpu* ppu, uint8_t timer_bcd, uint8_t start_slot);

/** @brief Dibuja el contador Mighty ('M') o multiplicador ('x') en OAM (Y=24, X=184..192). */
MJK_CORE_API uint8_t MjkPpu_DrawHudMightyOrMult(MjkPpu* ppu,
                                                bool is_bomb_room,
                                                uint8_t value_0_9,
                                                uint8_t start_slot);

/** @brief Renderiza la escena completa (BG + OAM) al framebuffer 256x240 ARGB8888. */
MJK_CORE_API void MjkPpu_RenderFrame(const MjkPpu* ppu,
                                     const MjkAssetPackage* pkg,
                                     uint32_t* out_fb_argb8888);

#endif /* MJK_PPU_H */
