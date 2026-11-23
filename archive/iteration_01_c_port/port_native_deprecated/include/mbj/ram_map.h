/**
 * @file ram_map.h
 * @brief Complete NES RAM Map ($0000-$07FF) and Zero-Page Scratchpad contracts
 *        for Mighty Bomb Jack (NES) Native PC Port.
 * 
 * Based directly on:
 *   - RES-005: Especificación Completa de RAM y Zero-Page Scratchpad
 *   - defs.asm: Struct offsets ($1C bytes per object/enemy, $1F for player)
 *   - ADR-004: Golden Standard US (REV_US)
 */

#ifndef MBJ_RAM_MAP_H
#define MBJ_RAM_MAP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. ZERO-PAGE SCRATCHPAD CONTRACT ($0000-$0014, 21 Bytes)
 * ============================================================================ */

typedef struct {
    uint8_t byte_0;   /* $00: Ptr Lo, Score mask, SoundQueue idx, raw joypad */
    uint8_t byte_1;   /* $01: Ptr Hi, Score attr, track header cursor */
    uint16_t off_2;   /* $02-$03: RoomHalfScreens ptr, Sound channel stream ptr */
    uint16_t off_4;   /* $04-$05: Layout Chunks / Metatile ptr */
    uint16_t off_6;   /* $06-$07: 4-metatile chunk ptr, SoundCommand_Fx subcmd */
    uint8_t byte_8;   /* $08: Column/chunk index (0..15) in HalfScreenLayout */
    uint8_t byte_9;   /* $09: Row/metatile counter during room decode */
    uint8_t byte_A;   /* $0A: Collision bitmask accumulator */
    uint8_t byte_B;   /* $0B: Room object header/type (ReadRoomData) */
    uint8_t byte_C;   /* $0C: Room object counter / attribute offset */
    uint8_t byte_D;   /* $0D: Room object data byte / attribute mask */
    uint8_t byte_E;   /* $0E: Y read cursor in room data stream */
    uint8_t byte_F;   /* $0F: Cell coordinate accumulator */
    uint8_t byte_10;  /* $10: Multiplier in AddScore / loop counter / door ID */
    uint8_t byte_11;  /* $11: Loop counter / EndingState_2 var */
    uint8_t byte_12;  /* $12: Spawn item index from chest */
    uint8_t byte_13;  /* $13: Loop counter in sub_B645 */
    uint8_t byte_14;  /* $14: Temporary offset added in sub_9592 */
} ZP_Scratchpad;

/* ============================================================================
 * 2. 28-BYTE SHARED OBJECT/ENEMY STRUCT (defs.asm:110-137)
 * ============================================================================ */

typedef struct {
    uint8_t status;               /* +0x00: Bit 0=active, 1=in-air, 5=dead floor, 7=dying */
    uint8_t collision_flag;       /* +0x01: Bit 0=wall right, 1=wall left, 2=floor, 3=ceiling */
    uint8_t x_vel_lo;             /* +0x02: Subpixel horizontal velocity */
    uint8_t x_pos_lo;             /* +0x03: Subpixel horizontal position */
    int8_t  x_vel_hi;             /* +0x04: Pixel horizontal velocity (+1 right, -2 left) */
    uint8_t x_pos_hi;             /* +0x05: Pixel horizontal position in screen */
    uint8_t unused_06;            /* +0x06 */
    uint8_t death_timer;          /* +0x07: Death rotation / delay timer */
    uint8_t y_vel_lo;             /* +0x08: Subpixel vertical velocity */
    uint8_t y_pos_lo;             /* +0x09: Subpixel vertical position */
    int8_t  y_vel_hi;             /* +0x0A: Pixel vertical velocity */
    uint8_t y_pos_hi;             /* +0x0B: Pixel vertical position in screen */
    uint8_t var_0C;               /* +0x0C: AI parameter / sub-timer */
    uint8_t var_0D;               /* +0x0D: AI parameter / sub-timer */
    uint8_t sprite_id;            /* +0x0E: Current sprite / tile index */
    uint8_t facing_attr;          /* +0x0F: Facing direction & OAM attributes */
    uint8_t ai_state;             /* +0x10: Archetype-specific AI state machine */
    uint8_t anim_frame;           /* +0x11: Animation tick counter */
    uint8_t enemy_type;           /* +0x12: 1=Mummy, 2=Hanezo, 3=Geji, 4=Gameido, 5=Dokuron, 6=Desufa, 7=Horus */
    uint8_t coin_transform_timer; /* +0x13: Countdown if transformed to coin */
    uint8_t speed_modifier;       /* +0x14: Speed index into word_C0D3 */
    uint8_t target_x;             /* +0x15: Destination / tracking target X */
    uint8_t target_y;             /* +0x16: Destination / tracking target Y */
    uint8_t jump_step;            /* +0x17: Parabolic jump step if leaping */
    uint8_t flutter_state;        /* +0x18: Midair flutter / drop state */
    uint8_t padding[3];           /* +0x19..+0x1B: 28 bytes total */
} ObjectStructRAM;

