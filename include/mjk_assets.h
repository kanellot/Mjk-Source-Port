/**
 * @file mjk_assets.h
 * @brief Cargador del paquete de assets empaquetados clean-room (assets.dat).
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FILOSOFÍA: BYOR (Bring Your Own ROM) - Cero assets con copyright en el ejecutable.
 */

#ifndef MJK_ASSETS_H
#define MJK_ASSETS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MJK_CHR_ROM_SIZE        32768
#define MJK_CHR_BANK_SIZE       8192
#define MJK_CHR_BANKS_COUNT     4
#define MJK_NES_TILE_SIZE       16    // 16 bytes por tile 8x8 planar 2bpp

typedef struct {
    uint8_t* raw_buffer;
    size_t   total_size;
    
    // Punteros internos a las secciones
    const uint8_t* chr_rom;        ///< 32.768 bytes de CHR-ROM (4 bancos de 8KB)
    size_t         chr_size;
    const uint8_t* active_chr_bank;///< Banco 1 activo (offset 8192, 512 tiles reales de NES)
    
    const uint8_t* audio_data;     ///< Secuencias sonoras y comandos chiptune
    size_t         audio_size;
    
    const uint8_t* text_data;      ///< Tablas de caracteres y diálogos
    size_t         text_size;
    
    bool is_loaded;
} MjkAssetPackage;

/**
 * @brief Carga y valida el archivo binario assets.dat.
 * @param path Ruta hacia el archivo (por defecto "assets/assets.dat").
 * @param package Estructura receptora de punteros.
 * @return true si el paquete es válido y se cargó correctamente.
 */
bool MjkAssets_Load(const char* path, MjkAssetPackage* package);

/**
 * @brief Libera los recursos de memoria del paquete de assets.
 */
void MjkAssets_Unload(MjkAssetPackage* package);

/**
 * @brief Decodifica un tile planar 2bpp de la NES a un buffer de 8x8 índices de paleta (0..3).
 * 
 * [CONCEPTO EDUCATIVO]:
 * Los tiles de NES de 8x8 píxeles se almacenan en 16 bytes:
 * - Bytes 0..7: Plano de bits bajo (Bit 0).
 * - Bytes 8..15: Plano de bits alto (Bit 1).
 * Combinando el bit bajo y el bit alto de cada fila se obtiene el índice de color (0, 1, 2 o 3).
 * 
 * @param tile_data Puntero a los 16 bytes del tile en CHR-ROM.
 * @param out_pixels Buffer de salida de 64 bytes (8x8 píxeles con valores 0 a 3).
 */
void MjkAssets_DecodeTile2bpp(const uint8_t* tile_data, uint8_t out_pixels[64]);

#endif // MJK_ASSETS_H
