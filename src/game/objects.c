/**
 * @file objects.c
 * @brief Bombas, mechas, cofres rojos/naranjas, bloques secretos, items y Mighty (R-08, R-09).
 */

#include "mjk/objects.h"
#include "mjk/game.h"
#include <math.h>
#include <string.h>

static void calc_object_screen_pos(const MjkGame* game,
                                   const MjkRoomObj* obj,
                                   bool in_neighbor,
                                   float* out_x,
                                   float* out_y) {
    float sx = (float)(obj->grid_x * MJK_METATILE_SIZE + 8) +
               ((in_neighbor && game->world.current.is_horizontal) ? 256.0f : 0.0f) -
               floorf(game->world.camera.scroll_x);
    float sy = (float)(obj->grid_y * MJK_METATILE_SIZE + 8) +
               ((in_neighbor && !game->world.current.is_horizontal) ? 240.0f : 0.0f) -
               floorf(game->world.camera.scroll_y);
    if (sx < 0.0f)   { sx = 0.0f; }
    if (sx > 255.0f) { sx = 255.0f; }
    if (sy < 0.0f)   { sy = 0.0f; }
    if (sy > 239.0f) { sy = 239.0f; }
    *out_x = sx;
    *out_y = sy;
}

void MjkObjects_InitForRoom(MjkObjectSystem* obj, MjkGame* game) {
    if (!obj) {
        return;
    }
    /* Conservar power_coin_acc entre salas mientras no haya una Power Coin armada. */
    uint8_t saved_acc = obj->power_coin_armed ? 0u : obj->power_coin_acc;
    memset(obj, 0, sizeof(*obj));
    obj->power_coin_acc = saved_acc;
    if (!game) {
        return;
    }
    if (game->world.current.is_bomb_room) {
        game->fire_bombs_collected = 0u;
        for (uint8_t i = 0u; i < game->world.current.object_count; ++i) {
            const MjkRoomObj* ro = &game->world.current.objects[i];
            if (ro->kind == OBJ_KIND_BOMB && ro->is_lit && !ro->is_collected) {
                obj->spark.active  = true;
                obj->spark.grid_x  = ro->grid_x;
                obj->spark.grid_y  = ro->grid_y;
                obj->spark.tile_id = 0x0Du;
                break;
            }
        }
    }
}

bool MjkObjects_UseMightyCoin(MjkGame* game, bool* out_trigger_transmute) {
    if (out_trigger_transmute) {
        *out_trigger_transmute = false;
    }
    if (!game || !game->assets || game->world.current.is_bomb_room) {
        return false;
    }
    if (game->mighty_coins == 0u || game->mighty_level >= 3u) {
        return false;
    }
    game->mighty_coins--;
    game->mighty_level++;
    game->mighty_presses_left = game->assets->mighty_a_presses[game->mighty_level & 3u];
    MjkGame_QueueSound(game, SOUND_USED_MIGHTY_COIN);
    if (out_trigger_transmute && game->mighty_level == 3u) {
        *out_trigger_transmute = true;
    }
    return true;
}

static void advance_corridor_fuse(MjkGame* game, MjkRoomInstance* room, uint8_t picked_idx) {
    for (uint8_t n = (uint8_t)(picked_idx + 1u); n < room->object_count; ++n) {
        MjkRoomObj* cand = &room->objects[n];
        if (cand->kind == OBJ_KIND_BOMB && !cand->is_collected) {
            cand->is_lit = true;
            MjkWorld_SaveObjectState(&game->world, room, n);
            return;
        }
    }

    /* Propagar mecha oculta al siguiente slot de sala de la seccion */
    size_t cur_slot = MJK_PERSIST_SLOTS;
    for (size_t s = 0u; s < MJK_PERSIST_SLOTS; ++s) {
        if (game->world.persist.slots[s].room_id == room->room_id) {
            cur_slot = s;
            break;
        }
    }
    for (size_t s = cur_slot + 1u; s < MJK_PERSIST_SLOTS; ++s) {
        MjkPersistSlot* ns = &game->world.persist.slots[s];
        if (ns->room_id == 0u) {
            break;
        }
        for (uint8_t k = 0u; k < ns->count; ++k) {
            if (ns->objects[k].item_kind == ITEM_BOMB && !ns->objects[k].is_collected) {
                ns->objects[k].is_lit = true;
                if (game->world.has_neighbor && game->world.neighbor.room_id == ns->room_id) {
                    for (uint8_t r = 0u; r < game->world.neighbor.object_count; ++r) {
                        if (game->world.neighbor.objects[r].persist_index == k) {
                            game->world.neighbor.objects[r].is_lit = true;
                            break;
                        }
                    }
                }
                return;
            }
        }
    }
}

