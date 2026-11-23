/**
 * @file mjk_player.c
 * @brief Implementación de cinemática, salto de 64 pasos y colisiones 9-probe.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: jack_physics_contract.md (asm/src/prg.asm:1304 y data.asm:187-257)
 */

#include "mjk_player.h"
#include "mjk_room_data.h"
#include "mjk_room_decoder.h"
#include <string.h>

/* ============================================================================
 * TABLA CANÓNICA DE VELOCIDADES DE SALTO (word_C0E3)
 * asm/src/data.asm:187-251
 * 64 magnitudes de 16 bits sin signo (formato Q8.8 subpíxeles por frame)
 * ============================================================================ */
const uint16_t k_jump_velocity_table[64] = {
    0x0380, 0x037C, 0x0378, 0x0374, 0x0370, 0x0368, 0x0360, 0x0358,
    0x0350, 0x0348, 0x0340, 0x0330, 0x0320, 0x0310, 0x0300, 0x02F0,
    0x02E0, 0x02D0, 0x02C0, 0x02B0, 0x02A0, 0x0290, 0x0280, 0x0270,
    0x0260, 0x0250, 0x0240, 0x0230, 0x0220, 0x0210, 0x0200, 0x01F0,
    0x01E0, 0x01D0, 0x01C0, 0x01B0, 0x01A0, 0x0190, 0x0180, 0x0170,
    0x0160, 0x0150, 0x0140, 0x0130, 0x0120, 0x0110, 0x0100, 0x00F0,
    0x00E0, 0x00D0, 0x00C0, 0x00B0, 0x00A0, 0x0090, 0x0080, 0x0070,
    0x005E, 0x004C, 0x003A, 0x0028, 0x0010, 0x0008, 0x0002, 0x0001
};

/* ============================================================================
 * TABLA CANÓNICA DE TASA DE AVANCE DEL ACUMULADOR (word_C163)
 * asm/src/data.asm:252-256
 * Modulación según D-Pad vertical (Q8.8 unsigned):
 * [0] Neutro: 0x00F0 (240/256 pasos/frame)
 * [1] Abajo:  0x0160 (352/256 pasos/frame)
 * [2] Arriba: 0x00A0 (160/256 pasos/frame)
 * [3] Ambos:  0x00D0 (208/256 pasos/frame)
 * ============================================================================ */
const uint16_t k_jump_rate_table[4] = {
    0x00F0, // 0: Neutro
    0x0160, // 1: Abajo
    0x00A0, // 2: Arriba
    0x00D0  // 3: Ambos
};

/* ============================================================================
 * TABLA CANÓNICA DE SPRITES (SpritesTable - asm/src/data.asm:364-411)
 * 46 entradas de 5 bytes cada una:
 *   Byte 0: ControlByte (simetría / flips iniciales)
 *   Bytes 1-4: Tiles CHR { TL, TR, BL, BR }
 * ============================================================================ */
