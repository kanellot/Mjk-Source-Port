# RES-018: Bytecode de RoomData, Persistencia en RoomDataRAM (8 Slots) y Blitteo de Objetos

- **Estado**: `[CONFIRMADA]`
- **Fecha**: 2026-10-25
- **Subrutinas clave**:
  - `GameState_2_LoadRoom`: `asm/src/prg.asm:3130-3251`
  - `ReadRoomData`: `asm/src/prg.asm:3257-3341`
  - `HandleRoomDataObject`: `asm/src/prg.asm:3347-3377`
  - `JT_94AD_*`: `asm/src/prg.asm:3381-3511`
  - `LoadRoomData`: `asm/src/prg.asm:3515-3535`
  - `IfNotTitleScreenRoomDoThings`: `asm/src/prg.asm:3077-3126`
  - `sub_9592` (Decodificador de objetos): `asm/src/prg.asm:3541-3680`
  - `sub_9696` / `sub_96B7` (Blitteo al nametable): `asm/src/prg.asm:3684-3717`
  - `sub_92AD` / `sub_92E2` (Cálculo PPU YX): `asm/src/prg.asm:3022-3071`
  - `LoadLayoutChunk` / `sub_9716` / `sub_9729`: `asm/src/prg.asm:2703-2719, 3773-3820`
- **Tablas clave**:
  - `SectionRoomsTable`: `asm/src/data.asm:640-683`
  - `RoomDataPointers`: `asm/src/data.asm:2227-2482`
  - `ItemToTileTable`: `asm/src/data.asm:888-920`
  - `DoorPositionTileTable`: `asm/src/prg.asm:3823-3839`
  - `UnknownDoorTable97B0`: `asm/src/prg.asm:3840-3855`
- **Variables RAM**:
  - `RoomDataRAM`: `$0603..$0682` (128 bytes, 8 slots de 16 bytes: `asm/src/ram.asm:1846-1974`)
  - `byte_683` (Scratch Slot 8): `$0683..$0692` (16 bytes: `asm/src/ram.asm:1975-1990`)
  - Punteros y registros Zero Page:
    - `off_58` (`$0058-$0059`): Puntero al bytecode de la sala en ROM
    - `byte_5A` (`$005A`): Offset activo en `RoomDataRAM` para el objeto actual
    - `byte_5F` (`$005F`): Índice de sala dentro de la sección actual (0..7)
    - `byte_AC` (`$00AC`): Contador de repetición de bombas consecutivas
    - `byte_E` (`$000E`): Índice de escritura en `RoomDataRAM` durante `ReadRoomData`
    - `byte_F` (`$000F`): Offset de lectura en ROM (`off_58`) durante `ReadRoomData`
    - `byte_5C` (`$005C`): Byte de comando de objeto activo en decodificación
    - `byte_70` (`$0070`): Coordenada YX del objeto (tile 1)
    - `byte_71` (`$0071`): Coordenada YX secundaria (tile 2 en puertas)
    - `MaybeCollectedThing` (`$005E`): Estado del objeto actual en `RoomDataRAM`
    - `MaybeTileCollected` (`$0054`): Metatile ID correspondiente extraído de `ItemToTileTable`

---

## 1. Arquitectura de Memoria Persistente de Salas: `RoomDataRAM`

### 1.1 Estructura de Slots (8 Slots de Sección + 1 Slot Scratch)

`RoomDataRAM` reside en la dirección fija `$0603` de la RAM (`asm/src/ram.asm:1846`):
- Ocupa exactamente 128 bytes contiguos (`$0603` a `$0682`), divididos en **8 slots de 16 bytes** (`$10` bytes por slot).
- Inmediatamente a continuación, en `$0683` (`byte_683`, `asm/src/ram.asm:1975`), se encuentra el **Slot 8 (Scratch Slot)** de 16 bytes, reservado para salas especiales que no forman parte de la sección precargada (p. ej. Salas de Bombas `$90..$9F` o transiciones directas).