/* ============================================================================
 * 3. 31-BYTE PLAYER STRUCT ($03CE-$03EC)
 * ============================================================================ */

typedef struct {
    uint8_t status;               /* $03CE (+0x00): Bit 0=active, 1=in air, 3=jump, 7=dying */
    uint8_t collision_flag;       /* $03CF (+0x01): Bit 0=R, 1=L, 2=Floor, 3=Ceiling */
    uint8_t x_vel_lo;             /* $03D0 (+0x02): Velocity X subpixel */
    uint8_t x_pos_lo;             /* $03D1 (+0x03): Position X subpixel */
    int8_t  x_vel_hi;             /* $03D2 (+0x04): Velocity X pixel (+1 R, -2 L) */
    uint8_t x_pos_hi;             /* $03D3 (+0x05): Position X pixel (screen) */
    uint8_t unused_06;            /* $03D4 (+0x06) */
    uint8_t death_timer;          /* $03D5 (+0x07): Death spin / ground delay */
    uint8_t y_vel_lo;             /* $03D6 (+0x08): Velocity Y subpixel */
    uint8_t y_pos_lo;             /* $03D7 (+0x09): Position Y subpixel */
    int8_t  y_vel_hi;             /* $03D8 (+0x0A): Velocity Y pixel */
    uint8_t y_pos_hi;             /* $03D9 (+0x0B): Position Y pixel (screen) */
    uint8_t unused_0C[2];         /* $03DA-$03DB */
    uint8_t sprite_id;            /* $03DC (+0x0E): Current Jack sprite index */
    uint8_t facing_attr;          /* $03DD (+0x0F): $00=Left, $08=Right, $21=behind door */
    uint8_t unused_10;            /* $03DE (+0x10) */
    uint8_t walk_anim_counter;    /* $03DF (+0x11): Walk cycle toggle (every 8 ticks) */
    uint8_t mighty_level;         /* $03E0 (+0x12): 0=Normal, 1=Blue, 2=Orange, 3=Green */
    uint8_t mighty_presses_left;  /* $03E1 (+0x13): Mighty Power button A presses left */
    uint8_t mighty_coins;         /* $03E2 (+0x14): Hoarded Mighty Coins (0..9, 10=Greed) */
    uint8_t score_multiplier;     /* $03E3 (+0x15): Current score multiplier (1..4) */
    uint8_t lives;                /* $03E4 (+0x16): Remaining lives (default 3) */
    uint8_t secret_coins;         /* $03E5 (+0x17): Secret Coins for Good Ending */
    uint8_t jump_table_step;      /* $03E6 (+0x18): Index 0..63 into word_C0E3 */
    uint8_t jump_flutter_state;   /* $03E7 (+0x19): $00=normal, $40=flutter in air */
    uint8_t padding[5];           /* $03E8-$03EC: 31 bytes total */
} PlayerStructRAM;

/* ============================================================================
 * 4. COMPLETE NES 2048-BYTE RAM MAP ($0000-$07FF)
 * ============================================================================ */

