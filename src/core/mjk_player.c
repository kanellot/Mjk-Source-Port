/**
 * @file mjk_player.c
 * @brief Implementación de física Q8.8, flutter y colisiones 9-probe de Jack.
 */

#include "mjk_player.h"
#include <string.h>

/* Offsets de 9 sondas desde el centro de Jack (asm/src/prg.asm:5863-5872). */
static const int8_t k_probe_dx[9] = { 0, -4,  4, -4, 4, -8, -8,  8, 8 };
static const int8_t k_probe_dy[9] = { 0, -8, -8,  8, 8, -4,  4, -4, 4 };

/* Máscaras de cuadrantes 8x8 para metatiles 0x38..0x4B (asm/src/prg.asm:5873-5882). */
static const uint8_t k_partial_masks[10] = {
    0xCCu, 0xFCu, 0x48u, 0x5Au, 0x7Bu, 0xDEu, 0x21u, 0x35u, 0xACu, 0x3Cu
};

static void resolve_probe_cell(const MjkRoomState* cur_room,
                               const MjkRoomState* right_room,
                               int32_t wx,
                               int32_t wy,
                               const MjkRoomState** out_src,
                               uint8_t* out_mx,
                               uint8_t* out_my) {
    if (wy < 0) { wy = 0; }
    if (cur_room && !cur_room->is_horizontal && wy >= 240 && right_room) {
        *out_src = right_room;
        wy -= 240;
        if (wy >= 240) { wy = 239; }
        uint32_t uwx = (uint32_t)(wx & 255);
        *out_mx = (uint8_t)(uwx >> 4u);
        *out_my = (uint8_t)((uint32_t)wy >> 4u);
        return;
    }
    if (wy >= 240) { wy = 239; }
    if (wx < 0) { wx = 0; }
    uint32_t uwx = (uint32_t)(wx & 511);
    *out_src = (uwx >= 256u && right_room) ? right_room : cur_room;
    *out_mx = (uint8_t)((uwx & 255u) >> 4u);
    uint8_t my = (uint8_t)((uint32_t)wy >> 4u);
    if (my >= MJK_ROOM_MT_ROWS) {
        my = MJK_ROOM_MT_ROWS - 1u;
    }
    *out_my = my;
}

static bool is_probe_solid(const MjkRoomState* cur_room,
                           const MjkRoomState* right_room,
                           int32_t wx,
                           int32_t wy,
                           bool is_ground_probe,
                           uint8_t* out_fire_tile) {
    const MjkRoomState* src = NULL;
    uint8_t mx = 0u;
    uint8_t my = 0u;
    resolve_probe_cell(cur_room, right_room, wx, wy, &src, &mx, &my);
    if (!src) {
        return false;
    }
    uint8_t m = src->static_metatiles[my][mx];
    if (m >= 0x5Cu && m <= 0x62u) {
        if (out_fire_tile) {
            *out_fire_tile = m;
        }
        return false;
    }
    if (m != 0x00u && m != 0x27u && m != 0x2Fu) {
        if (m >= 0x38u && m <= 0x4Bu) {
            uint8_t idx = (uint8_t)((m - 0x38u) >> 1u);
            uint8_t packed = k_partial_masks[idx];
            uint8_t nibble = ((m & 1u) == 0u) ? (uint8_t)(packed >> 4u) : (uint8_t)(packed & 0x0Fu);
            uint8_t qx = (uint8_t)((wx >> 3u) & 1u);
            uint8_t qy = (uint8_t)((wy >> 3u) & 1u);
            uint8_t bit = (uint8_t)(3u - (qy * 2u + qx));
            return ((nibble >> bit) & 1u) != 0u;
        }
        return true;
    }

    uint8_t cell_yx = (uint8_t)((my << 4u) | mx);
    for (uint8_t i = 0u; i < src->object_count; ++i) {
        const MjkRoomObject* ro = &src->objects[i];
        uint8_t kind = (uint8_t)(ro->ram_state & 0x1Fu);
        if (kind == 0x12u) {
            /* P-09: Puertas cerradas visibles ($12/$92: bit 6=0 y bit 5=0) son sólidas. */
            if ((ro->ram_state & 0x60u) == 0u && (ro->yx == cell_yx || ro->yx2 == cell_yx)) {
                return true;
            }
            continue;
        }
        if (ro->yx != cell_yx) {
            continue;
        }
        if (kind == 0x10u || kind == 0x11u || kind == 0x13u) {
            if ((ro->ram_state & 0x20u) == 0u) {
                return true;
            }
            return is_ground_probe;
        }
    }
    return false;
}

