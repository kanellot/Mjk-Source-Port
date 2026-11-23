/**
 * @file mjk_room_data.h
 * @brief Tablas canónicas de geometría 4-tier, metatiles, paletas y salas NES.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: Disassembly 6502 (asm/src/data.asm y asm/src/roomdata-us.asm)
 */

#ifndef MJK_ROOM_DATA_H
#define MJK_ROOM_DATA_H

#include <stdint.h>
#include <stdbool.h>

#define MJK_TOTAL_ROOMS_COUNT       257
#define MJK_LAYOUTS_COUNT           132
#define MJK_CHUNKS_COUNT            215
#define MJK_METATILES_COUNT         153
#define MJK_PALETTE_SETS_COUNT      9

// RoomDataRAM: 8 slots de sección + 1 slot scratch (RES-018 y ram.asm:1846-1990)
#define MJK_ROOMDATA_SLOT_SIZE      16
#define MJK_ROOMDATA_SECTION_SLOTS  8
#define MJK_ROOMDATA_SCRATCH_SLOT   8
#define MJK_ROOMDATA_TOTAL_SLOTS    9
#define MJK_SECTIONS_COUNT          42

// Flags canónicos de estado dinámico de objetos en RoomDataRAM (RES-018)
#define MJK_OBJ_STATE_LIT_INITIAL   0x80  ///< Bomba encendida inicial de sala
#define MJK_OBJ_STATE_LIT_FUSE      0x40  ///< Bomba con mecha activa en cadena
#define MJK_OBJ_STATE_DOOR_OPEN     0x40  ///< Puerta desbloqueada y abierta (bit 6)
#define MJK_OBJ_STATE_HIDDEN        0x20  ///< Objeto oculto / invisible (bit 5)
#define MJK_OBJ_STATE_COLLECTED     0x0F  ///< Objeto recogido / vacío ($0F)
#define MJK_OBJ_STATE_EMPTY         0xFF  ///< Fin de lista / slot no inicializado

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
#endif

typedef struct {
    union {
        uint8_t raw[MJK_ROOMDATA_SLOT_SIZE]; ///< 16 bytes raw contiguos
        struct {
            uint8_t room_id;                 ///< Byte 0: ID de sala (1..256)
            uint8_t padding;                 ///< Byte 1: Padding no usado
            uint8_t objects[14];             ///< Bytes 2..15: Estados de objetos terminados en 0xFF
        };
    };
} MjkRoomDataSlot;

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

typedef struct {
    MjkRoomDataSlot slots[MJK_ROOMDATA_SECTION_SLOTS]; ///< Slots 0..7 ($0603..$0682)
    MjkRoomDataSlot scratch;                           ///< Slot 8 Scratch ($0683..$0692)
} MjkRoomDataRAM;

typedef struct {
    uint8_t half_0; ///< Mitad superior (horizontal) o izquierda (vertical)
    uint8_t half_1; ///< Mitad inferior (horizontal) o derecha (vertical)
} MjkRoomHalfScreen;

typedef struct {
    uint8_t flags;  ///< Bit 2: 1 = Horizontal (+32 PPU), 0 = Vertical (+1 PPU)
    uint8_t up;
    uint8_t down;
    uint8_t right;
    uint8_t left;
} MjkAdjacentRoom;

typedef struct {
    uint8_t tl;     ///< Tile Top-Left
    uint8_t tr;     ///< Tile Top-Right
    uint8_t bl;     ///< Tile Bottom-Left
    uint8_t br;     ///< Tile Bottom-Right
} MjkMetatileDef;

// Metatiles canónicos de puertas (data.asm:1685-1702)
#define MJK_METATILE_DOOR_CLOSED         0x87  ///< Barras sólidas ($38, $39, $38, $39)
#define MJK_METATILE_DOOR_PHASE1         0x88  ///< Fase intermedia 1 ($39, $38, $39, $38)
#define MJK_METATILE_DOOR_PHASE2         0x89  ///< Semi-abierta ($38, $24, $38, $24)
#define MJK_METATILE_DOOR_OPEN           0x8A  ///< Puerta abierta ($24, $38, $24, $38)

#define MJK_METATILE_DOOR_SPECIAL_CLOSED 0x8B  ///< Puerta vertical especial cerrada
#define MJK_METATILE_DOOR_SPECIAL_PHASE1 0x8C  ///< Puerta vertical especial fase 1
#define MJK_METATILE_DOOR_SPECIAL_PHASE2 0x8D  ///< Puerta vertical especial fase 2
#define MJK_METATILE_DOOR_SPECIAL_OPEN   0x8E  ///< Puerta vertical especial abierta

typedef struct {
    uint8_t y_tile;
    uint8_t x_tile;
    uint8_t closed_tile;
    uint8_t open_tile;
} MjkDoorPositionTile;

typedef struct {
    uint8_t y;
    uint8_t x;
} MjkDoorSpawnPos;

// Tablas canónicas extraídas de la ROM original NES (Fase 6 y RES-017 / RES-018)
extern const MjkRoomHalfScreen   g_room_half_screens[MJK_TOTAL_ROOMS_COUNT];
extern const uint8_t             g_half_screen_layouts[MJK_LAYOUTS_COUNT][16];
extern const uint8_t             g_layout_chunks[MJK_CHUNKS_COUNT][8];
extern const MjkMetatileDef      g_metatile_definitions[MJK_METATILES_COUNT];
extern const MjkAdjacentRoom     g_adjacent_rooms[MJK_TOTAL_ROOMS_COUNT];
extern const MjkDoorPositionTile g_door_position_tile_table[16];
extern const MjkDoorSpawnPos     g_door_entry_xy_table[16];
extern const uint8_t             g_tile_attributes[40];
extern const uint8_t             MainPalette[32];
extern const uint8_t             BackgroundPaletteSets[MJK_PALETTE_SETS_COUNT][12];
extern const uint8_t*            g_room_data_table[256];
extern const uint8_t* const      g_section_rooms_table[MJK_SECTIONS_COUNT];
extern const uint8_t             g_item_to_tile_table[32];
extern const uint8_t             g_door_state_tile_table[8];

/**
 * @brief Obtiene la subpaleta de 2 bits (0..3) asignada a un metatile.
 */
static inline uint8_t MjkRoomData_GetMetatileSubpalette(uint8_t metatile_id) {
    if (metatile_id >= 160) return 0;
    uint8_t b = g_tile_attributes[metatile_id >> 2];
    uint8_t shift = (uint8_t)(6 - 2 * (metatile_id & 3));
    return (uint8_t)((b >> shift) & 3);
}

#ifdef __cplusplus
}
#endif

#endif // MJK_ROOM_DATA_H
