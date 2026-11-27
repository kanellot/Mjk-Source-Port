# Mundo, Salas, Cámara y Sistema de Puertas (`REV_US`)

Especificación del formato de 256 salas (`RoomData`), la persistencia en `RoomDataRAM`, los tres modos de cámara/scroll y las 33 condiciones de puerta de `REV_US`.

**Fuentes**: `asm/src/prg.asm` (`2579-2857`, `3077-3855`, `5312-5365`, `6696-7425`), `asm/src/data.asm` (`322-339`, `640-683`, `888-920`, `1685-1800`, `2227-2482`), `asm/src/roomdata-us.asm`.

---

## 1. Decodificación de Salas y Persistencia (`RoomDataRAM`)

### Jerarquía de 4 Niveles de Geometría
Cada sala (`$00..$FF`) se reconstruye en una matriz de `15×16` metatiles `16×16` a partir de 4 niveles en ROM:
1. `RoomHalfScreens` → selecciona mitad izquierda y mitad derecha de pantalla.
2. `HalfScreenLayouts` → lista de bloques de diseño.
3. `LayoutChunks` → tiras de metatiles.
4. `MetatileDefinitions` → 4 tiles CHR `8×8` + atributos por metatile.

### Persistencia en `RoomDataRAM` (`$0603–$0692`)
- **8 ranuras de sección (`$0603–$0682`, 16 B cada una)** + **1 ranura scratch (`$0683–$0692`)** para *Bomb Rooms* (`$90..$9F`) o salas fuera de sección.
- Layout de cada ranura de 16 bytes:
  - Byte `0`: `RoomID` (o `0` si vacía).
  - Byte `1`: padding (sin uso).
  - Bytes `2..14`: estado dinámico de hasta 13 objetos interactivos (bombas, cofres, bloques secretos, puertas).
  - Terminador `$FF`.
- Al entrar a una sala con cabecera `$Ex` cuya sección cambia, `GameState_2_LoadRoom` (`prg.asm:3150-3251`) precarga las salas de `SectionRoomsTable` en las 8 ranuras. Al volver a una sala ya presente en `RoomDataRAM`, `IfNotTitleScreenRoomDoThings` (`prg.asm:3077-3135`) reutiliza la ranura preservando objetos recogidos y puertas abiertas.

### Bytecode de Objetos (`HandleRoomDataObject`, `prg.asm:3347-3377`)

| Rango Opcode | Tipo de Entidad | Formato |
| :---: | :--- | :--- |
| `$01..$0F` | Grupo de bombas (`N` bombas) | `1B cmd + N bytes YX` |
| `$10..$2F` / `$30..$4F` | Cofre Rojo / Naranja visible | `1B cmd + 1B YX` (+ `1B` evento si Esfinge) |
| `$50..$6F` | Bloque secreto multi-golpe | `1B cmd + 1B YX` |
| `$70..$7F` | Ítem expuesto directo | `1B cmd + 1B YX` |
| `$80..$8F` | Puerta de salida | `1B cmd + 1B arg` |
| `$90..$9F` / `$A0..$AF` | Cofre Rojo / Naranja oculto | `1B cmd + 1B YX` (+ `1B` evento si Esfinge) |
| `$E0..$EF` | Cabecera de sala (`$E0 \| pal`, `(cond<<3) \| music`, `flags \| section`) | `3 bytes fijos` |
| `$FF` | Fin de sala | `0 bytes` |

---

## 2. Topología (`AdjacentRoomsTable`) y Cámara / Scroll

`AdjacentRoomsTable` (`data.asm:1704-1800`) define 5 bytes por sala: `[Flags, Up, Down, Right, Left]`.