static void collect_bomb(MjkObjectSystem* obj,
                         MjkGame* game,
                         MjkRoomInstance* room,
                         bool in_neighbor,
                         uint8_t idx) {
    MjkRoomObj* ro = &room->objects[idx];
    bool lit = ro->is_lit;
    bool was_first = !obj->first_bomb_picked;

    if (was_first) {
        obj->first_bomb_picked = true;
        obj->bomb_order = (!room->is_bomb_room && ro->is_order_marker)
            ? BOMB_ORDER_ACTIVE : BOMB_ORDER_NONE;
    } else if (!room->is_bomb_room && !lit && obj->bomb_order == BOMB_ORDER_ACTIVE) {
        obj->bomb_order = BOMB_ORDER_DISQUALIFIED;
    }

    bool is_order_bonus = (!room->is_bomb_room && !was_first &&
                           lit && ro->is_order_marker &&
                           obj->bomb_order == BOMB_ORDER_ACTIVE);

    /* Acumulador de Power Coin (20 pts; congelado si armada o transmutacion activa) */
    bool triggered_pc = false;
    if (!obj->power_coin_armed && game->enemies.transmute_timer == 0) {
        obj->power_coin_acc = (uint8_t)(obj->power_coin_acc + (lit ? 2u : 1u));
        if (obj->power_coin_acc >= 20u) {
            obj->power_coin_acc = 0u;
            triggered_pc = true;
        }
    }

    if (is_order_bonus) {
        MjkGame_AddScore(game, SCORE_200);
        MjkGame_AddScore(game, SCORE_10000);
        obj->bomb_order = BOMB_ORDER_COMPLETED;
        if (!triggered_pc) { MjkGame_QueueSound(game, SOUND_BOMB_BONUS); }
    } else if (lit) {
        MjkGame_AddScore(game, SCORE_200);
        if (!triggered_pc) { MjkGame_QueueSound(game, SOUND_BOMB_LIT); }
    } else {
        MjkGame_AddScore(game, SCORE_100);
        if (!triggered_pc) { MjkGame_QueueSound(game, SOUND_BOMB_UNLIT); }
    }

    ro->is_collected = true;
    ro->is_lit       = false;
    MjkWorld_SaveObjectState(&game->world, room, idx);
    MjkWorld_SetVisualMetatile(room, game->assets, ro->grid_x, ro->grid_y, 0x2Fu);

    if (lit && room->is_bomb_room) {
        game->fire_bombs_collected++;
    }
    if (triggered_pc) {
        if (room->is_bomb_room) {
            MjkEnemies_SpawnPowerCoinBombRoom(&game->enemies, game->assets);
            MjkGame_QueueSound(game, SOUND_POWER_COIN_SPAWN);
        } else {
            obj->power_coin_armed = true;
            MjkGame_QueueSound(game, SOUND_BOMB_CHEST_BONUS);
        }
    }

    /* Animacion puff+popup de 2 fases (32f humo + 32f popup) */
    obj->puff.active    = true;
    obj->puff.phase     = 0u;
    obj->puff.timer     = 32;
    obj->puff.was_lit   = lit;
    obj->puff.mult_tile = 0x0Fu;
    calc_object_screen_pos(game, ro, in_neighbor, &obj->puff.x, &obj->puff.y);

    if (room->is_bomb_room) {
        bool any_left = false;
        for (uint8_t k = 0u; k < room->object_count; ++k) {
            if (room->objects[k].kind == OBJ_KIND_BOMB && !room->objects[k].is_collected) {
                any_left = true;
                break;
            }
        }
        if (!any_left) {
            obj->spark.active = false;
            uint8_t exit_door = game->assets->bomb_room_exit_door[room->room_id & 0x0Fu];
            for (uint8_t d = 0u; d < room->object_count; ++d) {
                if (room->objects[d].kind == OBJ_KIND_DOOR && room->objects[d].door_slot == exit_door) {
                    MjkDoors_OpenInRoom(game, room, d);
                    break;
                }
            }
            game->entry_door_type = exit_door;
            game->state           = GS_ANIMATE_DOOR_OPEN;
            game->sub_state       = 0u;
        } else if (was_first || lit) {
            obj->spark.active = false;
            for (uint8_t step = 1u; step < room->object_count; ++step) {
                uint8_t n = (uint8_t)((idx + step) % room->object_count);
                if (room->objects[n].kind == OBJ_KIND_BOMB && !room->objects[n].is_collected) {
                    room->objects[n].is_lit = true;
                    MjkWorld_SaveObjectState(&game->world, room, n);
                    obj->spark.active     = true;
                    obj->spark.grid_x     = room->objects[n].grid_x;
                    obj->spark.grid_y     = room->objects[n].grid_y;
                    obj->spark.tile_id    = 0x0Du;
                    obj->spark.anim_timer = 0u;
                    break;
                }
            }
        }
    } else if (was_first || lit) {
        obj->spark.active = false;
        advance_corridor_fuse(game, room, idx);
    }
}

