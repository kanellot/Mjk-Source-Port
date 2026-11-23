/**
 * @file mjk_player.h
 * @brief Cinemática, física de salto de 64 pasos y colisiones 9-probe de Jack.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: jack_physics_contract.md (asm/src/prg.asm:1304 y data.asm:187-257)
 */

#ifndef MJK_PLAYER_H
#define MJK_PLAYER_H

#include "mjk_types.h"
#include "mjk_platform.h"
#include "mjk_ppu.h"

#define MJK_JUMP_STEPS             64
#define MJK_WALK_SPEED_SUBPIXELS   384  ///< Velocidad horizontal Tecmo: 0x0180 subpíxeles = 1.5 px/f

// Tablas canónicas de trayectoria de salto y modulación (asm/src/data.asm:187-256)
extern const uint16_t k_jump_velocity_table[64];
extern const uint16_t k_jump_rate_table[4];

// Alias canónicos para compatibilidad
#define g_mjk_canonical_jump_table k_jump_velocity_table
#define g_mjk_jump_advance_table   k_jump_rate_table

// Alias canónico de tipo para Jack
typedef JackPlayer mjk_player_t;

// Flags canónicos de CollisionFlag ($03CF - RES-016)
#define CF_RIGHT_WALL   0x01  ///< Bit 0: Muro derecho
#define CF_LEFT_WALL    0x02  ///< Bit 1: Muro izquierdo
#define CF_FLOOR        0x04  ///< Bit 2: Suelo / Plataforma
#define CF_CEILING      0x08  ///< Bit 3: Cabeza / Techo
#define CF_ON_GROUND    0x10  ///< Bit 4: En suelo / sustentación

// Tabla canónica SpritesTable (asm/src/data.asm:364-411, 46 entradas x 5 bytes)
extern const uint8_t g_mjk_sprites_table[46][5];

/**
 * @brief Evalúa la solidez canónica de un punto espacial (px, py) según las reglas de RES-016.
 * FUENTE DE VERDAD: sub_9B6A (asm/src/prg.asm:4480-4611) y byte_A3EA.
 */
bool mjk_player_is_tile_solid(const uint8_t room_metatiles[MJK_ROOM_METATILES_COUNT],
                              int px, int py, mjk_player_t* player, uint8_t probe_idx);

/**
 * @brief Aplica las 9 sondas espaciales canónicas (byte_A3D8) y resuelve las colisiones contra la sala.
 * FUENTE DE VERDAD: RES-016 (byte_A3D8, sub_9965, loc_99BB, sub_A1B9, sub_A1EB)
 */
void mjk_player_update_collision(mjk_player_t *player, const uint8_t room_metatiles[MJK_ROOM_METATILES_COUNT]);

/**
 * @brief Inicializa a Jack en una posición determinada.
 */
void Jack_Init(JackPlayer* jack, int16_t x, int16_t y);

/**
 * @brief Actualiza la cinemática horizontal de Jack (tabla word_C163).
 */
void Jack_UpdateHorizontalVelocity(JackPlayer* jack, const MjkInputState* inputs);

/**
 * @brief Actualiza la física vertical (salto parabólico de 64 pasos, aleteo y gravedad).
 */
void Jack_UpdateVerticalPhysics(JackPlayer* jack, const MjkInputState* inputs);

/**
 * @brief Resuelve las colisiones de Jack mediante la matriz de 9 sondas espaciales.
 * @param room_metatiles Matriz de 16x15 metatiles (1 byte por metatile: 0 = vacío, >0 = sólido).
 */
void Jack_ResolveCollisions(JackPlayer* jack, const uint8_t* room_metatiles);

/**
 * @brief Actualiza la posición, físicas y colisiones completas de Jack en un frame.
 */
void Jack_Update(JackPlayer* jack, const MjkInputState* inputs, const uint8_t* room_metatiles);

/**
 * @brief Actualiza la posición, físicas y colisiones multi-sala completas de Jack en un frame (sub_9A03).
 */
void Jack_UpdateMultiRoom(JackPlayer* jack, const MjkInputState* inputs, const uint8_t* room_metatiles,
                          uint8_t room_id, uint8_t scroll_x, uint8_t scroll_y, uint8_t room_flags);

/**
 * @brief Determina el ID canónico de sprite para Jack en SpritesTable ($00..$2A).
 * FUENTE DE VERDAD: asm/src/prg.asm:1604-1635 y RES-012
 * 
 * @param jack Puntero a la entidad Jack.
 * @param frame_counter Contador global de frames de simulación / renderizado.
 * @return uint8_t Índice canónico del sprite en SpritesTable.
 */
uint8_t Jack_DetermineSprite(const JackPlayer* jack, uint64_t frame_counter);

/**
 * @brief Ensambla los 4 sprites OAM de 8x8 correspondientes al metasprite de Jack de 16x16,
 *        reproduciendo fielmente la rutina WriteSprite de la NES.
 * FUENTE DE VERDAD: asm/src/prg.asm:2267-2374, data.asm:364-411, 588-596
 * 
 * @param sprite_id Índice en SpritesTable ($00..$2D).
 * @param anchor_x Coordenada horizontal central de anclaje (en NES: TempSpriteX).
 * @param anchor_y Coordenada vertical central de anclaje (en NES: TempSpriteY).
 * @param facing_left true si Jack mira a la izquierda (byte_3DD = $00), false si a la derecha ($04).
 * @param out_sprites Array receptor de 4 MjkOamSprite.
 */
