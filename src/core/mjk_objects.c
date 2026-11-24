/**
 * @file mjk_objects.c
 * @brief Implementación de bombas, mechas, cofres, secretos, ítems y Mighty (T-19, T-20).
 */

#include "mjk_objects.h"
#include "mjk_game.h"
#include <string.h>

static MjkRoomPersistSlot* get_slot_for_room(MjkGame* game, uint8_t room_id) {
    for (size_t i = 0u; i < MJK_ROOM_PERSIST_SLOTS; ++i) {
        if (game->persist.slots[i].room_id == room_id) {
            return &game->persist.slots[i];
        }
    }
    if (game->persist.scratch.room_id == room_id) {
        return &game->persist.scratch;
    }
    return NULL;
}

void MjkObjects_SetRoomMetatile(MjkGame* game,
                                MjkRoomState* target_room,
                                uint8_t target_nt,
                                uint8_t yx,
                                uint8_t metatile_id) {
    uint8_t my = (uint8_t)((yx >> 4u) & 0x0Fu);
    uint8_t mx = (uint8_t)(yx & 0x0Fu);
    if (my >= MJK_ROOM_MT_ROWS || mx >= MJK_ROOM_MT_COLS) {
        return;
    }
    const uint8_t* attr_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_TILE_ATTR_TABLE, 40u);
    uint8_t attr = 0u;
    if (attr_tbl && metatile_id < 153u) {
        uint8_t b = attr_tbl[metatile_id >> 2u];
        attr = (uint8_t)((b >> (6u - 2u * (metatile_id & 3u))) & 3u);
    }
    target_room->metatiles[my][mx] = metatile_id;
    target_room->attributes[my][mx] = attr;
    MjkPpu_SetMetatile(&game->ppu, game->pkg, target_nt, mx, my, metatile_id, attr);
}

void MjkObjects_SetMetatile(MjkGame* game, uint8_t yx, uint8_t metatile_id) {
    MjkObjects_SetRoomMetatile(game, &game->room, 0u, yx, metatile_id);
}

static void play_sfx(MjkGame* game, MjkSoundId snd) {
    game->last_sound_event = snd;
    if (game->sound_queue_count < MJK_GAME_SOUND_QUEUE_CAP) {
        game->sound_queue[game->sound_queue_count++] = snd;
    }
}