```
+---------------------------------------------------------------------------------------------------+
| Offset Relativo | Dirección RAM | Slot ID | Propósito                                             |
+---------------------------------------------------------------------------------------------------+
| +$00            | $0603-$0612   | Slot 0  | Sala 0 de la sección actual (primera sala visitable)  |
| +$10            | $0613-$0622   | Slot 1  | Sala 1 de la sección actual                           |
| +$20            | $0623-$0632   | Slot 2  | Sala 2 de la sección actual                           |
| +$30            | $0633-$0642   | Slot 3  | Sala 3 de la sección actual                           |
| +$40            | $0643-$0652   | Slot 4  | Sala 4 de la sección actual                           |
| +$50            | $0653-$0662   | Slot 5  | Sala 5 de la sección actual                           |
| +$60            | $0663-$0672   | Slot 6  | Sala 6 de la sección actual                           |
| +$70            | $0673-$0682   | Slot 7  | Sala 7 de la sección actual                           |
| +$80            | $0683-$0692   | Slot 8  | Scratch Slot (byte_683): Salas de Bombas / Transitorias|
+---------------------------------------------------------------------------------------------------+
```

### 1.2 Layout Interno de un Slot de 16 Bytes

Cada slot de 16 bytes tiene la siguiente distribución estricta (`asm/src/prg.asm:3240-3279, 3354, 3120-3122`):

```
Byte 0:       [RoomID] (1 byte)
              - ID de la sala asignada al slot (p. ej. 1 para Room 1, 2 para Room 2).
              - Si es 0x00, el slot está vacío o marca el fin de la lista de salas de la sección.

Byte 1:       [Unused / Padding] (1 byte)
              - En ReadRoomData (línea 3267), byte_E se inicializa a (slot_offset + 1).
              - En HandleRoomDataObject (línea 3354), antes de escribir el primer objeto,
                se ejecuta obligatoriamente INC byte_E, por lo que el primer objeto se
                escribe en (slot_offset + 2). Byte 1 nunca es sobreescrito.

Bytes 2..14:  [ObjectStates] (hasta 13 objetos interactivos por sala)
              - Almacena el estado dinámico en tiempo de ejecución de cada objeto:
                bombas, cofres, puertas, bloques multi-hit, ítems expuestos.
              - NO almacena coordenadas YX (las coordenadas permanecen fijas en ROM).
              - Almacena el tipo de ítem según ItemToTileTable o flags de estado.

Byte 15:      [$FF Terminator] (1 byte)
              - Marca de fin de objetos en el slot (escrito por ReadRoomData al encontrar $FF en ROM,
                línea 3279). Puede ubicarse antes del byte 15 si la sala tiene menos objetos.
```

### 1.3 Ciclo de Vida: ¿Cuándo se copia de ROM a RAM y cuándo se preserva RAM?

#### Precarga de Sección Completa (`GameState_2_LoadRoom`, `asm/src/prg.asm:3150-3251`)
Cuando el jugador entra a una sala cuya cabecera `$Ex` define una nueva sección (`byte 2 & 0x7F = section_id`):
1. `_SpecialBitSet` (línea 3194) toma el ID de sección, consulta `SectionRoomsTable` (`asm/src/data.asm:640`) y obtiene el puntero a la lista de salas (p. ej. `Section_1_1: .BYTE 1, 2, 3, 4, 5, 6, 0`).
2. Itera sobre cada sala de la lista con `byte_5F = 0..N`:
   - Escribe el `RoomID` en el Byte 0 del slot: `RoomDataRAM[byte_5F << 4] = RoomID` (línea 3243).
   - Apunta `off_58` al bytecode ROM de esa sala con `LoadRoomData` (línea 3233).
   - Invoca `ReadRoomData` (línea 3245), que decodifica el bytecode ROM y rellena los bytes 2..14 del slot con sus estados iniciales.
3. **Resultado**: Las 6 o 7 salas que componen el bloque/sección son inicializadas en RAM simultáneamente en un solo pase.

#### Preservación de RAM al cambiar de sala (`IfNotTitleScreenRoomDoThings`, `asm/src/prg.asm:3077-3135`)
Al ingresar a una sala durante el juego:
1. `loc_92FD` busca `CurrentRoomID` comparando secuencialmente contra el Byte 0 de cada slot de `RoomDataRAM`:
   ```asm
   loc_930C:
       CMP RoomDataRAM,Y      ; Y = 0x00, 0x10, 0x20...
       BEQ loc_9330           ; ¡ENCONTRADA EN RAM!
       DEC byte_F
       BEQ loc_931F           ; No encontrada en los slots de sección
       TYA
       ADC #$10
       TAY
       BNE loc_930C
   ```
