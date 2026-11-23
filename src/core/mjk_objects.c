/**
 * @file mjk_objects.c
 * @brief Implementación de bombas, mechas, cofres, secretos, ítems y Mighty (T-19, T-20).
 */

#include "mjk_objects.h"
#include "mjk_game.h"
#include <string.h>

static MjkRoomPersistSlot* get_active_slot(MjkGame* game) {
    for (size_t i = 0u; i < MJK_ROOM_PERSIST_SLOTS; ++i) {
        if (game->persist.slots[i].room_id == game->current_room_id) {
            return &game->persist.slots[i];
        }
    }
    if (game->persist.scratch.room_id == game->current_room_id) {
        return &game->persist.scratch;
    }
    return NULL;
}

void MjkObjects_SetMetatile(MjkGame* game, uint8_t yx, uint8_t metatile_id) {
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
    game->room.metatiles[my][mx] = metatile_id;
    game->room.attributes[my][mx] = attr;
    MjkPpu_SetMetatile(&game->ppu, game->pkg, 0u, mx, my, metatile_id, attr);
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
    memset(obj, 0, sizeof(*obj));
    if (!game) {
        return;
    }
    for (uint8_t i = 0u; i < game->room.object_count; ++i) {
        if ((game->room.objects[i].ram_state & 0x4Fu) == 0x45u) {
            obj->spark.active = true;
            obj->spark.yx = game->room.objects[i].yx;
            obj->spark.tile_id = 0x0Du;
            break;
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
                         MjkRoomPersistSlot* slot, uint8_t obj_idx) {
    MjkRoomObject* ro = &game->room.objects[obj_idx];
    uint8_t st = ro->ram_state;
    bool lit = ((st & 0x40u) != 0u);

    if (!obj->first_bomb_picked) {
        obj->first_bomb_picked = true;
        obj->maybe_bomb_thing = ((st & 0x80u) != 0u) ? 1u : 0u;
    }

    if (st == 0xC5u && obj->maybe_bomb_thing == 1u) {
        MjkGame_AddScore(game, MJK_SCORE_10000);
        obj->maybe_bomb_thing |= 0x80u;
        play_sfx(game, MJK_SND_BOMB_BONUS);
    } else if (lit) {
        MjkGame_AddScore(game, MJK_SCORE_200);
        play_sfx(game, MJK_SND_BOMB_LIT);
    } else {
        MjkGame_AddScore(game, MJK_SCORE_100);
        play_sfx(game, MJK_SND_BOMB_UNLIT);
    }

    ro->ram_state = 0x0Fu;
    if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
        slot->objects[ro->ram_index] = 0x0Fu;
    }
    MjkObjects_SetMetatile(game, ro->yx, 0x2Fu);

    if (lit) {
        game->fire_bombs_collected++;
        obj->power_coin_acc += 2u;
    } else {
        obj->power_coin_acc += 1u;
    }
    if (obj->power_coin_acc >= 20u) {
        obj->power_coin_acc = 0u;
        obj->power_coin_armed = 1u;
        if (!game->room.is_bomb_room) {
            play_sfx(game, MJK_SND_BOMB_CHEST_BONUS);
        }
    }

    obj->puff.active = true;
    obj->puff.phase = 0u;
    obj->puff.timer = 32u;
    obj->puff.bomb_type = lit ? 1u : 0u;
    obj->puff.x = MjkUQ8_FromInt((uint8_t)(((ro->yx & 0x0Fu) << 4u) + 8u));
    obj->puff.y = MjkUQ8_FromInt((uint8_t)((ro->yx & 0xF0u) + 8u));

    obj->spark.active = false;
    for (uint8_t n = (uint8_t)(obj_idx + 1u); n < game->room.object_count; ++n) {
        if ((game->room.objects[n].ram_state & 0x0Fu) == 0x05u) {
            game->room.objects[n].ram_state |= 0x40u;
            if (slot && game->room.objects[n].ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
                slot->objects[game->room.objects[n].ram_index] |= 0x40u;
            }
            if (game->room.is_bomb_room) {
                obj->spark.active = true;
                obj->spark.yx = game->room.objects[n].yx;
            }
            break;
        }
    }
    if (game->room.is_bomb_room) {
        obj->fire_bombs_total++;
        if (obj->fire_bombs_total >= 23u) {
            obj->spark.active = false;
            game->state = MJK_GS_ANIMATE_DOOR_OPEN;
            game->sub_state = 0u;
        }
    }
}

static void open_chest(MjkGame* game, MjkRoomPersistSlot* slot, uint8_t obj_idx) {
    MjkRoomObject* ro = &game->room.objects[obj_idx];
    uint8_t item_id = (uint8_t)(ro->opcode & 0x0Fu);
    if ((item_id == 7u || item_id == 9u) && game->collected_1up != 0u) {
        item_id = 8u;
    }

    const uint8_t* item_to_tile = MjkAssets_ResolveCpuPtr(game->pkg, MJK_ADDR_ITEM_TO_TILE, 18u);
    if (item_id < 9u) {
        ro->ram_state = item_id;
        if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
            slot->objects[ro->ram_index] = item_id;
        }
        if (item_to_tile) {
            MjkObjects_SetMetatile(game, ro->yx, item_to_tile[item_id]);
        }
    } else {
        ro->ram_state = 0x0Fu;
        if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
            slot->objects[ro->ram_index] = 0x0Fu;
        }
        MjkObjects_SetMetatile(game, ro->yx, 0x2Fu);
    }
}