void MjkObjects_InitForRoom(MjkObjectSystem* obj, MjkGame* game) {
    if (!obj) {
        return;
    }
    /* O-05 (prg.asm:423-427, 4980-5006): conservar power_coin_acc entre salas si no está armada. */
    uint8_t saved_acc = (obj->power_coin_armed == 0u) ? obj->power_coin_acc : 0u;
    memset(obj, 0, sizeof(*obj));
    obj->power_coin_acc = saved_acc;
    if (!game) {
        return;
    }
    if (game->room.is_bomb_room) {
        game->fire_bombs_collected = 0u;
        for (uint8_t i = 0u; i < game->room.object_count; ++i) {
            if ((game->room.objects[i].ram_state & 0x4Fu) == 0x45u) {
                obj->spark.active = true;
                obj->spark.yx = game->room.objects[i].yx;
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
    if (!game || !game->pkg || game->room.is_bomb_room) {
        return false;
    }
    if (game->mighty_coins == 0u || game->mighty_level >= 3u) {
        return false;
    }
    game->mighty_coins--;
    game->mighty_level++;
    const uint8_t* ap_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_MIGHTY_A_PRESSES, 4u);
    game->mighty_presses_left = ap_tbl ? ap_tbl[game->mighty_level & 3u] : 20u;
    MjkPpu_SetJackMightyColor(&game->ppu, game->pkg, game->mighty_level);
    play_sfx(game, MJK_SND_USED_MIGHTY_COIN);
    if (out_trigger_transmute && game->mighty_level == 3u) {
        *out_trigger_transmute = true;
    }
    return true;
}

static void collect_bomb(MjkObjectSystem* obj, MjkGame* game,
                         MjkRoomState* target_room, uint8_t target_nt,
                         bool in_right, MjkRoomPersistSlot* slot, uint8_t obj_idx) {
    MjkRoomObject* ro = &target_room->objects[obj_idx];
    uint8_t st = ro->ram_state;
    bool lit = ((st & 0x40u) != 0u);
    bool was_first_bomb = !obj->first_bomb_picked;

    if (was_first_bomb) {
        obj->first_bomb_picked = true;
        obj->maybe_bomb_thing = (!target_room->is_bomb_room && (st & 0x80u) != 0u) ? 1u : 0u;
    } else if (!target_room->is_bomb_room && !lit) {
        /* Coger una bomba apagada en pasillo descalifica el bonus de orden (loc_9F09, prg.asm:5062-5072). */
        obj->maybe_bomb_thing |= 2u;
    }

    bool is_final_order_bonus = (!target_room->is_bomb_room && !was_first_bomb &&
                                 st == 0xC5u && obj->maybe_bomb_thing == 1u);
    /* O-05 (prg.asm:4980-5006): congelar power_coin_acc mientras PowerCoin armada o activa. */
    bool triggered_power_coin = false;
    if (obj->power_coin_armed == 0u && game->enemies.transmute_timer == 0u) {
        obj->power_coin_acc += lit ? 2u : 1u;
        if (obj->power_coin_acc >= 20u) {
            obj->power_coin_acc = 0u;
            triggered_power_coin = true;
        }
    }

    if (is_final_order_bonus) {
        /* O-04 (prg.asm:5009-5010, 5041-5059): $C5 final suma Score_200 + Score_10000 (10.200). */
        MjkGame_AddScore(game, MJK_SCORE_200);
        MjkGame_AddScore(game, MJK_SCORE_10000);
        obj->maybe_bomb_thing |= 0x80u;
        if (!triggered_power_coin) {
            play_sfx(game, MJK_SND_BOMB_BONUS);
        }
    } else if (lit) {
        MjkGame_AddScore(game, MJK_SCORE_200);
        if (!triggered_power_coin) {
            play_sfx(game, MJK_SND_BOMB_LIT);
        }
    } else {
        MjkGame_AddScore(game, MJK_SCORE_100);
        if (!triggered_power_coin) {
            play_sfx(game, MJK_SND_BOMB_UNLIT);
        }
    }

    ro->ram_state = 0x0Fu;
    if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
        slot->objects[ro->ram_index] = 0x0Fu;
    }
    MjkObjects_SetRoomMetatile(game, target_room, target_nt, ro->yx, 0x2Fu);

    if (lit && target_room->is_bomb_room) {
        game->fire_bombs_collected++;
    }
    if (triggered_power_coin) {
        if (target_room->is_bomb_room) {
            MjkEnemy_SpawnPowerCoinBombRoom(&game->enemies, game);
            play_sfx(game, MJK_SND_POWER_COIN_SPAWN);
        } else {
            obj->power_coin_armed = 1u;
            play_sfx(game, MJK_SND_BOMB_CHEST_BONUS);
        }
    }

    /* Slot único puff+popup (sub_A270/sub_A29C, prg.asm:5647-5747): recoger otra bomba sobrescribe el anterior. */
    obj->puff.active = true;
    obj->puff.phase = 0u;
    obj->puff.timer = 32u;
    obj->puff.bomb_type = lit ? 1u : 0u;
    obj->puff.mult_tile = 0x0Fu;
    int32_t scroll_x = (int32_t)(game->camera.scroll_x_fixed >> 8);
    int32_t scroll_y = (int32_t)(game->camera.scroll_y_fixed >> 8);
    bool horiz = game->room.is_horizontal;
    int32_t puff_sx = (int32_t)(((ro->yx & 0x0Fu) << 4u) + 8u) + ((in_right && horiz) ? 256 : 0) - scroll_x;
    int32_t puff_sy = (int32_t)((ro->yx & 0xF0u) + 8u) + ((in_right && !horiz) ? 240 : 0) - scroll_y;
    if (puff_sx < 0) { puff_sx = 0; }
    if (puff_sx > 255) { puff_sx = 255; }
    if (puff_sy < 0) { puff_sy = 0; }
    if (puff_sy > 239) { puff_sy = 239; }
    obj->puff.x = MjkUQ8_FromInt((uint8_t)puff_sx);
    obj->puff.y = MjkUQ8_FromInt((uint8_t)puff_sy);

    if (target_room->is_bomb_room) {
        bool any_remaining = false;
        for (uint8_t k = 0u; k < target_room->object_count; ++k) {
            if ((target_room->objects[k].ram_state & 0x0Fu) == 0x05u) {
                any_remaining = true;
                break;
            }
        }
        if (!any_remaining) {
            obj->spark.active = false;
            const uint8_t* entry_tbl = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_MAYBE_ENTRY_TYPE, 32u);
            uint8_t exit_door = entry_tbl ? entry_tbl[(target_room->room_id & 0x0Fu) * 2u + 1u] : 0u;
            for (uint8_t d = 0u; d < target_room->object_count; ++d) {
                const MjkRoomObject* dro = &target_room->objects[d];
                if ((dro->opcode & 0xF0u) == 0x80u && (dro->opcode & 0x0Fu) == exit_door) {
                    MjkDoors_OpenDoorInRoom(game, target_room, target_nt, d);
                    break;
                }
            }
            game->entry_door_type = exit_door;
            game->state = MJK_GS_ANIMATE_DOOR_OPEN;
            game->sub_state = 0u;
        } else if (was_first_bomb || lit) {
            /* Solo avanza la mecha si era la 1.ª bomba o la bomba encendida (prg.asm:5109-5161). */
            obj->spark.active = false;
            for (uint8_t step = 1u; step < target_room->object_count; ++step) {
                uint8_t n = (uint8_t)((obj_idx + step) % target_room->object_count);
                if ((target_room->objects[n].ram_state & 0x0Fu) == 0x05u) {
                    target_room->objects[n].ram_state |= 0x40u;
                    if (slot && target_room->objects[n].ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
                        slot->objects[target_room->objects[n].ram_index] |= 0x40u;
                    }
                    obj->spark.active = true;
                    obj->spark.yx = target_room->objects[n].yx;
                    obj->spark.tile_id = 0x0Du;
                    obj->spark.anim_timer = 0u;
                    break;
                }
            }
        }
    } else if (was_first_bomb || lit) {
        obj->spark.active = false;
        bool lit_in_room = false;
        for (uint8_t n = (uint8_t)(obj_idx + 1u); n < target_room->object_count; ++n) {
            if ((target_room->objects[n].ram_state & 0x0Fu) == 0x05u) {
                target_room->objects[n].ram_state |= 0x40u;
                if (slot && target_room->objects[n].ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
                    slot->objects[target_room->objects[n].ram_index] |= 0x40u;
                }
                lit_in_room = true;
                break;
            }
        }
        /* loc_9F31 (prg.asm:5088-5103): propagar mecha oculta al siguiente slot de sala de la sección. */
        if (!lit_in_room && slot) {
            size_t cur_slot_idx = MJK_ROOM_PERSIST_SLOTS;
            for (size_t s = 0u; s < MJK_ROOM_PERSIST_SLOTS; ++s) {
                if (&game->persist.slots[s] == slot) {
                    cur_slot_idx = s;
                    break;
                }
            }
            for (size_t s = cur_slot_idx + 1u; s < MJK_ROOM_PERSIST_SLOTS && !lit_in_room; ++s) {
                MjkRoomPersistSlot* ns = &game->persist.slots[s];
                if (ns->room_id == 0u) {
                    break;
                }
                for (uint8_t k = 0u; k < MJK_ROOM_SLOT_OBJ_BYTES; ++k) {
                    if (ns->objects[k] == 0xFFu) {
                        break;
                    }
                    if ((ns->objects[k] & 0x7Fu) == 0x05u) {
                        ns->objects[k] |= 0x40u;
                        if (game->camera.neighbor_room_id == ns->room_id) {
                            for (uint8_t r = 0u; r < game->right_room.object_count; ++r) {
                                if (game->right_room.objects[r].ram_index == k) {
                                    game->right_room.objects[r].ram_state |= 0x40u;
                                    break;
                                }
                            }
                        }
                        lit_in_room = true;
                        break;
                    }
                }
            }
        }
    }
}

