# Arquitectura del Source Port Limpio y Educacional en C (Mighty Bomb Jack)

- **ID de Entregable**: `research/architecture/source_port_architecture.md`
- **Fase**: Fase 5 — Diseño de Arquitectura del Source Port
- **Regla de Referencia**: `ADR-006` en [`project/DECISIONS.md`](../../project/DECISIONS.md)
- **Estándar del Lenguaje**: C11 estándar con enteros de ancho fijo (`<stdint.h>`, `<stdbool.h>`)
- **Plataformas Objetivo**: **Windows y Linux** (CMake + SDL2, compilación limpia sin warnings)
- **Estrategia de Construcción**: **Reescritura limpia de `src/core/`** (reutilizando `research/` y `src/platform/mjk_platform_sdl.c`), abstrayendo quirks puros de hardware NES de 1987 y preservando 100% de experiencia jugable, estética y audio.
- **Filosofía**: Código limpio, modular (< 600 líneas por módulo), determinista, extensible y de alto valor pedagógico.

---

## 1. Visión General y Desacoplamiento Arquitectónico

El sistema se divide estrictamente en dos capas ortogonales:
1. **El Núcleo de Simulación (`src/core/`)**: Escrito en C11 puro, sin ninguna dependencia externa (ni SDL, ni `<windows.h>`, ni POSIX). Contiene la simulación del juego, extractor/cargador BYOR v2 sobre buffers en memoria, física de Jack, decodificador limpio de 257 salas (`MjkRoomState`), IA de enemigos y sintetizador 2A03 alimentado dinámicamente por `MjkAssetPackage`.
2. **La Capa de Plataforma / Frontend (`src/platform/`)**: Implementa la interfaz multiplataforma (Windows y Linux) mediante SDL2 (`mjk_platform_sdl.c` + `main.c`): ventana, escalado pixel-perfect, mandos y teclado, resolución de rutas (`SDL_GetBasePath()`), auto-extracción BYOR al arrancar si detecta `mbj-us.nes`, Debug Overlay (`F1`–`F4`) y audio a 44.1 kHz.

```
+---------------------------------------------------------------------------------------------------+
|                                     FRONTEND (src/platform/)                                      |
|                                                                                                   |
|   [SDL2 Application Host — Windows & Linux (CMake)]                                               |
|   * main.c                   -> Arranque, auto-extracción BYOR desde mbj-us.nes, loop 60.0988 Hz  |
|   * mjk_platform_sdl.c       -> Ventana, audio 44.1 kHz, gamepad/teclado, Debug Overlay (F1-F4)   |
+---------------------------------------------------------------------------------------------------+
                                                  |
                                                  |  HAL API (include/mjk_platform.h)
                                                  v
+---------------------------------------------------------------------------------------------------+
|                                      NÚCLEO (src/core/)                                           |
|                                                                                                   |
|  [Puro C11 - Cero dependencias de SO/SDL - Abstracciones Limpias - Orientado a la Educación]      |
|                                                                                                   |
|  * mjk_types.h               -> Q8.8, enums de estado y MjkActionResult (sin hack de pila)        |
|  * mjk_sha1 / mjk_rom_extractor / mjk_assets -> Validación SHA-1 y carga BYOR v2 (assets.dat)     |
|  * mjk_ppu.h / .c            -> Renderizado 2C02 (paletas, tiles 2bpp, OAM sin límite 8/scanline) |
|  * mjk_game.h / .c           -> Orquestador compacto del frame + máquina de estados               |
|  * mjk_player / mjk_camera   -> Salto 64 pasos, flutter, colisión 9-probe y cámara de pasillos    |
|  * mjk_room_decoder / objects / doors -> MjkRoomState claro, 257 salas, bombas, cofres, 33 puertas|
|  * mjk_enemy.h / .c          -> 6 arquetipos, spawns, mutación (A%6)+2 y Power Coins              |
|  * mjk_hud.h / .c            -> Marcador BCD unificado, StageTimer $60, Torture 50 saltos y GDV  |
|  * mjk_apu.h / .c            -> Sintetizador 2A03 + intérprete bytecode desde MjkAssetPackage     |
+---------------------------------------------------------------------------------------------------+
```

