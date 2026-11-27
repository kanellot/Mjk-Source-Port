# Mighty Bomb Jack — Constitución del Workspace (Gemini 4 Neon + Hydrogen)

> **Propósito**: Guía operativa concisa y regla raíz del proyecto (< 6 KB). Delega guías extensas a `.agents/skills/`.

---

## 1. Objetivo Final del Proyecto

Reescribir un **source port jugablemente muy fiel** de *Mighty Bomb Jack* (NES USA 1987, `REV_US`) en **C11 estándar**:
- **Juego completo de inicio a fin**: Desde Copyright/Title Screen hasta las 17 rondas (257 salas), Torture Room, Bomb Rooms y los 4 finales. Abstracciones C limpias en lugar de quirks de hardware (stalls PPU-off, `PLA;PLA`, flicker 8 sprites) o sincronía TAS ciclo-a-ciclo.
- **Multiplataforma y Educativo**: Windows y Linux (CMake + SDL2, `-Wall -Wextra`), módulos < 600 líneas explicando Q8.8, BCD, 4-tier rooms y 2A03, con diseño extensible.
- **Clean-Room BYOR Integrado en C**: Cero bytes con copyright de Tecmo en el repo ni en C. El ejecutable valida `mbj-us.nes` (`SHA-1: a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`) y extrae `assets/assets.dat` al arrancar, sin Python.

---

## 2. Mapa de Directorios y Permisos

| Directorio | Acceso | Contenido y Reglas |
| :--- | :---: | :--- |
| `asm/` | **SOLO LECTURA** | Desensamblado 6502 original (*Golden Ground Truth*). Protegido por hook determinista. |
| `src/` + `include/` | R/W | `src/core/` (C11 puro, **cero SDL y cero APIs de SO**) y `src/platform/` (HAL SDL2 + Debug Overlay). |
| `tests/c/` | R/W | Tests unitarios C11 deterministas con `CTest` (Capa 1). |
| `tools/` | R/W | Consulta ASM (`asm_query.py`, `asm_inventory.json`), oráculo NES (`nes_oracle.py`) y `verify_scenarios.py` (Capa 2). |
| `docs/` | R/W | Documentación técnica (`ARCHITECTURE.md`, `GAME_SYSTEMS.md`, `ASSETS_AND_AUDIO.md`, `TESTING_GUIDE.md`), `docs/research/` (referencias ASM) y `docs/project/` (`PROJECT_STATUS.md`, `TASKS.md`, `DECISIONS.md`). |
| `.agents/` | R/W | Skills progresivas bajo demanda y hook de protección. |
| `scratch/` | R/W | Capturas y fixtures temporales. **Prohibidos los scripts `.py`.** |
| `./` (raíz) | Restringido | Solo `AGENTS.md`, `CMakeLists.txt`/wrappers de build, manual PDF y vídeo original. |

---

## 3. Orquestación (Neon + Hydrogen) y Micro-Tareas Resilientes a Cuota

### Flujo por Defecto — Plan / Execute Nativo (`Orchestrator (Neon)`)
1. **Planificación (`Gemini 4 Neon`)**: Consulta `GAME_COMPLETE_FLOW.md`, `asm_query.py`, `docs/research/` y `nes_oracle.py`, y emite un plan acotado a **1 sola micro-tarea** de `docs/project/TASKS.md` vía `exit_plan_mode`.
2. **Ejecución (`Gemini 4 Hydrogen`)**: Implementa en C11 limpio (~150–350 líneas + test `CTest`), compila, verifica que `ctest` pasa al 100% y actualiza `CHECKPOINT ACTIVO` en `docs/project/TASKS.md`. Si algo falla dos veces, devuelve control con `enter_plan_mode`.

### Regla Anti-Agotamiento de Cuota (1 Micro-Tarea = 1 Estado Verde)
- Ejecutar **estrictamente 1 micro-tarea** por ciclo de trabajo para poder retomar en cualquier momento si se agota la cuota de tokens.
- Nunca cerrar una micro-tarea con la compilación rota o tests fallando; actualizar siempre `CHECKPOINT ACTIVO` al terminar.
- **Commit por Hito**: Al completar la última micro-tarea de cada Hito (`T-03`, `T-07`, `T-11`, `T-15`, `T-24`, `T-26`, `T-30`) con compilación y `ctest` al 100%, crear un commit limpio en la rama activa resumiendo el hito (sin binarios ni ROMs).

### Paralelismo (Solo si se divide limpiamente)
- Investigación multi-archivo solo lectura -> subagente `research` (`flash`) o `run_workflow`.
- Módulos disjuntos -> `swarm` con workers `gemini-4-hydrogen-medium` (`workspace: "share"`).

---

## 4. Verificación en 3 Capas

1. **Capa 1 (`CTest` C11)**: Gate rápido (< 1 s) sobre contratos puros, BYOR v2 y 257 salas.
2. **Capa 2 (Micro-escenarios cortos, 60–240 frames)**: `nes_oracle.py` (`cynes`) + `verify_scenarios.py` validan reglas jugables, OAM/paletas y eventos APU sin efecto mariposa.
3. **Capa 3 (Debug Overlay SDL2)**: Teclas `F1`–`F4` para hitboxes, warp a cualquier sala/final y juego real.

---

## 5. Reglas Invariantes (Cero Excepciones)

1. **Visión global primero**: Antes de implementar pantallas, estados o transiciones, consultar [`docs/research/GAME_COMPLETE_FLOW.md`](docs/research/GAME_COMPLETE_FLOW.md) (y usar manual PDF / vídeo original solo para flujo/experiencia global, confirmando constantes siempre en `asm/src/`).
2. **Evidencia antes que suposiciones**: Toda constante, fórmula o tabla debe tener estado `[CONFIRMADA]` en `asm/src/<archivo>:<líneas>` o traza de `nes_oracle.py`.
3. **Orden de consulta ahorrador de tokens**: 1) `python tools/asm_query.py ...` -> 2) `docs/research/GAME_COMPLETE_FLOW.md` / `CROSS_REFERENCE_INDEX.md` -> 3) `docs/research/contracts/` -> 4) `RES-012..027` -> 5) `read_file` acotado sobre `asm/src/`. Prohibidos greps ciegos.
4. **Prohibido `scratch/*.py`**: Usar `tools/nes_oracle.py`, `tools/asm_query.py` y `ctest`. (Bloqueado por hook.)
5. **Reanudación quirúrgica**: Al retomar tras pausa o fin de cuota, leer únicamente `docs/project/TASKS.md` (`CHECKPOINT ACTIVO`) y `docs/project/PROJECT_STATUS.md`.
6. **Clean-Room BYOR**: Cero `.nes`, `.dat`, `.chr` en Git ni arrays de Tecmo en C.

---

## 6. Skills (`.agents/skills/`)

- **`mjk-verification-workflow`**: Verificación en 3 capas (`CTest`, `nes_oracle.py`/escenarios, Overlay `F1`–`F4`).
- **`mjk-asm-ground-truth`**: Consulta ASM con `asm_query.py` y epistemología de 4 estados (`REV_US`).
- **`mjk-educational-c-port`**: Estándar C11 educativo, abstracciones limpias, Windows+Linux y BYOR integrado en C.