const uint8_t g_mjk_sprites_table[46][5] = {
    /* 0x00: Jack Idle / De pie frontal */  { 0x44, 0x10, 0x10, 0x12, 0x12 }, // data.asm:365
    /* 0x01: Salto vertical */              { 0x44, 0x11, 0x11, 0x13, 0x13 }, // data.asm:366
    /* 0x02: Planeo/Aleteo/Descenso vert */ { 0x44, 0x14, 0x14, 0x16, 0x16 }, // data.asm:367
    /* 0x03: Title/Intro salto */           { 0x44, 0x15, 0x15, 0x17, 0x17 }, // data.asm:368
    /* 0x04: Caminar 1 / Ascenso diag */    { 0x00, 0x1C, 0x1D, 0x1E, 0x1F }, // data.asm:369
    /* 0x05: Caminar 2 */                   { 0x00, 0x1C, 0x1D, 0x20, 0x21 }, // data.asm:370
    /* 0x06: Caída diag / Planeo lat */     { 0x00, 0x18, 0x19, 0x1A, 0x1B }, // data.asm:371
    /* 0x07 */                              { 0x00, 0x48, 0x49, 0x4A, 0x4B }, // data.asm:372
    /* 0x08 */                              { 0x00, 0x78, 0x79, 0x7A, 0x7B }, // data.asm:373
    /* 0x09 */                              { 0x00, 0x7C, 0x7D, 0x7E, 0x7F }, // data.asm:374
    /* 0x0A */                              { 0x00, 0xD0, 0xD1, 0xD2, 0xD3 }, // data.asm:375
    /* 0x0B */                              { 0x00, 0xD4, 0xD5, 0xD6, 0xD7 }, // data.asm:376
    /* 0x0C */                              { 0x00, 0xC0, 0xC1, 0xC2, 0xC3 }, // data.asm:377
    /* 0x0D */                              { 0x00, 0xC4, 0xC5, 0xC6, 0xC7 }, // data.asm:378
    /* 0x0E */                              { 0x00, 0x70, 0x71, 0x72, 0x73 }, // data.asm:379
    /* 0x0F */                              { 0x00, 0x74, 0x75, 0x76, 0x77 }, // data.asm:380
    /* 0x10 */                              { 0x00, 0x60, 0x61, 0x62, 0x63 }, // data.asm:381
    /* 0x11 */                              { 0x00, 0x64, 0x65, 0x66, 0x67 }, // data.asm:382
    /* 0x12 */                              { 0x00, 0x5C, 0x5D, 0x5E, 0x5F }, // data.asm:383
    /* 0x13 */                              { 0x00, 0x58, 0x59, 0x5A, 0x5B }, // data.asm:384
    /* 0x14 */                              { 0x00, 0x68, 0x69, 0x6A, 0x6B }, // data.asm:385
    /* 0x15 */                              { 0x00, 0x6C, 0x6D, 0x6E, 0x6F }, // data.asm:386
    /* 0x16 */                              { 0x00, 0xA8, 0xA9, 0xAA, 0xAB }, // data.asm:387
    /* 0x17 */                              { 0x00, 0xAC, 0xAD, 0xAE, 0xAF }, // data.asm:388
    /* 0x18 */                              { 0x00, 0xB8, 0xB9, 0xBA, 0xBB }, // data.asm:389
    /* 0x19 */                              { 0x00, 0x2A, 0x2B, 0x80, 0x81 }, // data.asm:390
    /* 0x1A */                              { 0x00, 0x2E, 0x2F, 0x84, 0x85 }, // data.asm:391
    /* 0x1B */                              { 0x00, 0x22, 0x23, 0x28, 0x29 }, // data.asm:392
    /* 0x1C */                              { 0x00, 0x26, 0x27, 0x2C, 0x2D }, // data.asm:393
    /* 0x1D */                              { 0x00, 0x86, 0x0F, 0x87, 0x0F }, // data.asm:394
    /* 0x1E */                              { 0x00, 0x24, 0x25, 0x26, 0x27 }, // data.asm:395
    /* 0x1F */                              { 0x33, 0xA1, 0x8B, 0x8B, 0xA1 }, // data.asm:396
    /* 0x20 */                              { 0x00, 0x40, 0x41, 0x42, 0x43 }, // data.asm:397
    /* 0x21 */                              { 0x44, 0x44, 0x44, 0x46, 0x46 }, // data.asm:398
    /* 0x22 */                              { 0x00, 0x82, 0x83, 0x0F, 0x0F }, // data.asm:399
    /* 0x23 */                              { 0x44, 0xA0, 0xA0, 0xA2, 0xA2 }, // data.asm:400
    /* 0x24 */                              { 0x00, 0xE0, 0xE1, 0xE2, 0xE3 }, // data.asm:401
    /* 0x25 */                              { 0xE4, 0x8C, 0x8C, 0x8C, 0x8C }, // data.asm:402
    /* 0x26 */                              { 0x00, 0x50, 0x51, 0x52, 0x53 }, // data.asm:403
    /* 0x27 */                              { 0x00, 0x54, 0x55, 0x56, 0x57 }, // data.asm:404
    /* 0x28: Muerte Giro Frame A */         { 0x44, 0x45, 0x45, 0x47, 0x47 }, // data.asm:405
    /* 0x29: Muerte Giro Frame B */         { 0x44, 0x4D, 0x4D, 0x4F, 0x4F }, // data.asm:406
    /* 0x2A: Derrotado / Humo / Aplastado */{ 0x40, 0x0F, 0x0F, 0x8A, 0x8A }, // data.asm:407
    /* 0x2B */                              { 0x00, 0xA4, 0xA5, 0xA6, 0xA7 }, // data.asm:408
    /* 0x2C */                              { 0x00, 0xB0, 0xB1, 0xB2, 0xB3 }, // data.asm:409
    /* 0x2D */                              { 0x00, 0xB4, 0xB5, 0xB6, 0xB7 }  // data.asm:410
};

/* ============================================================================
 * TABLA DE ATRIBUTOS Y DESPLAZAMIENTOS OAM (SpriteAttributeTable)
 * asm/src/data.asm:588-596
 * ============================================================================ */
static const uint8_t s_sprite_attribute_table[8] = {
    0x00, 0x27, 0x55, 0x8D, 0xAA, 0x72, 0xFF, 0xD8
};

