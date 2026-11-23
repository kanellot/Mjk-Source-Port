# Diario de Refinamiento Iterativo y Empuje de Horizonte (Fase 7)

**Archivo TAS de Referencia Activo**: `tests/tas/session_20261101_185107.tas`  
**Total de Fotogramas a Homologar**: 4.221 frames (~70 segundos de juego activo)  
**Meta**: 100% de paridad determinista en los 5 niveles de la pirámide  

---

## Barra de Progreso Global (Horizonte de Paridad)

```
[===================>-----------------------------------------------] 30.04%
Último frame homologado: Frame 1.267 / 4.221 (Nivel 1 Activo)
```

---

## Registro Cronológico de Iteraciones

### [Iteración #001] — Frame 181 / 4.221 (Superado hasta Frame 344)
* **Fecha y Hora**: 2026-11-01 19:04
* **Nivel de la Pirámide**: Nivel 1 (Macro-Flujo)
* **Discrepancia Previa**: `title_screen_state` (golden_nes: 2 vs port_c: 3)
* **Causa Raíz en ASM**:
  * `asm/src/prg.asm:7522-7570` (`TitleScreen_0` / `DrawFullScreen`): La rutina de dibujado completo de metatiles desactiva la NMI y consume ~208.000 ciclos (~8 frames de latencia de hardware) manteniendo `$0045 = 2`.
  * `asm/src/prg.asm:7644-7659`: La caída parabólica de Jack dura exactamente 73 fotogramas canónicos antes de transicionar a `in_game = 1` en el frame 344.
* **Archivos Modificados**: `src/core/mjk_game.c:1534-1583`.
* **Resultado del Oráculo**: **PASS** en los fotogramas 0 a 344 (+164 fotogramas avanzados, paridad 1:1 absoluta).
* **Siguiente Divergencia Detectada**: Frame 345 (`current_room`: NES=0 vs Port=1).
* **Commit Git**: `fix(macro): correccion de temporizadores de title screen y caida de jack (frame 181->344)`

---

### [Iteración #002] — Frame 345 / 4.221 (Superado hasta Frame 1.267)
* **Fecha y Hora**: 2026-11-01 19:28
* **Nivel de la Pirámide**: Nivel 1 (Macro-Flujo)
* **Discrepancia Previa**: `current_room` (`$03BF`, golden_nes: 0 vs port_c: 1) y desincronización de estados 0..4.
* **Causa Raíz en ASM**:
  * `asm/src/prg.asm:47-60`, `7582-7583`, `7650-7656`: Durante el boot y la secuencia de título (`TitleScreen_3`), `CurrentRoomID` (`$03BF`) vale `0`. Solo pasa a `1` cuando se ejecuta `GameState_0_Init` (`prg.asm:343-344`). En C estaba inicializado prematuramente a `1` en `MjkGame_Init`.
  * `asm/src/prg.asm:353-358`: `GameState_0_Init` establece `EntryDoorType = $0F` y `PlayerGDV = $47`.
  * `asm/src/prg.asm:7671-7690`: `GameState_1_RoundIntro` carga `$60` (96) y decrementa inmediatamente en el mismo frame (95 frames efectivos).
  * `asm/src/prg.asm:379-440`: `GameState_3_DrawScreen` consume 8 frames de transferencia PPU antes de pasar a `GameState_4_EnterRoom`.
  * `asm/src/prg.asm:445-623`: `GameState_4_EnterRoom` dura exactamente 167 frames (72f apertura puerta + 24f caminata de Jack + 72f cierre puerta) antes de iniciar `GameState_5_Main` en el frame 617.
* **Archivos Modificados**: `src/core/mjk_game.c:1378`, `src/core/mjk_state_machine.c:27-45`, `src/core/mjk_state_machine.c:440-502`.
* **Resultado del Oráculo**: **PASS** desde el Frame 0 hasta el Frame 1.267 (+923 fotogramas avanzados, **30.04%** de la partida homologada).
* **Siguiente Divergencia Detectada**: Frame 1.268 (`current_room`: NES=2 vs Port=1 — transición por puerta de la Sala 1 a la Sala 2).
* **Commit Git**: `fix(macro): paridad de inicializacion current_room y estados 0..4 (frame 345->1267)`


