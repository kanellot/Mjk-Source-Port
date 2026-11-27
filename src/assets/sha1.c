/**
 * @file sha1.c
 * @brief Implementacion C11 pura de SHA-1 (RFC 3174) y CRC-32 IEEE 802.3.
 *
 * Se utiliza exclusivamente en la frontera de carga (`src/assets/`) para verificar
 * la autenticidad de `mbj-us.nes` y la integridad del paquete `assets/assets.dat`.
 */

#include "mjk/rom_loader.h"
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
        uint32_t f, k;
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

void MjkHash_Sha1(const void* data, size_t len, uint8_t out_digest[MJK_SHA1_DIGEST_LEN]) {
    if (!out_digest || (!data && len > 0u)) {
        return;
    }
    uint32_t state[5] = {
        0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u
    };
    const uint8_t* bytes = (const uint8_t*)data;
    size_t offset = 0u;

    while ((len - offset) >= 64u) {
        sha1_transform_block(state, bytes + offset);
        offset += 64u;
    }

    uint8_t tail[64];
    memset(tail, 0, sizeof(tail));
    size_t rem = len - offset;
    if (rem > 0u) {
        memcpy(tail, bytes + offset, rem);
    }
    tail[rem++] = 0x80u;

    if (rem > 56u) {
        sha1_transform_block(state, tail);
        memset(tail, 0, sizeof(tail));
    }

    uint64_t total_bits = (uint64_t)len * 8ULL;
    write_be32(tail + 56, (uint32_t)((total_bits >> 32) & 0xFFFFFFFFu));
    write_be32(tail + 60, (uint32_t)(total_bits & 0xFFFFFFFFu));
    sha1_transform_block(state, tail);

    for (int i = 0; i < 5; ++i) {
        write_be32(out_digest + (i * 4), state[i]);
    }
}

void MjkHash_Sha1Hex(const uint8_t digest[MJK_SHA1_DIGEST_LEN], char out_hex[MJK_SHA1_HEX_LEN + 1u]) {
    static const char k_hex[] = "0123456789abcdef";
    if (!digest || !out_hex) {
        return;
    }
    for (size_t i = 0u; i < MJK_SHA1_DIGEST_LEN; ++i) {
        out_hex[i * 2u]      = k_hex[(digest[i] >> 4) & 0x0Fu];
        out_hex[i * 2u + 1u] = k_hex[digest[i] & 0x0Fu];
    }
    out_hex[MJK_SHA1_HEX_LEN] = '\0';
}

uint32_t MjkHash_Crc32(const void* data, size_t len) {
    if (!data && len > 0u) {
        return 0u;
    }
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0u; i < len; ++i) {
        crc ^= (uint32_t)bytes[i];
        for (int bit = 0; bit < 8; ++bit) {
            uint32_t mask = (crc & 1u) ? 0xEDB88320u : 0u;
            crc = (crc >> 1) ^ mask;
        }
    }
    return crc ^ 0xFFFFFFFFu;
}