void Jack_Init(JackPlayer* jack, int16_t x, int16_t y) {
    if (!jack) return;
    memset(jack, 0, sizeof(JackPlayer));
    jack->pos_x = FIXED_FROM_INT(x);
    jack->pos_y = FIXED_FROM_INT(y);
    jack->state = JACK_STATE_ON_GROUND;
    jack->is_alive = true;
    jack->on_ground = true;
    jack->collision_flags = CF_ON_GROUND;
    jack->power_level = MIGHTY_POWER_NONE;
    jack->mighty_level = 0;
    jack->mighty_presses_left = 0;
    jack->mighty_coins = 0;
    jack->lives = 3;
    jack->enemy_coin_timer = 0;
    jack->sprite_id = 0;
    jack->death_timer = 0;
    jack->jump_accumulator = 0;
    jack->subpixel_vx = 0;
    jack->subpixel_vy = 0;
    jack->flutter_active = false;
}

void Jack_UpdateHorizontalVelocity(JackPlayer* jack, const MjkInputState* inputs) {
    if (!jack) return;

    if (inputs && (inputs->held & MJK_BTN_RIGHT)) {
        jack->facing_left = false;
        // asm/src/prg.asm:1547-1562 (loc_8A10): si CF_RIGHT_WALL está activo, no aplica avance
        if (!(jack->collision_flags & CF_RIGHT_WALL)) {
            jack->vel_x = 1;
            jack->subpixel_vx = MJK_WALK_SPEED_SUBPIXELS; // +0x0180 (1.5 px/f)
        } else {
            jack->vel_x = 0;
            jack->subpixel_vx = 0;
        }
    } else if (inputs && (inputs->held & MJK_BTN_LEFT)) {
        jack->facing_left = true;
        // asm/src/prg.asm:1547-1562 (loc_8A10): si CF_LEFT_WALL está activo, no aplica avance
        if (!(jack->collision_flags & CF_LEFT_WALL)) {
            jack->vel_x = -1;
            jack->subpixel_vx = -MJK_WALK_SPEED_SUBPIXELS; // -0x0180 (-1.5 px/f)
        } else {
            jack->vel_x = 0;
            jack->subpixel_vx = 0;
        }
    } else {
        // Parada instantánea en seco (fricción infinita), tanto en suelo como en aire
        // asm/src/prg.asm:1533-1563 (loc_89EF)
        jack->vel_x = 0;
        jack->subpixel_vx = 0;
    }

    jack->pos_x += jack->subpixel_vx;
}

void Jack_UpdateVerticalPhysics(JackPlayer* jack, const MjkInputState* inputs) {
    if (!jack) return;

    // 1. Manejo de pulsación de botón A (Salto o Flutter/Aleteo continuo)
    // FUENTE DE VERDAD: asm/src/prg.asm:1397-1447 (_MightyLevelAPressCheck y _NoMightyLevelDown)
    if (inputs && (inputs->just_pressed & MJK_BTN_A)) {
        mjk_player_on_jump_press(jack);
        if (jack->on_ground) {
            // Salto inicial desde el suelo:
            // Bit 1 de PlayerStruct activo, Carry = 0 -> accum = 0x0000 (Paso 0)
            jack->on_ground = false;
            jack->state = JACK_STATE_JUMPING;
            jack->jump_accumulator = 0x0000;
            jack->flutter_active = true;
            jack->queued_sfx = 0x01; // SFX_JUMP ($01)
        } else {
            // Aleteo / Flutter en el aire:
            // Bit 1 ya activo, Carry = 1 -> ROR A; ROR A -> accum = 0x4000 (Paso 64, ápice parabólico)
            jack->state = JACK_STATE_GLIDING;
            jack->jump_accumulator = 0x4000;
            jack->flutter_active = true;
        }
    }

    // Si Jack continúa en el suelo y no saltó, no hay avance vertical
    if (jack->on_ground) {
        jack->vel_y = 0;
        jack->subpixel_vy = 0;
        jack->state = JACK_STATE_ON_GROUND;
        jack->jump_accumulator = 0;
        jack->jump_step_index = 0;
        jack->flutter_active = false;
        return;
    }

    // 2. Modulación por Joypad vertical (word_C163)
    // FUENTE DE VERDAD: asm/src/prg.asm:1658-1685 (DoSomethingWithObjYVel)
    uint8_t joy_vert = 0;
    if (inputs) {
        if (inputs->held & MJK_BTN_DOWN) joy_vert |= 1;
        if (inputs->held & MJK_BTN_UP)   joy_vert |= 2;
    }
    uint16_t advance = k_jump_rate_table[joy_vert];

    // Avance de 16 bits del acumulador
    uint32_t next_accum = (uint32_t)jack->jump_accumulator + advance;
    // Clamping de accum_hi a un máximo de 0x7F (127 decimal)
    if ((next_accum >> 8) > 0x7F) {
        next_accum = (0x7F << 8) | (next_accum & 0xFF);
    }
    jack->jump_accumulator = (uint16_t)next_accum;

    // 3. Cálculo de velocidad Y idéntico a sub_8AE8 (asm/src/prg.asm:1689-1709)
    uint8_t accum_hi = (uint8_t)(jack->jump_accumulator >> 8);
    bool is_descent = (accum_hi >= 64);
    uint8_t table_idx;

    if (is_descent) {
        // Descenso simétrico reflejado: (accum_hi ^ 0xFF) & 0x3F
        table_idx = (uint8_t)((accum_hi ^ 0xFF) & 0x3F);
    } else {
        // Ascenso directo
        table_idx = accum_hi;
    }

    jack->jump_step_index = table_idx;
    uint16_t magnitude = k_jump_velocity_table[table_idx];

    if (is_descent) {
        jack->subpixel_vy = (int16_t)magnitude;                  // Descenso positivo (+)
        jack->state = (jack->flutter_active && accum_hi == 0x40) ? JACK_STATE_GLIDING : JACK_STATE_FALLING;
    } else {
        jack->subpixel_vy = (int16_t)(-(int32_t)magnitude);      // Ascenso negativo (-)
        jack->state = JACK_STATE_JUMPING;
    }

    // PlayerYVelHi canónico
    jack->vel_y = (int8_t)(jack->subpixel_vy >> 8);

    // 4. Integración de posición vertical Q8.8 (ApplyPlayerYVelocity)
    jack->pos_y += jack->subpixel_vy;
}

