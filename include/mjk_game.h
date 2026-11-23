/**
 * @file mjk_game.h
 * @brief Orquestador del bucle de simulación a 60 Hz y máquina de estados.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: Pipeline NMI (prg.asm:91-123) y GameHandler (prg.asm:302)
 */

#ifndef MJK_GAME_H
#define MJK_GAME_H

#include "mjk_types.h"
#include "mjk_platform.h"
#include "mjk_assets.h"
#include "mjk_apu.h"
#include "mjk_camera.h"
#include "mjk_room_data.h"
#include "mjk_enemy.h"

#define MJK_PALETTE_SIZE    32
#define MJK_MAX_BOMB_PUFFS   8

/**
 * @brief Entidad canónica de animación de recolección de bomba ($0786..$078D).
 * Reproduce la máquina de estados de sub_A270 y sub_A29C (TASK-F7-009 / RES-036).
 * Fase 1: Nube de humo Metasprite $25 (32 frames).
 * Fase 2: Puntuación flotante canónica 100/200 pts (32 frames).
 */
typedef struct MjkBombPuffEffect {
    bool     active;   ///< byte_786 & 0x80 (Entidad activa)
    uint8_t  phase;    ///< 1: Fase 1 (Humo Metasprite $25), 2: Fase 2 (Score Popup)
    uint8_t  timer;    ///< byte_78C: 32 frames por fase (64 frames en total)
    int16_t  x;        ///< byte_789: Posición central X en pantalla (px)
    int16_t  y;        ///< byte_78B: Posición central Y en pantalla (px)
    uint16_t points;   ///< byte_787: Puntos a mostrar en Fase 2 (100 o 200)
} MjkBombPuffEffect;

