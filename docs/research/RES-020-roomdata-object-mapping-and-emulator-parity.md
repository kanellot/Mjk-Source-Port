# RES-020: Paridad Canónica de Mapeo de Objetos (Bombas y Cofres), Decodificación de Bytecode y Arnés de Verificación con Emulador

- **Estado**: `[CONFIRMADA / GROUND TRUTH]`
- **Fecha**: 2026-10-25
- **Misión / Tarea**: TASK-F6-029 (Paridad Canónica de Mapeo de Objetos, Eliminación de Glitches Visuales y Arnés de Verificación con Emulador)
- **Fuentes Primarias (Golden Ground Truth)**:
  - `asm/src/prg.asm` (Desmontaje 6502 de rutinas de carga, decodificación y blitteo: líneas 3130-3341, 3347-3535, 3540-3680, 3684-3820, 4835-5165, 5255-5460, 5675-5705)
  - `asm/src/roomdata-us.asm` (Definición canónica de salas US: `RoomData_01` línea 4-5, `RoomData_02` línea 6-7)
  - `asm/src/data.asm` (`SectionRoomsTable` línea 640-683, `ItemToTileTable` línea 888-920, `RoomDataPointers` línea 2227-2482)
  - `asm/src/ram.asm` (`SpriteDMAArea` línea 656, `PPUUpdateBuffer` línea 1285, `AttributeTableBuffer` línea 1683, `PaletteBuffer` línea 1813, `RoomDataRAM` línea 1846, `byte_683` línea 1975)
  - `asm/src/defs.asm` (Constantes y enumeraciones: `Item_Bomb` línea 286, `Item_RedChest` línea 297, `Item_OrangeChest` línea 298, `RF_BombRoom` línea 216)

---

## 1. Decodificación de Cabeceras $E0 y Mecánica de Avance de Bytes en `sub_9592`

### 1.1 Estructura Tridimensional de la Cabecera `$E0` (3 Bytes)
En el bytecode de salas de Mighty Bomb Jack, cualquier secuencia que comience con un byte cuyo cuarteto superior sea `$E` (`$E0..$EF`) representa una cabecera de configuración escénica de sala. Esta cabecera consta invariablemente de **3 bytes contiguos**:

1. **Byte 0 (`$Ex`)**: Cuarteto superior `$E` (código de cabecera), cuarteto inferior `0..$F` correspondiente a `RoomPalette` (`asm/src/prg.asm:3151-3152`).
2. **Byte 1 (`$00..$07`)**: Tipo de música ambiental (`RoomMusic = byte & 7`, `asm/src/prg.asm:3154-3156`).
3. **Byte 2 (`$00..$FF`)**: Índice de sección y flags de transición (`asm/src/prg.asm:3158-3206`).
   - Bits 0..6 (`byte & $7F`): Índice 1-based en `SectionRoomsTable` (al decrementar `DEY`, línea 3196, se indexa la sección).
   - Bit 7 (`byte & $80`): Flag de checkpoint/warp (`loc_93B5`, actualiza `CheckpointRoomIDMaybe` y `byte_33E`, líneas 3173-3191).
   - Bit 6 (`byte & $40`): Flag de carga diferida / control (`INC SomeLoadFlag`, línea 3162).

Ejemplo canónico en `RoomData_01` (`asm/src/roomdata-us.asm:5`):
```assembly
RoomData_01:
    .BYTE $E0, 0, $81, $8F, 0, 5, $85, $87, $39, $6B, $6D, $10, $C6, $11, $CC, $FF
```
- Byte 0: `$E0` -> Cabecera, `RoomPalette = 0`.
- Byte 1: `$00` -> `RoomMusic = 0`.
- Byte 2: `$81` -> Bit 7 = 1 (punto de control inicial activo), Bits 0..6 = 1 (Sección 1 -> `Section_1_1`).

---

### 1.2 Avance de Punteros en `ReadRoomData` (Fase de Precarga en RAM)
Durante la inicialización de la sección (`GameState_2_LoadRoom`), `ReadRoomData` (`asm/src/prg.asm:3257-3341`) parsea el bytecode de la sala para rellenar los slots de `RoomDataRAM`:

