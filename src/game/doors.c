/**
 * @file doors.c
 * @brief Implementacion de las 33 condiciones de puertas, eventos Esfinge y cruce (R-10).
 */

#include "mjk/doors.h"
#include "mjk/game.h"
#include <math.h>

/* Tabla REV_US de apertura de puertas por indice de evento (0..31) */
static const uint8_t k_door_event_table_us[32] = {
    0x1Fu, 0x06u, 0x17u, 0x03u, 0x03u, 0x1Au, 0x03u, 0x1Fu,
    0x0Eu, 0x03u, 0x0Fu, 0x16u, 0x1Eu, 0x17u, 0x16u, 0x0Bu,
    0x03u, 0x1Eu, 0x1Du, 0x10u, 0x13u, 0x1Au, 0x11u, 0x19u,
    0x1Du, 0x1Eu, 0x0Au, 0x1Fu, 0x05u, 0x12u, 0x03u, 0x16u
};

static const MjkRoomInstance* active_room_for_player(const MjkGame* game) {
    if (game->world.has_neighbor) {
        int wx = (int)floorf(game->player.x) + (int)floorf(game->world.camera.scroll_x);
        int wy = (int)floorf(game->player.y) + (int)floorf(game->world.camera.scroll_y);
        if ((!game->world.current.is_horizontal && wy >= 240) ||
            (game->world.current.is_horizontal && wx >= 256)) {
            return &game->world.neighbor;
        }
    }
    return &game->world.current;
}

int MjkDoors_GetRoomCondition(const MjkGame* game) {
    if (!game || !game->assets) {
        return -1;
    }
    const MjkRoomInstance* room = active_room_for_player(game);
    if (room->room_id == 0x61u) {
        return DOOR_COND_ROOM_61_SPECIAL;
    }
    return (room->door_cond_raw == 0u) ? -1 : (int)(room->door_cond_raw - 1u);
}

bool MjkDoors_EvaluateCondition(MjkGame* game, uint8_t cond_idx) {
    if (!game) {
        return false;
    }
    switch (cond_idx) {
        case DOOR_COND_JUMP_TOUCH:
            return game->player.just_jumped;

        case DOOR_COND_MIGHTY3_A:
        case DOOR_COND_MIGHTY3_B:
        case DOOR_COND_MIGHTY3_C:
        case DOOR_COND_MIGHTY3_D:
        case DOOR_COND_MIGHTY3_E:
            return game->mighty_level == 3u;

        case DOOR_COND_FLAG_09:
            if ((game->door_flag_09 & 0x80u) == 0u && (game->door_flag_09 & 0x01u) != 0u) {
                game->door_flag_09 = 0x80u;
                return true;
            }
            return false;

        case DOOR_COND_BOMB_ORDER:
            if (game->objects.bomb_order == BOMB_ORDER_COMPLETED) {
                game->objects.bomb_order = BOMB_ORDER_NONE;
                return true;
            }
            return false;

        case DOOR_COND_ZERO_TIMER_A:
        case DOOR_COND_ZERO_TIMER_B:
            if ((game->world.camera.room_flags & ROOM_FLAG_AT_SCROLL_EDGE) != 0u &&
                game->mighty_coins == 0u && game->scoring.timer_seconds == 0 &&
                game->player.sprite_id == 6u) {
                game->scoring.timer_frozen = false;
                game->door_flag_09 = 1u;
                return true;
            }
            return false;

        case DOOR_COND_TWO_FIRE_DEATHS:
            if (game->door_1e_deaths == 2u && !game->player.is_dying) {
                game->door_1e_deaths = 0u;
                return true;
            }
            return false;

        case DOOR_COND_ROOM_61_SPECIAL:
            if (game->door_flag_1f == 0u) {
                game->door_flag_1f = 1u;
                return true;
            }
            return false;

        default:
            /* Condiciones reservadas / no-op en REV_US */
            return false;
    }
}

