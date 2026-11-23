/**
 * @file mjk_oracle_api.h
 * @brief API C de introspección y co-simulación para el Oráculo Jerárquico de Paridad.
 */

#ifndef MJK_ORACLE_API_H
#define MJK_ORACLE_API_H

#include "mjk_game.h"
#include "mjk_ppu.h"

#ifdef _WIN32
#define MJK_API __declspec(dllexport)
#else
#define MJK_API
#endif

#pragma pack(push, 1)

/**
 * @brief Instantánea canónica normalizada (Snapshot) para comparación con la ROM de NES.
 */
typedef struct {
    // --- NIVEL 1: Macro-Flujo y Máquina de Estados ---
    uint32_t frame;
    uint8_t  game_state;           ///< GameStateId ($0044)
    uint8_t  title_screen_state;   ///< TitleScreenState ($0045)
    uint8_t  in_game;              ///< InGameFlag ($0043)
    uint8_t  current_room;         ///< CurrentRoomID ($03BF)
    uint32_t state_timer;          ///< StateTimer
    uint32_t stage_timer_seconds;  ///< Temporizador de juego
    uint16_t greed_jump_counter;   ///< Contador de saltos de codicia
    uint8_t  lives_count;          ///< Vidas en BCD ($03E4)
    uint8_t  score_bcd[4];         ///< Puntuación en BCD (4 bytes)

    // --- NIVEL 2: Objetos y Persistencia del Mundo ---
    uint8_t  door_tile_x;          ///< Coordenada X de puerta
    uint8_t  door_tile_y;          ///< Coordenada Y de puerta
    uint8_t  door_is_open;         ///< Flag de puerta abierta
    uint8_t  door_condition_id;    ///< Condición activa de puerta
    uint8_t  bomb_count;           ///< Bombas totales en sala
    uint8_t  active_fire_bomb_index; ///< Índice de bomba con mecha activa
    uint8_t  chest_count;          ///< Cofres totales en sala
    uint8_t  room_data_slot[16];   ///< Volcado del Slot RoomDataRAM activo ($0603..)

    // --- NIVEL 3: Actores y Ciclo de Vida de Enemigos ---
    uint8_t  active_enemies_count;
    struct {
        uint8_t type;
        uint8_t status;
        int16_t x;
        int16_t y;
        uint8_t timer;
        uint8_t active;
    } enemies[6];
    uint16_t enemy_coin_timer;     ///< Temporizador de monedas ($006A..$006B)
    uint8_t  spark_visible;        ///< Chispa OAM visible
    uint8_t  spark_x;              ///< Chispa OAM X
    uint8_t  spark_y;              ///< Chispa OAM Y

    // --- NIVEL 4: Cinemática, Físicas Continuas y Jack ---
    int16_t  jack_x;               ///< Coordenada X en píxeles
    int16_t  jack_y;               ///< Coordenada Y en píxeles
    uint8_t  jack_sub_x;           ///< Subpíxel X (0..15)
    uint8_t  jack_sub_y;           ///< Subpíxel Y (0..15)
    int16_t  jack_vel_x;           ///< Velocidad X
    int16_t  jack_vel_y;           ///< Velocidad Y
    uint8_t  jack_state;           ///< Estado cinemático de Jack
    uint8_t  on_ground;            ///< Flag en suelo
    uint8_t  flutter_active;       ///< Flag planeo con alas

    // --- NIVEL 5: Micro-Visual y Sprites OAM ---
    uint8_t  oam_sprites[256];     ///< 64 sprites OAM de hardware ($0200..$02FF)
    uint32_t backdrop_color;       ///< Color RGBA del píxel superior izquierdo (fondo)
} MjkCanonicalSnapshot;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

MJK_API void* MjkOracle_Create(const char* asset_path);
MJK_API void  MjkOracle_Destroy(void* oracle);
MJK_API void  MjkOracle_Step(void* oracle, uint8_t input_mask);
MJK_API void  MjkOracle_GetSnapshot(void* oracle, MjkCanonicalSnapshot* out_snapshot);
MJK_API const uint32_t* MjkOracle_GetFramebuffer(void* oracle);

#ifdef __cplusplus
}
#endif

#endif // MJK_ORACLE_API_H
