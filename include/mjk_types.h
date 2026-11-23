/**
 * @file mjk_types.h
 * @brief Tipos base C11, aritmética Q8.8/BCD y enumeraciones canónicas (REV_US).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Este header pertenece al núcleo puro (`src/core/`) y no depende de SDL ni del SO.
 * Modela de forma legible las representaciones que el procesador 6502 implementa a mano:
 *   1. Punto fijo Q8.8 (`PosHi:PosLo`) sin unidad de coma flotante (FPU).
 *   2. Aritmética decimal BCD (`PlayerScore`, `StageTimer`, `PlayerGDV`, `GreedyJumps`).
 *   3. `MjkActionResult` para reemplazar el retorno por manipulación de pila (`PLA; PLA`).
 */

#ifndef MJK_TYPES_H
#define MJK_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * 1. PUNTO FIJO Q8.8 (PosHi:PosLo y VelHi:VelLo en 6502)
 * ============================================================================
 * En Mighty Bomb Jack, las coordenadas y velocidades se guardan como 16 bits:
 *   - Byte alto (8 bits): parte entera (píxeles o píxeles/frame).
 *   - Byte bajo (8 bits): parte fraccionaria en 1/256 avos de píxel (0..255).
 * Usamos desplazamientos y sumas sobre uint16_t para evitar comportamiento indefinido
 * en C11 (UB por shift de negativos u overflow con signo).
 */

typedef int16_t  mjk_q8_8_t;   /**< Punto fijo Q8.8 con signo (velocidades, deltas). */
typedef uint16_t mjk_uq8_8_t;  /**< Punto fijo Q8.8 sin signo (coordenadas de sala/mundo). */

#define MJK_Q8_FROM_INT(whole)  MjkQ8_FromInt((int16_t)(whole))
#define MJK_Q8_FLOOR(val)       MjkQ8_FloorInt((val))
#define MJK_Q8_FRAC(val)        MjkQ8_Frac((val))
#define MJK_Q8_ADD(a, b)        MjkQ8_Add((a), (b))
#define MJK_Q8_SUB(a, b)        MjkQ8_Sub((a), (b))

static inline mjk_q8_8_t MjkQ8_FromInt(int16_t whole) {
    return (mjk_q8_8_t)((uint16_t)whole << 8);
}

static inline mjk_q8_8_t MjkQ8_FromParts(int8_t hi, uint8_t lo) {
    return (mjk_q8_8_t)(((uint16_t)(uint8_t)hi << 8) | (uint16_t)lo);
}

static inline int16_t MjkQ8_FloorInt(mjk_q8_8_t v) {
    uint16_t hi = (uint16_t)v >> 8;
    return (hi & 0x80u) ? (int16_t)((int)hi - 256) : (int16_t)hi;
}

static inline uint8_t MjkQ8_Frac(mjk_q8_8_t v) {
    return (uint8_t)((uint16_t)v & 0xFFu);
}

static inline mjk_q8_8_t MjkQ8_Add(mjk_q8_8_t a, mjk_q8_8_t b) {
    return (mjk_q8_8_t)((uint16_t)a + (uint16_t)b);
}

static inline mjk_q8_8_t MjkQ8_Sub(mjk_q8_8_t a, mjk_q8_8_t b) {
    return (mjk_q8_8_t)((uint16_t)a - (uint16_t)b);
}

static inline mjk_uq8_8_t MjkUQ8_FromInt(uint8_t whole) {
    return (mjk_uq8_8_t)((uint16_t)whole << 8);
}

static inline mjk_uq8_8_t MjkUQ8_FromParts(uint8_t hi, uint8_t lo) {
    return (mjk_uq8_8_t)(((uint16_t)hi << 8) | (uint16_t)lo);
}

static inline uint8_t MjkUQ8_Int(mjk_uq8_8_t v) {
    return (uint8_t)(v >> 8);
}

static inline uint8_t MjkUQ8_Frac(mjk_uq8_8_t v) {
    return (uint8_t)(v & 0xFFu);
}

static inline mjk_uq8_8_t MjkUQ8_AddSigned(mjk_uq8_8_t pos, mjk_q8_8_t delta) {
    return (mjk_uq8_8_t)((uint16_t)pos + (uint16_t)delta);
}

/* ============================================================================
 * 2. ARITMÉTICA DECIMAL BCD EMPAQUETADA (2 dígitos por byte: 0x00..0x99)
 * ============================================================================
 * El 6502 de la NES carece de modo decimal hardware (`CLD`/`SED` fue eliminado en 2A03),
 * por lo que Mighty Bomb Jack ajusta manualmente cada nibble (0..9) en RAM:
 * `PlayerScore` ($00A0..$00A3), `StageTimer` ($00A4), `PlayerGDV` y `GreedyJumpsRemaining`.
 */

