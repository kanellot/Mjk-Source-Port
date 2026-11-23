/**
 * @file room.h
 * @brief 4-Tier Room Geometry Decoder, metatile map synthesis,
 *        and 32 US Door Conditions for Mighty Bomb Jack (NES).
 * 
 * Directly conforms to:
 *   - RES-007: Arquitectura del Decodificador de Salas 4-Tier
 *   - ADR-004: Golden Standard US (32 Door Conditions)
 */

#ifndef MBJ_ROOM_H
#define MBJ_ROOM_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/types.h"
#include "mbj/render.h"
#include "mbj/room_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MBJ_ROOM_ROWS 15
#define MBJ_ROOM_COLS 16

typedef struct {
    uint8_t room_id;
    uint8_t palette_id;
    uint8_t music_id;
    uint8_t door_condition;
    uint8_t section_id;
    bool is_horizontal;
    AdjacentRoomEntry adjacency;
    uint8_t metatiles[MBJ_ROOM_ROWS][MBJ_ROOM_COLS];
} ActiveRoom;

/* ============================================================================
 * ROOM DECODING & PRESENTATION APIS
 * ============================================================================ */

void Room_DecodeGeometry(uint8_t room_id, ActiveRoom *out_room);

void Room_RenderBackground(uint32_t *screen_buffer, const ActiveRoom *room,
                           const CHRBank *bank, const uint8_t bg_palette[16]);

/* ============================================================================
 * 32 US DOOR CONDITIONS EVALUATION (ADR-004)
 * ============================================================================ */

bool Door_EvaluateCondition(uint8_t condition_id, const void *game_ptr);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_ROOM_H */
