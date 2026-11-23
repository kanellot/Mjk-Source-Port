/**
 * @file test_hash.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para SHA-1 (RFC 3174) y CRC-32.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_hash.h"

static int g_failures = 0;

static void check_sha1_str(const char* input, const char* expected_hex) {
    uint8_t digest[MJK_SHA1_DIGEST_SIZE];
    char hex[MJK_SHA1_HEX_LEN + 1u];

    MjkSha1_Compute(input, strlen(input), digest);
    MjkSha1_FormatHex(digest, hex);

    if (strcmp(hex, expected_hex) != 0) {
        fprintf(stderr, "[FAIL] SHA-1('%s') = %s (esperado %s)\n", input, hex, expected_hex);
        g_failures++;
    }
}

static void check_sha1_chunked(void) {
    MjkSha1Context ctx;
    uint8_t digest[MJK_SHA1_DIGEST_SIZE];
    char hex[MJK_SHA1_HEX_LEN + 1u];

    MjkSha1_Init(&ctx);
    MjkSha1_Update(&ctx, "a", 1u);
    MjkSha1_Update(&ctx, "", 0u);
    MjkSha1_Update(&ctx, "bc", 2u);
    MjkSha1_Final(&ctx, digest);
    MjkSha1_FormatHex(digest, hex);

    if (strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d") != 0) {
        fprintf(stderr, "[FAIL] SHA-1 chunked('abc') = %s\n", hex);
        g_failures++;
    }
}

static void check_crc32_vectors(void) {
    uint32_t empty_crc = MjkCrc32_Compute("", 0u);
    if (empty_crc != 0x00000000u) {
        fprintf(stderr, "[FAIL] CRC-32('') = 0x%08X (esperado 0x00000000)\n", (unsigned)empty_crc);
        g_failures++;
    }

    const char* check = "123456789";
    uint32_t std_crc = MjkCrc32_Compute(check, strlen(check));
    if (std_crc != 0xCBF43926u) {
        fprintf(stderr, "[FAIL] CRC-32('123456789') = 0x%08X (esperado 0xCBF43926)\n",
                (unsigned)std_crc);
        g_failures++;
    }

    uint32_t st = MjkCrc32_Init();
    st = MjkCrc32_Update(st, "1234", 4u);
    st = MjkCrc32_Update(st, "56789", 5u);
    uint32_t split_crc = MjkCrc32_Final(st);
    if (split_crc != 0xCBF43926u) {
        fprintf(stderr, "[FAIL] CRC-32 incremental = 0x%08X\n", (unsigned)split_crc);
        g_failures++;
    }
}

static void check_optional_rom_hash(void) {
#ifdef MJK_SOURCE_DIR
    char rom_path[512];
    snprintf(rom_path, sizeof(rom_path), "%s/asm/bin/mbj-us.nes", MJK_SOURCE_DIR);
    FILE* f = fopen(rom_path, "rb");
    if (!f) {
        printf("[INFO] ROM local no encontrada en %s; saltando verificacion de ROM.\n", rom_path);
        return;
    }

    uint8_t* buf = (uint8_t*)malloc(65552u);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "[FAIL] Sin memoria para leer ROM.\n");
        g_failures++;
        return;
    }

    size_t read_bytes = fread(buf, 1u, 65552u, f);
    int extra = fgetc(f);
    fclose(f);

    if (read_bytes != 65552u || extra != EOF) {
        free(buf);
        fprintf(stderr, "[FAIL] Tamano inesperado de mbj-us.nes (%u bytes)\n", (unsigned)read_bytes);
        g_failures++;
        return;
    }

    uint8_t digest[MJK_SHA1_DIGEST_SIZE];
    char hex[MJK_SHA1_HEX_LEN + 1u];
    MjkSha1_Compute(buf, 65552u, digest);
    MjkSha1_FormatHex(digest, hex);

    if (strcmp(hex, "a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7") != 0) {
        fprintf(stderr, "[FAIL] SHA-1 mbj-us.nes = %s\n", hex);
        g_failures++;
    }

    uint32_t crc = MjkCrc32_Compute(buf, 65552u);
    if (crc != 0x73DD920Fu) {
        fprintf(stderr, "[FAIL] CRC-32 mbj-us.nes = 0x%08X (esperado 0x73DD920F)\n", (unsigned)crc);
        g_failures++;
    }

    free(buf);
    printf("[OK] ROM REV_US verificada (SHA-1 %s, CRC-32 0x%08X)\n", hex, (unsigned)crc);
#endif
}

int main(void) {
    check_sha1_str("", "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    check_sha1_str("abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
    check_sha1_str("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                   "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    check_sha1_chunked();
    check_crc32_vectors();
    check_optional_rom_hash();

    if (g_failures != 0) {
        return 1;
    }

    printf("[PASS] mjk_hash (RFC 3174 SHA-1 + IEEE CRC-32)\n");
    return 0;
}