bool mjk_player_is_tile_solid(const uint8_t room_metatiles[MJK_ROOM_METATILES_COUNT],
                              int px, int py, mjk_player_t* player, uint8_t probe_idx) {
    uint8_t target_room = (player && player->room_id) ? player->room_id : 0;
    int target_x = px;
    int target_y = py;

    // sub_9A03 (asm/src/prg.asm:4251-4314): Detección de cruce de frontera multi-sala
    if (target_room != 0 && target_room < MJK_TOTAL_ROOMS_COUNT) {
        const MjkAdjacentRoom* adj = &g_adjacent_rooms[target_room];

        // Horizontal: cruce de frontera de 256 píxeles (asm: loc_9A1A - sub_9A03)
        if (target_x >= 256) {
            // loc_9A1A: Si no es SingleScreen ($10) ni SectionStart ($80), cruza a vecina derecha
            if (!(player->room_flags & 0x90) && adj->right != 0) {
                target_room = adj->right;
                target_x -= 256;
            } else {
                // Paridad loc_9A39: Wrap modular dentro de la sala actual sin pared sintética
                target_x &= 0xFF;
            }
        } else if (target_x < 0) {
            // Cruce hacia la sala vecina izquierda
            if (!(player->room_flags & 0x10) && adj->left != 0) {
                target_room = adj->left;
                target_x += 256;
            } else {
                // Paridad loc_9A39: Wrap modular dentro de la sala actual sin pared sintética
                target_x &= 0xFF;
            }
        }

        // Vertical: cruce de frontera de 240 píxeles
        if (target_y >= 240) {
            if (adj->down != 0) {
                target_room = adj->down;
                target_y -= 240;
            } else {
                return true;
            }
        } else if (target_y < 0) {
            if (adj->up != 0) {
                target_room = adj->up;
                target_y += 240;
            } else {
                return true;
            }
        }
    } else {
        // Sala sin adyacencia definida o test plano: límites de nametable 256x240
        if (target_x < 0 || target_x >= 256 || target_y < 0 || target_y >= 240) {
            return true;
        }
    }

    if (target_x < 0 || target_x >= 256 || target_y < 0 || target_y >= 240) {
        return true;
    }

    int col = target_x >> 4;
    int row = target_y >> 4;
    if (col < 0) col = 0;
    if (col > 15) col = 15;
    if (row < 0) row = 0;
    if (row > 14) row = 14;

    uint8_t tile_id = 0;
    if (player && target_room == player->room_id && room_metatiles) {
        tile_id = room_metatiles[row * MJK_ROOM_TILES_W + col];
    } else if (target_room != 0) {
        // Carga metatiles de la sala adyacente (JSR LoadRoomStuff: loc_9A3C)
        static uint8_t s_cached_target_room = 0;
        static uint8_t s_cached_metatiles[MJK_ROOM_TILES_H][MJK_ROOM_TILES_W];
        if (s_cached_target_room != target_room) {
            bool is_h = false;
            MjkRoom_DecodeGeometry(target_room, s_cached_metatiles, &is_h);
            s_cached_target_room = target_room;
        }
        tile_id = s_cached_metatiles[row][col];
    } else if (room_metatiles) {
        tile_id = room_metatiles[row * MJK_ROOM_TILES_W + col];
    }

    // 1. Casos Especiales Pasantes (Vacío / Ítems coleccionables / Cofres dinámicos / Puertas abiertas)
    // - Ítems coleccionables: 0x27, 0x2F, 0x34, 0x47..0x50, 0x63, 0x8F, 0x90, 0x91
    // - Cofres dinámicos: 0x28, 0x4A
    // - Puertas abiertas: 0x81..0x98
    if (tile_id == 0x00 || tile_id == 0x27 || tile_id == 0x28 || tile_id == 0x2F ||
        tile_id == 0x34 || (tile_id >= 0x47 && tile_id <= 0x50) ||
        tile_id == 0x63 || tile_id == 0x8F || tile_id == 0x90 || tile_id == 0x91 ||
        (tile_id >= 0x81 && tile_id <= 0x98)) {
        return false;
    }

    // 2. Peligro Mortal (Fuego / Lava) - asm/src/prg.asm:4543-4555 (loc_9BBB)
    if (tile_id >= 0x5C && tile_id <= 0x62) {
        if (probe_idx == 0 && player) {
            player->is_alive = false;
            player->state = JACK_STATE_DYING;
        }
        return false;
    }

    // 3. Metatiles de Solidez Total (16x16 px) - asm/src/prg.asm:4560-4563 (loc_9BEB)
    if (tile_id < 0x38 || tile_id >= 0x4C) {
        return true;
    }

    // 4. Metatiles de Solidez Parcial (8x8 px) vía byte_A3EA - asm/src/prg.asm:4564-4576
    static const uint8_t k_quadrant_masks[10] = {
        0xCC, 0xFC, 0x48, 0x5A, 0x7B, 0xDE, 0x21, 0x35, 0xAC, 0x3C
    };

    uint8_t offset = (uint8_t)(tile_id - 0x38);
    uint8_t raw = k_quadrant_masks[offset >> 1];
    uint8_t mask = (offset & 1) ? (uint8_t)((raw << 4) & 0xF0) : (uint8_t)(raw & 0xF0);

    // Desglose de cuadrante: TL=(0,0), TR=(1,0), BL=(0,1), BR=(1,1)
    uint8_t quad = (uint8_t)(((target_y & 8) ? 2 : 0) + ((target_x & 8) ? 1 : 0));
    return (mask & (0x80 >> quad)) != 0;
}