typedef struct {
    // Configuración y assets
    MjkAssetPackage* assets;
    
    // Sintetizador de audio chiptune APU 2A03
    MjkApuState      apu;

    // Matriz activa de metatiles (16x15 = 240 bytes)
    uint8_t          room_metatiles[MJK_ROOM_TILES_W * MJK_ROOM_TILES_H];

    // Estado global de la sesión
    bool               in_game;              ///< InGameFlag ($0043): false = Título, true = En partida
    TitleScreenStateId title_screen_state;   ///< TitleScreenState ($0045): 0 a 5
    uint16_t           copyright_timer;      ///< CopyrightScreenTimer ($0088): 260 frames
    fixed8_t           title_jack_x;         ///< Posición X de Jack en Title Screen
    fixed8_t           title_jack_y;         ///< Posición Y de Jack en Title Screen
    fixed8_t           title_jack_vy;        ///< Velocidad Y de caída en Title Screen
    GameStateId        current_state;        ///< GameState ($0044): 0x00 a 0x0C
    uint8_t            sub_state;            ///< Subestado dentro del GameState activo
    uint16_t           state_timer;          ///< Temporizador interno de frames del estado
    
    // Progresión y Puntuación
    uint8_t          current_round;        ///< Ronda actual (1 a 17)
    uint8_t          current_room;         ///< Sala actual (0 a 256)
    uint8_t          lives_count;          ///< Vidas restantes (empieza en 3)
    uint8_t          a_press_counter;      ///< APressCounter ($00FA): pulsaciones de botón A
    uint8_t          score_bcd[4];         ///< Puntuación en BCD (8 dígitos decimales)
    uint8_t          mighty_coins;         ///< Monedas Mighty recogidas (para subir de nivel)
    uint8_t          torture_jumps_left;   ///< Saltos obligatorios en sala de tortura (50 BCD en US)
    uint16_t         stage_timer_seconds;  ///< Temporizador decreciente de la sala
    uint8_t          gdv_value;            ///< Game Display Value (afecta al final desbloqueado)
    uint8_t          high_gdv;             ///< High GDV ($009B)
    uint8_t          extra_lives_found;    ///< ExtraLivesFound ($03E3)
    uint8_t          secret_coins_collected;///< PlayerSecretCoins ($03E5)
    uint8_t          crystal_balls_collected;///< PlayerCrystalBallsCollected ($03DC)
    uint8_t          checkpoint_room_id;   ///< CheckpointRoomIDMaybe ($0344)
    uint8_t          torture_scene_flag;   ///< TortureRoomSceneFlag ($033D)
    uint8_t          torture_message_timer;///< TortureRoomMessageTimer ($009A)

    // Entidades del mundo
    JackPlayer       jack;
    EnemyEntity      enemies[MJK_MAX_ENEMIES];
    RoomObject       objects[MJK_MAX_OBJECTS];
    uint8_t          active_enemies_count;
    uint8_t          active_objects_count;
    MjkEnemyManager  enemy_mgr;                  ///< Gestor de IA, spawning y transmutación de enemigos (TASK-F6-026)

    // Bombas y Mechas (Sprint 4: TASK-F6-022 / TASK-F6-022B)
    MjkBombEntity    bombs[24];
    uint8_t          bomb_count;
    uint8_t          active_fire_bomb_index;
    uint8_t          fire_bombs_collected;
    uint8_t          total_bombs_collected;
    bool             chain_active;
    bool             is_bomb_room;

    // Variables canónicas de mechas y Power Coin (RES-022)
    uint8_t          maybe_bomb_thing;           ///< MaybeBombThing ($00F8): bit 0=started, bit 1=broken, bit 7=perfect bonus
    uint8_t          room_bomb_picked_flag;      ///< byte_F7 ($00F7): 0=primer pickup en sala, 1+=iniciado
    uint8_t          power_coin_bomb_counter;    ///< MaybeTempCollectableFlag2 ($0356): acumulador hasta 20
    uint8_t          power_coin_trigger_flag;    ///< MaybeTempCollectableFlag ($0357): 0=inactivo, 1=armado para cofre, 3=spawned
    uint8_t          fire_bombs_collected_total; ///< FireBombsCollected2 ($00AF): total recogidas en bomb room (hasta 23)
    MjkBombFuseSystem bomb_fuse;                 ///< Subsistema canónico de mecha y chispa OAM
    MjkBombPuffEffect bomb_puffs[MJK_MAX_BOMB_PUFFS]; ///< Animaciones de recogida de bombas (RES-036 / TASK-F7-009)

    // Puerta de escape y transición (Sprint 4: TASK-F6-021 / RES-017)
    uint8_t          door_tile_x;
    uint8_t          door_tile_y;
    uint8_t          door_condition_id;
    bool             door_condition_active;
    bool             door_is_open;
    uint8_t          entry_door_type;          ///< EntryDoorType ($033B, 0..15)
    uint8_t          door_anim_timer;          ///< byte_338 en RAM
    uint8_t          door_anim_phase;          ///< byte_339 en RAM (0, 1, 2)
    uint8_t          door_anim_dir;            ///< MaybeDoorReturnTwiceFlag ($0337): $81 open, $01 close
    bool             door_is_animating;        ///< DoorIsAnimatingMaybe ($0393)
    
    // Flags canónicos de evaluación de puertas (RES-017)
    uint8_t          door_check_flag_09;        ///< DoorCheckFlag_09 ($039A)
    uint8_t          door_check_flag_1e_deaths; ///< DoorCheckFlag_1E_DeathCount ($0395)
    uint8_t          door_check_flag_1f;        ///< DoorCheckFlag_1F ($0399)
    uint8_t          door_check_flag_19_1a;     ///< DoorCheckFlag_19_1A ($0397)

    // Cofres del tesoro y Bloques Secretos (TASK-F6-023 / RES-023)
    MjkChestEntity       chests[16];
    uint8_t              chest_count;
    MjkSecretBlockEntity secret_blocks[16];
    uint8_t              secret_block_count;
    uint8_t              secret_block_hits_left;    ///< byte_335 ($0335): contador regresivo de impactos
    uint8_t              secret_block_active_coord; ///< byte_336 ($0336): coordenada (Y, X) del bloque activo
    uint8_t              chest_target_coord;        ///< byte_F0 ($00F0): coordenada activa cofre/bloque
    uint8_t              chest_spawning_cmd;        ///< _MaybeBlockSpawningObj ($0315)
    uint8_t              collected_1up_this_round;  ///< Collected1UPThisRoundFlag ($0376)
    MjkRoomDataRAM       room_data_ram;             ///< Memoria canónica RoomDataRAM ($0600..$063F)

    // Power Coins y Transmutación (Sprint 4: TASK-F6-024 / RES-024)
    uint16_t         enemy_coin_timer;     ///< EnemyCoinTimer ($006A..$006B): 320 frames (0x0140)
    uint16_t         transmute_timer;      ///< Alias de compatibilidad hacia enemy_coin_timer
    uint8_t          transmute_streak;     ///< byte_54 ($0054): racha/combo de monedas (0..6)
    uint8_t          gold_coins_collected; ///< GoldCoinsCollected ($0366): monedas de oro acumuladas (hasta 40)

    // Feedback Visual Puntuación Flotante (Sprint 5: TASK-F6-026)
    MjkFloatingScore floating_scores[MJK_MAX_FLOATING_SCORES];

    // Cinemática de Cámara y Scroll de Escenarios (TASK-F6-024 / RES-019)
    CameraState      camera;
    int16_t          camera_x;
    int16_t          camera_y;
    bool             is_horizontal_room;

    // Codicia y Sala de Tortura (Sprint 5: TASK-F6-028)
    uint16_t         greed_jump_counter;

    // Paletas PPU activas (16 colores de fondo, 16 colores de sprites)
    uint32_t         palette_rgba[MJK_PALETTE_SIZE];

    // Control de entrada previo (para detección de flancos en paso 7)
    uint8_t          prev_input_held;
    uint64_t         total_frames_rendered;
} MjkGameState;
typedef MjkGameState mjk_game_t;

