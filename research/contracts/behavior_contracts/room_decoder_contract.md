# Contrato Funcional: Decodificador de Salas, Puertas, Bombas y Cofres

- **ID de Contrato**: `research/contracts/behavior_contracts/room_decoder_contract.md`
- **Subsistema**: 03 — Escenarios, Metatiles y Decodificación de Salas
- **Tarea**: `TASK-F3-003`
- **Fuentes de Ensamblador**:
  - `asm/src/prg.asm:3130-3450` (`GameState_2_LoadRoom`, `ReadRoomData`, `LoadLayoutChunk`)
  - `asm/src/prg.asm:6741-6865` (`CheckAndMaybeOpenDoor`, condiciones de puertas USA)
  - `asm/src/data.asm:2227-2482` (`RoomDataPointers`, 257 salas)
  - `asm/src/roomdata-us.asm:1-485` (`RoomData_01` a `RoomData_FF`)
- **Estado Epistémico**: `[CONFIRMADA]`

---

## 1. Decodificador Jerárquico 4-Tier de Escenarios

Cada una de las 257 salas de la pirámide se decodifica en 4 capas de abstracción para comprimir la geometría completa en los 32 KB de ROM:

```
[Nivel 1: Puntero de Sala]
  CurrentRoomID (1..257) -> RoomDataPointers[CurrentRoomID]
           |
           v
[Nivel 2: Descompresión de Chunks (Layout Header)]
  Bytecodes de repetición y plantillas estructurales ($E0-$EF)
           |
           v
[Nivel 3: Cuadrícula de Metatiles (16x15)]
  Matriz de 240 metatiles de 16x16 píxeles en RAM ($0500-$05BF)
           |
           v
[Nivel 4: Baldosas PPU CHR (8x8)]
  Cada metatile se expande en 4 baldosas CHR de 8x8 (Top-Left, Top-Right, Bot-Left, Bot-Right)
```

### Formato del Bytecode de `RoomData` (`roomdata-us.asm`)
1. **Comando de Plantilla de Sala (`$E0 - $EF`)**:
   - Byte 0: Prefijo `$Ex` (donde $x$ indica el tipo de fondo: $0$=estándar pirámide, $1$=pasaje horizontal, $2$=pasaje vertical, $3$=sala real, $4$=tesoro).
   - Byte 1..N: Índice de bloques preensamblados que se copian en ráfaga a la matriz de metatiles.
2. **Definición de Puertas y Puntos de Salida (`$80 - $8F`)**:
   - Indica las coordenadas de la puerta y el índice de condición asociado (0 a 32).
3. **Terminador de Flujo**:
   - Marcador `$FF`: Señaliza el fin de la carga de geometría de la sala.

---

## 2. Las 33 Condiciones de Apertura de Puertas USA (`REV_US`)

En `prg.asm:6745-6780`, la tabla de salto `CheckAndMaybeOpenDoor` gobierna los requisitos para que una puerta se abra al interactuar con ella:

| Índice | Etiqueta en `asm/` | Condición Lógica Exacta | Efecto / Destino |
| :---: | :--- | :--- | :--- |
| **`$00`** | `DoorCond_rts` | Ninguna (puerta cerrada incondicional) | Inactiva |
| **`$01`** | `DoorCond_rts` | No abre | Inactiva |
| **`$02`** | `DoorCond_02_JumpedOn` | `JustJumpedFlag == 1` | Abre si Jack salta sobre la puerta |
| **`$03-$06`** | `DoorCond_rts` | No abre | Inactivas |
| **`$07`** | `DoorCond_XX_Mighty3` | `PlayerMightyLevel == 3` | Requiere estar en nivel de poder Mighty 3 |
| **`$08`** | `DoorCond_rts` | No abre | Inactiva |
| **`$09`** | `DoorCond_09_FBWarp2` | `DoorCheckFlag_09 & $80 == 0` | Warp inverso de Ronda 4 a Ronda 7 |
| **`$0A`** | `DoorCond_rts` | No abre en US (en JP requería `StageTimer == 30`) | Inactiva en USA |
| **`$0B`** | `DoorCond_rts` | No abre | Inactiva |
| **`$0C`** | `DoorCond_0C_AllFireBombs` | `MaybeBombThing & $80 != 0` | Requiere recoger todas las bombas con mecha |
| **`$0D`** | `DoorCond_rts` | No abre | Inactiva |
| **`$0E`** | `DoorCond_XX_Mighty3` | `PlayerMightyLevel == 3` (en JP `Timer == 60`) | Requiere Mighty Level 3 en USA |
| **`$0F-$16`** | `DoorCond_rts` | No abren | Inactivas |
| **`$17`** | `DoorCond_XX_Mighty3` | `PlayerMightyLevel == 3` | Puerta secreta de tesoro |
| **`$18`** | `DoorCond_XX_Mighty3` | `PlayerMightyLevel == 3` | Puerta de pasadizo |
| **`$19`** | `DoorCond_1D_FBWarp1` | `RoomFireBombsCollected >= 20` | Warp por bombas de fuego |
| **`$1A`** | `DoorCond_1A_US_rts` | Inactiva en US (en JP `Score Tens == 70`) | Inactiva en USA |
| **`$1B`** | `DoorCond_XX_Mighty3` | `PlayerMightyLevel == 3` | Puerta secreta de Sphinx |
| **`$1C`** | `DoorCond_rts` | No abre | Inactiva |
| **`$1D`** | `DoorCond_1D_FBWarp1` | `RoomFireBombsCollected >= 20` | Warp de fase especial |
| **`$1E`** | `DoorCond_1E_R5R11Warp` | Sala `$FC`: Salto directo de Ronda 5 a 11 | Warp de avance rápido |
| **`$1F`** | `DoorCond_1F_R11WarpBack`| Sala `$61`: Retroceso de Ronda 11 a 6 | Trampa de retroceso |
| **`$20`** | `DoorCond_20_US` | Exclusiva USA: evalúa flag secreto de cofre | Apertura especial US |

---

## 3. Cadena de 23 Bombas y Detección de Fire Bombs

Cada sala de bombas contiene exactamente **23 bombas**:
1. **Secuencia de Fuego (Fire Bomb Sequence)**:
   - La primera bomba que Jack recoge enciende la mecha de la siguiente en la secuencia prefijada.
   - Si Jack recoge la bomba encendida, suma **200 puntos** y enciende la siguiente.
   - Si Jack recoge una bomba apagada, suma solo **100 puntos** y apaga la cadena de mechas.
2. **Bonificación de Ronda (`RoundClearBonus`)**:
   - Si se recogen **20 o más bombas con mecha encendida**:
     - 20 bombas encendidas: **$10.000$ puntos** BCD.
     - 21 bombas encendidas: **$20.000$ puntos** BCD.
     - 22 bombas encendidas: **$30.000$ puntos** BCD.
     - 23 bombas encendidas (perfecto): **$50.000$ puntos** BCD.

---

## 4. Apertura de Cofres y Niveles de Poder Mighty

Los cofres de tesoro se sitúan en baldosas específicas y tienen dos estados: Cerrado y Abierto.
1. **Apertura por Salto**: Jack debe descender sobre la superficie superior del cofre con $v_y > 0$ (`sub_9B6A`).
2. **Requisitos de Apertura**:
   - Algunos cofres requieren estar en **Mighty Level 1, 2 o 3**.
   - Si Jack no tiene el nivel requerido, el cofre actúa como un bloque sólido normal y no se abre.
3. **Poder Mighty (Botón B)**:
   - Al pulsar B con monedas Mighty disponibles (`MightyCoins > 0`):
     - Consume 1 moneda Mighty.
     - Eleva el nivel: `MightyLevel = min(3, MightyLevel + 1)`.
     - Inicia temporizador de duración de poder (`MightyTimer = 320` frames).
     - Actualiza la paleta de Jack a azul (`Level 1`), naranja (`Level 2`) o dorado brillante (`Level 3`).
