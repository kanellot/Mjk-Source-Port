# RES-026: Sistema de Feedback Visual de Puntuación Flotante

**Fecha**: 2026-10-25  
**Autor**: Nivel 2 Specialist Chain (`asm_researcher`, `c_implementer`, `emulator_verifier`)  
**Fuentes ASM**:
- `asm/src/prg.asm:2548-2573` (`sub_8FB1` score popup trigger)
- `asm/src/ram.asm:929-940` (`byte_30B..byte_310` variables OAM de popups de puntuación)
- `asm/src/prg.asm:4860-4965` (`MaybePickupItem`)

---

## 1. Objetivos y Alcance
Documentar e implementar el subsistema de feedback visual de puntuación flotante en *Mighty Bomb Jack*:
1. **Generación Dinámica de Marcadores**: Disparo automático ante eventos de recompensa (recolección de bomba +100/+200, apertura de cofre +100..+10000, captura de moneda transmutada +100..+8000, o combate letal Mighty Nivel 3 +500).
2. **Cinemática Ascendente**: Desplazamiento vertical ascendente uniforme (~1 píxel por frame) durante una vida útil de 30 frames.
3. **Desvanecimiento y Reciclado de Slots**: Desactivación inmediata al expirar el temporizador de 30 frames y liberación del slot en el búfer circular/estático `MJK_MAX_FLOATING_SCORES` (8 slots concurrentes).
4. **Renderizado con Fuentes Auténticas**: Trazado transparente de dígitos en capas superiores del PPU (`MjkPPU_BlitTile` con subpaleta de sprites).

---

## 2. Evidencia Empírica de Ensamblador 6502

### 2.1 Rutina Canónica `sub_8FB1` (`prg.asm:2551-2573`)
```assembly
sub_8FB1:
	CLC
	ADC #$30            ; Base ASCII / tile para dígitos en CHR
	STA byte_30B
	LDA #$30
	STA byte_30C
	LDX #$F             ; Duración / flags de visualización
	STX byte_30D
	LDY ScoreMultiplier
	BEQ loc_8FD3
	LDA a:RoomStatusFlags
	AND #RF_BombRoom
	BEQ loc_8FD3
	DEY
	TYA
	CLC
	ADC #$3C
	TAX

loc_8FD3:
	STX byte_30E
	RTS
```

---

## 3. Implementación Arquitectónica en C11 (`src/core/mjk_hud.c`)
- **Estructura `MjkFloatingScore`**:
  ```c
  typedef struct {
      int16_t  x;
      int16_t  y;
      uint16_t value;              ///< Valor numérico a representar
      uint8_t  timer;              ///< Temporizador (30 frames)
      bool     active;
  } MjkFloatingScore;
  ```
- **Funciones Principales**:
  - `MjkGame_SpawnFloatingScore(game, x, y, value)`: Asigna slot inactivo, fija `y` inicial y `timer = 30`.
  - `MjkGame_UpdateFloatingScores(game)`: Decrementa `y` en 1 píxel por frame, reduce `timer` y desactiva al llegar a 0.
  - `RenderInGameScreen`: Blit de dígitos individuales centrados horizontalmente sin interferir con el HUD.

---

## 4. Cobertura y Verificación
- Suite unitaria determinista: `tests/test_floating_scores.c` (TEST 16/16).
  - Subtest 1: Inicialización precisa de marcadores.
  - Subtest 2: Ascenso de 15 px en 15 frames y desaparición en el frame 30.
  - Subtest 3: Concurrencia de hasta 8 marcadores independientes sin desbordamiento.
  - Subtest 4: Validación de triggers de gameplay (bombas, cofres y combate).
- Estado: **16/16 tests pasando con 0 errores y compilación limpia de `bin/mjk.exe`**.