2. **Si la sala ya está en un slot (`loc_9330`)**:
   - `LDA #0; STA byte_683` (limpia el ID del scratch slot).
   - `INY; INY; STY byte_5A` (`byte_5A = slot_offset + 2`).
   - **NO SE VUELVE A LEER DE ROM**. Todo el estado persistido en RAM (bombas recolectadas marcadas como `$0F`, cofres abiertos, puertas desbloqueadas) se preserva intacto 1:1.
3. **Si la sala NO está en los slots de sección (`loc_931F`)**:
   - Ocurre con Salas de Bombas secretas (`$90..$9F`) o salas fuera de la sección activa.
   - Utiliza el Slot 8 en offset `$80` (`$0683` / `byte_683`).
   - Si `RoomDataRAM[0x80] != CurrentRoomID`, escribe el nuevo ID y llama a `ReadRoomData` para inicializar el slot scratch.
   - Fija `byte_5A = 0x80 + 2 = 0x82` (`$0685`).

---

## 2. Bytecode de RoomData: Especificación Completa de Comandos ($00 a $FF)

Los bytecodes de salas están codificados como un flujo de instrucciones de longitud variable. La tabla de despacho principal se ejecuta en `HandleRoomDataObject` (`asm/src/prg.asm:3347-3377`) según el nibble superior `(byte >> 4)`:

```
+----------------------------------------------------------------------------------------------------+
| Rango Opcode | Handler ASM       | Tipo de Objeto / Comando           | Formato de Argumentos      |
+----------------------------------------------------------------------------------------------------+
| $01..$0F     | JT_94AD_0         | Grupo de Bombas                    | 1 byte cmd + N bytes YX    |
| $10..$2F     | JT_94AD_multi     | Cofre Rojo Visible                 | 1 byte cmd + 1 byte YX*    |
| $30..$4F     | JT_94AD_multi     | Cofre Naranja Visible              | 1 byte cmd + 1 byte YX*    |
| $50..$6F     | JT_94AD_5_6       | Bloque Oculto Multi-Golpe          | 1 byte cmd + 1 byte YX     |
| $70..$7F     | JT_94AD_7         | Ítem Expuesto Directo              | 1 byte cmd + 1 byte YX     |
| $80..$8F     | JT_94AD_8_Door    | Puerta de Salida                   | 1 byte cmd + 1 byte arg    |
| $90..$9F     | JT_94AD_multi     | Cofre Rojo Oculto (Invisible)      | 1 byte cmd + 1 byte YX*    |
| $A0..$AF     | JT_94AD_multi     | Cofre Naranja Oculto (Invisible)   | 1 byte cmd + 1 byte YX*    |
| $B0..$DF     | JT_94AD_rts       | Comandos Especiales / Bomba Fuego  | 1 byte cmd + 1 byte pos    |
| $E0..$EF     | JT_94AD_E         | Cabecera de Sala / Sección         | 3 bytes fijos              |
| $FF          | loc_9429          | Delimitador Fin de Bytecode        | 0 bytes args (terminador)  |
+----------------------------------------------------------------------------------------------------+
* Si el nibble bajo es 6 (Esfinge), incluye 1 byte extra de ID de evento.
```

### 2.1 Cabecera de Sala ($E0..$EF)
Aparece como el primer comando de la sala principal de cada sección (`asm/src/prg.asm:3150-3164`).
- **Byte 0 (`$Ex`)**:
  - `bits 7..4 = 0xE`: Identificador de cabecera.
  - `bits 3..0`: Índice de paleta de la sala (`RoomPalette = byte`).
- **Byte 1 (`$00..$07`)**:
  - `bits 2..0`: Tipo de música ambiental de la sala (`RoomMusic = byte & 7`).
- **Byte 2 (`$xx`)**:
  - `bit 7`: Flag de temporizador / checkpoint. Si es 1, reinicia `TimerStatusMaybe` y guarda backup de checkpoint/warp.
  - `bit 6`: Si es 1, aborta la precarga de sección (`INC SomeLoadFlag; RTS`).
  - `bits 4..0`: ID de sección (1-indexado). Accede a `SectionRoomsTable[section - 1]` para cargar la lista de salas asociadas a los 8 slots.
- **Efecto en `ReadRoomData`**: `JT_94AD_E` (línea 3508) salta los 3 bytes sin consumir slot en `RoomDataRAM` (`byte_E` no se incrementa).

### 2.2 Grupo de Bombas ($01..$0F)
Define una secuencia de bombas consecutivas (`asm/src/prg.asm:3291-3305, 3381-3398`).
- **Formato**:
  - `Byte 0`: `$0N` donde `N = 1..15` es el número de bombas en el grupo.
  - `Bytes 1..N`: Coordenadas YX de cada bomba (`Y = nibble alto`, `X = nibble bajo`).