void MjkDoors_OpenInRoom(MjkGame* game, MjkRoomInstance* room, uint8_t door_obj_idx) {
    if (!game || !room || door_obj_idx >= room->object_count) {
        return;
    }
    MjkRoomObj* ro = &room->objects[door_obj_idx];
    if (ro->kind != OBJ_KIND_DOOR) {
        return;
    }
    ro->is_open          = true;
    ro->is_hidden        = false;
    ro->is_sphinx_locked = false;
    MjkWorld_SaveObjectState(&game->world, room, door_obj_idx);

    uint8_t closed_mt = 0u;
    MjkAssets_GetDoorLayout(ro->door_slot, NULL, NULL, NULL, NULL, &closed_mt, NULL);
    MjkWorld_SetVisualMetatile(room, game->assets, ro->grid_x, ro->grid_y, (uint8_t)(closed_mt + 2u));
    MjkWorld_SetVisualMetatile(room, game->assets, ro->grid_x2, ro->grid_y2, (uint8_t)(closed_mt + 3u));
}

static bool trigger_door_event(MjkGame* game, uint8_t event_idx, bool check_sphinx_pending) {
    if (!game || event_idx >= MJK_SPHINX_FLAG_COUNT) {
        return false;
    }
    if (check_sphinx_pending && game->world.persist.sphinx_opened[event_idx] != 1u) {
        return false;
    }
    game->world.persist.sphinx_opened[event_idx] = 2u;
    MjkGame_QueueSound(game, SOUND_DOOR);

    uint8_t val = k_door_event_table_us[event_idx];
    if ((val & 0x10u) == 0u) {
        return false;
    }

    uint8_t door_slot = (uint8_t)(val & 0x0Fu);
    bool in_nb = (active_room_for_player(game) == &game->world.neighbor);
    MjkRoomInstance* rooms[2] = {
        in_nb ? &game->world.neighbor : &game->world.current,
        in_nb ? &game->world.current : (game->world.has_neighbor ? &game->world.neighbor : NULL)
    };

    for (int r = 0; r < 2; ++r) {
        MjkRoomInstance* tr = rooms[r];
        if (!tr) { continue; }
        for (uint8_t i = 0u; i < tr->object_count; ++i) {
            if (tr->objects[i].kind == OBJ_KIND_DOOR && tr->objects[i].door_slot == door_slot) {
                MjkDoors_OpenInRoom(game, tr, i);
                game->entry_door_type = door_slot;
                game->state           = GS_ANIMATE_DOOR_OPEN;
                game->state_timer     = 72;
                game->sub_state       = 1u;
                return true;
            }
        }
    }
    return false;
}

static bool player_touching_door_for_cond(const MjkGame* game, uint8_t cond) {
    int wx = (int)floorf(game->player.x) + (int)floorf(game->world.camera.scroll_x);
    int wy = (int)floorf(game->player.y) + (int)floorf(game->world.camera.scroll_y);
    const int px[5] = { wx, wx - 8, wx + 8, wx, wx };
    const int py[5] = { wy, wy, wy, wy - 8, wy + 8 };

    for (int p = 0; p < 5; ++p) {
        int x = px[p] < 0 ? 0 : px[p];
        int y = py[p] < 0 ? 0 : py[p];
        const MjkRoomInstance* tr = &game->world.current;
        if (!game->world.current.is_horizontal && y >= 240 && game->world.has_neighbor) {
            tr = &game->world.neighbor;
            y -= 240;
        } else if (game->world.current.is_horizontal && x >= 256 && game->world.has_neighbor) {
            tr = &game->world.neighbor;
            x -= 256;
        }
        if (x >= 256) { x = 255; }
        if (y >= 240) { y = 239; }
        uint8_t gx = (uint8_t)(x >> 4);
        uint8_t gy = (uint8_t)(y >> 4);
        for (uint8_t i = 0u; i < tr->object_count; ++i) {
            const MjkRoomObj* ro = &tr->objects[i];
            if (ro->kind != OBJ_KIND_DOOR) { continue; }
            bool hit = (ro->grid_x == gx && ro->grid_y == gy) || (ro->grid_x2 == gx && ro->grid_y2 == gy);
            if (hit && (uint8_t)(ro->door_arg >> 3u) == cond) {
                return true;
            }
        }
    }
    return false;
}