static void open_chest(MjkObjectSystem* obj, MjkGame* game,
                       MjkRoomState* target_room, uint8_t target_nt,
                       bool in_right, MjkRoomPersistSlot* slot, uint8_t obj_idx) {
    MjkRoomObject* ro = &target_room->objects[obj_idx];
    uint8_t item_id = (uint8_t)(ro->opcode & 0x0Fu);
    if ((item_id == 7u || item_id == 9u) && game->collected_1up != 0u) {
        item_id = 8u;
    } else if (item_id == (uint8_t)MJK_ENEMY_BROTHER && game->collected_1up != 0u) {
        item_id = (uint8_t)MJK_ENEMY_CAPTIVE;
    }

    int32_t scroll_x = (int32_t)(game->camera.scroll_x_fixed >> 8);
    int32_t scroll_y = (int32_t)(game->camera.scroll_y_fixed >> 8);
    bool horiz = game->room.is_horizontal;
    int32_t sx_i = (int32_t)(((ro->yx & 0x0Fu) << 4u) + 8u) + ((in_right && horiz) ? 256 : 0) - scroll_x;
    int32_t sy_i = (int32_t)((ro->yx & 0xF0u) + 8u) + ((in_right && !horiz) ? 240 : 0) - scroll_y;
    if (sx_i < 0) { sx_i = 0; }
    if (sx_i > 255) { sx_i = 255; }
    if (sy_i < 0) { sy_i = 0; }
    if (sy_i > 239) { sy_i = 239; }
    uint8_t sx = (uint8_t)sx_i;
    uint8_t sy = (uint8_t)sy_i;

    uint8_t c_my = (uint8_t)((ro->yx >> 4u) & 0x0Fu);
    uint8_t c_mx = (uint8_t)(ro->yx & 0x0Fu);
    if (c_my < MJK_ROOM_MT_ROWS && c_mx < MJK_ROOM_MT_COLS) {
        target_room->static_metatiles[c_my][c_mx] = 0x2Fu;
    }

    const uint8_t* item_to_tile = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_ITEM_TO_TILE, 18u);
    if (item_id < 9u) {
        ro->ram_state = item_id;
        if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
            slot->objects[ro->ram_index] = item_id;
        }
        if (item_to_tile) {
            MjkObjects_SetRoomMetatile(game, target_room, target_nt, ro->yx, item_to_tile[item_id]);
        }
    } else {
        ro->ram_state = 0x0Fu;
        if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
            slot->objects[ro->ram_index] = 0x0Fu;
        }
        MjkObjects_SetRoomMetatile(game, target_room, target_nt, ro->yx, 0x2Fu);
        MjkEnemy_SpawnCollectible(&game->enemies, (MjkEnemyType)item_id, sx, sy);
        if (item_id == (uint8_t)MJK_ENEMY_EXTRA_COIN || item_id == (uint8_t)MJK_ENEMY_BONUS_COIN) {
            play_sfx(game, MJK_SND_BONUS_COIN_SPAWN);
        }
    }

    if (obj->power_coin_armed != 0u) {
        obj->power_coin_armed = 0u;
        MjkEnemy_SpawnCollectible(&game->enemies, MJK_ENEMY_POWER_COIN, sx, sy);
    }
}

