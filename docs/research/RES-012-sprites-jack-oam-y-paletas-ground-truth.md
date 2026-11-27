# RES-012: Sprites de Jack, Ensamblado OAM y Paletas Canónicas (Ground Truth 6502)

- **ID de Investigación**: RES-012
- **Subsistema**: OAM / PPU / Gráficos de Actores / Físicas de Animación de Jack
- **Fuentes Ensamblador Canónicas**: `asm/src/prg.asm`, `asm/src/data.asm`, `asm/src/ram.asm`
- **Estado**: `[CONFIRMADA]`
- **Fecha**: 2026-10-25

---

## 1. Tabla de Sprites de Jack (`SpritesTable` - `asm/src/data.asm:364-411`)

### 1.1 Estructura de `SpritesTable`
La tabla `SpritesTable` se define en `asm/src/data.asm:364`. Consta de 46 entradas (`asm/src/data.asm:366`), indexadas de `$00` a `$2D`.

Cada entrada tiene una longitud fija de **5 bytes**:
- **Byte 0 (`ControlByte`)**: Byte de control de simetría y flipping intrínseco.
  - `$44` (`%01000100`): Sprite con simetría horizontal intrínseca. Los cuadrantes derechos (2 y 4) tienen el bit H-Flip preactivado.
  - `$00` (`%00000000`): Sprite asimétrico de 4 tiles independientes. Mirando a la izquierda se dibuja sin flip; mirando a la derecha se invierten las posiciones y se aplica H-Flip a los 4 tiles.
  - `$40` (`%01000000`): Sprite con H-Flip únicamente en el cuadrante 4 (usado en Jack aplastado/humo).
- **Byte 1**: Tile para el cuadrante 1 (Top-Left mirando a la izquierda).
- **Byte 2**: Tile para el cuadrante 2 (Top-Right mirando a la izquierda).
- **Byte 3**: Tile para el cuadrante 3 (Bottom-Left mirando a la izquierda).
- **Byte 4**: Tile para el cuadrante 4 (Bottom-Right mirando a la izquierda).

Jack es un **metasprite de 16x16 píxeles compuesto por 4 tiles de 8x8**:
- Cuadrante 1: Top-Left (TL, offset `X-8`, `Y-8` respecto al punto ancla).
- Cuadrante 2: Top-Right (TR, offset `X`, `Y-8` respecto al punto ancla).
- Cuadrante 3: Bottom-Left (BL, offset `X-8`, `Y` respecto al punto ancla).
- Cuadrante 4: Bottom-Right (BR, offset `X`, `Y` respecto al punto ancla).

### 1.2 Mapeo Completo de Estados y Tiles de Jack

| Estado / Animación | Índice (`PlayerSprite`) | Cita ROM (`data.asm`) | ControlByte | Tile TL (Q1) | Tile TR (Q2) | Tile BL (Q3) | Tile BR (Q4) | Comportamiento de Ensamblado |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **Idle / De pie frontal** | `$00` | L:365 | `$44` | `$10` | `$10` | `$12` | `$12` | Simétrico. Cabeza `$10`, cuerpo/piernas `$12`. TR y BR usan H-Flip. |
| **Salto / Ascenso vertical** | `$01` | L:366 | `$44` | `$11` | `$11` | `$13` | `$13` | Simétrico. Brazos arriba `$11`, piernas juntas `$13`. TR y BR usan H-Flip. |
| **Planeo / Aleteo / Descenso** | `$02` | L:367 | `$44` | `$14` | `$14` | `$16` | `$16` | Simétrico. Brazos extendidos `$14`, piernas `$16`. TR y BR usan H-Flip. |
| **Salto Especial (Title/Intro)** | `$03` | L:368 | `$44` | `$15` | `$15` | `$17` | `$17` | Simétrico. Pose de salto hacia la pirámide (`prg.asm:7636`). |
| **Caminar Frame 1 / Ascenso diagonal** | `$04` | L:369 | `$00` | `$1C` | `$1D` | `$1E` | `$1F` | Asimétrico. Torso `$1C-$1D`, piernas paso 1 `$1E-$1F`. |
| **Caminar Frame 2** | `$05` | L:370 | `$00` | `$1C` | `$1D` | `$20` | `$21` | Asimétrico. Torso `$1C-$1D`, piernas paso 2 `$20-$21`. |
| **Caída diagonal / Planeo lateral** | `$06` | L:371 | `$00` | `$18` | `$19` | `$1A` | `$1B` | Asimétrico. Jack inclinado en caída lateral (`prg.asm:6938`). |
| **Muerte Giro Frame A** | `$28` | L:405 | `$44` | `$45` | `$45` | `$47` | `$47` | Simétrico. Alterna cada 6 frames en muerte (`prg.asm:1319, 1358-1360`). |
| **Muerte Giro Frame B** | `$29` | L:406 | `$44` | `$4D` | `$4D` | `$4F` | `$4F` | Simétrico. Alterna con `$28`. |
| **Derrotado / Humo / Aplastado** | `$2A` | L:407 | `$40` | `$0F` | `$0F` | `$8A` | `$8A` | Tile `$0F` es transparente; base `$8A` aplastada (`prg.asm:1330`). |

