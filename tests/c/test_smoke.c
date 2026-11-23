/**
 * @file test_smoke.c
 * @brief Test unitario C11 (Capa 1 / CTest) para validar el esqueleto mjk_core.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mjk_build_info.h"

_Static_assert(sizeof(uint8_t) == 1, "uint8_t debe medir 1 byte");
_Static_assert(sizeof(int16_t) == 2, "int16_t debe medir 2 bytes");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t debe medir 4 bytes");

int main(void) {
    const char* version = MjkCore_Version();
    if (!version || strcmp(version, "0.1.0-clean-slate") != 0) {
        fprintf(stderr, "[FAIL] MjkCore_Version() devolvio '%s'\n", version ? version : "(null)");
        return 1;
    }

    uint32_t abi = MjkCore_AbiVersion();
    if (abi != 1u) {
        fprintf(stderr, "[FAIL] MjkCore_AbiVersion() devolvio %u (esperado 1)\n", (unsigned)abi);
        return 1;
    }

    printf("[PASS] mjk_smoke (%s, ABI v%u)\n", version, (unsigned)abi);
    return 0;
}
