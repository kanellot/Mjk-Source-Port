/**
 * @file game_state.h
 * @brief Game state machine lifecycle and core simulation state
 *        for Mighty Bomb Jack (NES) Native Port.
 * 
 * Directly corresponds to 6502 disassembly:
 *   - Main dispatch: prg.asm:292 (MainLogicHandler -> TitleScreenHandler / GameHandler)
 *   - GameState table: prg.asm:302 (13 authentic game states: State 0 to C)
 *   - ADR-004: USA Revision rules
 */

#ifndef MBJ_GAME_STATE_H
#define MBJ_GAME_STATE_H

#include "mbj/types.h"
#include "mbj/ram_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * AUTHENTIC 6502 GAME STATES (prg.asm:302, JumpTable in GameHandler)
 * ============================================================================ */

typedef enum {
    STATE_0_INIT              = 0x00, /* Round/Game initialization */
    STATE_1_ROUND_INTRO       = 0x01, /* "Round X" / Jack lives intro */
    STATE_2_LOAD_ROOM         = 0x02, /* Room data bytecode decompression */
    STATE_3_DRAW_SCREEN       = 0x03, /* PPU nametable buffer rendering */
    STATE_4_ENTER_ROOM        = 0x04, /* Door open / Jack entry animation */
    STATE_5_MAIN              = 0x05, /* Active gameplay simulation loop */
    STATE_6_TIME_OVER         = 0x06, /* Timer expired / life lost */
    STATE_7_TORTURE_ROOM      = 0x07, /* Greedy penalty sequence (50 jumps US) */
    STATE_8_ROUND_CLEAR       = 0x08, /* Bomb room cleared tally sequence */
    STATE_9_EXIT_ROOM         = 0x09, /* Exit door close animation */
    STATE_A_GAME_OVER         = 0x0A, /* Game Over sequence & GDV display */
    STATE_B_DOOR_OPEN         = 0x0B, /* Secret door unsealing animation */
    STATE_C_ENDING            = 0x0C  /* Pyramid demolition & ending cinematics */
} GameStateID;

typedef enum {
    LOGIC_MODE_TITLE          = 0,    /* Title Screen / Copyright demo */
    LOGIC_MODE_INGAME         = 1     /* Active GameHandler execution */
} InGameFlag;

/* ============================================================================
 * PLAYER ENTITY STRUCTURE (Mapped from RAM $0300-$0310)
 * ============================================================================ */

typedef struct {
    subpixel_t x;             /* 8.8 fixed-point horizontal position */
    subpixel_t y;             /* 8.8 fixed-point vertical position */
    subpixel_t vx;            /* Horizontal velocity */
    subpixel_t vy;            /* Vertical velocity */
    uint8_t facing;           /* 0 = Left, 1 = Right */
    uint8_t flags;            /* Status flags (InAir, Flutter, Dead, Stunned) */
    uint8_t jump_phase;       /* Index into parabolic velocity table (0..127) */
    uint8_t jump_phase_sub;   /* Sub-step accumulator for jump rate table */
    uint8_t lives;            /* Remaining lives (default: 3) */
    uint8_t mighty_level;     /* Mighty Power Level (0..3) */
    uint8_t mighty_coins;     /* Hoarded Mighty Coins (0..10; >10 triggers greed) */
} PlayerState;

/* ============================================================================
 * MAIN GAME STATE CONTAINER
 * ============================================================================ */

typedef struct {
    InGameFlag in_game_flag;  /* 0 = Title/Demo, 1 = In Game */
    GameStateID state;        /* Active GameState (0..12) */
    uint8_t current_room;     /* Active Room ID (1..257) */
    uint8_t checkpoint_room;  /* Room to restart at upon losing a life */
    uint32_t frame_count;     /* Total 60 Hz simulation ticks */

    /* Timers & Counters */
    uint8_t stage_timer_bcd;  /* Countdown stage timer in BCD (starts at 60) */
    uint8_t timer_divider;    /* 256-frame clock divider */
    uint8_t state_timer;      /* General purpose state duration timer */

    /* Torture Room & Greed (ADR-004: 50 jumps for US version) */
    uint8_t torture_jumps_left; /* BCD jump countdown (starts at 0x50 in US) */

    /* Player State */
    PlayerState player;

    /* Complete NES 2048-Byte RAM Image ($0000-$07FF) */
    NES_RAM ram;

    /* PPU Screen Buffer (256x240 pixels in RGBA or palette indices) */
    uint32_t screen_buffer[MBJ_NES_WIDTH * MBJ_NES_HEIGHT];
} GameState;

/* ============================================================================
 * CORE LIFECYCLE FUNCTIONS
 * ============================================================================ */

void Game_Init(GameState *game);
void Game_Reset(GameState *game);
void Game_Update(GameState *game, const GameInput *input);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_GAME_STATE_H */
