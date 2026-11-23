/**
 * @file mjk_room_decoder.h
 * @brief Decodificador 4-tier de las 256 salas (0x00..0xFF) y persistencia de objetos.
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Reconstruye la geometría 16x15 de metatiles 16x16 desde las 4 capas comprimidas
 * en ROM (`RoomHalfScreens` -> `HalfScreenLayouts` -> `LayoutChunks` -> `MetatileDefinitions`)
 * y decodifica los objetos dinámicos (bombas, cofres, puertas, ítems) con la misma
 * máquina de persistencia de 8 slots + scratch de `RoomDataRAM` ($0603..$0682).
 */

#ifndef MJK_ROOM_DECODER_H
#define MJK_ROOM_DECODER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_build_info.h"
#include "mjk_ppu.h"

#define MJK_ROOM_MT_COLS         16u
#define MJK_ROOM_MT_ROWS         15u
#define MJK_ROOM_MAX_OBJECTS     32u
#define MJK_ROOM_PERSIST_SLOTS   8u
#define MJK_ROOM_SLOT_OBJ_BYTES  14u

typedef struct {
    uint8_t flags;
    uint8_t up;
    uint8_t down;
    uint8_t right;
    uint8_t left;
} MjkRoomAdjacency;

typedef struct {
    uint8_t opcode;
    uint8_t yx;
    uint8_t yx2;
    uint8_t extra;
    uint8_t ram_state;
    uint8_t ram_index;
} MjkRoomObject;

typedef struct {
    uint8_t room_id;
    uint8_t padding;
    uint8_t objects[MJK_ROOM_SLOT_OBJ_BYTES];
} MjkRoomPersistSlot;

typedef struct {
    MjkRoomPersistSlot slots[MJK_ROOM_PERSIST_SLOTS];
    MjkRoomPersistSlot scratch;
    uint8_t            sphinx_flags[16];
} MjkRoomPersistRam;

typedef struct {
    uint8_t          room_id;
    MjkRoomAdjacency adj;
    uint8_t          palette_set;
    uint8_t          music_id;
    uint8_t          section_id;
    bool             is_horizontal;
    bool             is_bomb_room;
    uint8_t          static_metatiles[MJK_ROOM_MT_ROWS][MJK_ROOM_MT_COLS];
    uint8_t          metatiles[MJK_ROOM_MT_ROWS][MJK_ROOM_MT_COLS];
    uint8_t          attributes[MJK_ROOM_MT_ROWS][MJK_ROOM_MT_COLS];
    MjkRoomObject    objects[MJK_ROOM_MAX_OBJECTS];
    uint8_t          object_count;
} MjkRoomState;

MJK_CORE_API bool MjkRoom_DecodeGeometry(const MjkAssetPackage* pkg,
                                         uint8_t room_id,
                                         MjkRoomState* out_room);

MJK_CORE_API bool MjkRoom_PreloadSection(const MjkAssetPackage* pkg,
                                         uint8_t room_id,
                                         MjkRoomPersistRam* persist,
                                         uint8_t crystal_balls_mask);

MJK_CORE_API bool MjkRoom_DecodeObjects(const MjkAssetPackage* pkg,
                                        uint8_t room_id,
                                        MjkRoomPersistRam* persist,
                                        MjkRoomState* room);

MJK_CORE_API void MjkRoom_BlitToPpu(const MjkRoomState* room,
                                    const MjkAssetPackage* pkg,
                                    MjkPpu* ppu,
                                    uint8_t nt_idx);

#endif /* MJK_ROOM_DECODER_H */
