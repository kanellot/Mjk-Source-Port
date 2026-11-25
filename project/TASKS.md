# Tablero Maestro de Tickets Nucleares — Refactor Integral C11 (`refactor`)

> **CHECKPOINT ACTIVO (Última actualización: 2026-11-25)**
> - **Rama activa**: `refactor` (base `7a74c53`)
> - **Skill obligatoria para subagentes**: `.agents/skills/mjk-clean-refactor/SKILL.md`
> - **Último ticket completado**: **`R-15`** (Hito R4 cerrado: renderizador solo-lectura con tiles pre-decodificados y cache RGB, y motor 2A03 con LUTs + acumulador NTSC 733/734; `29/29` CTests verdes).
> - **Siguiente ticket nuclear**: **`R-16`** (Capa SDL2 limpia y `src/main.c` sin debug/overlay/flags de test).
> - **Protocolo tras fin de cuota**: Leer únicamente este `CHECKPOINT ACTIVO`, el ticket `[TODO]` siguiente y `.agents/skills/mjk-clean-refactor/SKILL.md`. Ejecutar **estrictamente 1 ticket**, verificar compilación sin warnings + `ctest` 100% verde y actualizar este checkpoint.
> - **Política de commits**: 1 commit limpio al cerrar cada Hito (`R-02`, `R-04`, `R-13`, `R-15`, `R-17`, `R-20`), sin binarios ni ROMs.

---

## Hito R1 — Congelación de Comportamiento (Golden Vectors) e Infraestructura

- `[DONE]` **`R-01`** *(Secuencial — Bloquea R-04..R-14)*: **Capturar Golden Vectors del motor actual (`tests/c/golden_reference.h`)**.
  - **Alcance**: Crear un capturador temporal (fuera del ejecutable `mjk`) que enlaza contra el `mjk_core` actual y genera `tests/c/golden_reference.h` (cero bytes de ROM: solo hashes FNV-1a de geometría estática y conteo de objetos de las 256 salas, trayectorias `(x, y)` de salto neutro/Arriba/Abajo/flutter, resultados de las 33 puertas, estados de flujo completo y hashes de 5 frames clave del renderizador).
  - **Gate**: `tests/c/golden_reference.h` generado y compilable; los `18/18` tests actuales siguen pasando.

- `[DONE]` **`R-02`** *(Depende de: R-01)*: **Tipos base, `InputLatch` y CMake modular (`include/mjk/types.h`, `include/mjk/input.h`, `src/game/input.c`, `tests/c/test_input.c`)**.
  - **Alcance**: Crear los headers modernos base (`Vec2`, `Facing`, `ActionResult`, `GameMode`, `SoundId`, `ItemKind`, `EnemyKind`, `RoomFlags`), implementar `InputLatch` (acumula flancos pendientes y los consume una sola vez por paso de simulación) y añadir en `CMakeLists.txt` el target `mjk_engine`, la opción `MJK_ENABLE_SANITIZERS`, `MJK_TEST_ROM_PATH` y `SKIP_RETURN_CODE 77` en `CTest`.
  - **Gate**: `mjk_engine` compila sin warnings; `test_input` verifica iteraciones de `0` pasos (retiene flanco) y `2` pasos (consume flanco una sola vez). *(Commit de Hito R1.)*

---

## Hito R2 — Frontera Única de Assets BYOR (`src/assets/`)

- `[DONE]` **`R-03`** *(Depende de: R-02)*: **SHA-1 y Cargador BYOR (`include/mjk/rom_loader.h`, `src/assets/sha1.c`, `src/assets/rom_loader.c`)**.
  - **Alcance**: Implementar SHA-1, validación iNES (`mbj-us.nes`, `65.552` bytes + SHA-1 canónico), lectura/escritura de `assets.dat` v2 con CRC-32 y helper de resolución de ROM para tests (`77` skip si ausente, `1` fail si explícita inválida).
  - **Gate**: Compila limpio sin warnings; unit tests de SHA-1 (válido/corrupto/tamaño) y round-trip `assets.dat` + CRC-32 corrupto pasan al 100%.