// Desplazamientos canónicos de las 9 sondas (byte_A3D8 - asm/src/prg.asm:5863-5872)
static const int8_t k_canonical_probe_dx[9] = {  0, -4,  4, -4,  4, -8, -8,  8,  8 };
static const int8_t k_canonical_probe_dy[9] = {  0, -8, -8,  8,  8, -4,  4, -4,  4 };

void mjk_player_update_collision(mjk_player_t *player, const uint8_t room_metatiles[MJK_ROOM_METATILES_COUNT]) {
    if (!player) return;

    int jx = FIXED_TO_INT(player->pos_x);
    int jy = FIXED_TO_INT(player->pos_y);

    int scroll_x = player->scroll_x;
    int scroll_y = player->scroll_y;

    // Centro geométrico de Jack (ancla espacial para las 9 sondas)
    // sub_9A03: ProbeWorldX = ActorScreenX + ScrollX + ProbeOffsetX
    int cx = jx + 8 + scroll_x;
    int cy = jy + 8 + scroll_y;

    // Sonda 0: Centro / Ancla (0, 0) - Chequeo de fuego / lava mortal
    int p0_x = cx + k_canonical_probe_dx[0];
    int p0_y = cy + k_canonical_probe_dy[0];
    mjk_player_is_tile_solid(room_metatiles, p0_x, p0_y, player, 0);

    // Acumulador de bits de sondas canónico (byte_30F)
    // Sondas verticales según velocidad vertical (sub_9965:4171-4178)
    bool ceiling_hit = false;
    bool floor_hit = false;

    if (player->vel_y < 0 || player->subpixel_vy < 0) {
        // Ascenso: Sondas 1 y 2 (Cabeza / Techo: -4, -8 y +4, -8)
        for (int p = 1; p <= 2; p++) {
            int px = cx + k_canonical_probe_dx[p];
            int py = cy + k_canonical_probe_dy[p];
            if (mjk_player_is_tile_solid(room_metatiles, px, py, player, (uint8_t)p)) {
                ceiling_hit = true;
            }
        }
    } else {
        // Descenso o reposo: Sondas 3 y 4 (Pies / Suelo: -4, +8 y +4, +8)
        for (int p = 3; p <= 4; p++) {
            int px = cx + k_canonical_probe_dx[p];
            int py = cy + k_canonical_probe_dy[p];
            if (mjk_player_is_tile_solid(room_metatiles, px, py, player, (uint8_t)p)) {
                floor_hit = true;
            }
        }
    }

    // Sondas laterales siempre activas (sub_9965)
    // Sondas 5 y 6 (Muro izquierdo: -8, -4 y -8, +4)
    bool left_hit = false;
    for (int p = 5; p <= 6; p++) {
        int px = cx + k_canonical_probe_dx[p];
        int py = cy + k_canonical_probe_dy[p];
        if (mjk_player_is_tile_solid(room_metatiles, px, py, player, (uint8_t)p)) {
            left_hit = true;
        }
    }

    // Sondas 7 y 8 (Muro derecho: +8, -4 y +8, +4)
    bool right_hit = false;
    for (int p = 7; p <= 8; p++) {
        int px = cx + k_canonical_probe_dx[p];
        int py = cy + k_canonical_probe_dy[p];
        if (mjk_player_is_tile_solid(room_metatiles, px, py, player, (uint8_t)p)) {
            right_hit = true;
        }
    }

    // Compresión por pares en CollisionFlag (loc_99BB)
    uint8_t cf = (uint8_t)(player->collision_flags & 0xE0);
    if (right_hit)   cf |= CF_RIGHT_WALL;
    if (left_hit)    cf |= CF_LEFT_WALL;
    if (floor_hit)   cf |= CF_FLOOR;
    if (ceiling_hit) cf |= CF_CEILING;

    // 1. Choque con techo (sub_A1B9:5530-5555)
    if (cf & CF_CEILING) {
        if (player->vel_y < 0 || player->subpixel_vy < 0) {
            jy |= 7; // Expulsión de techo: Y = Y | 7
            player->pos_y = FIXED_FROM_INT(jy);
            player->vel_y = 0;
            player->subpixel_vy = 0;
            player->jump_accumulator = 0x4000; // Ápice/descenso inmediato (byte_3E7 = $40)
            player->ceiling_hit = true;
            player->state = JACK_STATE_FALLING;
        }
    }

    // 2. Aterrizaje sobre suelo o plataforma (sub_A1EB:5559-5609)
    if (cf & CF_FLOOR) {
        if (player->vel_y >= 0 && player->subpixel_vy >= 0) {
            jy &= ~7; // Alineación exacta a múltiplo de 8 píxeles (AND #$F8)
            player->pos_y = FIXED_FROM_INT(jy);
            player->vel_y = 0;
            player->subpixel_vy = 0;
            player->jump_accumulator = 0;
            player->jump_step_index = 0;
            player->flutter_active = false;
            player->on_ground = true;
            player->ceiling_hit = false;
            player->state = JACK_STATE_ON_GROUND;
            cf |= CF_ON_GROUND;
        }
    } else {
        // En el aire: apaga CF_ON_GROUND
        cf &= ~CF_ON_GROUND;
        player->on_ground = false;
        if (player->state == JACK_STATE_ON_GROUND) {
            // Caminar fuera de una cornisa/plataforma: fuerza inicio de caída libre
            player->state = JACK_STATE_FALLING;
            player->jump_accumulator = 0x4000;
        }
    }

    // 3. Resolución de muros laterales (loc_8A06 / loc_8A10)
    if (cf & CF_RIGHT_WALL) {
        if (player->subpixel_vx > 0 || player->vel_x > 0) {
            player->subpixel_vx = 0;
            player->vel_x = 0;
            if (scroll_x == 0 || (player->room_flags & 0x08)) {
                int rcol = (jx + 16) >> 4;
                player->pos_x = FIXED_FROM_INT(rcol * 16 - 16);
            }
        }
    }
    if (cf & CF_LEFT_WALL) {
        if (player->subpixel_vx < 0 || player->vel_x < 0) {
            player->subpixel_vx = 0;
            player->vel_x = 0;
            if (scroll_x == 0 || (player->room_flags & 0x08)) {
                int lcol = (jx) >> 4;
                player->pos_x = FIXED_FROM_INT((lcol + 1) * 16);
            }
        }
    }

    player->collision_flags = cf;
}

