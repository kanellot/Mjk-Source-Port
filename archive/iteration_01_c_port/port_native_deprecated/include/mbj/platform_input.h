/**
 * @file platform_input.h
 * @brief Input hardware abstraction for Windows XInput (Xbox Series / One)
 *        and unified Keyboard polling for Mighty Bomb Jack.
 * 
 * Conforms to:
 *   - ADR-005: Xbox Series Plug-and-Play & Keyboard Fallback
 */

#ifndef MBJ_PLATFORM_INPUT_H
#define MBJ_PLATFORM_INPUT_H

#include "mbj/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void Platform_Input_Init(void);
void Platform_Input_Poll(GameInput *in);

#ifdef __cplusplus
}
#endif

#endif /* MBJ_PLATFORM_INPUT_H */
