/**
 * @file mjk_types.h
 * @brief Tipos base, punto fijo Q8.8, estructuras semánticas y enumeraciones didácticas.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: Disassembly 6502 (asm/src/prg.asm y asm/src/data.asm)
 * REGLAS DE ARQUITECTURA: ADR-006 (Código limpio, educacional, cero variables crípticas)
 */

#ifndef MJK_TYPES_H
#define MJK_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MJK_ROOM_TILES_W         16    ///< Columnas de metatiles por sala (256 px / 16)
#define MJK_ROOM_TILES_H         15    ///< Filas de metatiles por sala (240 px / 16)
#define MJK_ROOM_METATILES_COUNT (MJK_ROOM_TILES_W * MJK_ROOM_TILES_H) // 240 metatiles

#define MJK_MAX_ENEMIES          8     ///< Máximo de 8 slots de entidades simultáneas en RAM ($0600-$07FF)
#define MJK_MAX_OBJECTS          32    ///< Máximo de objetos simultáneos por sala

/* ============================================================================
 * 1. ARITMÉTICA EN PUNTO FIJO Q8.8 (FIXED-POINT MATH)
 * ============================================================================
 * [CONCEPTO EDUCATIVO]:
 * La CPU Ricoh 2A03 (6502) de la NES no tiene unidad de coma flotante (FPU).
 * Las velocidades y coordenadas fraccionarias de Jack y los enemigos se calculaban
 * utilizando enteros de 16 bits donde el byte alto representa el píxel en pantalla
 * y el byte bajo representa la fracción subpíxel acumulada cada frame.
 * 
 * En el ensamblador original:
 * - Coordenada X entera: byte_48
 * - Coordenada X subpíxel: byte_4A
 * - Coordenada Y entera: byte_44
 * - Coordenada Y subpíxel: byte_46
 */

typedef int32_t fixed8_t;

#define FIXED_FROM_INT(i)        ((fixed8_t)((int32_t)(i) << 8))
#define FIXED_TO_INT(f)          ((int16_t)((f) >> 8))
#define FIXED_FRACTION(f)        ((uint8_t)((f) & 0xFF))
#define FIXED_ADD(a, b)          ((fixed8_t)((a) + (b)))
#define FIXED_SUB(a, b)          ((fixed8_t)((a) - (b)))

/* ============================================================================
 * 2. MÁQUINA DE ESTADOS PRINCIPAL (GAME STATES & TITLE SCREEN)
 * ============================================================================
 * [CONCEPTO EDUCATIVO]:
 * Controlada por la variable original GameState ($0044) y TitleScreenState ($0045)
 * despachadas por JumpTable en prg.asm:302 y prg.asm:7473.
 */

typedef enum {
    TITLE_STATE_0_COPYRIGHT_INIT = 0,  ///< Inicialización de pantalla de Copyright (REV_US TitleScreen_US_0)
    TITLE_STATE_1_COPYRIGHT_WAIT = 1,  ///< Espera de retención de Copyright (260 frames)
    TITLE_STATE_2_TITLE_INIT     = 2,  ///< Carga y decodificación de Pantalla de Título (TitleScreen_0)
    TITLE_STATE_3_TITLE_PLAYER   = 3,  ///< Colocación de Jack en título (TitleScreen_1)
    TITLE_STATE_4_WAIT_START     = 4,  ///< Espera de pulsación de botón START (TitleScreen_2)
    TITLE_STATE_5_JACK_FALLING   = 5   ///< Cinemática de Jack cayendo parabólicamente con alas (TitleScreen_3)
} TitleScreenStateId;

