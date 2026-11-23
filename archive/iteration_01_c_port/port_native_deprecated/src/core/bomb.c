/**
 * @file bomb.c
 * @brief Bomb collection logic, 23-bomb flashing sequence, and multipliers
 *        for Mighty Bomb Jack (NES).
 * 
 * Directly conforms to:
 *   - RES-007: Lógica de Bombas y Cadena de Mechas
 *   - prg.asm:4970-5164 (JTPickup_Bomb)
 */

#include "mbj/entities.h"
#include "mbj/game_state.h"
#include "mbj/room_data.h"
#include <string.h>

uint8_t Entities_UnpackFromRoom(uint8_t room_id, RoomEntity *out_entities, uint8_t max_entities) {
    if (!out_entities || max_entities == 0) return 0;
    memset(out_entities, 0, sizeof(RoomEntity) * max_entities);

    const uint8_t *data = g_room_data_table[room_id];
    if (!data) return 0;

    uint8_t count = 0;
    size_t cursor = 0;

    /* Skip 3-byte room header */
    if (data[0] >= 0xE0 && data[0] <= 0xEF) {
        cursor = 3;
    }

    bool first_bomb = true;

    while (data[cursor] != 0xFF && cursor < 256 && count < max_entities) {
        uint8_t cmd = data[cursor++];

        if (cmd >= 0x01 && cmd <= 0x0F) {
            /* Group of bombs */
            uint8_t bomb_count = cmd;
            for (uint8_t b = 0; b < bomb_count && count < max_entities; b++) {
                uint8_t yx = data[cursor++];
                out_entities[count].type = ENTITY_BOMB;
                out_entities[count].x_px = (uint8_t)((yx & 0x0F) * 16);
                out_entities[count].y_px = (uint8_t)(((yx >> 4) & 0x0F) * 16);
                out_entities[count].state = 1; /* Active */
                if (first_bomb) {
                    out_entities[count].is_lit = true; /* First bomb of chain is lit */
                    first_bomb = false;
                }
                count++;
            }
        } else if (cmd >= 0x10 && cmd <= 0x2F) {
            /* Red Chest */
            uint8_t yx = data[cursor++];
            out_entities[count].type = ENTITY_CHEST_RED;
            out_entities[count].x_px = (uint8_t)((yx & 0x0F) * 16);
            out_entities[count].y_px = (uint8_t)(((yx >> 4) & 0x0F) * 16);
            out_entities[count].state = 1; /* Closed */
            out_entities[count].sub_item = cmd & 0x0F;
            count++;
            if (cmd == 0x16 && cursor < 256) cursor++; /* Skip Sphinx byte */
        } else if (cmd >= 0x30 && cmd <= 0x4F) {
            /* Orange Chest */
            uint8_t yx = data[cursor++];
            out_entities[count].type = ENTITY_CHEST_ORANGE;
            out_entities[count].x_px = (uint8_t)((yx & 0x0F) * 16);
            out_entities[count].y_px = (uint8_t)(((yx >> 4) & 0x0F) * 16);
            out_entities[count].state = 1; /* Closed */
            out_entities[count].sub_item = cmd & 0x0F;
            count++;
            if (cmd == 0x36 && cursor < 256) cursor++;
        } else if (cmd >= 0x50 && cmd <= 0x6F) {
            /* Hidden Multi-Hit Block */
            uint8_t yx = data[cursor++];
            out_entities[count].type = ENTITY_HIDDEN_BLOCK;
            out_entities[count].x_px = (uint8_t)((yx & 0x0F) * 16);
            out_entities[count].y_px = (uint8_t)(((yx >> 4) & 0x0F) * 16);
            out_entities[count].state = 0; /* Hidden */
            out_entities[count].hit_counter = cmd & 0x0F;
            count++;
        } else if (cmd >= 0x70 && cmd <= 0x7F) {
            /* Exposed Item */
            uint8_t yx = data[cursor++];
            out_entities[count].type = ENTITY_MIGHTY_COIN;
            out_entities[count].x_px = (uint8_t)((yx & 0x0F) * 16);
            out_entities[count].y_px = (uint8_t)(((yx >> 4) & 0x0F) * 16);
            out_entities[count].state = 1;
            count++;
        } else if (cmd >= 0x80 && cmd <= 0x8F) {
            /* Door object */
            if (cursor < 256) cursor++;
        } else if (cmd >= 0x90 && cmd <= 0xAF) {
            /* Hidden chest */
            if (cursor < 256) cursor++;
            if (cmd == 0x96 && cursor < 256) cursor++;
        } else if (cmd >= 0xB0) {
            if (cursor < 256) cursor++;
        }
    }

    return count;
}

void Entities_CollectBomb(void *game_ptr, RoomEntity *bomb, RoomEntity *all_bombs, uint8_t total_bombs) {
    if (!game_ptr || !bomb || bomb->type != ENTITY_BOMB || bomb->state == 0) return;
    GameState *game = (GameState *)game_ptr;

    bool was_lit = bomb->is_lit;
    bomb->state = 0; /* Collected */
    bomb->is_lit = false;

    /* Authentically light next available bomb in sequence */
    if (was_lit && all_bombs) {
        for (uint8_t i = 0; i < total_bombs; i++) {
            if (all_bombs[i].type == ENTITY_BOMB && all_bombs[i].state != 0) {
                all_bombs[i].is_lit = true;
                break;
            }
        }
    }

    /* Track collected count */
    game->ram.room_bombs_collected++;
    if (was_lit) {
        game->ram.active_lit_bomb_idx++;
    }
}
