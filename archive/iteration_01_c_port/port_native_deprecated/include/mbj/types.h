/**
 * @file types.h
 * @brief Global primitive types, fixed-point macros, and controller definitions
 *        for Mighty Bomb Jack (NES) Native PC Port.
 * 
 * In accordance with:
 *   - ADR-004: Golden Standard US (REV_US)
 *   - ADR-005: Raylib Presentation, Fullscreen & Xbox Series / Keyboard Input
 *   - RES-001 / RES-004 / RES-005: 6502 Memory & Types
 */

#ifndef MBJ_TYPES_H
#define MBJ_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * NES HARDWARE DIMENSIONS & RATIOS
 * ============================================================================ */

#define MBJ_NES_WIDTH       256
#define MBJ_NES_HEIGHT      240
#define MBJ_NES_FPS         60

/* Default window dimensions (3x integer scale) */
#define MBJ_WINDOW_WIDTH    (MBJ_NES_WIDTH * 3)   /* 768 */
#define MBJ_WINDOW_HEIGHT   (MBJ_NES_HEIGHT * 3)  /* 720 */

/* ============================================================================
 * 8.8 FIXED-POINT SUBPIXEL MATH (RES-006)
 * ============================================================================ */

typedef int32_t subpixel_t;

#define SUBPIXEL_SHIFT      8
#define SUBPIXEL_ONE        (1 << SUBPIXEL_SHIFT) /* 256 */

#define SUBPIXEL_FROM_PIXEL(px)     ((subpixel_t)((int32_t)(px) << SUBPIXEL_SHIFT))
#define PIXEL_FROM_SUBPIXEL(sub)    ((int32_t)((sub) >> SUBPIXEL_SHIFT))
#define SUBPIXEL_FRACTION(sub)      ((uint8_t)((sub) & 0xFF))

/* ============================================================================
 * NES JOYPAD BUTTON MASKS (prg.asm:117-124, ReadJoypad)
 * ============================================================================ */

#define JP_BTN_RIGHT    (1 << 0) /* 0x01 */
#define JP_BTN_LEFT     (1 << 1) /* 0x02 */
#define JP_BTN_DOWN     (1 << 2) /* 0x04 */
#define JP_BTN_UP       (1 << 3) /* 0x08 */
#define JP_BTN_START    (1 << 4) /* 0x10 */
#define JP_BTN_SELECT   (1 << 5) /* 0x20 */
#define JP_BTN_B        (1 << 6) /* 0x40 - Mighty Power / Pry Chest */
#define JP_BTN_A        (1 << 7) /* 0x80 - Jump / Midair Flutter */

typedef struct {
    uint8_t buttons;      /* Current frame bitmask (JP_BTN_*) */
    uint8_t prev_buttons; /* Previous frame bitmask for edge detection */
} GameInput;

static inline bool GameInput_IsDown(const GameInput *in, uint8_t mask) {
    return in && (in->buttons & mask) != 0;
}

static inline bool GameInput_JustPressed(const GameInput *in, uint8_t mask) {
    return in && ((in->buttons & mask) != 0) && ((in->prev_buttons & mask) == 0);
}

static inline bool GameInput_JustReleased(const GameInput *in, uint8_t mask) {
    return in && ((in->buttons & mask) == 0) && ((in->prev_buttons & mask) != 0);
}

/* ============================================================================
 * CANONICAL REVISION IDENTIFIER (ADR-004)
 * ============================================================================ */

typedef enum {
    GAME_REV_USA = 0, /* Canonical baseline: 50 jumps torture, 32 doors */
    GAME_REV_JAPAN = 1
} GameRevision;

#define ACTIVE_GAME_REVISION GAME_REV_USA

/* ============================================================================
 * CONTROL FLOW ABSTRACTION (RES-011 Double Return replacement)
 * ============================================================================ */

typedef enum {
    FLOW_CONTINUE = 0,
    FLOW_ABORT_FRAME,
    FLOW_DOUBLE_RETURN
} FlowControl;

#ifdef __cplusplus
}
#endif

#endif /* MBJ_TYPES_H */
