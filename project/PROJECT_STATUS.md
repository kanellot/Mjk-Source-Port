# Estado del Proyecto: Mighty Bomb Jack — Source Port C11

**Última actualización**: 2026-11-23  
**Fase Activa**: **Hito 4 Completado (`T-12`..`T-15`) — Siguiente: Hito 5 (`T-16`, Mecánicas de Gameplay)**  
**Versión Base Canónica**: **USA / NTSC (`REV_US`)**  
**Entorno Agéntico**: **Gemini 4 Neon** (Planificación/Diagnóstico) + **Gemini 4 Hydrogen** (Ejecución/Verificación)

---

## 1. Estado del Workspace

| Pilar | Estado | Situación Actual |
| :--- | :---: | :--- |
| **Limpieza del Workspace** | `[OK]` | Eliminados `archive/` (134 MB), `bin/` antiguo, vídeos viejos de bugs (65 MB), `assets/assets.dat`, sesiones `.tas` y scripts Python rígidos. Conservados `Mighty-Bomb-Jack-original.mp4`, Manual PDF y `third_party/sdl2/`. |
| **Ground Truth ASM (`asm/`)** | `[OK]` | Desensamblado 6502 completo e inmutable (`asm/bin/mbj-us.nes` presente). |
| **Documentación (`research/`)** | `[OK]` | Depurada y alineada a [`GAME_COMPLETE_FLOW.md`](../research/GAME_COMPLETE_FLOW.md), `verification_strategy.md` (3 Capas), `asset_extractor_spec.md` (BYOR v2 en C) y `source_port_architecture.md` (Clean-Slate). |
| **Entorno Agéntico (`.agents/`)** | `[OK]` | `AGENTS.md` (< 6 KB), 3 skills (`mjk-verification-workflow`, `mjk-asm-ground-truth`, `mjk-educational-c-port`) y hook `guard_workspace.py`. |
| **Oráculo NES (`tools/`)** | `[OK]` | `tools/asm_query.py` (consulta AST instantánea) + `tools/nes_oracle.py` (oráculo autónomo `cynes` para fixtures RAM/OAM y PNG). |
| **Source Port C11 (`src/`)** | `[EN PROGRESO]` | **Hito 4 completado (`T-12`..`T-15`)** (`mjk_title.c`, `mjk_state_machine.c` `GameState 0..C` + `8/8` CTest en verde). Quedan `T-16`..`T-30` (Hitos 5–7) en [`TASKS.md`](./TASKS.md). |

---

## 2. Siguiente Paso Inmediato

Ejecutar **`T-16`** (inicio de **Hito 5**) en [`project/TASKS.md`](./TASKS.md): implementar `mjk_player.c` (parte 1: cinemática Q8.8, tabla de salto de 64 pasos, salto alto/bajo, flutter y movimiento horizontal) + `CTest`.