- `[DONE]` **`R-04`** *(Depende de: R-03, R-01)*: **Decodificador Único `asset_bank.c` (`include/mjk/assets.h`, `src/assets/asset_bank.c`, `tests/c/test_assets.c`)**.
  - **Alcance**: Decodificar **una sola vez** al cargar `mbj-us.nes`/`assets.dat`: 512 tiles `uint8_t tiles[512][8][8]` (pre-decodificados), paleta maestra NTSC y paletas de sala, `float jump_speed[64]`, `float jump_rate[3]`, `uint32_t score_table[]`, 32 cadenas `REV_US`, pistas de audio y las **256 salas (`RoomTemplate rooms[256]`)** con objetos tipados.
  - **Gate**: `test_assets` valida las 256 salas contra `golden_reference.h`, tiles, tablas y comportamiento `SKIP (77)` / `FAIL (1)`. *(Commit de Hito R2.)*

---

## Hito R3 — Lógica Pura del Juego (`src/game/`)

- `[DONE]` **`R-05`** *(Depende de: R-04 — Paralelizable con R-06)*: **Puntuación, GDV y Temporizador (`include/mjk/scoring.h`, `src/game/scoring.c`, `tests/c/test_scoring.c`)**.
  - **Alcance**: Aritmética en enteros normales (`uint32_t score`, `int gdv`, `int high_gdv`, `int timer_seconds`, `int lives`), multiplicadores `×1..×5` en Bomb Room, eventos cada 5.000 pts (Bonus/Extra Coin sin duplicar), `StageTimer` (`60→0`, divisor 256f) y Time Over.
  - **Gate**: `test_scoring` cubre toda la tabla de puntos, multiplicadores, 5.000 pts, GDV y StageTimer al 100%.

- `[DONE]` **`R-06`** *(Depende de: R-04 — Paralelizable con R-05)*: **Mundo, Cámara y Persistencia (`include/mjk/world.h`, `src/game/world.c`, `tests/c/test_world.c`)**.
  - **Alcance**: Carga de sala actual y vecina (`RoomState`), cámara en `float`, clamping en bordes (`AT_SCROLL_EDGE`/`SCROLL_STOP`), cruce de costuras (`room_crossed`) y persistencia limpia de 8 salas por sección + scratch de Bomb Room.
  - **Gate**: `test_world` valida salas H/V/Single, scroll, seams y persistencia entre visitas al 100%.

- `[DONE]` **`R-07`** *(Depende de: R-06)*: **Física de Jack y Colisiones (`include/mjk/player.h`, `src/game/player.c`, `tests/c/test_player.c`)**.
  - **Alcance**: Movimiento horizontal (`±1.5f` px/f y parada instantánea), curva parabólica `float` de 64 pasos, flutter por flanco de subida, 9 sondas sobre tipos semánticos (sólido, 10 parciales `8×8`, 7 fuegos `0x5C..0x62`, puertas cerradas, cofres cerrados vs ocultos) y secuencia de muerte (volteretas → suelo → 64f).
  - **Gate**: `test_player` coincide frame a frame con `golden_reference.h` y cubre los 10 parciales, 7 fuegos, cofres y muerte.

- `[DONE]` **`R-08`** *(Depende de: R-05, R-06, R-07)*: **Objetos Parte 1: Bombas, Mechas y Puff (`include/mjk/objects.h`, `src/game/objects.c`, `tests/c/test_objects.c`)**.
  - **Alcance**: Bombas apagadas (`100`) y encendidas (`200`), cadena de mechas + chispa en Bomb Room (23 bombas y apertura de puerta de salida), bonus de orden perfecto en pasillo (`10.200` pts), acumulador Power Coin (`20` pts) y animación puff+popup (`32f + 32f`).
  - **Gate**: `test_objects` (parte 1) pasa 100% en bombas, mechas, orden perfecto y puff.

