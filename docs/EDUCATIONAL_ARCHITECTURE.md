# Arquitectura Educativa y Guía de Modding — Mighty Bomb Jack (C11 Clean-Room)

> **Propósito**: Guía técnica y pedagógica del source port C11 de *Mighty Bomb Jack* (NES USA 1987, `REV_US`). Explica cómo se tradujo un cartucho 6502 de 64 KB en una arquitectura modular de C11 estándar (< 520 líneas por módulo), portable entre Windows y Linux, sin bytes propietarios de Tecmo y fácil de estudiar o extender.

---

## 1. Arquitectura de 2 Capas y Flujo de 1 Frame (`60.0988 Hz`)

El proyecto separa estrictamente la lógica del juego del hardware anfitrión:

```text
+-------------------------------------------------------------------+
| CAPA DE PLATAFORMA (src/platform/ + src/main.c)                   |
| - SDL2 ventana 256x240 escalable, teclado/mando, audio PCM 44.1kHz|
| - Debug Overlay Capa 3 (src/platform/mjk_debug_overlay.c, F1-F6)  |
+-------------------------------------------------------------------+
                 | MjkInputState, framebuffer ARGB8888, 735 samples
                 v
+-------------------------------------------------------------------+
| NÚCLEO PURO C11 (src/core/ — CERO SDL, CERO Win32/POSIX)          |
| - BYOR v2: mjk_sha1.c, mjk_rom_extractor.c, mjk_assets.c          |
| - Video/Mundo: mjk_ppu.c, mjk_room_decoder.c, mjk_camera.c        |
| - Flujo: mjk_title.c, mjk_state_machine.c, mjk_end_screens.c      |
| - Gameplay: mjk_player.c, mjk_objects.c, mjk_doors.c, mjk_enemy.c |
| - HUD/Audio: mjk_hud.c, mjk_apu_synth.c, mjk_apu.c                |
| - Verificación Capa 2: mjk_verify_api.c (exportado en mjk_core)   |
+-------------------------------------------------------------------+
```

En cada iteración del bucle principal (`60.0988 Hz` NTSC):
1. `MjkPlatform_PollInputs()` captura el mando NES (`MjkInputState`: `held`, `just_pressed`, `just_released`) y las teclas de debug (`F1`–`F6`, `[`, `]`).
2. `MjkGame_StepFrame(&game, &input)` avanza un tick de `MjkTitle` (si `MJK_MODE_TITLE`) o `GameHandler 0..C` (si `MJK_MODE_INGAME`).
3. `MjkGame_StepAudio(&game, samples, 735)` drena la cola de sonidos hacia `MjkApu` y sintetiza **735 muestras** PCM 16-bit mono a `44.1 kHz`.
4. `MjkGame_Render(&game, framebuffer)` rasteriza el estado de `MjkPpu` a `uint32_t[256 * 240]` ARGB8888; si el overlay está activo, `MjkDebugOverlay_Render` dibuja telemetría encima.
5. `MjkPlatform_PresentFramebuffer` y `MjkPlatform_QueueAudioSamples` envían vídeo y audio a SDL2.

---

## 2. Clean-Room BYOR v2 (*Bring Your Own ROM*)

El repositorio contiene **cero bytes con copyright de Tecmo** (ni ROMs `.nes`, ni archivos `.dat`/`.chr`, ni arrays de gráficos/música en C).

- Al arrancar (`src/main.c`), el ejecutable busca `assets/assets.dat` (formato binario v2 con cabecera de 64 bytes y checksum **CRC-32**).
- Si no existe, busca `mbj-us.nes` (en el directorio actual, junto al ejecutable o vía `--rom <ruta>`), valida la cabecera iNES (65.552 bytes, Mapper 3 CNROM) y su **SHA-1 canónico** (`a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`) mediante `mjk_sha1.c`, y extrae `assets/assets.dat` automáticamente con `mjk_rom_extractor.c` (sin Python).
- Cada módulo de `src/core/` lee gráficos, textos, tablas de física, salas y secuencias musicales mediante `MjkAssets_ResolveCpuPtr(pkg, cpu_addr, min_bytes)` (`include/mjk_assets.h`), conservando las direcciones originales `$B3FD..$FEB4` de la ROM.

