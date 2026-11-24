/**
 * @file mjk_doors.c
 * @brief Implementación de las 33 condiciones de puertas REV_US (asm/src/prg.asm:5312-5368, 6696-7091).
 *
 * REGLA ANTI-BUG (RES-029): las puertas existen solo donde el bytecode RoomData declara
 * `$80..$8F`; nunca se inventan desde AdjacentRoomsTable ni se estampan tiles crudos 0x94..0x9B.
 */

#include "mjk_doors.h"
#include "mjk_game.h"

int MjkDoors_RoomConditionIndex(const MjkGame* game) {
    if (!game || !game->pkg) {
        return -1;
    }
    if (game->current_room_id == 0x61u) {
        return (int)MJK_DOOR_COND_R11_TO_R6;
    }
    const uint8_t* rdata = MjkAssets_GetRoomDataPtr(game->pkg, game->current_room_id, NULL);
    if (!rdata || (rdata[0] & 0xF0u) != 0xE0u) {
        return -1;
    }
    uint8_t raw = (uint8_t)(rdata[1] >> 3u);
    return (raw == 0u) ? -1 : (int)(raw - 1u);
}

bool MjkDoors_EvaluateCondition(MjkGame* game, uint8_t cond_idx) {
    if (!game) {
        return false;
    }
    switch (cond_idx) {
        case 0x02u:
            return game->player.just_jumped;

        case 0x07u: case 0x0Eu: case 0x17u: case 0x18u: case 0x1Bu:
            return game->mighty_level == 3u;

        case 0x09u:
            if ((game->door_flag_09 & 0x80u) == 0u && (game->door_flag_09 & 0x01u) != 0u) {
                game->door_flag_09 = 0x80u;
                return true;
            }
            return false;

        case 0x0Cu:
            if ((game->objects.maybe_bomb_thing & 0x80u) != 0u) {
                game->objects.maybe_bomb_thing &= 0x7Fu;
                return true;
            }
            return false;

        case 0x19u: case 0x1Du:
            if ((game->camera.room_flags & MJK_RF_AT_SCROLL_EDGE) != 0u &&
                game->mighty_coins == 0u && game->stage_timer_bcd == 0u &&
                game->player.sprite_id == 6u) {
                game->timer_status = 0u;
                game->door_flag_09 = 1u;
                return true;
            }
            return false;

        case 0x1Eu:
            if (game->door_1e_deaths == 2u && !game->player.is_dying) {
                game->door_1e_deaths = 0u;
                return true;
            }
            return false;

        case 0x1Fu:
            if (game->door_flag_1f == 0u) {
                game->door_flag_1f = 1u;
                return true;
            }
            return false;

        default:
            /* $00,$01,$03..$06,$08,$0A,$0B,$0D,$0F..$16,$1A,$1C,$20: RTS (no-op en REV_US). */
            return false;
    }
}

static const uint8_t k_unknown_door_table_us[32] = {
    0x1Fu, 0x06u, 0x17u, 0x03u, 0x03u, 0x1Au, 0x03u, 0x1Fu,
    0x0Eu, 0x03u, 0x0Fu, 0x16u, 0x1Eu, 0x17u, 0x16u, 0x0Bu,
    0x03u, 0x1Eu, 0x1Du, 0x10u, 0x13u, 0x1Au, 0x11u, 0x19u,
    0x1Du, 0x1Eu, 0x0Au, 0x1Fu, 0x05u, 0x12u, 0x03u, 0x16u
};

static void sync_door_persist(MjkGame* game, uint8_t room_id, uint8_t ram_index, uint8_t ram_state) {
    if (ram_index >= MJK_ROOM_SLOT_OBJ_BYTES) {
        return;
    }
    for (size_t i = 0u; i < MJK_ROOM_PERSIST_SLOTS; ++i) {
        MjkRoomPersistSlot* s = &game->persist.slots[i];
        if (s->room_id == room_id) {
            s->objects[ram_index] = ram_state;
        }
    }
    if (game->persist.scratch.room_id == room_id) {
        game->persist.scratch.objects[ram_index] = ram_state;
    }
}

