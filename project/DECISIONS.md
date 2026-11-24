# Registro de Decisiones de Arquitectura e Ingeniería (ADR)

Este registro documenta las decisiones arquitectónicas, metodológicas e invariantes técnicas vigentes del source port C11 de *Mighty Bomb Jack* (`REV_US`), desarrollado con **Gemini 4 Neon + Hydrogen**.

---

## Índice de Decisiones

- [ADR-001: Epistemología de Evidencia en 4 Estados y Ground Truth 6502](#adr-001-epistemología-de-evidencia-en-4-estados-y-ground-truth-6502)
- [ADR-002: Revisión USA / NTSC (`REV_US`) como Versión Base Canónica](#adr-002-revisión-usa--ntsc-rev_us-como-versión-base-canónica)
- [ADR-003: Consulta ASM Determinista (`tools/asm_query.py`) y Capitalización en `research/`](#adr-003-consulta-asm-determinista-toolsasm_querypy-y-capitalización-en-research)
- [ADR-004: Orquestación Agéntica con Gemini 4 Neon + Hydrogen, Skills Progresivas y Hook de Protección](#adr-004-orquestación-agéntica-con-gemini-4-neon--hydrogen-skills-progresivas-y-hook-de-protección)
- [ADR-005: Arquitectura C11 Clean-Room Desacoplada en 2 Capas (Windows + Linux)](#adr-005-arquitectura-c11-clean-room-desacoplada-en-2-capas-windows--linux)
- [ADR-006: Política Clean-Room BYOR v2 con Extractor Integrado en C11](#adr-006-política-clean-room-byor-v2-con-extractor-integrado-en-c11)
- [ADR-007: Fidelidad de Paletas 2C02, Sprites OAM y Audio 2A03 Derivada del ASM](#adr-007-fidelidad-de-paletas-2c02-sprites-oam-y-audio-2a03-derivada-del-asm)
- [ADR-008: Abstracciones Educativas Limpias en C11 (`Q8.8`, `BCD`, `MjkActionResult`)](#adr-008-abstracciones-educativas-limpias-en-c11-q88-bcd-mjkactionresult)
- [ADR-009: Sistema de Puertas `REV_US`, 33 Condiciones y Topología de 257 Salas](#adr-009-sistema-de-puertas-rev_us-33-condiciones-y-topología-de-257-salas)
- [ADR-010: Verificación Comportamental en 3 Capas (`CTest` + `cynes` + Debug Overlay)](#adr-010-verificación-comportamental-en-3-capas-ctest--cynes--debug-overlay)
- [ADR-011: Pipeline CI/CD Multiplataforma en GitHub Actions (Windows y Linux)](#adr-011-pipeline-cicd-multiplataforma-en-github-actions-windows-y-linux)

---

## ADR-001: Epistemología de Evidencia en 4 Estados y Ground Truth 6502

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Para construir un port jugablemente fiel sin inventar constantes, hitboxes ni temporizaciones, toda decisión de diseño debe partir del desensamblado 6502 original (`asm/src/`) y/o telemetría verificable del emulador (`cynes`).
- **Decisión**:
  1. Toda afirmación técnica se clasifica en uno de 4 estados: `[CONFIRMADA]` (cita directa `asm/src/<archivo>:<líneas>` o traza de `nes_oracle.py`), `[INFERIDA]`, `[DESCONOCIDA]` o `[CONTRADICTORIA]`.
  2. Queda prohibido adoptar constantes, fórmulas o tablas basadas en suposiciones no verificadas.
  3. El directorio `asm/` es **solo lectura** (protegido de forma determinista).
- **Consecuencias**:
  Eliminación de placeholders y garantía de trazabilidad directa entre el código C11 y la ROM original.

---

## ADR-002: Revisión USA / NTSC (`REV_US`) como Versión Base Canónica

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  *Mighty Bomb Jack* cuenta con revisiones `REV_JP` (Famicom 1986), `REV_JPA` y `REV_US` (NES USA 1987).
- **Decisión**:
  Adoptar `REV_US` (`mbj-us.nes`, `65 552` bytes, `SHA-1: a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`) como única línea canónica:
  - Textos en inglés y pantalla de Copyright USA (`TitleScreen_US_0/1`).
  - Penalización de la Sala de Tortura fijada en `50` saltos (`$50` BCD).
  - Disposición de puertas, cofres y secretos específicos de `REV_US` (incluyendo las 33 condiciones de puerta y el caso especial de la sala `$61`).
- **Consecuencias**:
  Contratos unívocos y verificación determinista contra una única ROM de referencia.

---

## ADR-003: Consulta ASM Determinista (`tools/asm_query.py`) y Capitalización en `research/`

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Releer archivos ensamblador de miles de líneas en cada turno desperdicia contexto y favorece inconsistencias.
- **Decisión**:
  1. Indexar subrutinas, tablas ROM, JumpTables y uso de RAM en `tools/asm_inventory.json` consultable instantáneamente mediante `tools/asm_query.py`.
  2. Documentar todo hallazgo en `research/` (`GAME_COMPLETE_FLOW.md`, `CROSS_REFERENCE_INDEX.md`, `contracts/`, `inventories/`, `control_flow/`, `RES-012..036`) con contraste final contra `asm/src/`.
- **Consecuencias**:
  Mínimo consumo de tokens, conocimiento persistente navegable y rigor matemático en la cobertura del código 6502.

---

## ADR-004: Orquestación Agéntica con Gemini 4 Neon + Hydrogen, Skills Progresivas y Hook de Protección

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  El proyecto requiere mantener reglas claras (< 6 KB en `AGENTS.md`), ejecución modular resiliente a cuota (1 micro-tarea = 1 estado verde) y protección automática de invariantes.
- **Decisión**:
  1. Mantener `AGENTS.md` conciso como constitución raíz del workspace.
  2. Usar el flujo nativo **Plan/Execute** con **Gemini 4 Neon** (planificación, diagnóstico ASM/oráculo) y **Gemini 4 Hydrogen** (implementación C11 y verificación `CTest`).
  3. Externalizar guías extensas en 3 skills bajo demanda (`.agents/skills/mjk-asm-ground-truth`, `mjk-educational-c-port`, `mjk-verification-workflow`).
  4. Instalar el hook determinista `PreToolUse` (`.agents/hooks.json` + `.agents/hooks/guard_workspace.py`) que bloquea escrituras en `asm/` y scripts temporales `scratch/*.py`.
- **Consecuencias**:
  Ciclos de trabajo predecibles, cero truncamiento de contexto y cumplimiento automático de las reglas del repositorio.

---

## ADR-005: Arquitectura C11 Clean-Room Desacoplada en 2 Capas (Windows + Linux)

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  El port debe compilar sin warnings en Windows (MSVC) y Linux (GCC/Clang), servir como referencia educativa y ser fácil de mantener y extender.
- **Decisión**:
  1. **Lenguaje**: C11 estricto (`-std=c11`, `-Wall -Wextra -Wpedantic -Werror` en GCC/Clang; `/W4 /WX /utf-8` en MSVC).
  2. **Separación estricta en 2 capas**:
     - `src/core/`: 19 módulos puros de simulación (cero includes de SDL2 y cero APIs Win32/POSIX), cada uno acotado y enfocado a un subsistema.
     - `src/platform/` + `src/main.c`: HAL SDL2 (ventana 256×240 escalable, audio PCM 44.1 kHz, teclado/mando con hotplug, bucle NTSC `60.0988 Hz` y Debug Overlay `F1`–`F6`).
  3. **Build unificado**: `CMakeLists.txt` con `CTest` integrado y wrappers finos (`build.bat` en Windows, `Makefile` en Linux).
- **Consecuencias**:
  Portabilidad nativa Windows/Linux, código didáctico y facilidad para compilar el núcleo en modo headless para pruebas.

---

## ADR-006: Política Clean-Room BYOR v2 con Extractor Integrado en C11

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  El repositorio debe estar 100 % libre de bytes con copyright de Tecmo (gráficos, música, textos, mapas) y el usuario final no debe depender de Python para arrancar el juego.
- **Decisión**:
  1. Integrar en `src/core/` el validador criptográfico (`mjk_sha1.c`) y el extractor (`mjk_rom_extractor.c`).
  2. Al arrancar, `mjk` carga `assets/assets.dat` v2 (con verificación **CRC-32**). Si no existe, localiza `mbj-us.nes` (o la ruta `--rom`), verifica cabecera iNES (`65 552` B) y **SHA-1**, y extrae automáticamente `CHR-ROM`, tablas/bytecodes APU, 32 cadenas PPU, 257 salas, paletas y tablas de juego.
  3. Los módulos del núcleo acceden a los datos por dirección CPU original mediante `MjkAssets_ResolveCpuPtr`.
- **Consecuencias**:
  Distribución 100 % Clean-Room autocontenida en Windows y Linux, sin scripts externos ni assets propietarios en Git.

---

## ADR-007: Fidelidad de Paletas 2C02, Sprites OAM y Audio 2A03 Derivada del ASM

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Adivinar colores RGB, sprites o efectos sonoros a ojo introduce desviaciones y artefactos respecto al hardware NES.
- **Decisión**:
  1. **Vídeo (`mjk_ppu.c`)**: paleta maestra NTSC 2C02 de 64 colores, decodificación planar 2bpp desde `CHR-ROM` y atributos OAM (paleta + flip H/V) directamente de `SpritesTable` y `SpriteAnimationTable`.
  2. **Audio (`mjk_apu_synth.c` + `mjk_apu.c`)**: síntesis hardware 2A03 a `44.1 kHz` (Pulse 1/2 con sweep, Triangle 32 pasos y Noise LFSR) controlada por el intérprete de bytecode original de 8 canales (4 BGM + 4 SFX).
- **Consecuencias**:
  Estética y sonido fieles a la NES de 1987 sin tablas aproximadas.

---

## ADR-008: Abstracciones Educativas Limpias en C11 (`Q8.8`, `BCD`, `MjkActionResult`)

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  El port busca una experiencia jugable fiel y legible, evitando reproducir hacks de hardware 6502 que oscurecen el código sin aportar valor jugable.
- **Decisión**:
  1. Expresar física en punto fijo `Q8.8` (`mjk_q8_8_t` / `mjk_uq8_8_t`) y marcadores en `BCD` little-endian (`MjkBcd_*`) mediante helpers seguros sin UB en `include/mjk_types.h`.
  2. Reemplazar los saltos destructivos de pila `PLA; PLA` por el retorno explícito `MjkActionResult`.
  3. Omitir quirks puramente de hardware sin beneficio jugable (límite artificial de 8 sprites/scanline con flicker, stalls de PPU apagada).
  4. Documentar el diseño en [`docs/EDUCATIONAL_ARCHITECTURE.md`](../docs/EDUCATIONAL_ARCHITECTURE.md).
- **Consecuencias**:
  Código claro para estudiar arquitectura de videojuegos retro y base limpia para mods o extensiones.

---

## ADR-009: Sistema de Puertas `REV_US`, 33 Condiciones y Topología de 257 Salas

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  La progresión por las 17 rondas y 257 salas depende de la decodificación 4-tier (`RoomHalfScreens` → `HalfScreenLayouts` → `LayoutChunks` → `MetatileDefinitions`), las 33 condiciones de puerta y el grafo de adyacencias.
- **Decisión**:
  1. Preservar el caso especial de la sala `$61` (`prg.asm:6757`, condición forzada a `0x19`).
  2. Evaluar las 33 condiciones USA en `src/core/mjk_doors.c` y mutar los 4 metatiles de puerta en `MjkRoomState` y `MjkRoomPersist`.
  3. Resolver transiciones mediante `AdjacentRoomsTable` e inversión de orientación `EntryDoorType = ExitDoorType ^ 4`.
- **Consecuencias**:
  Navegación completa y determinista desde la Sala `0x01` hasta los 4 finales del juego.

---

## ADR-010: Verificación Comportamental en 3 Capas (`CTest` + `cynes` + Debug Overlay)

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Validar un port completo requiere pruebas rápidas de regresión, contraste objetivo frente a la ROM original y diagnóstico interactivo en vivo.
- **Decisión**:
  Adoptar 3 capas desacopladas (detalladas en [`research/architecture/verification_strategy.md`](../research/architecture/verification_strategy.md)):
  1. **Capa 1 (`CTest` C11)**: 18 ejecutables unitarios deterministas (< 1.5 s) sobre contratos puros, BYOR v2, 257 salas y flujo completo.
  2. **Capa 2 (Micro-escenarios semánticos, 60–240 frames)**: `tools/nes_oracle.py` (`cynes`) + `tools/verify_scenarios.py` vía `mjk_verify_api.c`.
  3. **Capa 3 (Debug Overlay SDL2)**: `F1` telemetría/hitboxes 9-probe, `F2`/`[`/`]`/`F6` warp a salas `0..256`, `F3` cheats, `F4`/`F5` pausa y frame-step.
- **Consecuencias**:
  Detección rápida de regresiones, validación directa contra la ROM real y depuración visual en vivo.

---

## ADR-011: Pipeline CI/CD Multiplataforma en GitHub Actions (Windows y Linux)

- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Los usuarios deben poder descargar ejecutables listos para usar en Windows y Linux sin compilar manualmente, manteniendo la política Clean-Room BYOR.
- **Decisión**:
  Configurar [`.github/workflows/release.yml`](../.github/workflows/release.yml) disparado en `push` a `main` (y `workflow_dispatch`) para:
  1. Compilar en `Release` y ejecutar `CTest` en `windows-latest` (MSVC + SDL2 2.30.8 cacheado) y `ubuntu-latest` (GCC + `libsdl2-dev` + Ninja).
  2. Empaquetar `mighty-bomb-jack-<tag>-windows-x64.zip` (`mjk.exe` + `SDL2.dll` + `README.txt`) y `mighty-bomb-jack-<tag>-linux-x64.tar.gz` (`mjk` + `README.txt`), más `SHA256SUMS.txt`.
  3. Publicar automáticamente la GitHub Release con aviso BYOR.
- **Consecuencias**:
  Releases reproducibles, verificadas por `CTest` y 100 % libres de assets con copyright.
