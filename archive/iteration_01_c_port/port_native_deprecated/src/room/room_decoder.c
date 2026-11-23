/**
 * @file room_decoder.c
 * @brief 4-Tier Room Geometry Decoder and Background Metatile Renderer
 *        for Mighty Bomb Jack (NES US Revision).
 * 
 * Directly conforms to:
 *   - RES-007: Arquitectura del Decodificador de Salas 4-Tier
 *   - ADR-004: Golden Standard US (REV_US)
 */

#include "mbj/room.h"
#include <string.h>

void Room_DecodeGeometry(uint8_t room_id, ActiveRoom *out_room) {
    if (!out_room) return;
    memset(out_room, 0, sizeof(ActiveRoom));

    out_room->room_id = room_id;
    out_room->adjacency = g_adjacent_rooms[room_id];
    out_room->is_horizontal = (out_room->adjacency.flags & 0x04) != 0;

    RoomHalfScreenEntry half = g_room_half_screens[room_id];

    /* 4-Tier Geometry Decoding (RES-007) */
    if (out_room->is_horizontal) {
        /* Horizontal Room: half_0 = top 8 rows, half_1 = bottom 7 rows */
        for (int c = 0; c < MBJ_ROOM_COLS; c++) {
            /* Top half (rows 0..7) */
            uint8_t chunk_top = g_half_screen_layouts[half.half_0][c];
            if (chunk_top >= 0x60) chunk_top -= 0x0B; /* Authentic compression quirk */

            for (int r = 0; r < 8; r++) {
                out_room->metatiles[r][c] = g_layout_chunks[chunk_top][r];
            }

            /* Bottom half (rows 8..14) */
            uint8_t chunk_bot = g_half_screen_layouts[half.half_1][c];
            if (chunk_bot >= 0x60) chunk_bot -= 0x0B;

            for (int r = 8; r < 15; r++) {
                out_room->metatiles[r][c] = g_layout_chunks[chunk_bot][r - 8];
            }
        }
    } else {
        /* Vertical Room: half_0 = left 8 cols, half_1 = right 8 cols */
        for (int r = 0; r < MBJ_ROOM_ROWS; r++) {
            /* Left half (cols 0..7) */
            uint8_t chunk_left = g_half_screen_layouts[half.half_0][r];
            if (chunk_left >= 0x60) chunk_left -= 0x0B;

            for (int c = 0; c < 8; c++) {
                out_room->metatiles[r][c] = g_layout_chunks[chunk_left][c];
            }

            /* Right half (cols 8..15) */
            uint8_t chunk_right = g_half_screen_layouts[half.half_1][r];
            if (chunk_right >= 0x60) chunk_right -= 0x0B;

            for (int c = 8; c < 16; c++) {
                out_room->metatiles[r][c] = g_layout_chunks[chunk_right][c - 8];
            }
        }
    }

    /* Unpack room bytecode header if present */
    const uint8_t *data = g_room_data_table[room_id];
    if (data && data[0] >= 0xE0 && data[0] <= 0xEF) {
        out_room->palette_id = data[0] & 0x0F;
        out_room->door_condition = (data[1] >> 3) & 0x1F;
        out_room->music_id = data[1] & 0x07;
        out_room->section_id = data[2];
    }
}

void Room_RenderBackground(uint32_t *screen_buffer, const ActiveRoom *room,
                           const CHRBank *bank, const uint8_t bg_palette[16]) {
    if (!screen_buffer || !room || !bank || !bg_palette) return;

    /* Render 15x16 metatiles onto 256x240 screen buffer */
    for (int r = 0; r < MBJ_ROOM_ROWS; r++) {
        for (int c = 0; c < MBJ_ROOM_COLS; c++) {
            uint8_t meta_id = room->metatiles[r][c];
            MetatileDef def = g_metatile_definitions[meta_id];

            int px = c * 16;
            int py = r * 16;

            /* Background tiles reside in Pattern Table 1 (tiles 256..511) */
            uint16_t t_tl = (uint16_t)(def.tl + 256);
            uint16_t t_tr = (uint16_t)(def.tr + 256);
            uint16_t t_bl = (uint16_t)(def.bl + 256);
            uint16_t t_br = (uint16_t)(def.br + 256);

            /* Default background subpalette 0 */
            Renderer_BlitTile(screen_buffer, px, py, bank, t_tl, bg_palette, false, false, false);
            Renderer_BlitTile(screen_buffer, px + 8, py, bank, t_tr, bg_palette, false, false, false);
            Renderer_BlitTile(screen_buffer, px, py + 8, bank, t_bl, bg_palette, false, false, false);
            Renderer_BlitTile(screen_buffer, px + 8, py + 8, bank, t_br, bg_palette, false, false, false);
        }
    }
}
