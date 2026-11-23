/**
 * @file entities.h
 * @brief Interactive room entities (Bombs, 23-bomb chain, Chests, Mighty Coins,
 *        Power Coins, and Crystal Balls) for Mighty Bomb Jack (NES).
 * 
 * Directly conforms to:
 *   - RES-007: Bytecode de Objetos, Bombas y Cofres
 *   - prg.asm:4970-5164 (JTPickup_Bomb, JTPickup_Chest)
 */

#ifndef MBJ_ENTITIES_H
#define MBJ_ENTITIES_H

#include <stdint.h>
#include <stdbool.h>
#include "mbj/types.h"
#include "mbj/render.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROOM_ENTITIES 32

typedef enum {
    ENTITY_NONE         = 0x00,
    ENTITY_BOMB         = 0x01,
    ENTITY_CHEST_RED    = 0x02,
    ENTITY_CHEST_ORANGE = 0x03,
    ENTITY_HIDDEN_BLOCK = 0x04,
    ENTITY_MIGHTY_COIN  = 0x05,
    ENTITY_POWER_COIN   = 0x06,
    ENTITY_CRYSTAL_BALL = 0x07,
    ENTITY_SPHINX       = 0x08,
    ENTITY_EXTRA_LIFE   = 0x09
} EntityType;

typedef struct {
    EntityType type;
    uint8_t x_px;
    uint8_t y_px;
    uint8_t state;        /* 0 = collected, 1 = normal, 2 = lit (bomb), 3 = open (chest) */
    uint8_t sub_item;     /* Item spawned when chest/block opens */
    uint8_t hit_counter;  /* Hits remaining for multi-headbutt blocks */
    bool is_lit;          /* For bombs: fuse is actively lit */
} RoomEntity;

/* ============================================================================
 * ENTITY SUBSYSTEM APIS
 * ============================================================================ */

uint8_t Entities_UnpackFromRoom(uint8_t room_id, RoomEntity *out_entities, uint8_t max_entities);

void Entities_Update(void *game_ptr, RoomEntity *entities, uint8_t count);

void Entities_Render(uint32_t *screen_buffer, const RoomEntity *entities, uint8_t count,
                     const CHRBank *bank, uint32_t frame_count);

void Entities_CollectBomb(void *game_ptr, RoomEntity *bomb, RoomEntity *all_bombs, uint8_t total_bombs);

void Entities_OpenChest(void *game_ptr, RoomEntity *chest);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_ENTITIES_H */