- `[DONE]` **`R-09`** *(Depende de: R-08)*: **Objetos Parte 2: Cofres, Secretos, Ítems y Mighty (`src/game/objects.c`, `tests/c/test_objects.c`)**.
  - **Alcance**: Cofres rojos (todos los ítems), cofres naranjas (Mighty ≥ 1), bloques secretos desde abajo, niveles Mighty `0..3` con degradación `{1,20,30,35}` y transmutación en nivel 3 (prohibido en Bomb Room).
  - **Gate**: `test_objects` completo cubre cada ítem, cofres rojos/naranjas, secretos y Mighty 0..3.

- `[DONE]` **`R-10`** *(Depende de: R-06, R-09)*: **Sistema de Puertas (`include/mjk/doors.h`, `src/game/doors.c`, `tests/c/test_doors.c`)**.
  - **Alcance**: Las **33 condiciones de puerta (`0..32`)** con nombres claros (`DOOR_COND_*`), caso especial Sala `$61`, eventos de Esfinge, Familia Real/Beelzebub, persistencia de apertura y detección de cruce de Jack.
  - **Gate**: `test_doors` prueba caso falso y verdadero de las 33 condiciones, Esfinge, Familia Real y cruce.

- `[DONE]` **`R-11`** *(Depende de: R-06, R-07)*: **Enemigos Parte 1: Dificultad, Spawns y Momias (`include/mjk/enemies.h`, `src/game/enemies.c`, `tests/c/test_enemies.c`)**.
  - **Alcance**: `DifficultyTable`, spawn diferido (máx. 4 vivos en 8 slots), patrulla de Momias, colisión letal con Jack y mutación determinista `(jump_press_count % 6) + 2`.
  - **Gate**: `test_enemies` (parte 1) valida dificultad, spawns, límite de 4 vivos y mutación.

- `[DONE]` **`R-12`** *(Depende de: R-11)*: **Enemigos Parte 2: 6 Arquetipos, Power Coin y Coleccionables (`src/game/enemies.c`, `tests/c/test_enemies.c`)**.
  - **Alcance**: Los 6 arquetipos (`Hanezo`, `GejiShogun`, `Gameido`, `Dokuron`, `Desufa`, `Horus`), Power Coin (`320f`, cadena `100..2000`, gracia post-transmutación) y Bonus/Extra/Secret Coin + Captive (1UP).
  - **Gate**: `test_enemies` completo cubre los 6 arquetipos, Power Coin y todos los coleccionables.

- `[DONE]` **`R-13`** *(Depende de: R-05..R-12)*: **Pantallas y Máquina de Estados Completa (`include/mjk/game.h`, `src/game/screens.c`, `src/game/game.c`, `tests/c/test_game_flow.c`)**.
  - **Alcance**: Integrar el ciclo completo `TITLE` (Copyright ↔ Title → Falling) → `ROUND_INTRO` → `PLAYING` (pausa, pérdida de vida/checkpoint) → `TIME_OVER` → `TORTURE` (`$AC`, 50 saltos US) → `ROUND_CLEAR` → `GAME_OVER` → **4 Finales (`$A0..$A3`)**.
  - **Gate**: `test_game_flow` valida todo el flujo y los 4 finales contra `golden_reference.h`. *(Commit de Hito R3.)*

---

## Hito R4 — Renderizador Optimizado (`src/render/`) y Audio 2A03 (`src/audio/`)

- `[DONE]` **`R-14`** *(Depende de: R-13 — Paralelizable con R-15)*: **Renderizador Solo-Lectura Optimizado (`include/mjk/renderer.h`, `src/render/renderer.c`, `tests/c/test_renderer.c`)**.
  - **Alcance**: Renderizar `256×240` ARGB8888 leyendo solo `const Game*` + `const GameAssets*`, usando tiles pre-decodificados `8×8` y caché plana de paletas RGB (`bg_pal[4][4]`, `spr_pal[4][4]`).
  - **Gate**: `test_renderer` valida Title, Sala 1, scroll, Bomb Room, Torture Room, HUD, flip H/V, Mighty 0..3 y **Golden Frame Checks** contra `golden_reference.h`.