void MjkDoors_OpenDoorInRoom(MjkGame* game,
                             MjkRoomState* target_room,
                             uint8_t target_nt,
                             uint8_t door_obj_idx) {
    if (!game || !target_room || door_obj_idx >= target_room->object_count) {
        return;
    }
    MjkRoomObject* ro = &target_room->objects[door_obj_idx];
    if ((ro->opcode & 0xF0u) != 0x80u) {
        return;
    }
    /* HandleOpenDoorMaybe / OpenBombRoomDoor (prg.asm:5362, 7070): $52 limpia bits 7 y 5 (oculta) y marca bit 6. */
    ro->ram_state = 0x52u;
    sync_door_persist(game, target_room->room_id, ro->ram_index, ro->ram_state);

    uint8_t yx0 = 0u;
    uint8_t yx1 = 0u;
    uint8_t base_mt = 0u;
    MjkRoom_GetDoorMeta((uint8_t)(ro->opcode & 0x0Fu), &yx0, &yx1, &base_mt);
    if (ro->yx != 0u) {
        MjkObjects_SetRoomMetatile(game, target_room, target_nt, ro->yx, (uint8_t)(base_mt + 2u));
    }
    if (ro->yx2 != 0u) {
        MjkObjects_SetRoomMetatile(game, target_room, target_nt, ro->yx2, (uint8_t)(base_mt + 3u));
    }
}

void MjkDoors_OpenRoomDoor(MjkGame* game, uint8_t door_obj_idx) {
    if (!game) {
        return;
    }
    MjkDoors_OpenDoorInRoom(game, &game->room, 0u, door_obj_idx);
}

static bool trigger_door_event(MjkGame* game, uint8_t event_idx, bool check_sphinx_flag) {
    if (!game || event_idx >= 32u) {
        return false;
    }
    if (check_sphinx_flag && (game->persist.sphinx_flags[event_idx] & 0x80u) == 0u) {
        return false;
    }
    game->persist.sphinx_flags[event_idx] = 0x52u;
    MjkGame_QueueSound(game, MJK_SND_DOOR);

    uint8_t val = k_unknown_door_table_us[event_idx];
    if ((val & 0x10u) == 0u) {
        return false;
    }

    uint8_t door_idx = (uint8_t)(val & 0x0Fu);
    int32_t wx = (int32_t)MjkUQ8_Int(game->player.pos_x) + (int32_t)(game->camera.scroll_x_fixed >> 8);
    int32_t wy = (int32_t)MjkUQ8_Int(game->player.pos_y) + (int32_t)(game->camera.scroll_y_fixed >> 8);
    bool in_neighbor = (game->camera.neighbor_room_id != 0u) &&
                       ((!game->room.is_horizontal && wy >= 240) ||
                        (game->room.is_horizontal && wx >= 256));

    MjkRoomState* rooms[2];
    uint8_t nts[2];
    if (in_neighbor) {
        rooms[0] = &game->right_room; nts[0] = 1u;
        rooms[1] = &game->room;       nts[1] = 0u;
    } else {
        rooms[0] = &game->room;       nts[0] = 0u;
        rooms[1] = (game->camera.neighbor_room_id != 0u) ? &game->right_room : NULL; nts[1] = 1u;
    }

    for (uint8_t r = 0u; r < 2u; ++r) {
        MjkRoomState* tr = rooms[r];
        if (!tr) {
            continue;
        }
        for (uint8_t i = 0u; i < tr->object_count; ++i) {
            const MjkRoomObject* ro = &tr->objects[i];
            if ((ro->opcode & 0xF0u) == 0x80u && (ro->opcode & 0x0Fu) == door_idx) {
                MjkDoors_OpenDoorInRoom(game, tr, nts[r], i);
                game->entry_door_type = door_idx;
                game->state = MJK_GS_ANIMATE_DOOR_OPEN;
                game->state_timer = 72u;
                game->sub_state = 1u;
                return true;
            }
        }
    }
    return false;
}

