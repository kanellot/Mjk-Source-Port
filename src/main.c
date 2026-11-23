/**
 * @file main.c
 * @brief Punto de entrada principal de Mighty Bomb Jack Source Port (PC).
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FILOSOFÍA: BYOR (Bring Your Own ROM) - Cero dependencias externas.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL.h>
#include "mjk_game.h"
#include "mjk_platform.h"
#include "mjk_assets.h"

// Funciones exportadas por mjk_platform_sdl.c
bool     Platform_Init(void);
void     Platform_Shutdown(void);
uint64_t Platform_GetTimeMicroseconds(void);
void     Platform_PollInputs(MjkInputState* out_inputs);
void     Platform_PresentFramebuffer(const uint32_t* framebuffer);
void     Platform_QueueAudioSamples(const int16_t* samples, uint32_t count);
bool     Platform_IsRunning(void);

#ifdef _WIN32
#include <windows.h>
#define PLATFORM_SLEEP(ms) Sleep(ms)
#else
#include <unistd.h>
#define PLATFORM_SLEEP(ms) usleep((ms) * 1000)
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("=====================================================================\n");
    printf(" MIGHTY BOMB JACK - NATIVE SOURCE PORT (SDL2 MULTIPLATAFORMA)\n");
    printf(" Preservación Educacional y Clean-Room\n");
    printf("=====================================================================\n\n");

    // 1. Cargar paquete de assets clean-room (assets/assets.dat)
    MjkAssetPackage assets;
    if (!MjkAssets_Load("assets/assets.dat", &assets)) {
        const char* msg = 
            "No se ha encontrado el archivo de assets: assets/assets.dat\n\n"
            "Por motivos de propiedad intelectual, este juego no incluye los assets de Tecmo.\n"
            "Para extraer los gráficos y sonidos con su copia legal del cartucho NES, ejecute:\n\n"
            "  python tools/extract_assets.py <su_rom_legal_de_mbj-us.nes>\n";
        printf("[AVISO]: %s\n", msg);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Mighty Bomb Jack - Se requieren Assets", msg, NULL);
        return 1;
    }

    printf("[OK] Paquete assets.dat cargado con éxito (CHR-ROM 32 KB y Audio).\n");

    // 2. Inicializar la plataforma unificada SDL2 (Ventana 4:3, Gamepad y Audio)
    if (!Platform_Init()) {
        fprintf(stderr, "[ERROR]: Fallo al inicializar el subsistema multimedia SDL2.\n");
        MjkAssets_Unload(&assets);
        return 1;
    }
    printf("[OK] Plataforma SDL2 inicializada con éxito (Ventana, Audio y Gamepad).\n");

    // 3. Inicializar el núcleo del juego
    MjkGameState game;
    MjkFrameOutput frame_output;
    memset(&frame_output, 0, sizeof(frame_output));

    MjkGame_Init(&game, &assets);

    MjkInputState current_inputs = {0, 0, 0};

    // Renderizar fotograma inicial (Pantalla de Título) inmediatamente
    MjkGame_SimulateFrame(&game, &current_inputs, &frame_output);
    Platform_PresentFramebuffer(frame_output.framebuffer);

    printf("[OK] Núcleo de simulación a 60 Hz listo. Comenzando bucle principal...\n");

    // 4. Bucle principal con acumulador de tiempo fijo a 60.0988 Hz (NTSC NES)
    const double SECONDS_PER_FRAME = 1.0 / 60.0988138974405;
    double accumulator = 0.0;
    uint64_t previous_time = Platform_GetTimeMicroseconds();

    while (Platform_IsRunning()) {
        uint64_t current_time = Platform_GetTimeMicroseconds();
        double delta_time = (double)(current_time - previous_time) / 1000000.0;
        previous_time = current_time;

        // Evitar espiral de la muerte si la ventana se mueve o pierde foco
        if (delta_time > 0.1) delta_time = 0.1;
        accumulator += delta_time;

        // Sondeo de mandos Xbox/PS y teclado
        Platform_PollInputs(&current_inputs);

        // Simular frames exactos de NMI según el acumulador temporal
        while (accumulator >= SECONDS_PER_FRAME) {
            MjkGame_SimulateFrame(&game, &current_inputs, &frame_output);
            
            // Encolar exactamente 1 frame de audio (735 muestras a 44.1 kHz) por cada frame simulado
            Platform_QueueAudioSamples(frame_output.audio_samples, frame_output.audio_sample_count);

            accumulator -= SECONDS_PER_FRAME;
        }

        // Presentación gráfica en pantalla
        Platform_PresentFramebuffer(frame_output.framebuffer);

        // Pequeño reposo para no saturar la CPU
        PLATFORM_SLEEP(1);
    }

    // 5. Liberación limpia de recursos
    Platform_Shutdown();
    MjkAssets_Unload(&assets);
    printf("\n[OK] Sesión finalizada correctamente. Memoria liberada.\n");
    return 0;
}