### Compilación y Arranque Rápido
- **Windows (MSVC + CMake)**: `build.bat test` (compila con `/W4 /WX /utf-8` y corre toda la suite `CTest`). Ejecutar `build\bin\mjk.exe --rom <ruta\mbj-us.nes>`.
- **Linux (GCC/Clang + SDL2)**: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure`. Ejecutar `./build/bin/mjk --rom <ruta/mbj-us.nes>`.

---

## 3. Conceptos 6502 Traducidos a C11 Educativo (`include/mjk_types.h`)

1. **Punto Fijo Q8.8 (`mjk_q8_8_t`, `mjk_uq8_8_t`)**: El 6502 no tiene FPU. Coordenadas y velocidades usan 16 bits (`byte_alto` = píxeles enteros, `byte_bajo` = fracción en `1/256` px). Los helpers `MjkUQ8_AddSigned` y `MjkQ8_FloorInt` operan sobre `uint16_t` para evitar comportamiento indefinido (UB) en C11.
2. **Aritmética BCD sin `SED` (`MjkBcd_AddByte`, `MjkBcd_Sub1`)**: Nintendo deshabilitó el modo decimal en la CPU 2A03. El marcador (`score_bcd[4]` little-endian), `stage_timer_bcd` (`$60`), `gdv_bcd` (`$47`) y `greedy_jumps_bcd` (`$50` US) se calculan ajustando nibbles `0..9` manualmente.
3. **`MjkActionResult` en lugar de `PLA; PLA`**: En 6502 varias rutinas (como salir de Torture Room o morir por fuego) manipulan la pila hardware para abortar frames. En C11 se reemplaza por un enum explícito (`MJK_ACTION_CONTINUE`, `MJK_ACTION_ROOM_CHANGED`, `MJK_ACTION_PLAYER_DIED`, `MJK_ACTION_TORTURE_TRIGGERED`).

---

## 4. Subsistemas del Motor

### 4.1. PPU 2C02 y Decodificador 4-Tier de 256 Salas (`mjk_ppu.c`, `mjk_room_decoder.c`)
- **CHR Bank 1**: En `mbj-us.nes`, los bancos 0, 2 y 3 son relleno anticopia `0xFF`; solo **Bank 1** (8 KB = 512 tiles 2bpp planares) contiene los gráficos reales (`$0000` Sprites, `$1000` Background, sprites 8×8 sin límite artificial de 8/scanline).
- **256 IDs de sala (`0x00..0xFF`)**: `Room 0x00` es el fondo de Title Screen; `0x01..0xFF` son las 255 salas jugables.
- **Compresión 4-Tier**: `RoomHalfScreens` (`$C6CA`) -> `HalfScreenLayouts` (`$C8CA`, con ajuste `if (chunk >= 0x60) chunk -= 0x0B`) -> `LayoutChunks` (`$D0EA`) -> `MetatileDefinitions` (`$D7AA`, 4 tiles 8×8) + `TileAttributeTable` (`$C4E1`, 2 bits por metatile). Respeta la orientación **Horizontal** (`adj_flag & 0x04`: mitades superior/inferior, tiras verticales) frente a **Vertical/Fija** (mitades izq/der, tiras horizontales).
- **Persistencia (`MjkRoomPersistRam`)**: 8 slots de 16 bytes para la sección activa (`$0603..$0682`) + 1 slot scratch (`$0683`) para Bomb Rooms (`$90..$9F`), preservando bombas recogidas, cofres abiertos y puertas desbloqueadas.

### 4.2. Cinemática de Jack, 9-Probe y Cámara (`mjk_player.c`, `mjk_camera.c`)
- **Horizontal**: Velocidad instantánea `±0x0180` (`±1.5` px/frame) sin inercia en suelo y aire.
- **Vertical**: Curva discreta de 64 pasos (`word_C0E3` en `$BFB0`) recorrida por un acumulador Q8.8 con tasa `word_C163` (`$00F0` neutro, `$0160` Abajo, `$00A0` Arriba). El *flutter* es estrictamente por flanco de subida (`just_pressed & A`), reiniciando el acumulador al ápice `0x4000`.
- **Colisión 9-Probe sobre `static_metatiles`**: Evalúa los 9 puntos de `byte_A3D8` exclusivamente contra la geometría estática de ROM (`room->static_metatiles`) —nunca contra metatiles de ítems revelados—, evitando bloqueos fantasma al acercarse a power-ups.
- **Cámara (`mjk_camera.c`)**: Ancla a Jack en `X=128` en pasillos horizontales con doble nametable (`nt[0]`/`nt[1]`) y exporta `delta_cam_x/y` para trasladar enemigos y popups en espacio de pantalla.

### 4.3. Objetos, 33 Puertas, 6 Enemigos y HUD (`mjk_objects.c`, `mjk_doors.c`, `mjk_enemy.c`, `mjk_hud.c`)
- **Bombas y Cofres**: Bombas `100`/`200` con cadena de mechas, chispa OAM en Bomb Rooms (23 bombas) y animación *puff* de 2 fases (32f humo `$25` + 32f popup `0x30..0x3F`). Cofres rojos (`0x28`, subpaleta 3) y naranjas (`0x4A`, subpaleta 0, requieren Mighty `>= 1`), bloques secretos (`$6N`) y degradación acumulativa de Mighty (`MightyLevelAPressesTable = {1,20,30,35}`).
- **33 Condiciones de Puerta (`REV_US`)**: Evaluadas desde el byte 1 de cabecera (`(b1 >> 3) - 1`, más el caso especial de Sala `$61` -> condición `$1F`).
- **Enemigos**: `DifficultyTable` (`$BF58`), máximo 4 monstruos vivos simultáneos en 8 slots, mutación `(a_press_counter % 6) + 2` hacia los 6 arquetipos (`Hanezo`, `GejiShogun`, `Gameido`, `Dokuron`, `Desufa`, `Horus`) y transmutación Power Coin de 320 frames (`100..2000` pts).

### 4.4. Sintetizador 2A03 y Motor Bytecode de 8 Canales (`mjk_apu_synth.c`, `mjk_apu.c`)
- **Sintetizador (`mjk_apu_synth.c`)**: Emula Pulse 1/2 (4 duty cycles + hardware sweep `$4001/$4005` a 120 Hz), Triangle de 32 pasos, Noise LFSR de 15 bits (16 periodos NTSC) y mezclador no lineal NTSC con filtro DC pasa-altos.
- **Bytecode VM (`mjk_apu.c`)**: Ejecuta los 8 canales (`0..3` BGM, `4..7` SFX) leyendo directamente desde `MjkAssetPackage`. Un SFX activo inhibe la escritura hardware de su canal BGM hermano sin detener su reloj y restaura sus registros al terminar (`$F6..$FF`).

---

## 5. Guía Práctica de Modding y Extensibilidad

Gracias al aislamiento en módulos pequeños de `src/core/`, modificar o extender el juego es directo:

1. **Añadir un nuevo arquetipo de enemigo**:
   - Añade su valor a `MjkEnemyType` en `include/mjk_types.h`.
   - Agrega su lógica de movimiento en `step_enemy_motion()` y su metasprite/paleta en `archetype_sprite_id()` / `archetype_palette()` dentro de `src/core/mjk_enemy.c`.
   - Añade un caso unitario en `tests/c/test_enemy.c`.
2. **Ajustar física o añadir un modo de dificultad**:
   - Modifica la velocidad horizontal (`0x0180`) o el comportamiento del *flutter* (`0x4000`) en `MjkPlayer_Update()` (`src/core/mjk_player.c`), o escala los timers de `DifficultyTable` en `MjkEnemy_InitForRoom()` (`src/core/mjk_enemy.c`).
3. **Crear una nueva condición de puerta o evento de sala**:
   - Añade un `case` en `MjkDoors_EvaluateCondition()` (`src/core/mjk_doors.c`) y prueba la apertura/cruce en `tests/c/test_doors.c`.
4. **Añadir telemetría personalizada al Debug Overlay**:
   - Extiende `MjkDebugTelemetry` en `include/mjk_debug_overlay.h` y dibújalo con la fuente 5×7 Clean-Room en `src/platform/mjk_debug_overlay.c` sin tocar `src/core/`.

---

## 6. Verificación en 3 Capas

1. **Capa 1 (`CTest` C11 — < 1 s)**: Tests unitarios deterministas en `tests/c/` (`test_types`, `test_hash`, `test_assets`, `test_ppu`, `test_room_decoder`, `test_state_machine`, `test_player`, `test_camera`, `test_objects`, `test_doors`, `test_enemy`, `test_hud`, `test_apu`, `test_full_game`).
2. **Capa 2 (Micro-escenarios semánticos vs NES Oracle)**: `python tools/verify_scenarios.py` carga `mjk_core.dll` / `libmjk_core.so` vía `ctypes` (`include/mjk_verify_api.h`) y valida escenarios cortos (Copyright -> Title -> Sala 1 -> Salto/SFX -> Objetos -> Warps `$90`/`$AC`/`$A3`) frente a `tools/nes_oracle.py` (`cynes`). Usa `--save-diffs` para generar comparativas PNG de 3 paneles en `scratch/`.
3. **Capa 3 (Debug Overlay SDL2 en vivo)**:
   - `F1`: Telemetría (sala, estado, coordenadas, vidas, Mighty, timer) y caja de colisión de Jack.
   - `F2` + `[` / `]` + `F6`: Warp inmediato a cualquier sala (`0x00..0xFF`, incluyendo `$90` Bomb Room, `$AC` Torture Room y `$A0..$A3` Finales).
   - `F3`: Ciclo de cheats de prueba (`OFF` / `INVINCIBLE` / `MIGHTY3`).
   - `F4` / `F5`: Pausa y avance frame a frame.
