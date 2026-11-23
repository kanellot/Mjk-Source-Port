/**
 * @file mjk_game.h
 * @brief Estructura raíz `MjkGame` y máquina de estados completa (`GameHandler` 0..C).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Integra el ciclo completo de Mighty Bomb Jack (REV_US):
 * Título -> Init -> RoundIntro -> LoadRoom -> DrawScreen -> EnterRoom -> Main
 * -> TimeOver / TortureRoom / RoundClear / ExitRoom / AnimateDoorOpen / Ending / GameOver.
 */

#ifndef MJK_GAME_H
#define MJK_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "mjk_assets.h"
#include "mjk_camera.h"
#include "mjk_doors.h"
#include "mjk_enemy.h"
#include "mjk_hud.h"
#include "mjk_objects.h"
#include "mjk_platform.h"
#include "mjk_player.h"
#include "mjk_ppu.h"
#include "mjk_room_decoder.h"
#include "mjk_title.h"
#include "mjk_types.h"

#define MJK_GAME_SOUND_QUEUE_CAP 8u

typedef struct MjkGame {
    const MjkAssetPackage* pkg;
    MjkTopMode             top_mode;
    MjkGameState           state;
    MjkTitleSystem         title;
    MjkPpu                 ppu;
    MjkRoomState           room;
    MjkRoomState           right_room;
    MjkRoomPersistRam      persist;
    MjkPlayer              player;
    MjkCamera              camera;
    MjkObjectSystem        objects;
    MjkEnemyManager        enemies;
    MjkFloatingScore       popups[MJK_FLOAT_SCORE_SLOTS];

    uint32_t score_5000_mark;
    uint8_t  door_flag_09;
    uint8_t  door_flag_1f;
    uint8_t  door_1e_deaths;

    uint8_t  current_room_id;
    uint8_t  checkpoint_room_id;
    uint8_t  torture_return_room_id;
    uint8_t  entry_door_type;
    uint8_t  entry_door_backup;
    uint8_t  torture_return_door_type;

    uint8_t  lives;
    uint8_t  gdv_bcd;
    uint8_t  high_gdv_bcd;
    uint8_t  score_bcd[4];
    uint8_t  high_score_bcd[4];
    uint8_t  stage_timer_bcd;
    uint16_t timer_frames;
    uint8_t  timer_status;

    uint8_t  mighty_coins;
    uint8_t  mighty_level;
    uint8_t  mighty_presses_left;
    uint8_t  score_multiplier;
    uint8_t  last_bomb_room_cleared;
    uint8_t  fire_bombs_collected;
    uint8_t  extra_lives_found;
    uint8_t  crystal_balls_mask;
    uint8_t  secret_coins;
    uint8_t  gold_coins;
    uint8_t  collected_1up;

    bool     paused;
    uint16_t state_timer;
    uint8_t  sub_state;
    uint8_t  gs8_status;
    uint8_t  stage_timer_copy;
    uint8_t  fire_bomb_bonus_bcd;

    uint8_t  torture_scene_flag;
    uint8_t  greedy_jumps_bcd;

    uint8_t  ending_type;
    uint8_t  ending_state;
    uint8_t  ending_depth;

    uint8_t  jack_x;
    uint8_t  jack_y;

    MjkSoundId last_sound_event;
    MjkSoundId sound_queue[MJK_GAME_SOUND_QUEUE_CAP];
    uint8_t    sound_queue_count;
} MjkGame;

MJK_CORE_API void MjkGame_Init(MjkGame* game, const MjkAssetPackage* pkg);
MJK_CORE_API void MjkGame_StepFrame(MjkGame* game, const MjkInputState* input);
MJK_CORE_API void MjkGame_Render(const MjkGame* game, uint32_t* out_fb_argb8888);
MJK_CORE_API void MjkGame_WarpToRoom(MjkGame* game, uint8_t room_id);

MJK_CORE_API void MjkGame_QueueSound(MjkGame* game, MjkSoundId snd);
MJK_CORE_API void MjkGame_AddScore(MjkGame* game, MjkScoreId score_id);
MJK_CORE_API uint8_t MjkGame_AddGdvBcd(uint8_t current_gdv_bcd, uint8_t delta);

MJK_CORE_API MjkActionResult MjkGame_TriggerTortureRoom(MjkGame* game);
MJK_CORE_API void MjkGame_OnTortureJump(MjkGame* game);
MJK_CORE_API void MjkGame_LoseLife(MjkGame* game);
MJK_CORE_API void MjkGame_ExitRoom(MjkGame* game, uint8_t exit_door_dir);

MJK_CORE_API void MjkState_StepRoundClear(MjkGame* game);
MJK_CORE_API void MjkState_StepGameOver(MjkGame* game);
MJK_CORE_API void MjkState_StepEnding(MjkGame* game, const MjkInputState* input);

#endif /* MJK_GAME_H */
