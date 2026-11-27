/**
 * @file renderer.c
 * @brief Implementacion del renderizador solo-lectura optimizado (Ticket R-14).
 */

#include "mjk/renderer.h"
#include <math.h>
#include <string.h>

#define BG_TILE_BASE 256u
#define MAX_SPRITES  64u

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t tile;
    uint8_t attr;
} SpriteCmd;

typedef struct {
    uint32_t  backdrop;
    uint32_t  bg_pal[4][4];
    uint32_t  spr_pal[4][4];
    uint8_t   nt[2][30][32];
    uint8_t   attr[2][15][16];
    uint16_t  scroll_x;
    uint8_t   scroll_y;
    SpriteCmd sprites[MAX_SPRITES];
    uint8_t   sprite_count;
} RenderScene;

static void build_palette_cache(RenderScene* sc, const MjkAssets* a, const uint8_t pal_ram[32]) {
    sc->backdrop = a->master_palette[pal_ram[0] & 0x3Fu];
    for (int p = 0; p < 4; ++p) {
        sc->bg_pal[p][0]  = sc->backdrop;
        sc->spr_pal[p][0] = 0u;
        for (int c = 1; c < 4; ++c) {
            sc->bg_pal[p][c]  = a->master_palette[pal_ram[p * 4 + c] & 0x3Fu];
            sc->spr_pal[p][c] = a->master_palette[pal_ram[16 + p * 4 + c] & 0x3Fu];
        }
    }
}

static void clear_nametable(RenderScene* sc, uint8_t fill_tile) {
    memset(sc->nt, fill_tile, sizeof(sc->nt));
    memset(sc->attr, 0, sizeof(sc->attr));
}

static void write_string(RenderScene* sc, const MjkAssets* a, MjkStringId id) {
    if ((unsigned)id >= (unsigned)STR_COUNT) { return; }
    const MjkTextString* s = &a->strings[id];
    for (uint8_t i = 0u; i < s->length && (uint8_t)(s->col + i) < 32u; ++i) {
        if (s->row < 30u && s->nt_idx < 2u) {
            sc->nt[s->nt_idx][s->row][s->col + i] = s->tiles[i];
        }
    }
}

static void blit_room_metatiles(RenderScene* sc, const MjkAssets* a,
                                const uint8_t metatiles[MJK_ROOM_ROWS][MJK_ROOM_COLS],
                                const uint8_t palettes[MJK_ROOM_ROWS][MJK_ROOM_COLS],
                                uint8_t nt_idx) {
    for (uint8_t my = 0u; my < MJK_ROOM_ROWS; ++my) {
        uint8_t row = (uint8_t)(my * 2u);
        for (uint8_t mx = 0u; mx < MJK_ROOM_COLS; ++mx) {
            uint8_t mt = metatiles[my][mx];
            if (mt >= MJK_METATILE_COUNT) { mt = 0u; }
            uint8_t col = (uint8_t)(mx * 2u);
            sc->nt[nt_idx][row][col]           = a->metatile_tiles[mt][0];
            sc->nt[nt_idx][row][col + 1u]      = a->metatile_tiles[mt][1];
            sc->nt[nt_idx][row + 1u][col]      = a->metatile_tiles[mt][2];
            sc->nt[nt_idx][row + 1u][col + 1u] = a->metatile_tiles[mt][3];
            sc->attr[nt_idx][my][mx]           = palettes ? (uint8_t)(palettes[my][mx] & 3u)
                                                          : a->metatile_palette[mt];
        }
    }
}

static void push_sprite(RenderScene* sc, uint8_t x, uint8_t y, uint8_t tile, uint8_t attr) {
    if (sc->sprite_count < MAX_SPRITES) {
        SpriteCmd* s = &sc->sprites[sc->sprite_count++];
        s->x    = x;
        s->y    = y;
        s->tile = tile;
        s->attr = attr;
    }
}

