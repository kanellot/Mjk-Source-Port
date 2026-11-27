/**
 * @file world.c
 * @brief Implementacion del mundo, persistencia por seccion, camara en float y colisiones.
 */

#include "mjk/world.h"
#include <math.h>
#include <string.h>

/* Mascaras de cuadrantes 8x8 para los 10 pares de metatiles parciales (0x38..0x4B) */
static const uint8_t k_partial_masks[10] = {
    0xCCu, 0xFCu, 0x48u, 0x5Au, 0x7Bu, 0xDEu, 0x21u, 0x35u, 0xACu, 0x3Cu
};

/* Estado inicial de puertas en REV_US indexado por (door_arg & 7) */
static const uint8_t k_door_init_flags[8] = {
    0x12u, 0x52u, 0x72u, 0x92u, 0x92u, 0xB2u, 0xB2u, 0x32u
};

static void init_persist_slot(MjkPersistSlot* slot,
                              MjkSectionPersist* persist,
                              const MjkAssets* assets,
                              uint8_t room_id,
                              bool is_first_in_section,
                              bool is_last_in_section,
                              uint8_t crystal_balls_mask) {
    memset(slot, 0, sizeof(*slot));
    slot->room_id = room_id;
    const MjkRoomTemplate* tmpl = &assets->rooms[room_id];

    /* Contar cuantas bombas hay para marcar la primera/ultima de pasillo */
    int bomb_total = 0;
    for (uint8_t i = 0u; i < tmpl->object_count; ++i) {
        if (tmpl->objects[i].kind == OBJ_KIND_BOMB) {
            bomb_total++;
        }
    }

    int bomb_seen = 0;
    uint8_t p_idx = 0u;
    for (uint8_t i = 0u; i < tmpl->object_count && p_idx < MJK_PERSIST_OBJ_CAP; ++i) {
        const MjkRoomObjectDef* d = &tmpl->objects[i];
        if (!d->is_persistent) {
            continue;
        }
        MjkPersistObjState* ps = &slot->objects[p_idx++];
        memset(ps, 0, sizeof(*ps));
        ps->initialized = true;
        ps->item_kind   = d->item_kind;

        if (d->kind == OBJ_KIND_BOMB) {
            bomb_seen++;
            bool is_corridor = (room_id < 0x90u);
            if (is_first_in_section && is_corridor && bomb_seen == 1) {
                ps->is_order_marker = true;
            } else if (is_last_in_section && is_corridor && bomb_seen == bomb_total) {
                ps->is_order_marker = true;
            }
            ps->is_lit = false;
        } else if (d->kind == OBJ_KIND_RED_CHEST || d->kind == OBJ_KIND_ORANGE_CHEST) {
            ps->is_hidden = d->starts_hidden;
        } else if (d->kind == OBJ_KIND_SECRET_BLOCK) {
            ps->is_hidden = true;
        } else if (d->kind == OBJ_KIND_LOOSE_ITEM) {
            if ((d->item_kind == ITEM_CRYSTAL_BALL_1 && (crystal_balls_mask & 1u) != 0u) ||
                (d->item_kind == ITEM_CRYSTAL_BALL_2 && (crystal_balls_mask & 2u) != 0u)) {
                ps->is_collected = true;
                ps->item_kind    = ITEM_NOTHING;
            }
        } else if (d->kind == OBJ_KIND_DOOR) {
            uint8_t st = k_door_init_flags[d->door_arg & 7u];
            ps->is_open          = ((st & 0x40u) != 0u);
            ps->is_hidden        = ((st & 0x20u) != 0u);
            ps->is_sphinx_locked = ((st & 0x80u) != 0u);
            uint8_t sp_idx = (uint8_t)(d->door_arg >> 3u);
            if (ps->is_sphinx_locked && sp_idx < MJK_SPHINX_FLAG_COUNT && persist) {
                if (persist->sphinx_opened[sp_idx] == 2u) {
                    ps->is_open          = true;
                    ps->is_hidden        = false;
                    ps->is_sphinx_locked = false;
                } else {
                    persist->sphinx_opened[sp_idx] = 1u;
                }
            }
        }
    }
    slot->count = p_idx;
}

