---
name: mjk-clean-refactor
description: >-
  Execute atomic refactor tickets (R-01..R-20) for Mighty Bomb Jack in modern,
  educational, high-performance C11 without 6502/ASM architectural baggage or
  debug/verification code in production. Activate when implementing any R-xx
  ticket in src/, include/mjk/, tests/c/, or docs/.
---

# Guía de Subagente — Refactor Integral C11 (`R-01`..`R-20`)

## 1. Protocolo Nuclear Anti-Agotamiento de Cuota

1. **Un solo ticket por ejecución**: Ejecuta estrictamente el ticket `R-xx` asignado. No avances al siguiente sin dejar compilación y `ctest` al 100% en verde.
2. **Reanudación quirúrgica**: Lee solo `docs/project/TASKS.md` (`CHECKPOINT ACTIVO` + descripción de tu ticket) y los archivos fuente estrictamente necesarios para ese ticket.
3. **Estado verde obligatorio**: Antes de cerrar tu ticket:
   - Compila sin warnings (`/W4 /WX` en MSVC; `-Wall -Wextra -Wpedantic -Werror` en GCC/Clang).
   - Ejecuta `ctest --test-dir build -C Release --output-on-failure` (o `Debug`) y verifica 100% de tests en verde.
   - Actualiza `[DONE]` en tu ticket y el bloque `CHECKPOINT ACTIVO` en `docs/project/TASKS.md`.

---

## 2. Reglas de Arquitectura Moderna (Cero Herencia 6502)

### Mapa de Capas
- `include/mjk/*.h`: Contratos públicos limpios con structs planos y enums autodocumentados.
- `src/assets/`: **Única frontera de bajo nivel NES** (`sha1.c`, `rom_loader.c`, `asset_bank.c`). Lee `mbj-us.nes` / `assets.dat` v2 y decodifica todo **una sola vez** al arrancar.
- `src/game/`: Lógica pura (`scoring.c`, `world.c`, `player.c`, `objects.c`, `doors.c`, `enemies.c`, `screens.c`, `game.c`). **CERO SDL, CERO direcciones ROM (`$C0E3`...), CERO `MjkAssets_ResolveCpuPtr`, CERO escrituras a PPU/OAM.**
- `src/render/`: Renderizador puro de solo lectura (`renderer.c`: recibe `const Game*` y `const GameAssets*`, escribe `uint32_t[256 * 240]`).
- `src/audio/`: Sintetizador 2A03 y secuenciador 8 canales encapsulados (`synth_2a03.c`, `audio_engine.c`).
- `src/platform/`: Solo SDL2 (`platform_sdl.c`) y `src/main.c` mínimo (`--rom` y `--assets`).

### Tipos y Estilo Educativo
- **Enteros normales**: `uint32_t score`, `int gdv`, `int timer_seconds`, `int lives`, `int greedy_jumps` (prohibido BCD en lógica de juego).
- **Física en `float`**: `float x, y, vx, vy, jump_progress` (las constantes `1/256` del juego son exactas en IEEE-754).
- **Campos semánticos**: Prohibidos `ram_state` con bitmasks crípticas, coordenadas empaquetadas `0xYX`, `maybe_bomb_thing` o `gs8_status`. Usa `enum` y `bool` con nombres claros.
- **Tamaño y claridad**: Módulos `.c` ≤ 450 líneas, funciones cortas (≤ 40 líneas) y firmas limpias (2–5 parámetros).
- **Comentarios pedagógicos**: Explica brevemente el *qué* y el *por qué* de cada regla del juego para que alguien que aprende C pueda seguir el código sin conocer ensamblador 6502.
- **Cero código de debug/test en `src/`**: Prohibido `debug_overlay`, `verify_api`, cheats, warps de debug, `--headless`, `--smoke-test` o `--frames`.

---

## 3. Reglas de Rendimiento

1. **Tiles pre-decodificados**: `GameAssets` almacena los 512 tiles `8×8` ya decodificados en `uint8_t tiles[512][8][8]` (32 KB en caché L1). Nunca decodificar bitplanes 2bpp por píxel en el bucle de render.
2. **Caché de paletas ARGB8888**: Pre-resolver `uint32_t bg_pal[4][4]` y `uint32_t spr_pal[4][4]` al cambiar paleta; evitar doble indirección por píxel.
3. **Tablas LUT en audio 2A03**: Usar `float pulse_lut[31]` y `float tnd_lut[203]` en `synth_2a03.c` en lugar de divisiones por muestra a 44.1 kHz.
4. **Acumulador fraccional NTSC en audio**: Alternar `733`/`734` muestras/frame (`44100.0 / 60.0988138974405`) para evitar deriva y drops en SDL2.
5. **Cero alocaciones en caliente**: Prohibido `malloc`/`free` durante el bucle de juego.

---

## 4. Reglas de Verificación (`tests/c/` + `CTest`)

1. **Clean-Room BYOR**: Cero bytes propietarios de Tecmo en `src/`, `include/`, `tests/` o Git. `tests/c/golden_reference.h` solo contiene hashes y trayectorias numéricas de comportamiento.
2. **CTest honesto (`SKIP_RETURN_CODE 77`)**:
   - Los tests que necesiten la ROM buscan `MJK_ROM_PATH` (entorno) → `MJK_TEST_ROM_PATH` (CMake) → `asm/bin/mbj-us.nes`.
   - Si no existe ninguna ROM y no se especificó una ruta explícita: imprimir aviso y retornar `77` (`Skipped`).
   - Si el usuario especificó una ruta explícita y el archivo falta o es inválido: retornar `1` (`FAIL`).
3. **Cobertura exhaustiva**: No limitarse a smoke tests; probar casos límite, casos falso/verdadero y validar contra `tests/c/golden_reference.h`.
4. **InputLatch**: Probar explícitamente iteraciones de `0` pasos (retiene flanco) y `2` pasos (consume flanco una sola vez).