typedef enum {
    GAME_STATE_0_INIT              = 0x00,  ///< Inicialización de memoria y PPU
    GAME_STATE_1_ROUND_INTRO       = 0x01,  ///< Pantalla "ROUND X" con Jack saludando
    GAME_STATE_2_LOAD_ROOM         = 0x02,  ///< Carga y decodificación de metatiles de sala
    GAME_STATE_3_DRAW_SCREEN       = 0x03,  ///< Volcado de nametables y atributos a VRAM
    GAME_STATE_4_ENTER_ROOM        = 0x04,  ///< Animación de entrada de Jack por la puerta
    GAME_STATE_5_MAIN_LOOP         = 0x05,  ///< Bucle principal de juego activo a 60 Hz
    GAME_STATE_6_TIME_OVER         = 0x06,  ///< Fin del tiempo de sala (bloqueo y penalización)
    GAME_STATE_7_TORTURE_ROOM      = 0x07,  ///< Sala de tortura (50 saltos para salir)
    GAME_STATE_8_ROUND_CLEAR       = 0x08,  ///< Recuento de bonificaciones de fin de ronda
    GAME_STATE_9_EXIT_ROOM         = 0x09,  ///< Jack saliendo por la puerta abierta
    GAME_STATE_A_GAME_OVER         = 0x0A,  ///< Fin de partida (vidas agotadas)
    GAME_STATE_B_ANIMATE_DOOR_OPEN = 0x0B,  ///< Secuencia de apertura de puerta de escape
    GAME_STATE_C_ENDING            = 0x0C   ///< Escena cinemática de final y créditos
} GameStateId;

/* ============================================================================
 * 3. CINEMÁTICA Y ESTADOS DE JACK (PLAYER STATES)
 * ============================================================================
 * [CONCEPTO EDUCATIVO]:
 * Jack tiene una máquina de sub-estados para modelar su física única:
 * salto parabólico (con curva de 64 pasos), planeo/aleteo sostenido, caída libre,
 * agacharse e invencibilidad tras beber Mighty Drinks.
 */

typedef enum {
    JACK_STATE_ON_GROUND     = 0,  ///< Parado o caminando sobre una superficie sólida
    JACK_STATE_JUMPING       = 1,  ///< En ascenso siguiendo la tabla de 64 pasos (word_C0E3)
    JACK_STATE_GLIDING       = 2,  ///< Aleteo aéreo mediante pulsaciones repetidas de 'A'
    JACK_STATE_FALLING       = 3,  ///< En caída libre acelerada por gravedad
    JACK_STATE_CROUCHING     = 4,  ///< Agachado en el suelo
    JACK_STATE_ENTERING_DOOR = 5,  ///< Entrando en la puerta de salida
    JACK_STATE_DYING         = 6   ///< Animación de muerte tras colisionar con un enemigo
} JackActionState;

typedef enum {
    MIGHTY_POWER_NONE   = 0,  ///< Jack normal (traje rojo/azul)
    MIGHTY_POWER_BLUE   = 1,  ///< Nivel 1: Abre cofres rojos
    MIGHTY_POWER_ORANGE = 2,  ///< Nivel 2: Abre cofres naranjas
    MIGHTY_POWER_GREEN  = 3   ///< Nivel 3: Abre cofres verdes y puertas secretas
} MightyPowerLevel;