/**
 * @brief Inicializa una nueva instancia del juego con assets precargados.
 */
void MjkGame_Init(MjkGameState* game, MjkAssetPackage* assets);

/**
 * @brief Ejecuta exactamente 1 frame de simulación (reproduciendo el pipeline de 7 pasos de la NMI de NES).
 * 
 * [CONCEPTO EDUCATIVO]:
 * En la NES, el 100% de la lógica de juego se ejecutaba dentro de la interrupción NMI cada 16.67 ms:
 * 1. Prólogo y flags
 * 2. OAM DMA (sprites)
 * 3. Actualización de buffers VRAM
 * 4. Lectura de mandos P1 y P2
 * 5. MainLogicHandler (TitleScreenHandler o GameHandler según InGameFlag)
 * 6. Actualización de audio y APU
 * 7. Epílogo y actualización de flancos de entrada
 */
void MjkGame_SimulateFrame(MjkGameState* game, const MjkInputState* inputs, MjkFrameOutput* output);

/**
 * @brief Fuerza una transición a un nuevo estado de juego reiniciando sub_state y timer.
 */
void MjkGame_ChangeState(MjkGameState* game, GameStateId new_state);

/**
 * @brief Inicia una nueva partida reseteando puntuaciones, vidas y cargando la ronda 1.
 */
void MjkGame_StartNewGame(MjkGameState* game);

/**
 * @brief Spawnea un marcador de puntos flotante en coordenadas de pantalla.
 */
void MjkGame_SpawnFloatingScore(MjkGameState* game, int16_t x, int16_t y, uint16_t value);
void MjkGame_UpdateFloatingScores(MjkGameState* game);

/**
 * @brief Spawnea un efecto visual de recogida de bomba en 2 fases (Humo $25 -> Score) (RES-036).
 */
void MjkGame_SpawnBombPuff(MjkGameState* game, int16_t x, int16_t y, uint16_t points);

/**
 * @brief Actualiza frame a frame las animaciones de recogida de bombas (sub_A29C).
 */
void MjkGame_UpdateBombPuffs(MjkGameState* game);
void MjkGame_UpdateBombs(MjkGameState* game);

/**
 * @brief Actualiza la subpaleta de sprites de Jack según su nivel Mighty (0..3).
 */
void MjkGame_UpdateJackPalette(MjkGameState* game);

/**
 * @brief Abre o revela un cofre del tesoro de forma canónica (sub_A1A6 / MaybeOpenHiddenBlock).
 */
void MjkGame_OpenChest(MjkGameState* game, uint8_t chest_idx);

