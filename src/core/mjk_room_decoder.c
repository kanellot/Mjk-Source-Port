/**
 * @file mjk_room_decoder.c
 * @brief Decodificador 4-tier de las 256 salas y máquina de persistencia de objetos.
 */

#include "mjk_room_decoder.h"
#include <string.h>

/* Tablas estructurales de puertas en REV_US (asm/src/prg.asm:3823-3855). */
static const uint8_t k_door_pos_tile_table[16][4] = {
    { 0x11u, 0x12u, 0x87u, 0x95u },
    { 0x1Du, 0x1Eu, 0x87u, 0x95u },
    { 0x11u, 0x12u, 0x87u, 0x95u },
    { 0x1Du, 0x1Eu, 0x87u, 0x95u },
    { 0xE1u, 0xE2u, 0x87u, 0x95u },
    { 0xEDu, 0xEEu, 0x87u, 0x95u },
    { 0xD1u, 0xD2u, 0x87u, 0x95u },
    { 0xDDu, 0xDEu, 0x87u, 0x95u },
    { 0x2Cu, 0x3Cu, 0x8Bu, 0x97u },
    { 0xBFu, 0xCFu, 0x8Bu, 0x97u },
    { 0x2Fu, 0x3Fu, 0x8Bu, 0x97u },
    { 0xBFu, 0xCFu, 0x8Bu, 0x97u },
    { 0x23u, 0x33u, 0x8Bu, 0x97u },
    { 0xB0u, 0xC0u, 0x8Bu, 0x97u },
    { 0x20u, 0x30u, 0x8Bu, 0x97u },
    { 0xB0u, 0xC0u, 0x8Bu, 0x97u }
};

static const uint8_t k_door_state_table_us[8] = {
    0x12u, 0x52u, 0x72u, 0x92u, 0x92u, 0xB2u, 0xB2u, 0x32u
};

static uint8_t get_metatile_attr(const uint8_t* tile_attr_tbl, uint8_t metatile_id) {
    uint8_t byte_val = tile_attr_tbl[metatile_id >> 2u];
    uint8_t shift = (uint8_t)(6u - 2u * (metatile_id & 3u));
    return (uint8_t)((byte_val >> shift) & 3u);
}

static void apply_metatile_xy(MjkRoomState* room,
                              const uint8_t* tile_attr_tbl,
                              uint8_t yx,
                              uint8_t metatile_id) {
    uint8_t my = (uint8_t)((yx >> 4u) & 0x0Fu);
    uint8_t mx = (uint8_t)(yx & 0x0Fu);
    if (my < MJK_ROOM_MT_ROWS && mx < MJK_ROOM_MT_COLS && metatile_id < 153u) {
        room->metatiles[my][mx] = metatile_id;
        room->attributes[my][mx] = get_metatile_attr(tile_attr_tbl, metatile_id);
    }
}

