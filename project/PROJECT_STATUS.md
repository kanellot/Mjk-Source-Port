# Estado del Proyecto: Mighty Bomb Jack — Source Port C11

**Última actualización**: 2026-11-23  
**Fase Activa**: **Hito 6 Completado (`T-25`, `T-26`) — Siguiente: Hito 7 (`T-27`..`T-30`, Capa 2 Semántica y Certificación)**  
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
| **Source Port C11 (`src/`)** | `[EN PROGRESO]` | **Hito 6 completado (`T-25`..`T-26`)** (APU 2A03 44.1 kHz + bytecode VM 8 canales BGM/SFX + `MjkGame_StepAudio` + `15/15` CTest en verde). Quedan `T-27`..`T-30` (Hito 7) en [`TASKS.md`](./TASKS.md). |

---

## 2. Siguiente Paso Inmediato

Ejecutar **`T-27`** (inicio de **Hito 7**) en [`project/TASKS.md`](./TASKS.md): implementar `mjk_verify_api.c` + `tools/verify_scenarios.py` para Capa 2 semántica frente a `cynes` (incluyendo los fixes de auditoría `#30`).