void MjkPlayer_Init(MjkPlayer* p, uint8_t x, uint8_t y) {
    if (!p) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->pos_x = MjkUQ8_FromInt(x);
    p->pos_y = MjkUQ8_FromInt(y);
    p->facing_attr = 0x04u;
}

static void step_death(MjkPlayer* p,
                       const MjkAssetPackage* pkg,
                       const MjkRoomState* cur_room,
                       const MjkRoomState* right_room,
                       uint16_t scroll_x,
                       uint16_t scroll_y,
                       MjkActionResult* out_res) {
    /* MainSub_1 (prg.asm:1304-1366, 1460-1545): velocidad X estrictamente 0 al morir. */
    p->vel_x = 0;

    if (p->death_grounded) {
        p->vel_y = 0;
        p->sprite_id = 0x2Au;
        p->death_timer++;
        if (p->death_timer >= 64u) { /* $40 frames tendido en el suelo (prg.asm:1348) */
            *out_res = MJK_ACTION_PLAYER_DIED;
        }
        return;
    }

    int32_t cx = (int32_t)MjkUQ8_Int(p->pos_x) + (int32_t)scroll_x;
    int32_t cy = (int32_t)MjkUQ8_Int(p->pos_y) + (int32_t)scroll_y;
    uint8_t ft0 = 0u, ft1 = 0u;
    bool g0 = is_probe_solid(cur_room, right_room, cx + k_probe_dx[3], cy + k_probe_dy[3], true, &ft0) || (ft0 != 0u);
    bool g1 = is_probe_solid(cur_room, right_room, cx + k_probe_dx[4], cy + k_probe_dy[4], true, &ft1) || (ft1 != 0u);
    bool on_ground = (MjkUQ8_Int(p->pos_y) >= 0xE0u) || g0 || g1;
    if (on_ground && p->vel_y >= 0) {
        p->death_grounded = true;
        p->death_timer = 0u;
        p->in_air = false;
        p->vel_y = 0;
        p->sprite_id = 0x2Au;
        return;
    }

    /* En el aire: volteretas ($28 <-> $29 cada 6f) cayendo en vertical con gravedad neutra (X=0). */
    p->sprite_id = ((p->death_timer / 6u) & 1u) ? 0x29u : 0x28u;
    p->death_timer++;

    if (!p->in_air) {
        p->in_air = true;
        p->jump_accum = 0x4000u;
    }
    uint16_t rate = 0x00F0u;
    MjkAssets_ReadCpuU16(pkg, MJK_ADDR_VEL_X_TABLE_C163, &rate); /* v_idx = 0 (sin input) */
    uint32_t next_acc = (uint32_t)p->jump_accum + (uint32_t)rate;
    if ((next_acc >> 8u) > 0x7Fu) {
        next_acc = (0x7Fu << 8u) | (next_acc & 0xFFu);
    }
    p->jump_accum = (uint16_t)next_acc;

    uint8_t step = (uint8_t)(p->jump_accum >> 8u);
    uint8_t tbl_idx = (step < 64u) ? step : (uint8_t)((step ^ 0xFFu) & 0x3Fu);
    uint16_t mag = 0u;
    MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_JUMP_TABLE_C0E3 + (uint16_t)tbl_idx * 2u), &mag);
    p->vel_y = (step < 64u) ? (mjk_q8_8_t)(-(int16_t)mag) : (mjk_q8_8_t)mag;

    p->pos_y = MjkUQ8_AddSigned(p->pos_y, p->vel_y);
    cy = (int32_t)MjkUQ8_Int(p->pos_y) + (int32_t)scroll_y;

    if (p->vel_y < 0) {
        if (is_probe_solid(cur_room, right_room, cx + k_probe_dx[1], cy + k_probe_dy[1], false, NULL) ||
            is_probe_solid(cur_room, right_room, cx + k_probe_dx[2], cy + k_probe_dy[2], false, NULL)) {
            p->jump_accum = (uint16_t)(0x4000u | (p->jump_accum & 0x00FFu));
            int32_t snap_wy = ((cy + k_probe_dy[1]) & ~7) + 16;
            int32_t new_sy = snap_wy - (int32_t)scroll_y;
            if (new_sy < 0) { new_sy = 0; }
            if (new_sy > 255) { new_sy = 255; }
            p->pos_y = MjkUQ8_FromInt((uint8_t)new_sy);
            p->vel_y = 0;
        }
    } else if (MjkUQ8_Int(p->pos_y) >= 0xE0u) {
        p->pos_y = MjkUQ8_FromInt(0xE0u);
        p->death_grounded = true;
        p->death_timer = 0u;
        p->in_air = false;
        p->vel_y = 0;
        p->sprite_id = 0x2Au;
    } else if (is_probe_solid(cur_room, right_room, cx + k_probe_dx[3], cy + k_probe_dy[3], true, NULL) ||
               is_probe_solid(cur_room, right_room, cx + k_probe_dx[4], cy + k_probe_dy[4], true, NULL)) {
        int32_t snap_wy = ((cy + k_probe_dy[3]) & ~7) - 8;
        int32_t new_sy = snap_wy - (int32_t)scroll_y;
        if (new_sy < 0) { new_sy = 0; }
        if (new_sy > 255) { new_sy = 255; }
        p->pos_y = MjkUQ8_FromInt((uint8_t)new_sy);
        p->death_grounded = true;
        p->death_timer = 0u;
        p->in_air = false;
        p->vel_y = 0;
        p->sprite_id = 0x2Au;
    }
}

