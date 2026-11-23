/**
 * @file test_types.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para mjk_types.h y mjk_platform.h.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mjk_types.h"
#include "mjk_platform.h"

/* Validación estática de bits contra asm/src/defs.asm:43-50 */
_Static_assert(MJK_BTN_RIGHT  == 0x01u, "JP_Right debe ser 0x01");
_Static_assert(MJK_BTN_LEFT   == 0x02u, "JP_Left debe ser 0x02");
_Static_assert(MJK_BTN_DOWN   == 0x04u, "JP_Down debe ser 0x04");
_Static_assert(MJK_BTN_UP     == 0x08u, "JP_Up debe ser 0x08");
_Static_assert(MJK_BTN_START  == 0x10u, "JP_Start debe ser 0x10");
_Static_assert(MJK_BTN_SELECT == 0x20u, "JP_Select debe ser 0x20");
_Static_assert(MJK_BTN_B      == 0x40u, "JP_B debe ser 0x40");
_Static_assert(MJK_BTN_A      == 0x80u, "JP_A debe ser 0x80");
_Static_assert(sizeof(mjk_q8_8_t) == 2, "mjk_q8_8_t debe medir 2 bytes");
_Static_assert(sizeof(mjk_uq8_8_t) == 2, "mjk_uq8_8_t debe medir 2 bytes");

static int g_failures = 0;

static void check_cond(bool cond, const char* expr, int line) {
    if (!cond) {
        fprintf(stderr, "[FAIL] test_types.c:%d -> %s\n", line, expr);
        g_failures++;
    }
}

#define CHECK(expr) check_cond((bool)(expr), #expr, __LINE__)

static void test_q8_8_signed(void) {
    mjk_q8_8_t pos_half = MjkQ8_FromParts(1, 0x80u); /* +1.5 */
    CHECK(MjkQ8_FloorInt(pos_half) == 1);
    CHECK(MjkQ8_Frac(pos_half) == 0x80u);

    mjk_q8_8_t neg_half = MjkQ8_FromParts(-1, 0x80u); /* -0.5 -> floor = -1 */
    CHECK(neg_half == -128);
    CHECK(MjkQ8_FloorInt(neg_half) == -1);
    CHECK(MjkQ8_Frac(neg_half) == 0x80u);

    mjk_q8_8_t neg_one = MJK_Q8_FROM_INT(-1);
    CHECK(neg_one == -256);
    CHECK(MJK_Q8_FLOOR(neg_one) == -1);
    CHECK(MJK_Q8_FRAC(neg_one) == 0x00u);

    mjk_q8_8_t min_val = MjkQ8_FromParts(-128, 0x00u);
    CHECK(min_val == -32768);
    CHECK(MjkQ8_FloorInt(min_val) == -128);

    mjk_q8_8_t sum = MJK_Q8_ADD(pos_half, neg_half); /* +1.5 + (-0.5) = +1.0 */
    CHECK(MjkQ8_FloorInt(sum) == 1);
    CHECK(MjkQ8_Frac(sum) == 0u);

    mjk_q8_8_t diff = MJK_Q8_SUB(neg_half, MjkQ8_FromParts(1, 0x00u)); /* -1.5 */
    CHECK(MjkQ8_FloorInt(diff) == -2);
    CHECK(MjkQ8_Frac(diff) == 0x80u);
}

static void test_uq8_8_unsigned(void) {
    mjk_uq8_8_t x = MjkUQ8_FromInt(120u);
    CHECK(MjkUQ8_Int(x) == 120u);
    CHECK(MjkUQ8_Frac(x) == 0u);

    x = MjkUQ8_AddSigned(x, MjkQ8_FromParts(0, 0xC0u)); /* +0.75 */
    CHECK(MjkUQ8_Int(x) == 120u);
    CHECK(MjkUQ8_Frac(x) == 0xC0u);

    x = MjkUQ8_AddSigned(x, MjkQ8_FromParts(0, 0x80u)); /* +0.5 -> 121.25 */
    CHECK(MjkUQ8_Int(x) == 121u);
    CHECK(MjkUQ8_Frac(x) == 0x40u);

    x = MjkUQ8_AddSigned(x, MjkQ8_FromParts(-2, 0x00u)); /* -2.0 -> 119.25 */
    CHECK(MjkUQ8_Int(x) == 119u);
    CHECK(MjkUQ8_Frac(x) == 0x40u);

    mjk_uq8_8_t p = MjkUQ8_FromParts(40u, 0x20u);
    CHECK(MjkUQ8_Int(p) == 40u);
    CHECK(MjkUQ8_Frac(p) == 0x20u);
}

static void test_bcd_arithmetic(void) {
    CHECK(MjkBcd_IsValidByte(0x00u));
    CHECK(MjkBcd_IsValidByte(0x47u));
    CHECK(MjkBcd_IsValidByte(0x99u));
    CHECK(!MjkBcd_IsValidByte(0x0Au));
    CHECK(!MjkBcd_IsValidByte(0xFAu));

    CHECK(MjkBcd_ToBinary(0x50u) == 50u);
    CHECK(MjkBcd_FromBinary(60u) == 0x60u);
    CHECK(MjkBcd_FromBinary(120u) == 0x99u);

    uint8_t cy = 0u;
    CHECK(MjkBcd_AddByte(0x19u, 0x01u, 0u, &cy) == 0x20u && cy == 0u);
    CHECK(MjkBcd_AddByte(0x99u, 0x01u, 0u, &cy) == 0x00u && cy == 1u);
    CHECK(MjkBcd_AddByte(0x99u, 0x99u, 1u, &cy) == 0x99u && cy == 1u);

    uint8_t br = 0u;
    CHECK(MjkBcd_SubByte(0x50u, 0x01u, 0u, &br) == 0x49u && br == 0u);
    CHECK(MjkBcd_SubByte(0x10u, 0x05u, 1u, &br) == 0x04u && br == 0u);
    CHECK(MjkBcd_SubByte(0x00u, 0x01u, 0u, &br) == 0x99u && br == 1u);
}

static void test_input_edges(void) {
    MjkInputState in = {0u, 0u, 0u};

    MjkInput_Update(&in, (uint8_t)(MJK_BTN_A | MJK_BTN_RIGHT));
    CHECK(in.held == (MJK_BTN_A | MJK_BTN_RIGHT));
    CHECK(in.just_pressed == (MJK_BTN_A | MJK_BTN_RIGHT));
    CHECK(in.just_released == 0u);

    MjkInput_Update(&in, (uint8_t)(MJK_BTN_A | MJK_BTN_LEFT));
    CHECK(in.held == (MJK_BTN_A | MJK_BTN_LEFT));
    CHECK(in.just_pressed == MJK_BTN_LEFT);
    CHECK(in.just_released == MJK_BTN_RIGHT);

    MjkInput_Update(&in, 0u);
    CHECK(in.held == 0u);
    CHECK(in.just_pressed == 0u);
    CHECK(in.just_released == (MJK_BTN_A | MJK_BTN_LEFT));
}

int main(void) {
    test_q8_8_signed();
    test_uq8_8_unsigned();
    test_bcd_arithmetic();
    test_input_edges();

    if (g_failures != 0) {
        fprintf(stderr, "[FAIL] mjk_types (%d fallos)\n", g_failures);
        return 1;
    }

    printf("[PASS] mjk_types (Q8.8, BCD, MjkButtonMask, MjkInput_Update)\n");
    return 0;
}
