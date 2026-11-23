/**
 * @file xinput_controller.c
 * @brief Input hardware implementation for Xbox Series / One controllers
 *        and keyboard fallback for Mighty Bomb Jack Native Port.
 * 
 * Conforms to:
 *   - ADR-005: Raylib Presentation, Fullscreen & Xbox Series / Keyboard Input
 */

#include "mbj/platform_input.h"
#include "raylib.h"

#define GAMEPAD_STICK_DEADZONE 0.25f

void Platform_Input_Init(void) {
    /* Raylib initializes gamepad and keyboard subsystems automatically */
}

void Platform_Input_Poll(GameInput *in) {
    if (!in) return;

    in->prev_buttons = in->buttons;
    in->buttons = 0;

    /* -------------------------------------------------------------------------
     * 1. KEYBOARD INPUT (Arrows / WASD, Z / Space, X / Shift, Enter, Backspace)
     * ------------------------------------------------------------------------- */
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))       in->buttons |= JP_BTN_LEFT;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))      in->buttons |= JP_BTN_RIGHT;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))         in->buttons |= JP_BTN_UP;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))       in->buttons |= JP_BTN_DOWN;
    if (IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE))      in->buttons |= JP_BTN_A;       /* Jump / Flutter */
    if (IsKeyDown(KEY_X) || IsKeyDown(KEY_LEFT_SHIFT)) in->buttons |= JP_BTN_B;       /* Mighty Power / Pry */
    if (IsKeyDown(KEY_ENTER))                          in->buttons |= JP_BTN_START;   /* Pause / Start */
    if (IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_BACKSPACE)) in->buttons |= JP_BTN_SELECT;

    /* -------------------------------------------------------------------------
     * 2. XBOX SERIES / ONE CONTROLLER (Gamepad 0..3 with hotplug detection)
     * ------------------------------------------------------------------------- */
    for (int pad = 0; pad < 4; pad++) {
        if (!IsGamepadAvailable(pad)) continue;

        /* D-Pad */
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  in->buttons |= JP_BTN_LEFT;
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) in->buttons |= JP_BTN_RIGHT;
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_UP))    in->buttons |= JP_BTN_UP;
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  in->buttons |= JP_BTN_DOWN;

        /* Left Analog Stick with circular deadzone */
        float axis_x = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_X);
        float axis_y = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_Y);

        if (axis_x < -GAMEPAD_STICK_DEADZONE) in->buttons |= JP_BTN_LEFT;
        if (axis_x >  GAMEPAD_STICK_DEADZONE) in->buttons |= JP_BTN_RIGHT;
        if (axis_y < -GAMEPAD_STICK_DEADZONE) in->buttons |= JP_BTN_UP;
        if (axis_y >  GAMEPAD_STICK_DEADZONE) in->buttons |= JP_BTN_DOWN;

        /* Action Buttons */
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))  in->buttons |= JP_BTN_A; /* A */
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) in->buttons |= JP_BTN_B; /* B */
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))  in->buttons |= JP_BTN_B; /* X */

        /* Start & Select */
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_MIDDLE_RIGHT))     in->buttons |= JP_BTN_START;  /* Menu / Start */
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_MIDDLE_LEFT))      in->buttons |= JP_BTN_SELECT; /* View / Select */

        break; /* Primary active gamepad polled */
    }
}
