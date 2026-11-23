# Estado del Proyecto: Mighty Bomb Jack — Source Port C11

**Última actualización**: 2026-11-23  
**Fase Activa**: **Reinicio Limpio Completado — Listo para Hito 1 (Infraestructura CMake + Esqueleto C11)**  
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
| **Source Port C11 (`src/`)** | `[PENDIENTE]` | Planificado en **30 micro-tareas cortas (`T-01`..`T-30`, 7 Hitos)** resilientes a cuota en [`TASKS.md`](./TASKS.md). |

---

## 2. Siguiente Paso Inmediato

Iniciar **Hito 1 (`T-01`)** en [`project/TASKS.md`](./TASKS.md) previa confirmación del usuario.