static MjkActionResult pickup_item(MjkGame* game, MjkRoomPersistSlot* slot, uint8_t obj_idx) {
    MjkRoomObject* ro = &game->room.objects[obj_idx];
    uint8_t item_id = ro->ram_state;
    ro->ram_state = 0x0Fu;
    if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
        slot->objects[ro->ram_index] = 0x0Fu;
    }
    MjkObjects_SetMetatile(game, ro->yx, 0x2Fu);

    switch (item_id) {
        case MJK_ITEM_100_BAG:    MjkGame_AddScore(game, MJK_SCORE_100); break;
        case MJK_ITEM_300_BAG:    MjkGame_AddScore(game, MJK_SCORE_300); break;
        case MJK_ITEM_1000_BAG:   MjkGame_AddScore(game, MJK_SCORE_1000); break;
        case MJK_ITEM_GOLD_COIN:
            MjkGame_AddScore(game, MJK_SCORE_500);
            game->gold_coins++;
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
            break;
        case MJK_ITEM_TECMO_PLATE:
            MjkGame_AddScore(game, MJK_SCORE_100000);
            game->lives++;
            game->extra_lives_found++;
            game->collected_1up = 1u;
            play_sfx(game, MJK_SND_1UP_CAPTIVE);
            break;
        case MJK_ITEM_MIGHTY_COIN:
            MjkGame_AddScore(game, MJK_SCORE_1000);
            game->mighty_coins++;
            if (game->mighty_coins >= 10u) {
                return MjkGame_TriggerTortureRoom(game);
            }
            break;
        case MJK_ITEM_CRYSTAL_BALL_1:
            game->crystal_balls_mask |= 1u;
            play_sfx(game, MJK_SND_CRYSTAL_BALL);
            break;
        case MJK_ITEM_CRYSTAL_BALL_2:
            game->crystal_balls_mask |= 2u;
            play_sfx(game, MJK_SND_CRYSTAL_BALL);
            break;
        case MJK_ITEM_KING:
        case MJK_ITEM_QUEEN:
        case MJK_ITEM_PRINCESS:
            MjkGame_AddScore(game, MJK_SCORE_10000);
            break;
        case MJK_ITEM_BEELZEBUT_MASK:
            MjkGame_AddScore(game, MJK_SCORE_1000000);
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
            MjkEnemy_ActivatePowerCoin(&game->enemies);
        }
    }

    if (obj->puff.active) {
        obj->puff.x = (mjk_uq8_8_t)(obj->puff.x + (game->camera.delta_cam_x << 8));
        obj->puff.y = (mjk_uq8_8_t)(obj->puff.y + (game->camera.delta_cam_y << 8));
        if (obj->puff.timer > 0u) {
            obj->puff.timer--;
        }
        if (obj->puff.timer == 0u) {
            if (obj->puff.phase == 0u) {
                obj->puff.phase = 1u;
                obj->puff.timer = 32u;
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

    MjkRoomPersistSlot* slot = get_active_slot(game);
    uint8_t jx = (uint8_t)(MjkUQ8_Int(game->player.pos_x) + (uint8_t)(game->camera.scroll_x_fixed >> 8));
    uint8_t jy = (uint8_t)(MjkUQ8_Int(game->player.pos_y) + (uint8_t)(game->camera.scroll_y_fixed >> 8));
    uint8_t j_yx = (uint8_t)((jy & 0xF0u) | (jx >> 4u));
    uint8_t feet_yx = (uint8_t)(((jy + 12u) & 0xF0u) | (jx >> 4u));

    for (uint8_t i = 0u; i < game->room.object_count; ++i) {
        MjkRoomObject* ro = &game->room.objects[i];
        uint8_t st = ro->ram_state;
        if (st == 0x0Fu) {
            continue;
        }

        if ((st & 0x0Fu) == 0x05u && ro->yx == j_yx) {
            collect_bomb(obj, game, slot, i);
            continue;
        }

        if ((st & 0x1Fu) == 0x10u || (st & 0x1Fu) == 0x11u) {
            bool is_orange = ((st & 0x1Fu) == 0x11u);
            if (is_orange && game->mighty_level == 0u) {
                continue;
            }
            if (ro->yx == feet_yx && game->player.just_jumped) {
                if ((st & 0x20u) != 0u) {
                    ro->ram_state &= (uint8_t)0xDFu;
                    if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
                        slot->objects[ro->ram_index] = ro->ram_state;
                    }
                    MjkObjects_SetMetatile(game, ro->yx, is_orange ? 0x4Au : 0x28u);
                } else {
                    open_chest(game, slot, i);
                }
                continue;
            }
            if (ro->yx == j_yx && game->mighty_level >= 2u && (st & 0x20u) == 0u) {
                open_chest(game, slot, i);
                continue;
            }
        }

        if (st == 0x33u && ro->yx == j_yx) {
            if (obj->secret_block_yx != ro->yx) {
                obj->secret_block_yx = ro->yx;
                obj->secret_hits_left = (uint8_t)(ro->opcode & 0x0Fu);
            }
            if (obj->secret_hits_left > 0u) {
                obj->secret_hits_left--;
            }
            if (obj->secret_hits_left == 0u) {
                ro->ram_state = 0x0Fu;
                if (slot && ro->ram_index < MJK_ROOM_SLOT_OBJ_BYTES) {
                    slot->objects[ro->ram_index] = 0x0Fu;
                }
                MjkObjects_SetMetatile(game, ro->yx, 0x2Fu);
                play_sfx(game, MJK_SND_SECRET_BLOCK_REVEALED);
            }
            continue;
        }

        if (st <= 0x0Eu && ro->yx == j_yx) {
            MjkActionResult res = pickup_item(game, slot, i);
            if (res != MJK_ACTION_CONTINUE) {
                return res;
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
        } else if ((size_t)slot + 2u <= MJK_PPU_OAM_COUNT) {
            MjkPpu_SetOam(ppu, slot++, (uint8_t)(px - 8u), (uint8_t)(py - 4u),
                          (uint8_t)(0x31u + obj->puff.bomb_type), 0u);
            MjkPpu_SetOam(ppu, slot++, px, (uint8_t)(py - 4u), 0x30u, 0u);
        }
    }
    return slot;
}