bool MjkRoom_DecodeGeometry(const MjkAssetPackage* pkg,
                            uint8_t room_id,
                            MjkRoomState* out_room) {
    if (!pkg || !out_room) {
        return false;
    }
    memset(out_room, 0, sizeof(*out_room));
    out_room->room_id = room_id;
    out_room->is_bomb_room = (room_id >= 0x90u && room_id <= 0x9Fu);

    const uint8_t* adj_tbl = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_ADJACENT_ROOMS, 256u * 5u);
    const uint8_t* halves = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_ROOM_HALF_SCREENS, 256u * 2u);
    const uint8_t* layouts = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_HALF_SCREEN_LAYTS, 130u * 16u);
    const uint8_t* chunks = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_LAYOUT_CHUNKS, 216u * 8u);
    const uint8_t* attr_tbl = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_TILE_ATTR_TABLE, 40u);
    if (!adj_tbl || !halves || !layouts || !chunks || !attr_tbl) {
        return false;
    }

    const uint8_t* adj_entry = adj_tbl + ((size_t)room_id * 5u);
    out_room->adj.flags = adj_entry[0];
    out_room->adj.up    = adj_entry[1];
    out_room->adj.down  = adj_entry[2];
    out_room->adj.right = adj_entry[3];
    out_room->adj.left  = adj_entry[4];
    out_room->is_horizontal = ((out_room->adj.flags & 0x04u) != 0u);

    uint8_t half0 = halves[(size_t)room_id * 2u];
    uint8_t half1 = halves[(size_t)room_id * 2u + 1u];
    if (half0 >= 130u || half1 >= 130u) {
        return false;
    }

    if (out_room->is_horizontal) {
        for (uint8_t mx = 0u; mx < MJK_ROOM_MT_COLS; ++mx) {
            uint8_t c0 = layouts[(size_t)half0 * 16u + mx];
            uint8_t c1 = layouts[(size_t)half1 * 16u + mx];
            if (c0 >= 0x60u) { c0 = (uint8_t)(c0 - 0x0Bu); }
            if (c1 >= 0x60u) { c1 = (uint8_t)(c1 - 0x0Bu); }
            if (c0 >= 216u || c1 >= 216u) {
                return false;
            }
            for (uint8_t my = 0u; my < 8u; ++my) {
                uint8_t m = chunks[(size_t)c0 * 8u + my];
                if (m >= 153u) { return false; }
                out_room->metatiles[my][mx] = m;
                out_room->attributes[my][mx] = get_metatile_attr(attr_tbl, m);
            }
            for (uint8_t my = 8u; my < MJK_ROOM_MT_ROWS; ++my) {
                uint8_t m = chunks[(size_t)c1 * 8u + (my - 8u)];
                if (m >= 153u) { return false; }
                out_room->metatiles[my][mx] = m;
                out_room->attributes[my][mx] = get_metatile_attr(attr_tbl, m);
            }
        }
    } else {
        for (uint8_t my = 0u; my < MJK_ROOM_MT_ROWS; ++my) {
            uint8_t c0 = layouts[(size_t)half0 * 16u + my];
            uint8_t c1 = layouts[(size_t)half1 * 16u + my];
            if (c0 >= 0x60u) { c0 = (uint8_t)(c0 - 0x0Bu); }
            if (c1 >= 0x60u) { c1 = (uint8_t)(c1 - 0x0Bu); }
            if (c0 >= 216u || c1 >= 216u) {
                return false;
            }
            for (uint8_t mx = 0u; mx < 8u; ++mx) {
                uint8_t m = chunks[(size_t)c0 * 8u + mx];
                if (m >= 153u) { return false; }
                out_room->metatiles[my][mx] = m;
                out_room->attributes[my][mx] = get_metatile_attr(attr_tbl, m);
            }
            for (uint8_t mx = 8u; mx < MJK_ROOM_MT_COLS; ++mx) {
                uint8_t m = chunks[(size_t)c1 * 8u + (mx - 8u)];
                if (m >= 153u) { return false; }
                out_room->metatiles[my][mx] = m;
                out_room->attributes[my][mx] = get_metatile_attr(attr_tbl, m);
            }
        }
    }

    if (room_id >= 1u) {
        const uint8_t* rdata = MjkAssets_GetRoomDataPtr(pkg, room_id, NULL);
        if (rdata && (rdata[0] & 0xF0u) == 0xE0u) {
            out_room->palette_set = (uint8_t)(rdata[0] & 0x0Fu);
            out_room->music_id = (uint8_t)(rdata[1] & 0x07u);
            out_room->section_id = (uint8_t)(rdata[2] & 0x7Fu);
        }
    }
    memcpy(out_room->static_metatiles, out_room->metatiles, sizeof(out_room->static_metatiles));
    return true;
}