**Inexistencia de Sprite de Agacharse (Crouch)**:
Mighty Bomb Jack **no implementa ningún sprite de agacharse ni sentarse**. La rutina de colisión y aterrizaje en `asm/src/prg.asm:5606-5607` asigna inmediatamente `PlayerSprite = 0` (Idle) y limpia los bits de estar en el aire en `PlayerStruct` (`asm/src/prg.asm:1523-1525`). Si el pad detecta dirección horizontal, pasa instantáneamente a caminar (Sprites 4/5).

---

## 2. Tabla de Atributos y Flipping (`SpriteAttributeTable` - `asm/src/data.asm:588-596`)

### 2.1 Definición de la Tabla
Definida en `asm/src/data.asm:588-596`:
```asm
SpriteAttributeTable:
	.BYTE    0              ; Índice 0: $00 (%00000000)
	.BYTE  $27				; Índice 1: $27 (%00100111)
	.BYTE  $55				; Índice 2: $55 (%01010101)
	.BYTE  $8D				; Índice 3: $8D (%10001101)
	.BYTE  $AA				; Índice 4: $AA (%10101010)
	.BYTE  $72				; Índice 5: $72 (%01110010)
	.BYTE  $FF				; Índice 6: $FF (%11111111)
	.BYTE  $D8				; Índice 7: $D8 (%11011000)
```

La rutina `WriteSprite` toma los bits 3-2 de `TempSpriteAttributesish` (`AND #$C; LSR A; TAX` en `asm/src/prg.asm:2294-2297`) para seleccionar el par `[byte_4E, byte_4F]`:
- **Jack Mirando a la Izquierda** (`byte_3DD = $00` -> `TempSpriteAttributesish = $01`):
  - Offset `X = 0`:
    - `byte_4E = SpriteAttributeTable[0] = $00`
    - `byte_4F = SpriteAttributeTable[1] = $27`
- **Jack Mirando a la Derecha** (`byte_3DD = $04` -> `TempSpriteAttributesish = $05`):
  - Offset `X = 2`:
    - `byte_4E = SpriteAttributeTable[2] = $55`
    - `byte_4F = SpriteAttributeTable[3] = $8D`