static void open_chest(MjkObjectSystem* obj,
                       MjkGame* game,
                       MjkRoomInstance* room,
                       bool in_neighbor,
                       uint8_t idx) {
    MjkRoomObj* ro = &room->objects[idx];
    uint8_t item_id = (uint8_t)ro->item_kind;
    if ((item_id == 7u || item_id == 9u) && game->collected_1up) {
        item_id = 8u;
    } else if (item_id == (uint8_t)ENEMY_BROTHER && game->collected_1up) {
        item_id = (uint8_t)ENEMY_CAPTIVE;
    }

    float sx = 0.0f, sy = 0.0f;
    calc_object_screen_pos(game, ro, in_neighbor, &sx, &sy);

    if (ro->grid_y < MJK_ROOM_ROWS && ro->grid_x < MJK_ROOM_COLS) {
        room->static_metatiles[ro->grid_y][ro->grid_x] = 0x2Fu;
    }

    if (item_id < 9u) {
        ro->is_revealed = true;
        ro->is_hidden   = false;
        ro->item_kind   = (MjkItemKind)item_id;
        MjkWorld_SaveObjectState(&game->world, room, idx);
        MjkWorld_SetVisualMetatile(room, game->assets, ro->grid_x, ro->grid_y,
                                   game->assets->item_to_metatile[item_id]);
    } else {
        ro->is_collected = true;
        ro->is_revealed  = true;
        ro->is_hidden    = false;
        MjkWorld_SaveObjectState(&game->world, room, idx);
        MjkWorld_SetVisualMetatile(room, game->assets, ro->grid_x, ro->grid_y, 0x2Fu);
        MjkEnemies_Spawn(&game->enemies, (MjkEnemyKind)item_id, sx, sy);
        if (item_id == (uint8_t)ENEMY_EXTRA_COIN || item_id == (uint8_t)ENEMY_BONUS_COIN) {
            MjkGame_QueueSound(game, SOUND_BONUS_COIN_SPAWN);
        }
    }

    if (obj->power_coin_armed) {
        obj->power_coin_armed = false;
        MjkEnemies_Spawn(&game->enemies, ENEMY_POWER_COIN, sx, sy);
    }
}