bool MjkDoors_Update(MjkGame* game) {
    if (!game || game->room.object_count == 0u) {
        return false;
    }
    int cond = MjkDoors_RoomConditionIndex(game);
    if (cond < 0 || !MjkDoors_EvaluateCondition(game, (uint8_t)cond)) {
        return false;
    }
    /* HandleOpenDoorMaybe (prg.asm:7030-7088): actualiza sphinx_flags[cond] = $52 y abre solo la puerta de UnknownDoorTable. */
    return trigger_door_event(game, (uint8_t)cond, true);
}

void MjkDoors_HandleSphinxEvent(MjkGame* game, uint8_t event_idx) {
    (void)trigger_door_event(game, event_idx, true);
}

bool MjkDoors_TriggerEventDirect(MjkGame* game, uint8_t event_idx) {
    return trigger_door_event(game, event_idx, false);
}

static void resolve_door_probe(MjkGame* game, int32_t px, int32_t py,
                               MjkRoomState** out_room, uint8_t* out_cell) {
    if (px < 0) { px = 0; }
    if (py < 0) { py = 0; }
    if (!game->room.is_horizontal && py >= 240 && game->camera.neighbor_room_id != 0u) {
        *out_room = &game->right_room;
        py -= 240;
        if (py >= 240) { py = 239; }
        if (px >= 256) { px = 255; }
    } else if (px >= 256 && game->camera.neighbor_room_id != 0u) {
        *out_room = &game->right_room;
        px -= 256;
        if (px >= 256) { px = 255; }
        if (py >= 240) { py = 239; }
    } else {
        *out_room = &game->room;
        if (px >= 256) { px = 255; }
        if (py >= 240) { py = 239; }
    }
    *out_cell = (uint8_t)((((uint8_t)py >> 4u) << 4u) | ((uint8_t)px >> 4u));
}

MjkActionResult MjkDoors_CheckPlayerEntry(MjkGame* game) {
    if (!game) {
        return MJK_ACTION_CONTINUE;
    }
    int32_t wx = (int32_t)MjkUQ8_Int(game->player.pos_x) + (int32_t)(game->camera.scroll_x_fixed >> 8);
    int32_t wy = (int32_t)MjkUQ8_Int(game->player.pos_y) + (int32_t)(game->camera.scroll_y_fixed >> 8);

    int32_t vx = wx;
    int32_t vy = wy;
    if (game->player.vel_y < 0) {
        vy = wy - 9;
    } else if (game->player.vel_y > 0) {
        vy = wy + 9;
    }
    int32_t hx = wx + (((game->player.facing_attr & 0x04u) != 0u) ? 10 : -10);
    int32_t hy = wy;

    int32_t px[3] = { wx, vx, hx };
    int32_t py[3] = { wy, vy, hy };

    for (uint8_t p = 0u; p < 3u; ++p) {
        MjkRoomState* target_room = NULL;
        uint8_t cell = 0u;
        resolve_door_probe(game, px[p], py[p], &target_room, &cell);
        if (!target_room) {
            continue;
        }
        for (uint8_t i = 0u; i < target_room->object_count; ++i) {
            MjkRoomObject* ro = &target_room->objects[i];
            if ((ro->opcode & 0xF0u) != 0x80u) {
                continue;
            }
            if ((ro->ram_state & 0x40u) == 0u || (ro->ram_state & 0x20u) != 0u) {
                continue;
            }
            if (cell == ro->yx || cell == ro->yx2) {
                if ((ro->extra & 1u) == 0u) {
                    ro->ram_state &= 0xBFu;
                    sync_door_persist(game, target_room->room_id, ro->ram_index, ro->ram_state);
                }
                MjkGame_QueueSound(game, MJK_SND_SILENCE);
                game->exit_source_room_id = target_room->room_id;
                game->entry_door_type = (uint8_t)(ro->opcode & 0x0Fu);
                game->state = MJK_GS_EXIT_ROOM;
                game->sub_state = 0u;
                return MJK_ACTION_ROOM_CHANGED;
            }
        }
    }
    return MJK_ACTION_CONTINUE;
}