### 2.2 Decodificación de Bits de Atributo OAM NES
En `WriteSprite` (`asm/src/prg.asm:2328-2355`), los atributos OAM se generan mediante la combinación de `word_50` y `byte_4E`:
```asm
2328: 	LDAc TempSpriteAttributesish
2329: 	AND #$23             ; Retiene Bit 5 (prioridad) y Bits 1-0 (paleta)
2330: 	ASL A
2331: 	ASL A                ; Desplaza: Bit 5 -> Bit 7, Bits 1-0 -> Bits 3-2
2332: 	STA a:word_50
...
2349: 	LDA a:word_50        ; [ P, 0, 0, 0, Pal1, Pal0, 0, 0 ]
2350: 	LSR byte_4E          ; Bit 0 de byte_4E -> Carry
2351: 	ROR A                ; Carry -> Bit 7, P -> Bit 6, Pal -> Bits 2-1
2352: 	LSR byte_4E          ; Bit 1 de byte_4E -> Carry
2353: 	ROR A                ; Carry -> Bit 7, H_FLIP -> Bit 6, P -> Bit 5, Pal -> Bits 1-0!
2354: 	INX
2355: 	STA SpriteDMAArea,X  ; Escribe Byte 2 del sprite en OAM
```
El byte OAM resultante contiene:
- **Bit 7**: Flip Vertical (`V_FLIP`) procedente del segundo desplazamiento de `byte_4E`.
- **Bit 6**: Flip Horizontal (`H_FLIP`) procedente del primer desplazamiento de `byte_4E`.
- **Bit 5**: Prioridad de fondo (`0` = delante del fondo, `1` = detrás del fondo).
- **Bits 4-2**: `0` (no usados por hardware NES PPU).
- **Bits 1-0**: Selección de Paleta de Sprite (`00`..`11`).

### 2.3 Decodificación de `byte_4F` (Desplazamientos X, Y de los Cuadrantes)
En cada una de las 4 iteraciones (`byte_4B = 1, 2, 3, 4`), `byte_4F` se desplaza dos veces mediante `LSR byte_4F` (`asm/src/prg.asm:2339, 2358`):
- **1er desplazamiento (Y)**: Si Carry = 1, ejecuta `ADC #$F7` (con Carry=1: `$F7 + 1 = $F8 = -8`). Resulta en `TempSpriteY - 8`. Si Carry = 0, no suma nada (`TempSpriteY`).
- **2do desplazamiento (X)**: Si Carry = 1, ejecuta `ADC #$F7` -> `TempSpriteX - 8`. Si Carry = 0, `TempSpriteX`.

**Al mirar a la Izquierda (`byte_4F = $27` = `%00100111`)**:
- Iteración 1: Y bit 0 = 1 (`Y-8`), X bit 1 = 1 (`X-8`) -> **Top-Left (TL)**
- Iteración 2: Y bit 2 = 1 (`Y-8`), X bit 3 = 0 (`X+0`) -> **Top-Right (TR)**
- Iteración 3: Y bit 4 = 0 (`Y+0`), X bit 5 = 1 (`X-8`) -> **Bottom-Left (BL)**
- Iteración 4: Y bit 6 = 0 (`Y+0`), X bit 7 = 0 (`X+0`) -> **Bottom-Right (BR)**

**Al mirar a la Derecha (`byte_4F = $8D` = `%10001101`)**:
- Iteración 1: Y bit 0 = 1 (`Y-8`), X bit 1 = 0 (`X+0`) -> **Top-Right (TR)**
- Iteración 2: Y bit 2 = 1 (`Y-8`), X bit 3 = 1 (`X-8`) -> **Top-Left (TL)**
- Iteración 3: Y bit 4 = 0 (`Y+0`), X bit 5 = 0 (`X+0`) -> **Bottom-Right (BR)**
- Iteración 4: Y bit 6 = 0 (`Y+0`), X bit 7 = 1 (`X-8`) -> **Bottom-Left (BL)**

### 2.4 Comportamiento de Flipping al Mirar a la Derecha
1. Para sprites asimétricos (`ControlByte = $00` como Sprites 4, 5, 6):
   - Al mirar a la izquierda: `byte_4E = $00 ^ $00 = $00`. Ningún tile tiene H-Flip.
   - Al mirar a la derecha: `byte_4E = $55 ^ $00 = $55`. Los cuatro tiles reciben `H_FLIP = 1` y sus posiciones se invierten horizontalmente mediante `$8D`, logrando un reflejo horizontal perfecto del metasprite de 16x16.