1. **Lectura inicial del primer byte (`$E0`)**:
   - `asm/src/prg.asm:3274`: `LDY byte_F` (inicialmente 0) -> `LDA (off_58),Y` (obtiene `$E0`).
   - `asm/src/prg.asm:3284-3285`: `INC byte_F` (avanza a 1) -> `TAX` (X = `$E0`).
   - `asm/src/prg.asm:3286-3290`: `LDA byte_AC` (es 0) -> `TXA; AND #$F0; BNE loc_9496`. Como `$E0 & $F0 = $E0 != 0`, salta a `loc_9496`.
2. **Salto a `loc_9496` y consumo del segundo byte**:
   - `asm/src/prg.asm:3334`: `INC byte_F` (avanza `byte_F` de 1 a 2, saltando el Byte 1 de música `$00`).
   - `asm/src/prg.asm:3337-3339`: `STX byte_D; TXA; JSR HandleRoomDataObject`.
3. **Mapeo a la tabla de saltos y consumo del tercer byte**:
   - `asm/src/prg.asm:3348-3353`: `HandleRoomDataObject` hace `LSR A` 4 veces (A = `$0E`). Comprueba `CMP #$B; BCS loc_94AD`. Al ser `$0E >= $0B`, **NO incrementa `byte_E`** (el puntero de escritura en `RoomDataRAM` no se altera).
   - `asm/src/prg.asm:3357, 3376`: El vector `$0E` apunta a `JT_94AD_E`.
   - `asm/src/prg.asm:3508-3511`:
     ```assembly
     JT_94AD_E:
         INC byte_F
         RTS
     ```
     `INC byte_F` incrementa `byte_F` de 2 a 3 (saltando el Byte 2 de sección/flags `$81`).
4. **Balance Total en `ReadRoomData`**:
   - Bytes consumidos de ROM: exactamente 3 (`byte_F = 3`).
   - Objetos escritos en `RoomDataRAM`: 0 (`byte_E` inalterado).
   - El siguiente byte a leer queda posicionado con precisión milimétrica en el offset 3 (`$8F`, comando de puerta).

---

### 1.3 Avance de Punteros en `sub_9592` (Fase de Blitteo y Renderizado Gráfico)
Durante el renderizado de la sala (`sub_9696` en `asm/src/prg.asm:3684-3699`), se ejecuta un bucle que llama secuencialmente a `sub_9592` para extraer cada objeto y a `sub_96B7` para blittear sus metatiles:

1. **Lectura en `sub_9592` (`asm/src/prg.asm:3541-3587`)**:
   - Offset 0: `LDY #0; LDA (off_58),Y` (obtiene `$E0`).
   - `asm/src/prg.asm:3558`: `LDA byte_AC` (es 0) -> salta a `loc_95BD`.
   - `asm/src/prg.asm:3567-3571`:
     ```assembly
     loc_95BD:
         STX a:byte_5C               ; byte_5C = $E0
         INY                         ; Y = 1
         LDA (off_58),Y              ; Lee Byte 1 ($00)
         STA a:UnknownDoorFlag
         STA byte_70
         TXA
         AND #$F0                    ; $E0 & $F0 != 0 -> salta a loc_95E1
         BNE loc_95E1
     ```
   - `asm/src/prg.asm:3586-3587`:
     ```assembly
     loc_95E1:
         INY                         ; Y = 2
         STY byte_0                  ; byte_0 = 2
     ```
2. **Detección explícita de `$E0` y avance de `byte_0`**:
   - `asm/src/prg.asm:3595-3604`:
     ```assembly
     LDA a:byte_5C
     ...
     AND #$F0
     CMP #$E0
     BEQ loc_961A                    ; Coincidencia exacta con cabecera $E0
     ```
   - `asm/src/prg.asm:3614-3616`:
     ```assembly
     loc_961A:
         INC byte_0                  ; byte_0 se incrementa de 2 a 3
         BNE loc_9677
     ```
