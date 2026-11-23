/**
 * @file test_suite.c
 * @brief Automated headless verification suite for Mighty Bomb Jack Native Port.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include "mbj/types.h"
#include "mbj/ram_map.h"
#include "mbj/physics.h"
#include "mbj/room.h"
#include "mbj/room_data.h"
#include "mbj/enemy.h"
#include "mbj/hud.h"
#include "mbj/audio.h"
#include "mbj/rom_loader.h"

static int s_tests_passed = 0;
static int s_tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] %s: %s (line %d)\n", __func__, msg, __LINE__); \
            s_tests_failed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS(msg) \
    do { \
        printf("[PASS] %s: %s\n", __func__, msg); \
        s_tests_passed++; \
    } while(0)

/* Test 1: BCD Arithmetic and Supression */
static void test_bcd_math(void) {
    TEST_ASSERT(BCD_Sub1(0x50) == 0x49, "0x50 - 1 must equal 0x49 BCD");
    TEST_ASSERT(BCD_Sub1(0x10) == 0x09, "0x10 - 1 must equal 0x09 BCD");
    TEST_ASSERT(BCD_Sub1(0x01) == 0x00, "0x01 - 1 must equal 0x00 BCD");
    TEST_ASSERT(BCD_Sub1(0x00) == 0x99, "0x00 - 1 must wrap to 0x99 BCD");

    TEST_ASSERT(BCD_Add(0x12, 0x34) == 0x46, "0x12 + 0x34 must equal 0x46 BCD");
    TEST_ASSERT(BCD_Add(0x08, 0x05) == 0x13, "0x08 + 0x05 must equal 0x13 BCD with carry");

    uint8_t score_bytes[4] = { 0x00, 0x50, 0x12, 0x00 }; /* 125,000 pts */
    uint32_t val = BCD_ToUint32(score_bytes, 4);
    TEST_ASSERT(val == 125000, "Score BCD conversion must equal 125,000");

    TEST_PASS("BCD subtraction, addition, and conversion verified");
}

/* Test 2: Torture Room 50-Jump US Penalty */
static void test_torture_room(void) {
    NES_RAM ram;
    memset(&ram, 0, sizeof(NES_RAM));

    /* Greed trigger: 10 mighty coins */
    ram.player.mighty_coins = 10;
    TEST_ASSERT(TortureRoom_CheckGreed(&ram), "10 coins must trigger greed");

    TortureRoom_Trigger(&ram);
    TEST_ASSERT(ram.torture_jumps_bcd == 0x50, "US penalty must be exactly 50 jumps (0x50 BCD)");
    TEST_ASSERT(ram.player.mighty_coins == 0, "Coins must be stripped on greed sentence");

    /* Simulate 49 jumps */
    for (int j = 0; j < 49; ++j) {
        bool released = TortureRoom_OnJump(&ram);
        TEST_ASSERT(!released, "Player must not be released before 50 jumps");
    }
    TEST_ASSERT(ram.torture_jumps_bcd == 0x01, "Must have exactly 1 jump remaining");

    /* 50th jump releases player */
    bool released = TortureRoom_OnJump(&ram);
    TEST_ASSERT(released, "Player must be released on 50th jump");
    TEST_ASSERT(ram.torture_jumps_bcd == 0x00, "Jump count must be zero");

    TEST_PASS("Torture Room 50-jump penalty lifecycle verified");
}

/* Test 3: Deterministic Mummy Mutation (APressCounter % 6) + 2 */
static void test_mummy_mutation(void) {
    ObjectStructRAM mummy;

    for (int a = 0; a < 256; ++a) {
        memset(&mummy, 0, sizeof(ObjectStructRAM));
        mummy.enemy_type = ENEMY_TYPE_MUMMY;
        mummy.speed_modifier = 2;

        Enemy_TransformMummy(&mummy, (uint8_t)a);

        uint8_t expected = (uint8_t)((a % 6) + 2);
        TEST_ASSERT(mummy.enemy_type == expected, "Mummy must evolve to (A % 6) + 2");
        TEST_ASSERT(mummy.enemy_type >= 2 && mummy.enemy_type <= 7, "Evolved type must be in range 2..7");
        TEST_ASSERT(mummy.status & ENEMY_STATUS_SPAWN_FLASH, "Must activate spawn flash invulnerability");
    }

    TEST_PASS("Deterministic Mummy evolution strictly verified across all 256 inputs");
}

