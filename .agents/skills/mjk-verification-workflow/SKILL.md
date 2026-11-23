---
name: mjk-verification-workflow
description: >-
  Run the 3-layer behavioral verification workflow (C11 CTest unit tests, short
  semantic micro-scenarios against cynes via nes_oracle.py, and SDL2 Debug
  Overlay) for Mighty Bomb Jack. Activate when verifying C modules, diagnosing
  gameplay or rendering behavior, or exporting golden NES reference fixtures.
---

# Flujo de Verificación Comportamental en 3 Capas

## 1. Principio Rector (Cero Efecto Mariposa)

El objetivo es **fidelidad jugable, visual y sonora** con código C limpio y educativo, **no** emulación ciclo-a-ciclo ni sincronización rígida de TAS largos. Por ello:
- Prohibido introducir *stalls* artificiales o hacks solo para cuadrar relojes frame-a-frame.
- Cuando algo falle, contrasta la regla de juego real mediante `tools/asm_query.py` o un micro-escenario corto con `tools/nes_oracle.py`, y corrige la causa raíz limpiamente.

---

## 2. Las 3 Capas de Verificación

### Capa 1 — Tests Unitarios C11 (`CTest`) *(gate diario, < 1 s)*
```powershell
ctest --test-dir build --output-on-failure
```
Valida de forma determinista y sin dependencias:
- Extractor BYOR v2 y CRC32/SHA-1 contra `mbj-us.nes`.
- Decodificación completa de las **257 salas** (metatiles, puertas, bombas, cofres y paletas) frente a fixtures exportados de la ROM.
- Curva de salto de 64 pasos, *flutter*, colisiones 9-probe, 33 condiciones de puertas, cadena de mechas, mutación `(A % 6) + 2`, aritmética BCD, 50 saltos de Torture Room, GDV/4 finales y decodificador de bytecode APU.

### Capa 2 — Oráculo NES y Micro-Escenarios Semánticos (60–240 frames)
Usa [`tools/nes_oracle.py`](../../../tools/nes_oracle.py) sobre la ROM original (`asm/bin/mbj-us.nes`) para inspeccionar RAM/OAM o generar fixtures de referencia:
```powershell
# Inspeccionar RAM y OAM en una secuencia corta de botones
python tools/nes_oracle.py --script "270:START@2" --frames 360 --watch-ram 0x43,0x44,0x45,0x3BF --at-frame 350 --dump-oam

# Exportar snapshot dorado JSON y captura PNG de referencia
python tools/nes_oracle.py --script "270:START@2,620:RIGHT+A@20" --frames 660 --at-frame 640 --dump-state scratch/ref_jump.json --capture-png scratch/ref_jump.png
```
Y a partir del **Hito 6**, ejecuta la batería de micro-escenarios cortos con `tools/verify_scenarios.py`, que compara **resultados jugables y estado PPU/APU semántico** (altura de salto, estado de puertas/bombas, conteo de enemigos, puntuación, OAM/paletas NES e IDs de música/SFX) y genera diffs 3-paneles cuando hace falta.

### Capa 3 — Debug Overlay SDL2 (`F1`–`F4`) *(verificación humana inmediata)*
En builds de desarrollo del ejecutable `mjk`:
- **`F1`**: Mostrar/ocultar telemetría en vivo (sala, coordenadas, hitboxes 9-probe, enemigos, audio).
- **`F2`**: Selector/warp directo a cualquiera de las 257 salas, Bomb Room, Torture Room o Finales.
- **`F3`**: Modificadores de prueba (invencibilidad, nivel Mighty `0..3`).
- **`F4`**: Pausa y avance paso a paso frame a frame.