void Jack_ResolveCollisions(JackPlayer* jack, const uint8_t* room_metatiles) {
    mjk_player_update_collision(jack, room_metatiles);
}

void Jack_UpdateMultiRoom(JackPlayer* jack, const MjkInputState* inputs, const uint8_t* room_metatiles,
                          uint8_t room_id, uint8_t scroll_x, uint8_t scroll_y, uint8_t room_flags) {
    if (!jack) return;
    jack->room_id = room_id;
    jack->scroll_x = scroll_x;
    jack->scroll_y = scroll_y;
    jack->room_flags = room_flags;
    Jack_Update(jack, inputs, room_metatiles);
}

void Jack_Update(JackPlayer* jack, const MjkInputState* inputs, const uint8_t* room_metatiles) {
    if (!jack) return;
    if (!jack->is_alive) {
        jack->death_timer++;
        return;
    }

    // 1. Integración horizontal con velocidad subpíxel Q8.8
    Jack_UpdateHorizontalVelocity(jack, inputs);

    // 2. Integración vertical y salto de 64 pasos / aleteo continuo
    Jack_UpdateVerticalPhysics(jack, inputs);

    // 3. Resolución espacial con 9 sondas contra la matriz de metatiles
    Jack_ResolveCollisions(jack, room_metatiles);
}