static bool parse_room_data_to_slot(const MjkAssetPackage* pkg,
                                    uint8_t room_id,
                                    MjkRoomPersistSlot* slot,
                                    MjkRoomPersistRam* persist,
                                    bool is_slot_0,
                                    uint8_t byte_B_init,
                                    uint8_t crystal_balls_mask) {
    memset(slot, 0, sizeof(*slot));
    slot->room_id = room_id;

    const uint8_t* rdata = MjkAssets_GetRoomDataPtr(pkg, room_id, NULL);
    if (!rdata) {
        return false;
    }

    size_t f = ((rdata[0] & 0xF0u) == 0xE0u) ? 3u : 0u;
    size_t e = 0u;
    uint8_t bomb_rem = 0u;
    uint8_t byte_B = byte_B_init;
    bool is_bomb_room = (room_id >= 0x90u && room_id <= 0x9Fu);

    while (rdata[f] != 0xFFu) {
        uint8_t op = 0u;
        if (bomb_rem > 0u) {
            f++; /* YX de bomba encadenada */
            bomb_rem--;
            if (byte_B != 2u) {
                byte_B = 0u;
            }
        } else {
            op = rdata[f++];
            if ((op & 0xF0u) == 0x00u) {
                bomb_rem = is_bomb_room ? 0x18u : (uint8_t)((op - 1u) & 0x0Fu);
                if (is_slot_0 && room_id < 0x90u) {
                    byte_B = 1u;
                }
            }
            f++; /* YX */
        }

        uint8_t hi = (uint8_t)(op >> 4u);
        if (hi < 0x0Bu) {
            if (e >= MJK_ROOM_SLOT_OBJ_BYTES - 1u) {
                break;
            }
            if (hi == 0u) {
                uint8_t st = 0x05u;
                if (byte_B == 1u || (byte_B == 2u && bomb_rem == 0u)) {
                    st = 0x85u;
                }
                slot->objects[e++] = st;
            } else if (hi <= 4u || hi == 9u || hi == 0x0Au) {
                uint8_t st = (hi == 3u || hi == 4u) ? 0x11u : 0x10u;
                if ((op & 0x0Fu) == 6u) {
                    f++; /* Esfinge: +1 byte extra */
                }
                if (op >= 0x80u) {
                    st |= 0x20u;
                }
                slot->objects[e++] = st;
            } else if (hi == 5u || hi == 6u) {
                slot->objects[e++] = 0x33u;
            } else if (hi == 7u) {
                uint8_t item = (uint8_t)(rdata[f - 2u] & 0x0Fu);
                if ((item == 9u && (crystal_balls_mask & 1u) != 0u) ||
                    (item == 0x0Au && (crystal_balls_mask & 2u) != 0u)) {
                    item = 0x0Fu;
                }
                slot->objects[e++] = item;
            } else if (hi == 8u) {
                uint8_t yx_door = rdata[f - 1u];
                uint8_t st = k_door_state_table_us[yx_door & 7u];
                if ((st & 0x80u) != 0u && persist) {
                    persist->sphinx_flags[yx_door >> 3u] = st;
                }
                slot->objects[e++] = st;
            }
        } else if (hi == 0x0Eu) {
            f++; /* Opcode $Ex: +1 byte extra */
        }
    }

    if (e < MJK_ROOM_SLOT_OBJ_BYTES) {
        slot->objects[e] = 0xFFu;
    }
    return true;
}

bool MjkRoom_PreloadSection(const MjkAssetPackage* pkg,
                            uint8_t room_id,
                            MjkRoomPersistRam* persist,
                            uint8_t crystal_balls_mask) {
    if (!pkg || !persist) {
        return false;
    }
    if (room_id == 0u) {
        memset(persist, 0, sizeof(*persist));
        return true;
    }

    const uint8_t* rdata = MjkAssets_GetRoomDataPtr(pkg, room_id, NULL);
    if (!rdata) {
        return false;
    }

    if ((rdata[0] & 0xF0u) == 0xE0u && rdata[2] != 0u && (rdata[2] & 0x40u) == 0u) {
        uint8_t sec_idx = (uint8_t)((rdata[2] & 0x7Fu) - 1u);
        uint16_t sec_addr = 0u;
        if (!MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_SECTION_ROOMS_TBL + sec_idx * 2u), &sec_addr)) {
            return false;
        }
        const uint8_t* sec_rooms = MjkAssets_ResolveCpuPtr(pkg, sec_addr, 2u);
        if (!sec_rooms) {
            return false;
        }
        memset(persist->slots, 0, sizeof(persist->slots));
        for (size_t i = 0u; i < MJK_ROOM_PERSIST_SLOTS; ++i) {
            uint8_t r = sec_rooms[i];
            if (r == 0u) {
                break;
            }
            uint8_t next_r = (i + 1u < MJK_ROOM_PERSIST_SLOTS) ? sec_rooms[i + 1u] : 0u;
            uint8_t byte_B = ((next_r == 0u || next_r >= 0x90u) && r < 0x90u) ? 2u : 0u;
            if (!parse_room_data_to_slot(pkg, r, &persist->slots[i], persist, i == 0u, byte_B, crystal_balls_mask)) {
                return false;
            }
        }
        return true;
    }

    return parse_room_data_to_slot(pkg, room_id, &persist->scratch, persist, false, 0u, crystal_balls_mask);
}

static const MjkRoomPersistSlot* find_persist_slot(const MjkRoomPersistRam* persist, uint8_t room_id) {
    if (!persist) {
        return NULL;
    }
    for (size_t i = 0u; i < MJK_ROOM_PERSIST_SLOTS; ++i) {
        if (persist->slots[i].room_id == room_id) {
            return &persist->slots[i];
        }
    }
    if (persist->scratch.room_id == room_id) {
        return &persist->scratch;
    }
    return NULL;
}