2. Para sprites simétricos (`ControlByte = $44` como Sprites 0, 1, 2, 3):
   - Al mirar a la izquierda: `byte_4E = $44 ^ $00 = $44` (`%01000100`). Los cuadrantes derechos (TR y BR) reciben H-Flip.
   - Al mirar a la derecha: `byte_4E = $44 ^ $55 = $11` (`%00010001`). En el nuevo orden (TR en iteración 1, TL en iteración 2), los cuadrantes derechos vuelven a recibir H-Flip y los izquierdos se mantienen normales.

---

## 3. Lógica de Ensamblado OAM (`DrawPlayer` y `WriteSprite`)

### 3.1 Rutina `DrawPlayer` (`asm/src/prg.asm:1640-1654`)
```asm
DrawPlayer:
	LDA PlayerSprite             ; $03DC -> TempSpriteTile ($0038)
	STAc TempSpriteTile
	LDA byte_3DD                 ; $03DD ($00=Izq, $04=Der)
	ORA #1                       ; Activa Bit 0 (Paleta de Sprite 1)
	STAc TempSpriteAttributesish ; -> $0039
	LDA #0                       ; Prioridad máxima OAM
	STAc CurrentSpriteIndex      ; -> $003C
	LDA PlayerXPosHi             ; $03D3 -> TempSpriteX ($003A)
	STAc TempSpriteX
	LDA PlayerYPosHi             ; $03D9 -> TempSpriteY ($003B)
	STAc TempSpriteY
	JSR WriteSprite              ; Ensambla los 4 sprites en OAM
	RTS
```

### 3.2 Rutina `WriteSprite` (`asm/src/prg.asm:2267-2374`)
1. **Offset OAM**: Calcula `SpriteOffset = CurrentSpriteIndex << 2` (`prg.asm:2268-2271`). Para Jack, `CurrentSpriteIndex = 0`, por lo que `SpriteOffset = $00`. Jack ocupa invariablemente las primeras 4 entradas OAM (`$0200-$020F`), garantizando la máxima prioridad en pantalla por hardware NES.
2. **Puntero a Datos de Sprite**:
   - Carga el puntero base de `SpritesTable` en `$50-$51` (`prg.asm:2302-2308`).
   - Multiplica `TempSpriteTile` por 5 (`(TempSpriteTile << 2) + TempSpriteTile`) y suma a `$50-$51`, guardando el puntero efectivo en `$4C-$4D` (`prg.asm:2314-2323`).
3. **Control y Flipping**:
   - `byte_4E = (byte_4C)[0] ^ SpriteAttributeTable[X]` (`prg.asm:2325-2327`).
   - Prepara `word_50` con `(TempSpriteAttributesish & $23) << 2` (`prg.asm:2328-2332`).
4. **Bucle de Escritura en OAM** (`prg.asm:2337-2368`):
   Ejecuta 4 iteraciones con `byte_4B = 1..4`:
   - `SpriteDMAArea + 0, X`: Byte Y (`TempSpriteY` con offset de `byte_4F`).
   - `SpriteDMAArea + 1, X`: Byte Tile (`(byte_4C)[byte_4B]`).
   - `SpriteDMAArea + 2, X`: Byte Atributo (Paleta 1, Prioridad y H/V-Flip calculados con `word_50` y `byte_4E`).
   - `SpriteDMAArea + 3, X`: Byte X (`TempSpriteX` con offset de `byte_4F`).
5. **Actualización de OAM**: Al salir, incrementa `CurrentSpriteIndex += 4` (`prg.asm:2369-2372`).
6. **Transferencia DMA en VBlank**:
   En el NMI (`asm/src/prg.asm:101-104`), la CPU dispara la transferencia DMA hacia la PPU:
   ```asm
   LDA #0
   STA OAMADDR
   LDA #2
   STA OAM_DMA       ; Escribe #$02 en $4014 (DMA de $0200-$02FF)
   ```