MjkPersistSlot* MjkWorld_FindPersistSlot(MjkWorld* world, uint8_t room_id) {
    if (!world || room_id == 0u) {
        return NULL;
    }
    for (size_t i = 0u; i < MJK_PERSIST_SLOTS; ++i) {
        if (world->persist.slots[i].room_id == room_id) {
            return &world->persist.slots[i];
        }
    }
    if (world->persist.scratch.room_id == room_id) {
        return &world->persist.scratch;
    }
    return NULL;
}

static void preload_section_if_needed(MjkWorld* world,
                                      const MjkAssets* assets,
                                      uint8_t room_id,
                                      uint8_t crystal_balls_mask) {
    if (room_id == 0u) {
        memset(&world->persist, 0, sizeof(world->persist));
        return;
    }
    const MjkRoomTemplate* tmpl = &assets->rooms[room_id];
    if (tmpl->has_header && tmpl->section_raw != 0u && (tmpl->section_raw & 0x40u) == 0u) {
        uint8_t sec_idx = (uint8_t)((tmpl->section_raw & 0x7Fu) - 1u);
        if (sec_idx < assets->section_count) {
            memset(world->persist.slots, 0, sizeof(world->persist.slots));
            for (size_t i = 0u; i < MJK_PERSIST_SLOTS; ++i) {
                uint8_t r = assets->section_rooms[sec_idx][i];
                if (r == 0u) { break; }
                uint8_t next_r = (i + 1u < MJK_PERSIST_SLOTS) ? assets->section_rooms[sec_idx][i + 1u] : 0u;
                bool is_last = ((next_r == 0u || next_r >= 0x90u) && r < 0x90u);
                init_persist_slot(&world->persist.slots[i], &world->persist, assets,
                                  r, i == 0u, is_last, crystal_balls_mask);
            }
            return;
        }
    }

    if (!MjkWorld_FindPersistSlot(world, room_id)) {
        init_persist_slot(&world->persist.scratch, &world->persist, assets,
                          room_id, false, false, crystal_balls_mask);
    }
}

void MjkWorld_PreloadSection(MjkWorld* world,
                             const MjkAssets* assets,
                             uint8_t room_id,
                             uint8_t crystal_balls_mask) {
    if (!world || !assets) {
        return;
    }
    preload_section_if_needed(world, assets, room_id, crystal_balls_mask);
}

void MjkWorld_SetVisualMetatile(MjkRoomInstance* room,
                                const MjkAssets* assets,
                                uint8_t grid_x,
                                uint8_t grid_y,
                                uint8_t metatile_id) {
    if (!room || !assets || grid_x >= MJK_ROOM_COLS || grid_y >= MJK_ROOM_ROWS ||
        metatile_id >= MJK_METATILE_COUNT) {
        return;
    }
    room->visual_metatiles[grid_y][grid_x] = metatile_id;
    room->visual_palettes[grid_y][grid_x]  = assets->metatile_palette[metatile_id];
}