typedef struct {
    fixed8_t pos_x;              ///< Posición horizontal en punto fijo Q8.8
    fixed8_t pos_y;              ///< Posición vertical en punto fijo Q8.8
    int8_t   vel_x;              ///< Velocidad horizontal (-1, 0, +1)
    int8_t   vel_y;              ///< Velocidad vertical entera (-4..+3, PlayerYVelHi)
    int16_t  subpixel_vx;        ///< Velocidad horizontal en subpíxeles Q8.8 (+384, -384, 0)
    int16_t  subpixel_vy;        ///< Velocidad vertical en subpíxeles Q8.8 (-896..+896)
    
    JackActionState state;       ///< Estado cinemático activo
    MightyPowerLevel power_level;///< Nivel de poder Mighty Drink (0 a 3)
    uint8_t  mighty_level;       ///< PlayerMightyLevel ($03E0): nivel Mighty actual (0..3)
    uint8_t  mighty_presses_left;///< PlayerMightyLevelPressesLeft ($03E1): presupuesto de pulsaciones de salto (botón A)
    uint8_t  mighty_coins;       ///< PlayerMightyCoins ($03E2): monedas Mighty en inventario
    uint8_t  lives;              ///< PlayerLives ($03E4): vidas restantes de Jack
    uint8_t  a_press_counter;    ///< APressCounter ($00FA): contador global de pulsaciones de botón A
    uint16_t enemy_coin_timer;   ///< EnemyCoinTimer ($006A..$006B): temporizador de transmutación activa
    
    uint16_t jump_accumulator;   ///< Acumulador de salto de 16 bits ($03E7:$03E6 en Q8.8)
    uint8_t  jump_step_index;    ///< Paso actual [0..63] reflejado de la tabla de salto
    uint8_t  glide_flaps_left;   ///< Contador de aleteos permitidos en este salto
    uint16_t power_timer;        ///< Temporizador de duración de Mighty Power (frames)
    
    bool     facing_left;        ///< Orientación horizontal del sprite
    bool     on_ground;          ///< Flag de contacto con el suelo (probes 0 y 1)
    bool     ceiling_hit;        ///< Flag de colisión con techo (probes 6 y 7)
    bool     is_alive;           ///< Estado vital de Jack
    bool     flutter_active;     ///< Flag de aleteo/planeo activo (bit 3 de PlayerStruct)
    
    uint8_t  collision_flags;    ///< Registro maestro CollisionFlag ($03CF)
    
    uint8_t  room_id;            ///< Sala activa ($03BF CurrentRoomID)
    uint8_t  scroll_x;           ///< Coordenada de scroll X ($03C2 byte_3C2)
    uint8_t  scroll_y;           ///< Coordenada de scroll Y ($03C4 byte_3C4)
    uint8_t  room_flags;         ///< Flags de estado de sala ($03BE RoomStatusFlags)

    uint8_t  sprite_id;          ///< Sprite canónico activo ($03DC PlayerSprite en RAM)
    uint8_t  death_timer;        ///< Temporizador de secuencia de muerte ($0073 MaybeDeathTimer)
    uint8_t  queued_sfx;         ///< SFX encolado pendiente de emisión ($01 = salto, etc.)
} JackPlayer;

/* ============================================================================
 * 4. ARQUETIPOS DE ENEMIGOS (ENEMY ARCHETYPES)
 * ============================================================================
 * [CONCEPTO EDUCATIVO]:
 * Controlados por RunEnemyLogic (prg.asm:8465). Cada enemigo tiene un arquetipo
 * de movimiento y puede mutar periódicamente a un tipo diferente según:
 * NewType = (A % 6) + 2.
 */

typedef enum {
    ENEMY_TYPE_MUMMY         = 0,  ///< Momia: patrulla horizontal y saltos cortos
    ENEMY_TYPE_NULL          = 1,  ///< Slot vacío o inactivo
    ENEMY_TYPE_HANEZO        = 2,  ///< Pájaro Hanezo: vuelo sinusoidal ondulante
    ENEMY_TYPE_GEJI_SHOGUN   = 3,  ///< Geji-Shogun: oruga que trepa paredes
    ENEMY_TYPE_GAMEIDO       = 4,  ///< Tortuga Gameido: rebote diagonal en paredes
    ENEMY_TYPE_DOKURON       = 5,  ///< Calavera Dokuron: rastreo hacia la posición de Jack
    ENEMY_TYPE_DESUFA        = 6,  ///< Demonio Desufa: acechador veloz
    ENEMY_TYPE_HORUS         = 7,  ///< Halcón Horus: picado vertical cuando Jack pasa por debajo
    ENEMY_TYPE_POWER_COIN    = 8,  ///< Moneda de poder (transformación temporal)
    ENEMY_TYPE_EXTRA_COIN    = 9   ///< Moneda especial de bonificación
} EnemyType;

