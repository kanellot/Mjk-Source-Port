/**
 * @file test_input.c
 * @brief Unit tests de `MjkInputLatch` (Ticket R-02): iteraciones de 0 y 2 pasos.
 */

#include <stdio.h>
#include "mjk/input.h"
#include "mjk/types.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] %s\n", (msg));                             \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

int main(void) {
    MjkInputLatch latch;
    MjkInputStep step;
    MjkInputLatch_Init(&latch);

    /* 1. Iteracion con 0 pasos de simulacion: el flanco de subida NO se pierde */
    MjkInputLatch_Poll(&latch, MJK_INPUT_A);
    /* Siguiente poll del SO sin haber corrido ningun paso de simulacion todavia */
    MjkInputLatch_Poll(&latch, MJK_INPUT_A);
    MjkInputLatch_ConsumeStep(&latch, &step);
    CHECK(step.held == MJK_INPUT_A, "A mantenido tras 0-step poll");
    CHECK(step.pressed == MJK_INPUT_A, "Flanco de subida A retenido tras 0-step poll");
    CHECK(step.released == 0u, "Sin flanco de bajada");

    /* 2. Iteracion catch-up con 2 pasos consecutivos: el flanco se consume UNA sola vez */
    MjkInputLatch_Poll(&latch, (uint8_t)(MJK_INPUT_A | MJK_INPUT_START));
    MjkInputLatch_ConsumeStep(&latch, &step);
    CHECK(step.held == (MJK_INPUT_A | MJK_INPUT_START), "Paso 1: A y START mantenidos");
    CHECK(step.pressed == MJK_INPUT_START, "Paso 1: flanco START presente");

    MjkInputLatch_ConsumeStep(&latch, &step);
    CHECK(step.held == (MJK_INPUT_A | MJK_INPUT_START), "Paso 2: A y START siguen mantenidos");
    CHECK(step.pressed == 0u, "Paso 2: flanco START NO se duplica");
    CHECK(step.released == 0u, "Paso 2: sin flanco de bajada");

    /* 3. Pulsacion rapida (pulsar y soltar antes del siguiente paso): ambos flancos capturados */
    MjkInputLatch_Poll(&latch, 0u);
    MjkInputLatch_Poll(&latch, MJK_INPUT_B);
    MjkInputLatch_Poll(&latch, 0u);
    MjkInputLatch_ConsumeStep(&latch, &step);
    CHECK(step.held == 0u, "Tap rapido: held es 0 al final");
    CHECK((step.pressed & MJK_INPUT_B) != 0u, "Tap rapido: flanco de subida B retenido");
    CHECK((step.released & MJK_INPUT_B) != 0u, "Tap rapido: flanco de bajada B retenido");

    /* 4. Paso posterior limpio */
    MjkInputLatch_ConsumeStep(&latch, &step);
    CHECK(step.held == 0u && step.pressed == 0u && step.released == 0u, "Paso limpio tras consumir tap");

    if (g_failures > 0) {
        fprintf(stderr, "[ERROR] test_input fallo con %d errores.\n", g_failures);
        return 1;
    }
    printf("[OK] test_input superado.\n");
    return 0;
}