bool MjkDoors_Update(MjkGame* game) {
    if (!game) {
        return false;
    }
    int cond = MjkDoors_GetRoomCondition(game);
    if (cond < 0 || !MjkDoors_EvaluateCondition(game, (uint8_t)cond)) {
        return false;
    }
    if ((cond == DOOR_COND_JUMP_TOUCH || cond == DOOR_COND_MIGHTY3_A ||
         cond == DOOR_COND_MIGHTY3_B  || cond == DOOR_COND_MIGHTY3_C ||
         cond == DOOR_COND_MIGHTY3_D  || cond == DOOR_COND_MIGHTY3_E) &&
        !player_touching_door_for_cond(game, (uint8_t)cond)) {
        return false;
    }
    return trigger_door_event(game, (uint8_t)cond, true);
}

void MjkDoors_HandleSphinxEvent(MjkGame* game, uint8_t event_idx) {
    (void)trigger_door_event(game, event_idx, true);
}

bool MjkDoors_TriggerEventDirect(MjkGame* game, uint8_t event_idx) {
    return trigger_door_event(game, event_idx, false);
}

MjkActionResult MjkDoors_CheckPlayerEntry(MjkGame* game) {
    if (!game) {
        return ACTION_CONTINUE;
    }
    int wx = (int)floorf(game->player.x) + (int)floorf(game->world.camera.scroll_x);
    int wy = (int)floorf(game->player.y) + (int)floorf(game->world.camera.scroll_y);

    int vy = wy + ((game->player.vy < 0.0f) ? -9 : (game->player.vy > 0.0f) ? 9 : 0);
    int hx = wx + ((game->player.facing == FACING_RIGHT) ? 10 : -10);
    const int px[3] = { wx, wx, hx };
    const int py[3] = { wy, vy, wy };

    for (int p = 0; p < 3; ++p) {
        int x = px[p] < 0 ? 0 : px[p];
        int y = py[p] < 0 ? 0 : py[p];
        MjkRoomInstance* tr = &game->world.current;
        if (!game->world.current.is_horizontal && y >= 240 && game->world.has_neighbor) {
            tr = &game->world.neighbor;
            y -= 240;
        } else if (x >= 256 && game->world.has_neighbor) {
            tr = &game->world.neighbor;
            x -= 256;
        }
        if (x >= 256) { x = 255; }
        if (y >= 240) { y = 239; }
        uint8_t gx = (uint8_t)(x >> 4);
        uint8_t gy = (uint8_t)(y >> 4);

        for (uint8_t i = 0u; i < tr->object_count; ++i) {
            MjkRoomObj* ro = &tr->objects[i];
            if (ro->kind != OBJ_KIND_DOOR || !ro->is_open || ro->is_hidden) {
                continue;
            }
            if ((ro->grid_x == gx && ro->grid_y == gy) || (ro->grid_x2 == gx && ro->grid_y2 == gy)) {
                if ((ro->door_arg & 1u) == 0u) {
                    ro->is_open = false;
                    MjkWorld_SaveObjectState(&game->world, tr, i);
                }
                MjkGame_QueueSound(game, SOUND_SILENCE);
                game->exit_source_room_id = tr->room_id;
                game->entry_door_type     = ro->door_slot;
                game->state               = GS_EXIT_ROOM;
                game->sub_state           = 0u;
                return ACTION_ROOM_CHANGED;
            }
        }
    }
    return ACTION_CONTINUE;
}