uint8_t Jack_DetermineSprite(const JackPlayer* jack, uint64_t frame_counter) {
    if (!jack) return 0;

    // 1. Si no está vivo (!is_alive):
    // asm/src/prg.asm:1316-1363:
    // Alterna 0x28 y 0x29 cada 6 frames durante el giro letal.
    // Al alcanzar 72 frames (completar secuencia de caída/impacto), pasa a 0x2A (humo/aplastado).
    if (!jack->is_alive) {
        if (jack->death_timer >= 72) {
            return 0x2A;
        }
        uint64_t step = jack->death_timer ? (uint64_t)jack->death_timer : frame_counter;
        return ((step / 6) % 2 == 0) ? 0x28 : 0x29;
    }

    // 2. En el suelo:
    if (jack->on_ground || jack->state == JACK_STATE_ON_GROUND) {
        if (jack->vel_x == 0) {
            // Quieto: Sprite 0 (Idle frontal simétrico)
            // asm/src/data.asm:365 y asm/src/prg.asm:5606-5607
            return 0;
        } else {
            // Caminando: alterna Sprites 4 y 5 cada 8 frames
            // asm/src/data.asm:369-370 y asm/src/prg.asm:1625-1633
            return ((frame_counter / 8) % 2 == 0) ? 4 : 5;
        }
    }

    // 3. En el aire (asm/src/prg.asm:1478-1486 y 1605-1636):
    bool is_ascending;
    if (jack->jump_accumulator > 0) {
        is_ascending = ((jack->jump_accumulator >> 8) < 0x40);
    } else {
        is_ascending = (jack->subpixel_vy < 0 || jack->vel_y < 0);
    }

    if (jack->vel_x == 0) {
        // Ascenso vertical: Sprite 1 / Descenso o Flutter vertical: Sprite 2
        return is_ascending ? 1 : 2;
    } else {
        // Ascenso diagonal: Sprite 4 / Descenso o Caída diagonal: Sprite 6
        return is_ascending ? 4 : 6;
    }
}

void Jack_AssembleMetasprite(uint8_t sprite_id, int16_t anchor_x, int16_t anchor_y,
                             bool facing_left, MjkOamSprite out_sprites[4]) {
    if (!out_sprites) return;
    if (sprite_id >= 46) {
        sprite_id = 0;
    }
    const uint8_t* sprite_data = g_mjk_sprites_table[sprite_id];

    // En DrawPlayer (asm/src/prg.asm:1640-1645):
    // LDA byte_3DD ($00 = Izq, $04 = Der)
    // ORA #1 (fuerza Paleta de Sprite 1) -> TempSpriteAttributesish
    uint8_t temp_attr = facing_left ? 0x01 : 0x05;

    // asm/src/prg.asm:2294-2301:
    // LDA TempSpriteAttributesish; AND #$C; LSR A; TAX
    uint8_t x_idx = (uint8_t)((temp_attr & 0x0C) >> 1); // 0 si Izq, 2 si Der
    uint8_t byte_4E = s_sprite_attribute_table[x_idx];     // $00 si Izq, $55 si Der
    uint8_t byte_4F = s_sprite_attribute_table[x_idx + 1]; // $27 si Izq, $8D si Der

    // asm/src/prg.asm:2325-2327:
    // byte_4E = ControlByte ^ byte_4E
    byte_4E ^= sprite_data[0];

    // asm/src/prg.asm:2328-2332:
    // word_50 = (TempSpriteAttributesish & $23) << 2
    uint8_t word_50 = (uint8_t)((temp_attr & 0x23) << 2);

    // Bucle de 4 iteraciones (byte_4B = 1..4) (asm/src/prg.asm:2337-2368):
    for (int i = 0; i < 4; ++i) {
        // Desplazamiento Y (prg.asm:2338-2342): Carry = bit 0 de byte_4F
        uint8_t carry_y = byte_4F & 0x01;
        byte_4F >>= 1;
        int y_pos = anchor_y;
        if (carry_y) {
            y_pos -= 8; // En 6502: ADC #$F7 con carry=1 -> $F7+1 = $F8 = -8
        }

        // Tile CHR (prg.asm:2345-2348)
        uint8_t tile = sprite_data[1 + i];

        // Atributo OAM (prg.asm:2349-2355):
        // LDA word_50; LSR byte_4E; ROR A; LSR byte_4E; ROR A
        uint8_t attr_reg = word_50;
        uint8_t c1 = byte_4E & 0x01;
        byte_4E >>= 1;
        attr_reg = (uint8_t)((attr_reg >> 1) | (c1 ? 0x80 : 0));
        uint8_t c2 = byte_4E & 0x01;
        byte_4E >>= 1;
        attr_reg = (uint8_t)((attr_reg >> 1) | (c2 ? 0x80 : 0));
        uint8_t attr = attr_reg;

        // Desplazamiento X (prg.asm:2357-2361): Carry = bit 0 de byte_4F
        uint8_t carry_x = byte_4F & 0x01;
        byte_4F >>= 1;
        int x_pos = anchor_x;
        if (carry_x) {
            x_pos -= 8;
        }

        out_sprites[i].y = (uint8_t)y_pos;
        out_sprites[i].tile_id = tile;
        out_sprites[i].attributes = attr;
        out_sprites[i].x = (uint8_t)x_pos;
    }
}

