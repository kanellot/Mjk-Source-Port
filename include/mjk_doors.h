/**
 * @file mjk_doors.h
 * @brief Puertas REV_US: 33 condiciones de apertura, apertura en RoomDataRAM y cruce (T-21).
 */

#ifndef MJK_DOORS_H
#define MJK_DOORS_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_types.h"

struct MjkGame;

#define MJK_DOOR_COND_COUNT       33u
#define MJK_DOOR_COND_R11_TO_R6   0x1Fu /* Sala $61 (asm/src/prg.asm:6696-6740) */

/**
 * @brief Evalúa la condición `cond_idx` (0..32) de `CheckAndMaybeOpenDoor`
 *        (asm/src/prg.asm:6745-7009). Las condiciones no-op en REV_US devuelven `false`.
 */
MJK_CORE_API bool MjkDoors_EvaluateCondition(struct MjkGame* game, uint8_t cond_idx);

/** @brief Índice de condición de la sala actual (header $Ex byte 1 >> 3, o $1F en sala $61). */
MJK_CORE_API int MjkDoors_RoomConditionIndex(const struct MjkGame* game);

/** @brief Marca `|= 0x40` en la puerta, la repinta y encola `MJK_SND_DOOR`. */
MJK_CORE_API void MjkDoors_OpenRoomDoor(struct MjkGame* game, uint8_t door_obj_idx);

/** @brief Evalúa la condición de la sala y abre sus puertas; `true` si abrió alguna. */
MJK_CORE_API bool MjkDoors_Update(struct MjkGame* game);

/** @brief Detecta el cruce de Jack por una puerta abierta -> `MJK_ACTION_ROOM_CHANGED`. */
MJK_CORE_API MjkActionResult MjkDoors_CheckPlayerEntry(struct MjkGame* game);

#endif /* MJK_DOORS_H */