- **Almacenamiento en `RoomDataRAM`**:
  - Bomba estándar sin fuego: Almacena `#$05` (`ItemToTileTable[5] = $27`, tile de bomba).
  - Bomba con mecha encendida (primera bomba de la sala 1): Almacena `#$85` (`$80 | 0x05`, bit 7 activo).
- **Lógica de Asignación de Mecha (`byte_B`)**:
  - En la primera sala de la sección (`byte_5F == 0`), `byte_B = 1` (línea 3304). La **primera bomba** del grupo recibe `$85` (fuego activo), y las demás reciben `$05`.
  - En la última sala antes de una sala de bombas o fin de sección, `byte_B = 2` (línea 3226). La **última bomba** del grupo recibe `$85`.
- **Mapeo Especial en Sala de Bombas ($90..$9F)**:
  - Si `RoomID >= $90`, `byte_C = 1`.
  - Al leer un byte con nibble alto 0 (p. ej. `$00`), fuerza `byte_AC = $18` (24 bombas fijas) y guarda el puntero ROM en `byte_90:byte_91` (`asm/src/prg.asm:3310-3318`).

### 2.3 Cofres Visibles ($10..$2F y $30..$4F)
- **Cofre Rojo Visible (`$1N`, `$2N`)**:
  - `RoomDataRAM` almacena `#$10` (`ItemToTileTable[0x10] = $28`, metatile de cofre rojo).
  - El nibble bajo `N` define el ítem contenido (ver tabla 2.7).
- **Cofre Naranja Visible (`$3N`, `$4N`)**:
  - `RoomDataRAM` almacena `#$11` (`ItemToTileTable[0x11] = $4A`, metatile de cofre naranja).
  - El nibble bajo `N` define el ítem contenido. Requiere nivel Mighty >= 1 para abrirse.
- **Caso Especial Esfinge (`N == 6`)**:
  - Si el comando es `$16`, `$36`, etc., incluye un byte adicional de argumento: `[cmd] [YX] [SphinxEventID]`. `JT_94AD_multi` detecta `AND #$F == 6` e incrementa `byte_F` para saltar el byte de evento durante la precarga (`asm/src/prg.asm:3420-3423`).

### 2.4 Bloques Ocultos Multi-Golpe ($50..$6F)
- `RoomDataRAM` almacena `#$33` (`0x13 | 0x20`, `asm/src/prg.asm:3438-3442`).
- El bit 5 (`0x20`) es el **Flag de Objeto Oculto (Invisible)**. Durante el blitteo (`sub_96F5`), los objetos con bit 5 activo son omitidos y no se dibujan en el nametable.
- El nibble bajo `N` define la cantidad de cabezazos/impactos requeridos para romper/revelar el bloque (`asm/src/prg.asm:5438-5444`):
  - `$61`: Requiere 1 golpe.
  - `$62`: Requiere 2 golpes.

### 2.5 Ítems Expuestos Directamente ($70..$7F)
- Coloca un ítem directamente en el mapa sin cofre (`asm/src/prg.asm:3447-3470`).
- `RoomDataRAM` almacena directamente el nibble bajo `N` (`0x00..0x0E`).
- **Regla de Persistencia de Bolas de Cristal (`N == 9` o `N == $A`)**:
  - Si la bola de cristal correspondiente ya fue recolectada (`BIT PlayerCrystalBallsCollected != 0`, línea 3464), se almacena `#$0F` (vacío). La bola de cristal desaparece permanentemente del juego.

### 2.6 Puertas de Salida ($80..$8F)
- **Formato**: `[cmd $8N] [arg]`
- El nibble bajo `N` (0..15) es el índice en `DoorPositionTileTable` (`asm/src/prg.asm:3823`).
- El byte de argumento determina el tipo y estado inicial de la puerta:
  - `arg & 7` indexa `UnknownDoorTable97B0` (`asm/src/prg.asm:3840`):
    - Entrada 0 (`$12`): Puerta cerrada (bit 6 = 0).
    - Entrada 1 (`$52`): Puerta abierta (bit 6 = 1).
  - El valor de la tabla se almacena en `RoomDataRAM[byte_E]`.
  - Si el valor tiene bit 7 activo (negativo), se registra en `MaybeSphinxFlags[arg >> 3]`.

