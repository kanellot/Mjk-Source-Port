/**
 * @file assets.h
 * @brief Banco de recursos pre-decodificados de Mighty Bomb Jack (`MjkAssets`).
 *
 * PATRON "TRADUCTOR EN LA FRONTERA":
 * Al arrancar el juego, `MjkAssets` lee `assets/assets.dat` (o extrae desde
 * `mbj-us.nes`) y decodifica UNA SOLA VEZ todos los formatos binarios de la NES:
 *  - Los 512 tiles CHR 2bpp pasan a `uint8_t tiles[512][8][8]` (32 KB en cache L1).
 *  - Las tablas de fisica Q8.8 pasan a `float` exactos.
 *  - Las tablas de puntuacion BCD pasan a enteros `uint32_t`.
 *  - Las 256 salas comprimidas en 4 capas pasan a matrices `15x16` y listas de objetos.
 *
 * A partir de aqui, ningun modulo de `src/game/` ni `src/render/` vuelve a tocar
 * direcciones de CPU NES (`$C0E3`, `$C6CA`...) ni formatos empaquetados del 6502.
 */

#ifndef MJK_ENGINE_ASSETS_H
#define MJK_ENGINE_ASSETS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk/rom_loader.h"
#include "mjk/types.h"

#define MJK_SPRITE_DEF_COUNT    46u
#define MJK_ROOM_MAX_OBJECTS    32u
#define MJK_SECTION_MAX_COUNT   64u
#define MJK_SECTION_MAX_ROOMS   8u
#define MJK_STRING_MAX_LEN      48u

/** @brief Definicion de un metasprite 16x16 (4 tiles 8x8 + bits de volteo). */
typedef struct {
    uint8_t flip_bits;
    uint8_t tiles[4];
} MjkSpriteDef;

/** @brief Salas adyacentes y flags de scroll de una sala. */
typedef struct {
    uint8_t flags;
    uint8_t up;
    uint8_t down;
    uint8_t right;
    uint8_t left;
} MjkRoomAdjacency;

/** @brief Categoria semantica de un objeto estatico/dinamico en una sala. */
typedef enum {
    OBJ_KIND_NONE = 0,
    OBJ_KIND_BOMB,
    OBJ_KIND_RED_CHEST,
    OBJ_KIND_ORANGE_CHEST,
    OBJ_KIND_SECRET_BLOCK,
    OBJ_KIND_LOOSE_ITEM,
    OBJ_KIND_DOOR,
    OBJ_KIND_META_TRIGGER
} MjkObjKind;

/** @brief Descripcion pre-decodificada de un objeto dentro de la plantilla de sala. */
typedef struct {
    MjkObjKind  kind;
    uint8_t     grid_x;       /**< Columna de metatile [0..15]. */
    uint8_t     grid_y;       /**< Fila de metatile [0..14]. */
    uint8_t     grid_x2;      /**< Segunda columna (solo puertas). */
    uint8_t     grid_y2;      /**< Segunda fila (solo puertas). */
    MjkItemKind item_kind;    /**< Item contenido (cofre, secreto o suelto). */
    uint8_t     door_slot;    /**< Tipo/ranura de puerta (0..15). */
    uint8_t     door_arg;     /**< Argumento de puerta (bits de Esfinge / estado inicial). */
    uint8_t     sphinx_event; /**< Indice de evento de Esfinge si aplica. */
    bool        starts_hidden;/**< true si cofre/bloque comienza oculto. */
    bool        is_persistent;/**< true si ocupa ranura de persistencia en la sala. */
    uint8_t     raw_opcode;   /**< Opcode original (conservado solo para trazabilidad). */
    uint8_t     raw_yx;       /**< Coordenada YX original. */
    uint8_t     raw_yx2;      /**< Segunda coordenada YX original. */
    uint8_t     raw_extra;    /**< Byte extra original. */
} MjkRoomObjectDef;

