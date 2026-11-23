/**
 * @file mjk_build_info.c
 * @brief Implementación mínima del módulo de identidad del núcleo C11 puro.
 */

#include "mjk_build_info.h"

const char* MjkCore_Version(void) {
    return "0.1.0-clean-slate";
}

uint32_t MjkCore_AbiVersion(void) {
    return 1u;
}
