/**
 * @file types.h
 * @brief Tipos fundamentales, constantes y enumeraciones del motor Mighty Bomb Jack (C11).
 *
 * Este header define el vocabulario comun de todo el juego sin dependencias de
 * plataforma (cero SDL, cero llamadas al SO) ni tipos de bajo nivel del 6502
 * (sin BCD ni punto fijo Q8.8: usamos enteros estandar y float IEEE-754).
 */

#ifndef MJK_ENGINE_TYPES_H
#define MJK_ENGINE_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Dimensiones de pantalla, rejilla de salas y tasa de refresco NTSC */
#define MJK_SCREEN_WIDTH        256
#define MJK_SCREEN_HEIGHT       240
#define MJK_TILE_SIZE           8
#define MJK_METATILE_SIZE       16
#define MJK_ROOM_COLS           16
#define MJK_ROOM_ROWS           15
#define MJK_ROOM_COUNT          256
#define MJK_TILE_COUNT          512
#define MJK_METATILE_COUNT      153

#define MJK_NTSC_FPS            60.0988138974405
#define MJK_AUDIO_SAMPLE_RATE   44100u

/* Codigo de salida estandar de CTest para pruebas omitidas por ausencia de ROM */
#define MJK_TEST_SKIP_CODE      77

/** @brief Vector 2D en pixeles (o pixeles/frame). */
typedef struct {
    float x;
    float y;
} MjkVec2;

/** @brief Direccion horizontal a la que mira un personaje. */
typedef enum {
    FACING_LEFT  = 0,
    FACING_RIGHT = 1
} MjkFacing;

/** @brief Resultado de un paso de logica que puede interrumpir el frame actual. */
typedef enum {
    ACTION_CONTINUE = 0,
    ACTION_STATE_CHANGED,
    ACTION_ROOM_CHANGED,
    ACTION_PLAYER_DIED,
    ACTION_TORTURE_TRIGGERED
} MjkActionResult;

/** @brief Modo superior del juego: pantalla de titulo o partida activa. */
typedef enum {
    SCREEN_TITLE  = 0,
    SCREEN_INGAME = 1
} MjkScreenMode;

/** @brief Estados principales de la partida (GameHandler 0..C). */
typedef enum {
    GS_INIT              = 0,
    GS_ROUND_INTRO       = 1,
    GS_LOAD_ROOM         = 2,
    GS_DRAW_SCREEN       = 3,
    GS_ENTER_ROOM        = 4,
    GS_PLAYING           = 5,
    GS_TIME_OVER         = 6,
    GS_TORTURE_ROOM      = 7,
    GS_ROUND_CLEAR       = 8,
    GS_EXIT_ROOM         = 9,
    GS_GAME_OVER         = 10,
    GS_ANIMATE_DOOR_OPEN = 11,
    GS_ENDING            = 12
} MjkGameState;

/** @brief Subestados de la secuencia de inicio y titulo (REV_US). */
typedef enum {
    TITLE_COPYRIGHT_INIT = 0,
    TITLE_COPYRIGHT_WAIT = 1,
    TITLE_MAIN_INIT      = 2,
    TITLE_MAIN_WAIT      = 3,
    TITLE_JACK_FALLING   = 4
} MjkTitleState;

/** @brief Banderas de comportamiento de camara y tipo de sala. */
typedef enum {
    ROOM_FLAG_NONE            = 0x00,
    ROOM_FLAG_BOMB_ROOM       = 0x02,
    ROOM_FLAG_AT_SCROLL_EDGE  = 0x08,
    ROOM_FLAG_SINGLE_SCREEN   = 0x10,
    ROOM_FLAG_SCROLL_STOP     = 0x20,
    ROOM_FLAG_HORIZONTAL      = 0x40,
    ROOM_FLAG_SECTION_START   = 0x80
} MjkRoomFlags;

/** @brief Tipos de enemigos y entidades coleccionables en sala. */
typedef enum {
    ENEMY_MUMMY       = 0x00,
    ENEMY_UNKNOWN     = 0x01,
    ENEMY_HANEZO      = 0x02,
    ENEMY_GEJISHOGUN  = 0x03,
    ENEMY_GAMEIDO     = 0x04,
    ENEMY_DOKURON     = 0x05,
    ENEMY_DESUFA      = 0x06,
    ENEMY_HORUS       = 0x07,
    ENEMY_POWER_COIN  = 0x08,
    ENEMY_EXTRA_COIN  = 0x09,
    ENEMY_SECRET_COIN = 0x0A,
    ENEMY_CAPTIVE     = 0x0B,
    ENEMY_BALLOON     = 0x0C,
    ENEMY_MUMMY1      = 0x0D,
    ENEMY_BONUS_COIN  = 0x0E,
    ENEMY_MUMMY2      = 0x0F,
    ENEMY_BROTHER     = 0x10
} MjkEnemyKind;

/** @brief Tipos de items contenidos en cofres, bloques secretos y salas. */
typedef enum {
    ITEM_BAG_100        = 0,
    ITEM_BAG_300        = 1,
    ITEM_BAG_1000       = 2,
    ITEM_GOLD_COIN      = 3,
    ITEM_MIGHTY_DRINK   = 4,
    ITEM_BOMB           = 5,
    ITEM_SPHINX         = 6,
    ITEM_TECMO_PLATE    = 7,
    ITEM_MIGHTY_COIN    = 8,
    ITEM_CRYSTAL_BALL_1 = 9,
    ITEM_CRYSTAL_BALL_2 = 10,
    ITEM_KING           = 11,
    ITEM_QUEEN          = 12,
    ITEM_PRINCESS       = 13,
    ITEM_BEELZEBUT_MASK = 14,
    ITEM_NOTHING        = 15,
    ITEM_RED_CHEST      = 16,
    ITEM_ORANGE_CHEST   = 17
} MjkItemKind;