void Jack_AssembleMetasprite(uint8_t sprite_id, int16_t anchor_x, int16_t anchor_y,
                             bool facing_left, MjkOamSprite out_sprites[4]);

/**
 * @brief Comprueba colisión AABB entre Jack y una bomba en coordenadas de mundo continuas (scroll_x global).
 * FUENTE DE VERDAD: RES-020 (asm/src/prg.asm:4950 y sub_9A03)
 * 
 * @param jack Puntero a la entidad Jack.
 * @param bomb_room_id ID de sala a la que pertenece la bomba (1..256).
 * @param bomb_tile_x Columna del metatile de la bomba (0..15).
 * @param bomb_tile_y Fila del metatile de la bomba (0..14).
 * @param scroll_x Desplazamiento actual de scroll horizontal de la cámara (0..255).
 * @return true si hay contacto y la bomba es recolectada.
 */
bool Jack_CheckBombCollision(const JackPlayer* jack, uint8_t bomb_room_id,
                             uint8_t bomb_tile_x, uint8_t bomb_tile_y,
                             int16_t scroll_x);

/* ============================================================================
 * NIVELES MIGHTY, MONEDAS Y POWERUPS (TASK-F6-025 / RES-024)
 * ============================================================================ */

extern const uint8_t k_mighty_level_colors[4];
extern const uint8_t k_mighty_level_presses_table[4];

/**
 * @brief Obtiene el color de subpaleta de Jack según su nivel Mighty (0..3).
 * Retorna $16 (lvl 0), $12 (lvl 1), $26 (lvl 2), $2A (lvl 3).
 * FUENTE DE VERDAD: RES-024 (data.asm:306-307 MightyLevelColors)
 */
uint8_t mjk_player_get_palette_color(uint8_t level);

/**
 * @brief Consume una moneda Mighty si mighty_coins > 0 y mighty_level < 3.
 * Incrementa mighty_level, asigna presupuesto según MightyLevelAPressesTable
 * (lvl 1: 20, lvl 2: 30, lvl 3: 35), y si llega a nivel 3 activa enemy_coin_timer = 320.
 * FUENTE DE VERDAD: RES-024 (prg.asm:1745-1768 UseMightyCoin)
 */
bool mjk_player_use_mighty_coin(mjk_player_t* player);

/**
 * @brief Chequeo y degradación de nivel Mighty tras pulsar el botón de salto (botón A).
 * Resta 1 a mighty_presses_left y degrada nivel según umbrales:
 * - < 30 y nivel era 3 -> baja a 2
 * - < 20 y nivel era 2 -> baja a 1
 * - < 1 y nivel era 1 -> baja a 0
 * FUENTE DE VERDAD: RES-024 (prg.asm:1397-1421 _MightyLevelAPressCheck)
 */
uint8_t mjk_player_on_jump_press(mjk_player_t* player);

/**
 * @brief Concede una vida extra (+1UP) a Jack.
 * FUENTE DE VERDAD: RES-024 (prg.asm:5797-5830 MaybeAward1UP)
 */
void mjk_player_award_1up(mjk_player_t* player);

/**
 * @brief Comprueba si Jack puede abrir un cofre naranja (requiere Nivel Mighty >= 1).
 * FUENTE DE VERDAD: RES-024 (prg.asm:5300-5308)
 */
static inline bool can_open_orange_chest(const mjk_player_t* player) {
    return player && (player->mighty_level >= 1 || player->power_level >= MIGHTY_POWER_BLUE);
}

static inline bool mjk_player_can_open_orange_chest(const mjk_player_t* player) {
    return can_open_orange_chest(player);
}

/**
 * @brief Comprueba si Jack puede abrir un cofre rojo desde abajo con cabezazo (requiere Nivel Mighty >= 2).
 * FUENTE DE VERDAD: RES-024 (prg.asm:5259-5296)
 */
static inline bool can_headbutt_red_chest(const mjk_player_t* player) {
    return player && (player->mighty_level >= 2 || player->power_level >= MIGHTY_POWER_ORANGE);
}

static inline bool mjk_player_can_headbutt_red_chest(const mjk_player_t* player) {
    return can_headbutt_red_chest(player);
}

/**
 * @brief Comprueba si Jack puede abrir puertas con condición Mighty 3 (requiere Nivel Mighty == 3).
 * FUENTE DE VERDAD: RES-024 (prg.asm:6903-6909)
 */
static inline bool can_open_mighty3_door(const mjk_player_t* player) {
    return player && (player->mighty_level == 3 || player->power_level >= MIGHTY_POWER_GREEN);
}

static inline bool mjk_player_can_open_mighty3_door(const mjk_player_t* player) {
    return can_open_mighty3_door(player);
}

#endif // MJK_PLAYER_H