static MjkActionResult pickup_item(MjkGame* game, MjkRoomState* target_room, uint8_t target_nt,
                                   bool in_right, MjkRoomPersistSlot* slot, uint8_t obj_idx) {
    (void)in_right;
    MjkRoomObject* ro = &target_room->objects[obj_idx];
    uint8_t item_id = ro->ram_state;
    ro->ram_state = 0x0Fu;
    if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
        slot->objects[ro->ram_index] = 0x0Fu;
    }
    MjkObjects_SetRoomMetatile(game, target_room, target_nt, ro->yx, 0x2Fu);

    switch (item_id) {
        case MJK_ITEM_100_BAG:
            MjkGame_AddScore(game, MJK_SCORE_100);
            play_sfx(game, MJK_SND_BOMB_LIT);
            break;
        case MJK_ITEM_300_BAG:
            MjkGame_AddScore(game, MJK_SCORE_300);
            play_sfx(game, MJK_SND_BOMB_LIT);
            break;
        case MJK_ITEM_1000_BAG:
            MjkGame_AddScore(game, MJK_SCORE_1000);
            play_sfx(game, MJK_SND_BOMB_LIT);
            break;
        case MJK_ITEM_GOLD_COIN:
            MjkGame_AddScore(game, MJK_SCORE_500);
            game->gold_coins++;
            play_sfx(game, MJK_SND_COIN_COLLECTED);
            break;
        case MJK_ITEM_MIGHTY_DRINK: {
            MjkGame_AddScore(game, MJK_SCORE_1000);
            play_sfx(game, MJK_SND_TIME_PICKUP);
            uint8_t carry = 0u;
            game->stage_timer_bcd = MjkBcd_AddByte(game->stage_timer_bcd, 0x10u, 0u, &carry);
            if (carry != 0u || game->stage_timer_bcd >= 0xA0u) {
                return MjkGame_TriggerTortureRoom(game);
            }
            break;
        }
        case MJK_ITEM_SPHINX:
            MjkGame_AddScore(game, MJK_SCORE_10000);
            MjkDoors_HandleSphinxEvent(game, ro->extra);
            break;
        case MJK_ITEM_TECMO_PLATE:
            MjkGame_AddScore(game, MJK_SCORE_100000);
            game->lives++;
            game->extra_lives_found++;
            game->collected_1up = 1u;
            game->one_up_timer = 0x40u;
            play_sfx(game, MJK_SND_1UP_CAPTIVE);
            break;
        case MJK_ITEM_MIGHTY_COIN:
            MjkGame_AddScore(game, MJK_SCORE_1000);
            game->mighty_coins++;
            play_sfx(game, MJK_SND_COIN_COLLECTED);
            if (game->mighty_coins >= 10u) {
                return MjkGame_TriggerTortureRoom(game);
            }
            break;
        case MJK_ITEM_CRYSTAL_BALL_1:
            game->crystal_balls_mask |= 1u;
            game->gdv_bcd = MjkGame_AddGdvBcd(game->gdv_bcd, 1u);
            play_sfx(game, MJK_SND_CRYSTAL_BALL);
            break;
        case MJK_ITEM_CRYSTAL_BALL_2:
            game->crystal_balls_mask |= 2u;
            game->gdv_bcd = MjkGame_AddGdvBcd(game->gdv_bcd, 1u);
            play_sfx(game, MJK_SND_CRYSTAL_BALL);
            break;
        case MJK_ITEM_KING:
        case MJK_ITEM_QUEEN:
        case MJK_ITEM_PRINCESS:
            /* JTPickup_Family (prg.asm:5227-5237): 0 pts y loc_AACF con X = item_id + 6 ($11..$13). */
            MjkDoors_TriggerEventDirect(game, (uint8_t)(item_id + 6u));
            break;
        case MJK_ITEM_BEELZEBUT_MASK:
            /* JTPickup_Beelzebub (prg.asm:5238-5249): 1.000.000 pts, música y luego loc_AACF ($14). */
            MjkGame_AddScore(game, MJK_SCORE_1000000);
            play_sfx(game, MJK_MUS_COLLECTED_FULL_FAMILY);
            MjkDoors_TriggerEventDirect(game, (uint8_t)(item_id + 6u));
            break;
        default:
            break;
    }
    return MJK_ACTION_CONTINUE;
}