static void push_metasprite(RenderScene* sc, const MjkAssets* a,
                            uint8_t sprite_id, uint8_t x, uint8_t y, uint8_t attr_flags) {
    if ((attr_flags & 0x10u) != 0u) {
        push_sprite(sc, (uint8_t)(x - 4u), (uint8_t)(y - 4u), sprite_id, (uint8_t)(attr_flags & 0xE3u));
        return;
    }
    if (sprite_id >= MJK_SPRITE_DEF_COUNT) { return; }
    uint8_t idx         = (uint8_t)((attr_flags & 0x0Cu) >> 1u);
    uint8_t flip_mask   = a->sprite_attr_table[idx];
    uint8_t offset_mask = a->sprite_attr_table[idx + 1u];
    const MjkSpriteDef* def = &a->sprites[sprite_id];
    uint8_t flip_bits   = (uint8_t)(def->flip_bits ^ flip_mask);
    uint8_t base_attr   = (uint8_t)(attr_flags & 0x23u);

    for (int q = 0; q < 4; ++q) {
        uint8_t sy = (uint8_t)(y + ((offset_mask & 1u) ? (uint8_t)(-8) : 0u));
        offset_mask >>= 1u;
        uint8_t sx = (uint8_t)(x + ((offset_mask & 1u) ? (uint8_t)(-8) : 0u));
        offset_mask >>= 1u;
        uint8_t h_flip = (uint8_t)(flip_bits & 1u);
        uint8_t v_flip = (uint8_t)((flip_bits >> 1u) & 1u);
        flip_bits >>= 2u;
        push_sprite(sc, sx, sy, def->tiles[q], (uint8_t)(base_attr | (h_flip << 6u) | (v_flip << 7u)));
    }
}

static void push_two_digits(RenderScene* sc, int value, uint8_t x, uint8_t y, bool hide_leading_zero) {
    if (value < 0)  { value = 0; }
    if (value > 99) { value = 99; }
    uint8_t tens = (uint8_t)(value / 10);
    uint8_t ones = (uint8_t)(value % 10);
    push_sprite(sc, (uint8_t)(x - 12u), (uint8_t)(y - 4u), (hide_leading_zero && tens == 0u) ? 0x0Fu : tens, 0u);
    push_sprite(sc, (uint8_t)(x - 4u),  (uint8_t)(y - 4u), ones, 0u);
}

static void push_score_digits(RenderScene* sc, uint32_t score) {
    uint8_t digits[8];
    uint32_t tmp = score;
    for (int i = 7; i >= 0; --i) {
        digits[i] = (uint8_t)(tmp % 10u);
        tmp /= 10u;
    }
    uint8_t x = 88u;
    bool non_zero = false;
    for (int i = 0; i < 8; ++i) {
        bool show = (digits[i] != 0u) || (i == 7) || non_zero;
        if (show) {
            non_zero = true;
            push_sprite(sc, x, 16u, digits[i], 0u);
        }
        x = (uint8_t)(x + 8u);
    }
}