### 2.7 Cofres Ocultos / Secretos ($90..$AF)
- **Cofre Rojo Oculto (`$9N`)**:
  - `RoomDataRAM` almacena `#$30` (`0x10 | 0x20`: cofre rojo + flag oculto).
- **Cofre Naranja Oculto (`$AN`)**:
  - `RoomDataRAM` almacena `#$31` (`0x11 | 0x20`: cofre naranja + flag oculto).
- El objeto es completamente invisible hasta que Jack impacta el metatile por debajo (`MaybeOpenHiddenBlock`, `asm/src/prg.asm:5392`).

### 2.8 Tabla Maestra de Tipos de Ítems (`ItemToTileTable`, `asm/src/data.asm:888-906`)

```
+-------+---------------+-------------------------+----------------------------------------------+
| Valor | Metatile Tile | Nombre del Ítem         | Efecto al Recolectar                         |
+-------+---------------+-------------------------+----------------------------------------------+
| $00   | $4D           | Bolsa 100 pt            | +100 puntos                                  |
| $01   | $4E           | Bolsa 300 pt            | +300 puntos                                  |
| $02   | $4F           | Bolsa 1000 pt           | +1000 puntos                                 |
| $03   | $47           | Moneda de Oro           | Puntos / GDV bonus                           |
| $04   | $49           | Bebida Mighty (Drink)   | Aumenta nivel de poder Mighty (0->1->2)      |
| $05   | $27           | Bomba                   | Recolección de bombas (+100/+200 pts)        |
| $06   | $34           | Esfinge                 | Evento especial / warp / bonificación        |
| $07   | $48           | Placa Tecmo             | +100.000 puntos / 1-UP extra                 |
| $08   | $8F           | Moneda Mighty           | Contador Mighty (10 monedas = Sala Tortura)  |
| $09   | $4C           | Bola de Cristal 1       | GDV bonus permanente                         |
| $0A   | $4C           | Bola de Cristal 2       | GDV bonus permanente                         |
| $0B   | $63           | Rey (Familia)           | Rescate miembro familia real                 |
| $0C   | $91           | Reina (Familia)         | Rescate miembro familia real                 |
| $0D   | $90           | Princesa (Familia)      | Rescate miembro familia real                 |
| $0E   | $50           | Máscara de Belzebut     | Rescate final (+1.000.000 pts en US)         |
| $0F   | $2F           | Vacío (Recolectado)     | Metatile vacío / nada                        |
| $10   | $28           | Cofre Rojo Visible      | Contenedor interactivo golpeable             |
| $11   | $4A           | Cofre Naranja Visible   | Contenedor interactivo (requiere Mighty >= 1)|
+-------+---------------+-------------------------+----------------------------------------------+
```

### 2.9 Flags de Estado en Tiempo de Ejecución en `RoomDataRAM`
- **Bit 7 (`0x80`)**: Bomba con fuego inicial (`$85`). Permite evaluar en `JTPickup_Bomb` si el jugador inició la recolección por la bomba encendida original.
- **Bit 6 (`0x40`)**:
  - En Bombas: **Mecha Encendida**. Recolectar una bomba con bit 6 activo otorga doble puntuación y enciende la mecha de la siguiente bomba (`ORA #$40` en línea 5101).
  - En Puertas: **Puerta Abierta**. Si bit 6 es 1, la puerta está desbloqueada y transiciona al tocarla (`GameState = 9`, línea 5335). Si es 0, la puerta está cerrada.
- **Bit 5 (`0x20`)**: **Objeto Oculto**. Oculta el objeto durante el renderizado inicial (`sub_96F5`). Se limpia cuando Jack golpea el bloque.
- **Valor `$0F`**: Objeto completamente recolectado / desaparecido. Al redibujar, renderiza metatile vacío `$2F`.

---

## 3. Subrutinas de Decodificación y Blitteo: `sub_9592`, `sub_9696`, `sub_96B7`

### 3.1 Bucle de Renderizado de la Sala (`sub_9696`, `asm/src/prg.asm:3684-3700`)
```asm
sub_9696:
    LDA CurrentRoomID
    JSR IfNotTitleScreenRoomDoThings ; Busca slot o carga scratch; inicializa byte_5A
    LDAc PPUCtrlMirror
    AND #$FB
    STA PPUCTRL
    STAc word_50+1

loc_96A7:
    JSR sub_9592                    ; Decodifica un objeto del bytecode
    LDX #0
    JSR sub_96B7                    ; Blittea Metatile 1 en byte_70
    LDX #1
    JSR sub_96B7                    ; Blittea Metatile 2 en byte_71 (si es puerta)
    JMP loc_96A7
```