3. **Avance del puntero base ROM `off_58` y no incremento de `RoomDataRAM`**:
   - `asm/src/prg.asm:3664-3677`:
     ```assembly
     loc_9677:
         LDA byte_0                  ; A = 3
         CLC
         ADC off_58
         STA off_58                  ; off_58 += 3
         LDA off_58+1
         ADC #0
         STA off_58+1
         LDA byte_5C                 ; A = $E0
         AND #$F0                    ; A = $E0
         CMP #$B0                    ; $E0 >= $B0 -> Salta locret_9695
         BCS locret_9695
         INC byte_5A                 ; NO se ejecuta: byte_5A no se incrementa
     locret_9695:
         RTS
     ```
4. **Supresión del Renderizado Gráfico en `sub_96B7` (`asm/src/prg.asm:3746-3760`)**:
   - Tras retornar de `sub_9592`, `sub_9696` invoca inmediatamente a `sub_96B7`:
     ```assembly
     sub_96F5:
         LDA a:byte_70,X
         BEQ loc_9713
         LDA byte_5C
         AND #$F0                    ; $E0 & $F0 = $E0
         CMP #$B0
         BCS loc_970C                ; $E0 >= $B0 -> Salta a loc_970C
         ...
     loc_970C:
         CMP #$D0                    ; Comprueba si es $D0
         BNE loc_9713                ; Como $E0 != $D0, salta a loc_9713
         ...
     loc_9713:
         PLA
         PLA
         RTS                         ; Aborta sub_96B7 desapilando el retorno
     ```
   - Al saltar a `loc_9713`, desapila la dirección de retorno de `sub_96B7` y sale de inmediato sin escribir ningún metatile en PPU ni alterar atributos.

> [!IMPORTANT]
> **Garantía Anti-Desfase (Paridad Canónica)**:
> - Tanto `ReadRoomData` como `sub_9592` avanzan el puntero de lectura exactamente **3 bytes** (`$E0, param1, param2`).
> - Ninguna de las dos rutinas incrementa el índice del objeto persistente (`byte_E` o `byte_5A`).
> - Si un parser no canónico consumiera 1 o 2 bytes para `$E0`, el flujo de lectura interpretaría parámetros de audio/escena como coordenadas de objetos, desplazando todos los metatiles de la sala y generando colisiones erróneas o pantallas corruptas.

---

## 2. Decodificación del Comando de Bombas: Conteo Inicial y Secuencia Consecutiva YX

### 2.1 Sintaxis del Comando de Bombas
En las salas de Mighty Bomb Jack, las bombas se definen mediante un formato compacto de longitud variable:
1. **Byte de Conteo**: Un valor escalar cuyo cuarteto superior es `0x0` (`count = byte & 0x0F`, con `count` entre 1 y 15).
2. **Secuencia de Coordenadas YX**: Una ráfaga contigua de `count` bytes, donde cada byte representa la coordenada metatile de una bomba:
   - Cuarteto superior (bits 4..7): Fila metatile Y (`0..15` -> píxeles `Y * 16`).
   - Cuarteto inferior (bits 0..3): Columna metatile X (`0..15` -> píxeles `X * 16`).

---

### 2.2 Desglose Canónico de Sala 1 y Sala 2 (`asm/src/roomdata-us.asm`)

#### Sala 1 (`RoomData_01`, líneas 4-5)
```assembly
RoomData_01:
    .BYTE $E0, 0, $81, $8F, 0, 5, $85, $87, $39, $6B, $6D, $10, $C6, $11, $CC, $FF
```
- Byte de Conteo: `5` (5 bombas consecutivas).
- Coordenadas de las 5 bombas:
  1. `$85`: Fila Y = 8 (Y = 128 px), Columna X = 5 (X = 80 px).
  2. `$87`: Fila Y = 8 (Y = 128 px), Columna X = 7 (X = 112 px).
  3. `$39`: Fila Y = 3 (Y = 48 px), Columna X = 9 (X = 144 px).
  4. `$6B`: Fila Y = 6 (Y = 96 px), Columna X = 11 (X = 176 px).
  5. `$6D`: Fila Y = 6 (Y = 96 px), Columna X = 13 (X = 208 px).