### 3.3 Variables de RAM Involucradas

| Nombre Variable | Dirección RAM | Cita Definición (`ram.asm`) | Rol en el Sistema |
| :--- | :---: | :---: | :--- |
| `PlayerStruct` | `$03CE` | L:1223-1224 | Estado principal. Bit 1: en el aire (`1`) / suelo (`0`). Bit 7: muerto. |
| `CollisionFlag` | `$03CF` | L:1225-1226 | Flags de colisión con suelo/bloques. |
| `PlayerXVelLo` / `PlayerXVelHi` | `$03D0` / `$03D2` | L:1227, 1231 | Velocidad horizontal subpíxel y entera. |
| `PlayerXPosLo` / `PlayerXPosHi` | `$03D1` / `$03D3` | L:1229, 1233 | Posición horizontal subpíxel y píxel en pantalla. |
| `PlayerYVelLo` / `PlayerYVelHi` | `$03D6` / `$03D8` | L:1238, 1242 | Velocidad vertical subpíxel y entera. |
| `PlayerYPosLo` / `PlayerYPosHi` | `$03D7` / `$03D9` | L:1240, 1244 | Posición vertical subpíxel y píxel en pantalla. |
| `PlayerSprite` | `$03DC` | L:1248-1249 | Índice del sprite actual de Jack en `SpritesTable` (`0`..`$2D`). |
| `byte_3DD` | `$03DD` | L:1250-1251 | Dirección de Jack (`$00` = Izquierda, `$04` = Derecha). |
| `byte_3DF` | `$03DF` | L:1253-1254 | Temporizador de pasos para alternar frames 4 y 5 al caminar. |
| `byte_3E7` | `$03E7` | L:1269-1270 | Subestado de salto/vuelo (`$00` = primer ascenso, `$40` = planeo/flutter/caída). |
| `TempSpriteTile` | `$0038` | L:85-86 | Registro temporal ZP con el índice de sprite a dibujar. |
| `TempSpriteAttributesish` | `$0039` | L:87-88 | Registro temporal ZP de atributos y dirección. |
| `TempSpriteX` / `TempSpriteY` | `$003A` / `$003B` | L:89-92 | Coordenadas temporales ZP de anclaje para el metasprite. |
| `CurrentSpriteIndex` | `$003C` | L:93-94 | Índice del slot actual OAM (0..63). Reseteado a 0 por `DrawPlayer`. |
| `SpriteOffset` | `$004A` | L:120-121 | Puntero a offset dentro de `$0200` (`CurrentSpriteIndex << 2`). |
| `byte_4B` | `$004B` | L:123-124 | Contador de cuadrante del bucle OAM (`1`..`4`). |
| `byte_4C`-`byte_4D` | `$004C`-`$004D` | L:125-128 | Puntero de 16 bits a la entrada actual en `SpritesTable`. |
| `byte_4E` | `$004E` | L:129-130 | Máscara activa de flags H/V Flip. |
| `byte_4F` | `$004F` | L:131-132 | Máscara activa de desplazamientos de coordenadas X, Y. |
| `SpriteDMAArea` | `$0200` | L:656-657 | Buffer de 256 bytes de OAM copiado vía DMA `$4014`. |

---

## 4. Colores Exactos de la Paleta de Sprites de Jack (`MainPalette` - `asm/src/data.asm:258-268`)

### 4.1 Definición de Paletas en ROM
En `asm/src/data.asm:258-268`:
```asm
MainPalette:
	.BYTE  $21,  $F, $26, $30		; background 0
	.BYTE  $21,   8, $27, $38		; background 1
	.BYTE  $21,  $A, $1A, $39		; background 2
	.BYTE  $21,  $F, $16, $30		; background 3

	.BYTE  $21,	$16, $16, $30		; sprite 0 (PPU $3F10-$3F13)
	.BYTE  $21,	$30, $12, $16		; sprite 1 (PPU $3F14-$3F17) -> ¡PALETA DE JACK!
	.BYTE  $21,   8, $16, $30		; sprite 2 (PPU $3F18-$3F1B)
	.BYTE  $21,  $F, $2B, $30		; sprite 3 (PPU $3F1C-$3F1F)
```

