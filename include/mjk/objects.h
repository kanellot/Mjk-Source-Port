/**
 * @file objects.h
 * @brief Bombas, cadena de mechas, cofres rojos/naranjas, bloques secretos y Mighty.
 */

#ifndef MJK_ENGINE_OBJECTS_H
#define MJK_ENGINE_OBJECTS_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk/input.h"
#include "mjk/types.h"

struct MjkGame;

/** @brief Estado del bonus de orden perfecto al recoger bombas en pasillo. */
typedef enum {
    BOMB_ORDER_NONE = 0,
    BOMB_ORDER_ACTIVE,      /**< Se empezo por la bomba marcada; sigue valido. */
    BOMB_ORDER_DISQUALIFIED,/**< Se recogio una bomba apagada; bonus perdido. */
    BOMB_ORDER_COMPLETED    /**< Se completo el orden (dispara puerta cond $0C). */
} MjkBombOrderStatus;

/** @brief Animacion de 2 fases al recoger una bomba (32f humo + 32f popup 100/200). */
typedef struct {
    bool    active;
    uint8_t phase;     /**< 0 = humo ($25), 1 = popup 100/200. */
    int     timer;
    bool    was_lit;
    uint8_t mult_tile;
    float   x;
    float   y;
} MjkBombPuff;

/** @brief Chispa animada sobre la bomba encendida en Bomb Room. */
typedef struct {
    bool    active;
    uint8_t grid_x;
    uint8_t grid_y;
    uint8_t anim_timer;
    uint8_t tile_id;
} MjkFuseSpark;

/** @brief Sistema de objetos dinamicos de la sala actual. */
typedef struct {
    MjkBombOrderStatus bomb_order;
    bool               first_bomb_picked;
    uint8_t            power_coin_acc;
    bool               power_coin_armed;

    bool               has_chest_target;
    uint8_t            chest_target_x;
    uint8_t            chest_target_y;
    uint8_t            chest_target_room_id;

    uint8_t            secret_block_x;
    uint8_t            secret_block_y;
    uint8_t            secret_hits_left;

    MjkBombPuff        puff;
    MjkFuseSpark       spark;
} MjkObjectSystem;

/** @brief Inicializa el sistema de objetos al entrar en una sala (preserva power_coin_acc). */
void MjkObjects_InitForRoom(MjkObjectSystem* obj, struct MjkGame* game);

/** @brief Consume una Mighty Coin (niveles 1..3; en nivel 3 devuelve true en out_transmute). */
bool MjkObjects_UseMightyCoin(struct MjkGame* game, bool* out_trigger_transmute);

/** @brief Actualiza bombas, mechas, cofres, bloques secretos, items y uso de Mighty. */
MjkActionResult MjkObjects_Update(MjkObjectSystem* obj,
                                  struct MjkGame* game,
                                  const MjkInputStep* input);

#endif /* MJK_ENGINE_OBJECTS_H */
