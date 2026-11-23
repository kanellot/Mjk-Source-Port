/**
 * @file mjk_hash.h
 * @brief Primitivas criptográficas y de integridad en C11 puro (SHA-1 y CRC-32).
 *
 * [ESTÁNDAR EDUCATIVO]:
 * Implementación autocontenida (cero OpenSSL/zlib) usada por el extractor BYOR:
 *   - SHA-1 (RFC 3174) valida la autenticidad exacta de `mbj-us.nes` (65.552 B).
 *   - CRC-32 (IEEE 802.3, polinomial 0xEDB88320) protege `assets/assets.dat` v2.
 */

#ifndef MJK_HASH_H
#define MJK_HASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mjk_build_info.h"

#define MJK_SHA1_DIGEST_SIZE 20u
#define MJK_SHA1_HEX_LEN     40u

typedef struct {
    uint32_t state[5];
    uint64_t bit_count;
    uint8_t  buffer[64];
    size_t   buffer_len;
} MjkSha1Context;

MJK_CORE_API void MjkSha1_Init(MjkSha1Context* ctx);
MJK_CORE_API void MjkSha1_Update(MjkSha1Context* ctx, const void* data, size_t len);
MJK_CORE_API void MjkSha1_Final(MjkSha1Context* ctx, uint8_t out_digest[MJK_SHA1_DIGEST_SIZE]);
MJK_CORE_API void MjkSha1_Compute(const void* data, size_t len, uint8_t out_digest[MJK_SHA1_DIGEST_SIZE]);
MJK_CORE_API void MjkSha1_FormatHex(const uint8_t digest[MJK_SHA1_DIGEST_SIZE],
                                    char out_hex[MJK_SHA1_HEX_LEN + 1u]);

MJK_CORE_API uint32_t MjkCrc32_Init(void);
MJK_CORE_API uint32_t MjkCrc32_Update(uint32_t state, const void* data, size_t len);
MJK_CORE_API uint32_t MjkCrc32_Final(uint32_t state);
MJK_CORE_API uint32_t MjkCrc32_Compute(const void* data, size_t len);

#endif /* MJK_HASH_H */
