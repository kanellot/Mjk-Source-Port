/**
 * @file mjk_title.h
 * @brief Máquina de estados de Copyright USA y Title Screen (`TitleScreenHandler`).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Implementa el ciclo completo REV_US (asm/src/prg.asm:7473-7660):
 * Copyright USA (256f) <-> Title Screen (Sala 0, 512f) -> Jack Falling (Y >= $D8).
 */

#ifndef MJK_TITLE_H
#define MJK_TITLE_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_platform.h"
#include "mjk_ppu.h"
#include "mjk_types.h"

typedef struct {
    MjkTitleState state;
    uint16_t      timer;
    mjk_uq8_8_t   jack_x;
    mjk_uq8_8_t   jack_y;
    mjk_q8_8_t    jack_vx;
    mjk_q8_8_t    jack_vy;
    uint16_t      grav_index;
    uint8_t       jack_sprite;
} MjkTitleSystem;

MJK_CORE_API void MjkTitle_Init(MjkTitleSystem* ts);

/**
 * @brief Avanza 1 frame del ciclo Copyright/Title.
 * @return `true` cuando Jack termina de caer (Y >= $D8) y arranca el juego (`MJK_MODE_INGAME`).
 */
MJK_CORE_API bool MjkTitle_Step(MjkTitleSystem* ts,
                                const MjkInputState* input,
                                const MjkAssetPackage* pkg,
                                MjkPpu* ppu,
                                const uint8_t score_bcd[4],
                                uint8_t high_gdv_bcd);

#endif /* MJK_TITLE_H */
