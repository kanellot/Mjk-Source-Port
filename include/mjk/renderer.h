/**
 * @file renderer.h
 * @brief Renderizador solo-lectura 256x240 ARGB8888 optimizado (Ticket R-14).
 *
 * Toma un puntero `const MjkGame*` y `const MjkAssets*` y genera el framebuffer
 * `uint32_t[256 * 240]` sin modificar el estado del juego:
 *  - Usa los 512 tiles 8x8 pre-decodificados en `assets->tiles` (cero decodificacion
 *    de bitplanes 2bpp por pixel).
 *  - Pre-resuelve una cache plana de paletas ARGB8888 (`bg_pal[4][4]`, `spr_pal[4][4]`)
 *    una vez por frame para eliminar dobles indirecciones en el bucle de pixeles.
 */

#ifndef MJK_ENGINE_RENDERER_H
#define MJK_ENGINE_RENDERER_H

#include <stdint.h>
#include "mjk/assets.h"
#include "mjk/game.h"

/**
 * @brief Renderiza el frame actual del juego en `out_fb_argb8888` (256x240, 0xFFRRGGBB).
 */
void MjkRenderer_DrawFrame(const MjkGame* game,
                           const MjkAssets* assets,
                           uint32_t* out_fb_argb8888);

#endif /* MJK_ENGINE_RENDERER_H */