#### Sala 2 (`RoomData_02`, líneas 6-7)
```assembly
RoomData_02:
    .BYTE 6, $41, $43, $67, $69, $8D, $8F, $18, $C2, $10, $C8, $18, $CE, $FF
```
- Byte de Conteo: `6` (6 bombas consecutivas, arranca directamente sin cabecera `$E0` por pertenecer a la misma sección precargada).
- Coordenadas de las 6 bombas:
  1. `$41`: Fila Y = 4 (Y = 64 px), Columna X = 1 (X = 16 px).
  2. `$43`: Fila Y = 4 (Y = 64 px), Columna X = 3 (X = 48 px).
  3. `$67`: Fila Y = 6 (Y = 96 px), Columna X = 7 (X = 112 px).
  4. `$69`: Fila Y = 6 (Y = 96 px), Columna X = 9 (X = 144 px).
  5. `$8D`: Fila Y = 8 (Y = 128 px), Columna X = 13 (X = 208 px).
  6. `$8F`: Fila Y = 8 (Y = 128 px), Columna X = 15 (X = 240 px).

---

### 2.3 Mecánica del Bucle 6502 en `ReadRoomData` y Almacenamiento en `RoomDataRAM`
¿Cómo almacena el microprocesador 6502 estas bombas en `RoomDataRAM`?

1. **Lectura del Conteo Inicial (`asm/src/prg.asm:3288-3294`)**:
   ```assembly
   TXA
   AND #$F0
   BNE loc_9496        ; Si cuarteto alto != 0, salta. Para 5 o 6, el resultado es 0.
   DEX                 ; Decrementa conteo: e.g. 5 pasa a 4
   TXA
   AND #$0F
   STA byte_AC         ; byte_AC = count - 1 (número de coordenadas restantes)
   ```
2. **Configuración de Flags de Sala y Primera Bomba (`asm/src/prg.asm:3295-3305, 3307-3309`)**:
   - En la primera sala de la sección (`byte_5F == 0`), si `RoomID < $90` (sala de pasillo normal), establece `byte_B = 1` (`asm/src/prg.asm:3303-3304`).
   - Carga `X = 0` (`asm/src/prg.asm:3307`).
   - Comprueba `byte_C` (flag de Royal Room): como en pasillos es 0, salta a `loc_9496` (`asm/src/prg.asm:3308-3309`).
3. **Escritura del Primer Objeto en `JT_94AD_0` (`asm/src/prg.asm:3381-3397`)**:
   - `loc_9496` incrementa `byte_F` (avanza al offset de la coordenada de la 1ª bomba).
   - Invoca `HandleRoomDataObject`: cuarteto alto es 0 (`< $0B`), por lo que incrementa `byte_E` (puntero de escritura en `RoomDataRAM`). Salta a `JT_94AD_0`:
     ```assembly
     JT_94AD_0:
         LDY byte_E
         LDA #5                      ; 5 = Item_Bomb
         STA RoomDataRAM,Y           ; Almacena estado inicial de la bomba
         LDX byte_B                  ; Comprueba byte_B
         BEQ locret_94E8
         DEX
         BEQ loc_94E3                ; Si byte_B era 1, salta a loc_94E3
         LDA byte_AC
         BNE locret_94E8
     loc_94E3:
         LDA #$85                    ; Marca la 1ª bomba con Bit 7 ($85)
         STA RoomDataRAM,Y
     locret_94E8:
         RTS
     ```
4. **Bucle de Coordenadas Siguientes (`loc_9486`, `asm/src/prg.asm:3321-3331`)**:
   - En la siguiente iteración de `ReadRoomData`, `byte_AC != 0`:
     ```assembly
     loc_9486:
         LDX #0
         DEC byte_AC                 ; Decrementa contador de bombas restantes
         LDA byte_B
         CMP #2
         BEQ loc_9498
         STX byte_B                  ; Borra byte_B a 0 tras la primera bomba
         .BYTE $2C                   ; BIT $0FE6 (Opcodo de 3 bytes: salta INC byte_F)
     loc_9496:
         INC byte_F                  ; NO se ejecuta para las bombas 2..N
     loc_9498:
         STX byte_D                  ; X = 0
         TXA
         JSR HandleRoomDataObject    ; Incrementa byte_E y llama JT_94AD_0
     ```
   - Para las bombas 2 a N, `byte_B == 0`. Por tanto, `JT_94AD_0` almacena incondicionalmente `#5` (`0x05`) y retorna de inmediato en `locret_94E8`.