static MjkActionResult pickup_item(MjkGame* game, MjkRoomInstance* room, uint8_t idx) {
    MjkRoomObj* ro = &room->objects[idx];
    MjkItemKind item = ro->item_kind;

    ro->is_collected = true;
    MjkWorld_SaveObjectState(&game->world, room, idx);
    MjkWorld_SetVisualMetatile(room, game->assets, ro->grid_x, ro->grid_y, 0x2Fu);

    switch (item) {
        case ITEM_BAG_100:
            MjkGame_AddScore(game, SCORE_100);
            MjkGame_QueueSound(game, SOUND_BOMB_LIT);
            break;
        case ITEM_BAG_300:
            MjkGame_AddScore(game, SCORE_300);
            MjkGame_QueueSound(game, SOUND_BOMB_LIT);
            break;
        case ITEM_BAG_1000:
            MjkGame_AddScore(game, SCORE_1000);
            MjkGame_QueueSound(game, SOUND_BOMB_LIT);
            break;
        case ITEM_GOLD_COIN:
            MjkGame_AddScore(game, SCORE_500);
            game->scoring.gold_coins++;
            MjkGame_QueueSound(game, SOUND_COIN_COLLECTED);
            break;
        case ITEM_MIGHTY_DRINK:
            MjkGame_AddScore(game, SCORE_1000);
            MjkGame_QueueSound(game, SOUND_TIME_PICKUP);
            game->scoring.timer_seconds += 10;
            if (game->scoring.timer_seconds >= 100) {
                game->scoring.timer_seconds = 99;
                return MjkGame_TriggerTortureRoom(game);
            }
            break;
        case ITEM_SPHINX:
            MjkGame_AddScore(game, SCORE_10000);
            MjkDoors_HandleSphinxEvent(game, ro->sphinx_event);
            break;
        case ITEM_TECMO_PLATE:
            MjkGame_AddScore(game, SCORE_100000);
            game->scoring.lives++;
            game->extra_lives_found++;
            game->collected_1up = true;
            game->one_up_timer  = 64;
            MjkGame_QueueSound(game, SOUND_1UP_CAPTIVE);
            break;
        case ITEM_MIGHTY_COIN:
            MjkGame_AddScore(game, SCORE_1000);
            game->mighty_coins++;
            MjkGame_QueueSound(game, SOUND_COIN_COLLECTED);
            if (game->mighty_coins >= 10u) {
                return MjkGame_TriggerTortureRoom(game);
            }
            break;
        case ITEM_CRYSTAL_BALL_1:
            game->crystal_balls_mask |= 1u;
            MjkScoring_AddGdv(&game->scoring, 1);
            MjkGame_QueueSound(game, SOUND_CRYSTAL_BALL);
            break;
        case ITEM_CRYSTAL_BALL_2:
            game->crystal_balls_mask |= 2u;
            MjkScoring_AddGdv(&game->scoring, 1);
            MjkGame_QueueSound(game, SOUND_CRYSTAL_BALL);
            break;
        case ITEM_KING:
        case ITEM_QUEEN:
        case ITEM_PRINCESS:
            MjkDoors_TriggerEventDirect(game, (uint8_t)((uint8_t)item + 6u));
            break;
        case ITEM_BEELZEBUT_MASK:
            MjkGame_AddScore(game, SCORE_1000000);
            MjkGame_QueueSound(game, MUSIC_COLLECTED_FULL_FAMILY);
            MjkDoors_TriggerEventDirect(game, (uint8_t)((uint8_t)item + 6u));
            break;
        default:
            break;
    }
    return ACTION_CONTINUE;
}

static void tick_puff_and_spark(MjkObjectSystem* obj, const MjkGame* game) {
    if (obj->puff.active) {
        obj->puff.x += game->world.camera.delta_x;
        obj->puff.y += game->world.camera.delta_y;
        if (obj->puff.x < 0.0f || obj->puff.x >= 256.0f ||
            obj->puff.y < 0.0f || obj->puff.y >= 240.0f) {
            obj->puff.active = false;
        } else if (--obj->puff.timer <= 0) {
            if (obj->puff.phase == 0u) {
                obj->puff.phase = 1u;
                obj->puff.timer = 32;
                uint8_t m = game->scoring.score_multiplier;
                obj->puff.mult_tile = (game->world.current.is_bomb_room && m >= 1u && m <= 4u)
                    ? (uint8_t)(0x3Bu + m) : 0x0Fu;
            } else {
                obj->puff.active = false;
            }
        }
    }
    if (obj->spark.active) {
        obj->spark.anim_timer++;
        if ((obj->spark.anim_timer % 3u) == 0u) {
            obj->spark.tile_id ^= 0x02u;
        }
    }
}

