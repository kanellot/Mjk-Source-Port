/**
 * @file test_ppu.c
 * @brief Tests unitarios C11 (Capa 1 / CTest) para el renderizador PPU 2C02 (T-08 / T-09).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjk_assets.h"
#include "mjk_platform.h"
#include "mjk_ppu.h"
#include "mjk_rom_extractor.h"

static int g_failures = 0;

static bool load_rom_assets(MjkAssetPackage* out_pkg) {
#ifdef MJK_SOURCE_DIR
    char rom_path[512];
    snprintf(rom_path, sizeof(rom_path), "%s/asm/bin/mbj-us.nes", MJK_SOURCE_DIR);
    FILE* f = fopen(rom_path, "rb");
    if (!f) {
        return false;
    }
    uint8_t* rom = (uint8_t*)malloc(MJK_ROM_EXPECTED_SIZE);
    if (!rom) {
        fclose(f);
        return false;
    }
    size_t got = fread(rom, 1u, MJK_ROM_EXPECTED_SIZE, f);
    fclose(f);
    if (got != MJK_ROM_EXPECTED_SIZE) {
        free(rom);
        return false;
    }
    uint8_t* v2_buf = NULL;
    size_t v2_size = 0u;
    char sha1_hex[MJK_SHA1_HEX_LEN + 1u];
    MjkAssetError err = MjkRom_BuildAssetsV2Buffer(rom, MJK_ROM_EXPECTED_SIZE, &v2_buf, &v2_size, sha1_hex);
    free(rom);
    if (err != MJK_ASSET_OK) {
        free(v2_buf);
        return false;
    }
    err = MjkAssets_LoadFromMemory(v2_buf, v2_size, out_pkg);
    free(v2_buf);
    return (err == MJK_ASSET_OK);
#else
    (void)out_pkg;
    return false;
#endif
}

static void test_master_palette_and_hud_pure(void) {
    if (MjkPpu_GetMasterColor(0x0Fu) != 0xFF000000u ||
        MjkPpu_GetMasterColor(0x20u) != 0xFFFFFEFFu) {
        fprintf(stderr, "[FAIL] Paleta maestra 2C02 inesperada.\n");
        g_failures++;
    }

    MjkPpu ppu;
    MjkPpu_Init(&ppu, NULL);

    uint8_t score_bcd[4] = { 0x50u, 0x00u, 0x00u, 0x00u };
    uint8_t next = MjkPpu_DrawHudScore(&ppu, score_bcd, 0u);
    if (next != 8u) {
        fprintf(stderr, "[FAIL] DrawHudScore slot=%u (esperado 8)\n", (unsigned)next);
        g_failures++;
    }
    for (int i = 0; i < 6; ++i) {
        if (ppu.oam[i].y != 0xF8u) {
            fprintf(stderr, "[FAIL] Digito %d deberia estar oculto (y=0x%02X)\n", i, ppu.oam[i].y);
            g_failures++;
        }
    }
    if (ppu.oam[6].y != 0x10u || ppu.oam[6].tile != 5u ||
        ppu.oam[7].y != 0x10u || ppu.oam[7].tile != 0u) {
        fprintf(stderr, "[FAIL] Ultimos digitos de score incorrectos\n");
        g_failures++;
    }

    next = MjkPpu_DrawHudTimer(&ppu, 0x60u, 8u);
    if (next != 11u || ppu.oam[8].tile != 0x0Eu || ppu.oam[9].tile != 6u || ppu.oam[10].tile != 0u) {
        fprintf(stderr, "[FAIL] DrawHudTimer('60') incorrecto\n");
        g_failures++;
    }
}

static void test_ppu_with_assets(void) {
    MjkAssetPackage pkg;
    if (!load_rom_assets(&pkg)) {
        printf("[INFO] ROM local no encontrada; saltando tests PPU con CHR-ROM.\n");
        return;
    }

    MjkPpu ppu;
    MjkPpu_Init(&ppu, &pkg);
    if (ppu.palette_ram[0] != 0x21u) {
        fprintf(stderr, "[FAIL] palette_ram[0]=0x%02X (esperado 0x21)\n", ppu.palette_ram[0]);
        g_failures++;
    }

    if (!MjkPpu_LoadRoomPalette(&ppu, &pkg, 0u)) {
        fprintf(stderr, "[FAIL] LoadRoomPalette(0) fallo\n");
        g_failures++;
    }
    uint8_t bg0 = ppu.palette_ram[4];
    for (size_t i = 0u; i < MJK_PPU_PALETTE_SIZE; i += 4u) {
        if (ppu.palette_ram[i] != bg0) {
            fprintf(stderr, "[FAIL] Backdrop no replicado en palette_ram[%u]\n", (unsigned)i);
            g_failures++;
        }
    }

    uint8_t c_before = ppu.palette_ram[0x17u];
    MjkPpu_SetJackMightyColor(&ppu, &pkg, 1u);
    if (ppu.palette_ram[0x17u] == c_before) {
        fprintf(stderr, "[FAIL] SetJackMightyColor(1) no actualizo palette_ram[0x17]\n");
        g_failures++;
    }

    uint8_t s_r = MjkPpu_WriteMetasprite(&ppu, &pkg, 0u, 128u, 120u, 0x00u, 0u);
    uint8_t s_l = MjkPpu_WriteMetasprite(&ppu, &pkg, 0u, 128u, 120u, 0x04u, 4u);
    if (s_r != 4u || s_l != 8u || ppu.oam[0].y >= 0xF0u) {
        fprintf(stderr, "[FAIL] WriteMetasprite Jack fallo\n");
        g_failures++;
    }
    if ((ppu.oam[0].attr & 0x40u) == (ppu.oam[4].attr & 0x40u)) {
        fprintf(stderr, "[FAIL] H-flip de Jack L/R no cambio\n");
        g_failures++;
    }

    if (!MjkPpu_WriteString(&ppu, &pkg, MJK_STR_PUSH_START_BUTTON)) {
        fprintf(stderr, "[FAIL] WriteString PUSH_START_BUTTON fallo\n");
        g_failures++;
    }
    /* $01C8 -> row 14, col 8; primer tile 'P' ($19) */
    if (ppu.nt[0][14][8] != 0x19u) {
        fprintf(stderr, "[FAIL] nt[0][14][8]=0x%02X (esperado 'P'=0x19)\n", ppu.nt[0][14][8]);
        g_failures++;
    }

    uint32_t* fb = (uint32_t*)malloc((size_t)MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT * sizeof(uint32_t));
    if (fb) {
        MjkPpu_RenderFrame(&ppu, &pkg, fb);
        uint32_t c0 = fb[0];
        bool diff = false;
        for (size_t i = 1u; i < (size_t)MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT; ++i) {
            if (fb[i] != c0) {
                diff = true;
                break;
            }
        }
        if (!diff) {
            fprintf(stderr, "[FAIL] RenderFrame produjo imagen uniforme\n");
            g_failures++;
        }
        free(fb);
    }

    MjkAssets_Unload(&pkg);
}

int main(void) {
    test_master_palette_and_hud_pure();
    test_ppu_with_assets();

    if (g_failures > 0) {
        fprintf(stderr, "[FAIL] test_ppu: %d fallo(s).\n", g_failures);
        return 1;
    }
    printf("[OK] test_ppu: paleta 2C02, 2bpp CHR Bank 1, metasprites, texto y HUD OAM OK.\n");
    return 0;
}
