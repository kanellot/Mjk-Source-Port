/**
 * @file chest.c
 * @brief Chest prying, headbutt opening, item spawning, and entity renderer
 *        for Mighty Bomb Jack (NES).
 * 
 * Directly conforms to:
 *   - RES-007: Apertura de Cofres y Bloques Ocultos
 *   - prg.asm:5392-5458 (MaybeOpenHiddenBlock)
 */

#include "mbj/entities.h"
#include "mbj/game_state.h"

void Entities_OpenChest(void *game_ptr, RoomEntity *chest) {
    if (!game_ptr || !chest || chest->state >= 2) return;
    GameState *game = (GameState *)game_ptr;

    chest->state = 2; /* Open */

    /* Item award from chest */
    if (game->ram.player.mighty_coins < 10) {
        game->ram.player.mighty_coins++;
    } else {
        /* Greed penalty: hoarding 10 coins triggers exile to Torture Room */
        game->state = STATE_7_TORTURE_ROOM;
        game->ram.torture_jumps_bcd = 0x50; /* 50 jumps in US (ADR-004) */
    }
}

void Entities_Update(void *game_ptr, RoomEntity *entities, uint8_t count) {
    if (!game_ptr || !entities || count == 0) return;
    GameState *game = (GameState *)game_ptr;

    int jack_x = (int)game->ram.player.x_pos_hi;
    int jack_y = (int)game->ram.player.y_pos_hi;

    for (uint8_t i = 0; i < count; i++) {
        RoomEntity *ent = &entities[i];
        if (ent->state == 0) continue; /* Inactive */

        int ent_x = (int)ent->x_px;
        int ent_y = (int)ent->y_px;

        /* Standard 16x16 bounding box collision test */
        if (jack_x + 12 >= ent_x && jack_x <= ent_x + 12 &&
            jack_y + 12 >= ent_y && jack_y <= ent_y + 12) {

            if (ent->type == ENTITY_BOMB) {
                Entities_CollectBomb(game, ent, entities, count);
            } else if (ent->type == ENTITY_CHEST_RED) {
                Entities_OpenChest(game, ent);
            } else if (ent->type == ENTITY_CHEST_ORANGE) {
                if (game->ram.player.mighty_level >= 1) {
                    Entities_OpenChest(game, ent);
                }
            } else if (ent->type == ENTITY_MIGHTY_COIN) {
                if (game->ram.player.mighty_coins < 10) {
                    game->ram.player.mighty_coins++;
                }
                ent->state = 0; /* Collected */
            }
        }
    }
}

void Entities_Render(uint32_t *screen_buffer, const RoomEntity *entities, uint8_t count,
                     const CHRBank *bank, uint32_t frame_count) {
    if (!screen_buffer || !entities || count == 0 || !bank) return;

    const uint8_t item_palette[4] = { 0x0F, 0x16, 0x27, 0x30 }; /* Black, Orange, Tan, White */
    const uint8_t spark_palette[4] = { 0x0F, 0x27, 0x30, 0x30 }; /* Flashing spark */

    for (uint8_t i = 0; i < count; i++) {
        const RoomEntity *ent = &entities[i];
        if (ent->state == 0) continue; /* Collected / Inactive */

        int px = ent->x_px;
        int py = ent->y_px;

        if (ent->type == ENTITY_BOMB) {
            /* Bomb Metatile (Tile 0x27) */
            Renderer_BlitTile(screen_buffer, px, py, bank, 0x27, item_palette, false, false, true);
            Renderer_BlitTile(screen_buffer, px + 8, py, bank, 0x27, item_palette, true, false, true);
            Renderer_BlitTile(screen_buffer, px, py + 8, bank, 0x27, item_palette, false, true, true);
            Renderer_BlitTile(screen_buffer, px + 8, py + 8, bank, 0x27, item_palette, true, true, true);

            /* Lit spark flickering on top of bomb */
            if (ent->is_lit && (frame_count & 4)) {
                Renderer_BlitTile(screen_buffer, px + 4, py - 4, bank, 0x26, spark_palette, false, false, true);
            }
        } else if (ent->type == ENTITY_CHEST_RED || ent->type == ENTITY_CHEST_ORANGE) {
            /* Chest closed (0x28) vs open (0x2A) */
            uint16_t tile = (ent->state >= 2) ? 0x2A : 0x28;
            Renderer_BlitTile(screen_buffer, px, py, bank, tile, item_palette, false, false, true);
            Renderer_BlitTile(screen_buffer, px + 8, py, bank, tile, item_palette, true, false, true);
            Renderer_BlitTile(screen_buffer, px, py + 8, bank, tile, item_palette, false, true, true);
            Renderer_BlitTile(screen_buffer, px + 8, py + 8, bank, tile, item_palette, true, true, true);
        } else if (ent->type == ENTITY_MIGHTY_COIN) {
            /* Rotating coin sprite (Tile 0x21) */
            Renderer_BlitTile(screen_buffer, px + 4, py + 4, bank, 0x21, item_palette, false, false, true);
        }
    }
}
