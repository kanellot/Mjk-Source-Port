/**
 * @file world.h
 * @brief Estado de salas activas, camara en float, persistencia y colisiones.
 *
 * `MjkWorld` unifica lo que antes estaba repartido entre `mjk_room_decoder`,
 * `mjk_camera` y consultas duplicadas de colision en `mjk_player`/`mjk_enemy`:
 *  - Instancia de sala actual (`current`) y sala vecina (`neighbor`) en pasillos.
 *  - Persistencia de 8 salas por seccion + 1 ranura scratch para Bomb Rooms.
 *  - Camara suave en `float` con clamping y deteccion de cruce de costura (`room_crossed`).
 *  - Consulta unica de colision (`MjkWorld_IsPointSolid`) compartida por Jack y enemigos.
 */

#ifndef MJK_ENGINE_WORLD_H
#define MJK_ENGINE_WORLD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk/assets.h"
#include "mjk/types.h"

#define MJK_PERSIST_SLOTS       8u
#define MJK_PERSIST_OBJ_CAP     32u
#define MJK_SPHINX_FLAG_COUNT   32u

/** @brief Estado mutable de un objeto dentro de la sala activa. */
typedef struct {
    MjkObjKind  kind;
    uint8_t     grid_x;
    uint8_t     grid_y;
    uint8_t     grid_x2;
    uint8_t     grid_y2;
    MjkItemKind item_kind;
    uint8_t     door_slot;
    uint8_t     door_arg;
    uint8_t     sphinx_event;
    uint8_t     persist_index;
    uint8_t     raw_opcode;

    /* Banderas semanticas claras (reemplazan las bitmasks de ram_state) */
    bool        is_lit;           /**< Bomba encendida (vale 200 pts). */
    bool        is_order_marker;  /**< Bomba de inicio/fin de cadena en pasillo ($85/$C5). */
    bool        is_open;          /**< Puerta abierta y transitable. */
    bool        is_sphinx_locked; /**< Puerta vinculada a un evento de Esfinge. */
    bool        is_hidden;        /**< Cofre/bloque/puerta aun oculto. */
    bool        is_collected;     /**< Bomba o item ya recogido (o cofre vaciado). */
    bool        is_revealed;      /**< Cofre/bloque ya abierto mostrando su item. */
} MjkRoomObj;

/** @brief Sala instanciada en memoria lista para gameplay y renderizado. */
typedef struct {
    uint8_t          room_id;
    MjkRoomAdjacency adj;
    uint8_t          palette_set;
    uint8_t          music_index;
    uint8_t          section_id;
    uint8_t          door_cond_raw;
    bool             is_horizontal;
    bool             is_bomb_room;
    uint8_t          static_metatiles[MJK_ROOM_ROWS][MJK_ROOM_COLS];
    uint8_t          visual_metatiles[MJK_ROOM_ROWS][MJK_ROOM_COLS];
    uint8_t          visual_palettes[MJK_ROOM_ROWS][MJK_ROOM_COLS];
    MjkRoomObj       objects[MJK_ROOM_MAX_OBJECTS];
    uint8_t          object_count;
} MjkRoomInstance;

/** @brief Estado persistido de un objeto en la memoria de seccion. */
typedef struct {
    bool        initialized;
    MjkItemKind item_kind;
    bool        is_lit;
    bool        is_order_marker;
    bool        is_open;
    bool        is_sphinx_locked;
    bool        is_hidden;
    bool        is_collected;
    bool        is_revealed;
} MjkPersistObjState;

/** @brief Ranura de persistencia para una sala dentro de la seccion actual. */
typedef struct {
    uint8_t            room_id;
    uint8_t            count;
    MjkPersistObjState objects[MJK_PERSIST_OBJ_CAP];
} MjkPersistSlot;

/** @brief Persistencia de seccion (8 salas) + scratch (Bomb Rooms) + eventos Esfinge. */
typedef struct {
    MjkPersistSlot slots[MJK_PERSIST_SLOTS];
    MjkPersistSlot scratch;
    uint8_t        sphinx_opened[MJK_SPHINX_FLAG_COUNT]; /**< 0=pendiente, 1=cerrada, 2=abierta. */
} MjkSectionPersist;

/** @brief Camara de sala/pasillo en pixeles `float`. */
typedef struct {
    uint8_t room_flags;
    uint8_t current_room_id;
    uint8_t neighbor_room_id;
    float   scroll_x;
    float   scroll_y;
    float   delta_x;
    float   delta_y;
    bool    room_crossed;
} MjkCamera;

/** @brief Estado completo del mundo activo (sala actual, vecina, camara y persistencia). */
typedef struct {
    MjkRoomInstance   current;
    MjkRoomInstance   neighbor;
    bool              has_neighbor;
    uint8_t           active_palette_set; /**< Paleta fija del pasillo/seccion (no cambia al hacer scroll). */
    MjkSectionPersist persist;
    MjkCamera         camera;
} MjkWorld;

/** @brief Limpia por completo el mundo y su persistencia de seccion. */
void MjkWorld_Reset(MjkWorld* world);

/** @brief Precarga la seccion (si la sala inicia seccion) y carga `current` + `neighbor`. */
bool MjkWorld_LoadRoom(MjkWorld* world,
                       const MjkAssets* assets,
                       uint8_t room_id,
                       uint8_t crystal_balls_mask);

/** @brief Cambia un metatile visual en `room` actualizando automaticamente su paleta. */
void MjkWorld_SetVisualMetatile(MjkRoomInstance* room,
                                const MjkAssets* assets,
                                uint8_t grid_x,
                                uint8_t grid_y,
                                uint8_t metatile_id);

/** @brief Sincroniza un objeto modificado de `room` hacia su ranura de persistencia. */
void MjkWorld_SaveObjectState(MjkWorld* world,
                              const MjkRoomInstance* room,
                              uint8_t obj_index);

/** @brief Precarga la persistencia de seccion para `room_id` si aun no esta cargada. */
void MjkWorld_PreloadSection(MjkWorld* world,
                             const MjkAssets* assets,
                             uint8_t room_id,
                             uint8_t crystal_balls_mask);

/** @brief Busca la ranura de persistencia de `room_id` (o NULL si no existe). */
MjkPersistSlot* MjkWorld_FindPersistSlot(MjkWorld* world, uint8_t room_id);

/**
 * @brief Actualiza la camara del mundo a partir de la posicion y velocidad de Jack.
 * Si se cruza una costura entre salas, actualiza `current` y `neighbor` y marca `room_crossed`.
 */
void MjkWorld_UpdateCamera(MjkWorld* world,
                           const MjkAssets* assets,
                           float* player_x,
                           float* player_y,
                           float player_vx,
                           float player_vy,
                           uint8_t crystal_balls_mask);

/**
 * @brief Consulta si un punto `(world_x, world_y)` en pixeles es solido.
 * Soporta metatiles solidos, los 10 metatiles parciales 8x8 (0x38..0x4B),
 * puertas cerradas, cofres cerrados vs ocultos (solidos solo por arriba) y fuego (0x5C..0x62).
 */
bool MjkWorld_IsPointSolid(const MjkWorld* world,
                           int world_x,
                           int world_y,
                           bool is_ground_probe,
                           uint8_t* out_fire_tile);

/** @brief Consulta de solidez para enemigos (incluye bordes superior/inferior de sala). */
bool MjkWorld_IsEnemyPointSolid(const MjkWorld* world, int world_x, int world_y);

#endif /* MJK_ENGINE_WORLD_H */
