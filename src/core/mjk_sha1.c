/**
 * @file mjk_sha1.c
 * @brief Implementación C11 pura de SHA-1 (RFC 3174) y CRC-32 IEEE 802.3.
 */

#include "mjk_hash.h"
#include <string.h>

static uint32_t rol32(uint32_t value, unsigned bits) {
    return (value << bits) | (value >> (32u - bits));
}

static uint32_t read_be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
}

static void write_be32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)((v >> 24) & 0xFFu);
    p[1] = (uint8_t)((v >> 16) & 0xFFu);
    p[2] = (uint8_t)((v >> 8)  & 0xFFu);
    p[3] = (uint8_t)(v & 0xFFu);
}

static void sha1_transform_block(uint32_t state[5], const uint8_t block[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = read_be32(block + (i * 4));
    }
    for (int i = 16; i < 80; ++i) {
        w[i] = rol32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1u);
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];

    for (int t = 0; t < 80; ++t) {
        uint32_t f = 0u;
        uint32_t k = 0u;
        if (t < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999u;
        } else if (t < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (t < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }

        uint32_t temp = rol32(a, 5u) + f + e + k + w[t];
        e = d;
        d = c;
        c = rol32(b, 30u);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void MjkSha1_Init(MjkSha1Context* ctx) {
    if (!ctx) {
        return;
    }
    ctx->state[0] = 0x67452301u;
    ctx->state[1] = 0xEFCDAB89u;
    ctx->state[2] = 0x98BADCFEu;
    ctx->state[3] = 0x10325476u;
    ctx->state[4] = 0xC3D2E1F0u;
    ctx->bit_count = 0ULL;
    ctx->buffer_len = 0u;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

void MjkSha1_Update(MjkSha1Context* ctx, const void* data, size_t len) {
    if (!ctx || (!data && len > 0u)) {
        return;
    }
    if (len == 0u) {
        return;
    }

    const uint8_t* bytes = (const uint8_t*)data;
    ctx->bit_count += (uint64_t)len * 8ULL;

    size_t offset = 0u;
    if (ctx->buffer_len > 0u) {
        size_t need = 64u - ctx->buffer_len;
        size_t take = (len < need) ? len : need;
        memcpy(ctx->buffer + ctx->buffer_len, bytes, take);
        ctx->buffer_len += take;
        offset += take;

        if (ctx->buffer_len == 64u) {
            sha1_transform_block(ctx->state, ctx->buffer);
            ctx->buffer_len = 0u;
        }
    }

    while ((len - offset) >= 64u) {
        sha1_transform_block(ctx->state, bytes + offset);
        offset += 64u;
    }

    size_t rem = len - offset;
    if (rem > 0u) {
        memcpy(ctx->buffer, bytes + offset, rem);
        ctx->buffer_len = rem;
    }
}

void MjkSha1_Final(MjkSha1Context* ctx, uint8_t out_digest[MJK_SHA1_DIGEST_SIZE]) {
    if (!ctx || !out_digest) {
        return;
    }

    uint64_t total_bits = ctx->bit_count;
    ctx->buffer[ctx->buffer_len++] = 0x80u;

    if (ctx->buffer_len > 56u) {
        while (ctx->buffer_len < 64u) {
            ctx->buffer[ctx->buffer_len++] = 0x00u;
        }
        sha1_transform_block(ctx->state, ctx->buffer);
        ctx->buffer_len = 0u;
    }

    while (ctx->buffer_len < 56u) {
        ctx->buffer[ctx->buffer_len++] = 0x00u;
    }

    write_be32(ctx->buffer + 56, (uint32_t)((total_bits >> 32) & 0xFFFFFFFFu));
    write_be32(ctx->buffer + 60, (uint32_t)(total_bits & 0xFFFFFFFFu));
    sha1_transform_block(ctx->state, ctx->buffer);

    for (int i = 0; i < 5; ++i) {
        write_be32(out_digest + (i * 4), ctx->state[i]);
    }

    memset(ctx, 0, sizeof(*ctx));
}

void MjkSha1_Compute(const void* data, size_t len, uint8_t out_digest[MJK_SHA1_DIGEST_SIZE]) {
    MjkSha1Context ctx;
    MjkSha1_Init(&ctx);
    MjkSha1_Update(&ctx, data, len);
    MjkSha1_Final(&ctx, out_digest);
}

void MjkSha1_FormatHex(const uint8_t digest[MJK_SHA1_DIGEST_SIZE],
                       char out_hex[MJK_SHA1_HEX_LEN + 1u]) {
    static const char k_hex[] = "0123456789abcdef";
    if (!digest || !out_hex) {
        return;
    }
    for (size_t i = 0u; i < MJK_SHA1_DIGEST_SIZE; ++i) {
        out_hex[i * 2u]      = k_hex[(digest[i] >> 4) & 0x0Fu];
        out_hex[i * 2u + 1u] = k_hex[digest[i] & 0x0Fu];
    }
    out_hex[MJK_SHA1_HEX_LEN] = '\0';
}

/* ============================================================================
 * CRC-32 IEEE 802.3 (Polinomial reflejado 0xEDB88320)
 * ============================================================================ */

uint32_t MjkCrc32_Init(void) {
    return 0xFFFFFFFFu;
}

uint32_t MjkCrc32_Update(uint32_t state, const void* data, size_t len) {
    if (!data && len > 0u) {
        return state;
    }
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = state;

    for (size_t i = 0u; i < len; ++i) {
        crc ^= (uint32_t)bytes[i];
        for (int bit = 0; bit < 8; ++bit) {
            uint32_t mask = (crc & 1u) ? 0xEDB88320u : 0u;
            crc = (crc >> 1) ^ mask;
        }
    }
    return crc;
}

uint32_t MjkCrc32_Final(uint32_t state) {
    return state ^ 0xFFFFFFFFu;
}

uint32_t MjkCrc32_Compute(const void* data, size_t len) {
    uint32_t state = MjkCrc32_Init();
    state = MjkCrc32_Update(state, data, len);
    return MjkCrc32_Final(state);
}