typedef struct {
    fixed8_t  pos_x;             ///< Posición horizontal Q8.8
    fixed8_t  pos_y;             ///< Posición vertical Q8.8
    int8_t    vel_x;             ///< Dirección/velocidad horizontal entera (-1, 0, +1)
    int8_t    vel_y;             ///< Dirección/velocidad vertical entera (-1, 0, +1)
    int16_t   subpixel_vx;       ///< Velocidad horizontal canónica Q8.8 (0x0080 Momia, word_C0D3 mutados)
    int16_t   subpixel_vy;       ///< Velocidad vertical canónica Q8.8
    EnemyType type;              ///< Arquetipo de comportamiento
    uint8_t   state_timer;       ///< Temporizador de fase o ciclo de animación
    uint8_t   anim_frame;        ///< Cuadro de animación activo
    uint8_t   status;            ///< Flags de estado (bit 3: 0x08 = spawn/mutación humo, bit 4: 0x10 = moneda)
    uint8_t   flash_timer;       ///< Temporizador de spawn/mutación (32 frames: 16 invisible + 16 humo)
    bool      active;            ///< Flag de slot activo en la sala
    bool      facing_left;       ///< Dirección de avance
} EnemyEntity;

/* ============================================================================
 * 5. OBJETOS DE SALA (ROOM OBJECTS / BOMBAS Y COFRES)
 * ============================================================================
 */

// Constantes canónicas de estados de bomba (TASK-F6-022B / RES-022)
#define MJK_BOMB_STATE_UNLIT           0x05  ///< Bomba apagada estándar (100 pts)
#define MJK_BOMB_STATE_STARTER_OR_END  0x85  ///< Bomba detonadora / bomba final de sección
#define MJK_BOMB_FLAG_FUSE_LIT         0x40  ///< Mecha activa / fuego encendido (200 pts)
#define MJK_BOMB_STATE_EMPTY           0x0F  ///< Bomba recogida / slot consumido ($0F)

// Banderas canónicas de MaybeBombThing ($00F8 / RES-022)
#define MJK_BOMB_THING_STARTED         0x01  ///< Bit 0: Cadena iniciada con bomba starter
#define MJK_BOMB_THING_BROKEN          0x02  ///< Bit 1: Cadena rota por recogida fuera de turno
#define MJK_BOMB_THING_PERFECT_BONUS   0x80  ///< Bit 7: Bonus perfecto 10.000 pts / puerta 0x0C

#define MJK_POWER_COIN_THRESHOLD       20    ///< Umbral acumulador de bombas para Power Coin

typedef enum {
    OBJ_TYPE_NONE        = 0x00,
    OBJ_TYPE_BOMB_NORMAL = 0x01,  ///< Bomba apagada (100 puntos)
    OBJ_TYPE_BOMB_FIRE   = 0x02,  ///< Bomba encendida / parpadeante (200 puntos)
    OBJ_TYPE_CHEST       = 0x03,  ///< Cofre cerrado (requiere salto o Mighty Power)
    OBJ_TYPE_DOOR        = 0x04,  ///< Puerta de salida de la sala
    OBJ_TYPE_SPHINX      = 0x05,  ///< Esfinge secreta (10.000 puntos)
    OBJ_TYPE_COIN        = 0x06   ///< Moneda suelta
} RoomObjectType;

typedef struct {
    uint8_t        tile_x;       ///< Posición en rejilla de sala (0 a 15)
    uint8_t        tile_y;       ///< Posición en rejilla de sala (0 a 14)
    RoomObjectType type;         ///< Tipo de objeto
    uint8_t        state;        ///< Estado (cerrado, abriéndose, recogido)
    bool           collected;    ///< Flag de objeto ya consumido
} RoomObject;

#define MJK_MAX_FLOATING_SCORES 8

typedef struct {
    int16_t  x;
    int16_t  y;
    uint16_t value;              ///< Valor a mostrar (100, 200, 500, 1000, etc.)
    uint8_t  timer;              ///< Temporizador de visualización (30 frames)
    bool     active;
} MjkFloatingScore;