MjkActionResult MjkObjects_Update(MjkObjectSystem* obj,
                                  MjkGame* game,
                                  const MjkInputState* input) {
    if (!obj || !game || !game->pkg) {
        return MJK_ACTION_CONTINUE;
    }

    if (input && (input->just_pressed & MJK_BTN_B) != 0u) {
        bool transmute = false;
        MjkObjects_UseMightyCoin(game, &transmute);
        if (transmute) {
            MjkEnemy_ActivatePowerCoinEx(&game->enemies, game);
        }
    }

    if (obj->puff.active) {
        int32_t pnx = (int32_t)obj->puff.x + (int32_t)game->camera.delta_cam_x * 256;
        int32_t pny = (int32_t)obj->puff.y + (int32_t)game->camera.delta_cam_y * 256;
        if (pnx < 0 || pnx >= 0x10000 || pny < 0 || pny >= 0x10000) {
            obj->puff.active = false;
        } else {
            obj->puff.x = (mjk_uq8_8_t)pnx;
            obj->puff.y = (mjk_uq8_8_t)pny;
        }
        if (obj->puff.active && obj->puff.timer > 0u) {
            obj->puff.timer--;
        }
        if (obj->puff.active && obj->puff.timer == 0u) {
            if (obj->puff.phase == 0u) {
                obj->puff.phase = 1u;
                obj->puff.timer = 32u;
                obj->puff.mult_tile = (game->room.is_bomb_room &&
                                       game->score_multiplier >= 1u &&
                                       game->score_multiplier <= 4u)
                    ? (uint8_t)(0x3Bu + game->score_multiplier) : 0x0Fu;
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

    int32_t wx = (int32_t)MjkUQ8_Int(game->player.pos_x) + (int32_t)(game->camera.scroll_x_fixed >> 8);
    int32_t wy = (int32_t)MjkUQ8_Int(game->player.pos_y) + (int32_t)(game->camera.scroll_y_fixed >> 8);
    bool in_neighbor = false;
    int32_t local_x = wx;
    int32_t local_y = wy;
    if (!game->room.is_horizontal && wy >= 240 && game->camera.neighbor_room_id != 0u) {
        in_neighbor = true;
        local_y = wy - 240;
    } else if (wx >= 256 && game->camera.neighbor_room_id != 0u) {
        in_neighbor = true;
        local_x = wx - 256;
    }
    if (local_x < 0) { local_x = 0; }
    if (local_x > 255) { local_x = 255; }
    if (local_y < 0) { local_y = 0; }
    if (local_y > 239) { local_y = 239; }
    bool in_right = in_neighbor;
    MjkRoomState* target_room = in_neighbor ? &game->right_room : &game->room;
    uint8_t target_nt = in_neighbor ? 1u : 0u;
    MjkRoomPersistSlot* slot = get_slot_for_room(game, target_room->room_id);

    uint8_t jx = (uint8_t)local_x;
    uint8_t jy = (uint8_t)local_y;
    uint8_t j_yx = (uint8_t)((jy & 0xF0u) | (jx >> 4u));
    uint8_t feet_yx = (uint8_t)(((uint8_t)((local_y + 8 < 240) ? (local_y + 8) : 239) & 0xF0u) | (jx >> 4u));

    bool facing_right = (game->player.facing_attr == 0x04u);
    if (input && (input->held & MJK_BTN_RIGHT) != 0u && (input->held & MJK_BTN_LEFT) == 0u) {
        facing_right = true;
    } else if (input && (input->held & MJK_BTN_LEFT) != 0u && (input->held & MJK_BTN_RIGHT) == 0u) {
        facing_right = false;
    }
    int32_t side_wx = facing_right ? (wx + 9) : (wx - 9);
    bool has_side = (side_wx >= 0 && side_wx < 512 &&
                     (side_wx < 256 || (game->room.is_horizontal && game->camera.neighbor_room_id != 0u)));
    bool side_in_right = (!game->room.is_horizontal) ? in_neighbor
                                                     : (side_wx >= 256 && game->camera.neighbor_room_id != 0u);
    MjkRoomState* side_room = side_in_right ? &game->right_room : &game->room;
    uint8_t side_nt = side_in_right ? 1u : 0u;
    uint8_t side_yx = has_side
        ? (uint8_t)((jy & 0xF0u) | (((uint32_t)side_wx & 0xFFu) >> 4u))
        : 0xFFu;
    bool has_head = (game->player.vel_y < 0 || (game->player.collision_flags & MJK_CF_CEILING) != 0u) &&
                    (jy >= 8u);
    uint8_t head_yx = has_head ? (uint8_t)(((uint8_t)(jy - 8u) & 0xF0u) | (jx >> 4u)) : 0xFFu;

    if (game->player.just_jumped && obj->chest_target_yx != 0u) {
        bool chest_in_right = (obj->chest_target_room_id == game->right_room.room_id &&
                               game->camera.neighbor_room_id != 0u);
        MjkRoomState* chest_room = chest_in_right ? &game->right_room : &game->room;
        uint8_t chest_nt = chest_in_right ? 1u : 0u;
        MjkRoomPersistSlot* chest_slot = get_slot_for_room(game, chest_room->room_id);
        for (uint8_t i = 0u; i < chest_room->object_count; ++i) {
            MjkRoomObject* ro = &chest_room->objects[i];
            uint8_t st = ro->ram_state;
            if (ro->yx == obj->chest_target_yx && st == 0x33u) {
                /* Bloque secreto $33 (MaybeOpenHiddenBlock, prg.asm:5392-5458): romper saltando encima. */
                if (obj->secret_block_yx != ro->yx) {
                    obj->secret_block_yx = ro->yx;
                    obj->secret_hits_left = (uint8_t)(ro->opcode & 0x0Fu);
                }
                if (obj->secret_hits_left > 0u) {
                    obj->secret_hits_left--;
                }
                if (obj->secret_hits_left == 0u) {
                    ro->ram_state = 0x0Fu;
                    if (chest_slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
                        chest_slot->objects[ro->ram_index] = 0x0Fu;
                    }
                    MjkObjects_SetRoomMetatile(game, chest_room, chest_nt, ro->yx, 0x2Fu);
                    uint8_t s_my = (uint8_t)((ro->yx >> 4u) & 0x0Fu);
                    uint8_t s_mx = (uint8_t)(ro->yx & 0x0Fu);
                    if (s_my < MJK_ROOM_MT_ROWS && s_mx < MJK_ROOM_MT_COLS) {
                        chest_room->static_metatiles[s_my][s_mx] = 0x2Fu;
                    }
                    play_sfx(game, MJK_SND_SECRET_BLOCK_REVEALED);
                }
                break;
            }
            if (ro->yx == obj->chest_target_yx &&
                ((st & 0x1Fu) == 0x10u || (st & 0x1Fu) == 0x11u)) {
                bool is_orange = ((st & 0x1Fu) == 0x11u);
                if (is_orange && game->mighty_level == 0u) {
                    break;
                }
                if ((st & 0x20u) != 0u) {
                    ro->ram_state &= (uint8_t)0xDFu;
                    if (chest_slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
                        chest_slot->objects[ro->ram_index] = ro->ram_state;
                    }
                    MjkObjects_SetRoomMetatile(game, chest_room, chest_nt, ro->yx,
                                               is_orange ? 0x4Au : 0x28u);
                    /* O-06 (prg.asm:5392-5431): revelar cofre oculto también spawnea PowerCoin armada. */
                    if (obj->power_coin_armed != 0u) {
                        obj->power_coin_armed = 0u;
                        int32_t scx = (int32_t)(game->camera.scroll_x_fixed >> 8);
                        int32_t scy = (int32_t)(game->camera.scroll_y_fixed >> 8);
                        bool horiz = game->room.is_horizontal;
                        int32_t cx = (int32_t)(((ro->yx & 0x0Fu) << 4u) + 8u) + ((chest_in_right && horiz) ? 256 : 0) - scx;
                        int32_t cy = (int32_t)((ro->yx & 0xF0u) + 8u) + ((chest_in_right && !horiz) ? 240 : 0) - scy;
                        if (cx < 0) { cx = 0; } else if (cx > 255) { cx = 255; }
                        if (cy < 0) { cy = 0; } else if (cy > 239) { cy = 239; }
                        MjkEnemy_SpawnCollectible(&game->enemies, MJK_ENEMY_POWER_COIN, (uint8_t)cx, (uint8_t)cy);
                    }
                } else {
                    open_chest(obj, game, chest_room, chest_nt, chest_in_right, chest_slot, i);
                }
                break;
            }
        }
        obj->chest_target_yx = 0u;
    } else if (!game->player.just_jumped) {
        obj->chest_target_yx = 0u;
    }

    for (uint8_t i = 0u; i < target_room->object_count; ++i) {
        MjkRoomObject* ro = &target_room->objects[i];
        uint8_t st = ro->ram_state;
        if (st == 0x0Fu) {
            continue;
        }

        if ((st & 0x0Fu) == 0x05u && ro->yx == j_yx) {
            collect_bomb(obj, game, target_room, target_nt, in_right, slot, i);
            continue;
        }

        if ((st & 0x1Fu) == 0x10u || (st & 0x1Fu) == 0x11u) {
            bool is_orange = ((st & 0x1Fu) == 0x11u);
            if (is_orange && game->mighty_level == 0u) {
                continue;
            }
            if (!game->player.in_air && ro->yx == feet_yx) {
                obj->chest_target_yx = ro->yx;
                obj->chest_target_room_id = target_room->room_id;
            }
            bool hit_mighty2 = (game->mighty_level >= 2u && (st & 0x20u) == 0u) &&
                               (ro->yx == j_yx ||
                                (has_head && ro->yx == head_yx) ||
                                (has_side && side_room == target_room && ro->yx == side_yx));
            if (hit_mighty2) {
                open_chest(obj, game, target_room, target_nt, in_right, slot, i);
                continue;
            }
        }

        if (st == 0x33u) {
            if (!game->player.in_air && ro->yx == feet_yx) {
                obj->chest_target_yx = ro->yx;
                obj->chest_target_room_id = target_room->room_id;
            }
            continue;
        }

        if (st <= 0x0Eu && ro->yx == j_yx) {
            MjkActionResult res = pickup_item(game, target_room, target_nt, in_right, slot, i);
            if (res != MJK_ACTION_CONTINUE) {
                return res;
            }
        }
    }

    if (has_side && side_room != target_room && game->mighty_level >= 2u) {
        MjkRoomPersistSlot* side_slot = get_slot_for_room(game, side_room->room_id);
        for (uint8_t i = 0u; i < side_room->object_count; ++i) {
            MjkRoomObject* ro = &side_room->objects[i];
            uint8_t st = ro->ram_state;
            if (((st & 0x1Fu) == 0x10u || (st & 0x1Fu) == 0x11u) &&
                (st & 0x20u) == 0u && ro->yx == side_yx) {
                open_chest(obj, game, side_room, side_nt, side_in_right, side_slot, i);
                break;
            }
        }
    }

    return MJK_ACTION_CONTINUE;
}

uint8_t MjkObjects_RenderOam(const MjkObjectSystem* obj,
                             const MjkAssetPackage* pkg,
                             MjkPpu* ppu,
                             uint8_t start_slot) {
    if (!obj || !pkg || !ppu) {
        return start_slot;
    }
    uint8_t slot = start_slot;
    if (obj->spark.active && slot < MJK_PPU_OAM_COUNT) {
        uint8_t sx = (uint8_t)(((obj->spark.yx & 0x0Fu) << 4u) + 8u);
        uint8_t sy = (uint8_t)((obj->spark.yx & 0xF0u) - 4u);
        MjkPpu_SetOam(ppu, slot++, sx, sy, obj->spark.tile_id, 0x12u);
    }
    if (obj->puff.active) {
        uint8_t px = MjkUQ8_Int(obj->puff.x);
        uint8_t py = MjkUQ8_Int(obj->puff.y);
        if (obj->puff.phase == 0u) {
            slot = MjkPpu_WriteMetasprite(ppu, pkg, 0x25u, px, py, 0x00u, slot);
        } else if ((size_t)slot + 4u <= MJK_PPU_OAM_COUNT) {
            uint8_t lx = (px >= 8u) ? (uint8_t)(px - 8u) : 0u;
            uint8_t ty = (py >= 8u) ? (uint8_t)(py - 8u) : 0u;
            MjkPpu_SetOam(ppu, slot++, lx, ty, (uint8_t)(0x31u + obj->puff.bomb_type), 0u);
            MjkPpu_SetOam(ppu, slot++, px, ty, 0x30u, 0u);
            MjkPpu_SetOam(ppu, slot++, lx, py, 0x0Fu, 0u);
            MjkPpu_SetOam(ppu, slot++, px, py, obj->puff.mult_tile, 0u);
        }
    }
    return slot;
}
