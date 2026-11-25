/**
 * @file asset_bank.c
 * @brief Decodificador unico de `assets.dat` v2 / `mbj-us.nes` hacia `MjkAssets`.
 *
 * Este archivo es el UNICO lugar del motor que conoce las direcciones originales
 * de la ROM (`$B3FD..$FEB4`), los bitplanes 2bpp de la CHR-ROM y la compresion
 * de salas en 4 capas. Al terminar `MjkAssets_LoadFromAssetsMemory`, todo queda
 * en arrays planos, `float` y `uint32_t` listos para consumo directo.
 */

#include "mjk/assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Direcciones CPU internas de REV_US (confinadas exclusivamente a este archivo) */
enum {
    ADDR_ENEMY_INIT_STATUS  = 0xBF47u,
    ADDR_DIFFICULTY_TABLE   = 0xBF58u,
    ADDR_GAMEIDO_DIV_TABLE  = 0xBF98u,
    ADDR_ENEMY_SPEED_TABLE  = 0xBFA0u,
    ADDR_JUMP_TABLE         = 0xBFB0u,
    ADDR_JUMP_RATE_TABLE    = 0xC030u,
    ADDR_MAIN_PALETTE       = 0xC038u,
    ADDR_BG_PALETTE_SETS    = 0xC058u,
    ADDR_MIGHTY_COLORS      = 0xC0C4u,
    ADDR_MIGHTY_A_PRESSES   = 0xC0C8u,
    ADDR_MUSIC_OPTIONS      = 0xC0CCu,
    ADDR_DOOR_ENTRY_XY      = 0xC0D4u,
    ADDR_VECTOR_TABLE       = 0xC0F4u,
    ADDR_MAYBE_ENTRY_TYPE   = 0xC0FCu,
    ADDR_SPRITES_TABLE      = 0xC11Cu,
    ADDR_STRING_PTR_TABLE   = 0xC202u,
    ADDR_SPRITE_ATTR_TABLE  = 0xC4D9u,
    ADDR_TILE_ATTR_TABLE    = 0xC4E1u,
    ADDR_SECTION_ROOMS_TBL  = 0xC509u,
    ADDR_FIRE_BOMB_BONUS    = 0xC646u,
    ADDR_MULTIPLIED_SCORE   = 0xC64Au,
    ADDR_SCORE_ADD_TABLE    = 0xC66Au,
    ADDR_ITEM_TO_TILE       = 0xC6AAu,
    ADDR_ROOM_HALF_SCREENS  = 0xC6CAu,
    ADDR_HALF_SCREEN_LAYTS  = 0xC8CAu,
    ADDR_LAYOUT_CHUNKS      = 0xD0EAu,
    ADDR_METATILE_DEFS      = 0xD7AAu,
    ADDR_ADJACENT_ROOMS     = 0xDA0Eu,
    ADDR_ENEMY_SPAWN_POS    = 0xDF0Eu,
    ADDR_ROOM_DATA_PTRS     = 0xF1A0u,
    ADDR_SOUND_POINTERS     = 0xF45Cu
};