MjkActionResult MjkPlayer_Update(MjkPlayer* p,
                                 const MjkInputState* input,
                                 const MjkAssetPackage* pkg,
                                 const MjkRoomState* cur_room,
                                 const MjkRoomState* right_room,
                                 uint8_t room_flags,
                                 uint16_t scroll_x,
                                 uint16_t scroll_y,
                                 bool in_torture,
                                 uint8_t* mighty_level,
                                 uint8_t* mighty_presses,
                                 uint8_t* mighty_coins,
                                 MjkSoundId* out_sfx,
                                 bool* out_add_10pts) {
    if (!p || !pkg || !cur_room) {
        return MJK_ACTION_CONTINUE;
    }
    if (out_sfx) { *out_sfx = MJK_SND_SILENCE; }
    if (out_add_10pts) { *out_add_10pts = false; }
    p->just_jumped = false;

    if (p->is_dying) {
        MjkActionResult res = MJK_ACTION_CONTINUE;
        step_death(p, pkg, cur_room, right_room, scroll_x, scroll_y, &res);
        return res;
    }

    const uint8_t held = input ? input->held : 0u;
    const uint8_t pressed = input ? input->just_pressed : 0u;
    (void)mighty_coins;

    /* 1. Movimiento horizontal instantáneo (±1.5 px/f = ±0x0180 Q8.8); prioridad Derecha (P-10). */
    p->vel_x = 0;
    if ((held & MJK_BTN_RIGHT) != 0u) {
        p->vel_x = (mjk_q8_8_t)0x0180;
        p->facing_attr = 0x04u;
    } else if ((held & MJK_BTN_LEFT) != 0u) {
        p->vel_x = (mjk_q8_8_t)(-0x0180);
        p->facing_attr = 0x00u;
    }

    /* 2. Salto / Flutter estrictamente con flanco de subida */
    uint8_t jump_mask = ((room_flags & MJK_RF_BOMB_ROOM) != 0u)
        ? (uint8_t)(MJK_BTN_A | MJK_BTN_B)
        : (uint8_t)MJK_BTN_A;

    if ((pressed & jump_mask) != 0u) {
        p->a_press_counter++;
        if (mighty_level && mighty_presses && *mighty_level > 0u && *mighty_presses > 0u) {
            (*mighty_presses)--;
            const uint8_t* ap_tbl = MjkAssets_ResolveCpuPtr(pkg, MJK_ADDR_MIGHTY_A_PRESSES, 4u);
            uint8_t threshold = ap_tbl ? ap_tbl[(*mighty_level - 1u) & 3u] : 1u;
            if (*mighty_presses < threshold) {
                (*mighty_level)--;
            }
        }
        if (!p->in_air) {
            p->in_air = true;
            p->jump_accum = 0x0000u;
            p->just_jumped = true;
            if (out_sfx) { *out_sfx = MJK_SND_JUMP; }
            if (!in_torture && out_add_10pts) { *out_add_10pts = true; }
        } else {
            p->jump_accum = 0x4000u; /* Flutter */
        }
    }

    /* 3. Curva vertical de 64 pasos (word_C0E3 + word_C163) */
    if (p->in_air) {
        uint8_t v_idx = (uint8_t)((held & (MJK_BTN_UP | MJK_BTN_DOWN)) >> 2u);
        uint16_t rate = 0x00F0u;
        MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_VEL_X_TABLE_C163 + (uint16_t)v_idx * 2u), &rate);
        uint32_t next_acc = (uint32_t)p->jump_accum + (uint32_t)rate;
        if ((next_acc >> 8u) > 0x7Fu) {
            next_acc = (0x7Fu << 8u) | (next_acc & 0xFFu);
        }
        p->jump_accum = (uint16_t)next_acc;

        uint8_t step = (uint8_t)(p->jump_accum >> 8u);
        uint8_t tbl_idx = (step < 64u) ? step : (uint8_t)((step ^ 0xFFu) & 0x3Fu);
        uint16_t mag = 0u;
        MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_JUMP_TABLE_C0E3 + (uint16_t)tbl_idx * 2u), &mag);
        p->vel_y = (step < 64u) ? (mjk_q8_8_t)(-(int16_t)mag) : (mjk_q8_8_t)mag;
    } else {
        p->vel_y = 0;
    }

    /* 4. Integrar posición y resolver colisiones 9-probe sobre static_metatiles */
    p->collision_flags = 0u;
    p->pos_x = MjkUQ8_AddSigned(p->pos_x, p->vel_x);
    int32_t cx = (int32_t)MjkUQ8_Int(p->pos_x) + (int32_t)scroll_x;
    int32_t cy = (int32_t)MjkUQ8_Int(p->pos_y) + (int32_t)scroll_y;

    if (p->vel_x > 0) {
        if (is_probe_solid(cur_room, right_room, cx + k_probe_dx[7], cy + k_probe_dy[7], false, NULL) ||
            is_probe_solid(cur_room, right_room, cx + k_probe_dx[8], cy + k_probe_dy[8], false, NULL)) {
            p->collision_flags |= MJK_CF_RIGHT_WALL;
            p->pos_x = MjkUQ8_AddSigned(p->pos_x, (mjk_q8_8_t)(-p->vel_x));
            p->vel_x = 0;
        }
    } else if (p->vel_x < 0) {
        if (is_probe_solid(cur_room, right_room, cx + k_probe_dx[5], cy + k_probe_dy[5], false, NULL) ||
            is_probe_solid(cur_room, right_room, cx + k_probe_dx[6], cy + k_probe_dy[6], false, NULL)) {
            p->collision_flags |= MJK_CF_LEFT_WALL;
            p->pos_x = MjkUQ8_AddSigned(p->pos_x, (mjk_q8_8_t)(-p->vel_x));
            p->vel_x = 0;
        }
    }

    p->pos_y = MjkUQ8_AddSigned(p->pos_y, p->vel_y);
    cx = (int32_t)MjkUQ8_Int(p->pos_x) + (int32_t)scroll_x;
    cy = (int32_t)MjkUQ8_Int(p->pos_y) + (int32_t)scroll_y;

    if (p->vel_y < 0) {
        if (is_probe_solid(cur_room, right_room, cx + k_probe_dx[1], cy + k_probe_dy[1], false, NULL) ||
            is_probe_solid(cur_room, right_room, cx + k_probe_dx[2], cy + k_probe_dy[2], false, NULL)) {
            p->collision_flags |= MJK_CF_CEILING;
            p->jump_accum = (uint16_t)(0x4000u | (p->jump_accum & 0x00FFu));
            int32_t snap_wy = ((cy + k_probe_dy[1]) & ~7) + 16;
            int32_t new_sy = snap_wy - (int32_t)scroll_y;
            if (new_sy < 0) { new_sy = 0; }
            if (new_sy > 255) { new_sy = 255; }
            p->pos_y = MjkUQ8_FromInt((uint8_t)new_sy);
            p->vel_y = 0;
        }
    } else {
        bool clamp_bottom = ((room_flags & (MJK_RF_SINGLE_SCREEN | MJK_RF_HORIZONTAL | MJK_RF_AT_SCROLL_EDGE)) != 0u) &&
                            (MjkUQ8_Int(p->pos_y) >= 0xE0u);
        if (clamp_bottom) {
            /* MainSub_1 (prg.asm:1520-1531): suelo de seguridad en Y = $E0 (224 px). */
            p->collision_flags |= MJK_CF_GROUND;
            p->in_air = false;
            p->pos_y = MjkUQ8_FromInt(0xE0u);
            p->vel_y = 0;
        } else if (is_probe_solid(cur_room, right_room, cx + k_probe_dx[3], cy + k_probe_dy[3], true, NULL) ||
                   is_probe_solid(cur_room, right_room, cx + k_probe_dx[4], cy + k_probe_dy[4], true, NULL)) {
            p->collision_flags |= MJK_CF_GROUND;
            p->in_air = false;
            int32_t snap_wy = ((cy + k_probe_dy[3]) & ~7) - 8;
            int32_t new_sy = snap_wy - (int32_t)scroll_y;
            if (new_sy < 0) { new_sy = 0; }
            if (new_sy > 255) { new_sy = 255; }
            p->pos_y = MjkUQ8_FromInt((uint8_t)new_sy);
            p->vel_y = 0;
        } else {
            /* sub_A1EB (prg.asm:5565-5574): al salir por un borde (in_air 0->1), resetear a $40xx. */
            if (!p->in_air) {
                p->in_air = true;
                p->jump_accum = 0x4000u;
            }
        }
    }

    /* Sonda 0: muerte por fuego ($5C..$62) */
    uint8_t fire_tile = 0u;
    is_probe_solid(cur_room, right_room,
                   (int32_t)MjkUQ8_Int(p->pos_x) + (int32_t)scroll_x,
                   (int32_t)MjkUQ8_Int(p->pos_y) + (int32_t)scroll_y,
                   false,
                   &fire_tile);
    if (fire_tile != 0u) {
        p->is_dying = true;
        p->death_grounded = false;
        p->death_timer = 0u;
        p->vel_x = 0;
        p->fire_tile_died_to = fire_tile;
        if (out_sfx) { *out_sfx = MJK_SND_DEATH; }
    }

    /* 5. Selección de sprite de Jack (loc_8A66 mantiene animación al empujar contra pared) */
    bool pushing_h = ((held & MJK_BTN_RIGHT) != 0u) != ((held & MJK_BTN_LEFT) != 0u);
    if (!p->in_air) {
        if (p->vel_x != 0 || pushing_h) {
            p->walk_timer++;
            p->sprite_id = ((p->walk_timer & 4u) != 0u) ? 0x05u : 0x04u;
        } else {
            p->sprite_id = 0x00u;
        }
    } else if (p->vel_y < 0) {
        p->sprite_id = (p->vel_x != 0 || pushing_h) ? 0x04u : 0x01u;
    } else {
        p->sprite_id = (p->vel_x != 0 || pushing_h) ? 0x06u : 0x02u;
    }

    return MJK_ACTION_CONTINUE;
}
