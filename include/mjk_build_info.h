/**
 * @file mjk_build_info.h
 * @brief Información de compilación y visibilidad ABI del núcleo C11 de Mighty Bomb Jack.
 *
 * [CONCEPTO EDUCATIVO]:
 * Separar el núcleo de simulación (`mjk_core`) de la plataforma (`SDL2`) permite
 * compilar el mismo código tanto como librería estática para el juego como
 * librería compartida (.dll / .so) para herramientas de verificación externas.
 */

#ifndef MJK_BUILD_INFO_H
#define MJK_BUILD_INFO_H

#include <stdint.h>

#if defined(MJK_CORE_SHARED)
#  if defined(_WIN32)
#    if defined(MJK_CORE_BUILDING_DLL)
#      define MJK_CORE_API __declspec(dllexport)
#    else
#      define MJK_CORE_API __declspec(dllimport)
#    endif
#  elif defined(__GNUC__) || defined(__clang__)
#    define MJK_CORE_API __attribute__((visibility("default")))
#  else
#    define MJK_CORE_API
#  endif
#else
#  define MJK_CORE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Devuelve la versión semántica del núcleo de simulación. */
MJK_CORE_API const char* MjkCore_Version(void);

/** @brief Devuelve el número de versión ABI del núcleo (`1` en base limpia). */
MJK_CORE_API uint32_t MjkCore_AbiVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* MJK_BUILD_INFO_H */
