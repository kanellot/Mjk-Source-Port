# Decisiones de Diseño — Mighty Bomb Jack (C11)

Este documento resume las decisiones arquitectónicas y de ingeniería vigentes del source port en C11 de *Mighty Bomb Jack* (`REV_US`).

---

## 1. Versión Canónica: NES USA / NTSC (`REV_US`, 1987)

- **Decisión**: Tomar la versión USA (`mbj-us.nes`, `65.552` bytes, Mapper 3 CNROM, `SHA-1: a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`) como única referencia canónica.
- **Implicaciones**:
  - Pantalla de Copyright y textos en inglés.
  - Penalización de la Sala de Tortura fijada en **50 saltos**.
  - Topología de **256 salas** (`$00..$FF`), **33 condiciones de puerta (`0..32`)** y caso especial de la sala `$61`.
- **Motivo**: Garantizar reglas unívocas y verificación determinista frente a una sola ROM de referencia.

---

## 2. Política Clean-Room BYOR (*Bring Your Own ROM*)

- **Decisión**: El repositorio y los ejecutables distribuidos no contienen ningún byte con copyright de Tecmo (ni gráficos CHR, ni música, ni textos, ni mapas).
- **Cómo funciona**:
  - `src/assets/` integra la validación **SHA-1** de `mbj-us.nes` y genera/carga `assets/assets.dat` v2 protegido con **CRC-32**.
  - No se requiere Python ni herramientas externas para jugar.
  - Toda la decodificación de la ROM ocurre **una sola vez al arrancar**, transformando los datos binarios en estructuras C11 limpias (`MjkAssets`).
- **Motivo**: Cumplimiento legal estricto, distribución autocontenida en Windows/Linux/macOS y desacople total entre el formato de la ROM y la lógica del juego.

---

## 3. Arquitectura Modular en 5 Capas (C11 Estándar)

- **Decisión**: Organizar el motor en cinco capas con dependencias estrictamente unidireccionales y compilar con `-std=c11` y warnings como error (`/W4 /WX` en MSVC; `-Wall -Wextra -Wpedantic -Werror` en GCC/Clang):
  1. `src/assets/` — única frontera que lee la ROM / `assets.dat` y pre-decodifica todos los datos.
  2. `src/game/` — lógica pura de juego (cero SDL2, cero ROM, cero render, cero código de debug).
  3. `src/render/` — renderizador solo-lectura `256×240` ARGB8888.
  4. `src/audio/` — sintetizador 2A03 y secuenciador de 8 canales.
  5. `src/platform/` + `src/main.c` — capa mínima SDL2 (vídeo, audio, teclado/gamepad y bucle fijo `60.0988 Hz`).
- **Motivo**: Código legible con valor educativo, fácil de portar a otras plataformas y testeable de forma headless sin inicializar SDL2.

---

## 4. Código C11 Moderno en Lugar de Convenciones 6502

- **Decisión**: Preservar la fidelidad jugable del original mediante tipos y abstracciones claras de C11, evitando arrastrar artefactos de hardware de 1987:
  - **Física en `float` IEEE-754**: todas las constantes fraccionarias del juego original son múltiplos exactos de $1/256$, por lo que `float` reproduce las trayectorias al píxel exacto con código mucho más claro que aritmética Q8.8 manual.
  - **Enteros estándar**: `uint32_t score`, `int gdv`, `int lives`, `int timer_seconds` en lugar de BCD empaquetado.
  - **Estados y resultados explícitos**: `enum` y `bool` autodocumentados en lugar de bitmasks crípticas o manipulaciones de pila.
  - **Sin quirks de hardware sin valor jugable**: se omite el parpadeo artificial de 8 sprites por scanline y los bloqueos por PPU apagada.
- **Motivo**: Priorizar claridad, mantenibilidad y rendimiento sin sacrificar la sensación ni las reglas del juego original.

---

## 5. Rendimiento Determinista y Cero Alocaciones en Partida

- **Decisión**:
  - **Tiles en caché L1**: los 512 tiles `8×8` se pre-decodifican una vez a `uint8_t tiles[512][8][8]` (32 KB), eliminando decodificación 2bpp por píxel.
  - **Paletas planas**: `bg_pal[4][4]` y `spr_pal[4][4]` pre-resueltas a ARGB8888 por frame.
  - **Tablas LUT en audio 2A03**: `pulse_lut[31]` y `tnd_lut[16][16]` precalculadas en `MjkSynth_Init`, eliminando divisiones por muestra a `44.1 kHz`.
  - **Acumulador fraccional NTSC**: alterna `733`/`734` muestras/frame (`44100 / 60.0988139 ≈ 733.7915`) para sincronizar audio y vídeo sin deriva.
  - **Latch de entrada (`MjkInputLatch`)**: evita perder o duplicar flancos de pulsación cuando el bucle de paso fijo ejecuta 0 o 2 ticks en un frame.
  - **Cero `malloc`/`free`** dentro del bucle principal de juego.
- **Motivo**: Latencia constante, audio limpio sin drops en SDL2 y uso mínimo de CPU.

---

## 6. Verificación con `CTest` y `SKIP_RETURN_CODE 77`

- **Decisión**: Mantener toda la verificación fuera del binario de producción (`mjk`), concentrada en **11 suites unitarias en `tests/c/`** con soporte opcional de ASan/UBSan (`-DMJK_ENABLE_SANITIZERS=ON`).
- **Política de ROM en tests**:
  - Si no existe ROM local ni ruta explícita, los tests que dependen de ella devuelven **`77`** (`Skipped`), nunca un falso `Passed`.
  - Si el usuario especifica una ruta explícita (`MJK_ROM_PATH` o `-DMJK_TEST_ROM_PATH`) y falta o es inválida, el test devuelve **`1`** (`Failed`).
- **Motivo**: Ejecutable de producción limpio y CI/CD reproducible tanto con ROM local como en entornos Clean-Room sin ROM.

---

## 7. CI/CD Multiplataforma en GitHub Actions

- **Decisión**: Automatizar en [`.github/workflows/release.yml`](../.github/workflows/release.yml) la compilación `Release`, ejecución de `CTest` y publicación de paquetes Clean-Room (`windows-x64.zip`, `linux-x64.tar.gz` y `SHA256SUMS.txt`).
- **Motivo**: Releases reproducibles y verificadas sin distribuir assets propietarios.