### 3.2 Decodificador de Objetos (`sub_9592`, `asm/src/prg.asm:3541-3680`)
1. Comprueba si `(off_58) == $FF`. Si es así, saca la dirección de retorno con `PLA; PLA; RTS` para salir limpiamente del bucle `loc_96A7`.
2. Lee `byte_5C` (comando) y `byte_70` (coordenada YX).
3. Si el comando es un grupo de bombas (`byte_5C & 0xF0 == 0`), establece el contador de repetición `byte_AC = (cmd & 0xF) - 1`. En las siguientes iteraciones, reutiliza `byte_5C = 0` y lee solo la coordenada YX de la siguiente bomba.
4. Consulta `RoomDataRAM[byte_5A]` y extrae:
   - `MaybeCollectedThing = RoomDataRAM[byte_5A]`
   - `MaybeTileCollected = ItemToTileTable[MaybeCollectedThing & 0x1F]`
5. Si el objeto es una puerta (`byte_5C & 0xF0 == 0x80`):
   - Consulta `DoorPositionTileTable` usando `(byte_5C & 0x0F) * 4`.
   - Obtiene dos coordenadas: `byte_70` (primer bloque) y `byte_71` (segundo bloque).
   - Ajusta los metatiles según el estado abierto/cerrado (`RoomDataRAM & 0x40`).
6. Si `byte_5C & 0xF0 < 0xB0`, incrementa `byte_5A` para apuntar al siguiente objeto en el slot de RAM.

### 3.3 Blitteo de Metatiles (`sub_96B7`, `sub_96F5`, `sub_9716`, `sub_9729`)
1. **Filtro de Visibilidad (`sub_96F5`, `asm/src/prg.asm:3746-3769`)**:
   - Si `byte_70,X == 0`, aborta con `PLA; PLA; RTS`.
   - Si `MaybeCollectedThing & 0x20 != 0` (objeto oculto), **no se dibuja** (aborta con `PLA; PLA; RTS`).
2. **Cálculo de Dirección PPU (`sub_92AD` y `sub_92E2`, `asm/src/prg.asm:3022-3071`)**:
   - `YX` se convierte a offset en el nametable:
     $$\text{PPU Offset} = (Y \times 64) + (X \times 2)$$
   - Almacenado en `byte_3EF : byte_3F0`.
3. **Expansión de Metatile 16x16 (`LoadLayoutChunk` / `sub_9716`, `asm/src/prg.asm:2703, 3773`)**:
   - Cada metatile de 16x16 consta de 4 tiles de 8x8 (top-left, top-right, bottom-left, bottom-right).
   - Se leen de la tabla `MetatileDefinitions` (`BasePointer 4`) y se copian a `PPUUpdateBuffer[0..3]`.
4. **Escritura en PPU (`sub_9729`, `asm/src/prg.asm:3789-3820`)**:
   - Escribe los 2 tiles superiores en `$2000 | (byte_3EF:byte_3F0)`.
   - Suma `32` a la dirección PPU y escribe los 2 tiles inferiores en la fila siguiente.

---

## 4. Desglose Exhaustivo Byte por Byte: Sala 1 y Sala 2

### 4.1 Sala 1 (`RoomData_01`, `asm/src/roomdata-us.asm:4-5`)

Bytecode original:
```asm
RoomData_01:
    .BYTE $E0, 0, $81, $8F, 0, 5, $85, $87, $39, $6B, $6D, $10, $C6, $11, $CC, $FF
```

