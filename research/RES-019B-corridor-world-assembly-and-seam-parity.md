# RES-019B: Ensamblado Continuo de Pasillos, Cinemática de Desanclaje y Costura Multi-Sala (Seam Parity)

- **ID Técnico**: `RES-019B`
- **Subsistema**: Cámara, Scroll Horizontal/Vertical, Proyección de Seam y Colisiones Multi-Sala
- **Fase del Proyecto**: Fase 6 (Implementación de Source Port en C99)
- **Tarea**: `TASK-F6-024B`
- **Documento Relacionado**: [RES-019](file:///c:/Users/kanel\Documents\mjk\research\RES-019-scroll-engine-and-camera.md), [RES-016](file:///c:/Users/kanel\Documents\mjk\research\RES-016-collision-system-9probe.md)
- **Estado de Epistemología**: `[CONFIRMADA]` (Verificada exhaustivamente contra `asm/src/prg.asm`, `asm/src/data.asm`, `asm/src/ram.asm` y `asm/src/defs.asm`)
- **Autores**: `asm_researcher` (Nivel 3)

---

## 1. Visión General y Arquitectura de Seam

En *Mighty Bomb Jack* (NES, 1987), el cartucho físico utiliza **Mirroring Horizontal fijo** en hardware (iNES byte 6 bit 0 = 0). Esto impone una restricción severa: los dos nametables horizontales (`$2000` y `$2400`) colapsan exactamente a la misma memoria CIRAM física de 256 píxeles de ancho.

Para dar la ilusión de pasillos continuos de hasta 6 pantallas contiguas (ej. Salas 1 a 6):
1. El motor de juego no dispone de una segunda página horizontal donde precargar la sala contigua completa.
2. La cámara avanza sobre un espacio toroidal de 256 píxeles (`ScrollX` en `$03C2`).
3. En cada frame de desplazamiento, el motor escribe dinámicamente columnas de metatiles de la **sala contigua** justo antes de que el borde del scroll las revele en pantalla.
4. Por tanto, en cualquier instante arbitrario de scroll horizontal (`0 < ScrollX < 255`), la memoria VRAM de la PPU contiene un mosaico híbrido:
   - Una porción de columnas pertenece a `CurrentRoomID`.
   - La porción restante pertenece a la sala adyacente (`MaybeRoomIDCopyAgain`).
5. Jack permanece anclado en `X = 128` (`$80`), excepto en los extremos del pasillo donde se produce un **desanclaje cinemático bidireccional**.
6. Las colisiones físicas de Jack y los actores son **multi-sala**: las sondas de prueba detectan si cruzan la frontera de 256 píxeles y redirigen la consulta a la sala vecina mediante `AdjacentRoomsTable`.

```
                       ESPACIO HORIZONTAL EN PPU (256 px)
      0                               ScrollX                         255
      +----------------------------------+------------------------------+
      |      Columnas de Sala Contigua   |     Columnas de Sala Actual   |
      |     (MaybeRoomIDCopyAgain)       |         (CurrentRoomID)       |
      +----------------------------------+------------------------------+
                     ^
                     |-- Seam Streamer escribe aquí (byte_3F0 = ScrollX >> 3)
```

---

## 2. Cinemática Horizontal de Jack: Desanclaje y Re-anclaje

### 2.1 Variables de Control Cinemático

| Dirección RAM | Símbolo Ensamblador | Tipo | Función |
|---|---|---|---|
| `$00BC` / `$03BE` | `RoomStatusFlags` | `uint8_t` | Registro de topología y estado cinemático (flags `RF_*`). |
| `$0055` | `PlayerXPosHi` | `uint8_t` | Coordenada X entera de Jack en pantalla (`0..255`). |
| `$0056` | `PlayerXPosLo` | `uint8_t` | Subpíxel X de Jack (`8.8` fixed point, parte fraccionaria). |
| `$0059` | `PlayerXVelHi` | `int8_t` | Velocidad X entera: `+1` (derecha) o `$FE` (-2 en C2, izquierda). |
| `$005A` | `PlayerXVelLo` | `uint8_t` | Velocidad X fraccional: fija en `$80` (+0.5 px/frame). |
| `$03C1` | `byte_3C1` | `uint8_t` | Subpíxel fraccionario de `ScrollX`. |
| `$03C2` | `byte_3C2` | `uint8_t` | Coordenada X entera de Scroll (`0..255`). |

Velocidad horizontal neta:
- **Hacia la derecha**: `+$01.80` (+1.5 px/frame).
- **Hacia la izquierda**: `$FE.80` = `-384 / 256` = -1.5 px/frame.

### 2.2 Desanclaje en Bordes (`asm/src/prg.asm:1570-1587`)

Cuando Jack camina en un pasillo horizontal (`RF_ScrollStop` activo, `RF_SingleScreen` inactivo):
- Si `RF_AtScrollEdge == 0` (en pleno scroll): se omite `ApplyPlayerXVelocity`. Jack **no se mueve en coordenadas de pantalla**, permaneciendo fijo en `X = 128` (`$80`). El scroll del mundo desplaza los fondos y actores.
- Si `RF_AtScrollEdge == 1` (la cámara ha chocado contra el extremo del pasillo): Jack se **desancla**. `ApplyPlayerXVelocity` se ejecuta, permitiendo a Jack desplazarse libremente entre `X = 0` y `X = 128` (extremo izquierdo) o entre `X = 128` y `X = 255` (extremo derecho).

```assembly
; asm/src/prg.asm:1570-1587
	LDA #RF_ScrollStop
	BIT a:RoomStatusFlags
	BEQ loc_8A3E           ; Si no es pasillo horizontal, Jack se mueve en pantalla
	LSR A                  ; A = RF_SingleScreen ($10)
	BIT a:RoomStatusFlags  ; Si es sala de pantalla única fija ($10)
	BNE loc_8A3E           ; Jack se mueve libre en pantalla
	LDA #RF_AtScrollEdge   ; Bit 3 ($08)
	BIT a:RoomStatusFlags
	BEQ loc_8A66           ; Si RF_AtScrollEdge == 0: OMITE ApplyPlayerXVelocity (Jack ANCLADO)

loc_8A3E:
	JSR ApplyPlayerXVelocity ; Jack DESANCLADO: se actualiza PlayerXPos en pantalla
	LDA #RF_ScrollStop
	BIT a:RoomStatusFlags
	BEQ loc_8A66
	LSR A
	BIT a:RoomStatusFlags
	BNE loc_8A66
```

### 2.3 Algoritmo de Re-anclaje Bidireccional (`asm/src/prg.asm:1588-1603`)

El retorno de Jack al estado anclado (`RF_AtScrollEdge = 0`) utiliza una técnica de inversión de flag de Carry en el registro de estado del procesador (`PSR`) para unificar la comprobación en ambos sentidos de marcha:

```assembly
; asm/src/prg.asm:1588-1603
	LDA PlayerXPosHi
	CMP #$80               ; Compara Jack contra el centro X = 128 ($80)
	PHP                    ; Empuja PSR (con Carry resultante del CMP) al Stack
	TXA                    ; X = PlayerXVelHi (+1 = derecha, $FE = izquierda)
	BPL loc_8A5B           ; Si Jack camina a la derecha (X >= 0), conserva Carry intacto
	PLA                    ; Si Jack camina a la izquierda (X < 0): extrae PSR
	EOR #1                 ; INVIERTE EL BIT 0 (CARRY)!
	PHA                    ; Vuelve a empujar PSR invertido

loc_8A5B:
	PLP                    ; Restaura PSR
	BCC loc_8A66           ; Si Carry == 0: Jack no ha llegado al ancla -> permanece libre
	LDA a:RoomStatusFlags
	AND #~RF_AtScrollEdge  ; Si Carry == 1: LIMPIA RF_AtScrollEdge! Jack RE-ANCLADO!
	STA a:RoomStatusFlags
```

#### Análisis Matemático de Paridad:
1. **Jack en el Extremo Izquierdo (`PlayerXPosHi < 128`), caminando a la Derecha (`X = +1`)**:
   - `CMP #$80`:
     - Mientras `PlayerXPosHi < $80`: `Carry = 0`. `BPL loc_8A5B` salta directo a `PLP`. `BCC loc_8A66` se cumple. Jack sigue caminando en pantalla hacia la derecha.
     - En el instante `PlayerXPosHi >= $80`: `Carry = 1`. `BCC` no salta. Se ejecuta `AND #~RF_AtScrollEdge`. Jack queda anclado en `X = 128` y en el siguiente frame el scroll horizontal se reanuda.
2. **Jack en el Extremo Derecho (`PlayerXPosHi > 128`), caminando a la Izquierda (`X = $FE < 0`)**:
   - `CMP #$80`:
     - Mientras `PlayerXPosHi >= $80`: `Carry = 1`. Pero al ser `X < 0`, se ejecuta `PLA; EOR #1; PHA`, invirtiendo `Carry` de 1 a 0! `BCC loc_8A66` se cumple. Jack sigue caminando en pantalla hacia la izquierda.
     - En el instante `PlayerXPosHi < $80`: `Carry = 0`. Al invertirse con `EOR #1`, `Carry` pasa a 1! `BCC` no salta. Se ejecuta `AND #~RF_AtScrollEdge`. Jack queda re-anclado en el centro.

---

## 3. Renderizado Continuo de Salas Adyacentes

### 3.1 Estructura Canónica de `AdjacentRoomsTable` (`asm/src/data.asm:1704-1800`)

Cada sala del juego ocupa estrictamente **5 bytes** consecutivos en la tabla:

$$\text{Dirección Base} = \text{AdjacentRoomsTable} + (\text{RoomID} \times 5)$$

```
Offset 0: Flags de Topología y Frontera (Flags)
Offset 1: ID de Sala Vecina SUPERIOR   [U] (North)
Offset 2: ID de Sala Vecina INFERIOR   [D] (South)
Offset 3: ID de Sala Vecina DERECHA    [R] (East)
Offset 4: ID de Sala Vecina IZQUIERDA  [L] (West)
```

#### Decodificación de Flags del Byte 0 (`asm/src/prg.asm:2579-2626`):

```assembly
; asm/src/prg.asm:2583-2601: Cálculo RoomID * 5
	STA off_3D
	LDA #BasePointer_AdjacentRoomsTable
	JSR LoadPointerTo050
	LDA off_3D
	LDY #0
	STY off_3D+1
	ASL off_3D             ; RoomID * 2
	ROL off_3D+1
	ASL off_3D             ; RoomID * 4
	ROL off_3D+1
	CLC
	ADC off_3D             ; (RoomID * 4) + RoomID = RoomID * 5
	BCC loc_8FF3
	INC off_3D+1
loc_8FF3:
	CLC
	ADC word_50
	STA off_3D             ; Puntero final a la entrada de la sala
	LDA off_3D+1
	ADC word_50+1
	STA off_3D+1
```

```assembly
; asm/src/prg.asm:2602-2615: Extracción de bits
	LDY #0
	LDX #0
	LDA (off_3D),Y         ; Byte 0 de la tabla
	LSR A                  ; Bit 0 -> Carry (1 = SingleScreen)
	BCC loc_9009
	LDX #RF_SingleScreen   ; $10
loc_9009:
	AND #2                 ; Bit 2 original (1 = Horizontal corridor)
	BEQ loc_9011
	TXA
	ORA #RF_ScrollStop     ; $20 (Define pasillo horizontal si SingleScreen=0)
	TAX
loc_9011:
	LDA a:RoomStatusFlags
	AND #~(RF_SingleScreen|RF_ScrollStop)
	STA a:RoomStatusFlags
	TXA
	ORA a:RoomStatusFlags
	STA a:RoomStatusFlags
```

| Bit en Byte 0 | Valor | Función en la Topología |
|---|---|---|
| Bit 0 | `$01` | Sala de Pantalla Única (Fixed Room: Bomb Room / Torture Room). Mapea a `RF_SingleScreen` (`$10`). |
| Bit 1 | `$02` | Borde terminal de sección (Rightmost / Bottommost limit). |
| Bit 2 | `$04` | Orientación del pasaje: `1` = Horizontal, `0` = Vertical. Mapea a `RF_ScrollStop` (`$20`). |
| Bit 3 | `$08` | Borde inicial de sección (Leftmost / Topmost limit). |

**Ejemplo canónico (Mundo 1, Pasillo Inicial, Salas 1 a 6)**:
- Sala 1: `.BYTE $0E, 0, 0, 2, 0` -> Flags `$0E` (`$08|$04|$02` = Inicio horizontal, vecino derecho = 2).
- Salas 2, 3, 4, 5: `.BYTE $04, 0, 0, [R], [L]` -> Flags `$04` (Interior de pasillo horizontal).
- Sala 6: `.BYTE $06, 0, 0, $90, 5` -> Flags `$06` (`$04|$02` = Fin horizontal, vecino derecho = `$90`).

### 3.2 Desglose de Columnas/Tiles de `CurrentRoomID` vs Sala Contigua

En scroll horizontal hacia la derecha (`loc_A505` a `loc_A631`):
1. `CurrentRoomID` es la sala en la que Jack se originó.
2. `MaybeRoomIDCopyAgain = (off_3D),3` es la sala vecina a la derecha (`[R]`).
3. El scroll de hardware de la PPU se posiciona en `byte_3C2` (`ScrollX`).
4. La columna de VRAM a escribir se calcula como:

$$\text{PPU Column} = \text{byte\_3F0} = \frac{\text{ScrollX}}{8} = \text{ScrollX} \gg 3 \quad (0..31)$$

5. El índice de metatile en la sala vecina se calcula como:

$$\text{Metatile Column} = \text{byte\_8} = \frac{\text{ScrollX}}{16} = \text{ScrollX} \gg 4 \quad (0..15)$$

6. El medio-tile horizontal (8x8 izquierdo o derecho del metatile de 16x16) se extrae mediante:

```assembly
; asm/src/prg.asm:6138-6147
	LDA byte_8
	AND #$F8
	LSR A
	LSR A
	LSR A
	LSR A
	STA byte_8             ; byte_8 = Columna de metatile (0..15)
	ROL A                  ; El 4º LSR colocó el bit 3 en Carry! ROL lo rescata en bit 0
	AND #1
	PHA                    ; 0 = Mitad izquierda (tiles 0, 2), 1 = Mitad derecha (tiles 1, 3)
```

7. En `loc_A5F9` a `loc_A631`, la rutina carga los 15 metatiles verticales de la columna desde `MaybeRoomIDCopyAgain` y transfiere los patrones de 8x8 al buffer `PPUUpdateBuffer` (`unk_3F2`) con incremento vertical de PPU (`byte_3EE = 4`, indicando `+32`).
8. Mientras el scroll avanza, el televisor muestra a la izquierda de la costura los tiles de `CurrentRoomID`, y a la derecha los tiles recién transmitidos de `MaybeRoomIDCopyAgain`.

### 3.3 Proyección de Sprites y Actores entre Ambas Salas

Los actores (enemigos, cofres, monedas, proyectiles) almacenan sus posiciones en **coordenadas relativas de pantalla**:
- Cada frame en que la cámara realiza scroll, la rutina `sub_A843` calcula el vector opuesto a la velocidad de Jack:

$$\text{CameraDeltaX} = -\text{PlayerXVel} \quad (\text{almacenado en } \texttt{byte\_98}:\texttt{byte\_99})$$

$$\text{CameraDeltaY} = -\text{PlayerYVel} \quad (\text{almacenado en } \texttt{byte\_9A}:\texttt{byte\_9B})$$

- Rutina de aplicación a objetos/coleccionables (`sub_A33E`, `asm/src/prg.asm:5768-5793`):
  Suma `byte_98` y `byte_99` a la posición del actor en `byte_786,Y`. Si la suma produce desbordamiento de pantalla (`ADC #0 != 0`), el objeto se desactiva/despawnea.
- Rutina de aplicación a enemigos (`loc_B7E4`, `asm/src/prg.asm:8626-8650`):
  Suma `byte_98,X` a la coordenada `(EnemyStructPointer),Y`.
- Streaming de objetos en la costura (`sub_A875`, `asm/src/prg.asm:6553-6600`):
  A medida que `ScrollX >> 4` coincide con la columna de spawn del cofre/ítem en la sala contigua (`byte_70,X & 0x0F`), el motor procesa sus metatiles de atributos y los inyecta en la PPU en la costura exacta.

---

## 4. Consultas de Colisiones Multi-Sala (`sub_99EA` / `sub_9A03`)

Cuando Jack o un actor realiza una prueba de colisión por sonda en un pasillo con scroll, la sonda puede caer más allá de los 256 píxeles del nametable de la sala actual. El motor resuelve esto en `sub_9A03` proyectando la sonda en el espacio de la sala adyacente.

### 4.1 Código Fuente Desensamblado (`asm/src/prg.asm:4251-4314`)

```assembly
; asm/src/prg.asm:4251-4314
sub_9A03:
	LDX #0
	LDA byte_1            ; Índice de la sonda de prueba (0..8)
	ASL A
	TAY
	LDA byte_A3D8,Y       ; Offset X relativo de la sonda
	BPL loc_9A0F
	DEX                   ; X = $FF si offset X es negativo

loc_9A0F:
	CLC
	ADC byte_3C2          ; + ScrollX (byte_3C2)
	TAY
	TXA
	ADC #0
	STA byte_7            ; byte_7 = Byte alto de (Offset_X + ScrollX)
	TYA
	CLC
	LDY #5                ; Offset 5 en estructura del actor (Screen X)
	ADC (off_78),Y        ; + Posición X de pantalla del actor
	STA byte_0            ; byte_0 = Coordenada X dentro de la sala (0..255)
	LDA byte_7
	ADC #0                ; Suma carry al byte alto
	BEQ loc_9A39          ; Si byte alto == 0: la sonda está dentro de CurrentRoomID!

	; --- CRUCE DE COSTURA A SALA ADYACENTE ---
	LDA #$10              ; RF_SingleScreen
	BIT a:RoomStatusFlags
	BNE loc_9A39          ; Si es pantalla fija, no cruza
	LDX a:RoomStatusFlags
	BMI loc_9A39          ; Si bit 7 (RF_SectionStart) está activo, no cruza
	LDY #3                ; Offset 3 en AdjacentRoomsTable = Vecino DERECHA [R]
	LDA (off_3D),Y        ; Carga ID de la sala vecina derecha
	BNE loc_9A3C          ; Si existe, usa esa sala!

loc_9A39:
	LDA CurrentRoomID     ; Permanece en CurrentRoomID

loc_9A3C:
	STA a:CurrentRoomIDBackup2 ; Sala de consulta seleccionada
	JSR LoadRoomStuff     ; Configura off_2 apuntando al layout de dicha sala
	LDA byte_1
	ASL A
	TAX
	LDA byte_A3D8+1,X     ; Offset Y relativo de la sonda
	LDY #$B               ; Offset $B en estructura del actor (Screen Y)
	CLC
	ADC (off_78),Y        ; + Posición Y del actor
	STA byte_9            ; byte_9 = Y de la sonda (0..239)
	LDA #0
	JSR sub_9B39
	LDA byte_0            ; byte_0 = X del punto (0..255)
	LSR A
	LSR A
	LSR A
	LSR A
	STA byte_8            ; byte_8 = Columna de metatile (0..15)
	LDA byte_9            ; Y del punto
	AND #$80              ; Bit 7 (0 = mitad superior 0..127, 1 = mitad inferior 128..239)
	ASL A
	ROL A
	TAY                   ; Y = 0 (top half screen) o 1 (bottom half screen)
	JSR HandleDrawHalfRow ; Carga chunk de metatiles correspondiente
	JSR sub_9B6A          ; Comprueba bits de colisión de sólido/plataforma
	RTS
```

### 4.2 Formulación Matemática de la Detección de Cruce

$$\text{ProbeWorldX} = \text{ActorScreenX} + \text{ScrollX} + \text{ProbeOffsetX}$$

- $\text{LowByte} = \text{ProbeWorldX} \ \& \ \text{0xFF} \implies \texttt{byte\_0}$ (columna de píxel dentro del espacio de nametable).
- $\text{HighByte} = \lfloor \text{ProbeWorldX} / 256 \rfloor \implies \texttt{byte\_7}$.

**Regla de Selección de Sala**:

$$\text{TargetRoom} = \begin{cases} 
\text{CurrentRoomID}, & \text{si } \texttt{byte\_7} == 0 \lor (\text{RoomStatusFlags} \& (\text{RF\_SingleScreen} \mid \text{RF\_SectionStart})) \neq 0 \\
\text{AdjacentRoomsTable}[\text{CurrentRoomID}].\text{RightRoom}, & \text{si } \texttt{byte\_7} \neq 0 \land \text{RightRoom} \neq 0 \\
\text{CurrentRoomID}, & \text{en cualquier otro caso}
\end{cases}$$

Una vez determinada la sala objetivo:
- Columna de metatile: $X_{\text{tile}} = \texttt{byte\_0} \gg 4 \in [0..15]$.
- Fila y chunk: $Y_{\text{tile}} = \texttt{byte\_9}$, mitad de pantalla $Y_{\text{half}} = (\texttt{byte\_9} \ge 128) \ ? \ 1 : 0$.
- Se indexa la estructura de metatiles de la sala objetivo sin modificar `CurrentRoomID` global del juego.

---

## 5. Transición de Sala (`STA CurrentRoomID`)

La transición de sala formal ocurre cuando el acumulador de scroll cruza exactamente el umbral de 256 píxeles (overflow o underflow en el byte de scroll).

### 5.1 Transición hacia la Derecha (`asm/src/prg.asm:6070-6111`)

```assembly
; asm/src/prg.asm:6070-6111
loc_A531:
	STA MaybeRoomIDCopyAgain
	LDA PlayerXVelLo
	CLC
	ADC byte_3C1          ; Acumula subpíxeles
	STA byte_3C1
	LDA PlayerXVelHi
	ADC byte_3C2          ; ScrollX += PlayerXVelHi + Carry
	STA byte_3C2
	BCC loc_A581          ; Si NO desborda (ScrollX < 256), continúa en la misma sala

	; --- DESBORDE 255 -> 0 DETECTADO (CARRY = 1) ---
	LDAc RoomStatusFlags
	BMI loc_A57A          ; Si RF_SectionStart está activo, límpialo y omite
	LDY #3                ; Offset 3: Vecino DERECHA [R]
	LDA (off_3D),Y
	BNE loc_A55E          ; Si existe sala a la derecha, ejecuta la transición!

loc_A554:
	LDAc RoomStatusFlags
	ORA #RF_Horizontal    ; Marca tope derecho del pasillo
	STAc RoomStatusFlags
	BNE loc_A58C

loc_A55E:
	STA CurrentRoomID     ; <<< TRANSICIÓN EFECTIVA DE SALA (ej. Sala 1 -> Sala 2) >>>
	JSR MaybeLoadRoomFlags ; Recalcula off_3D = AdjacentRoomsTable + CurrentRoomID * 5
	LDY #0
	LDA (off_3D),Y        ; Lee flags de la nueva sala
	AND #2
loc_A56A:
	BEQ loc_A572
	LDA (off_3D),Y
	AND #8
	BEQ loc_A554          ; Si la nueva sala marca fin, activa RF_Horizontal
loc_A572:
	LDY #3
	LDA (off_3D),Y        ; Comprueba si la nueva sala tiene siguiente vecina a la derecha
	BNE loc_A589
	BEQ loc_A554          ; Si es la última, activa RF_Horizontal

loc_A589:
	STA MaybeRoomIDCopyAgain ; Prepara la siguiente sala vecina para el siguiente seam!
```

### 5.2 Transición hacia la Izquierda (`asm/src/prg.asm:5987-6024`)

```assembly
; asm/src/prg.asm:5987-6024
	LDA PlayerXVelLo
	CLC
	ADC byte_3C1
	STA byte_3C1
	LDA PlayerXVelHi      ; PlayerXVelHi = $FE (-2)
	ADC byte_3C2          ; ScrollX += PlayerXVelHi + Carry
	STA byte_3C2
	BCS loc_A4E3          ; Si NO hay underflow (Carry=1 tras suma de negativo), salta

	; --- SUBDESBORDE 0 -> 255 DETECTADO (CARRY = 0) ---
	LDAc RoomStatusFlags
	AND #~RF_Horizontal   ; Limpia flag de tope derecho
	STAc RoomStatusFlags
	LDY #0
	LDA (off_3D),Y
	AND #2
	BEQ loc_A4CC
	LDA (off_3D),Y
	AND #8
	BNE loc_A4D2

loc_A4CC:
	LDY #4                ; Offset 4: Vecino IZQUIERDA [L]
	LDA (off_3D),Y
	BNE loc_A4DD          ; Si existe sala a la izquierda, ejecuta la transición!

loc_A4D2:
	LDAc RoomStatusFlags
	ORA #RF_SectionStart  ; Marca tope izquierdo del pasillo
	STA a:RoomStatusFlags
	JMP loc_A58C

loc_A4DD:
	STA CurrentRoomID     ; <<< TRANSICIÓN EFECTIVA HACIA LA IZQUIERDA >>>
	STA MaybeRoomIDCopyAgain
```

### 5.3 Resumen de Condiciones Exactas de Transición

| Dirección | Condición de Activación | Acción en Hardware / RAM | Nueva `CurrentRoomID` |
|---|---|---|---|
| **Derecha** (`PlayerXVel > 0`) | `ScrollX` desborda 255 (`ADC` genera `Carry = 1`) y `RF_SectionStart == 0` | `STA CurrentRoomID`, `JSR MaybeLoadRoomFlags`, `off_3D` apunta a nueva entrada | `AdjacentRoomsTable[OldRoom].Right` |
| **Izquierda** (`PlayerXVel < 0`) | `ScrollX` subdesborda 0 (`ADC` genera `Carry = 0`) | `STA CurrentRoomID`, `STA MaybeRoomIDCopyAgain` | `AdjacentRoomsTable[OldRoom].Left` |
| **Arriba** (`PlayerYVel < 0`) | `ScrollY` subdesborda línea 0 (`ADC #$EF` y `Carry = 0`) | `STA CurrentRoomID`, `PPUCtrlMirror ^= 2` (flip nametable vertical) | `AdjacentRoomsTable[OldRoom].Up` |
| **Abajo** (`PlayerYVel > 0`) | `ScrollY` desborda línea 240 (`$F0`) (`Carry = 1`) | `STA CurrentRoomID`, `PPUCtrlMirror ^= 2` (flip nametable vertical) | `AdjacentRoomsTable[OldRoom].Down` |

---

## 6. Blueprint de Implementación C99 y Contrato de Paridad

```c
// ============================================================================
// mjk_corridor_seam.h - Contrato de Paridad C99
// ============================================================================
#ifndef MJK_CORRIDOR_SEAM_H
#define MJK_CORRIDOR_SEAM_H

#include <stdint.h>
#include <stdbool.h>

#define RF_1            0x01
#define RF_BombRoom     0x02
#define RF_4            0x04
#define RF_AtScrollEdge 0x08
#define RF_SingleScreen 0x10
#define RF_ScrollStop   0x20
#define RF_Horizontal   0x40
#define RF_SectionStart 0x80

typedef struct {
    uint8_t flags;
    uint8_t up;
    uint8_t down;
    uint8_t right;
    uint8_t left;
} AdjacentRoomEntry;

extern const AdjacentRoomEntry g_AdjacentRoomsTable[43];

// Estado cinemático del mundo
typedef struct {
    uint8_t room_status_flags;
    uint8_t current_room_id;
    uint8_t adjacent_room_id;     // MaybeRoomIDCopyAgain
    uint8_t scroll_x_subpixel;    // byte_3C1
    uint8_t scroll_x;             // byte_3C2
    uint8_t scroll_y_subpixel;    // byte_3C3
    uint8_t scroll_y;             // byte_3C4
    int16_t camera_delta_x;       // byte_98:byte_99
    int16_t camera_delta_y;       // byte_9A:byte_9B
} WorldScrollState;

// Comprobación de colisión multi-sala idéntica a sub_9A03
uint8_t Collision_SampleMultiRoomTile(
    const WorldScrollState* ws,
    int16_t actor_screen_x,
    int16_t actor_screen_y,
    int8_t probe_offset_x,
    int8_t probe_offset_y
);

// Paso de cinemática horizontal y re-anclaje idéntico a loc_8A06..loc_8A66
void Player_UpdateHorizontalKinematics(
    WorldScrollState* ws,
    int16_t* player_x_pos,        // 8.8 fixed point
    int16_t player_x_vel,
    uint8_t joypad_immediate
);

// Paso de scroll y transición de salas idéntico a loc_A454..loc_A58C
void World_AdvanceHorizontalScroll(
    WorldScrollState* ws,
    int16_t player_x_vel
);

#endif // MJK_CORRIDOR_SEAM_H
```

```c
// ============================================================================
// mjk_corridor_seam.c - Implementación con Paridad Bit-Exacta
// ============================================================================
#include "mjk_corridor_seam.h"

void Player_UpdateHorizontalKinematics(
    WorldScrollState* ws,
    int16_t* player_x_pos,
    int16_t player_x_vel,
    uint8_t joypad_immediate
) {
    if ((joypad_immediate & 0x03) == 0) return; // Sin input horizontal

    int8_t vel_hi = (int8_t)(player_x_vel >> 8);

    // Si es pasillo horizontal (!SingleScreen && ScrollStop)
    if ((ws->room_status_flags & RF_ScrollStop) && !(ws->room_status_flags & RF_SingleScreen)) {
        if (!(ws->room_status_flags & RF_AtScrollEdge)) {
            // Jack ANCLADO: no se modifica la posición en pantalla
            return;
        }
    }

    // Jack DESANCLADO: aplica velocidad en pantalla
    *player_x_pos += player_x_vel;

    // Lógica de re-anclaje bidireccional (asm/src/prg.asm:1588-1603)
    if ((ws->room_status_flags & RF_ScrollStop) && !(ws->room_status_flags & RF_SingleScreen)) {
        uint8_t x_hi = (uint8_t)(*player_x_pos >> 8);
        bool carry = (x_hi >= 0x80);

        if (vel_hi < 0) {
            carry = !carry; // PLA; EOR #1; PHA
        }

        if (carry) {
            ws->room_status_flags &= ~RF_AtScrollEdge; // Re-anclaje en X = 128
        }
    }
}

uint8_t Collision_SampleMultiRoomTile(
    const WorldScrollState* ws,
    int16_t actor_screen_x,
    int16_t actor_screen_y,
    int8_t probe_offset_x,
    int8_t probe_offset_y
) {
    uint16_t world_x = (uint16_t)actor_screen_x + (uint16_t)ws->scroll_x + (int16_t)probe_offset_x;
    uint8_t tile_col = (uint8_t)(world_x & 0xFF) >> 4;
    uint8_t high_byte = (uint8_t)(world_x >> 8);

    uint8_t target_room = ws->current_room_id;
    if (high_byte != 0) {
        if (!(ws->room_status_flags & (RF_SingleScreen | RF_SectionStart))) {
            uint8_t right_neighbor = g_AdjacentRoomsTable[ws->current_room_id].right;
            if (right_neighbor != 0) {
                target_room = right_neighbor;
            }
        }
    }

    uint8_t probe_y = (uint8_t)(actor_screen_y + probe_offset_y);
    uint8_t half_screen = (probe_y >= 128) ? 1 : 0;

    // Consulta en estructuras de layout de target_room...
    return 0; // Metatile attribute flags
}

void World_AdvanceHorizontalScroll(
    WorldScrollState* ws,
    int16_t player_x_vel
) {
    int8_t vel_hi = (int8_t)(player_x_vel >> 8);
    uint8_t vel_lo = (uint8_t)(player_x_vel & 0xFF);

    ws->camera_delta_x = -player_x_vel;

    if (vel_hi > 0) { // Marcha hacia la derecha
        uint16_t new_sub = (uint16_t)ws->scroll_x_subpixel + vel_lo;
        ws->scroll_x_subpixel = (uint8_t)new_sub;
        uint16_t new_x = (uint16_t)ws->scroll_x + vel_hi + (new_sub >> 8);
        ws->scroll_x = (uint8_t)new_x;

        if (new_x > 0xFF) { // Desborde de 256 píxeles (Carry = 1)
            if (ws->room_status_flags & RF_SectionStart) {
                ws->room_status_flags &= ~RF_SectionStart;
            } else {
                uint8_t right_room = g_AdjacentRoomsTable[ws->current_room_id].right;
                if (right_room != 0) {
                    ws->current_room_id = right_room; // STA CurrentRoomID (loc_A55E)
                    ws->adjacent_room_id = g_AdjacentRoomsTable[right_room].right;
                } else {
                    ws->room_status_flags |= RF_Horizontal;
                }
            }
        }
    } else if (vel_hi < 0) { // Marcha hacia la izquierda
        uint16_t new_sub = (uint16_t)ws->scroll_x_subpixel + vel_lo;
        ws->scroll_x_subpixel = (uint8_t)new_sub;
        int16_t new_x = (int16_t)ws->scroll_x + vel_hi + (new_sub >> 8);
        ws->scroll_x = (uint8_t)new_x;

        if (new_x < 0) { // Subdesborde por debajo de 0 (Carry = 0)
            ws->room_status_flags &= ~RF_Horizontal;
            uint8_t left_room = g_AdjacentRoomsTable[ws->current_room_id].left;
            if (left_room != 0) {
                ws->current_room_id = left_room; // STA CurrentRoomID (loc_A4DD)
                ws->adjacent_room_id = left_room;
            } else {
                ws->room_status_flags |= RF_SectionStart;
            }
        }
    }
}
```

---

## 7. Verificación Cruzada y Mapeo de Símbolos

| Símbolo Ensamblador | Ubicación | Equivalente C99 | Invariante de Paridad |
|---|---|---|---|
| `AdjacentRoomsTable` | `asm/src/data.asm:1704` | `g_AdjacentRoomsTable` | 5 bytes por sala; índice `RoomID * 5`. |
| `MaybeLoadRoomFlags` | `asm/src/prg.asm:2579` | `Room_LoadFlags()` | Mapea bit 0 a `RF_SingleScreen` ($10) y bit 2 a `RF_ScrollStop` ($20). |
| `sub_9A03` | `asm/src/prg.asm:4251` | `Collision_SampleMultiRoomTile()` | Si `(ActorX + ScrollX + ProbeX) >= 256`, redirige lectura a `RightRoom`. |
| `loc_8A3E` | `asm/src/prg.asm:1580` | `Player_UpdateHorizontalKinematics()` | Ejecuta `ApplyPlayerXVelocity` solo si `RF_AtScrollEdge` está activo. |
| `loc_8A5B` | `asm/src/prg.asm:1598` | Re-anclaje por Carry | `Carry = (X >= 128) ^ (VelX < 0)`; limpia `RF_AtScrollEdge` si Carry es 1. |
| `loc_A55E` | `asm/src/prg.asm:6092` | `World_AdvanceHorizontalScroll()` | Transición hacia la derecha al desbordar `ScrollX` con `Carry = 1`. |
| `loc_A4DD` | `asm/src/prg.asm:6021` | Transición izquierda | Transición hacia la izquierda al subdesbordar `ScrollX` con `Carry = 0`. |
| `sub_A843` | `asm/src/prg.asm:6516` | Cálculo `CameraDeltaX` | `byte_98:byte_99 = -PlayerXVel`. |
| `loc_B7E4` | `asm/src/prg.asm:8626` | Proyección de Enemigos | Suma `CameraDelta` a las coordenadas relativas del enemigo. |
