/**
 * @file room_data.h
 * @brief Extracted authentic room geometry, layout chunks, metatiles,
 *        adjacency, and object bytecode tables for Mighty Bomb Jack (US Version).
 * 
 * Auto-generated from asm/src/data.asm and asm/src/roomdata-us.asm.
 */

#ifndef MBJ_ROOM_DATA_H
#define MBJ_ROOM_DATA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t half_0;
    uint8_t half_1;
} RoomHalfScreenEntry;

typedef struct {
    uint8_t flags;
    uint8_t up;
    uint8_t down;
    uint8_t right;
    uint8_t left;
} AdjacentRoomEntry;

typedef struct {
    uint8_t tl;
    uint8_t tr;
    uint8_t bl;
    uint8_t br;
} MetatileDef;

extern const RoomHalfScreenEntry g_room_half_screens[];
extern const uint8_t g_half_screen_layouts[][16];
extern const uint8_t g_layout_chunks[][8];
extern const MetatileDef g_metatile_definitions[];
extern const AdjacentRoomEntry g_adjacent_rooms[];
extern const uint8_t *g_room_data_table[256];

#ifdef __cplusplus
}
#endif

#endif /* MBJ_ROOM_DATA_H */