5. **Resultado en `RoomDataRAM`**:
   - Bomba 1: `$85` (Item 5 con bit 7 activo: objetivo inicial de bonificación de pasillo).
   - Bombas 2..N: `$05` (Item 5 estándar inactivo).

---

### 2.4 Parseo en `sub_9592` y Renderizado al Nametable (`sub_9696`)
Cuando `sub_9592` parsea la secuencia de bombas para dibujarlas:
1. **Primera Bomba (`asm/src/prg.asm:3567-3587`)**:
   - `byte_AC == 0`: toma la rama `loc_95BD`.
   - `STX byte_5C` (`byte_5C = count`, p. ej. 5).
   - Lee el siguiente byte en `LDA (off_58),Y; STA byte_70` (coordenada de la 1ª bomba, p. ej. `$85`).
   - `byte_AC = count - 1` (p. ej. 4).
   - `byte_0 = 2` (consume 2 bytes: conteo + 1ª coordenada).
   - Incrementa `byte_5A` (apunta a la 1ª bomba en `RoomDataRAM`).
2. **Bombas 2..N (`asm/src/prg.asm:3558-3564`)**:
   - `byte_AC != 0`: toma la rama de repetición.
   - `LDA (off_58),Y; STA byte_70` (coordenada directa de la bomba N).
   - `DEC byte_AC`.
   - `byte_0 = 1` (consume exactamente 1 byte de coordenada por cada bomba sucesiva).
   - Incrementa `byte_5A` (apunta a la bomba N en `RoomDataRAM`).
3. Cada bomba dibuja su metatile 2x2 correspondiente (`tile 0x27` extraído de `ItemToTileTable[5]`, `asm/src/data.asm:894`) en la dirección PPU calculada por `sub_92AD` / `sub_92E2`.

---

## 3. Coordenadas Exactas de Cofres Rojos ($10/$11)

### 3.1 Identificación y Propiedades de los Cofres
En el sistema de objetos, los comandos de cofre tienen cuarteto superior `$1`:
- **Comando `$10`**: Cofre rojo estándar (`Item_RedChest = 16`, `asm/src/defs.asm:297`). En `ItemToTileTable` (`asm/src/data.asm:905`), el metatile asignado es `$28`.
- **Comando `$11`**: Cofre naranja (`Item_OrangeChest = 17`, `asm/src/defs.asm:298`). En `ItemToTileTable` (`asm/src/data.asm:906`), el metatile asignado es `$4A`.
- **Contenido del Cofre (Cuarteto Inferior)**:
  - En `MaybeOpenHiddenBlock` (`asm/src/prg.asm:5407-5415`), el cuarteto inferior del comando determina el ítem que brota al abrir el cofre:
    - Cuarteto bajo `0` (`$10`): Bolsa de 100 pts (`Item_100Bag`).
    - Cuarteto bajo `1` (`$11`): Bolsa de 300 pts (`Item_300Bag`).
    - Cuarteto bajo `8` (`$18`): Moneda Mighty (`Item_MightyCoin`).

---

### 3.2 Coordenadas en Sala 1 (`RoomData_01`, `asm/src/roomdata-us.asm:5`)
```assembly
RoomData_01:
    ... $10, $C6, $11, $CC, $FF
```
- **Cofre 1**: Comando `$10`, Coordenada `$C6`.
  - Tipo: Cofre rojo conteniendo bolsa de 100 puntos.
  - Fila Metatile Y: `$C` (12 decimal) -> `12 * 16 = 192` píxeles (nivel de suelo).
  - Columna Metatile X: `$6` (6 decimal) -> `6 * 16 = 96` píxeles.
  - Dirección PPU Nametable 0: `$2000 + (12 * 64) + (6 * 2) = $230C`.
