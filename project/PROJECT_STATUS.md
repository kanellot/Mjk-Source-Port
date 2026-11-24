# Estado del Proyecto: Mighty Bomb Jack — Source Port C11

**Última actualización**: 2026-11-24  
**Fase Activa**: **COMPLETADO (`T-01`..`T-30` + Paridad Bloques 1–5 + Release CI/CD)**  
**Versión Base Canónica**: **USA / NTSC (`REV_US`)**  
**Entorno Agéntico**: **Gemini 4 Neon** (Planificación/Diagnóstico) + **Gemini 4 Hydrogen** (Ejecución/Verificación)

---

## 1. Estado del Workspace

| Pilar | Estado | Situación Actual |
| :--- | :---: | :--- |
| **Limpieza del Workspace** | `[OK]` | Cero ROMs, assets binarios ni rutas locales en Git. Conservados localmente `Mighty-Bomb-Jack-original.mp4`, Manual PDF y `third_party/sdl2/` (excluidos en `.gitignore`). |
| **Ground Truth ASM (`asm/`)** | `[OK]` | Desensamblado 6502 completo e inmutable (`Xkeeper0/mighty-bomb-jack`, licencia MIT), excluido del repo principal y citado en `LICENSE` y `README.md`. |
| **Documentación (`research/`)** | `[OK]` | Depurada, con enlaces relativos portables y alineada a [`GAME_COMPLETE_FLOW.md`](../research/GAME_COMPLETE_FLOW.md), `verification_strategy.md` (3 Capas), `asset_extractor_spec.md` (BYOR v2 en C) y `source_port_architecture.md`. |
| **Entorno Agéntico (`.agents/`)** | `[OK]` | `AGENTS.md` (< 6 KB), 3 skills (`mjk-verification-workflow`, `mjk-asm-ground-truth`, `mjk-educational-c-port`) y hook `guard_workspace.py`. |
| **Oráculo NES (`tools/`)** | `[OK]` | `tools/asm_query.py` (consulta AST instantánea), `tools/nes_oracle.py` (oráculo autónomo `cynes`) y `tools/verify_scenarios.py` (Capa 2). |
| **Source Port C11 (`src/`)** | `[OK]` | **Proyecto completado (`30/30` tareas `[DONE]` + 5 bloques de pulido de paridad)**: 19 módulos C11 puros (`0` SDL/OS en `src/core/`), BYOR v2 integrado, PPU 2C02, 257 salas, gameplay completo `REV_US`, APU 2A03 8 canales y `18/18` CTests en verde. |
| **CI/CD & Publicación** | `[OK]` | Pipeline GitHub Actions ([`.github/workflows/release.yml`](../.github/workflows/release.yml)) para releases automáticas Windows x64 y Linux x64 en `main`, `README.md` y `LICENSE` (MIT + atribución ASM). |

---

## 2. Siguiente Paso Inmediato

Repositorio auditado y listo para publicar en GitHub o extender siguiendo [`README.md`](../README.md) y [`docs/EDUCATIONAL_ARCHITECTURE.md`](../docs/EDUCATIONAL_ARCHITECTURE.md).
