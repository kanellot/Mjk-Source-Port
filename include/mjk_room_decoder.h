/**
 * @file mjk_room_decoder.h
 * @brief Decodificador de salas, metatiles, 33 puertas USA y descompresor 4-tier.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: room_decoder_contract.md (asm/src/prg.asm:6741-6865 y roomdata-us.asm)
 */

#ifndef MJK_ROOM_DECODER_H
#define MJK_ROOM_DECODER_H

#include "mjk_types.h"
#include "mjk_game.h"
#include "mjk_room_data.h"

#define MJK_TOTAL_ROOMS           257
#define MJK_BOMBS_PER_ROOM         23
#define MJK_DOOR_CONDITIONS_COUNT  33
#define DOOR_COND_MIGHTY3          0x07  ///< Condición canónica de puerta que exige Mighty Level 3 (prg.asm:6903-6909)

typedef enum {
    METATILE_EMPTY        = 0x00,  ///< Espacio vacío transitable
    METATILE_BRICK_SOLID  = 0x01,  ///< Pared o plataforma sólida indestructible
    METATILE_CHEST_CLOSED = 0x02,  ///< Cofre cerrado
    METATILE_CHEST_OPEN   = 0x03,  ///< Cofre abierto
    METATILE_DOOR_CLOSED  = 0x04,  ///< Puerta cerrada
    METATILE_DOOR_OPEN    = 0x05,  ///< Puerta abierta hacia la siguiente sala
    METATILE_BOMB_NORMAL  = 0x06,  ///< Bomba apagada (100 puntos)
    METATILE_BOMB_FIRE    = 0x07   ///< Bomba encendida / con mecha (200 puntos)
} MetatileType;

typedef struct {
    uint8_t room_id;
    uint8_t metatiles[MJK_ROOM_TILES_W * MJK_ROOM_TILES_H]; // 16x15 = 240 bytes
    uint8_t active_fire_bomb_index;                         // Índice [0..22] de la bomba encendida
    uint8_t fire_bombs_collected;                           // Contador de bombas con fuego recogidas
    uint8_t total_bombs_collected;                          // Contador de bombas totales recogidas
    bool    chain_active;                                   // Flag de secuencia de fuego en curso
    uint8_t door_x;                                         // Posición X de la puerta de salida
    uint8_t door_y;                                         // Posición Y de la puerta de salida
    uint8_t door_condition_id;                              // Índice [0..32] de la condición de apertura
    bool    door_is_open;                                   // Estado de la puerta

    // Estado canónico de bombas y mechas (TASK-F6-022B / RES-022)
    uint8_t maybe_bomb_thing;                               ///< MaybeBombThing ($00F8)
    uint8_t room_bomb_picked_flag;                          ///< byte_F7 ($00F7)
    uint8_t power_coin_bomb_counter;                        ///< MaybeTempCollectableFlag2 ($0356)
    uint8_t power_coin_trigger_flag;                        ///< MaybeTempCollectableFlag ($0357)
    uint8_t fire_bombs_collected_total;                     ///< FireBombsCollected2 ($00AF)
    MjkBombFuseSystem fuse_system;                          ///< Subsistema canónico de mecha y chispa OAM
} MjkRoomContext;

/**
 * @brief Inicializa y decodifica la sala solicitada en el contexto.
 */
void MjkRoom_Load(MjkRoomContext* room, uint8_t room_id);

/**
 * @brief Decodifica el bytecode ROM de una sala e inicializa su slot en RoomDataRAM (ReadRoomData: prg.asm:3257).
 * @param room_id ID de sala (1..256).
 * @param slot_idx Índice de slot en RAM (0..7 sección, 8 scratch).
 * @param ram Puntero a la estructura RoomDataRAM.
 * @param room_index_in_section Índice secuencial de la sala dentro de la sección (0..count-1).
 * @param total_rooms_in_section Cantidad total de salas que componen la sección.
 * @param player_crystal_balls Máscara de bits de bolas de cristal recolectadas (bit 0=bola 9, bit 1=bola 10).
 */
void MjkRoom_ReadRoomData(uint8_t room_id, uint8_t slot_idx, MjkRoomDataRAM* ram,
                          uint8_t room_index_in_section, uint8_t total_rooms_in_section,
                          uint8_t player_crystal_balls);

/**
 * @brief Precarga todas las salas de una sección en los slots de RoomDataRAM (GameState_2_LoadRoom: prg.asm:3197).
 * @param ram Puntero a la estructura RoomDataRAM.
 * @param section_id ID de sección (1-indexado, 1..42).
 * @param player_crystal_balls Máscara de bolas de cristal.
 */
void MjkRoom_InitSection(MjkRoomDataRAM* ram, uint8_t section_id, uint8_t player_crystal_balls);

/**
 * @brief Superpone y blittea los objetos activos y visibles sobre la matriz de metatiles (sub_9696 / sub_9592 / sub_96B7).
 * @param room_id ID de sala.
 * @param slot_data Puntero a los 16 bytes del slot activo en RoomDataRAM.
 * @param out_metatiles Matriz 15x16 de metatiles donde se graban los tiles dinámicos.
 */
void MjkRoom_BlitObjects(uint8_t room_id, const uint8_t* slot_data,
                         uint8_t out_metatiles[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W]);