/* Paleta maestra NTSC 2C02 (64 colores en formato ARGB8888: 0xFFRRGGBB) */
static const uint32_t k_nes_palette_2c02[64] = {
    0xFF666666u, 0xFF002A88u, 0xFF1412A7u, 0xFF3B00A4u,
    0xFF5C007Eu, 0xFF6E0040u, 0xFF6C0600u, 0xFF561D00u,
    0xFF333500u, 0xFF0B4800u, 0xFF005200u, 0xFF004F08u,
    0xFF00404Du, 0xFF000000u, 0xFF000000u, 0xFF000000u,

    0xFFADADADu, 0xFF155FD9u, 0xFF4240FFu, 0xFF7527FEu,
    0xFFA01ACCu, 0xFFB71E7Bu, 0xFFB53120u, 0xFF994E00u,
    0xFF6B6D00u, 0xFF388700u, 0xFF0C9300u, 0xFF008F32u,
    0xFF007C8Du, 0xFF000000u, 0xFF000000u, 0xFF000000u,

    0xFFFFFEFFu, 0xFF64B0FFu, 0xFF9290FFu, 0xFFC676FFu,
    0xFFF36AFFu, 0xFFFE6ECCu, 0xFFFE8170u, 0xFFEA9E22u,
    0xFFBCBE00u, 0xFF88D800u, 0xFF5CE430u, 0xFF45E082u,
    0xFF48CDDEu, 0xFF4F4F4Fu, 0xFF000000u, 0xFF000000u,

    0xFFFFFEFFu, 0xFFC0DFFFu, 0xFFD3D2FFu, 0xFFE8C8FFu,
    0xFFFBC2FFu, 0xFFFEC4EAu, 0xFFFECCC5u, 0xFFF7D8A5u,
    0xFFE4E594u, 0xFFCFEF96u, 0xFFBDF4ABu, 0xFFB3F3CCu,
    0xFFB5EBF2u, 0xFFB8B8B8u, 0xFF000000u, 0xFF000000u
};

/* Disposicion de puertas (coordenadas YX0, YX1 y metatiles cerrado/abierto) */
static const uint8_t k_door_layout[16][4] = {
    { 0x11u, 0x12u, 0x87u, 0x95u }, { 0x1Du, 0x1Eu, 0x87u, 0x95u },
    { 0x11u, 0x12u, 0x87u, 0x95u }, { 0x1Du, 0x1Eu, 0x87u, 0x95u },
    { 0xE1u, 0xE2u, 0x87u, 0x95u }, { 0xEDu, 0xEEu, 0x87u, 0x95u },
    { 0xD1u, 0xD2u, 0x87u, 0x95u }, { 0xDDu, 0xDEu, 0x87u, 0x95u },
    { 0x2Cu, 0x3Cu, 0x8Bu, 0x97u }, { 0xBFu, 0xCFu, 0x8Bu, 0x97u },
    { 0x2Fu, 0x3Fu, 0x8Bu, 0x97u }, { 0xBFu, 0xCFu, 0x8Bu, 0x97u },
    { 0x23u, 0x33u, 0x8Bu, 0x97u }, { 0xB0u, 0xC0u, 0x8Bu, 0x97u },
    { 0x20u, 0x30u, 0x8Bu, 0x97u }, { 0xB0u, 0xC0u, 0x8Bu, 0x97u }
};

typedef struct {
    const uint8_t* chr;
    const uint8_t* note_freq;
    const uint8_t* music;
    const uint8_t* text;
    const uint8_t* tables;
    const uint8_t* rooms;
} RawSections;

