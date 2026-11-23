/**
 * @file mjk_objects.h
 * @brief Bombas 100/200, mechas, cofres, bloques secretos, ítems y Mighty Power (T-19, T-20).
 */

#ifndef MJK_OBJECTS_H
#define MJK_OBJECTS_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_platform.h"
#include "mjk_ppu.h"
#include "mjk_types.h"

struct MjkGame;

typedef struct {
    bool        active;
    uint8_t     phase;     /**< 0 = puff ($25) 32f, 1 = popup 100/200 32f. */
    uint8_t     timer;
    uint8_t     bomb_type; /**< 0 = 100, 1 = 200. */
    mjk_uq8_8_t x;
    mjk_uq8_8_t y;
} MjkBombPuff;

typedef struct {
    bool    active;
    uint8_t yx;
    uint8_t anim_timer;
    uint8_t tile_id;
} MjkFuseSpark;

typedef struct {
    uint8_t      maybe_bomb_thing;
    bool         first_bomb_picked;
    uint8_t      fire_bombs_total;
    uint8_t      power_coin_acc;
    uint8_t      power_coin_armed;
    uint8_t      chest_target_yx;
    uint8_t      chest_target_cmd;
    uint8_t      chest_target_ram_idx;
    uint8_t      chest_target_prev_state;
    uint8_t      secret_block_yx;
    uint8_t      secret_hits_left;
    MjkBombPuff  puff;
    MjkFuseSpark spark;
} MjkObjectSystem;

MJK_CORE_API void MjkObjects_SetMetatile(struct MjkGame* game, uint8_t yx, uint8_t metatile_id);

MJK_CORE_API void MjkObjects_InitForRoom(MjkObjectSystem* obj, struct MjkGame* game);

MJK_CORE_API MjkActionResult MjkObjects_Update(MjkObjectSystem* obj,
                                               struct MjkGame* game,
                                               const MjkInputState* input);

MJK_CORE_API bool MjkObjects_UseMightyCoin(struct MjkGame* game,
                                           bool* out_trigger_transmute);

MJK_CORE_API uint8_t MjkObjects_RenderOam(const MjkObjectSystem* obj,
                                          const MjkAssetPackage* pkg,
                                          MjkPpu* ppu,
                                          uint8_t start_slot);

#endif /* MJK_OBJECTS_H */