/**
 * @brief Carga canónica de sala con persistencia de RAM y resolución de slots (IfNotTitleScreenRoomDoThings: prg.asm:3077).
 * @param room Contexto de sala receptor.
 * @param room_id ID de sala a cargar.
 * @param ram Memoria persistente de 8 slots de sección + slot scratch.
 * @param section_id ID de sección para eventual inicialización.
 * @param player_crystal_balls Máscara de bolas de cristal.
 */
void MjkRoom_LoadWithRAM(MjkRoomContext* room, uint8_t room_id, MjkRoomDataRAM* ram,
                         uint8_t section_id, uint8_t player_crystal_balls);

/**
 * @brief Evalúa una de las 33 condiciones de puertas USA (REV_US).
 * @return true si la puerta cumple su condición y debe abrirse.
 */
bool MjkRoom_CheckDoorCondition(uint8_t condition_id, const JackPlayer* jack, const MjkGameState* game);

/**
 * @brief Extrae la condición de puerta de la cabecera de sala (byte 1: bits 7-3 menos 1).
 *        Maneja la sala especial Room $61 forzando condición $1F (RES-017 / loc_A977).
 * @return Índice normalizado de condición (0..31), y setea out_has_condition a true si aplica.
 */
uint8_t MjkRoom_ExtractDoorCondition(uint8_t room_id, bool* out_has_condition);

/**
 * @brief Consulta la matriz topológica AdjacentRoomsTable para determinar la sala destino.
 * @param current_room ID de sala origen (0..256).
 * @param entry_door_type Tipo/orientación de la puerta (bits 3-2: 0=Up, 1=Down, 2=Right, 3=Left).
 * @return ID de la sala contigua (0 si no existe conexión).
 */
uint8_t MjkRoom_GetAdjacentRoom(uint8_t current_room, uint8_t entry_door_type);

/**
 * @brief Desencadena la apertura de la puerta física (HandleOpenDoorMaybe: prg.asm:7030).
 *        Actualiza estado de puerta, encola sonido canónico e inicia GAME_STATE_B_ANIMATE_DOOR_OPEN.
 */
void MjkRoom_HandleOpenDoorMaybe(MjkGameState* game);

/**
 * @brief Apertura automática de salida en salas de bombas tras recolectar 23 bombas (prg.asm:5352).
 */
void MjkRoom_OpenBombRoomDoor(MjkGameState* game);

/**
 * @brief Procesa la recogida de una bomba y gestiona la cadena de 23 bombas.
 * @return Puntos BCD obtenidos (+100 o +200).
 */
uint16_t MjkRoom_CollectBomb(MjkRoomContext* room, uint8_t bomb_index);

/**
 * @brief Implementación canónica de JTPickup_Bomb ($9E5B, prg.asm:4970)
 *        Procesa la recolección de bombas, cadena de mechas, Power Coin y bonus 10.000 pts.
 * @param game Puntero al estado maestro del juego.
 * @param bomb_index Índice de la bomba dentro de game->bombs[].
 * @return Puntos base obtenidos (+100 o +200).
 */
uint16_t MjkRoom_CollectBombCanonical(MjkGameState* game, uint8_t bomb_index);

/**
 * @brief Descomprime la rejilla 16x15 de metatiles de una sala mediante la jerarquía 4-tier.
 * @param room_id ID de sala (0..256).
 * @param out_metatiles Matriz de 15 filas x 16 columnas receptora de los IDs de metatile.
 * @param out_is_horizontal Flag opcional que indica si la sala es horizontal (+32 PPU).
 */
void MjkRoom_DecodeGeometry(uint8_t room_id, uint8_t out_metatiles[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W], bool* out_is_horizontal);

/**
 * @brief Genera la nametable completa de 32x30 tiles de 8x8 y su matriz de subpaletas.
 *        Si room_id == 0, superpone las cadenas auténticas de la Pantalla de Título.
 */
void MjkRoom_BuildNametable(uint8_t room_id, uint8_t out_nametable[30][32], uint8_t out_subpalettes[30][32]);

/**
 * @brief Escribe una cadena de tiles de texto en la nametable en una dirección PPU relativa a $2000.
 */
void MjkRoom_WriteNametableString(uint8_t out_nametable[30][32], uint8_t out_subpalettes[30][32],
                                  uint16_t ppu_addr_rel_2000, const uint8_t* tile_ids,
                                  size_t count, uint8_t subpalette);

/**
 * @brief Carga la paleta NES de 32 bytes de la sala según el algoritmo auténtico de LoadRoomPalette.
 */
void MjkRoom_LoadAuthenticPalettes(uint8_t room_id, uint8_t out_pal_32[32]);

/**
 * @brief Busca el índice de slot en RoomDataRAM (0..7 o 8 scratch) correspondiente a un room_id.
 * @return Índice [0..8], o -1 si no se encuentra cargado.
 */
int MjkRoom_FindSlot(const MjkRoomDataRAM* ram, uint8_t room_id);

/**
 * @brief Sincroniza el estado de un objeto (cofre o bloque) en la ranura activa de RoomDataRAM.
 */
void MjkRoom_SyncObjectStateToRAM(MjkRoomDataRAM* ram, uint8_t room_id, uint8_t ram_slot_idx, uint8_t state);

#endif // MJK_ROOM_DECODER_H
