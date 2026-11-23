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

void MjkDoors_OpenRoomDoor(MjkGame* game, uint8_t door_obj_idx) {
    if (!game || door_obj_idx >= game->room.object_count) {
        return;
    }
    MjkRoomObject* ro = &game->room.objects[door_obj_idx];
    if ((ro->opcode & 0xF0u) != 0x80u) {
        return;
    }
    ro->ram_state |= 0x40u;
    for (size_t i = 0u; i < MJK_ROOM_PERSIST_SLOTS; ++i) {
        MjkRoomPersistSlot* s = &game->persist.slots[i];
        if (s->room_id == game->current_room_id && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
            s->objects[ro->ram_index] = ro->ram_state;
        }
    }
    if (game->persist.scratch.room_id == game->current_room_id &&
        ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
        game->persist.scratch.objects[ro->ram_index] = ro->ram_state;
    }
    uint8_t d_idx = (uint8_t)(ro->opcode & 0x0Fu);
    uint8_t open_mt = (d_idx < 8u) ? 0x8Au : 0x8Eu;
    /* Repintado: 2 metatiles contiguos de DoorPositionTileTable (estado abierto). */
    MjkObjects_SetMetatile(game, ro->yx, open_mt);
    if (ro->yx2 != 0u) {
        MjkObjects_SetMetatile(game, ro->yx2, open_mt);
    }
}

bool MjkDoors_Update(MjkGame* game) {
    if (!game || game->room.object_count == 0u) {
        return false;
    }
    int cond = MjkDoors_RoomConditionIndex(game);
    if (cond < 0 || !MjkDoors_EvaluateCondition(game, (uint8_t)cond)) {
        return false;
    }
    bool opened = false;
    for (uint8_t i = 0u; i < game->room.object_count; ++i) {
        const MjkRoomObject* ro = &game->room.objects[i];
        if ((ro->opcode & 0xF0u) == 0x80u && (ro->ram_state & 0x40u) == 0u) {
            MjkDoors_OpenRoomDoor(game, i);
            opened = true;
        }
    }
    if (opened) {
        MjkGame_QueueSound(game, MJK_SND_DOOR);
        game->state = MJK_GS_ANIMATE_DOOR_OPEN;
        game->sub_state = 0u;
    }
    return opened;
}

MjkActionResult MjkDoors_CheckPlayerEntry(MjkGame* game) {
    if (!game) {
        return MJK_ACTION_CONTINUE;
    }
    uint8_t jx = (uint8_t)(MjkUQ8_Int(game->player.pos_x) + (uint8_t)(game->camera.scroll_x_fixed >> 8));
    uint8_t jy = MjkUQ8_Int(game->player.pos_y);
    uint8_t cell = (uint8_t)(((jy >> 4u) << 4u) | (jx >> 4u));
    for (uint8_t i = 0u; i < game->room.object_count; ++i) {
        const MjkRoomObject* ro = &game->room.objects[i];
        if ((ro->opcode & 0xF0u) != 0x80u || (ro->ram_state & 0x40u) == 0u) {
            continue;
        }
        if (cell == ro->yx || cell == ro->yx2) {
            MjkGame_QueueSound(game, MJK_SND_SILENCE);
            game->entry_door_type = (uint8_t)(ro->opcode & 0x0Fu);
            game->state = MJK_GS_EXIT_ROOM;
            game->sub_state = 0u;
            return MJK_ACTION_ROOM_CHANGED;
        }
    }
    return MJK_ACTION_CONTINUE;
}