static void instantiate_room(MjkWorld* world,
                             const MjkAssets* assets,
                             uint8_t room_id,
                             MjkRoomInstance* out_room) {
    memset(out_room, 0, sizeof(*out_room));
    const MjkRoomTemplate* tmpl = &assets->rooms[room_id];
    out_room->room_id       = tmpl->room_id;
    out_room->adj           = tmpl->adj;
    out_room->palette_set   = tmpl->palette_set;
    out_room->music_index   = tmpl->music_index;
    out_room->section_id    = tmpl->section_id;
    out_room->door_cond_raw = tmpl->door_cond_raw;
    out_room->is_horizontal = tmpl->is_horizontal;
    out_room->is_bomb_room  = tmpl->is_bomb_room;

    memcpy(out_room->static_metatiles, tmpl->static_metatiles, sizeof(out_room->static_metatiles));
    for (uint8_t y = 0u; y < MJK_ROOM_ROWS; ++y) {
        for (uint8_t x = 0u; x < MJK_ROOM_COLS; ++x) {
            uint8_t m = tmpl->static_metatiles[y][x];
            out_room->visual_metatiles[y][x] = m;
            out_room->visual_palettes[y][x]  = (m < MJK_METATILE_COUNT) ? assets->metatile_palette[m] : 0u;
        }
    }

    MjkPersistSlot* slot = MjkWorld_FindPersistSlot(world, room_id);
    uint8_t p_idx = 0u;
    for (uint8_t i = 0u; i < tmpl->object_count && out_room->object_count < MJK_ROOM_MAX_OBJECTS; ++i) {
        const MjkRoomObjectDef* d = &tmpl->objects[i];
        MjkRoomObj obj;
        memset(&obj, 0, sizeof(obj));
        obj.kind          = d->kind;
        obj.grid_x        = d->grid_x;
        obj.grid_y        = d->grid_y;
        obj.grid_x2       = d->grid_x2;
        obj.grid_y2       = d->grid_y2;
        obj.item_kind     = d->item_kind;
        obj.door_slot     = d->door_slot;
        obj.door_arg      = d->door_arg;
        obj.sphinx_event  = d->sphinx_event;
        obj.raw_opcode    = d->raw_opcode;
        obj.persist_index = p_idx;

        if (d->is_persistent && slot && p_idx < slot->count) {
            const MjkPersistObjState* ps = &slot->objects[p_idx];
            obj.item_kind        = ps->item_kind;
            obj.is_lit           = ps->is_lit;
            obj.is_order_marker  = ps->is_order_marker;
            obj.is_open          = ps->is_open;
            obj.is_sphinx_locked = ps->is_sphinx_locked;
            obj.is_hidden        = ps->is_hidden;
            obj.is_collected     = ps->is_collected;
            obj.is_revealed      = ps->is_revealed;
        }

        if (d->kind == OBJ_KIND_DOOR && obj.is_sphinx_locked) {
            uint8_t sp_idx = (uint8_t)(d->door_arg >> 3u);
            if (sp_idx < MJK_SPHINX_FLAG_COUNT && world->persist.sphinx_opened[sp_idx] == 2u) {
                obj.is_open          = true;
                obj.is_hidden        = false;
                obj.is_sphinx_locked = false;
                if (slot && d->is_persistent && p_idx < slot->count) {
                    slot->objects[p_idx].is_open          = true;
                    slot->objects[p_idx].is_hidden        = false;
                    slot->objects[p_idx].is_sphinx_locked = false;
                }
            }
        }

        /* Pintar el metatile visual correspondiente si el objeto es visible */
        if (d->is_persistent && !obj.is_hidden) {
            if (d->kind == OBJ_KIND_DOOR) {
                uint8_t normal_mt = 0u, sealed_mt = 0u;
                MjkAssets_GetDoorLayout(d->door_slot, NULL, NULL, NULL, NULL, &normal_mt, &sealed_mt);
                uint8_t base_mt = (uint8_t)((obj.is_sphinx_locked ? sealed_mt : normal_mt) +
                                            (obj.is_open ? 2u : 0u));
                MjkWorld_SetVisualMetatile(out_room, assets, obj.grid_x, obj.grid_y, base_mt);
                MjkWorld_SetVisualMetatile(out_room, assets, obj.grid_x2, obj.grid_y2, (uint8_t)(base_mt + 1u));
            } else if (d->raw_yx != 0u) {
                uint8_t mt = 0x2Fu;
                if (obj.is_collected) {
                    mt = 0x2Fu;
                } else if (d->kind == OBJ_KIND_RED_CHEST && !obj.is_revealed) {
                    mt = assets->item_to_metatile[ITEM_RED_CHEST];
                } else if (d->kind == OBJ_KIND_ORANGE_CHEST && !obj.is_revealed) {
                    mt = assets->item_to_metatile[ITEM_ORANGE_CHEST];
                } else if (d->kind == OBJ_KIND_BOMB) {
                    mt = assets->item_to_metatile[ITEM_BOMB];
                } else if ((unsigned)obj.item_kind < 18u) {
                    mt = assets->item_to_metatile[obj.item_kind];
                }
                MjkWorld_SetVisualMetatile(out_room, assets, obj.grid_x, obj.grid_y, mt);

                /* Despejar celda estatica en bombas normales, items sueltos y bloques rotos */
                bool is_plain_item = (d->kind == OBJ_KIND_LOOSE_ITEM) ||
                                     (d->kind == OBJ_KIND_BOMB && !obj.is_order_marker) ||
                                     obj.is_revealed || obj.is_collected;
                if (is_plain_item && obj.grid_y < MJK_ROOM_ROWS && obj.grid_x < MJK_ROOM_COLS) {
                    out_room->static_metatiles[obj.grid_y][obj.grid_x] = 0x2Fu;
                }
            }
        }

        out_room->objects[out_room->object_count++] = obj;
        if (d->is_persistent) {
            p_idx++;
        }
    }
}

