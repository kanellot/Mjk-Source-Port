/**
 * @file main.c
 * @brief Desktop presentation entry point, Raylib 5.5 windowing, 60Hz loop,
 *        Fullscreen toggle (4:3 pillarbox), and Xbox Series / Keyboard input.
 * 
 * Conforms to:
 *   - ADR-004: Golden Standard US (REV_US)
 *   - ADR-005: Raylib Presentation, Fullscreen & Xbox Series / Keyboard Input
 */

#include "mbj/types.h"
#include "mbj/game_state.h"
#include "mbj/render.h"
#include "mbj/room.h"
#include "mbj/collision.h"
#include "mbj/entities.h"
#include "mbj/enemy.h"
#include "mbj/hud.h"
#include "mbj/audio.h"
#include "mbj/rom_loader.h"
#include "mbj/platform_input.h"

#include "raylib.h"
#include <stdio.h>
#include <string.h>

static APU_SynthState s_apu;

static void AudioProcessCallback(void *buffer, unsigned int frames) {
    APU_GenerateSamples(&s_apu, (float*)buffer, (int)frames, 48000);
}

int main(int argc, char *argv[]) {
    bool take_screenshot_and_exit = false;
    const char *preferred_path = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--screenshot") == 0) {
            take_screenshot_and_exit = true;
        } else if (argv[i][0] != '-') {
            preferred_path = argv[i];
        }
    }

    /* 1. Setup Window & Graphics context */
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(MBJ_WINDOW_WIDTH, MBJ_WINDOW_HEIGHT, "Mighty Bomb Jack (NES) - Native PC Port [USA REV_US]");
    SetTargetFPS(MBJ_NES_FPS);

    /* Texture for streaming raw 256x240 pixel buffers directly to GPU */
    Image canvas_img = GenImageColor(MBJ_NES_WIDTH, MBJ_NES_HEIGHT, BLACK);
    Texture2D canvas_texture = LoadTextureFromImage(canvas_img);
    UnloadImage(canvas_img);
    SetTextureFilter(canvas_texture, TEXTURE_FILTER_POINT);

    /* 3. Universal ROM Loader: Extract CHR (Graphics) and PRG (Audio) */
    MBJ_RomData rom_data;
    bool rom_loaded = ROM_LoadFromDisk(preferred_path, &rom_data);
    const CHRBank *chr_bank = &rom_data.chr_bank;

    /* 3b. Initialize 2A03 APU Audio Engine & Raylib Continuous Audio Stream */
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(1024);
    AudioStream audio_stream = LoadAudioStream(48000, 32, 1);
    SetAudioStreamCallback(audio_stream, AudioProcessCallback);
    PlayAudioStream(audio_stream);

    APU_Init(&s_apu);
    SoundDriver_InitWithROM(&rom_data);
    SoundDriver_Play(MUSIC_MAIN);

    /* 4. Initialize Simulation & Input */
    GameState game;
    Game_Init(&game);

    GameInput input;
    memset(&input, 0, sizeof(GameInput));
    Platform_Input_Init();

    /* Decode Room 1 geometry & entities */
    ActiveRoom active_room;
    Room_DecodeGeometry(game.current_room, &active_room);

    RoomEntity room_entities[MAX_ROOM_ENTITIES];
    uint8_t entity_count = Entities_UnpackFromRoom(game.current_room, room_entities, MAX_ROOM_ENTITIES);

    /* Initialize Enemy Manager and spawn initial room monsters */
    Enemy_InitManager();
    Enemy_SpawnMummy(&game.ram, game.current_room);

    /* Standard Sprite Palette 1 (Jack's Suit: White, Blue, Red) */
    const uint8_t jack_palette[4] = { 0x0F, 0x30, 0x12, 0x16 }; /* Transparent, White, Blue, Red */

    /* Standard Background Palette 0 (Room Theme: Black, Green/Stone, Tan, White) */
    const uint8_t bg_palette[16] = {
        0x0F, 0x2A, 0x17, 0x30,
        0x0F, 0x16, 0x27, 0x30,
        0x0F, 0x02, 0x12, 0x30,
        0x0F, 0x09, 0x19, 0x30
    };

    printf("=====================================================================\n");
    printf(" Mighty Bomb Jack (NES) Native Port - Clean Engine [US Version]\n");
    printf(" Controls:\n");
    printf("   Xbox Series / One Controller:\n");
    printf("     - D-Pad / Left Stick: Move\n");
    printf("     - A Button: Jump & Flutter\n");
    printf("     - B / X Button: Mighty Power & Chests\n");
    printf("     - Start: Pause / Start\n");
    printf("   Keyboard:\n");
    printf("     - Arrows / WASD: Move\n");
    printf("     - Z / Space: Jump & Flutter\n");
    printf("     - X / Shift: Mighty Power\n");
    printf("     - Enter: Start\n");
    printf("     - 1 .. 9: Switch Room (1..9)\n");
    printf("   Display:\n");
    printf("     - F11 or Alt+Enter: Toggle Fullscreen (4:3 Pillarbox)\n");
    printf("=====================================================================\n");

    /* 5. Main 60 Hz Interactive Loop */
    while (!WindowShouldClose()) {
        /* Fullscreen toggle (F11 or Alt+Enter) */
        if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
            ToggleFullscreen();
        }

        /* Number keys 1..9 for instant room testing */
        for (int k = KEY_ONE; k <= KEY_NINE; k++) {
            if (IsKeyPressed(k)) {
                game.current_room = (uint8_t)(k - KEY_ONE + 1);
                Room_DecodeGeometry(game.current_room, &active_room);
                entity_count = Entities_UnpackFromRoom(game.current_room, room_entities, MAX_ROOM_ENTITIES);
                game.ram.player.x_pos_hi = 32;
                game.ram.player.y_pos_hi = 176;
                memset(game.ram.enemies, 0, sizeof(game.ram.enemies));
                Enemy_SpawnMummy(&game.ram, game.current_room);
            }
        }

        /* Step 2A03 Audio Synthesizer at 60 Hz */
        SoundDriver_Update60Hz(&s_apu);

        /* Poll Inputs & Step Simulation */
        Platform_Input_Poll(&input);
        if (GameInput_JustPressed(&input, JP_BTN_A)) {
            SoundDriver_Play(SOUND_JUMP);
        }
        Game_Update(&game, &input);

        /* Resolve 9-probe collisions against room geometry */
        Collision_ResolvePlayer(&game.ram.player, active_room.metatiles);

        /* Update active entities (collect bombs, open chests, collect coins) */
        Entities_Update(&game, room_entities, entity_count);

        /* Update AI of active enemies & check collisions */
        Enemy_UpdateAll(&game.ram, (const uint8_t*)active_room.metatiles);
        for (int i = 0; i < 8; ++i) {
            if (game.ram.enemies[i].status & ENEMY_STATUS_ACTIVE) {
                bool coin_col = false;
                if (Enemy_CheckPlayerCollision(&game.ram, &game.ram.enemies[i], &coin_col)) {
                    if (coin_col) {
                        SoundDriver_Play(SOUND_COIN_COLLECTED);
                    } else {
                        /* Jack hit by enemy monster: reset to start */
                        game.ram.player.x_pos_hi = 32;
                        game.ram.player.y_pos_hi = 176;
                    }
                }
            }
        }

        /* Render Room Background Metatiles */
        Room_RenderBackground(game.screen_buffer, &active_room, chr_bank, bg_palette);

        /* Render Interactive Room Entities (Bombs, Chests, Items) */
        Entities_Render(game.screen_buffer, room_entities, entity_count, chr_bank, game.frame_count);

        /* Render Active Enemies */
        for (int i = 0; i < 8; ++i) {
            ObjectStructRAM *e = &game.ram.enemies[i];
            if (e->status & ENEMY_STATUS_ACTIVE) {
                if ((e->status & ENEMY_STATUS_SPAWN_FLASH) && (e->anim_frame & 2)) {
                    continue; /* Flash blink */
                }
                int ex = (int)e->x_pos_hi;
                int ey = (int)e->y_pos_hi;
                const uint8_t *epal = jack_palette;
                bool flip = (e->facing_attr & 8) != 0;
                Renderer_BlitTile(game.screen_buffer, ex, ey, chr_bank, e->sprite_id, epal, flip, false, true);
                Renderer_BlitTile(game.screen_buffer, ex, ey + 8, chr_bank, (uint8_t)(e->sprite_id + 1), epal, flip, false, true);
            }
        }

        /* Draw authentic Jack CHR tiles (16x16 metatiles from SpritesTable) */
        int jack_px = (int)game.ram.player.x_pos_hi;
        int jack_py = (int)game.ram.player.y_pos_hi;

        if (rom_loaded) {
            bool face_left = (game.ram.player.facing_attr == 0);
            uint8_t top_tile = 0x10;
            uint8_t bot_tile = 0x12;

            if (game.ram.player.jump_flutter_state != 0 || game.ram.player.jump_table_step != 0) {
                top_tile = 0x14;
                bot_tile = 0x16;
            } else if ((game.frame_count / 8) & 1) {
                top_tile = 0x11;
                bot_tile = 0x13;
            }

            if (!face_left) {
                Renderer_BlitTile(game.screen_buffer, jack_px, jack_py, chr_bank, top_tile, jack_palette, false, false, true);
                Renderer_BlitTile(game.screen_buffer, jack_px + 8, jack_py, chr_bank, top_tile, jack_palette, true, false, true);
                Renderer_BlitTile(game.screen_buffer, jack_px, jack_py + 8, chr_bank, bot_tile, jack_palette, false, false, true);
                Renderer_BlitTile(game.screen_buffer, jack_px + 8, jack_py + 8, chr_bank, bot_tile, jack_palette, true, false, true);
            } else {
                Renderer_BlitTile(game.screen_buffer, jack_px, jack_py, chr_bank, top_tile, jack_palette, true, false, true);
                Renderer_BlitTile(game.screen_buffer, jack_px + 8, jack_py, chr_bank, top_tile, jack_palette, false, false, true);
                Renderer_BlitTile(game.screen_buffer, jack_px, jack_py + 8, chr_bank, bot_tile, jack_palette, true, false, true);
                Renderer_BlitTile(game.screen_buffer, jack_px + 8, jack_py + 8, chr_bank, bot_tile, jack_palette, false, false, true);
            }

            /* Render authentic HUD Top bar (Score, Stage Timer, Mighty Coins) */
            HUD_Render(game.screen_buffer, &game.ram, chr_bank, game.frame_count);
        }

        /* Update GPU canvas texture with game screen buffer */
        UpdateTexture(canvas_texture, game.screen_buffer);

        /* Present canvas scaled to current window/fullscreen maintaining 4:3 */
        BeginDrawing();
        ClearBackground(BLACK);

        int win_w = GetRenderWidth();
        int win_h = GetRenderHeight();

        /* 4:3 Aspect Ratio Fit */
        float scale = (float)win_h / (float)MBJ_NES_HEIGHT;
        if ((float)win_w / (float)MBJ_NES_WIDTH < scale) {
            scale = (float)win_w / (float)MBJ_NES_WIDTH;
        }

        float render_w = (float)MBJ_NES_WIDTH * scale;
        float render_h = (float)MBJ_NES_HEIGHT * scale;
        float offset_x = ((float)win_w - render_w) * 0.5f;
        float offset_y = ((float)win_h - render_h) * 0.5f;

        Rectangle src_rect = { 0.0f, 0.0f, (float)MBJ_NES_WIDTH, (float)MBJ_NES_HEIGHT };
        Rectangle dst_rect = { offset_x, offset_y, render_w, render_h };
        Vector2 origin = { 0.0f, 0.0f };

        DrawTexturePro(canvas_texture, src_rect, dst_rect, origin, 0.0f, WHITE);
        EndDrawing();

        /* Screenshot on frame 60 or on F12 */
        if (game.frame_count == 60 || IsKeyPressed(KEY_F12)) {
            Image shot_nes = {
                .data = game.screen_buffer,
                .width = MBJ_NES_WIDTH,
                .height = MBJ_NES_HEIGHT,
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
            };
            Image shot_scaled = ImageCopy(shot_nes);
            ImageResizeNN(&shot_scaled, MBJ_WINDOW_WIDTH, MBJ_WINDOW_HEIGHT);
            ExportImage(shot_scaled, "mbj_screenshot.png");
            UnloadImage(shot_scaled);
            printf("[Screenshot] Captured frame %u to mbj_screenshot.png (768x720 3x pixel-perfect)\n", game.frame_count);
            if (take_screenshot_and_exit) {
                break;
            }
        }
    }

    /* Cleanup */
    UnloadAudioStream(audio_stream);
    CloseAudioDevice();
    UnloadTexture(canvas_texture);
    CloseWindow();
    return 0;
}