### Tres Modos de Pantalla (`RoomStatusFlags` en `$00BC` / `$03BE`)
1. **Pantalla Única (`RF_SingleScreen = $10`)**: *Bomb Rooms*, Sala de Tortura y cámaras fijas (`ScrollX = 0, ScrollY = 0`).
2. **Pasillo Horizontal (`RF_SingleScreen = 0, RF_ScrollStop = $20`)**: 2 a 6 salas contiguas Este–Oeste. Jack queda anclado en `X = 128` (`$80`) mientras la cámara desplaza el mundo a `1.5 px/frame`, desanclándose en los extremos (`RF_AtScrollEdge = $08`).
3. **Pozo Vertical (`RF_SingleScreen = 0, RF_ScrollStop = 0`)**: 2 a 8 salas Norte–Sur. Jack queda anclado en `Y = 121` (`$79`) y alterna Nametable `$2000`/`$2800` cada 240 líneas.

---

## 3. Sistema de Puertas y 33 Condiciones (`REV_US`)

En cada frame de `GameState_5_Main`, `MaybeHandleDoorOpening` (`prg.asm:6696-6740`) extrae los 5 bits superiores del Byte 1 de cabecera (`(byte1 >> 3) - 1`, rango `$00..$1E`). En la sala especial **`$61`** fuerza el índice **`$1F`** (`prg.asm:6725-6728`).

### Resumen de Condiciones Activas en `REV_US` (`CheckAndMaybeOpenDoor`, `prg.asm:6741-7005`)

| Índice | Etiqueta | Condición de Apertura en `REV_US` |
| :---: | :--- | :--- |
| **`$02`** | `DoorCond_02_JumpedOn` | Jack realiza un salto (`JustJumpedFlag != 0`). |
| **`$07, $0E, $17, $18, $1B`** | `DoorCond_XX_Mighty3` | `PlayerMightyLevel == 3`. *(En JP `$0E` era `StageTimer == $60`.)* |
| **`$09`** | `DoorCond_09_FBWarp2` | Flag bi-estable `DoorCheckFlag_09` activado tras el warp `$1D` (R4 → R7). |
| **`$0C`** | `DoorCond_0C_AllFireBombs` | Bit 7 de `MaybeBombThing` activo (todas las bombas encendidas recogidas en orden). |
| **`$19, $1D`** | `DoorCond_1D_FBWarp1` | 4 condiciones conjuntas: `RF_AtScrollEdge`, `PlayerMightyCoins == 0`, `StageTimer == 0` y `PlayerSprite == 6`. Activa `DoorCheckFlag_09 = 1`. |
| **`$1E`** | `DoorCond_1E_R5R11Warp` | 2 muertes contra tile de fuego `$5D` (`DoorCheckFlag_1E_DeathCount == 2`), warp R5 → R11. |
| **`$1F`** | `DoorCond_1F_R11WarpBack` | Uso único en sala `$61` (`DoorCheckFlag_1F == 0`), warp R11 → R6. |
| **`$20`** | `DoorCond_20_US` | Entrada 33 de tabla en `REV_US` (inalcanzable por el rango de 5 bits de cabecera). |
| *Resto* | `DoorCond_rts` | Cerrada / apertura por evento externo (Esfinge o *Bomb Room*). |

### Transición de Puertas y *Bomb Rooms*
- **Bomb Rooms (`$90..$9F`)**: no usan la tabla anterior; al recoger las **23 bombas** (`FireBombsCollected2 == 23`, `prg.asm:5126`), `OpenBombRoomDoor` abre la puerta de salida (`GameState = $B`).
- **Cruce de puerta (`JTPickup_Door` → `GameState_9_ExitRoom`, `prg.asm:5312-5346, 7294-7372`)**: consulta `AdjacentRoomsTable` según la dirección de salida (`EntryDoorType >> 2`: `0`=Up, `1`=Down, `2`=Right, `3`=Left), invierte la orientación de llegada mediante **`EntryDoorType = ExitDoorType ^ 4`** y posiciona a Jack con `DoorEntryXYPositionTable` (`data.asm:322-339`).