static void init_camera_for_room(MjkCamera* cam, const MjkRoomInstance* room) {
    memset(cam, 0, sizeof(*cam));
    cam->current_room_id = room->room_id;

    if (room->is_bomb_room || (room->adj.flags & 0x01u) != 0u) {
        cam->room_flags = (uint8_t)(ROOM_FLAG_SINGLE_SCREEN | (room->is_bomb_room ? ROOM_FLAG_BOMB_ROOM : 0u));
        return;
    }

    if (!room->is_horizontal) {
        cam->room_flags = ROOM_FLAG_AT_SCROLL_EDGE;
        if (room->adj.up == 0u || (room->adj.flags & 0x08u) != 0u) {
            cam->room_flags |= ROOM_FLAG_SECTION_START;
        }
        if ((room->adj.flags & 0x0Au) != 0x02u && room->adj.down != 0u) {
            cam->neighbor_room_id = room->adj.down;
        }
        return;
    }

    cam->room_flags = (uint8_t)(ROOM_FLAG_HORIZONTAL | ROOM_FLAG_SCROLL_STOP | ROOM_FLAG_AT_SCROLL_EDGE);
    if (room->adj.left == 0u || (room->adj.flags & 0x08u) != 0u) {
        cam->room_flags |= ROOM_FLAG_SECTION_START;
    }
    if ((room->adj.flags & 0x0Au) != 0x02u && room->adj.right != 0u) {
        cam->neighbor_room_id = room->adj.right;
    }
}

void MjkWorld_Reset(MjkWorld* world) {
    if (world) {
        memset(world, 0, sizeof(*world));
    }
}

bool MjkWorld_LoadRoom(MjkWorld* world,
                       const MjkAssets* assets,
                       uint8_t room_id,
                       uint8_t crystal_balls_mask) {
    if (!world || !assets) {
        return false;
    }
    preload_section_if_needed(world, assets, room_id, crystal_balls_mask);
    instantiate_room(world, assets, room_id, &world->current);
    world->active_palette_set = world->current.palette_set;
    init_camera_for_room(&world->camera, &world->current);

    if (world->camera.neighbor_room_id != 0u) {
        instantiate_room(world, assets, world->camera.neighbor_room_id, &world->neighbor);
        world->has_neighbor = true;
    } else {
        memset(&world->neighbor, 0, sizeof(world->neighbor));
        world->has_neighbor = false;
    }
    return true;
}

void MjkWorld_SaveObjectState(MjkWorld* world,
                              const MjkRoomInstance* room,
                              uint8_t obj_index) {
    if (!world || !room || obj_index >= room->object_count) {
        return;
    }
    const MjkRoomObj* obj = &room->objects[obj_index];
    MjkPersistSlot* slot = MjkWorld_FindPersistSlot(world, room->room_id);
    if (!slot || obj->persist_index >= slot->count) {
        return;
    }
    MjkPersistObjState* ps = &slot->objects[obj->persist_index];
    ps->item_kind        = obj->item_kind;
    ps->is_lit           = obj->is_lit;
    ps->is_order_marker  = obj->is_order_marker;
    ps->is_open          = obj->is_open;
    ps->is_sphinx_locked = obj->is_sphinx_locked;
    ps->is_hidden        = obj->is_hidden;
    ps->is_collected     = obj->is_collected;
    ps->is_revealed      = obj->is_revealed;
}