- **Cofre 2**: Comando `$11`, Coordenada `$CC`.
  - Tipo: Cofre rojo (marcado con `$11` en bytecode para contenido 300 pts, renderizado como cofre de suelo).
  - Fila Metatile Y: `$C` (12 decimal) -> `12 * 16 = 192` píxeles (nivel de suelo).
  - Columna Metatile X: `$C` (12 decimal) -> `12 * 16 = 192` píxeles.
  - Dirección PPU Nametable 0: `$2000 + (12 * 64) + (12 * 2) = $2318`.

---

### 3.3 Coordenadas en Sala 2 (`RoomData_02`, `asm/src/roomdata-us.asm:7`)
```assembly
RoomData_02:
    ... $18, $C2, $10, $C8, $18, $CE, $FF
```
- **Cofre 1**: Comando `$18`, Coordenada `$C2`.
  - Tipo: Cofre rojo conteniendo moneda Mighty (`Item_MightyCoin`).
  - Fila Metatile Y: `$C` (12 decimal) -> `12 * 16 = 192` píxeles (nivel de suelo).
  - Columna Metatile X: `$2` (2 decimal) -> `2 * 16 = 32` píxeles.
  - Dirección PPU Nametable 0: `$2000 + (12 * 64) + (2 * 2) = $2304`.
- **Cofre 2**: Comando `$10`, Coordenada `$C8`.
  - Tipo: Cofre rojo conteniendo bolsa de 100 puntos.
  - Fila Metatile Y: `$C` (12 decimal) -> `12 * 16 = 192` píxeles (nivel de suelo).
  - Columna Metatile X: `$8` (8 decimal) -> `8 * 16 = 128` píxeles.
  - Dirección PPU Nametable 0: `$2000 + (12 * 64) + (8 * 2) = $2310`.
- **Cofre 3**: Comando `$18`, Coordenada `$CE`.
  - Tipo: Cofre rojo conteniendo moneda Mighty (`Item_MightyCoin`).
  - Fila Metatile Y: `$C` (12 decimal) -> `12 * 16 = 192` píxeles (nivel de suelo).
  - Columna Metatile X: `$E` (14 decimal) -> `14 * 16 = 224` píxeles.
  - Dirección PPU Nametable 0: `$2000 + (12 * 64) + (14 * 2) = $231C`.

Todos los cofres de las Salas 1 y 2 reposan exactamente sobre el piso inferior (Y=192), alineados con la rejilla física de colisión.

---

## 4. Estado Inicial Canónico de las Bombas: Apagadas (0x05) y Mecánica de la Mecha

### 4.1 Estado Inicial en Pasillos: SIN Chispa y Apagadas
En los pasillos y salas normales (Salas 1 a 16):
1. **Estado en `RoomDataRAM`**:
   - La primera bomba recibe el valor `$85` (Bit 7 = 1, Bit 6 = 0, Bits 0..4 = 5).
   - Las bombas restantes reciben el valor `$05` (Bit 7 = 0, Bit 6 = 0, Bits 0..4 = 5).
   - **Bit 6 (`0x40`) representa el flag de Bomba Encendida / Fire Bomb**.
   - En el arranque, **ninguna bomba tiene el Bit 6 activo**. Todas arrancan formalmente **apagadas**.
2. **Ausencia de Sprite de Chispa (OAM DMA Slot 4)**:
   - El sprite de la chispa de la mecha (`asm/src/prg.asm:5369-5388, MaybeDrawFireBombSprite`) se almacena en `SpriteDMAArea+$10..$13` (bytes 16..19 de la tabla OAM).
   - `MaybeDrawFireBombSprite` **SOLO se invoca dentro de la rama de Royal Rooms** (`asm/src/prg.asm:5014-5016, 5160`).
   - En las salas de pasillo, `MaybeDrawFireBombSprite` nunca es ejecutado durante la carga ni durante la recolección normal.
   - En consecuencia, en los pasillos **NO existe sprite de chispa** sobre las bombas. Las bombas son metatiles fijos estáticos (`tile $27`).

---

### 4.2 Propagación de Mecha en Pasillos (`loc_9EB2`)
Cuando el jugador recoge una bomba en un pasillo:
1. `JTPickup_Bomb` (`asm/src/prg.asm:4970-4977`) comprueba si la bomba estaba encendida:
   ```assembly
   LDY byte_5A
   LDA RoomDataRAM,Y
   LDX #1
   AND #$40                    ; ¿Bomba encendida?
   BEQ loc_9E68
   INX                         ; X = 2 si estaba encendida (doble puntuación y sonido 8)
   ```