bool MjkRoom_DecodeObjects(const MjkAssetPackage* pkg,
                           uint8_t room_id,
                           MjkRoomPersistRam* persist,
                           MjkRoomState* room) {
    if (!pkg || !room) {
        return false;
    }
    room->object_count = 0u;
    if (room_id == 0u) {
        return true;
    }

    const uint8_t* item_to_tile = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_ITEM_TO_TILE, 18u);
    const uint8_t* attr_tbl = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_TILE_ATTR_TABLE, 40u);
    const uint8_t* rdata = MjkAssets_GetRoomDataPtr(pkg, room_id, NULL);
    if (!item_to_tile || !attr_tbl || !rdata) {
        return false;
    }

    const MjkRoomPersistSlot* slot = find_persist_slot(persist, room_id);
    size_t f = ((rdata[0] & 0xF0u) == 0xE0u) ? 3u : 0u;
    uint8_t bomb_rem = 0u;
    uint8_t ram_idx = 0u;

    while (rdata[f] != 0xFFu && room->object_count < MJK_ROOM_MAX_OBJECTS) {
        MjkRoomObject obj;
        memset(&obj, 0, sizeof(obj));

        if (bomb_rem > 0u) {
            obj.opcode = 0u;
            obj.yx = rdata[f++];
            bomb_rem--;
        } else {
            obj.opcode = rdata[f++];
            obj.yx = rdata[f++];
            if ((obj.opcode & 0xF0u) == 0x00u) {
                bomb_rem = room->is_bomb_room ? 0x18u : (uint8_t)((obj.opcode - 1u) & 0x0Fu);
            }
        }

        obj.ram_index = ram_idx;
        obj.ram_state = (slot && ram_idx < MJK_ROOM_SLOT_OBJ_BYTES) ? slot->objects[ram_idx] : 0u;
        uint8_t tile = item_to_tile[obj.ram_state & 0x1Fu];
        uint8_t tile2 = 0u;

        if (obj.opcode == 0x16u || obj.opcode == 0x36u || obj.opcode == 0x96u || (obj.opcode & 0xF0u) == 0xE0u) {
            obj.extra = rdata[f++];
        } else if ((obj.opcode & 0xF0u) == 0x80u) {
            uint8_t raw_arg = obj.yx;
            obj.extra = raw_arg;
            if (persist && (k_door_state_table_us[raw_arg & 7u] & 0x80u) != 0u &&
                (obj.ram_state & 0x80u) != 0u) {
                obj.ram_state = persist->sphinx_flags[raw_arg >> 3u];
            }
            uint8_t d_idx = (uint8_t)(obj.opcode & 0x0Fu);
            uint8_t y_off = ((obj.ram_state & 0x40u) != 0u) ? 2u : 0u;
            uint8_t open_off = ((obj.ram_state & 0x80u) != 0u) ? 1u : 0u;
            obj.yx  = k_door_pos_tile_table[d_idx][0];
            obj.yx2 = k_door_pos_tile_table[d_idx][1];
            tile  = (uint8_t)(k_door_pos_tile_table[d_idx][2u + open_off] + y_off);
            tile2 = (uint8_t)(tile + 1u);
        }

        if (obj.yx != 0u && (obj.opcode & 0xF0u) < 0xB0u && (obj.ram_state & 0x20u) == 0u) {
            apply_metatile_xy(room, attr_tbl, obj.yx, tile);
            if ((obj.opcode & 0xF0u) == 0x80u && obj.yx2 != 0u) {
                apply_metatile_xy(room, attr_tbl, obj.yx2, tile2);
            }
        }

        room->objects[room->object_count++] = obj;
        if ((obj.opcode & 0xF0u) < 0xB0u) {
            ram_idx++;
        }
    }
    return true;
}

void MjkRoom_BlitToPpu(const MjkRoomState* room,
                       const MjkAssetPackage* pkg,
                       MjkPpu* ppu,
                       uint8_t nt_idx) {
    if (!room || !pkg || !ppu) {
        return;
    }
    for (uint8_t my = 0u; my < MJK_ROOM_MT_ROWS; ++my) {
        for (uint8_t mx = 0u; mx < MJK_ROOM_MT_COLS; ++mx) {
            MjkPpu_SetMetatile(ppu, pkg, nt_idx, mx, my,
                               room->metatiles[my][mx],
                               room->attributes[my][mx]);
        }
    }
}

void MjkRoom_GetDoorMeta(uint8_t door_type,
                         uint8_t* yx0,
                         uint8_t* yx1,
                         uint8_t* base_mt) {
    uint8_t idx = (uint8_t)(door_type & 0x0Fu);
    if (yx0)     { *yx0     = k_door_pos_tile_table[idx][0]; }
    if (yx1)     { *yx1     = k_door_pos_tile_table[idx][1]; }
    if (base_mt) { *base_mt = k_door_pos_tile_table[idx][2]; }
}