bool Jack_CheckBombCollision(const JackPlayer* jack, uint8_t bomb_room_id,
                             uint8_t bomb_tile_x, uint8_t bomb_tile_y,
                             int16_t scroll_x) {
    if (!jack || !jack->is_alive) return false;

    int jx = FIXED_TO_INT(jack->pos_x);
    int jy = FIXED_TO_INT(jack->pos_y);

    int screen_bx;
    if (bomb_room_id == jack->room_id || bomb_room_id == 0) {
        screen_bx = bomb_tile_x * 16 - scroll_x;
    } else if (jack->room_id < MJK_TOTAL_ROOMS_COUNT && g_adjacent_rooms[jack->room_id].right == bomb_room_id) {
        screen_bx = (256 - scroll_x) + (bomb_tile_x * 16);
    } else if (jack->room_id < MJK_TOTAL_ROOMS_COUNT && g_adjacent_rooms[jack->room_id].left == bomb_room_id) {
        screen_bx = (-256 - scroll_x) + (bomb_tile_x * 16);
    } else {
        screen_bx = bomb_tile_x * 16 - scroll_x;
    }

    int screen_by = (int)(bomb_tile_y * 16) - (int)jack->scroll_y;

    // Caja de colisión AABB canónica (16x16 metatile vs Jack [jx+4..jx+12, jy..jy+16])
    // FUENTE DE VERDAD: RES-020 (asm/src/prg.asm:4950 y sub_9A03)
    return (jx + 12 >= screen_bx && jx + 4 <= screen_bx + 16 &&
            jy + 16 >= screen_by && jy <= screen_by + 16);
}

/* ============================================================================
 * NIVELES MIGHTY, MONEDAS Y POWERUPS (TASK-F6-025 / RES-024)
 * ============================================================================ */

// asm/src/data.asm:306-307 MightyLevelColors: .BYTE $16, $12, $26, $2A
const uint8_t k_mighty_level_colors[4] = { 0x16, 0x12, 0x26, 0x2A };

// asm/src/data.asm:309-310 MightyLevelAPressesTable: .BYTE 1, 20, 30, 35
const uint8_t k_mighty_level_presses_table[4] = { 1, 20, 30, 35 };

uint8_t mjk_player_get_palette_color(uint8_t level) {
    if (level > 3) level = 3;
    return k_mighty_level_colors[level];
}

bool mjk_player_use_mighty_coin(mjk_player_t* player) {
    if (!player) return false;
    if (player->mighty_coins == 0 || player->mighty_level >= 3) {
        return false;
    }

    player->mighty_coins--;
    player->mighty_level++;
    player->power_level = (MightyPowerLevel)player->mighty_level;
    player->mighty_presses_left = k_mighty_level_presses_table[player->mighty_level];

    if (player->mighty_level == 3) {
        player->enemy_coin_timer = 320;
    }
    return true;
}

uint8_t mjk_player_on_jump_press(mjk_player_t* player) {
    if (!player || player->mighty_level == 0) return 0;

    if (player->mighty_presses_left > 0) {
        player->mighty_presses_left--;
    }

    // Comprobación de umbrales inferiores según MightyLevelAPressesTable (asm/src/prg.asm:1397-1421):
    // Nivel 3: umbral tabla[2] = 30 -> si cae < 30, desciende a 2
    // Nivel 2: umbral tabla[1] = 20 -> si cae < 20, desciende a 1
    // Nivel 1: umbral tabla[0] = 1  -> si cae < 1, desciende a 0
    if (player->mighty_level == 3) {
        if (player->mighty_presses_left < 30) {
            player->mighty_level = 2;
        }
    } else if (player->mighty_level == 2) {
        if (player->mighty_presses_left < 20) {
            player->mighty_level = 1;
        }
    } else if (player->mighty_level == 1) {
        if (player->mighty_presses_left < 1) {
            player->mighty_level = 0;
        }
    }

    player->power_level = (MightyPowerLevel)player->mighty_level;
    return player->mighty_level;
}

void mjk_player_award_1up(mjk_player_t* player) {
    if (!player) return;
    if (player->lives < 99) {
        player->lives++;
    }
}