static inline bool MjkBcd_IsValidByte(uint8_t b) {
    return ((b >> 4) <= 9u) && ((b & 0x0Fu) <= 9u);
}

static inline uint8_t MjkBcd_ToBinary(uint8_t b) {
    return (uint8_t)(((b >> 4) * 10u) + (b & 0x0Fu));
}

static inline uint8_t MjkBcd_FromBinary(uint8_t n) {
    uint8_t clamped = (n > 99u) ? 99u : n;
    return (uint8_t)(((clamped / 10u) << 4) | (clamped % 10u));
}

static inline uint8_t MjkBcd_AddByte(uint8_t a, uint8_t b, uint8_t carry_in, uint8_t* carry_out) {
    unsigned lo = (unsigned)(a & 0x0Fu) + (unsigned)(b & 0x0Fu) + (carry_in ? 1u : 0u);
    unsigned cy_lo = 0u;
    if (lo >= 10u) {
        lo -= 10u;
        cy_lo = 1u;
    }
    unsigned hi = (unsigned)(a >> 4) + (unsigned)(b >> 4) + cy_lo;
    unsigned cy_hi = 0u;
    if (hi >= 10u) {
        hi -= 10u;
        cy_hi = 1u;
    }
    if (carry_out) {
        *carry_out = (uint8_t)cy_hi;
    }
    return (uint8_t)(((hi & 0x0Fu) << 4) | (lo & 0x0Fu));
}

static inline uint8_t MjkBcd_SubByte(uint8_t a, uint8_t b, uint8_t borrow_in, uint8_t* borrow_out) {
    int lo = (int)(a & 0x0Fu) - (int)(b & 0x0Fu) - (borrow_in ? 1 : 0);
    int br_lo = 0;
    if (lo < 0) {
        lo += 10;
        br_lo = 1;
    }
    int hi = (int)(a >> 4) - (int)(b >> 4) - br_lo;
    int br_hi = 0;
    if (hi < 0) {
        hi += 10;
        br_hi = 1;
    }
    if (borrow_out) {
        *borrow_out = (uint8_t)br_hi;
    }
    return (uint8_t)((((unsigned)hi & 0x0Fu) << 4) | ((unsigned)lo & 0x0Fu));
}

/* ============================================================================
 * 3. ABSTRACCIONES LIMPIAS Y ENUMS CANÓNICOS (asm/src/prg.asm & defs.asm)
 * ============================================================================ */

/** @brief Resultado explícito de acciones en un frame (reemplaza el `PLA; PLA` 6502). */
typedef enum {
    MJK_ACTION_CONTINUE = 0,
    MJK_ACTION_STATE_CHANGED,
    MJK_ACTION_ROOM_CHANGED,
    MJK_ACTION_PLAYER_DIED,
    MJK_ACTION_TORTURE_TRIGGERED
} MjkActionResult;

/** @brief Selector principal `MainLogicHandler` (asm/src/prg.asm:292). */
typedef enum {
    MJK_MODE_TITLE  = 0,
    MJK_MODE_INGAME = 1
} MjkTopMode;

/** @brief Estados de juego `GameHandler` (asm/src/prg.asm:302). */
typedef enum {
    MJK_GS_INIT              = 0x0,
    MJK_GS_ROUND_INTRO       = 0x1,
    MJK_GS_LOAD_ROOM         = 0x2,
    MJK_GS_DRAW_SCREEN       = 0x3,
    MJK_GS_ENTER_ROOM        = 0x4,
    MJK_GS_MAIN              = 0x5,
    MJK_GS_TIME_OVER         = 0x6,
    MJK_GS_TORTURE_ROOM      = 0x7,
    MJK_GS_ROUND_CLEAR       = 0x8,
    MJK_GS_EXIT_ROOM         = 0x9,
    MJK_GS_GAME_OVER         = 0xA,
    MJK_GS_ANIMATE_DOOR_OPEN = 0xB,
    MJK_GS_ENDING            = 0xC
} MjkGameState;

/** @brief Subestados de `TitleScreenHandler` en REV_US (asm/src/prg.asm:7496-7659). */
typedef enum {
    MJK_TS_US_COPYRIGHT_INIT = 0,
    MJK_TS_US_COPYRIGHT_WAIT = 1,
    MJK_TS_TITLE_INIT        = 2,
    MJK_TS_TITLE_WAIT        = 3,
    MJK_TS_JACK_FALLING      = 4
} MjkTitleState;

/** @brief Flags de sala `enum RoomFlag` (asm/src/defs.asm:214-222). */
typedef enum {
    MJK_RF_1              = 0x01,
    MJK_RF_BOMB_ROOM      = 0x02,
    MJK_RF_4              = 0x04,
    MJK_RF_AT_SCROLL_EDGE = 0x08,
    MJK_RF_SINGLE_SCREEN  = 0x10,
    MJK_RF_SCROLL_STOP    = 0x20,
    MJK_RF_HORIZONTAL     = 0x40,
    MJK_RF_SECTION_START  = 0x80
} MjkRoomFlags;