| Byte # | Hex | Interpretación Técnica y Comportamiento en Ensamblador |
|:---:|:---:|:---|
| 0 | `$E0` | **Cabecera (Byte 0)**: Nibble alto `$E` (Header). Nibble bajo `$0` -> Paleta de la sala 0 (`RoomPalette = $E0`). |
| 1 | `$00` | **Cabecera (Byte 1)**: Tipo de música ambiental = 0 (`RoomMusic = 0`). |
| 2 | `$81` | **Cabecera (Byte 2)**: Bit 7 activo -> guarda checkpoint/warp; bits 6..0 = `$01` -> Sección 1 (`Section_1_1`). Precarga salas 1..6 en Slots 0..5 de `RoomDataRAM`. |
| 3 | `$8F` | **Puerta de Salida**: Comando `$8` (Puerta). Nibble `$F` -> Entrada 15 en `DoorPositionTileTable` (offset `$3C`: `.BYTE $B0, $C0, $8B, $97`). Puerta lateral vertical en Y=11/12, X=0. |
| 4 | `$00` | **Argumento de Puerta**: `0 & 7 = 0` -> `UnknownDoorTable97B0[0] = $12` (puerta cerrada). Se escribe en `RoomDataRAM[Slot0 + 2] = $12`. |
| 5 | `$05` | **Grupo de Bombas**: Comando `$0` (Bombas). Cantidad = 5 bombas consecutivas. Como Sala 1 es el inicio de la sección (`byte_5F == 0`), `byte_B = 1` (primera bomba encendida). |
| 6 | `$85` | **Bomba 1 (Mecha Encendida)**: Coordenada Y=8, X=5. `RoomDataRAM[Slot0 + 3] = $85` (`$80 | 0x05`, bomba con mecha). |
| 7 | `$87` | **Bomba 2 (Apagada)**: Coordenada Y=8, X=7. `RoomDataRAM[Slot0 + 4] = $05` (bomba sin mecha). |
| 8 | `$39` | **Bomba 3 (Apagada)**: Coordenada Y=3, X=9. `RoomDataRAM[Slot0 + 5] = $05`. |
| 9 | `$6B` | **Bomba 4 (Apagada)**: Coordenada Y=6, X=11. `RoomDataRAM[Slot0 + 6] = $05`. |
| 10 | `$6D` | **Bomba 5 (Apagada)**: Coordenada Y=6, X=13. `RoomDataRAM[Slot0 + 7] = $05`. |
| 11 | `$10` | **Cofre Rojo Visible 1**: Comando `$1` (Cofre Rojo Visible). Contenido: `$0` (Bolsa de 100 pt). `RoomDataRAM[Slot0 + 8] = $10`. |
| 12 | `$C6` | **Coordenada Cofre 1**: Y=12, X=6. |
| 13 | `$11` | **Cofre Rojo Visible 2**: Comando `$1` (Cofre Rojo Visible). Contenido: `$1` (Bolsa de 300 pt). `RoomDataRAM[Slot0 + 9] = $10`. |
| 14 | `$CC` | **Coordenada Cofre 2**: Y=12, X=12. |
| 15 | `$FF` | **Delimitador Fin de Sala**: Escribe `$FF` en `RoomDataRAM[Slot0 + 10]` y finaliza la decodificación. |

**Estado resultante de `RoomDataRAM` para Sala 1 (Slot 0: `$0603..$0612`)**:
```
$0603: 01 (RoomID = 1)
$0604: 00 (Unused padding)
$0605: 12 (Puerta 8F, cerrada)
$0606: 85 (Bomba 1, fuego encendido)
$0607: 05 (Bomba 2, apagada)
$0608: 05 (Bomba 3, apagada)
$0609: 05 (Bomba 4, apagada)
$060A: 05 (Bomba 5, apagada)
$060B: 10 (Cofre rojo 1)
$060C: 10 (Cofre rojo 2)
$060D: FF (Fin de objetos)
$060E..$0612: [Sin usar en Slot 0]
```

---

### 4.2 Sala 2 (`RoomData_02`, `asm/src/roomdata-us.asm:6-7`)

Bytecode original:
```asm
RoomData_02:
    .BYTE 6, $41, $43, $67, $69, $8D, $8F, $18, $C2, $10, $C8, $18, $CE, $FF
```

