# Tablero Maestro de Micro-Tareas: Mighty Bomb Jack (Clean-Slate C11 Port)

> **CHECKPOINT ACTIVO (Última actualización: 2026-11-24)**
> - **Último subpaso completado**: **PROYECTO FINALIZADO Y PUBLICADO — Hitos 1–7 (`T-01`..`T-30`, 30/30 `[DONE]`) + Paridad Bloques 1–5 + CI/CD Release** — 19 módulos C11 puros en `src/core/`, `18/18` CTests en verde (`0` warnings MSVC `/W4 /WX` y GCC 15 `-Wall -Wextra -Wpedantic -Werror`), `5/5` escenarios Capa 2 frente a `cynes`, `README.md`, `LICENSE` (MIT + atribución ASM) y pipeline GitHub Actions (Windows x64 + Linux x64) en `main`.
> - **Protocolo tras fin de cuota**: Todos los hitos (`T-01`..`T-30`) y el pulido final de publicación están completados y certificados en verde.
> - **Siguiente micro-tarea**: **Ninguna (`30/30` completadas)**.
> - **Política de commits**: Todos los hitos (`T-03`, `T-07`, `T-11`, `T-15`, `T-24`, `T-26`, `T-30`), bloques de paridad y pipeline CI/CD cerrados con commits limpios en `main`.

---

## Hito 1 — Infraestructura Multiplataforma y Esqueleto Limpio

- `[DONE]` **`T-01`**: Crear `CMakeLists.txt` unificado (Windows MSVC/MinGW + Linux GCC/Clang, `-Wall -Wextra`) con targets `mjk_core`, `mjk` (SDL2) y `mjk_tests` (`CTest`), y dejar `build.bat` / `Makefile` como wrappers finos. *(Gate: compila target dummy + `ctest` pasa.)*
- `[DONE]` **`T-02`**: Definir `include/mjk_types.h` (aritmética `Q8.8`/`BCD`, `MjkActionResult`, enums canónicos `REV_US`) e `include/mjk_platform.h` (contrato HAL limpio sin dependencias de SO). *(Gate: compila limpio + `CTest` de macros Q8.8.)*
- `[DONE]` **`T-03`**: Adaptar `src/platform/mjk_platform_sdl.c` y `src/main.c` al contrato HAL limpio (bucle `60.0988 Hz`, gamepad/teclado, resolución de rutas vía `SDL_GetBasePath()`). *(Gate: ventana SDL2 arranca/cierra limpio en Windows y Linux.)*

## Hito 2 — Extractor BYOR v2 Integrado en C (Cero Python al Arrancar)

- `[DONE]` **`T-04`**: Implementar `mjk_sha1.c` y `mjk_crc32` en C11 puro + `CTest` verificando vectores estándar y el hash de `mbj-us.nes` (`a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`).
- `[DONE]` **`T-05`**: Implementar primera mitad de `mjk_rom_extractor.c`: validación iNES (`65.552` B + SHA-1) y extracción de `CHR-ROM` (32 KB), tablas/bytecodes de audio (`music-data.asm`) y cadenas de texto PPU (`$C244..$C500`) + `CTest`.
- `[DONE]` **`T-06`**: Completar `mjk_rom_extractor.c`: extracción de tablas de las 257 salas (`RoomHalfScreens`, `HalfScreenLayouts`, `LayoutChunks`, `MetatileDefinitions`, `AdjacentRoomsTable`, `RoomData_01..FF`, paletas) y tablas de sprites/spawns/dificultad -> escritura de `assets/assets.dat` v2 + `CTest`.
- `[DONE]` **`T-07`**: Implementar `mjk_assets.c` (carga/validación CRC-32 de `assets.dat` v2) e integrar auto-extracción al arranque en `main.c` (`--rom`, ruta local/exe y cuadro de aviso SDL2 si falta ROM) + `CTest`.

## Hito 3 — Renderizador PPU 2C02, 257 Salas y Debug Overlay

- `[DONE]` **`T-08`**: Implementar `mjk_ppu.c` (paleta maestra 2C02 de 64 colores, decodificación planar 2bpp, blitter de metatiles 16x16 y sprites OAM sin flicker artificial de 8/scanline) + `CTest`.
- `[DONE]` **`T-09`**: Añadir al PPU el renderizado de cadenas de texto (`WriteStringToPPU`) y dígitos/HUD directamente desde los tiles oficiales de `CHR-ROM` + `CTest`.
- `[DONE]` **`T-10`**: Implementar `mjk_room_decoder.c` con `MjkRoomState` limpio (4-tier + paletas + persistencia de 8 salas) + `CTest` validando geometría de las **257 salas** contra fixture de `nes_oracle.py`.
- `[DONE]` **`T-11`**: Implementar **Capa 3 Debug Overlay SDL2**: `F1` telemetría/hitboxes 9-probe, `F2` warp directo a cualquier sala `0..256`/Bomb/Torture/Final, `F3` invencibilidad/Mighty, `F4` pausa/frame-step.

## Hito 4 — Esqueleto Completo del Juego (Todas las Pantallas y Transiciones)

