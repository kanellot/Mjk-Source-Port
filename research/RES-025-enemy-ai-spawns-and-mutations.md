# RES-025: Spawning Diferido de Enemigos, IA de Momias, Mutación Temporal y Colisiones

**Fecha**: 2026-10-25  
**Autor**: Nivel 2 Specialist Chain (`asm_researcher`, `c_implementer`, `emulator_verifier`)  
**Fuentes ASM**:
- `asm/src/prg.asm:8150-8450` (`HandleEnemySpawnTimer`, `_SpawnEnemy`, `InitEnemy`, `RunEnemyInit`)
- `asm/src/prg.asm:9208-9575` (`ObjInit_0_Mummy`, `ObjCode_0_Mummy`, `TransformMummy`)
- `asm/src/prg.asm:8995-9035` (`sub_BA00` slot allocation, `TurnEnemiesIntoCoins`)
- `asm/src/data.asm:116-141` (`DifficultyTable`)
- `asm/src/data.asm:1963-2000` (`EnemySpawnPositionTable`)
- `asm/src/ram.asm:171, 386, 1057-1060` (`EnemyCoinTimer`, `APressCounter`, `EnemySpawnTimer`, `EnemyTransformTimer`)

---

## 1. Objetivos y Alcance
Documentar e implementar con total rigor y paridad 1:1 el subsistema de generación de enemigos de *Mighty Bomb Jack*:
1. **Cadencia de Spawning Diferido**: Spawneo periódico basado en la tabla de dificultad del cartucho original (no instantáneo al cargar sala).
2. **Límite Canónico de Enemigos Vivos**: Máximo estricto de 4 enemigos activos concurrentes en la sala (`prg.asm:8203`).
3. **Física e IA de Patrulla de Momias**: Movimiento horizontal, inversión ante límites de pantalla y metatiles sólidos, caída por gravedad.
4. **Fórmula Determinista de Mutación**: Mutación matemática `(APressCounter % 6) + 2` al agotarse el temporizador de sala.
5. **Colisiones Letales, Power Coins e Invulnerabilidad**: Muerte de Jack sin poder, transmutación en monedas con bonificación multiplicadora (100 -> 8000 pts) y arrollamiento letal con Mighty Nivel 3.

---

## 2. Evidencia Empírica de Ensamblador 6502

### 2.1 Tabla Canónica de Dificultad (`data.asm:116-141`)
```assembly
DifficultyTable:
; based on last bomb room (i.e. last "stage x cleared")
;   Speed   Spawn   TF_Normal   TF_BombRooms    Round
.BYTE  0,    $A0,     $80,         $40       ; 1/2
.BYTE  1,    $80,     $70,         $38       ; 3
.BYTE  2,    $70,     $60,         $30       ; 4
.BYTE  3,    $60,     $50,         $28       ; 5
.BYTE  4,    $58,     $40,         $20       ; 6
.BYTE  5,    $50,     $30,         $18       ; 7
.BYTE  4,    $48,     $20,         $10       ; 8+
```

### 2.2 Temporizador y Límite de Spawning (`prg.asm:8198-8217`)
```assembly
_AfterDiffInit:
	DEC EnemySpawnTimer
	BNE locret_B5A6
	JSR LoadDifficultySettings
	LDA AliveEnemyCount		; CMP #4
	CMP #4				; Maximum of 4 enemies alive
	BCC _SpawnEnemy			; If less than 4, spawn new one
locret_B5A6:
	RTS

_SpawnEnemy:
	JSR sub_BA00            ; Busca slot libre (0..7)
	INC AliveEnemyCount		; Conteo de vivos <= 4
	LDA #0                  ; Tipo 0 = Momia
	JSR InitEnemy
	JSR ChooseEnemySpawnPoint
	JSR OrEnemyStatusWith08
	RTS
```

### 2.3 Mutación Determinista `(APressCounter % 6) + 2` (`prg.asm:9362-9375`)
```assembly
TransformMummy:
	LDA a:APressCounter		; Contador global de pulsaciones botón A ($00FA)
	STA a:Mod_Number
	LDA #0
	STA a:Mod_Number+1
	LDA #6
	STA a:Mod_Modulus
	JSR CalculateModulus    ; Mod_Remainder = APressCounter % 6
	INC a:Mod_Remainder
	INC a:Mod_Remainder     ; + 2
	LDA a:Mod_Remainder
	JSR RunEnemyInit
	JSR OrEnemyStatusWith08
	RTS
```

Los 6 arquetipos mutados resultantes son:
- `2`: **Hanezo** (pájaro con vuelo sinusoidal)
- `3`: **GejiShogun** (oruga trepadora)
- `4`: **Gameido** (tortuga con rebote 45°)
- `5`: **Dokuron** (calavera diagonal perseguidora)
- `6`: **Desufa** (demonio cazador directo)
- `7`: **Horus** (halcón en picado)

---

## 3. Implementación en C99 (`src/core/mjk_enemy.c`)
- **Estructura `MjkEnemyManager`**:
  - `enemies[MJK_MAX_ENEMIES]`: 8 slots físicos.
  - `spawn_timer`: inicializado a `0xA0` (160 frames en Ronda 1).
  - `transform_timer`: `0x80` (128 frames en pasillo) / `0x40` (64 frames en sala de bombas).
  - `alive_enemy_count`: controlado estrictamente (máx 4).
  - `transmute_timer`: 320 frames durante Power Coin (pausa el spawning según `prg.asm:8178`).
- **Mapeo de Sprites en `MjkGame_Render`**:
  - Moneda de transmutación: tile `$21`, subpaleta 2 con parpadeo en los últimos 64 frames.
  - Momia estándar: tile `$25`.
  - Formas mutadas: `$29` a `$3D`.
  - Inversión horizontal automática según `facing_left` (bit 6 OAM).

---

## 4. Cobertura y Verificación
- Suite unitaria determinista: `tests/test_enemy_spawns.c` (TEST 15/15).
  - Subtest 1: Paridad de la tabla de dificultad bit a bit con `data.asm`.
  - Subtest 2: Spawning diferido (160 frames) y corte en 4 enemigos.
  - Subtest 3: Patrullaje horizontal, inversión en metatiles y límites de pantalla.
  - Subtest 4: 8 casos de mutación modular `(A % 6) + 2` probados exhaustivamente.
  - Subtest 5: Colisión letal sin poder, recolección transmutada progresiva (+100..+8000) y destrucción con Mighty Nivel 3 (+500).
- Estado: **15/15 tests pasando con 0 errores y compilación limpia de `bin/mjk.exe`**.
