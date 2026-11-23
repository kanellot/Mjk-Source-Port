/**
 * @file mjk_camera.h
 * @brief Scroll horizontal en pasillos, costuras/seams y clamping (T-18).
 */

#ifndef MJK_CAMERA_H
#define MJK_CAMERA_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_player.h"
#include "mjk_room_decoder.h"
#include "mjk_types.h"

typedef struct {
    uint8_t  room_flags;
    uint8_t  current_room_id;
    uint8_t  neighbor_room_id;
    int32_t  scroll_x_fixed; /**< Q24.8 dentro de [0, 256). */
    int32_t  scroll_y_fixed;
    int16_t  delta_cam_x;
    int16_t  delta_cam_y;
    bool     room_crossed;
} MjkCamera;

MJK_CORE_API void MjkCamera_InitForRoom(MjkCamera* cam,
                                        uint8_t room_id,
                                        const MjkRoomAdjacency* adj,
                                        bool is_bomb_room);

MJK_CORE_API void MjkCamera_Update(MjkCamera* cam,
                                   MjkPlayer* player,
                                   const MjkAssetPackage* pkg);

#endif /* MJK_CAMERA_H */