2. Al no ser Royal Room (`asm/src/prg.asm:5013-5015`), salta a `loc_9EB2`.
3. Busca hacia adelante en `RoomDataRAM` la siguiente bomba disponible (`asm/src/prg.asm:5074-5087`):
   ```assembly
   loc_9F20:
       INY
       LDA RoomDataRAM,Y
       CMP #$FF
       BEQ loc_9F31
       AND #$7F
       CMP #5                  ; Comprueba si es Item_Bomb
       BEQ loc_9F40
       DEX
       BNE loc_9F20
   ```
4. Al encontrar la siguiente bomba, activa su flag de encendido (`asm/src/prg.asm:5099-5103`):
   ```assembly
   loc_9F40:
       LDA RoomDataRAM,Y
       ORA #$40                ; Activa Bit 6 ($40) -> la bomba queda ENCENDIDA
       STA RoomDataRAM,Y
   ```
5. En pasillos, este encendido es puramente lógico en `RoomDataRAM` (otorga doble bonificación si se recoge a continuación); no genera sprite de llama en OAM.

---

### 4.3 Encendido y Chispa en Royal Rooms (`$90..$9F`)
En las Royal Rooms (Salas de Bombas de palacio real):
1. La bandera `RF_BombRoom` (`RoomStatusFlags & 2`, `asm/src/defs.asm:216`) está activa.
2. Al cargar la sala, `byte_C` se activa (`asm/src/prg.asm:3260-3264`), fijando `byte_AC = $18` (24 bombas) y configurando el puntero `byte_90` al flujo de coordenadas ROM.
3. Al recoger la primera bomba (`byte_F7 == 0`, `asm/src/prg.asm:5109-5116`), busca la siguiente bomba con `CMP #5`, activa su flag `ORA #$40` (`asm/src/prg.asm:5153`), y llama inmediatamente a `MaybeDrawFireBombSprite` (`asm/src/prg.asm:5160`):
   ```assembly
   MaybeDrawFireBombSprite:
       PHA
       AND #$F0
       SEC
       SBC #4
       STA SpriteDMAArea+$10   ; Y = (coord & 0xF0) - 4
       PLA
       ASL A; ASL A; ASL A; ASL A
       CLC
       ADC #8
       STA SpriteDMAArea+$13   ; X = (coord << 4) + 8
       LDA #$0D
       STA SpriteDMAArea+$11   ; Tile ID inicial = $0D
       LDA #$12
       STA SpriteDMAArea+$12   ; Atributos = $12 (Paleta 2, prioridad sobre fondo)
       RTS
   ```
4. Durante la ejecución normal, la chispa parpadea alternando su tile cada 3 frames entre `$0D` y `$0F` mediante `EOR #2` (`asm/src/prg.asm:5684-5688`).
5. Cuando se recolectan las 23 bombas del palacio, el sprite se oculta escribiendo `$F8` en su posición Y (`asm/src/prg.asm:5130-5131`).

---

## 5. Direcciones Exactas de RAM (NES Hardware & Memory Map)

