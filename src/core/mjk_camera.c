/**
 * @file mjk_camera.c
 * @brief Implementación de cámara y scroll de pasillos (RES-019, RES-033, RES-035).
 */

#include "mjk_camera.h"
#include <string.h>

static bool get_adj(const MjkAssetPackage* pkg, uint8_t room_id, MjkRoomAdjacency* out_adj) {
    const uint8_t* tbl = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_ADJACENT_ROOMS, 256u * 5u);
    if (!tbl || !out_adj) {
        return false;
    }
    const uint8_t* e = tbl + ((size_t)room_id * 5u);
    out_adj->flags = e[0];
    out_adj->up    = e[1];
    out_adj->down  = e[2];
    out_adj->right = e[3];
    out_adj->left  = e[4];
    return true;
}

void MjkCamera_InitForRoom(MjkCamera* cam,
                           uint8_t room_id,
                           const MjkRoomAdjacency* adj,
                           bool is_bomb_room) {
    if (!cam) {
        return;
    }
    memset(cam, 0, sizeof(*cam));
    cam->current_room_id = room_id;

    if (is_bomb_room || !adj || (adj->flags & 0x01u) != 0u) {
        cam->room_flags = (uint8_t)(MJK_RF_SINGLE_SCREEN | (is_bomb_room ? MJK_RF_BOMB_ROOM : 0u));
        return;
    }

    if ((adj->flags & 0x04u) == 0u) {
        cam->room_flags = 0u;
        if (adj->up == 0u || (adj->flags & 0x08u) != 0u) {
            cam->room_flags |= (uint8_t)(MJK_RF_AT_SCROLL_EDGE | MJK_RF_SECTION_START);
        }
        if ((adj->flags & 0x0Au) != 0x02u && adj->down != 0u) {
            cam->neighbor_room_id = adj->down;
        } else {
            cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
        }
        return;
    }

    cam->room_flags = (uint8_t)(MJK_RF_HORIZONTAL | MJK_RF_SCROLL_STOP);
    if (adj->left == 0u || (adj->flags & 0x08u) != 0u) {
        cam->room_flags |= (uint8_t)(MJK_RF_AT_SCROLL_EDGE | MJK_RF_SECTION_START);
    }
    if ((adj->flags & 0x0Au) != 0x02u && adj->right != 0u) {
        cam->neighbor_room_id = adj->right;
    } else {
        cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
    }
}

static void update_vertical_camera(MjkCamera* cam,
                                   MjkPlayer* player,
                                   const MjkAssetPackage* pkg,
                                   const MjkRoomAdjacency* adj) {
    uint8_t px = MjkUQ8_Int(player->pos_x);
    if (px < 8u) { player->pos_x = MjkUQ8_FromInt(8u); }
    if (px > 248u) { player->pos_x = MjkUQ8_FromInt(248u); }

    if ((cam->room_flags & MJK_RF_AT_SCROLL_EDGE) != 0u) {
        uint8_t py = MjkUQ8_Int(player->pos_y);
        if (cam->scroll_y_fixed == 0 && py >= 120u && player->vel_y > 0 && cam->neighbor_room_id != 0u) {
            cam->room_flags &= (uint8_t)~MJK_RF_AT_SCROLL_EDGE;
        } else if (py <= 120u && player->vel_y < 0 && adj->up != 0u && (adj->flags & 0x0Au) != 0x0Au) {
            cam->room_flags &= (uint8_t)~MJK_RF_AT_SCROLL_EDGE;
        } else {
            if (py < 8u) { player->pos_y = MjkUQ8_FromInt(8u); }
            if (py > 232u) { player->pos_y = MjkUQ8_FromInt(232u); }
            return;
        }
    }

    int32_t old_sy = cam->scroll_y_fixed >> 8;
    int32_t dy_fixed = (int32_t)player->pos_y - (120 << 8);
    if (dy_fixed == 0) {
        dy_fixed = (int32_t)player->vel_y;
    }
    int32_t next_sy = cam->scroll_y_fixed + dy_fixed;
    int32_t unwrapped_sy = next_sy;
    player->pos_y = MjkUQ8_FromInt(120u);

    if (next_sy >= 0xF000) {
        if (cam->neighbor_room_id != 0u) {
            cam->current_room_id = cam->neighbor_room_id;
            next_sy -= 0xF000;
            cam->room_crossed = true;
            MjkRoomAdjacency next_adj;
            if (get_adj(pkg, cam->current_room_id, &next_adj) &&
                (next_adj.flags & 0x0Au) != 0x02u && next_adj.down != 0u) {
                cam->neighbor_room_id = next_adj.down;
            } else {
                cam->neighbor_room_id = 0u;
                next_sy = 0;
                unwrapped_sy = 0xF000;
                cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
            }
        } else {
            next_sy = 0;
            unwrapped_sy = 0;
            cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
        }
    } else if (next_sy < 0) {
        if (adj->up != 0u && (adj->flags & 0x0Au) != 0x0Au) {
            cam->neighbor_room_id = cam->current_room_id;
            cam->current_room_id = adj->up;
            next_sy += 0xF000;
            cam->room_crossed = true;
            cam->room_flags &= (uint8_t)~MJK_RF_SECTION_START;
        } else {
            next_sy = 0;
            unwrapped_sy = 0;
            cam->room_flags |= (uint8_t)(MJK_RF_AT_SCROLL_EDGE | MJK_RF_SECTION_START);
        }
    }

    cam->scroll_y_fixed = next_sy;
    int32_t new_unwrapped_py = (unwrapped_sy >= 0)
        ? (unwrapped_sy >> 8)
        : -(((-unwrapped_sy) + 255) >> 8);
    cam->delta_cam_y = (int16_t)(old_sy - new_unwrapped_py);
}