typedef struct {
    /* Page 0: Zero-Page ($0000-$00FF) */
    ZP_Scratchpad zp_scratch;     /* $0000-$0014: 21 bytes scratchpad */
    uint8_t zp_reserved[10];      /* $0015-$001E */
    uint8_t brother_counter;      /* $001F: Brother spawner counter */
    uint8_t zp_reserved2[8];      /* $0020-$0027 */
    uint16_t div16_dividend;      /* $0028-$0029: Mod_Number */
    uint8_t div16_divisor;        /* $002A: Mod_Modulus */
    uint8_t div16_remainder;      /* $002B: Mod_Remainder */
    uint8_t zp_reserved3[4];      /* $002C-$002F */
    uint16_t jumptable_ptr;       /* $0030-$0031: JumpTablePointer */
    uint16_t jumptable_target;    /* $0032-$0033: JumpTableTarget */
    uint8_t zp_reserved4[4];      /* $0034-$0037 */
    uint8_t temp_sprite_tile;     /* $0038: TempSpriteTile */
    uint8_t temp_sprite_attr;     /* $0039: TempSpriteAttributesish */
    uint8_t temp_sprite_x;        /* $003A: TempSpriteX */
    uint8_t temp_sprite_y;        /* $003B: TempSpriteY */
    uint8_t current_sprite_idx;   /* $003C: CurrentSpriteIndex (0..63) */
    uint16_t room_layout_ptr;     /* $003D-$003E: off_3D */
    uint8_t ending_substate;      /* $003F: EndingState (0..4) */
    uint8_t pause_flags;          /* $0040: Bit 5=Pause, Bit 6=Started */
    uint8_t zp_reserved5[2];      /* $0041-$0042 */
    uint8_t in_game_flag;         /* $0043: InGameFlag (0=Title, 1=InGame) */
    uint8_t game_state;           /* $0044: GameState (0..12) */
    uint8_t title_screen_state;   /* $0045: TitleScreenState */
    uint8_t update_palette_flag;  /* $0046: UpdatePaletteFlag */
    uint8_t round_clear_state;    /* $0047: RoundClearState */
    uint16_t string_data_ptr;     /* $0048-$0049 */
    uint8_t sprite_offset;        /* $004A: SpriteOffset (idx * 4) */
    uint8_t write_sprite_zp[5];   /* $004B-$004F */
    uint16_t word_50;             /* $0050-$0051 */
    uint8_t zp_reserved6[2];      /* $0052-$0053 */
    uint8_t collected_slot_idx;   /* $0054 */
    uint8_t zp_reserved7;         /* $0055 */
    uint8_t enemy_draw_count;     /* $0056 */
    uint8_t door_packed_info;     /* $0057 */
    uint16_t room_data_rom_ptr;   /* $0058-$0059 */
    uint16_t room_data_ram_ptr;   /* $005A-$005B */
    uint8_t room_search_idx;      /* $005C */
    uint8_t zp_reserved8;         /* $005D */
    uint8_t collected_obj_type;   /* $005E: MaybeCollectedThing (0..19) */
    uint8_t room_data_read_cursor;/* $005F */
    uint8_t sound_engine_zp[6];   /* $0060-$0065 */
    uint8_t zp_reserved9[2];      /* $0066-$0067 */
    uint8_t room_music_idx;       /* $0068 */
    uint8_t zp_reserved10;        /* $0069 */
    uint16_t enemy_coin_timer;    /* $006A-$006B: 320-frame coin conversion */
    uint8_t zp_reserved11[4];     /* $006C-$006F */
    uint8_t obj_tile_pos[2];      /* $0070-$0071 */
    uint8_t replacement_tile[2];  /* $0072-$0073 */
    uint8_t chest_ram_offset;     /* $0074 */
    uint8_t zp_reserved12[3];     /* $0075-$0077 */
    uint16_t active_entity_ptr;   /* $0078-$0079: off_78 */
    uint16_t enemy_struct_ptr;    /* $007A-$007B: EnemyStructPointer */
    uint8_t zp_reserved13[12];    /* $007C-$0087 */
    uint8_t copyright_timer;      /* $0088 */
    uint8_t title_timer;          /* $0089 */
    uint8_t zp_reserved14[6];     /* $008A-$008F */
    uint16_t lit_bomb_table_ptr;  /* $0090-$0091 */
    uint8_t computed_screen_x;    /* $0092 */
    uint8_t zp_reserved15;        /* $0093 */
    uint8_t computed_screen_y;    /* $0094 */
    uint8_t zp_reserved16[3];     /* $0095-$0097 */
    int16_t negated_player_vx;    /* $0098-$0099 */
    int16_t negated_player_vy;    /* $009A-$009B */
    uint8_t zp_reserved17[4];     /* $009C-$009F */
    uint8_t player_score_bcd[4];  /* $00A0-$00A3: 8 BCD digits */
    uint8_t stage_timer_bcd;      /* $00A4: Countdown timer in BCD (starts at $60) */
    uint8_t timer_subframes;      /* $00A5: 256-frame divider */
    uint8_t zp_reserved18[2];     /* $00A6-$00A7 */
    uint8_t timer_status;         /* $00A8 */
    uint8_t time_over_timer;      /* $00A9: 128 frames */
    uint8_t zp_reserved19[2];     /* $00AA-$00AB */
    uint8_t room_chunk_counter;   /* $00AC */
    uint8_t zp_reserved20[63];    /* $00AD-$00EB */
    uint8_t probe_x_px;           /* $00EC */
    uint8_t probe_y_px;           /* $00ED */
    uint8_t hidden_block_idx;     /* $00EE */
    uint8_t zp_reserved21;        /* $00EF */
    uint8_t spawn_item_type;      /* $00F0 */
    uint8_t block_spawn_state;    /* $00F1 */
    uint8_t ppu_update_flag1;     /* $00F2 */
    uint8_t just_jumped_flag;     /* $00F3 */
    uint8_t nametable_select_bit; /* $00F4 */
    uint8_t pending_room_palette; /* $00F5 */
    uint8_t collected_1up_flag;   /* $00F6 */
    uint8_t room_bombs_collected; /* $00F7 */
    uint8_t active_lit_bomb_idx;  /* $00F8 */
    uint8_t round_clear_flags;    /* $00F9 */
    uint8_t a_press_counter;      /* $00FA: Determines enemy type: (A % 6) + 2 */
    uint8_t hud_dirty_flag;       /* $00FB */
    uint8_t ppu_tile_patch_flag;  /* $00FC */
    uint8_t zp_reserved22;        /* $00FD */
    uint8_t post_coin_timer;      /* $00FE: 32 frames grace period */
    uint8_t zp_reserved23;        /* $00FF */

    /* Page 1: CPU Stack ($0100-$01FF) */
    uint8_t cpu_stack[256];

    /* Page 2: OAM Sprite Buffer ($0200-$02FF) */
    uint8_t oam_buffer[256];      /* 64 sprites * 4 bytes: Y, Tile, Attr, X */

    /* Page 3: Game Logic State ($0300-$03FF) */
    uint8_t page3_misc[26];       /* $0300-$0319 */
    uint8_t torture_jumps_bcd;    /* $0319: 50 jumps in US (ADR-004) */
    uint8_t page3_misc2[180];     /* $031A-$03CD */
    PlayerStructRAM player;       /* $03CE-$03EC: 31-byte authentic player struct */
    uint8_t page3_tail[19];       /* $03ED-$03FF */

    /* Page 4-5: Audio Engine State & Music Drivers ($0400-$05FF) */
    uint8_t audio_ram[512];

    /* Page 6-7: Room RAM, Interactive Objects & 8 Enemy Slots ($0600-$07FF) */
    uint8_t room_data_ram[166];   /* $0600-$06A5 */
    ObjectStructRAM enemies[8];   /* $06A6-$0785: 8 slots * 28 bytes = 224 bytes */
    uint8_t page7_tail[122];      /* $0786-$07FF */
} NES_RAM;

#ifdef __cplusplus
}
#endif

#endif /* MBJ_RAM_MAP_H */
