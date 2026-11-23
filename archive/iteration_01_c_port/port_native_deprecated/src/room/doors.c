/**
 * @file doors.c
 * @brief 32 US Door Conditions Evaluator for Mighty Bomb Jack (NES).
 * 
 * Directly conforms to:
 *   - ADR-004: Golden Standard US (REV_US)
 *   - RES-007: Tabla de 32 Condiciones de Puertas
 *   - prg.asm:6745-7050 (CheckAndMaybeOpenDoor)
 */

#include "mbj/room.h"
#include "mbj/game_state.h"

bool Door_EvaluateCondition(uint8_t condition_id, const void *game_ptr) {
    if (!game_ptr) return false;
    const GameState *game = (const GameState *)game_ptr;

    switch (condition_id) {
        case 0x02:
            /* Jumped while touching door */
            return (game->ram.just_jumped_flag != 0);

        case 0x07:
        case 0x0E: /* In US version, Door 0x0E requires Mighty Level 3 (ADR-004) */
        case 0x17:
        case 0x18:
        case 0x1B:
            /* Requires Mighty Level 3 (Orange Jack) */
            return (game->ram.player.mighty_level >= 3);

        case 0x0C:
            /* All fire bombs collected in chain order */
            return ((game->ram.active_lit_bomb_idx & 0x80) != 0);

        case 0x0A:
        case 0x19:
        case 0x1A:
            /* Obsolete JP conditions disabled in US version (ADR-004) */
            return false;

        case 0x1E:
            /* Round 5 -> 11 Warp */
            return (game->current_room == 0xFC);

        case 0x1F:
            /* Round 11 -> 6 Warp Back */
            return (game->current_room == 0x61);

        default:
            /* Default generic door condition: opens when round objectives are met */
            return true;
    }
}