void MjkCamera_Update(MjkCamera* cam,
                      MjkPlayer* player,
                      const MjkAssetPackage* pkg) {
    if (!cam || !player || !pkg) {
        return;
    }
    cam->delta_cam_x = 0;
    cam->delta_cam_y = 0;
    cam->room_crossed = false;

    if ((cam->room_flags & MJK_RF_SINGLE_SCREEN) != 0u) {
        uint8_t px = MjkUQ8_Int(player->pos_x);
        if (px < 8u) {
            player->pos_x = MjkUQ8_FromInt(8u);
        } else if (px > 248u) {
            player->pos_x = MjkUQ8_FromInt(248u);
        }
        return;
    }

    MjkRoomAdjacency adj;
    if (!get_adj(pkg, cam->current_room_id, &adj)) {
        return;
    }

    if ((cam->room_flags & MJK_RF_HORIZONTAL) == 0u && (adj.flags & 0x04u) == 0u) {
        update_vertical_camera(cam, player, pkg, &adj);
        return;
    }

    if ((cam->room_flags & MJK_RF_AT_SCROLL_EDGE) != 0u) {
        uint8_t px = MjkUQ8_Int(player->pos_x);
        if (cam->scroll_x_fixed == 0 && px >= 128u && player->vel_x > 0 && cam->neighbor_room_id != 0u) {
            cam->room_flags &= (uint8_t)0xF7u;
            player->pos_x = MjkUQ8_FromInt(128u);
        } else if (px <= 128u && player->vel_x < 0 && (adj.flags & 0x0Au) == 0x02u) {
            cam->room_flags &= (uint8_t)0xF7u;
            player->pos_x = MjkUQ8_FromInt(128u);
        } else {
            if (px < 8u) { player->pos_x = MjkUQ8_FromInt(8u); }
            if (px > 248u) { player->pos_x = MjkUQ8_FromInt(248u); }
            return;
        }
    }

    int32_t old_sx = cam->scroll_x_fixed >> 8;
    int32_t next_sx = cam->scroll_x_fixed + (int32_t)player->vel_x;
    int32_t unwrapped_sx = next_sx;
    player->pos_x = MjkUQ8_FromInt(128u);

    if (next_sx >= 0x10000) {
        if (cam->neighbor_room_id != 0u) {
            cam->current_room_id = cam->neighbor_room_id;
            next_sx -= 0x10000;
            cam->room_crossed = true;
            if (get_adj(pkg, cam->current_room_id, &adj) &&
                (adj.flags & 0x0Au) != 0x02u && adj.right != 0u) {
                cam->neighbor_room_id = adj.right;
            } else {
                cam->neighbor_room_id = 0u;
                next_sx = 0;
                unwrapped_sx = 0x10000;
                cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
            }
        } else {
            next_sx = 0;
            unwrapped_sx = 0;
            cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
        }
    } else if (next_sx < 0) {
        if (adj.left != 0u && (adj.flags & 0x0Au) != 0x0Au) {
            cam->neighbor_room_id = cam->current_room_id;
            cam->current_room_id = adj.left;
            next_sx += 0x10000;
            cam->room_crossed = true;
            cam->room_flags &= (uint8_t)0x7Fu;
        } else {
            next_sx = 0;
            unwrapped_sx = 0;
            cam->room_flags |= (uint8_t)(MJK_RF_AT_SCROLL_EDGE | MJK_RF_SECTION_START);
        }
    }

    cam->scroll_x_fixed = next_sx;
    int32_t new_unwrapped_px = (unwrapped_sx >= 0)
        ? (unwrapped_sx >> 8)
        : -(((-unwrapped_sx) + 255) >> 8);
    cam->delta_cam_x = (int16_t)(old_sx - new_unwrapped_px);
}
