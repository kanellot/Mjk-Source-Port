/**
 * @file input.h
 * @brief Lectura de mando y acumulador de flancos (`InputLatch`) para paso fijo.
 *
 * En un bucle de simulacion a paso fijo (60.0988 Hz), el bucle externo del SO
 * puede ejecutar 0 pasos (si el monitor corre mas rapido) o 2+ pasos (catch-up).
 * `MjkInputLatch` acumula los flancos de subida/bajada al leer el hardware y los
 * entrega exactamente una vez al siguiente paso de simulacion, evitando perder
 * pulsaciones de salto/pausa o dispararlas dos veces seguidas.
 */

#ifndef MJK_ENGINE_INPUT_H
#define MJK_ENGINE_INPUT_H

#include <stdint.h>

/* Mascara de botones del mando */
#define MJK_INPUT_RIGHT  0x01u
#define MJK_INPUT_LEFT   0x02u
#define MJK_INPUT_DOWN   0x04u
#define MJK_INPUT_UP     0x08u
#define MJK_INPUT_START  0x10u
#define MJK_INPUT_SELECT 0x20u
#define MJK_INPUT_B      0x40u
#define MJK_INPUT_A      0x80u

/** @brief Instantanea de entrada consumida por un unico paso de simulacion. */
typedef struct {
    uint8_t held;     /**< Botones mantenidos pulsados en este paso. */
    uint8_t pressed;  /**< Botones recien pulsados (flanco de subida, una sola vez). */
    uint8_t released; /**< Botones recien soltados (flanco de bajada, una sola vez). */
} MjkInputStep;

/** @brief Acumulador de entrada entre el polling de plataforma y el paso fijo. */
typedef struct {
    uint8_t prev_raw;
    uint8_t current_held;
    uint8_t pending_pressed;
    uint8_t pending_released;
} MjkInputLatch;

/** @brief Inicializa el latch con todos los botones sueltos. */
void MjkInputLatch_Init(MjkInputLatch* latch);

/**
 * @brief Registra una lectura cruda de botones desde teclado/gamepad.
 * Puede llamarse varias veces antes de un paso de simulacion sin perder flancos.
 */
void MjkInputLatch_Poll(MjkInputLatch* latch, uint8_t raw_buttons);

/**
 * @brief Consume los flancos acumulados para un paso de simulacion.
 * Entrega `held`, `pressed` y `released` en `out_step`, y limpia los flancos
 * pendientes para que un segundo paso consecutivo no vuelva a dispararlos.
 */
void MjkInputLatch_ConsumeStep(MjkInputLatch* latch, MjkInputStep* out_step);

#endif /* MJK_ENGINE_INPUT_H */
