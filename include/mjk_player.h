/**
 * @file mjk_player.h
 * @brief Cinemática Q8.8 de Jack, tabla de salto de 64 pasos, flutter y 9-probe (T-16, T-17).
 */

#ifndef MJK_PLAYER_H
#define MJK_PLAYER_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_platform.h"
#include "mjk_room_decoder.h"
#include "mjk_types.h"

#define MJK_CF_RIGHT_WALL 0x01u
#define MJK_CF_LEFT_WALL  0x02u
#define MJK_CF_CEILING    0x04u
#define MJK_CF_GROUND     0x08u

typedef struct {
    mjk_uq8_8_t pos_x;
    mjk_uq8_8_t pos_y;
    mjk_q8_8_t  vel_x;
    mjk_q8_8_t  vel_y;
    uint16_t    jump_accum;
    uint8_t     collision_flags;
    uint8_t     sprite_id;
    uint8_t     facing_attr; /**< 0x04 = derecha, 0x00 = izquierda. */
    uint8_t     walk_timer;
    bool        in_air;
    bool        just_jumped;
    bool        is_dying;
    bool        death_grounded; /**< Bit 5 ($20) de PlayerStruct (prg.asm:1326-1350). */
    uint8_t     death_timer;
    uint8_t     fire_tile_died_to;
    uint8_t     a_press_counter;
} MjkPlayer;

MJK_CORE_API void MjkPlayer_Init(MjkPlayer* p, uint8_t x, uint8_t y);

MJK_CORE_API MjkActionResult MjkPlayer_Update(MjkPlayer* p,
                                              const MjkInputState* input,
                                              const MjkAssetPackage* pkg,
                                              const MjkRoomState* cur_room,
                                              const MjkRoomState* right_room,
                                              uint8_t room_flags,
                                              uint16_t scroll_x,
                                              uint16_t scroll_y,
                                              bool in_torture,
                                              uint8_t* mighty_level,
                                              uint8_t* mighty_presses,
                                              uint8_t* mighty_coins,
                                              MjkSoundId* out_sfx,
                                              bool* out_add_10pts);

#endif /* MJK_PLAYER_H */