/** @brief Identificadores de musica (BGM) y efectos de sonido (SFX). */
typedef enum {
    SOUND_SILENCE               = 0x00,
    SOUND_JUMP                  = 0x01,
    SOUND_DEATH                 = 0x02,
    SOUND_BONUS_COIN_SPAWN      = 0x03,
    SOUND_COIN_COLLECTED        = 0x04,
    SOUND_POWER_COIN_SPAWN      = 0x05,
    SOUND_POWER_COIN_MUSIC      = 0x06,
    SOUND_BOMB_UNLIT            = 0x07,
    SOUND_BOMB_LIT              = 0x08,
    SOUND_YOU_ARE_GREEDY        = 0x09,
    SOUND_1UP_CAPTIVE           = 0x0A,
    SOUND_CRYSTAL_BALL          = 0x0B,
    SOUND_0C_NULL               = 0x0C,
    SOUND_BONUS_STATUE          = 0x0D,
    SOUND_SECRET_BLOCK_REVEALED = 0x0E,
    SOUND_DOOR                  = 0x0F,
    MUSIC_MAIN                  = 0x10,
    MUSIC_TREASURE_ROOM         = 0x11,
    MUSIC_SIDE_ROOM             = 0x12,
    MUSIC_LABYRINTH             = 0x13,
    SOUND_14_NULL               = 0x14,
    MUSIC_OUTSIDE_1             = 0x15,
    MUSIC_MAIN_WITH_INTRO       = 0x16,
    MUSIC_GAME_OVER             = 0x17,
    MUSIC_ROUND_CLEAR           = 0x18,
    SOUND_BOMB_CHEST_BONUS      = 0x19,
    SOUND_1A                    = 0x1A,
    SOUND_PAUSE                 = 0x1B,
    SOUND_UNPAUSE               = 0x1C,
    SOUND_USED_MIGHTY_COIN      = 0x1D,
    MUSIC_OUTSIDE_2             = 0x1E,
    MUSIC_TORTURE_ROOM          = 0x1F,
    SOUND_BOMB_BONUS            = 0x20,
    SOUND_TIME_PICKUP           = 0x21,
    MUSIC_ENDING                = 0x22,
    MUSIC_COLLECTED_FULL_FAMILY = 0x23,
    SOUND_COUNT                 = 0x24
} MjkSoundId;

/** @brief Identificadores de cadenas de texto de pantalla (REV_US). */
typedef enum {
    STR_PUSH_START_BUTTON  = 0x00,
    STR_GAME_OVER          = 0x01,
    STR_TIME_OVER          = 0x02,
    STR_YOU_ARE_GREEDY     = 0x03,
    STR_GO_TO_TORTURE_ROOM = 0x04,
    STR_ROUND_CLEAR        = 0x05,
    STR_TIME_BONUS         = 0x06,
    STR_YOUVE_GOTTEN       = 0x07,
    STR_FIRE_BOMBS         = 0x08,
    STR_SPECIAL_BONUS      = 0x09,
    STR_YOUR_GDV           = 0x0A,
    STR_C_TECMO            = 0x0B,
    STR_HIGH_GDV           = 0x0C,
    STR_ENDING_1A          = 0x0D,
    STR_ENDING_1B          = 0x0E,
    STR_ENDING_1C          = 0x0F,
    STR_ENDING_2A          = 0x10,
    STR_ENDING_2B          = 0x11,
    STR_ENDING_2C          = 0x12,
    STR_ENDING_3A          = 0x13,
    STR_ENDING_3B          = 0x14,
    STR_ENDING_3C          = 0x15,
    STR_ENDING_3D          = 0x16,
    STR_ENDING_4A          = 0x17,
    STR_ENDING_4B          = 0x18,
    STR_ENDING_4C          = 0x19,
    STR_ENDING_4D          = 0x1A,
    STR_TM                 = 0x1B,
    STR_MIGHTY_BOMB_JACK_TM = 0x1C,
    STR_TM_AND_C_TECMO_LTD = 0x1D,
    STR_LICENSED_BY        = 0x1E,
    STR_NINTENDO_OF_AMERICA = 0x1F,
    STR_COUNT              = 0x20
} MjkStringId;

/** @brief Entradas de la tabla de puntuacion del juego. */
typedef enum {
    SCORE_10      = 0x00,
    SCORE_100     = 0x01,
    SCORE_200     = 0x02,
    SCORE_300     = 0x03,
    SCORE_500     = 0x04,
    SCORE_800     = 0x05,
    SCORE_1200    = 0x06,
    SCORE_2000    = 0x07,
    SCORE_1000    = 0x08,
    SCORE_10000   = 0x09,
    SCORE_100000  = 0x0A,
    SCORE_20000   = 0x0B,
    SCORE_50000   = 0x0C,
    SCORE_30000   = 0x0D,
    SCORE_1000000 = 0x0E,
    SCORE_2000000 = 0x0F,
    SCORE_COUNT   = 0x10
} MjkScoreId;

#endif /* MJK_ENGINE_TYPES_H */