| Elemento / Subsistema | Dirección NES RAM | Tamaño | Archivo / Línea | Propósito Canónico |
| :--- | :--- | :--- | :--- | :--- |
| **`RoomDataRAM` (Base)** | `$0603` | 128 bytes | `asm/src/ram.asm:1846` | Base de persistencia de estados de objetos para 8 slots de sala |
| - Slot 0 (Sala 0 de sección) | `$0603..$0612` | 16 bytes | `asm/src/ram.asm:1846-1861` | Byte 0: RoomID, Bytes 2..14: Bombas/Cofres, Byte 15: `$FF` |
| - Slot 1 (Sala 1 de sección) | `$0613..$0622` | 16 bytes | `asm/src/ram.asm:1862-1877` | Estado de bombas y cofres de la 2ª sala de la sección |
| - Slot 2 (Sala 2 de sección) | `$0623..$0632` | 16 bytes | `asm/src/ram.asm:1878-1893` | Estado de bombas y cofres de la 3ª sala de la sección |
| - Slot 3 a Slot 7 | `$0633..$0682` | 80 bytes | `asm/src/ram.asm:1894-1974` | Slots restantes de la sección |
| - Slot 8 (Scratch / Royal) | `$0683..$0692` | 16 bytes | `asm/src/ram.asm:1975-1990` | Reservado para salas especiales y Royal Rooms (`byte_683`) |
| **Bombas Activas (Valores)** | Dentro de slot | 1 byte c/u | `asm/src/prg.asm:3383, 3393` | `$05` (apagada), `$45` (encendida/lit), `$85` (primer objetivo), `$0F` (recogida) |
| **Cofres Activos (Valores)** | Dentro de slot | 1 byte c/u | `asm/src/prg.asm:3416, 4871` | `$10` (rojo), `$11` (naranja), `$20` (oculto/sin abrir), `$0F` (abierto/recogido) |
| **Chispa de Mecha (OAM Slot 4)**| `$0210..$0213` | 4 bytes | `asm/src/ram.asm:656`, `asm/src/prg.asm:5374-5386` | Offset `$10` de `SpriteDMAArea`: Y (`$210`), Tile (`$211`), Attr (`$212`), X (`$213`) |
| **`AttributeTableBuffer`** | `$0563..$05A2` | 64 bytes | `asm/src/ram.asm:1683-1746` | Buffer RAM de atributos de paleta transferidos a PPU `$23C0` |
| **`PaletteBuffer`** | `$05E3..$0602` | 32 bytes | `asm/src/ram.asm:1813-1845` | Paletas activas transferidas a PPU `$3F00` |
| **`PPUUpdateBuffer`** | `$03F2..$03F5` | 4 bytes | `asm/src/ram.asm:1285-1289` | Buffer de 4 baldosas (2x2 tiles) para el metatile en curso |
| **Nametable VRAM PPU** | `$2000..$23BF` | 960 bytes | Hardware PPU | Matriz de 32x30 tiles. Fórmula de metatiles: `$2000 + (Y * 64) + (X * 2)` |

---

## 6. Arnés de Verificación con Emulador y Criterios de Paridad Bit-a-Bit

Para garantizar paridad estricta entre la reimplementación y el emulador (Mesen / FCEUX / BizHawk):
1. **Verificación de Memoria en `$0603..$0692`**:
   - Al cargar la Sala 1 (`GameState == 3`), el dump de memoria de `$0603..$0612` debe contener exactamente:
     ```
     0603: 01 xx 85 05 05 05 05 10 10 FF ...
     ```
     (Room ID `01`, 5 bombas con la primera en `$85` y 4 en `$05`, 2 cofres rojos en `$10`, terminador `$FF`).
   - Al cargar la Sala 2, el slot correspondiente en `RoomDataRAM` debe contener:
     ```
     0613: 02 xx 05 05 05 05 05 05 10 10 10 FF ...
     ```
     (Room ID `02`, 6 bombas apagadas `$05`, 3 cofres rojos `$10`, terminador `$FF`).
2. **Verificación del Nametable PPU en VRAM `$2000..$23BF`**:
   - Para Sala 1:
     - Metatile `$27` (bomba) en `$220A` (`$85`), `$220E` (`$87`), `$20D2` (`$39`), `$2196` (`$6B`), `$219A` (`$6D`).
     - Metatile `$28` (cofre rojo) en `$230C` (`$C6`) y `$2318` (`$CC`).
   - Para Sala 2:
     - Metatile `$27` (bomba) en `$2102` (`$41`), `$2106` (`$43`), `$218E` (`$67`), `$2192` (`$69`), `$221A` (`$8D`), `$221E` (`$8F`).
     - Metatile `$28` (cofre rojo) en `$2304` (`$C2`), `$2310` (`$C8`), `$231C` (`$CE`).
3. **Verificación del OAM (`SpriteDMAArea+$10`)**:
   - En pasillos (Salas 1 y 2), `SpriteDMAArea+$10` debe permanecer en `$F8` (oculto fuera de pantalla). Ningún sprite de chispa debe aparecer hasta la transición a una Royal Room.