static void build_playing_scene(RenderScene* sc, const MjkGame* g, const MjkAssets* a, uint8_t pal_ram[32]) {
    if (g->world.active_palette_set < 9u) {
        const uint8_t* src = a->room_palettes[g->world.active_palette_set];
        memcpy(pal_ram + 4u, src, 12u);
        for (size_t i = 0u; i < 32u; i += 4u) { pal_ram[i] = src[0]; }
    }
    pal_ram[0x17u] = a->mighty_colors[g->mighty_level & 3u];
    if (g->power_coin_palette_active) {
        pal_ram[0x19u] = 0x08u;
        pal_ram[0x1Au] = 0x26u;
        pal_ram[0x1Bu] = 0x30u;
    }

    sc->scroll_x = (uint16_t)floorf(g->world.camera.scroll_x);
    sc->scroll_y = (uint8_t)floorf(g->world.camera.scroll_y);
    blit_room_metatiles(sc, a, g->world.current.visual_metatiles, g->world.current.visual_palettes, 0u);
    if (g->world.has_neighbor) {
        blit_room_metatiles(sc, a, g->world.neighbor.visual_metatiles, g->world.neighbor.visual_palettes, 1u);
    }

    if (g->state == GS_ENTER_ROOM) {
        uint8_t dir = (uint8_t)((g->entry_door_type >> 2u) & 3u);
        int8_t dx = a->door_walk_vec[dir][1];
        uint8_t spr = (g->sub_state == 2u && dx != 0) ? (((g->state_timer >> 2) & 1) ? 0x04u : 0x05u) : 0x00u;
        uint8_t attr = (g->sub_state <= 1u) ? 0x21u : ((dx > 0) ? 0x05u : 0x01u);
        push_metasprite(sc, a, spr, (uint8_t)floorf(g->cinematic_jack_x), (uint8_t)floorf(g->cinematic_jack_y), attr);
        return;
    }

    /* Jack */
    uint8_t j_attr = (uint8_t)(((g->player.facing == FACING_RIGHT) ? 0x04u : 0x00u) | 0x01u);
    push_metasprite(sc, a, g->player.sprite_id, (uint8_t)floorf(g->player.x), (uint8_t)floorf(g->player.y), j_attr);

    /* HUD: Score, Timer, Mighty/Mult, Torture jumps, 1UP y popups flotantes */
    push_score_digits(sc, g->scoring.score);
    if (g->in_torture_mode) {
        push_two_digits(sc, g->greedy_jumps_left, 0x84u, 0x44u, true);
    } else {
        push_sprite(sc, 56u, 24u, 0x0Eu, 0u); /* 'T' */
        push_two_digits(sc, g->scoring.timer_seconds, 76u, 28u, true);
    }
    {
        uint8_t icon = g->world.current.is_bomb_room ? 0x0Cu : 0x0Bu;
        uint8_t val  = g->world.current.is_bomb_room
            ? (uint8_t)((g->scoring.score_multiplier % 10u) + 1u)
            : (uint8_t)(g->mighty_coins % 10u);
        push_sprite(sc, 184u, 24u, icon, 0u);
        push_sprite(sc, 192u, 24u, val, 0u);
    }
    if (g->one_up_timer > 0) {
        int jx = (int)floorf(g->player.x);
        int jy = (int)floorf(g->player.y);
        uint8_t ux0 = (jx >= 8) ? (uint8_t)(jx - 8) : 0u;
        uint8_t ux1 = (jx >= 0 && jx < 256) ? (uint8_t)jx : 0u;
        uint8_t uy  = (jy >= 16) ? (uint8_t)(jy - 16) : 0u;
        push_sprite(sc, ux0, uy, 0x82u, 0x01u);
        push_sprite(sc, ux1, uy, 0x83u, 0x01u);
    }
    for (size_t i = 0u; i < MJK_FLOAT_POPUP_SLOTS; ++i) {
        const MjkFloatingPopup* p = &g->scoring.popups[i];
        if (!p->active) { continue; }
        uint8_t px = (uint8_t)floorf(p->x), py = (uint8_t)floorf(p->y);
        push_sprite(sc, px, py, p->tl_tile, 0u);
        push_sprite(sc, (uint8_t)(px + 8u), py, p->tr_tile, 0u);
        push_sprite(sc, px, (uint8_t)(py + 8u), 0x0Fu, 0u);
        push_sprite(sc, (uint8_t)(px + 8u), (uint8_t)(py + 8u), p->br_tile, 0u);
    }

    /* Chispa y puff de bombas */
    if (g->objects.spark.active) {
        uint8_t sx = (uint8_t)(g->objects.spark.grid_x * 16u + 8u);
        uint8_t sy = (uint8_t)(g->objects.spark.grid_y * 16u - 4u);
        push_sprite(sc, sx, sy, g->objects.spark.tile_id, 0x12u);
    }
    if (g->objects.puff.active) {
        uint8_t px = (uint8_t)floorf(g->objects.puff.x);
        uint8_t py = (uint8_t)floorf(g->objects.puff.y);
        if (g->objects.puff.phase == 0u) {
            push_metasprite(sc, a, 0x25u, px, py, 0x00u);
        } else {
            uint8_t lx = (px >= 8u) ? (uint8_t)(px - 8u) : 0u;
            uint8_t ty = (py >= 8u) ? (uint8_t)(py - 8u) : 0u;
            push_sprite(sc, lx, ty, (uint8_t)(0x31u + (g->objects.puff.was_lit ? 1u : 0u)), 0u);
            push_sprite(sc, px, ty, 0x30u, 0u);
            push_sprite(sc, lx, py, 0x0Fu, 0u);
            push_sprite(sc, px, py, g->objects.puff.mult_tile, 0u);
        }
    }

    /* Enemigos y coleccionables */
    for (size_t i = 0u; i < MJK_ENEMY_MAX_SLOTS; ++i) {
        const MjkEnemy* e = &g->enemies.slots[i];
        if (!e->active || e->y >= 240.0f) { continue; }
        uint8_t ex = (uint8_t)floorf(e->x), ey = (uint8_t)floorf(e->y);
        if (e->transmuted) {
            uint8_t pal = (g->enemies.transmute_timer < 64) ? (uint8_t)(g->enemies.transmute_timer & 3) : 2u;
            push_metasprite(sc, a, 0x21u, ex, ey, pal);
        } else if (((unsigned)e->kind <= (unsigned)ENEMY_HORUS ||
                    e->kind == ENEMY_MUMMY1 || e->kind == ENEMY_MUMMY2) &&
                   e->spawn_flash_timer > 16) {
            push_metasprite(sc, a, 0x25u, ex, ey, 0x00u);
        } else {
            uint8_t step6 = (uint8_t)(e->anim_timer / 6u);
            uint8_t flip = 0u;
            if (e->kind == ENEMY_MUMMY && !e->facing_left)        { flip = 0x04u; }
            else if (e->kind == ENEMY_HANEZO && (step6 % 3u) == 2u) { flip = 0x04u; }
            else if (e->kind == ENEMY_HORUS && e->vx > 0.0f)      { flip = 0x04u; }
            else if (e->kind == ENEMY_DESUFA && (step6 & 1u) != 0u) { flip = 0x08u; }
            push_metasprite(sc, a, e->sprite_id, ex, ey, (uint8_t)((e->palette_idx & 3u) | flip));
        }
    }
}