typedef struct {
    uint8_t tile_x;              ///< Columna en rejilla (0..15)
    uint8_t tile_y;              ///< Fila en rejilla (0..14)
    bool    collected;           ///< Recogida por Jack
    uint8_t room_id;             ///< ID de sala a la que pertenece la bomba (1..256)
    uint8_t state;               ///< Estado canónico (0x05 unlit, 0x85 starter/end, 0x45 lit, 0xC5 final lit, 0x0F empty)
    uint8_t ram_offset;          ///< Offset relativo en RoomDataRAM (si aplica)
} MjkBombEntity;

typedef struct {
    uint8_t maybe_bomb_thing;           ///< MaybeBombThing ($00F8): bit 0=started, bit 1=broken, bit 7=perfect bonus
    uint8_t room_bomb_picked_flag;      ///< byte_F7 ($00F7): 0=primer pickup en sala, 1+=iniciado
    uint8_t power_coin_bomb_counter;    ///< MaybeTempCollectableFlag2 ($0356): acumulador hasta 20
    uint8_t power_coin_trigger_flag;    ///< MaybeTempCollectableFlag ($0357): 0=inactivo, 1=armado para cofre, 3=spawned
    uint8_t fire_bombs_collected;       ///< FireBombsCollected ($00AE): bombas con fuego recogidas en bomb room
    uint8_t fire_bombs_collected_total; ///< FireBombsCollected2 ($00AF): total recogidas en bomb room (hasta 23)
    
    // Simulación OAM Sprite 4 (Chispa de Mecha - prg.asm:5369-5388)
    uint8_t spark_sprite_y;             ///< OAM $0210 (Y-4)
    uint8_t spark_sprite_tile;          ///< OAM $0211 (0x0D <-> 0x0F)
    uint8_t spark_sprite_attr;          ///< OAM $0212 (0x12: subpaleta 2)
    uint8_t spark_sprite_x;             ///< OAM $0213 (X+8)
    uint8_t spark_anim_timer;           ///< byte_78D (periodo de 3 frames)
    bool    spark_visible;              ///< Flag de visibilidad activa de la chispa
} MjkBombFuseSystem;

typedef struct {
    uint8_t tile_x;              ///< Columna en rejilla (0..15)
    uint8_t tile_y;              ///< Fila en rejilla (0..14)
    uint8_t jumps_needed;        ///< Saltos requeridos sobre la tapa (1..2)
    uint8_t chest_type;          ///< 0 = Rojo ($10 / $28), 1 = Naranja ($11 / $4A), 2 = Secreto
    bool    is_open;             ///< Estado abierto
    bool    reward_spawned;      ///< Ítem ya expulsado
    uint8_t room_id;             ///< ID de sala a la que pertenece el cofre (1..256)
    bool    is_hidden;           ///< Bit 5 activo en RoomDataRAM ($20 -> $30/$31)
    uint8_t item_id;             ///< ID del ítem contenido (cuarteto inferior)
    uint8_t original_cmd;        ///< Byte original de comando ROM ($1N, $3N, $9N, $AN)
    uint8_t ram_slot_idx;        ///< Offset en raw[] de RoomDataRAM
    bool    collected;           ///< Ítem ya recogido por Jack ($0F)
} MjkChestEntity;

typedef struct {
    uint8_t tile_x;              ///< Columna en rejilla (0..15)
    uint8_t tile_y;              ///< Fila en rejilla (0..14)
    uint8_t hits_required;       ///< Golpes requeridos N del comando $6N (byte_335 inicial)
    uint8_t hits_left;           ///< Contador regresivo de impactos restantes
    bool    revealed;            ///< true si ya se reveló (metatile $2F)
    uint8_t room_id;             ///< ID de sala (1..256)
    uint8_t ram_slot_idx;        ///< Offset en raw[] de RoomDataRAM
    uint8_t original_cmd;        ///< Comando original ($6N)
} MjkSecretBlockEntity;

#endif // MJK_TYPES_H
