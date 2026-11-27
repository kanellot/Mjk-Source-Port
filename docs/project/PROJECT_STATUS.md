# Estado del Proyecto: Mighty Bomb Jack — Source Port C11 (Refactor Integral)

**Última actualización**: 2026-11-25  
**Rama Activa**: `refactor`  
**Fase Activa**: **COMPLETADO (`R-01`..`R-20`, 20/20 `[DONE]`)**  
**Versión Base Canónica**: **USA / NTSC (`REV_US`)**

---

## 1. Estado del Workspace

| Pilar | Estado | Situación Actual |
| :--- | :---: | :--- |
| **Motor Moderno (`src/` + `include/mjk/`)** | `[OK]` | 5 capas limpias (`assets/`, `game/`, `render/`, `audio/`, `platform/`). Enteros estándar, física `float` exacta, structs semánticos, cero código de debug/verify en producción y cero dependencias SDL/OS en `mjk_engine`. |
| **Optimizaciones de Rendimiento** | `[OK]` | 512 tiles `8×8` pre-decodificados en L1 (32 KB), caché plana de paletas ARGB8888, tablas LUT en el mezclador 2A03 (`< 1e-6`), acumulador fraccional NTSC (`733`/`734` muestras/frame), `MjkInputLatch` y cero `malloc`/`free` en partida. |
| **Verificación Unitaria (`tests/c/`)** | `[OK]` | `11/11` suites C11 en verde (`0` warnings MSVC `/W4 /WX`, `< 0.6 s`), Golden Vectors + Golden Frame Checks, `SKIP_RETURN_CODE 77` si falta ROM y `FAIL (1)` si ROM explícita inválida. |
| **Documentación (`docs/`)** | `[OK]` | Documentación humana unificada en `docs/` (`ARCHITECTURE.md`, `GAME_SYSTEMS.md`, `ASSETS_AND_AUDIO.md`, `TESTING_GUIDE.md`), referencias ASM en `docs/research/` y seguimiento del proyecto en `docs/project/`. |
| **Clean-Room BYOR** | `[OK]` | Cero ROMs ni bytes con copyright de Tecmo en el repositorio. |