- `[DONE]` **`T-12`**: Implementar `mjk_title.c`: ciclo completo `TitleScreen_US_0/1` (Copyright 256f) $\leftrightarrow$ `TitleScreen_0/1/2` (Title Screen 512f timeout) $\to$ `TitleScreen_3` (Jack Falling 73f) $\to$ `InGameFlag=1` + `CTest`.
- `[DONE]` **`T-13`**: Implementar `mjk_state_machine.c` para el ciclo central (`GameState 0..5, 9, B`): `Init`, `RoundIntro` (96f), `LoadRoom`, `DrawScreen`, `EnterRoom` (24f), `Main` (pausa `START`), `AnimateDoorOpen` y `ExitRoom` (`AdjacentRoomsTable` + `Entry ^ 4`) + `CTest`.
- `[DONE]` **`T-14`**: Implementar estados `RoundClear` (`0..3`, Time Bonus + Fire Bombs Bonus >= 20), `TimeOver` (128f), pérdida de vida/checkpoint (`LoseALifeMaybeGameOver`) y `GameOver` (256f + cálculo `PlayerGDV` y `HighGDV` -> Título) + `CTest`.
- `[DONE]` **`T-15`**: Implementar `TortureRoom` (mensaje 192f, sala `$AC`, 50 saltos BCD en US, retorno a checkpoint) y `GameState_C_Ending` (`EndingDoorChecks` en `$A0..$A3` con los **4 finales** según Crystal Balls y Secret Coins -> `GameOver`) + `CTest`.

## Hito 5 — Mecánicas de Gameplay Fiel por Subsistema

- `[DONE]` **`T-16`**: Implementar `mjk_player.c` (parte 1): cinemática Q8.8, tabla de salto de 64 pasos, salto alto/bajo (`Arriba`/`Abajo`), freno en aire, *flutter* y aceleración horizontal + `CTest`.
- `[DONE]` **`T-17`**: Implementar `mjk_player.c` (parte 2): matriz de colisiones 9-probe contra metatiles sólidos/parciales y muerte/invulnerabilidad + `CTest`.
- `[DONE]` **`T-18`**: Implementar `mjk_camera.c`: scroll horizontal en pasillos, costuras/seams entre salas de sección y clamping en fronteras + `CTest`.
- `[DONE]` **`T-19`**: Implementar `mjk_objects.c` (parte 1): bombas `100`/`200`, cadena de mechas con chispa, animación *puff* en 2 fases y lógica de Royal Palace / Bomb Room + `CTest`.
- `[DONE]` **`T-20`**: Implementar `mjk_objects.c` (parte 2): cofres rojos/naranjas, bloques secretos, ítems (Bags, Gold Coin, Mighty Drink, Sphinx, Extra 1UP, Bonus/Secret Coins, Crystal Balls) y niveles Mighty `0..3` con degradación por pulsaciones de `A` + `CTest`.
- `[DONE]` **`T-21`**: Implementar `mjk_doors.c`: evaluación de las **33 condiciones de puertas** USA (incluyendo caso especial sala `$61`), apertura en `RoomState` y detección de cruce por puerta + `CTest`.
- `[DONE]` **`T-22`**: Implementar `mjk_enemy.c` (parte 1): tabla de dificultad por ronda, spawns diferidos por sala (máx 4 vivos), patrulla de Momias y colisión letal con Jack + `CTest`.
- `[DONE]` **`T-23`**: Implementar `mjk_enemy.c` (parte 2): los 6 arquetipos mutados (Hanezo, GejiShogun, Gameido, Dokuron, Desufa, Horus), mutación `(APressCounter % 6) + 2` y Power Ball / Power Coin de 320 frames + `CTest`.
- `[DONE]` **`T-24`**: Implementar `mjk_hud.c`: aritmética BCD unificada little-endian (`$00A0..$00A3`), `StageTimer` (`$60` BCD, divisor 256f), sprites OAM del HUD (`Score` Y=16, `T`/`M` Y=24) y puntuación flotante + `CTest`.

## Hito 6 — Motor de Audio Chiptune APU 2A03

- `[DONE]` **`T-25`**: Implementar sintetizador hardware `mjk_apu_synth.c` (parte 1): osciladores Pulse 1/2 (4 duty cycles + hardware sweep `$4001/$4005`), Triangle 32 pasos y Noise LFSR de 15 bits a 44.1 kHz + `CTest`.
- `[DONE]` **`T-26`**: Implementar intérprete de bytecode BGM/SFX en `mjk_apu.c` (parte 2) 100% alimentado desde `MjkAssetPackage` (envolventes, pitch bend, prioridad SFX sobre música, 8 canales) y conectarlo a `MjkGame_StepAudio` y `main.c` + `CTest`.

## Hito 7 — Capa 2 Semántica, Juego Completo y Certificación Multiplataforma

- `[DONE]` **`T-27`**: Implementar `mjk_verify_api.c` (API limpia de inspección headless) + `tools/verify_scenarios.py` ejecutando micro-escenarios cortos (60–240 frames) frente a `cynes` (reglas jugables, OAM/paletas NES y eventos APU + PNG 3-paneles).
- `[DONE]` **`T-28`**: Auditoría *Full-Game Playthrough*: recorrer con `F2` (y jugando en vivo) todos los puntos del checklist de `research/GAME_COMPLETE_FLOW.md` (Copyright -> Title -> Pasajes -> Bomb Rooms -> RoundClear -> Torture Room -> 4 Finales -> GameOver -> Title) y corregir cualquier brecha de integración.
- `[DONE]` **`T-29`**: Pulido educativo y extensibilidad: verificar módulos < 600 líneas y comentarios didácticos con citas `asm/src/`, actualizar `research/CROSS_REFERENCE_INDEX.md` al nuevo código C, y añadir guía práctica de modding en `docs/EDUCATIONAL_ARCHITECTURE.md`.
- `[DONE]` **`T-30`**: Certificación final multiplataforma: compilar sin warnings (`-Wall -Wextra -Werror`) en Windows y Linux, pasar `ctest` al 100%, pasar `verify_scenarios.py` al 100% y validar ejecución limpia solo con `mbj-us.nes`.