En la inicialización y reinicio de sala (`asm/src/prg.asm:1937-1950`), `RestoreDefaultPalettes` copia los 32 bytes de `MainPalette` en `PaletteBuffer` (`$05E3-$0602`), los cuales son transferidos a la PPU en `$3F00-$3F1F` durante el NMI (`asm/src/prg.asm:260-270`).

### 4.2 Subpaleta Asignada a Jack
Jack utiliza de forma fija la **Subpaleta de Sprite 1** (direcciones PPU `$3F14` a `$3F17`).
Esto queda determinado de forma inmutable por la instrucción `ORA #1` en `DrawPlayer` (`asm/src/prg.asm:1644`), que fuerza los bits 1-0 del atributo de sprite a `%01` (Sprite Palette 1).

### 4.3 Valores Hexadecimales NES de la Paleta de Jack
Los 4 colores (transparente + 3 colores visibles) de la Subpaleta de Sprite 1 son:
1. **PPU `$3F14` (Color 0 / Backdrop transparente)**:
   - Valor NES: **`$21`** (Azul claro / Sky Blue, color universal de fondo).
2. **PPU `$3F15` (Color 1 de Sprite 1)**:
   - Valor NES: **`$30`** (Blanco puro / White). Utilizado para los ojos, ribetes y guantes de Jack.
3. **PPU `$3F16` (Color 2 de Sprite 1)**:
   - Valor NES: **`$12`** (Azul medio / Medium Blue). Utilizado para el traje azul y detalles principales del cuerpo.
4. **PPU `$3F17` (Color 3 de Sprite 1 - Color Dinámico Mighty)**:
   - Valor NES por defecto: **`$16`** (Rojo / Vermilion Red). Utilizado para la capa, botas y máscara/casco de Jack.

### 4.4 Modificación Dinámica por Mighty Drink / Power-up (`UpdateMightyPalette`)
Cuando Jack consume una Mighty Drink o utiliza una moneda Mighty, la rutina `UpdateMightyPalette` (`asm/src/prg.asm:1774-1779`) actualiza exclusivamente el **Color 3** (`PaletteBuffer + $17`, PPU `$3F17`):
```asm
UpdateMightyPalette:
	LDA MightyLevelColors,X      ; X = PlayerMightyLevel (0..3)
	STA PaletteBuffer+$17        ; PPU $3F17
	INC a:UpdatePaletteFlag
	RTS
```
La tabla `MightyLevelColors` se ubica en `asm/src/data.asm:306-307`:
```asm
MightyLevelColors:
	.BYTE  $16, $12, $26, $2A
```

- **Mighty Level 0 (Jack Normal)**:
  - Color 3 = **`$16`** (Rojo original).
  - Tríada visible completa: `$30` (Blanco), `$12` (Azul), `$16` (Rojo).
- **Mighty Level 1 (Nivel 1)**:
  - Color 3 = **`$12`** (Azul medio).
  - Tríada visible completa: `$30` (Blanco), `$12` (Azul), `$12` (Azul). (Jack adquiere un look monocromático azul/blanco).
- **Mighty Level 2 (Nivel 2)**:
  - Color 3 = **`$26`** (Naranja/Rosa brillante / Bright Pink-Orange).
  - Tríada visible completa: `$30` (Blanco), `$12` (Azul), `$26` (Naranja/Rosa).
- **Mighty Level 3 (Nivel 3 - Enemigos convertidos en monedas)**:
  - Color 3 = **`$2A`** (Verde brillante / Vivid Green).
  - Tríada visible completa: `$30` (Blanco), `$12` (Azul), `$2A` (Verde).

Al salir del estado Mighty o iniciar una sala (`asm/src/prg.asm:396-400`), el juego restablece `PaletteBuffer+$17` a `$16`.