static void update_vertical_camera(MjkWorld* world,
                                   const MjkAssets* assets,
                                   float* player_x,
                                   float* player_y,
                                   float player_vy) {
    MjkCamera* cam = &world->camera;
    const MjkRoomAdjacency* adj = &assets->rooms[cam->current_room_id].adj;

    if (*player_x < 8.0f)   { *player_x = 8.0f; }
    if (*player_x > 248.0f) { *player_x = 248.0f; }

    if ((cam->room_flags & ROOM_FLAG_AT_SCROLL_EDGE) != 0u) {
        if (cam->scroll_y == 0.0f && *player_y >= 120.0f && player_vy > 0.0f && cam->neighbor_room_id != 0u) {
            cam->room_flags &= (uint8_t)~ROOM_FLAG_AT_SCROLL_EDGE;
        } else if (*player_y <= 120.0f && player_vy < 0.0f && adj->up != 0u && (adj->flags & 0x0Au) != 0x0Au) {
            cam->room_flags &= (uint8_t)~ROOM_FLAG_AT_SCROLL_EDGE;
        } else {
            if (*player_y < 8.0f)   { *player_y = 8.0f; }
            if (*player_y > 232.0f) { *player_y = 232.0f; }
            return;
        }
    }

    float old_sy = floorf(cam->scroll_y);
    float dy = (*player_y - 120.0f);
    if (dy == 0.0f) { dy = player_vy; }
    float next_sy = cam->scroll_y + dy;
    float unwrapped = next_sy;
    *player_y = 120.0f;

    if (next_sy >= 240.0f) {
        if (cam->neighbor_room_id != 0u) {
            cam->current_room_id = cam->neighbor_room_id;
            next_sy -= 240.0f;
            cam->room_crossed = true;
            const MjkRoomAdjacency* nadj = &assets->rooms[cam->current_room_id].adj;
            if ((nadj->flags & 0x0Au) != 0x02u && nadj->down != 0u) {
                cam->neighbor_room_id = nadj->down;
            } else {
                cam->neighbor_room_id = 0u;
                next_sy = 0.0f;
                unwrapped = 240.0f;
                cam->room_flags |= ROOM_FLAG_AT_SCROLL_EDGE;
            }
        } else {
            next_sy = 0.0f;
            unwrapped = 0.0f;
            cam->room_flags |= ROOM_FLAG_AT_SCROLL_EDGE;
        }
    } else if (next_sy < 0.0f) {
        if (adj->up != 0u && (adj->flags & 0x0Au) != 0x0Au) {
            cam->neighbor_room_id = cam->current_room_id;
            cam->current_room_id  = adj->up;
            next_sy += 240.0f;
            cam->room_crossed = true;
            cam->room_flags &= (uint8_t)~ROOM_FLAG_SECTION_START;
        } else {
            next_sy = 0.0f;
            unwrapped = 0.0f;
            cam->room_flags |= (uint8_t)(ROOM_FLAG_AT_SCROLL_EDGE | ROOM_FLAG_SECTION_START);
        }
    }

    cam->scroll_y = next_sy;
    cam->delta_y  = old_sy - floorf(unwrapped);
}