---

## 2. La Capa de Abstracción de Hardware (HAL Interface)

El núcleo no conoce la existencia de SDL ni de Windows. La comunicación se realiza mediante una interfaz formal en `src/core/mjk_platform.h`:

```c
#ifndef MJK_PLATFORM_H
#define MJK_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#define MJK_SCREEN_WIDTH   256
#define MJK_SCREEN_HEIGHT  240
#define MJK_SAMPLE_RATE    44100

// Máscara digital de botones (mapea 1:1 el protocolo del mando NES $4016)
typedef enum {
    MJK_BTN_A      = (1 << 0),  // Salto / Aleteo
    MJK_BTN_B      = (1 << 1),  // Acción / Abrir cofre
    MJK_BTN_SELECT = (1 << 2),  // No utilizado en partida activa
    MJK_BTN_START  = (1 << 3),  // Pausa / Iniciar partida
    MJK_BTN_UP     = (1 << 4),  // Arriba
    MJK_BTN_DOWN   = (1 << 5),  // Agacharse
    MJK_BTN_LEFT   = (1 << 6),  // Izquierda
    MJK_BTN_RIGHT  = (1 << 7)   // Derecha
} MjkButtonMask;

typedef struct {
    uint8_t buttons_held;        // Botones actualmente presionados
    uint8_t buttons_just_pressed;// Flanco de subida: (held) & !(previous_held)
} MjkInputState;

typedef struct {
    // Buffer de vídeo RGBA32 de 256x240 píxeles generado por el núcleo
    uint32_t framebuffer[MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT];
    
    // Buffer de audio PCM generado por el sintetizador APU (mono/estéreo 16-bit)
    int16_t  audio_samples[2048];
    uint32_t audio_sample_count;
} MjkFrameOutput;

#endif // MJK_PLATFORM_H
```

---

## 3. Desglose Modular y Responsabilidades Didácticas

### 3.1 `mjk_types.h` (Fundamentos y Matemáticas Retro)
- **Concepto Educativo**: La NES de 1987 no tenía punto flotante (`float`). Todo el movimiento de Jack y los enemigos se calculaba en **punto fijo de 16 bits (Q8.8)**: 8 bits para el píxel en pantalla y 8 bits para la fracción subpíxel.
- **Implementación**:
  ```c
  typedef int16_t fixed8_t;
  #define FIXED_FROM_INT(i)   ((fixed8_t)((i) << 8))
  #define FIXED_TO_INT(f)     ((int8_t)((f) >> 8))
  #define FIXED_FRACTION(f)   ((uint8_t)((f) & 0xFF))
  ```

### 3.2 `mjk_player.h / .c` (Cinemática y Físicas de Jack)
- **Concepto Educativo**: La física de salto no utiliza cálculo diferencial continuo. Utiliza una **curva tabular de 64 pasos** (`word_C0E3` en `data.asm`) que modula la velocidad vertical paso a paso.
- **Trazabilidad 6502**:
  - `ApplyPlayerYVelocity` (`prg.asm:1234`)
  - `ApplyPlayerXVelocity` (`prg.asm:1200`)
  - `BigBeefySubHandlesBGColliding` (`prg.asm:3746`, matriz de colisión de 9 puntos de sondeo).

### 3.3 `mjk_room_decoder.h / .c` (Compresión de 257 Salas en 17 KB)
- **Concepto Educativo**: ¿Cómo meter 257 salas detalladas en solo 17 KB de memoria ROM?
  Tecmo diseñó un sistema de **indirección en 4 niveles**:
  1. *Puntero de Sala* $\to$ 2. *Chunk de Layout* $\to$ 3. *Metatile de 16x16* $\to$ 4. *Tiles de 8x8 de la PPU*.
- **Trazabilidad 6502**:
  - `LoadRoomData` (`prg.asm:3110`)
  - `CheckAndMaybeOpenDoor` (`prg.asm:6741`, las 33 condiciones de puertas USA).