static void rasterize_scene(const RenderScene* sc, const MjkAssets* a, uint32_t* fb) {
    uint8_t bg_opaque[MJK_SCREEN_HEIGHT][MJK_SCREEN_WIDTH];

    /* 1. Fondo usando tiles 8x8 pre-decodificados y cache plana de paletas ARGB8888 */
    for (uint32_t y = 0u; y < (uint32_t)MJK_SCREEN_HEIGHT; ++y) {
        uint32_t wy_sum  = y + (uint32_t)sc->scroll_y;
        uint8_t  v_nt    = (sc->scroll_x == 0u && wy_sum >= 240u) ? 1u : 0u;
        uint32_t wy      = wy_sum % 240u;
        uint8_t  t_row   = (uint8_t)(wy >> 3u);
        uint8_t  fine_y  = (uint8_t)(wy & 7u);
        uint8_t  my      = (uint8_t)(t_row >> 1u);
        uint32_t* fb_row = fb + (y * (uint32_t)MJK_SCREEN_WIDTH);

        for (uint32_t x = 0u; x < (uint32_t)MJK_SCREEN_WIDTH; ++x) {
            uint32_t wx     = (x + (uint32_t)sc->scroll_x) & 511u;
            uint8_t  nt_idx = (v_nt != 0u) ? 1u : (uint8_t)((wx >> 8u) & 1u);
            uint8_t  t_col  = (uint8_t)((wx >> 3u) & 31u);
            uint8_t  fine_x = (uint8_t)(wx & 7u);
            uint8_t  tile   = sc->nt[nt_idx][t_row][t_col];
            uint8_t  px     = a->tiles[BG_TILE_BASE + tile][fine_y][fine_x];

            bg_opaque[y][x] = (uint8_t)(px != 0u);
            fb_row[x]       = (px != 0u) ? sc->bg_pal[sc->attr[nt_idx][my][t_col >> 1u] & 3u][px] : sc->backdrop;
        }
    }

    /* 2. Sprites en orden inverso (indice 0 delante), sin limite artificial de 8/scanline */
    for (int s = (int)sc->sprite_count - 1; s >= 0; --s) {
        const SpriteCmd* e = &sc->sprites[s];
        if (e->y >= 0xF0u) { continue; }
        const uint32_t* pal = sc->spr_pal[e->attr & 3u];
        bool behind_bg = ((e->attr & 0x20u) != 0u);
        bool h_flip    = ((e->attr & 0x40u) != 0u);
        bool v_flip    = ((e->attr & 0x80u) != 0u);
        const uint8_t (*tile_px)[8] = a->tiles[e->tile];

        for (uint8_t row = 0u; row < 8u; ++row) {
            uint32_t py = (uint32_t)e->y + 1u + (uint32_t)row;
            if (py >= (uint32_t)MJK_SCREEN_HEIGHT) { continue; }
            uint8_t sr = v_flip ? (uint8_t)(7u - row) : row;
            for (uint8_t col = 0u; col < 8u; ++col) {
                uint32_t px_x = (uint32_t)e->x + (uint32_t)col;
                if (px_x >= (uint32_t)MJK_SCREEN_WIDTH) { continue; }
                uint8_t c = tile_px[sr][h_flip ? (uint8_t)(7u - col) : col];
                if (c == 0u || (behind_bg && bg_opaque[py][px_x])) { continue; }
                fb[py * (uint32_t)MJK_SCREEN_WIDTH + px_x] = pal[c];
            }
        }
    }
}