MjkActionResult MjkObjects_Update(MjkObjectSystem* obj,
                                  MjkGame* game,
                                  const MjkInputStep* input) {
    if (!obj || !game || !game->assets) {
        return ACTION_CONTINUE;
    }

    if (input && (input->pressed & MJK_INPUT_B) != 0u) {
        bool transmute = false;
        MjkObjects_UseMightyCoin(game, &transmute);
        if (transmute) {
            MjkEnemies_ActivatePowerCoin(&game->enemies, game);
        }
    }

    tick_puff_and_spark(obj, game);

    int wx = (int)floorf(game->player.x) + (int)floorf(game->world.camera.scroll_x);
    int wy = (int)floorf(game->player.y) + (int)floorf(game->world.camera.scroll_y);
    bool in_neighbor = false;
    int lx = wx, ly = wy;
    if (!game->world.current.is_horizontal && wy >= 240 && game->world.has_neighbor) {
        in_neighbor = true;
        ly = wy - 240;
    } else if (wx >= 256 && game->world.has_neighbor) {
        in_neighbor = true;
        lx = wx - 256;
    }
    if (lx < 0) { lx = 0; } else if (lx > 255) { lx = 255; }
    if (ly < 0) { ly = 0; } else if (ly > 239) { ly = 239; }

    MjkRoomInstance* room = in_neighbor ? &game->world.neighbor : &game->world.current;
    uint8_t jx = (uint8_t)(lx >> 4);
    uint8_t jy = (uint8_t)(ly >> 4);
    uint8_t feet_y = (uint8_t)(((ly + 8 < 240) ? (ly + 8) : 239) >> 4);

    /* Salto desde encima de cofre o bloque secreto */
    if (game->player.just_jumped && obj->has_chest_target) {
        bool target_in_nb = (game->world.has_neighbor &&
                             obj->chest_target_room_id == game->world.neighbor.room_id);
        MjkRoomInstance* croom = target_in_nb ? &game->world.neighbor : &game->world.current;
        for (uint8_t i = 0u; i < croom->object_count; ++i) {
            MjkRoomObj* ro = &croom->objects[i];
            if (ro->grid_x != obj->chest_target_x || ro->grid_y != obj->chest_target_y || ro->is_collected) {
                continue;
            }
            if (ro->kind == OBJ_KIND_SECRET_BLOCK) {
                if (obj->secret_block_x != ro->grid_x || obj->secret_block_y != ro->grid_y) {
                    obj->secret_block_x   = ro->grid_x;
                    obj->secret_block_y   = ro->grid_y;
                    obj->secret_hits_left = (uint8_t)(ro->raw_opcode & 0x0Fu);
                }
                if (obj->secret_hits_left > 0u) { obj->secret_hits_left--; }
                if (obj->secret_hits_left == 0u) {
                    ro->is_collected = true;
                    ro->is_hidden    = false;
                    MjkWorld_SaveObjectState(&game->world, croom, i);
                    MjkWorld_SetVisualMetatile(croom, game->assets, ro->grid_x, ro->grid_y, 0x2Fu);
                    if (ro->grid_y < MJK_ROOM_ROWS && ro->grid_x < MJK_ROOM_COLS) {
                        croom->static_metatiles[ro->grid_y][ro->grid_x] = 0x2Fu;
                    }
                    MjkGame_QueueSound(game, SOUND_SECRET_BLOCK_REVEALED);
                }
                break;
            }
            if ((ro->kind == OBJ_KIND_RED_CHEST || ro->kind == OBJ_KIND_ORANGE_CHEST) && !ro->is_revealed) {
                if (ro->kind == OBJ_KIND_ORANGE_CHEST && game->mighty_level == 0u) {
                    break;
                }
                if (ro->is_hidden) {
                    ro->is_hidden = false;
                    MjkWorld_SaveObjectState(&game->world, croom, i);
                    uint8_t mt = game->assets->item_to_metatile[
                        (ro->kind == OBJ_KIND_ORANGE_CHEST) ? ITEM_ORANGE_CHEST : ITEM_RED_CHEST];
                    MjkWorld_SetVisualMetatile(croom, game->assets, ro->grid_x, ro->grid_y, mt);
                    if (obj->power_coin_armed) {
                        obj->power_coin_armed = false;
                        float cx = 0.0f, cy = 0.0f;
                        calc_object_screen_pos(game, ro, target_in_nb, &cx, &cy);
                        MjkEnemies_Spawn(&game->enemies, ENEMY_POWER_COIN, cx, cy);
                    }
                } else {
                    open_chest(obj, game, croom, target_in_nb, i);
                }
                break;
            }
        }
        obj->has_chest_target = false;
    } else if (!game->player.just_jumped) {
        obj->has_chest_target = false;
    }

    /* Direccion lateral / cabeza para abrir cofres con Mighty >= 2 */
    bool facing_right = (game->player.facing == FACING_RIGHT);
    if (input && (input->held & MJK_INPUT_RIGHT) != 0u && (input->held & MJK_INPUT_LEFT) == 0u) {
        facing_right = true;
    } else if (input && (input->held & MJK_INPUT_LEFT) != 0u && (input->held & MJK_INPUT_RIGHT) == 0u) {
        facing_right = false;
    }
    int side_wx = facing_right ? (wx + 10) : (wx - 10);
    bool has_side = (side_wx >= 0 && side_wx < 512 &&
                     (side_wx < 256 || (game->world.current.is_horizontal && game->world.has_neighbor)));
    bool side_in_nb = (!game->world.current.is_horizontal) ? in_neighbor
                                                           : (side_wx >= 256 && game->world.has_neighbor);
    MjkRoomInstance* side_room = side_in_nb ? &game->world.neighbor : &game->world.current;
    uint8_t side_gx = (uint8_t)((side_wx & 255) >> 4);
    bool has_head = (game->player.vy < 0.0f || (game->player.collision_flags & MJK_COLL_CEILING) != 0u) && (ly >= 9);
    uint8_t head_gy = has_head ? (uint8_t)((ly - 9) >> 4) : 0xFFu;

    for (uint8_t i = 0u; i < room->object_count; ++i) {
        MjkRoomObj* ro = &room->objects[i];
        if (ro->is_collected) {
            continue;
        }
        if (ro->kind == OBJ_KIND_BOMB && ro->grid_x == jx && ro->grid_y == jy) {
            collect_bomb(obj, game, room, in_neighbor, i);
            continue;
        }
        if ((ro->kind == OBJ_KIND_RED_CHEST || ro->kind == OBJ_KIND_ORANGE_CHEST) && !ro->is_revealed) {
            if (ro->kind == OBJ_KIND_ORANGE_CHEST && game->mighty_level == 0u) {
                continue;
            }
            if (!game->player.in_air && ro->grid_x == jx && ro->grid_y == feet_y) {
                obj->has_chest_target     = true;
                obj->chest_target_x       = ro->grid_x;
                obj->chest_target_y       = ro->grid_y;
                obj->chest_target_room_id = room->room_id;
            }
            bool hit_m2 = (game->mighty_level >= 2u && !ro->is_hidden) &&
                          ((ro->grid_x == jx && ro->grid_y == jy) ||
                           (has_head && ro->grid_x == jx && ro->grid_y == head_gy) ||
                           (has_side && side_room == room && ro->grid_x == side_gx && ro->grid_y == jy));
            if (hit_m2) {
                open_chest(obj, game, room, in_neighbor, i);
                continue;
            }
        }
        if (ro->kind == OBJ_KIND_SECRET_BLOCK) {
            if (!game->player.in_air && ro->grid_x == jx && ro->grid_y == feet_y) {
                obj->has_chest_target     = true;
                obj->chest_target_x       = ro->grid_x;
                obj->chest_target_y       = ro->grid_y;
                obj->chest_target_room_id = room->room_id;
            }
            continue;
        }
        bool is_pickable = (ro->kind == OBJ_KIND_LOOSE_ITEM) ||
                           ((ro->kind == OBJ_KIND_RED_CHEST || ro->kind == OBJ_KIND_ORANGE_CHEST) && ro->is_revealed);
        if (is_pickable && ro->grid_x == jx && ro->grid_y == jy) {
            MjkActionResult res = pickup_item(game, room, i);
            if (res != ACTION_CONTINUE) {
                return res;
            }
        }
    }

    /* Cofre en sala vecina alcanzable lateralmente con Mighty >= 2 */
    if (has_side && side_room != room && game->mighty_level >= 2u) {
        for (uint8_t i = 0u; i < side_room->object_count; ++i) {
            MjkRoomObj* ro = &side_room->objects[i];
            if ((ro->kind == OBJ_KIND_RED_CHEST || ro->kind == OBJ_KIND_ORANGE_CHEST) &&
                !ro->is_revealed && !ro->is_hidden && !ro->is_collected &&
                ro->grid_x == side_gx && ro->grid_y == jy) {
                open_chest(obj, game, side_room, side_in_nb, i);
                break;
            }
        }
    }
    return ACTION_CONTINUE;
}