/** @brief Plantilla inmutable pre-decodificada de una de las 256 salas (`0x00..0xFF`). */
typedef struct {
    uint8_t          room_id;
    MjkRoomAdjacency adj;
    uint8_t          palette_set;
    uint8_t          music_index;
    uint8_t          section_id;
    uint8_t          section_raw;      /**< Byte 2 de cabecera ($Ex) para detectar inicio de seccion. */
    uint8_t          door_cond_raw;    /**< (header[1] >> 3) para condiciones de puerta. */
    bool             has_header;
    bool             is_horizontal;
    bool             is_bomb_room;
    uint8_t          static_metatiles[MJK_ROOM_ROWS][MJK_ROOM_COLS];
    MjkRoomObjectDef objects[MJK_ROOM_MAX_OBJECTS];
    uint8_t          object_count;
} MjkRoomTemplate;

/** @brief Cadena de texto de pantalla pre-decodificada. */
typedef struct {
    uint8_t nt_idx;
    uint8_t col;
    uint8_t row;
    uint8_t length;
    uint8_t tiles[MJK_STRING_MAX_LEN];
} MjkTextString;

/** @brief Bloque de datos de audio encapsulado (solo accedido por `src/audio/`). */
typedef struct {
    uint8_t  note_freq_table[MJK_NOTE_FREQ_SIZE];
    uint8_t  music_data[MJK_MUSIC_DATA_SIZE];
    uint16_t sound_cpu_addrs[SOUND_COUNT];
} MjkAudioBank;

/** @brief Contenedor global de todos los recursos pre-decodificados del juego. */
typedef struct MjkAssets {
    uint32_t payload_crc32;

    /* Graficos pre-decodificados (cache-friendly) */
    uint8_t      tiles[MJK_TILE_COUNT][MJK_TILE_SIZE][MJK_TILE_SIZE];
    uint32_t     master_palette[64];
    uint8_t      main_palette[32];
    uint8_t      room_palettes[9][12];
    uint8_t      mighty_colors[4];
    uint8_t      metatile_tiles[MJK_METATILE_COUNT][4];
    uint8_t      metatile_palette[MJK_METATILE_COUNT];
    uint8_t      item_to_metatile[18];
    uint8_t      sprite_attr_table[8];
    MjkSpriteDef sprites[MJK_SPRITE_DEF_COUNT];

    /* Fisica y tablas de gameplay en tipos nativos */
    float        jump_speed[64];
    float        jump_rate[4];
    uint8_t      mighty_a_presses[4];
    uint32_t     score_values[32];
    uint8_t      multiplied_score[32];
    uint8_t      fire_bomb_bonus[4];
    uint8_t      door_entry_xy[16][2];
    uint8_t      bomb_room_exit_door[16];
    uint8_t      music_options[8];

    /* Enemigos y secciones */
    uint8_t      difficulty_table[16][4];
    float        enemy_speed[8];
    uint8_t      gameido_div[8];
    uint8_t      enemy_init_status[17];
    uint8_t      enemy_spawn_pos[MJK_ROOM_COUNT][2];
    uint8_t      section_rooms[MJK_SECTION_MAX_COUNT][MJK_SECTION_MAX_ROOMS];
    uint8_t      section_count;

    /* 256 salas y 32 cadenas de texto */
    MjkRoomTemplate rooms[MJK_ROOM_COUNT];
    MjkTextString   strings[STR_COUNT];

    /* Datos de audio 2A03 */
    MjkAudioBank    audio;
} MjkAssets;

MjkAssetError MjkAssets_LoadFromAssetsMemory(const uint8_t* data, size_t size, MjkAssets* out_assets);
MjkAssetError MjkAssets_LoadFromAssetsFile(const char* path, MjkAssets* out_assets);
MjkAssetError MjkAssets_LoadFromRomFile(const char* rom_path, MjkAssets* out_assets);
void          MjkAssets_Unload(MjkAssets* assets);

/** @brief Devuelve metadatos estructurales de posicion/metatiles de una puerta (0..15). */
void MjkAssets_GetDoorLayout(uint8_t door_slot,
                             uint8_t* out_x0, uint8_t* out_y0,
                             uint8_t* out_x1, uint8_t* out_y1,
                             uint8_t* out_closed_mt, uint8_t* out_open_mt);

#endif /* MJK_ENGINE_ASSETS_H */
