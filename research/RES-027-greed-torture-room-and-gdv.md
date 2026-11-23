# RES-027: Mecánica de Codicia, Sala de Tortura (50 Saltos) y Sistema GDV

**Fecha**: 2026-10-25  
**Autor**: Nivel 2 Specialist Chain (`asm_researcher`, `c_implementer`, `emulator_verifier`)  
**Fuentes ASM**:
- `asm/src/prg.asm:698-772` (`GameState_7_GoToTortureRoom`, `loc_84A5`, `GreedyJumpsRemaining`)
- `asm/src/prg.asm:1040-1117` (`State_A_GameOver`, `AddAToGDV`, `BCDCheckFirstDigit`)
- `asm/src/prg.asm:7377-7425` (`EndingDoorChecks`, `_EndGame`)
- `asm/src/ram.asm:925-945` (`TortureRoomSceneFlag`, `TortureRoomMessageTimer`, `GreedyJumpsRemaining`)

---

## 1. Objetivos y Alcance
Especificar e implementar con paridad matemática e histórica 1:1 las mecánicas de:
1. **Detección de Codicia (Greed Trigger)**: Activación por saltos redundantes excesivos ($\ge 35$ saltos) o por recolectar Mighty Drink cuando el temporizador de sala supera $100$ segundos. Confiscación total de monedas Mighty.
2. **Sala de Tortura ($AC)**: Secuencia canónica de mensaje inicial de 192 frames (`YOU ARE GREEDY` / `GO TO THE TORTURE ROOM`), transporte a la cámara de penitencia `$AC`, y conteo regresivo estricto de **50 saltos obligatorios** en la versión US (`0x50` BCD). Retorno a la sala original de checkpoint.
3. **Cálculo de GDV (Game Display Value)**: Fórmula BCD oficial de puntuación al finalizar la partida o en Game Over, que determina el final obtenido (0 a 3).

---

## 2. Evidencia Empírica de Ensamblador 6502

### 2.1 Entrada y Escape de la Sala de Tortura (`prg.asm:698-765`)
```assembly
GameState_7_GoToTortureRoom:
	LDX TortureRoomSceneFlag
	BNE loc_849C
	INX
	STX TortureRoomSceneFlag
	JSR ClearAllSprites
	JSR ClearNametable
	LDA #Strings_YouAreGreedy
	JSR WriteStringToPPU		; greedy
	LDA #Strings_GoToTheTortureRoom
	JSR WriteStringToPPU		; torture
	LDA #$C0
	STA TortureRoomMessageTimer  ; 192 frames
...
loc_84A5:
	LDA #$AC
	STA CurrentRoomID		    ; -> Sala $AC (Torture room)
IFDEF REV_US
	LDA #$50                    ; 50 saltos en versión US
ELSE
	LDA #$10                    ; 10 saltos en JP
ENDIF
	STA GreedyJumpsRemaining
```

### 2.2 Cálculo del GDV en Game Over (`prg.asm:1050-1070`)
```assembly
	LDA LastBombRoomCleared
	AND #$F
	LSR A                       ; 1 punto cada 2 salas superadas (máx 7)
	JSR AddAToGDV
	LDA ExtraLivesFound
	LSR A                       ; 1 punto cada 2 vidas extras (1UP)
	JSR AddAToGDV
	LDA a:PlayerScore+2
	...
	LSR A; LSR A; LSR A         ; Bonus de puntuación (dividir por 200.000 pts -> 0 a 5)
	JSR AddAToGDV
```

### 2.3 Bonificación por Final y Selección de Final (`prg.asm:7377-7425`)
```assembly
	TXA                         ; X = room id (A0~A3)
	AND #3
	STA EndingType			    ; 0-3 (peor, bueno, genial, mejor)
...
_EndGame:
	LDA #3
	JSR AddAToGDV			    ; +3 puntos por alcanzar el final
```

---

## 3. Implementación en C99
- `MjkGame_AddAToGDV(current_gdv, points)`: Adición decimal BCD estricta de 2 dígitos.
- `MjkGame_CalculateFinalGDV(game, ending_type)`: Cálculo acumulativo según rondas superadas, vidas conseguidas, rango de puntuación y final.
- `MjkGame_EvaluateEnding(game)`:
  - Final 0 (Peor final por defecto).
  - Final 1 (Bueno): Requiere $\ge 2$ Crystal Balls.
  - Final 2 (Gran final): Requiere $\ge 5$ Secret Coins.
  - Final 3 (Mejor final): Requiere $\ge 5$ Secret Coins Y $\ge 2$ Crystal Balls.
- `MjkGame_TriggerTortureRoom(game)`: Guarda `checkpoint_room_id`, confisca `mighty_coins = 0` e inicializa 50 saltos.

---

## 4. Cobertura y Verificación
- Suite unitaria determinista: `tests/test_torture_room_gdv.c` (TEST 17/17).
  - Subtest 1: Aritmética decimal BCD de GDV (`AddAToGDV`).
  - Subtest 2: Disparador de codicia, confiscación y checkpoint.
  - Subtest 3: Secuencia de Sala de Tortura, conteo de 50 saltos y retorno.
  - Subtest 4: Evaluación de los 4 tipos de finales.
  - Subtest 5: Cálculo completo de GDV final y High GDV.
- Estado: **17/17 tests pasando al 100% con 0 errores y compilación limpia de `bin/mjk.exe`**.