/** @brief Arquetipos de enemigos y monedas/entidades `enum EnemyType` (asm/src/defs.asm:193-210). */
typedef enum {
    MJK_ENEMY_MUMMY       = 0x00,
    MJK_ENEMY_UNKNOWN     = 0x01,
    MJK_ENEMY_HANEZO      = 0x02,
    MJK_ENEMY_GEJISHOGUN  = 0x03,
    MJK_ENEMY_GAMEIDO     = 0x04,
    MJK_ENEMY_DOKURON     = 0x05,
    MJK_ENEMY_DESUFA      = 0x06,
    MJK_ENEMY_HORUS       = 0x07,
    MJK_ENEMY_POWER_COIN  = 0x08,
    MJK_ENEMY_EXTRA_COIN  = 0x09,
    MJK_ENEMY_SECRET_COIN = 0x0A,
    MJK_ENEMY_CAPTIVE     = 0x0B,
    MJK_ENEMY_BALLOON     = 0x0C,
    MJK_ENEMY_MUMMY1      = 0x0D,
    MJK_ENEMY_BONUS_COIN  = 0x0E,
    MJK_ENEMY_MUMMY2      = 0x0F,
    MJK_ENEMY_BROTHER     = 0x10
} MjkEnemyType;

/** @brief Tipos de ítem y cofres (asm/src/defs.asm:281-298). */
typedef enum {
    MJK_ITEM_100_BAG        = 0,
    MJK_ITEM_300_BAG        = 1,
    MJK_ITEM_1000_BAG       = 2,
    MJK_ITEM_GOLD_COIN      = 3,
    MJK_ITEM_MIGHTY_DRINK   = 4,
    MJK_ITEM_BOMB           = 5,
    MJK_ITEM_SPHINX         = 6,
    MJK_ITEM_TECMO_PLATE    = 7,
    MJK_ITEM_MIGHTY_COIN    = 8,
    MJK_ITEM_CRYSTAL_BALL_1 = 9,
    MJK_ITEM_CRYSTAL_BALL_2 = 10,
    MJK_ITEM_KING           = 11,
    MJK_ITEM_QUEEN          = 12,
    MJK_ITEM_PRINCESS       = 13,
    MJK_ITEM_BEELZEBUT_MASK = 14,
    MJK_ITEM_NOTHING        = 15,
    MJK_ITEM_RED_CHEST      = 16,
    MJK_ITEM_ORANGE_CHEST   = 17
} MjkItemType;

/** @brief IDs de música y SFX `enum Sound` (asm/src/defs.asm:85-120). */
typedef enum {
    MJK_SND_SILENCE               = 0x00,
    MJK_SND_JUMP                  = 0x01,
    MJK_SND_DEATH                 = 0x02,
    MJK_SND_BONUS_COIN_SPAWN      = 0x03,
    MJK_SND_COIN_COLLECTED        = 0x04,
    MJK_SND_POWER_COIN_SPAWN      = 0x05,
    MJK_SND_POWER_COIN_MUSIC      = 0x06,
    MJK_SND_BOMB_UNLIT            = 0x07,
    MJK_SND_BOMB_LIT              = 0x08,
    MJK_SND_YOU_ARE_GREEDY        = 0x09,
    MJK_SND_1UP_CAPTIVE           = 0x0A,
    MJK_SND_CRYSTAL_BALL          = 0x0B,
    MJK_SND_0C_NULL               = 0x0C,
    MJK_SND_BONUS_STATUE          = 0x0D,
    MJK_SND_SECRET_BLOCK_REVEALED = 0x0E,
    MJK_SND_DOOR                  = 0x0F,
    MJK_MUS_MAIN                  = 0x10,
    MJK_MUS_TREASURE_ROOM         = 0x11,
    MJK_MUS_SIDE_ROOM             = 0x12,
    MJK_MUS_LABYRINTH             = 0x13,
    MJK_SND_14_NULL               = 0x14,
    MJK_MUS_OUTSIDE_1             = 0x15,
    MJK_MUS_MAIN_WITH_INTRO       = 0x16,
    MJK_MUS_GAME_OVER             = 0x17,
    MJK_MUS_ROUND_CLEAR           = 0x18,
    MJK_SND_BOMB_CHEST_BONUS      = 0x19,
    MJK_SND_1A                    = 0x1A,
    MJK_SND_PAUSE                 = 0x1B,
    MJK_SND_UNPAUSE               = 0x1C,
    MJK_SND_USED_MIGHTY_COIN      = 0x1D,
    MJK_MUS_OUTSIDE_2             = 0x1E,
    MJK_MUS_TORTURE_ROOM          = 0x1F,
    MJK_SND_BOMB_BONUS            = 0x20,
    MJK_SND_TIME_PICKUP           = 0x21,
    MJK_MUS_ENDING                = 0x22,
    MJK_MUS_COLLECTED_FULL_FAMILY = 0x23
} MjkSoundId;