void MjkRenderer_DrawFrame(const MjkGame* game,
                           const MjkAssets* assets,
                           uint32_t* out_fb_argb8888) {
    if (!game || !assets || !out_fb_argb8888) {
        return;
    }

    RenderScene sc;
    memset(&sc, 0, sizeof(sc));
    clear_nametable(&sc, 0x24u);

    uint8_t pal_ram[32];
    memcpy(pal_ram, assets->main_palette, 32u);

    if (game->mode == SCREEN_TITLE) {
        if (game->title.state <= TITLE_MAIN_INIT) {
            write_string(&sc, assets, STR_MIGHTY_BOMB_JACK_TM);
            write_string(&sc, assets, STR_TM_AND_C_TECMO_LTD);
            write_string(&sc, assets, STR_LICENSED_BY);
            write_string(&sc, assets, STR_NINTENDO_OF_AMERICA);
            push_two_digits(&sc, 19, 0x74u, 0x63u, false);
            push_two_digits(&sc, 87, 0x84u, 0x63u, false);
        } else {
            blit_room_metatiles(&sc, assets, assets->rooms[0].static_metatiles, NULL, 0u);
            write_string(&sc, assets, STR_PUSH_START_BUTTON);
            write_string(&sc, assets, STR_C_TECMO);
            write_string(&sc, assets, STR_HIGH_GDV);
            write_string(&sc, assets, STR_TM);
            push_score_digits(&sc, game->scoring.score);
            push_two_digits(&sc, game->scoring.high_gdv, 0xCCu, 0x1Cu, true);
            push_two_digits(&sc, 19, 0xBCu, 0x93u, false);
            push_two_digits(&sc, 87, 0xCCu, 0x93u, false);
            uint8_t j_attr = (game->title.state == TITLE_JACK_FALLING) ? 0x21u : 0x01u;
            push_metasprite(&sc, assets, game->title.jack_sprite,
                            (uint8_t)floorf(game->title.jack_x),
                            (uint8_t)floorf(game->title.jack_y), j_attr);
        }
    } else if (game->state == GS_ROUND_INTRO) {
        push_metasprite(&sc, assets, 0u, 0x70u, 0x60u, 0x01u);
        push_sprite(&sc, 0x80u, 0x5Cu, 0x0Cu, 0u);
        push_two_digits(&sc, game->scoring.lives, 0x94u, 0x60u, true);
    } else if (game->state == GS_TIME_OVER) {
        write_string(&sc, assets, STR_TIME_OVER);
    } else if (game->state == GS_TORTURE_ROOM) {
        write_string(&sc, assets, STR_YOU_ARE_GREEDY);
        write_string(&sc, assets, STR_GO_TO_TORTURE_ROOM);
    } else if (game->state == GS_ROUND_CLEAR) {
        write_string(&sc, assets, STR_ROUND_CLEAR);
        write_string(&sc, assets, STR_TIME_BONUS);
        push_score_digits(&sc, game->scoring.score);
        push_metasprite(&sc, assets, 0u, 0x70u, 0x60u, 0x01u);
        push_sprite(&sc, 0x80u, 0x5Cu, 0x0Cu, 0u);
        push_two_digits(&sc, game->scoring.lives, 0x94u, 0x60u, true);
        push_two_digits(&sc, (int)((game->last_bomb_room_cleared & 0x0Fu) + 1u), 0x84u, 0x44u, true);
        push_two_digits(&sc, game->round_clear_timer_copy, 0xACu, 0x7Cu, true);
        push_sprite(&sc, 0xB0u, 0x78u, 0x00u, 0u);
        push_sprite(&sc, 0xB8u, 0x78u, 0x00u, 0u);
        if (game->round_clear_has_special) {
            write_string(&sc, assets, STR_YOUVE_GOTTEN);
            write_string(&sc, assets, STR_FIRE_BOMBS);
            write_string(&sc, assets, STR_SPECIAL_BONUS);
            push_two_digits(&sc, (int)game->fire_bombs_collected, 0x54u, 0xACu, true);
            push_two_digits(&sc, game->round_clear_fire_bonus, 0xB4u, 0xC4u, true);
            push_sprite(&sc, 0xB8u, 0xC0u, 0x00u, 0u);
            push_sprite(&sc, 0xC0u, 0xC0u, 0x00u, 0u);
            push_sprite(&sc, 0xC8u, 0xC0u, 0x00u, 0u);
        }
    } else if (game->state == GS_GAME_OVER) {
        write_string(&sc, assets, STR_GAME_OVER);
        write_string(&sc, assets, STR_YOUR_GDV);
        push_two_digits(&sc, game->scoring.gdv, 0xA4u, 0x84u, true);
    } else if (game->state == GS_ENDING) {
        static const MjkStringId k_end_start[4] = { STR_ENDING_1A, STR_ENDING_2A, STR_ENDING_3A, STR_ENDING_4A };
        static const uint8_t     k_end_count[4] = { 3u, 3u, 4u, 4u };
        static const uint8_t     k_destroy_depth[4] = { 12u, 30u, 56u, 90u };
        uint8_t et = game->ending_type & 3u;
        blit_room_metatiles(&sc, assets, assets->rooms[0].static_metatiles, NULL, 0u);
        for (int r = 0; r < 16; ++r) { memset(sc.nt[0][r], 0x24u, 32u); }
        uint8_t total = k_destroy_depth[et];
        uint8_t done = (game->ending_state >= 3u)
            ? total
            : ((total >= game->ending_depth) ? (uint8_t)(total - game->ending_depth) : 0u);
        uint8_t half_w = (uint8_t)(2u + done / 8u);
        if (half_w > 14u) { half_w = 14u; }
        for (uint8_t r = 0u; r < game->ending_carved_rows && (16u + r) < 30u; ++r) {
            uint8_t c_start = (uint8_t)(16u - half_w);
            uint8_t c_end   = (uint8_t)(16u + half_w);
            if (c_end >= 32u) { c_end = 31u; }
            for (uint8_t c = c_start; c <= c_end; ++c) {
                sc.nt[0][16u + r][c] = 0x24u;
            }
        }
        if (game->ending_state == 2u) {
            uint8_t debris_off = (uint8_t)((done & 3u) * 4u);
            uint8_t jy = (uint8_t)floorf(game->cinematic_jack_y);
            push_metasprite(&sc, assets, 0x25u, (uint8_t)(0x68u - debris_off), (uint8_t)(jy + debris_off), 0x00u);
            push_metasprite(&sc, assets, 0x25u, (uint8_t)(0x88u + debris_off), (uint8_t)(jy + debris_off), 0x00u);
        }
        if (game->ending_state >= 3u) {
            for (uint8_t i = 0u; i < k_end_count[et]; ++i) {
                write_string(&sc, assets, (MjkStringId)(k_end_start[et] + i));
            }
        }
        uint8_t spr = (game->ending_state >= 3u) ? 0x00u : 0x01u;
        push_metasprite(&sc, assets, spr, (uint8_t)floorf(game->cinematic_jack_x),
                        (uint8_t)floorf(game->cinematic_jack_y), 0x01u);
    } else if (game->state == GS_LOAD_ROOM || game->state == GS_DRAW_SCREEN) {
        /* Transicion limpia de sala sin destello de sprites/HUD */
    } else {
        build_playing_scene(&sc, game, assets, pal_ram);
    }

    build_palette_cache(&sc, assets, pal_ram);
    rasterize_scene(&sc, assets, out_fb_argb8888);
}