/**
 * @brief Procesa el impacto de Jack contra un cofre desde abajo (con la cabeza).
 */
void MjkGame_HitChestFromBelow(MjkGameState* game, uint8_t chest_idx);

/**
 * @brief Procesa el impacto contra un bloque secreto de pasillo ($6N).
 */
void MjkGame_HitSecretBlock(MjkGameState* game, uint8_t block_idx);

/**
 * @brief Procesa la recolección del ítem expuesto en un cofre abierto (MaybePickupItem).
 */
void MjkGame_CollectChestItem(MjkGameState* game, uint8_t chest_idx);

/**
 * @brief Actualiza colisiones e interacciones con cofres y bloques secretos en cada frame.
 */
void MjkGame_UpdateChestsAndSecrets(MjkGameState* game);

/* ============================================================================
 * CODICIA, SALA DE TORTURA Y SISTEMA GDV (TASK-F6-028 / RES-027)
 * ============================================================================ */

/**
 * @brief Suma puntos al GDV en formato BCD (prg.asm:1095 AddAToGDV).
 */
uint8_t MjkGame_AddAToGDV(uint8_t current_gdv, uint8_t points);

/**
 * @brief Calcula el GDV final de la partida según rondas, vidas, puntuación y final alcanzado.
 */
uint8_t MjkGame_CalculateFinalGDV(const MjkGameState* game, int8_t ending_type);

/**
 * @brief Evalúa las condiciones para desbloquear finales (0..3) según prg.asm:7377-7386.
 * @return 0 (Peor), 1 (Bueno), 2 (Genial), 3 (Mejor/Verdadero final).
 */
uint8_t MjkGame_EvaluateEnding(const MjkGameState* game);

/**
 * @brief Dispara la transición a la Sala de Tortura confiscando monedas y guardando checkpoint.
 */
void MjkGame_TriggerTortureRoom(MjkGameState* game);

/* ============================================================================
 * POWER COINS, TRANSMUTACIÓN, 1UP Y MIGHTY POWER (TASK-F6-025 / RES-024)
 * ============================================================================ */

extern const uint16_t k_transmuted_coin_scores[6];

/**
 * @brief Consume una moneda Mighty del inventario de Jack y sube de nivel Mighty.
 * FUENTE DE VERDAD: RES-024 (prg.asm:1448-1458, 1745-1768 UseMightyCoin)
 */
bool mjk_game_use_mighty_coin(mjk_game_t* game);

/**
 * @brief Procesa el tick frame a frame del temporizador de transmutación Power Coin (prg.asm:1368-1395).
 */
void mjk_game_tick_power_coin(mjk_game_t* game);

/**
 * @brief Comprueba si el efecto de transmutación de Power Coin está activo (enemy_coin_timer > 0).
 */
bool mjk_game_is_power_coin_active(const mjk_game_t* game);

/**
 * @brief Activa la transmutación de Power Coin por 320 frames (prg.asm:9019-9049).
 */
void mjk_game_activate_power_coin(mjk_game_t* game);

/**
 * @brief Recolecta una moneda transmutada aplicando la progresión canónica (200, 300, 500, 800, 1200, 2000).
 * FUENTE DE VERDAD: RES-024 (prg.asm:4032-4060 JT_98A4_multi)
 */
uint16_t mjk_game_collect_transmuted_coin(mjk_game_t* game);

/**
 * @brief Obtiene la puntuación correspondiente a una racha de monedas transmutadas (combo 1..6).
 */
uint16_t mjk_game_get_transmuted_coin_score(uint8_t streak);

/**
 * @brief Concede una vida extra (+1UP) y actualiza el contador de vidas e indicadores.
 * FUENTE DE VERDAD: RES-024 (prg.asm:5797-5830 MaybeAward1UP)
 */
void mjk_game_award_1up(mjk_game_t* game);

/**
 * @brief Procesa la recolección de Mighty Drink (+1000 pts, +10 BCD al temporizador, trigger tortura si >= 0xA0).
 * FUENTE DE VERDAD: RES-024 (prg.asm:4942-4953 JTPickup_MightyDrink)
 */
void mjk_game_collect_mighty_drink(mjk_game_t* game);

#endif // MJK_GAME_H
