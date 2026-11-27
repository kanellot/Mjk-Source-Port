---
name: mjk-asm-ground-truth
description: >-
  Query 6502 assembly subroutines, RAM variables, JumpTables, and ROM data tables
  from Mighty Bomb Jack (REV_US) with minimal token cost, enforcing the 4-state
  epistemology and zero-assumption rules. Activate when mapping NES RAM addresses,
  verifying game mechanics against asm/src/, or updating research documentation.
---

# Ground Truth 6502 y Consulta ASM de Bajo Coste

## 1. Versión Canónica y Regla de Verdad

- **Revisión canónica**: **USA / NTSC (`REV_US`)** (`asm/bin/mbj-us.nes`, 257 salas, Torture Room = 50 saltos BCD).
- **Fuente de verdad absoluta**: `asm/src/` (`prg.asm`, `data.asm`, `ram.asm`, `defs.asm`, `macros.asm`, `roomdata-us.asm`, `music-engine.asm`, `music-data.asm`). **Solo lectura inmutable.**

---

## 2. Epistemología de 4 Estados

Toda constante, dirección RAM, metatile, timer o fórmula usada en diseño o código debe clasificarse mental y documentalmente:
1. **`[CONFIRMADA]`**: Respaldo directo en `asm/src/<archivo>:<líneas>` o traza verificada de `cynes`. **Único estado válido para codificar en `src/`.**
2. **`[INFERIDA]`**: Hipótesis de trabajo coherente aún no verificada en 6502/`cynes`. **Prohibido implementarla como verdad.**
3. **`[DESCONOCIDA]`**: Sin evidencia suficiente.
4. **`[CONTRADICTORIA]`**: Conflicto entre docs, C y ASM/emulador; requiere comprobación directa en `asm/src/`.

---

## 3. Orden Obligatorio de Consulta (Mínimo Consumo de Tokens)

1. **Herramienta determinista (`tools/asm_query.py`)**:
   ```powershell
   # Consultar variable RAM (dirección + lectores/escritores)
   python tools/asm_query.py --ram 0x00A4
   python tools/asm_query.py --ram StageTimer

   # Consultar subrutina (archivo, rango de líneas, callers, callees, RAM R/W)
   python tools/asm_query.py --sub GameState_5_Main --snippet

   # Consultar JumpTable o tabla de datos ROM
   python tools/asm_query.py --jt GameHandler
   python tools/asm_query.py --table DifficultyTable --snippet

   # Buscar equivalencias ASM <-> C
   python tools/asm_query.py --xref StageTimer
   ```
2. **Mapa ASM <-> C**: `docs/nes-reference/asm-to-c-map.md`.
3. **Especificación por subsistema**: `docs/nes-reference/` (`game-flow.md`, `rom-and-memory-map.md`, `graphics-and-ppu.md`, `audio-2a03.md`, `physics-and-collision.md`, `world-and-rooms.md`, `gameplay-mechanics.md`) e inventarios en `docs/nes-reference/inventories/`.
4. **Lectura directa en `asm/src/`**: Usar `read_file` con `StartLine` y `EndLine` exactos obtenidos de los pasos anteriores (< 150 líneas por lectura).

---

## 4. Capitalización (No Repetir Trabajo)

Si al investigar se descubre un mapeo nuevo o se corrige una dirección RAM errónea:
- Actualizar `docs/nes-reference/asm-to-c-map.md` (y el documento de subsistema correspondiente) con la cita exacta `asm/src/<archivo>:<líneas>`.
- No crear informes `RES-###` duplicados si el dato cabe en `CROSS_REFERENCE_INDEX.md` o en el contrato correspondiente.
