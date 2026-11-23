/**
 * @file mjk_assets.c
 * @brief Implementación del cargador de assets clean-room y decodificador 2bpp.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 */

#define _CRT_SECURE_NO_WARNINGS
#include "mjk_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#pragma pack(push, 1)
typedef struct {
    char     magic[8];       // "MJKASSET"
    uint32_t version;        // 0x00010000
    uint32_t total_size;
    uint32_t crc32;
    uint32_t chr_offset;
    uint32_t chr_size;
    uint32_t audio_offset;
    uint32_t audio_size;
    uint32_t text_offset;
    uint32_t text_size;
} HeaderFile;
#pragma pack(pop)

static FILE* OpenAssetFile(const char* requested_path) {
    FILE* f = NULL;
    if (requested_path && requested_path[0] != '\0') {
        f = fopen(requested_path, "rb");
        if (f) return f;
    }

    // Rutas de búsqueda relativas de respaldo
    const char* fallbacks[] = {
        "assets/assets.dat",
        "../assets/assets.dat",
        "assets.dat",
        "bin/assets/assets.dat",
        NULL
    };

    for (int i = 0; fallbacks[i] != NULL; ++i) {
        f = fopen(fallbacks[i], "rb");
        if (f) return f;
    }

#ifdef _WIN32
    // Resolver la ruta absoluta en base al directorio del propio ejecutable (.exe)
    char exe_buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe_buf, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        char* last_slash = strrchr(exe_buf, '\\');
        if (!last_slash) last_slash = strrchr(exe_buf, '/');
        if (last_slash) {
            *last_slash = '\0';
            char candidate[MAX_PATH];
            // 1. <directorio_exe>\assets\assets.dat
            snprintf(candidate, sizeof(candidate), "%s\\assets\\assets.dat", exe_buf);
            f = fopen(candidate, "rb");
            if (f) return f;
            // 2. <directorio_exe>\..\assets\assets.dat
            snprintf(candidate, sizeof(candidate), "%s\\..\\assets\\assets.dat", exe_buf);
            f = fopen(candidate, "rb");
            if (f) return f;
            // 3. <directorio_exe>\assets.dat
            snprintf(candidate, sizeof(candidate), "%s\\assets.dat", exe_buf);
            f = fopen(candidate, "rb");
            if (f) return f;
        }
    }
#endif

    return NULL;
}

bool MjkAssets_Load(const char* path, MjkAssetPackage* package) {
    if (!package) return false;
    memset(package, 0, sizeof(MjkAssetPackage));

    FILE* f = OpenAssetFile(path);
    if (!f) {
        fprintf(stderr, "[ERROR MJK_ASSETS]: No se pudo abrir '%s' ni ninguna ruta alternativa de assets.\n", path ? path : "assets/assets.dat");
        return false;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < (long)sizeof(HeaderFile)) {
        fprintf(stderr, "[ERROR MJK_ASSETS]: Archivo corrupto o demasiado pequeño.\n");
        fclose(f);
        return false;
    }

    uint8_t* buffer = (uint8_t*)malloc((size_t)file_size);
    if (!buffer) {
        fprintf(stderr, "[ERROR MJK_ASSETS]: Memoria insuficiente para cargar assets.\n");
        fclose(f);
        return false;
    }

    size_t read_bytes = fread(buffer, 1, (size_t)file_size, f);
    fclose(f);

    if (read_bytes != (size_t)file_size) {
        free(buffer);
        return false;
    }

    HeaderFile* hdr = (HeaderFile*)buffer;
    if (memcmp(hdr->magic, "MJKASSET", 8) != 0) {
        fprintf(stderr, "[ERROR MJK_ASSETS]: Cabecera mágica inválida.\n");
        free(buffer);
        return false;
    }

    if (hdr->version != 0x00010000) {
        fprintf(stderr, "[ERROR MJK_ASSETS]: Versión incompatible de assets.dat.\n");
        free(buffer);
        return false;
    }

    package->raw_buffer = buffer;
    package->total_size = (size_t)file_size;

    // Asignar secciones internas
    if (hdr->chr_offset + hdr->chr_size <= (uint32_t)file_size) {
        package->chr_rom = buffer + hdr->chr_offset;
        package->chr_size = hdr->chr_size;
        // Banco 1 de CNROM (offset 8192) contiene los 512 tiles reales de NES
        if (package->chr_size >= 16384) {
            package->active_chr_bank = package->chr_rom + 8192;
        } else {
            package->active_chr_bank = package->chr_rom;
        }
    }

    if (hdr->audio_offset + hdr->audio_size <= (uint32_t)file_size) {
        package->audio_data = buffer + hdr->audio_offset;
        package->audio_size = hdr->audio_size;
    }

    if (hdr->text_offset + hdr->text_size <= (uint32_t)file_size) {
        package->text_data = buffer + hdr->text_offset;
        package->text_size = hdr->text_size;
    }

    package->is_loaded = true;
    return true;
}

void MjkAssets_Unload(MjkAssetPackage* package) {
    if (package && package->raw_buffer) {
        free(package->raw_buffer);
        memset(package, 0, sizeof(MjkAssetPackage));
    }
}

void MjkAssets_DecodeTile2bpp(const uint8_t* tile_data, uint8_t out_pixels[64]) {
    if (!tile_data || !out_pixels) return;

    /*
     * Formato Planar 2bpp de la PPU NES:
     * - Fila y (de 0 a 7):
     *   Plano 0: tile_data[y]
     *   Plano 1: tile_data[y + 8]
     * Para cada píxel x (de 0 a 7, siendo 0 el bit más significativo MSB):
     *   color = (bit1 << 1) | bit0
     */
    for (int y = 0; y < 8; ++y) {
        uint8_t plane0 = tile_data[y];
        uint8_t plane1 = tile_data[y + 8];

        for (int x = 0; x < 8; ++x) {
            int shift = 7 - x;
            uint8_t bit0 = (plane0 >> shift) & 1;
            uint8_t bit1 = (plane1 >> shift) & 1;
            out_pixels[y * 8 + x] = (uint8_t)((bit1 << 1) | bit0);
        }
    }
}
