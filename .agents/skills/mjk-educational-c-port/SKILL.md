---
name: mjk-educational-c-port
description: >-
  Enforce C11 educational coding standards, strict separation between platform-agnostic
  core and SDL2 HAL (Windows & Linux), clean abstractions of NES hardware quirks,
  and embedded C BYOR asset extraction. Activate when writing or refactoring C/CMake
  code in src/, include/, or implementing the integrated ROM extractor.
---

# Estándar Educativo C11, Abstracciones Limpias y BYOR Integrado en C

## 1. Arquitectura de Dos Capas (Windows + Linux)

- **`src/core/` + `include/`**:
  - **C11 puro** (`<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, `<string.h>`).
  - **CERO dependencias externas o de SO**: Prohibido `#include <windows.h>`, `<unistd.h>`, `<SDL.h>` o llamadas Win32/POSIX dentro de `src/core/`.
  - Compilación limpia sin warnings (`-Wall -Wextra -Werror` en GCC/Clang; `/W4` en MSVC) mediante `CMakeLists.txt` unificado.
- **`src/platform/`**:
  - Capa SDL2 unificada (`mjk_platform_sdl.c`) para ventana, audio 44.1 kHz, teclado/gamepad, resolución de rutas (`SDL_GetBasePath()`), cuadros de aviso al faltar la ROM y Debug Overlay (`F1`–`F4`).

---

## 2. Qué Preservar al 100% vs. Qué Abstraer Limpiamente

Para obtener **fidelidad jugable máxima** con **código legible, didáctico y extensible**:
- **Preservar fielmente**:
  - Cinemática de Jack (curva parabólica de 64 pasos, *flutter*, inercia horizontal y matriz 9-probe).
  - Geometría y progresión de las 257 salas, 33 condiciones de puertas, cadena de mechas, cofres/bloques secretos y power-ups.
  - Los 6 arquetipos de enemigos, cadencia de spawns, mutación `(A % 6) + 2` y Power Coins.
  - Aritmética BCD, `StageTimer` (`$60`), Sala de Tortura (50 saltos US), cálculo GDV y 4 finales.
  - Estética visual 2C02 (paletas, metasprites 16x16, HUD) y síntesis chiptune 2A03.
- **Abstraer limpiamente** (no replicar limitaciones arcaicas de 1987):
  - No emular *stalls* artificiales de 8 frames con la PPU apagada (`DrawFullScreen`).
  - Sustituir el aborto por pila `PLA; PLA` por un `enum MjkActionResult` explícito.
  - Reemplazar buffers opacos (`RoomDataRAM` críptico) por estructuras C claras (`MjkRoomState`, `MjkBomb`, `MjkChest`).
  - No replicar el parpadeo por límite de 8 sprites por scanline.

---

## 3. Estándar Educativo y Extensibilidad

1. **Nombres semánticos** (`JackPlayer`, `EnemyEntity`, `MjkRoomState`) documentando el símbolo 6502 equivalente en comentarios.
2. **Comentarios pedagógicos ("por qué")**: explicar Q8.8, BCD, indirección 4-tier y síntesis 2A03 citando `asm/src/*.asm`.
3. **Módulos acotados**: máximo ~500–600 líneas por archivo `.c`, funciones pequeñas y data-driven fáciles de modificar o ampliar.

---

## 4. Clean-Room BYOR v2 Integrado en C (Sin Python)

- **Cero bytes propietarios de Tecmo** en el repo ni en C (CHR-ROM, audio bytecode/envolventes, textos PPU, 257 salas/paletas/metatiles y tablas de sprites).
- El ejecutable integra el extractor nativo (`mjk_sha1.c` + `mjk_rom_extractor.c`):
  1. Al arrancar busca `assets/assets.dat` v2 válido; si existe, lo carga.
  2. Si no existe, busca `mbj-us.nes` (`--rom`, directorio actual o ejecutable), verifica su tamaño (`65.552` bytes) y **SHA-1** (`a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`), extrae los assets generando `assets/assets.dat` v2 y continúa la ejecución automáticamente.
  3. Si no encuentra ROM válida, informa al usuario mediante consola y cuadro de diálogo SDL2.