- `[DONE]` **`R-15`** *(Depende de: R-04 — Paralelizable con R-14)*: **Sintetizador 2A03 con LUTs y Motor de Audio NTSC (`include/mjk/audio.h`, `src/audio/synth_2a03.c`, `src/audio/audio_engine.c`, `tests/c/test_audio.c`)**.
  - **Alcance**: Pulse 1/2 (duty + sweep), Triangle, Noise LFSR, tablas LUT `pulse_lut[31]`/`tnd_lut[203]`, secuenciador de 8 canales (prioridad y restauración SFX→BGM) y **acumulador fraccional NTSC (`733`/`734` muestras/frame)**.
  - **Gate**: `test_audio` valida 4 voces, LUT, restauración BGM tras SFX, silencio, rango `int16_t` y deriva ≤ 1 muestra en 3.600 frames. *(Commit de Hito R4.)*

---

## Hito R5 — Plataforma Limpia y Retirada del Código Antiguo / Debug

- `[TODO]` **`R-16`** *(Depende de: R-14, R-15)*: **Capa SDL2 Limpia y `src/main.c` (`include/mjk/platform.h`, `src/platform/platform_sdl.c`, `src/main.c`)**.
  - **Alcance**: Bucle principal a `60.0988 Hz` usando `InputLatch` y acumulador fraccional de audio; solo CLI `--rom` y `--assets` con validación estricta; cero debug overlay, cero cheats, cero `--headless`/`--smoke-test`/`--frames`.
  - **Gate**: Ejecutable `mjk` compila limpio sin warnings y arranca/cierra correctamente con `mbj-us.nes`.

- `[TODO]` **`R-17`** *(Depende de: R-16)*: **Retirada de Código Viejo, Debug, DLL y Oráculo Python + `CMakeLists.txt` Final**.
  - **Alcance**: Eliminar `src/core/*`, `include/mjk_*.h` antiguos, `src/platform/mjk_debug_overlay.c`, target `mjk_core_shared`, `tools/nes_oracle.py`, `tools/verify_scenarios.py`, el capturador temporal de `R-01` y los tests antiguos. Dejar `CMakeLists.txt` limpio con `mjk_engine`, `mjk` y las 10 suites `CTest`.
  - **Gate**: `0` warnings (`/W4 /WX` y `-Wall -Wextra -Wpedantic -Werror`), `10/10` CTests en verde (y `Skipped (77)` verificado sin ROM). *(Commit de Hito R5.)*

---

## Hito R6 — Limpieza de `research/` y Documentación Técnica Humana

- `[TODO]` **`R-18`** *(Depende de: R-17 — Paralelizable con R-19)*: **Limpieza de `research/` y Actualización de `research/INDEX.md`**.
  - **Alcance**: Eliminar `RES-028`..`RES-036`, `audit/unresolved_questions.md`, `control_flow/unresolved_dependencies.md`, `inventories/unresolved_regions.md` y `architecture/verification_strategy.md`; conservar inventarios y referencias ASM y actualizar `research/INDEX.md`.
  - **Gate**: Cero enlaces rotos dentro de `research/`.

- `[TODO]` **`R-19`** *(Depende de: R-17 — Paralelizable con R-18)*: **Nueva Documentación Técnica Humana en `docs/`**.
  - **Alcance**: Crear `docs/ARCHITECTURE.md`, `docs/GAME_SYSTEMS.md`, `docs/ASSETS_AND_AUDIO.md` y `docs/TESTING_GUIDE.md` (con diagramas Mermaid y explicaciones claras para humanos); eliminar el antiguo `docs/EDUCATIONAL_ARCHITECTURE.md`.
  - **Gate**: Documentación completa, clara y alineada al código refactorizado.

- `[TODO]` **`R-20`** *(Depende de: R-18, R-19)*: **Cierre del Refactor (`README.md`, `AGENTS.md`, `project/PROJECT_STATUS.md`) y Certificación Final**.
  - **Alcance**: Actualizar `README.md`, `AGENTS.md` y `project/PROJECT_STATUS.md`; verificar compilación limpia + `ctest` completo (con ROM → `PASS` y sin ROM → `SKIP 77`); crear `walkthrough.md`.
  - **Gate**: `0` warnings, `10/10` CTests verdes, árbol Git limpio listo para commit final. *(Commit de Hito R6.)*