void MjkWorld_UpdateCamera(MjkWorld* world,
                           const MjkAssets* assets,
                           float* player_x,
                           float* player_y,
                           float player_vx,
                           float player_vy,
                           uint8_t crystal_balls_mask) {
    if (!world || !assets || !player_x || !player_y) {
        return;
    }
    MjkCamera* cam = &world->camera;
    cam->delta_x      = 0.0f;
    cam->delta_y      = 0.0f;
    cam->room_crossed = false;

    if ((cam->room_flags & ROOM_FLAG_SINGLE_SCREEN) != 0u) {
        if (*player_x < 8.0f)   { *player_x = 8.0f; }
        if (*player_x > 248.0f) { *player_x = 248.0f; }
        return;
    }

    const MjkRoomAdjacency* adj = &assets->rooms[cam->current_room_id].adj;
    if ((cam->room_flags & ROOM_FLAG_HORIZONTAL) == 0u && (adj->flags & 0x04u) == 0u) {
        update_vertical_camera(world, assets, player_x, player_y, player_vy);
    } else {
        if ((cam->room_flags & ROOM_FLAG_AT_SCROLL_EDGE) != 0u) {
            if (cam->scroll_x == 0.0f && *player_x >= 128.0f && player_vx > 0.0f && cam->neighbor_room_id != 0u) {
                cam->room_flags &= (uint8_t)~ROOM_FLAG_AT_SCROLL_EDGE;
            } else if (*player_x <= 128.0f && player_vx < 0.0f && adj->left != 0u && (adj->flags & 0x0Au) != 0x0Au) {
                cam->room_flags &= (uint8_t)~ROOM_FLAG_AT_SCROLL_EDGE;
            } else {
                if (*player_x < 8.0f)   { *player_x = 8.0f; }
                if (*player_x > 248.0f) { *player_x = 248.0f; }
                return;
            }
        }

        float old_sx = floorf(cam->scroll_x);
        float dx = (*player_x - 128.0f);
        if (dx == 0.0f) { dx = player_vx; }
        float next_sx = cam->scroll_x + dx;
        float unwrapped = next_sx;
        *player_x = 128.0f;

        if (next_sx >= 256.0f) {
            if (cam->neighbor_room_id != 0u) {
                cam->current_room_id = cam->neighbor_room_id;
                next_sx -= 256.0f;
                cam->room_crossed = true;
                const MjkRoomAdjacency* nadj = &assets->rooms[cam->current_room_id].adj;
                if ((nadj->flags & 0x0Au) != 0x02u && nadj->right != 0u) {
                    cam->neighbor_room_id = nadj->right;
                } else {
                    cam->neighbor_room_id = 0u;
                    next_sx = 0.0f;
                    unwrapped = 256.0f;
                    cam->room_flags |= ROOM_FLAG_AT_SCROLL_EDGE;
                }
            } else {
                next_sx = 0.0f;
                unwrapped = 0.0f;
                cam->room_flags |= ROOM_FLAG_AT_SCROLL_EDGE;
            }
        } else if (next_sx < 0.0f) {
            if (adj->left != 0u && (adj->flags & 0x0Au) != 0x0Au) {
                cam->neighbor_room_id = cam->current_room_id;
                cam->current_room_id  = adj->left;
                next_sx += 256.0f;
                cam->room_crossed = true;
                cam->room_flags &= (uint8_t)~ROOM_FLAG_SECTION_START;
            } else {
                next_sx = 0.0f;
                unwrapped = 0.0f;
                cam->room_flags |= (uint8_t)(ROOM_FLAG_AT_SCROLL_EDGE | ROOM_FLAG_SECTION_START);
            }
        }
        cam->scroll_x = next_sx;
        cam->delta_x  = old_sx - floorf(unwrapped);
    }

    if (cam->room_crossed) {
        (void)crystal_balls_mask;
        instantiate_room(world, assets, cam->current_room_id, &world->current);
        if (cam->neighbor_room_id != 0u) {
            instantiate_room(world, assets, cam->neighbor_room_id, &world->neighbor);
            world->has_neighbor = true;
        } else {
            memset(&world->neighbor, 0, sizeof(world->neighbor));
            world->has_neighbor = false;
        }
    }
}

static bool test_partial_metatile(uint8_t m, int wx, int wy) {
    uint8_t idx    = (uint8_t)((m - 0x38u) >> 1u);
    uint8_t packed = k_partial_masks[idx];
    uint8_t nibble = ((m & 1u) == 0u) ? (uint8_t)(packed >> 4u) : (uint8_t)(packed & 0x0Fu);
    uint8_t qx     = (uint8_t)((wx >> 3) & 1);
    uint8_t qy     = (uint8_t)((wy >> 3) & 1);
    uint8_t bit    = (uint8_t)(3u - (qy * 2u + qx));
    return ((nibble >> bit) & 1u) != 0u;
}