### 3.4 `mjk_enemy.h / .c` (IA de Enemigos y Máquina de Mutación)
- **Concepto Educativo**: La IA de los 6 arquetipos (Momias, Hanezo, Geji-Shogun, Gameido, Dokuron, Desufa) y la fórmula de mutación periódica cuando Jack permanece demasiado tiempo en una sala:
  $$\text{NewEnemyType} = (A \pmod 6) + 2$$
- **Trazabilidad 6502**:
  - `RunEnemyLogic` (`prg.asm:8465`)
  - `CalculateModulus` (`prg.asm:2207`)

### 3.5 `mjk_hud.h / .c` (Aritmética BCD y Sala de Tortura)
- **Concepto Educativo**: Los procesadores antiguos usaban BCD (*Binary Coded Decimal*) para evitar algoritmos lentos de división binaria a base 10 al dibujar puntuaciones.
- **Trazabilidad 6502**:
  - Rutinas de adición BCD en `prg.asm:7800-8050`.
  - Penalización de la Sala de Tortura: 50 saltos obligatorios para escapar en la versión USA.

### 3.6 `mjk_apu.h / .c` (Sintetizador de Audio 2A03 Chiptune)
- **Concepto Educativo**: No reproducimos MP3 o WAV estáticos. Emulamos los generadores de ondas reales de la NES:
  - 2 Canales de Onda Cuadrada (Pulse 1 y 2 con ciclo de trabajo variable de 12.5%, 25%, 50%, 75%).
  - 1 Canal de Onda Triangular (bajos y melodía de fondo).
  - 1 Canal de Ruido Blanco pseudoaleatorio generado por un registro LFSR de 15 bits (percusión y explosiones).
  - Intérprete del bytecode del `music-engine.asm` original extraído de la ROM.

---

## 4. Temporización y Modelo de Bucle a 60.0988 Hz

La NES funciona a una frecuencia de refresco exacta de **60.0988 Hz (NTSC)**. Si un juego de PC corre a 60 Hz exactos, acumula un desajuste de 1 frame cada 10 segundos.

El frontend implementará un **Acumulador de Tiempo Fijo (*Fixed Timestep Accumulator*)**:
```c
const double SECONDS_PER_NES_FRAME = 1.0 / 60.0988138974405;
double accumulator = 0.0;
uint64_t previous_time = Platform_GetTimeMicroseconds();

while (is_running) {
    uint64_t current_time = Platform_GetTimeMicroseconds();
    double delta_time = (current_time - previous_time) / 1000000.0;
    previous_time = current_time;

    // Evitar espiral de la muerte si el juego se pausa o pierde foco
    if (delta_time > 0.1) delta_time = 0.1;
    accumulator += delta_time;

    Platform_PollInput(&input_state);

    while (accumulator >= SECONDS_PER_NES_FRAME) {
        // Ejecución exacta de 1 frame de simulación del núcleo (NMI Pipeline)
        MjkGame_SimulateFrame(&game_instance, &input_state, &frame_output);
        accumulator -= SECONDS_PER_NES_FRAME;
    }

    Platform_PresentVideo(&frame_output);
    Platform_QueueAudio(&frame_output);
}
```

---

## 5. Estrategia de Verificación en 3 Capas

Ver la especificación completa en [`verification_strategy.md`](./verification_strategy.md):
1. **Capa 1 (`CTest` C11)**: Contratos puros, extractor BYOR v2 y las 257 salas frente a fixtures de la ROM.
2. **Capa 2 (Micro-escenarios cortos semánticos, 60–240 frames)**: `tools/nes_oracle.py` + `tools/verify_scenarios.py` validan resultados jugables, OAM/paletas NES y eventos APU sin TAS largos frágiles.
3. **Capa 3 (Debug Overlay SDL2)**: Atajos `F1`–`F4` para inspección manual inmediata de hitboxes, salas y finales.

---

## 6. Conclusión

Este diseño arquitectónico garantiza:
- **0% de dependencia de assets con copyright** (auto-extraídos en C desde la ROM del usuario al arrancar).
- **Código didáctico, limpio, modular y extensible**.
- **Compatibilidad nativa Windows y Linux** (CMake + SDL2).
- **Verificación rápida sin efecto mariposa**.
