/**
 * @file doors.h
 * @brief Sistema de puertas REV_US: 33 condiciones con nombre, Esfinge y cruce.
 */

#ifndef MJK_ENGINE_DOORS_H
#define MJK_ENGINE_DOORS_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk/types.h"
#include "mjk/world.h"

struct MjkGame;

#define MJK_DOOR_COND_COUNT 33u

/** @brief Condiciones activas de apertura de puertas en REV_US (0..32). */
typedef enum {
    DOOR_COND_JUMP_TOUCH      = 0x02,
    DOOR_COND_MIGHTY3_A       = 0x07,
    DOOR_COND_FLAG_09         = 0x09,
    DOOR_COND_BOMB_ORDER      = 0x0C,
    DOOR_COND_MIGHTY3_B       = 0x0E,
    DOOR_COND_MIGHTY3_C       = 0x17,
    DOOR_COND_MIGHTY3_D       = 0x18,
    DOOR_COND_ZERO_TIMER_A    = 0x19,
    DOOR_COND_MIGHTY3_E       = 0x1B,
    DOOR_COND_ZERO_TIMER_B    = 0x1D,
    DOOR_COND_TWO_FIRE_DEATHS = 0x1E,
    DOOR_COND_ROOM_61_SPECIAL = 0x1F
} MjkDoorCondition;

/** @brief Devuelve el indice de condicion (0..32) de la sala activa, o -1 si no tiene. */
int MjkDoors_GetRoomCondition(const struct MjkGame* game);

/** @brief Evalua si la condicion `cond_idx` (0..32) se cumple en el estado actual. */
bool MjkDoors_EvaluateCondition(struct MjkGame* game, uint8_t cond_idx);

/** @brief Abre una puerta especifica en `room`, actualiza sus metatiles y persistencia. */
void MjkDoors_OpenInRoom(struct MjkGame* game, MjkRoomInstance* room, uint8_t door_obj_idx);

/** @brief Evalua la condicion de la sala actual y abre su puerta si corresponde. */
bool MjkDoors_Update(struct MjkGame* game);

/** @brief Dispara un evento de puerta de Esfinge (comprueba si estaba pendiente). */
void MjkDoors_HandleSphinxEvent(struct MjkGame* game, uint8_t event_idx);

/** @brief Dispara un evento de puerta directo (Familia Real / Beelzebub). */
bool MjkDoors_TriggerEventDirect(struct MjkGame* game, uint8_t event_idx);

/** @brief Comprueba si Jack cruza una puerta abierta -> `ACTION_ROOM_CHANGED`. */
MjkActionResult MjkDoors_CheckPlayerEntry(struct MjkGame* game);

#endif /* MJK_ENGINE_DOORS_H */