static uint32_t read_le32(const uint8_t* p) {
    return ((uint32_t)p[0])       | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_le16(const uint8_t* p) {
    return (uint16_t)(((uint16_t)p[0]) | ((uint16_t)p[1] << 8));
}

static bool section_range_ok(uint32_t off, uint32_t len, uint32_t expected, size_t total) {
    return (len == expected) && (off >= MJK_ASSETS_HEADER_SIZE) &&
           ((size_t)off <= total) && ((size_t)len <= total - (size_t)off);
}

static const uint8_t* resolve_cpu(const RawSections* s, uint16_t addr, size_t min_bytes) {
    const struct { uint16_t base; size_t len; const uint8_t* ptr; } map[5] = {
        { MJK_CPU_NOTE_FREQ_ADDR,   MJK_NOTE_FREQ_SIZE,   s->note_freq },
        { MJK_CPU_GAME_TABLES_ADDR, MJK_GAME_TABLES_SIZE, s->tables },
        { MJK_CPU_TEXT_ADDR,        MJK_TEXT_SIZE,        s->text },
        { MJK_CPU_WORLD_ROOMS_ADDR, MJK_WORLD_ROOMS_SIZE, s->rooms },
        { MJK_CPU_MUSIC_DATA_ADDR,  MJK_MUSIC_DATA_SIZE,  s->music }
    };
    for (size_t i = 0u; i < 5u; ++i) {
        if (addr >= map[i].base && map[i].ptr) {
            size_t rel = (size_t)(addr - map[i].base);
            if (rel < map[i].len && min_bytes <= (map[i].len - rel)) {
                return map[i].ptr + rel;
            }
        }
    }
    return NULL;
}

static void predecode_chr_tiles(const uint8_t* chr_bank1, uint8_t out_tiles[MJK_TILE_COUNT][8][8]) {
    /* Cada tile NES ocupa 16 bytes: 8 bytes del plano 0 + 8 bytes del plano 1. */
    for (uint32_t t = 0u; t < MJK_TILE_COUNT; ++t) {
        const uint8_t* src = chr_bank1 + (t * 16u);
        for (uint32_t row = 0u; row < 8u; ++row) {
            uint8_t p0 = src[row];
            uint8_t p1 = src[row + 8u];
            for (uint32_t col = 0u; col < 8u; ++col) {
                uint8_t bit = (uint8_t)(7u - col);
                out_tiles[t][row][col] = (uint8_t)(((p0 >> bit) & 1u) | (((p1 >> bit) & 1u) << 1u));
            }
        }
    }
}

static void decode_room_objects_template(const RawSections* s, MjkRoomTemplate* room) {
    room->object_count = 0u;
    if (room->room_id == 0u) {
        return;
    }
    const uint8_t* ptr_entry = resolve_cpu(s, (uint16_t)(ADDR_ROOM_DATA_PTRS + (uint16_t)(room->room_id - 1u) * 2u), 2u);
    if (!ptr_entry) {
        return;
    }
    const uint8_t* rdata = resolve_cpu(s, read_le16(ptr_entry), 1u);
    if (!rdata) {
        return;
    }

    size_t f = 0u;
    if ((rdata[0] & 0xF0u) == 0xE0u) {
        room->has_header    = true;
        room->palette_set   = (uint8_t)(rdata[0] & 0x0Fu);
        room->music_index   = (uint8_t)(rdata[1] & 0x07u);
        room->door_cond_raw = (uint8_t)(rdata[1] >> 3u);
        room->section_raw   = rdata[2];
        room->section_id    = (uint8_t)(rdata[2] & 0x7Fu);
        f = 3u;
    }

    uint8_t bomb_rem = 0u;
    while (rdata[f] != 0xFFu && room->object_count < MJK_ROOM_MAX_OBJECTS) {
        MjkRoomObjectDef def;
        memset(&def, 0, sizeof(def));

        uint8_t op = 0u;
        uint8_t yx = 0u;
        if (bomb_rem > 0u) {
            op = 0u;
            yx = rdata[f++];
            bomb_rem--;
        } else {
            op = rdata[f++];
            yx = rdata[f++];
            if ((op & 0xF0u) == 0x00u) {
                bomb_rem = room->is_bomb_room ? 0x18u : (uint8_t)((op - 1u) & 0x0Fu);
            }
        }

        def.raw_opcode    = op;
        def.raw_yx        = yx;
        def.grid_x        = (uint8_t)(yx & 0x0Fu);
        def.grid_y        = (uint8_t)((yx >> 4u) & 0x0Fu);
        def.is_persistent = ((op & 0xF0u) < 0xB0u);

        uint8_t hi = (uint8_t)(op >> 4u);
        if (hi == 0u) {
            def.kind      = OBJ_KIND_BOMB;
            def.item_kind = ITEM_BOMB;
        } else if (hi <= 4u || hi == 9u || hi == 0x0Au) {
            def.kind          = (hi == 3u || hi == 4u) ? OBJ_KIND_ORANGE_CHEST : OBJ_KIND_RED_CHEST;
            def.item_kind     = (MjkItemKind)(op & 0x0Fu);
            def.starts_hidden = (op >= 0x80u);
            if ((op & 0x0Fu) == 6u) {
                def.raw_extra    = rdata[f++];
                def.sphinx_event = def.raw_extra;
            }
        } else if (hi == 5u || hi == 6u) {
            def.kind          = OBJ_KIND_SECRET_BLOCK;
            def.item_kind     = (MjkItemKind)(op & 0x0Fu);
            def.starts_hidden = true;
        } else if (hi == 7u) {
            def.kind      = OBJ_KIND_LOOSE_ITEM;
            def.item_kind = (MjkItemKind)(op & 0x0Fu);
        } else if (hi == 8u) {
            uint8_t d_slot = (uint8_t)(op & 0x0Fu);
            def.kind      = OBJ_KIND_DOOR;
            def.door_slot = d_slot;
            def.door_arg  = yx;
            def.raw_extra = yx;
            def.raw_yx    = k_door_layout[d_slot][0];
            def.raw_yx2   = k_door_layout[d_slot][1];
            def.grid_x    = (uint8_t)(def.raw_yx & 0x0Fu);
            def.grid_y    = (uint8_t)((def.raw_yx >> 4u) & 0x0Fu);
            def.grid_x2   = (uint8_t)(def.raw_yx2 & 0x0Fu);
            def.grid_y2   = (uint8_t)((def.raw_yx2 >> 4u) & 0x0Fu);
        } else {
            def.kind = OBJ_KIND_META_TRIGGER;
            if (hi == 0x0Eu) {
                def.raw_extra = rdata[f++];
            }
        }

        room->objects[room->object_count++] = def;
    }
}

static bool decode_all_rooms(const RawSections* s, MjkAssets* a) {
    const uint8_t* adj_tbl = resolve_cpu(s, ADDR_ADJACENT_ROOMS, 256u * 5u);
    const uint8_t* halves  = resolve_cpu(s, ADDR_ROOM_HALF_SCREENS, 256u * 2u);
    const uint8_t* layouts = resolve_cpu(s, ADDR_HALF_SCREEN_LAYTS, 130u * 16u);
    const uint8_t* chunks  = resolve_cpu(s, ADDR_LAYOUT_CHUNKS, 216u * 8u);
    if (!adj_tbl || !halves || !layouts || !chunks) {
        return false;
    }

    for (uint32_t r = 0u; r < MJK_ROOM_COUNT; ++r) {
        MjkRoomTemplate* room = &a->rooms[r];
        memset(room, 0, sizeof(*room));
        room->room_id      = (uint8_t)r;
        room->is_bomb_room = (r >= 0x90u && r <= 0x9Fu);

        const uint8_t* ae = adj_tbl + (r * 5u);
        room->adj.flags     = ae[0];
        room->adj.up        = ae[1];
        room->adj.down      = ae[2];
        room->adj.right     = ae[3];
        room->adj.left      = ae[4];
        room->is_horizontal = ((ae[0] & 0x04u) != 0u);

        uint8_t h0 = halves[r * 2u];
        uint8_t h1 = halves[r * 2u + 1u];
        if (h0 >= 130u || h1 >= 130u) {
            return false;
        }

        if (room->is_horizontal) {
            for (uint8_t mx = 0u; mx < MJK_ROOM_COLS; ++mx) {
                uint8_t c0 = layouts[(size_t)h0 * 16u + mx];
                uint8_t c1 = layouts[(size_t)h1 * 16u + mx];
                if (c0 >= 0x60u) { c0 = (uint8_t)(c0 - 0x0Bu); }
                if (c1 >= 0x60u) { c1 = (uint8_t)(c1 - 0x0Bu); }
                if (c0 >= 216u || c1 >= 216u) { return false; }
                for (uint8_t my = 0u; my < 8u; ++my) {
                    room->static_metatiles[my][mx] = chunks[(size_t)c0 * 8u + my];
                }
                for (uint8_t my = 8u; my < MJK_ROOM_ROWS; ++my) {
                    room->static_metatiles[my][mx] = chunks[(size_t)c1 * 8u + (my - 8u)];
                }
            }
        } else {
            for (uint8_t my = 0u; my < MJK_ROOM_ROWS; ++my) {
                uint8_t c0 = layouts[(size_t)h0 * 16u + my];
                uint8_t c1 = layouts[(size_t)h1 * 16u + my];
                if (c0 >= 0x60u) { c0 = (uint8_t)(c0 - 0x0Bu); }
                if (c1 >= 0x60u) { c1 = (uint8_t)(c1 - 0x0Bu); }
                if (c0 >= 216u || c1 >= 216u) { return false; }
                for (uint8_t mx = 0u; mx < 8u; ++mx) {
                    room->static_metatiles[my][mx] = chunks[(size_t)c0 * 8u + mx];
                }
                for (uint8_t mx = 8u; mx < MJK_ROOM_COLS; ++mx) {
                    room->static_metatiles[my][mx] = chunks[(size_t)c1 * 8u + (mx - 8u)];
                }
            }
        }
        decode_room_objects_template(s, room);
    }
    return true;
}

static bool decode_tables_and_strings(const RawSections* s, MjkAssets* a) {
    /* Paletas, metatiles y sprites */
    memcpy(a->master_palette, k_nes_palette_2c02, sizeof(a->master_palette));
    const uint8_t* p = resolve_cpu(s, ADDR_MAIN_PALETTE, 32u);
    if (!p) { return false; }
    memcpy(a->main_palette, p, 32u);

    p = resolve_cpu(s, ADDR_BG_PALETTE_SETS, 9u * 12u);
    if (!p) { return false; }
    memcpy(a->room_palettes, p, 9u * 12u);

    p = resolve_cpu(s, ADDR_MIGHTY_COLORS, 4u);
    if (!p) { return false; }
    memcpy(a->mighty_colors, p, 4u);

    p = resolve_cpu(s, ADDR_METATILE_DEFS, MJK_METATILE_COUNT * 4u);
    if (!p) { return false; }
    memcpy(a->metatile_tiles, p, MJK_METATILE_COUNT * 4u);

    p = resolve_cpu(s, ADDR_TILE_ATTR_TABLE, 40u);
    if (!p) { return false; }
    for (uint32_t m = 0u; m < MJK_METATILE_COUNT; ++m) {
        uint8_t b = p[m >> 2u];
        a->metatile_palette[m] = (uint8_t)((b >> (6u - 2u * (m & 3u))) & 3u);
    }

    p = resolve_cpu(s, ADDR_ITEM_TO_TILE, 18u);
    if (!p) { return false; }
    memcpy(a->item_to_metatile, p, 18u);

    p = resolve_cpu(s, ADDR_SPRITE_ATTR_TABLE, 8u);
    if (!p) { return false; }
    memcpy(a->sprite_attr_table, p, 8u);

    p = resolve_cpu(s, ADDR_SPRITES_TABLE, MJK_SPRITE_DEF_COUNT * 5u);
    if (!p) { return false; }
    for (uint32_t i = 0u; i < MJK_SPRITE_DEF_COUNT; ++i) {
        a->sprites[i].flip_bits = p[i * 5u];
        memcpy(a->sprites[i].tiles, p + i * 5u + 1u, 4u);
    }

    /* Fisica en float exacto (multiplos de 1/256 son exactos en IEEE-754) */
    p = resolve_cpu(s, ADDR_JUMP_TABLE, 64u * 2u);
    if (!p) { return false; }
    for (uint32_t i = 0u; i < 64u; ++i) {
        a->jump_speed[i] = (float)read_le16(p + i * 2u) / 256.0f;
    }

    p = resolve_cpu(s, ADDR_JUMP_RATE_TABLE, 4u * 2u);
    if (!p) { return false; }
    for (uint32_t i = 0u; i < 4u; ++i) {
        a->jump_rate[i] = (float)read_le16(p + i * 2u) / 256.0f;
    }

    /* Puntuacion BCD -> uint32_t plano */
    p = resolve_cpu(s, ADDR_SCORE_ADD_TABLE, 32u * 2u);
    if (!p) { return false; }
    static const uint32_t k_byte_mult[4] = { 1u, 100u, 10000u, 1000000u };
    for (uint32_t i = 0u; i < 32u; ++i) {
        uint8_t b_idx = p[i * 2u];
        uint8_t bcd   = p[i * 2u + 1u];
        uint32_t dec  = (uint32_t)(((bcd >> 4u) * 10u) + (bcd & 0x0Fu));
        a->score_values[i] = (b_idx < 4u) ? (dec * k_byte_mult[b_idx]) : 0u;
    }

    memcpy(a->mighty_a_presses, resolve_cpu(s, ADDR_MIGHTY_A_PRESSES, 4u), 4u);
    memcpy(a->multiplied_score, resolve_cpu(s, ADDR_MULTIPLIED_SCORE, 32u), 32u);
    memcpy(a->fire_bomb_bonus, resolve_cpu(s, ADDR_FIRE_BOMB_BONUS, 4u), 4u);
    memcpy(a->door_entry_xy, resolve_cpu(s, ADDR_DOOR_ENTRY_XY, 32u), 32u);
    memcpy(a->door_walk_vec, resolve_cpu(s, ADDR_VECTOR_TABLE, 8u), 8u);
    memcpy(a->music_options, resolve_cpu(s, ADDR_MUSIC_OPTIONS, 8u), 8u);

    p = resolve_cpu(s, ADDR_MAYBE_ENTRY_TYPE, 32u);
    if (!p) { return false; }
    for (uint32_t i = 0u; i < 16u; ++i) {
        a->bomb_room_entry_door[i] = p[i * 2u];
        a->bomb_room_exit_door[i]  = p[i * 2u + 1u];
    }

    /* Tablas de enemigos */
    memcpy(a->difficulty_table, resolve_cpu(s, ADDR_DIFFICULTY_TABLE, 64u), 64u);
    p = resolve_cpu(s, ADDR_ENEMY_SPEED_TABLE, 8u * 2u);
    if (!p) { return false; }
    for (uint32_t i = 0u; i < 8u; ++i) {
        a->enemy_speed[i] = (float)read_le16(p + i * 2u) / 256.0f;
    }
    memcpy(a->gameido_div, resolve_cpu(s, ADDR_GAMEIDO_DIV_TABLE, 8u), 8u);
    memcpy(a->enemy_init_status, resolve_cpu(s, ADDR_ENEMY_INIT_STATUS, 17u), 17u);
    memcpy(a->enemy_spawn_pos, resolve_cpu(s, ADDR_ENEMY_SPAWN_POS, 512u), 512u);

    /* Secciones (SectionRoomsTable) */
    a->section_count = 0u;
    for (uint32_t sec = 0u; sec < MJK_SECTION_MAX_COUNT; ++sec) {
        const uint8_t* sp = resolve_cpu(s, (uint16_t)(ADDR_SECTION_ROOMS_TBL + sec * 2u), 2u);
        if (!sp) { break; }
        uint16_t target = read_le16(sp);
        const uint8_t* list = resolve_cpu(s, target, 1u);
        if (!list) { break; }
        for (uint32_t j = 0u; j < MJK_SECTION_MAX_ROOMS; ++j) {
            a->section_rooms[sec][j] = list[j];
            if (list[j] == 0u) { break; }
        }
        a->section_count = (uint8_t)(sec + 1u);
    }

    /* 32 cadenas de texto REV_US */
    for (uint32_t id = 0u; id < (uint32_t)STR_COUNT; ++id) {
        const uint8_t* sp = resolve_cpu(s, (uint16_t)(ADDR_STRING_PTR_TABLE + id * 2u), 2u);
        if (!sp) { return false; }
        uint16_t str_addr = read_le16(sp);
        const uint8_t* raw = resolve_cpu(s, str_addr, 3u);
        if (!raw) { return false; }
        uint16_t ppu_off = read_le16(raw);
        a->strings[id].nt_idx = (uint8_t)((ppu_off >> 10u) & 1u);
        a->strings[id].row    = (uint8_t)((ppu_off >> 5u) & 31u);
        a->strings[id].col    = (uint8_t)(ppu_off & 31u);
        uint8_t len = 0u;
        while (len < MJK_STRING_MAX_LEN && raw[2u + len] != 0xFFu) {
            a->strings[id].tiles[len] = raw[2u + len];
            len++;
        }
        a->strings[id].length = len;
    }

    /* Bloque de audio 2A03 */
    memcpy(a->audio.note_freq_table, s->note_freq, MJK_NOTE_FREQ_SIZE);
    memcpy(a->audio.music_data, s->music, MJK_MUSIC_DATA_SIZE);
    a->audio.sound_cpu_addrs[0] = 0u;
    for (uint32_t id = 1u; id < (uint32_t)SOUND_COUNT; ++id) {
        const uint8_t* sp = resolve_cpu(s, (uint16_t)(ADDR_SOUND_POINTERS + (id - 1u) * 2u), 2u);
        a->audio.sound_cpu_addrs[id] = sp ? read_le16(sp) : 0u;
    }
    return true;
}

MjkAssetError MjkAssets_LoadFromAssetsMemory(const uint8_t* data, size_t size, MjkAssets* out_assets) {
    if (!out_assets) {
        return ASSET_ERR_IO;
    }
    memset(out_assets, 0, sizeof(*out_assets));
    if (!data || size < MJK_ASSETS_HEADER_SIZE) {
        return ASSET_ERR_BAD_SIZE;
    }
    if (memcmp(data, MJK_ASSETS_MAGIC, 8u) != 0) {
        return ASSET_ERR_BAD_MAGIC;
    }
    if (read_le32(data + 0x08u) != MJK_ASSETS_VERSION_V2) {
        return ASSET_ERR_BAD_VERSION;
    }
    uint32_t total_size = read_le32(data + 0x0Cu);
    if ((size_t)total_size != size || total_size < MJK_ASSETS_TOTAL_SIZE) {
        return ASSET_ERR_BAD_SIZE;
    }

    uint32_t expected_crc  = read_le32(data + 0x10u);
    uint32_t chr_off       = read_le32(data + 0x14u);
    uint32_t chr_sz        = read_le32(data + 0x18u);
    uint32_t audio_off     = read_le32(data + 0x1Cu);
    uint32_t audio_sz      = read_le32(data + 0x20u);
    uint32_t text_off      = read_le32(data + 0x24u);
    uint32_t text_sz       = read_le32(data + 0x28u);
    uint32_t tables_off    = read_le32(data + 0x2Cu);
    uint32_t tables_sz     = read_le32(data + 0x30u);
    uint32_t rooms_off     = read_le32(data + 0x34u);
    uint32_t rooms_sz      = read_le32(data + 0x38u);

    if (!section_range_ok(chr_off, chr_sz, MJK_ROM_CHR_SIZE, size) ||
        !section_range_ok(audio_off, audio_sz, MJK_AUDIO_SECTION_SIZE, size) ||
        !section_range_ok(text_off, text_sz, MJK_TEXT_SIZE, size) ||
        !section_range_ok(tables_off, tables_sz, MJK_GAME_TABLES_SIZE, size) ||
        !section_range_ok(rooms_off, rooms_sz, MJK_WORLD_ROOMS_SIZE, size)) {
        return ASSET_ERR_BAD_SIZE;
    }

    uint32_t actual_crc = MjkHash_Crc32(data + MJK_ASSETS_HEADER_SIZE, size - MJK_ASSETS_HEADER_SIZE);
    if (actual_crc != expected_crc) {
        return ASSET_ERR_BAD_CRC;
    }

    RawSections s;
    s.chr       = data + chr_off;
    s.note_freq = data + audio_off;
    s.music     = data + audio_off + MJK_NOTE_FREQ_SIZE;
    s.text      = data + text_off;
    s.tables    = data + tables_off;
    s.rooms     = data + rooms_off;

    /* CHR Bank 1 (offset 8192) contiene los 512 tiles reales del juego */
    predecode_chr_tiles(s.chr + MJK_ROM_CHR_BANK_SIZE, out_assets->tiles);
    if (!decode_tables_and_strings(&s, out_assets) || !decode_all_rooms(&s, out_assets)) {
        memset(out_assets, 0, sizeof(*out_assets));
        return ASSET_ERR_BAD_SIZE;
    }

    out_assets->payload_crc32 = expected_crc;
    return ASSET_OK;
}

MjkAssetError MjkAssets_LoadFromAssetsFile(const char* path, MjkAssets* out_assets) {
    if (!path || !out_assets) {
        return ASSET_ERR_IO;
    }
    FILE* f = fopen(path, "rb");
    if (!f) {
        return ASSET_ERR_IO;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return ASSET_ERR_IO; }
    long len = ftell(f);
    if (len <= 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return ASSET_ERR_IO; }
    size_t sz = (size_t)len;
    if (sz > 1024u * 1024u) { fclose(f); return ASSET_ERR_BAD_SIZE; }

    uint8_t* buf = (uint8_t*)malloc(sz);
    if (!buf) { fclose(f); return ASSET_ERR_OUT_OF_MEMORY; }
    size_t got = fread(buf, 1u, sz, f);
    fclose(f);
    if (got != sz) { free(buf); return ASSET_ERR_IO; }

    MjkAssetError err = MjkAssets_LoadFromAssetsMemory(buf, sz, out_assets);
    free(buf);
    return err;
}

MjkAssetError MjkAssets_LoadFromRomFile(const char* rom_path, MjkAssets* out_assets) {
    if (!rom_path || !out_assets) {
        return ASSET_ERR_IO;
    }
    FILE* f = fopen(rom_path, "rb");
    if (!f) {
        return ASSET_ERR_IO;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return ASSET_ERR_IO; }
    long fsz = ftell(f);
    if (fsz != (long)MJK_ROM_EXPECTED_SIZE || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return ASSET_ERR_BAD_SIZE;
    }
    uint8_t* rom = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom) { fclose(f); return ASSET_ERR_OUT_OF_MEMORY; }
    size_t got = fread(rom, 1u, MJK_ROM_EXPECTED_SIZE, f);
    fclose(f);
    if (got != MJK_ROM_EXPECTED_SIZE) {
        free(rom);
        return ASSET_ERR_BAD_SIZE;
    }

    uint8_t* dat = NULL;
    size_t dat_sz = 0u;
    MjkAssetError err = MjkRom_BuildAssetsDat(rom, MJK_ROM_EXPECTED_SIZE, &dat, &dat_sz, NULL);
    free(rom);
    if (err != ASSET_OK) {
        return err;
    }
    err = MjkAssets_LoadFromAssetsMemory(dat, dat_sz, out_assets);
    free(dat);
    return err;
}

void MjkAssets_Unload(MjkAssets* assets) {
    if (assets) {
        memset(assets, 0, sizeof(*assets));
    }
}

void MjkAssets_GetDoorLayout(uint8_t door_slot,
                             uint8_t* out_x0, uint8_t* out_y0,
                             uint8_t* out_x1, uint8_t* out_y1,
                             uint8_t* out_normal_mt, uint8_t* out_sealed_mt) {
    uint8_t idx = (uint8_t)(door_slot & 0x0Fu);
    if (out_x0)        { *out_x0        = (uint8_t)(k_door_layout[idx][0] & 0x0Fu); }
    if (out_y0)        { *out_y0        = (uint8_t)((k_door_layout[idx][0] >> 4u) & 0x0Fu); }
    if (out_x1)        { *out_x1        = (uint8_t)(k_door_layout[idx][1] & 0x0Fu); }
    if (out_y1)        { *out_y1        = (uint8_t)((k_door_layout[idx][1] >> 4u) & 0x0Fu); }
    if (out_normal_mt) { *out_normal_mt = k_door_layout[idx][2]; }
    if (out_sealed_mt) { *out_sealed_mt = k_door_layout[idx][3]; }
}