bool MjkWorld_IsPointSolid(const MjkWorld* world,
                           int wx,
                           int wy,
                           bool is_ground_probe,
                           uint8_t* out_fire_tile) {
    if (!world) {
        return false;
    }
    if (wy < 0) { wy = 0; }

    const MjkRoomInstance* src = &world->current;
    if (!world->current.is_horizontal && wy >= 240 && world->has_neighbor) {
        src = &world->neighbor;
        wy -= 240;
        if (wy >= 240) { wy = 239; }
        wx &= 255;
    } else {
        if (wy >= 240) { wy = 239; }
        if (wx < 0) { wx = 0; }
        int wrapped_x = wx & 511;
        if (wrapped_x >= 256 && world->has_neighbor) {
            src = &world->neighbor;
        }
        wx = wrapped_x & 255;
    }

    uint8_t mx = (uint8_t)(wx >> 4);
    uint8_t my = (uint8_t)(wy >> 4);
    if (my >= MJK_ROOM_ROWS) { my = MJK_ROOM_ROWS - 1u; }

    uint8_t m = src->static_metatiles[my][mx];
    if (m >= 0x5Cu && m <= 0x62u) {
        if (out_fire_tile) { *out_fire_tile = m; }
        return false;
    }
    if (m != 0x00u && m != 0x27u && m != 0x2Fu) {
        if (m >= 0x38u && m <= 0x4Bu) {
            return test_partial_metatile(m, wx, wy);
        }
        return true;
    }

    /* Objetos solidos: puertas cerradas visibles y cofres/secretos sin abrir */
    for (uint8_t i = 0u; i < src->object_count; ++i) {
        const MjkRoomObj* o = &src->objects[i];
        if (o->kind == OBJ_KIND_DOOR) {
            if (!o->is_open && !o->is_hidden &&
                ((o->grid_x == mx && o->grid_y == my) || (o->grid_x2 == mx && o->grid_y2 == my))) {
                return true;
            }
        } else if ((o->kind == OBJ_KIND_RED_CHEST || o->kind == OBJ_KIND_ORANGE_CHEST ||
                    o->kind == OBJ_KIND_SECRET_BLOCK) &&
                   !o->is_revealed && !o->is_collected &&
                   o->grid_x == mx && o->grid_y == my) {
            /* Visible: solido por todos lados; oculto: solido solo al aterrizar por arriba */
            return o->is_hidden ? is_ground_probe : true;
        }
    }
    return false;
}

bool MjkWorld_IsEnemyPointSolid(const MjkWorld* world, int wx, int wy) {
    if (!world || wy < 16) {
        return true;
    }
    if (wx < 0 || wx >= 512) {
        return false;
    }

    const MjkRoomInstance* src = &world->current;
    if (!world->current.is_horizontal && wy >= 240) {
        if (!world->has_neighbor || wy >= 480) { return true; }
        if (wx >= 256) { return false; }
        src = &world->neighbor;
        wy -= 240;
    } else {
        if (wy >= 240) { return true; }
        if (wx >= 256) {
            if (!world->current.is_horizontal || !world->has_neighbor) {
                return false;
            }
            src = &world->neighbor;
            wx -= 256;
        }
    }

    uint8_t mx = (uint8_t)(wx >> 4);
    uint8_t my = (uint8_t)(wy >> 4);
    if (my >= MJK_ROOM_ROWS) { my = MJK_ROOM_ROWS - 1u; }

    uint8_t m = src->static_metatiles[my][mx];
    if (m >= 0x5Cu && m <= 0x62u) {
        return false;
    }
    if (m != 0x00u && m != 0x27u && m != 0x2Fu) {
        if (m >= 0x38u && m <= 0x4Bu) {
            return test_partial_metatile(m, wx, wy);
        }
        return true;
    }

    /* Enemigos y monedas rebotantes solo chocan con cofres/bloques visibles sin abrir (NO con puertas cerradas) */
    for (uint8_t i = 0u; i < src->object_count; ++i) {
        const MjkRoomObj* o = &src->objects[i];
        if ((o->kind == OBJ_KIND_RED_CHEST || o->kind == OBJ_KIND_ORANGE_CHEST ||
             o->kind == OBJ_KIND_SECRET_BLOCK) &&
            !o->is_hidden && !o->is_revealed && !o->is_collected &&
            o->grid_x == mx && o->grid_y == my) {
            return true;
        }
    }
    return false;
}
