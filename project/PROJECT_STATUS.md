# Estado del Proyecto: Mighty Bomb Jack — Source Port C11

**Última actualización**: 2026-11-23  
**Fase Activa**: **Hito 5 Completado (`T-16`..`T-24`) — Siguiente: Hito 6 (`T-25`, Motor APU 2A03)**  
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
| **Source Port C11 (`src/`)** | `[EN PROGRESO]` | **Hito 5 completado (`T-16`..`T-24`)** (Jack Q8.8, cámara, bombas/cofres, 33 puertas, 6 enemigos + Power Coin, HUD BCD + `14/14` CTest en verde). Quedan `T-25`..`T-30` (Hitos 6–7) en [`TASKS.md`](./TASKS.md). |

---

## 2. Siguiente Paso Inmediato

Ejecutar **`T-25`** (inicio de **Hito 6**) en [`project/TASKS.md`](./TASKS.md): implementar sintetizador hardware 2A03 `mjk_apu_synth.c` (Pulse 1/2, Triangle 32 pasos y Noise LFSR 15-bit a 44.1 kHz) + `CTest`.
