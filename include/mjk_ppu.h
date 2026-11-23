/**
 * @file mjk_ppu.h
 * @brief Emulador del PPU Ricoh 2C02, decodificador 2bpp y blitter nativo NES.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: Hardware 2C02 y research/audit/nes_ground_truth_spec.md
 */

#ifndef MJK_PPU_H
#define MJK_PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MJK_SCREEN_WIDTH         256
#define MJK_SCREEN_HEIGHT        240
#define MJK_SCREEN_TOTAL_PIXELS  (MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} MjkNesRGB;

// Entrada individual OAM de 4 bytes (coincide con $0200 OAM DMA de NES)
typedef struct {
    uint8_t y;          ///< Coordenada Y en pantalla
    uint8_t tile_id;    ///< Índice de tile CHR (en Pattern Table 0: $0000)
    uint8_t attributes; ///< Bits 0-1: Paleta (0..3), Bit 5: Prioridad, Bit 6: H-Flip, Bit 7: V-Flip
    uint8_t x;          ///< Coordenada X en pantalla
} MjkOamSprite;

// Tabla maestra canónica RGB de 64 colores de la NES 2C02
extern const MjkNesRGB NES_2C02_Palette[64];

/**
 * @brief Convierte un índice de color NES ($00..$3F) a color empaquetado ARGB8888 (0xAARRGGBB).
 */
uint32_t MjkPPU_ColorToARGB(uint8_t nes_color_index);

/**
 * @brief Transforma un buffer de 32 bytes de paleta NES ($3F00-$3F1F) a 32 enteros ARGB8888.
 * @param nes_pal_32 Buffer de 32 bytes (16 BG + 16 Sprites).
 * @param out_pal_argb Buffer de salida de 32 valores ARGB8888.
 */
void MjkPPU_LoadPaletteARGB(const uint8_t nes_pal_32[32], uint32_t out_pal_argb[32]);

/**
 * @brief Decodifica un tile planar 2bpp (16 bytes) a un buffer de 64 píxeles (valores 0..3).
 * @param tile_16b Puntero a los 16 bytes del tile en CHR.
 * @param out_pixels Buffer receptor de 64 bytes (8x8 píxeles).
 */
void MjkPPU_DecodeTile8x8(const uint8_t* tile_16b, uint8_t out_pixels[64]);

/**
 * @brief Vuelca un tile de 8x8 al framebuffer 256x240 con soporte de subpaleta,
 *        transparencia de índice 0, clipping y volteo horizontal/vertical.
 * 
 * @param framebuffer Puntero al búfer de pantalla de 256x240 píxeles ARGB.
 * @param dst_x Coordenada horizontal izquierda en pantalla.
 * @param dst_y Coordenada vertical superior en pantalla.
 * @param tile_chr Puntero a los 16 bytes del tile en memoria CHR.
 * @param subpalette_argb Puntero a los 4 colores ARGB de la subpaleta activa.
 * @param transparent_zero Si es true, el índice 0 se ignora (comportamiento de sprites).
 *                         Si es false, el índice 0 se dibuja con subpalette_argb[0] (fondos).
 * @param flip_h Volteo horizontal del tile.
 * @param flip_v Volteo vertical del tile.
 */
void MjkPPU_BlitTile(uint32_t* framebuffer, int dst_x, int dst_y,
                     const uint8_t* tile_chr, const uint32_t* subpalette_argb,
                     bool transparent_zero, bool flip_h, bool flip_v);

/**
 * @brief Vuelca un metatile completo de 16x16 ({TL, TR, BL, BR}) al framebuffer.
 * 
 * @param framebuffer Búfer de pantalla ARGB de 256x240.
 * @param dst_x Coordenada horizontal (en píxeles, múltiplo de 16 o arbitrario).
 * @param dst_y Coordenada vertical (en píxeles, múltiplo de 16 o arbitrario).
 * @param chr_bank_512 Puntero al Banco 1 de CHR (8192 bytes con 512 tiles).
 * @param tl Índice de tile Top-Left en Background Pattern Table ($1000).
 * @param tr Índice de tile Top-Right en Background Pattern Table ($1000).
 * @param bl Índice de tile Bottom-Left en Background Pattern Table ($1000).
 * @param br Índice de tile Bottom-Right en Background Pattern Table ($1000).
 * @param subpalette_argb Puntero a 4 colores ARGB de la subpaleta del metatile.
 */
void MjkPPU_BlitMetatile(uint32_t* framebuffer, int dst_x, int dst_y,
                         const uint8_t* chr_bank_512, uint8_t tl, uint8_t tr,
                         uint8_t bl, uint8_t br, const uint32_t* subpalette_argb);

/**
 * @brief Vuelca un sprite individual OAM (8x8) al framebuffer aplicando sus atributos.
 * 
 * @param framebuffer Búfer de pantalla ARGB de 256x240.
 * @param chr_bank_512 Puntero al Banco 1 de CHR (8192 bytes con 512 tiles).
 * @param sprite Puntero a la estructura OAM del sprite.
 * @param sprite_palette_argb Paleta de 16 colores de sprites ARGB.
 */
void MjkPPU_BlitSprite(uint32_t* framebuffer, const uint8_t* chr_bank_512,
                       const MjkOamSprite* sprite, const uint32_t sprite_palette_argb[16]);

#ifdef __cplusplus
}
#endif

#endif // MJK_PPU_H
