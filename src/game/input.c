/**
 * @file input.c
 * @brief Implementacion de `MjkInputLatch` (consumo unico de flancos por paso).
 */

#include "mjk/input.h"
#include <string.h>

void MjkInputLatch_Init(MjkInputLatch* latch) {
    if (!latch) {
        return;
    }
    memset(latch, 0, sizeof(*latch));
}

void MjkInputLatch_Poll(MjkInputLatch* latch, uint8_t raw_buttons) {
    if (!latch) {
        return;
    }
    uint8_t newly_pressed  = (uint8_t)(raw_buttons & (uint8_t)(~latch->prev_raw));
    uint8_t newly_released = (uint8_t)((uint8_t)(~raw_buttons) & latch->prev_raw);

    latch->pending_pressed  |= newly_pressed;
    latch->pending_released |= newly_released;
    latch->current_held      = raw_buttons;
    latch->prev_raw          = raw_buttons;
}

void MjkInputLatch_ConsumeStep(MjkInputLatch* latch, MjkInputStep* out_step) {
    if (!latch || !out_step) {
        return;
    }
    out_step->held     = latch->current_held;
    out_step->pressed  = latch->pending_pressed;
    out_step->released = latch->pending_released;

    latch->pending_pressed  = 0u;
    latch->pending_released = 0u;
}
