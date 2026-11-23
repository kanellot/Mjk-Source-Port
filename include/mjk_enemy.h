/**
 * @file mjk_enemy.h
 * @brief IA de los 6 arquetipos de enemigos, spawns, mutación y Power Coins.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: enemy_ai_contract.md (asm/src/prg.asm:8150-8650, 9208-9575)
 *                   asm/src/data.asm:116-141 (DifficultyTable), 1963-2000 (EnemySpawnPositionTable)
 */

#ifndef MJK_ENEMY_H
#define MJK_ENEMY_H

#include "mjk_types.h"

#define MJK_POWER_COIN_DURATION_FRAMES 320 // 5.33 segundos a 60 Hz (prg.asm:9023-9026)
#define MJK_MAX_ALIVE_ENEMIES          4   // Máximo de 4 enemigos simultáneos (prg.asm:8203)

typedef struct {
    uint8_t speed_modifier;
    uint8_t spawn_timer;
    uint8_t transform_timer_normal;
    uint8_t transform_timer_bomb_room;
} MjkDifficultySetting;

typedef struct {
    EnemyEntity enemies[MJK_MAX_ENEMIES];
    uint16_t    transmute_timer;         ///< Temporizador de Power Coin (0 = inactivo)
    uint8_t     transmute_streak;        ///< Racha de monedas recogidas (para escala exponencial)
    uint8_t     spawn_timer;             ///< Temporizador de cadencia de spawn (DifficultyTable)
    uint8_t     transform_timer;         ///< Temporizador de mutación (DifficultyTable)
    uint8_t     speed_modifier;          ///< Modificador de velocidad
    uint8_t     alive_enemy_count;       ///< Conteo de enemigos vivos (máx 4)
    uint8_t     spawn_point_toggle;      ///< Alternador de puntos de spawn
    bool        is_bomb_room;            ///< Flag si la sala actual es bomb room
    uint8_t     current_round;           ///< Ronda para indexar dificultad
    uint8_t     current_room;            ///< Sala para indexar coordenadas de spawn
} MjkEnemyManager;

/**
 * @brief Obtiene la configuración de dificultad según la ronda (data.asm:131-141).
 */
const MjkDifficultySetting* MjkEnemy_GetDifficulty(uint8_t round);

/**
 * @brief Inicializa el gestor de enemigos configurando la dificultad de la ronda y sala.
 */
void MjkEnemy_Init(MjkEnemyManager* mgr, uint8_t round, uint8_t room, bool is_bomb_room);

/**
 * @brief Inicializa el gestor de enemigos con parámetros por defecto (compatibilidad).
 */
void MjkEnemy_InitAll(MjkEnemyManager* mgr);

/**
 * @brief Spawnea un enemigo en un slot libre.
 * @return Índice del slot (0..7) o -1 si todos los slots están ocupados.
 */
int MjkEnemy_Spawn(MjkEnemyManager* mgr, EnemyType type, int16_t x, int16_t y);

/**
 * @brief Gestiona el temporizador de spawning diferido y crea momias según la cadencia original.
 */
void MjkEnemy_TickSpawning(MjkEnemyManager* mgr);

/**
 * @brief Muta una momia al arquetipo determinista (A % 6) + 2 (prg.asm:9362-9375).
 */
void MjkEnemy_MutateMummy(EnemyEntity* enemy, uint8_t a_press_counter);

/**
 * @brief Activa el efecto de la Power Coin transmutando a todos los enemigos durante 320 frames.
 */
void MjkEnemy_ActivatePowerCoin(MjkEnemyManager* mgr);

/**
 * @brief Actualiza la IA, físicas y animaciones de todos los enemigos activos.
 */
void MjkEnemy_UpdateAll(MjkEnemyManager* mgr, const JackPlayer* jack, const uint8_t* room_metatiles, uint8_t a_press_counter);

/**
 * @brief Comprueba colisiones caja-caja (AABB 12x12) entre Jack y los enemigos.
 * @return Puntos ganados por transmutación/destrucción o -1 si Jack muere por contacto con enemigo.
 */
int32_t MjkEnemy_CheckJackCollisions(MjkEnemyManager* mgr, JackPlayer* jack);

#endif // MJK_ENEMY_H