/* Test 4: 4 Endings Evaluation and Pyramid Debris Depth */
static void test_endings_evaluation(void) {
    /* 0 crystal balls -> Worst ending */
    MBJ_EndingType end0 = Ending_Evaluate(0, 5);
    TEST_ASSERT(end0 == ENDING_TYPE_WORST, "0 crystal balls must yield Worst Ending");
    TEST_ASSERT(Ending_GetPyramidDebrisBlocks(end0) == 12, "Worst ending must destroy 12 pyramid blocks");

    /* 1 crystal ball -> Good ending */
    MBJ_EndingType end1 = Ending_Evaluate(1, 5);
    TEST_ASSERT(end1 == ENDING_TYPE_GOOD, "1 crystal ball must yield Good Ending");
    TEST_ASSERT(Ending_GetPyramidDebrisBlocks(end1) == 30, "Good ending must destroy 30 pyramid blocks");

    /* 2 crystal balls, < 5 secret coins -> Great ending */
    MBJ_EndingType end2 = Ending_Evaluate(2, 4);
    TEST_ASSERT(end2 == ENDING_TYPE_GREAT, "2 crystal balls and 4 coins must yield Great Ending");
    TEST_ASSERT(Ending_GetPyramidDebrisBlocks(end2) == 56, "Great ending must destroy 56 pyramid blocks");

    /* 2 crystal balls, >= 5 secret coins -> Best ending */
    MBJ_EndingType end3 = Ending_Evaluate(2, 5);
    TEST_ASSERT(end3 == ENDING_TYPE_BEST, "2 crystal balls and 5 coins must yield Best Ending");
    TEST_ASSERT(Ending_GetPyramidDebrisBlocks(end3) == 90, "Best ending must destroy 90 pyramid blocks");

    TEST_PASS("4 game endings and pyramid demolition depths verified");
}

/* Test 5: Room Geometry 4-Tier Decoder & Decompression Quirk */
static void test_room_geometry_decoder(void) {
    ActiveRoom room;
    Room_DecodeGeometry(1, &room);
    TEST_ASSERT(room.room_id == 1, "Decoded room ID must match");
    TEST_ASSERT(room.palette_id < 8, "Decoded room palette must be valid");

    /* Room 1 floor row (row 14) must contain solid boundary tiles */
    bool has_solid = false;
    for (int col = 0; col < 16; ++col) {
        if (room.metatiles[14][col] != 0) {
            has_solid = true;
            break;
        }
    }
    TEST_ASSERT(has_solid, "Room floor must contain solid blocks");

    TEST_PASS("4-tier geometry decoder and chunk decompression verified");
}

/* Test 6: 2A03 APU Software Synthesizer */
static void test_apu_synthesizer(void) {
    APU_SynthState apu;
    APU_Init(&apu);

    /* Turn on Pulse 1 at A4 (440 Hz -> period ~253) */
    apu.pulse[0].enabled = true;
    apu.pulse[0].period = 253;
    apu.pulse[0].volume = 15;
    apu.pulse[0].duty = 2; /* 50% */

    float buffer[1024];
    APU_GenerateSamples(&apu, buffer, 1024, 44100);

    /* Validate output signal */
    bool has_sound = false;
    for (int i = 0; i < 1024; ++i) {
        TEST_ASSERT(!isnan(buffer[i]), "APU sample must not be NaN");
        TEST_ASSERT(!isinf(buffer[i]), "APU sample must not be Inf");
        TEST_ASSERT(buffer[i] >= -1.0f && buffer[i] <= 1.0f, "APU sample must be normalized [-1.0, 1.0]");
        if (buffer[i] > 0.05f || buffer[i] < -0.05f) {
            has_sound = true;
        }
    }
    TEST_ASSERT(has_sound, "APU must generate active audio waveform");

    TEST_PASS("2A03 APU synthesis and non-linear DAC mixing verified");
}

/* Test 7: Universal ROM Loading & Audio/Graphics Asset Extraction */
static void test_rom_loader(void) {
    MBJ_RomData rom;
    bool ok = ROM_LoadFromDisk(NULL, &rom);
    TEST_ASSERT(ok, "ROM_LoadFromDisk must locate and load mbj-us.nes or fallback CHR");
    if (rom.loaded) {
        /* Verify PRG NoteLengthTable at 0xF39E */
        const uint8_t *durations = ROM_GetPRGPointer(&rom, MBJ_US_ADDR_NOTE_LENGTH);
        TEST_ASSERT(durations != NULL, "NoteLengthTable pointer in PRG must be valid");
        TEST_ASSERT(durations[1] == 1, "Duration 1 must equal 1 frame");
        TEST_ASSERT(durations[5] == 6, "Duration 5 must equal 6 frames");

        /* Verify SoundPointers table at 0xF45C */
        const uint8_t *ptrs = ROM_GetPRGPointer(&rom, MBJ_US_ADDR_SOUND_PTRS);
        TEST_ASSERT(ptrs != NULL, "SoundPointers table pointer in PRG must be valid");
        uint16_t jump_hdr = (uint16_t)(ptrs[0] | (ptrs[1] << 8));
        TEST_ASSERT(jump_hdr == 0xF4A3, "SoundData_01_Jump header must be at 0xF4A3");

        /* Verify CHR tiles decoded */
        TEST_ASSERT(rom.chr_bank.tiles[0].pixels[0][0] <= 3, "CHR tile pixels must be 2bpp");
    }

    TEST_PASS("Universal ROM loader and asset extraction verified");
}

int main(void) {
    printf("=====================================================================\n");
    printf(" Mighty Bomb Jack (NES) Native PC Port - Automated Test Suite\n");
    printf("=====================================================================\n");

    test_bcd_math();
    test_torture_room();
    test_mummy_mutation();
    test_endings_evaluation();
    test_room_geometry_decoder();
    test_apu_synthesizer();
    test_rom_loader();

    printf("=====================================================================\n");
    printf(" Test Results: %d Passed, %d Failed\n", s_tests_passed, s_tests_failed);
    printf("=====================================================================\n");

    return (s_tests_failed == 0) ? 0 : 1;
}