/** @brief IDs de cadenas de texto PPU `enum Strings` (asm/src/defs.asm:54-80). */
typedef enum {
    MJK_STR_PUSH_START_BUTTON  = 0x00,
    MJK_STR_GAME_OVER          = 0x01,
    MJK_STR_TIME_OVER          = 0x02,
    MJK_STR_YOU_ARE_GREEDY     = 0x03,
    MJK_STR_GO_TO_TORTURE_ROOM = 0x04,
    MJK_STR_ROUND_CLEAR        = 0x05,
    MJK_STR_TIME_BONUS         = 0x06,
    MJK_STR_YOUVE_GOTTEN       = 0x07,
    MJK_STR_FIRE_BOMBS         = 0x08,
    MJK_STR_SPECIAL_BONUS      = 0x09,
    MJK_STR_YOUR_GDV           = 0x0A,
    MJK_STR_C_TECMO            = 0x0B,
    MJK_STR_HIGH_GDV           = 0x0C,
    MJK_STR_ENDING_1A          = 0x0D,
    MJK_STR_ENDING_1B          = 0x0E,
    MJK_STR_ENDING_1C          = 0x0F,
    MJK_STR_ENDING_2A          = 0x10,
    MJK_STR_ENDING_2B          = 0x11,
    MJK_STR_ENDING_2C          = 0x12,
    MJK_STR_ENDING_3A          = 0x13,
    MJK_STR_ENDING_3B          = 0x14,
    MJK_STR_ENDING_3C          = 0x15,
    MJK_STR_ENDING_3D          = 0x16,
    MJK_STR_ENDING_4A          = 0x17,
    MJK_STR_ENDING_4B          = 0x18,
    MJK_STR_ENDING_4C          = 0x19,
    MJK_STR_ENDING_4D          = 0x1A,
    /* Cadenas adicionales exclusivas de REV_US (asm/src/data.asm:440-446) */
    MJK_STR_TM                 = 0x1B,
    MJK_STR_MIGHTY_BOMB_JACK_TM = 0x1C,
    MJK_STR_TM_AND_C_TECMO_LTD = 0x1D,
    MJK_STR_LICENSED_BY        = 0x1E,
    MJK_STR_NINTENDO_OF_AMERICA = 0x1F,
    MJK_STR_COUNT              = 0x20
} MjkStringId;

/** @brief Entradas de tabla de puntuación `enum Score` (asm/src/defs.asm:125-157). */
typedef enum {
    MJK_SCORE_10      = 0x00,
    MJK_SCORE_100     = 0x01,
    MJK_SCORE_200     = 0x02,
    MJK_SCORE_300     = 0x03,
    MJK_SCORE_500     = 0x04,
    MJK_SCORE_800     = 0x05,
    MJK_SCORE_1200    = 0x06,
    MJK_SCORE_2000    = 0x07,
    MJK_SCORE_1000    = 0x08,
    MJK_SCORE_10000   = 0x09,
    MJK_SCORE_100000  = 0x0A,
    MJK_SCORE_50000   = 0x0B,
    MJK_SCORE_20      = 0x0C,
    MJK_SCORE_30      = 0x0D,
    MJK_SCORE_40      = 0x0E,
    MJK_SCORE_50      = 0x0F,
    MJK_SCORE_400     = 0x10,
    MJK_SCORE_600     = 0x11,
    MJK_SCORE_800_DUP = 0x12,
    MJK_SCORE_900     = 0x13,
    MJK_SCORE_1500    = 0x14,
    MJK_SCORE_2500    = 0x15,
    MJK_SCORE_1600    = 0x16,
    MJK_SCORE_2400    = 0x17,
    MJK_SCORE_3200    = 0x18,
    MJK_SCORE_4000    = 0x19,
    MJK_SCORE_3600    = 0x1A,
    MJK_SCORE_4800    = 0x1B,
    MJK_SCORE_6000    = 0x1C,
    MJK_SCORE_8000    = 0x1D,
    MJK_SCORE_1E_X    = 0x1E,
    MJK_SCORE_1000000 = 0x1F
} MjkScoreId;

typedef enum {
    MJK_FACING_RIGHT = 0,
    MJK_FACING_LEFT  = 1
} MjkFacing;

typedef enum {
    MJK_MIGHTY_NONE   = 0,
    MJK_MIGHTY_BLUE   = 1,
    MJK_MIGHTY_ORANGE = 2,
    MJK_MIGHTY_GREEN  = 3
} MjkMightyLevel;

#endif /* MJK_TYPES_H */