| Byte # | Hex | Interpretación Técnica y Comportamiento en Ensamblador |
|:---:|:---:|:---|
| 0 | `$06` | **Grupo de Bombas**: No tiene cabecera `$Ex` porque pertenece a la Sección 1 precargada. Comando `$0` (Bombas). Cantidad = 6 bombas. `byte_5F == 1`, por lo que `byte_B = 0` (todas las bombas inician apagadas, `$05`). |
| 1 | `$41` | **Bomba 1 (Apagada)**: Coordenada Y=4, X=1. `RoomDataRAM[Slot1 + 2] = $05`. |
| 2 | `$43` | **Bomba 2 (Apagada)**: Coordenada Y=4, X=3. `RoomDataRAM[Slot1 + 3] = $05`. |
| 3 | `$67` | **Bomba 3 (Apagada)**: Coordenada Y=6, X=7. `RoomDataRAM[Slot1 + 4] = $05`. |
| 4 | `$69` | **Bomba 4 (Apagada)**: Coordenada Y=6, X=9. `RoomDataRAM[Slot1 + 5] = $05`. |
| 5 | `$8D` | **Bomba 5 (Apagada)**: Coordenada Y=8, X=13. `RoomDataRAM[Slot1 + 6] = $05`. |
| 6 | `$8F` | **Bomba 6 (Apagada)**: Coordenada Y=8, X=15. `RoomDataRAM[Slot1 + 7] = $05`. |
| 7 | `$18` | **Cofre Rojo Visible 1**: Comando `$1` (Cofre Rojo Visible). Contenido: `$8` (Moneda Mighty). `RoomDataRAM[Slot1 + 8] = $10`. |
| 8 | `$C2` | **Coordenada Cofre 1**: Y=12, X=2. |
| 9 | `$10` | **Cofre Rojo Visible 2**: Comando `$1` (Cofre Rojo Visible). Contenido: `$0` (Bolsa de 100 pt). `RoomDataRAM[Slot1 + 9] = $10`. |
| 10 | `$C8` | **Coordenada Cofre 2**: Y=12, X=8. |
| 11 | `$18` | **Cofre Rojo Visible 3**: Comando `$1` (Cofre Rojo Visible). Contenido: `$8` (Moneda Mighty). `RoomDataRAM[Slot1 + 10] = $10`. |
| 12 | `$CE` | **Coordenada Cofre 3**: Y=12, X=14. |
| 13 | `$FF` | **Delimitador Fin de Sala**: Escribe `$FF` en `RoomDataRAM[Slot1 + 11]` y finaliza la decodificación. |

**Estado resultante de `RoomDataRAM` para Sala 2 (Slot 1: `$0613..$0622`)**:
```
$0613: 02 (RoomID = 2)
$0614: 00 (Unused padding)
$0615: 05 (Bomba 1, apagada)
$0616: 05 (Bomba 2, apagada)
$0617: 05 (Bomba 3, apagada)
$0618: 05 (Bomba 4, apagada)
$0619: 05 (Bomba 5, apagada)
$061A: 05 (Bomba 6, apagada)
$061B: 10 (Cofre rojo 1, Moneda Mighty)
$061C: 10 (Cofre rojo 2, Bolsa 100)
$061D: 10 (Cofre rojo 3, Moneda Mighty)
$061E: FF (Fin de objetos)
$061F..$0622: [Sin usar en Slot 1]
```

---

## 5. Directrices de Implementación para C (`c_implementer`)

1. **Definición de Constantes y Estructuras en C**:
   ```c
   #define ROOM_DATA_RAM_SLOTS 8
   #define ROOM_DATA_RAM_SLOT_SIZE 16
   #define ROOM_DATA_SCRATCH_SLOT 8

   typedef struct {
       uint8_t room_id;       /* Byte 0 */
       uint8_t padding;       /* Byte 1 (unused) */
       uint8_t objects[14];   /* Bytes 2..15: estados de objetos terminados en 0xFF */
   } RoomDataSlot;

   typedef struct {
       RoomDataSlot slots[ROOM_DATA_RAM_SLOTS];
       RoomDataSlot scratch;  /* Corresponde a $0683 */
   } RoomDataRAMState;
   ```
2. **Flags de Objetos**:
   ```c
   #define OBJ_STATE_LIT_INITIAL   0x80
   #define OBJ_STATE_LIT_FUSE      0x40
   #define OBJ_STATE_DOOR_OPEN     0x40
   #define OBJ_STATE_HIDDEN        0x20
   #define OBJ_STATE_COLLECTED     0x0F
   ```
3. **Decodificación de Coordenadas PPU**:
   ```c
   /* sub_92AD / sub_92E2 */
   uint16_t ppu_offset = ((yx >> 4) * 64) + ((yx & 0x0F) * 2);
   ```
4. **Encadenamiento de Bombas**:
   - Al recolectar una bomba con `OBJ_STATE_LIT_FUSE` (`0x40`), buscar linealmente el siguiente objeto con tipo `0x05` en el slot actual. Si se encuentra, aplicar `|= 0x40`.
   - Si se llega a `0xFF`, avanzar al slot siguiente de la misma sección (`slot_index + 1`) y buscar la siguiente bomba para encender su mecha.
