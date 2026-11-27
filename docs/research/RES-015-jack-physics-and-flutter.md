# Informe Técnico: Cinemática 6502 de Jack, Salto Canónico de 64 Pasos y Mecánica de Aleteo (Flutter)

- **ID de Investigación**: `RES-015`
- **Tarea Asociada**: `TASK-F6-019` (Subtarea 1)
- **Área Temática**: 02 - Física y Colisiones
- **Fecha**: 2026-10-25
- **Autor / Agente**: `asm_researcher` (Nivel 3)
- **Estado**: `[COMPLETADA]`

---

## 1. Objetivo y Preguntas de Investigación

El objetivo de esta investigación es extraer del desmontaje canónico 6502 de NES (`asm/src/prg.asm` y `asm/src/data.asm`) la totalidad de las tablas numéricas, fórmulas de avance, cálculo de velocidades fijas Q8.8, subrutinas exactas y la mecánica de aleteo (flutter / planeo sostenido) de Jack.

- **Pregunta Principal 1**: ¿Cuáles son los valores canónicos exactos de la tabla de trayectoria de salto `word_C0E3` (64 pasos) y los factores de modulación `word_C163`?
- **Pregunta Principal 2**: ¿Cómo opera exactamente el algoritmo de velocidad vertical en `DoSomethingWithObjYVel` y `sub_8AE8`? ¿Cómo se refleja la tabla mediante `EOR #$FF; AND #$3F` para unificar ascenso y descenso?
- **Pregunta Principal 3**: ¿Cuál es el disparador exacto del aleteo (*flutter* continuo)? ¿Es por pulsación (*just_pressed*) o mantenimiento (*held*) del botón A? ¿Cómo interactúa el opcode `ROR A; ROR A` con el flag de acarreo para inicializar el salto vs el aleteo?
- **Pregunta Principal 4**: ¿Existe aceleración horizontal, inercia o fricción diferenciada entre aire y suelo?
- **Impacto en el Port**: Sustituir el modelo aproximado de gravedad ad-hoc y velocidad horizontal errónea en `src/core/mjk_player.c` y sincronizarlo con el contrato canónico 6502.

---

## 2. Fuentes de Verdad Canónicas (`asm/`)

- **Tablas de Datos**:
  - `asm/src/data.asm:187-251`: Etiqueta `word_C0E3` (Tabla canónica de 64 pasos de salto de 16 bits).
  - `asm/src/data.asm:252-256`: Etiqueta `word_C163` (Modulaciones de avance del acumulador de salto de 16 bits según D-Pad vertical).
- **Subrutinas Cinemáticas**:
  - `asm/src/prg.asm:1397-1447`: `_MightyLevelAPressCheck`, `_NoMightyLevelDown`, `_DontAddPoints` (Disparo de salto y aleteo vía botón A).
  - `asm/src/prg.asm:1460-1532`: Despacho de física vertical, animación de salto/caída y límites de pantalla.
  - `asm/src/prg.asm:1533-1636`: `loc_89EF` a `loc_8A99` (Cinemática horizontal, parada instantánea, animación de caminar y salto lateral).
  - `asm/src/prg.asm:1658-1685`: `DoSomethingWithObjYVel` (Avance del acumulador de 16 bits y escritura de velocidad Q8.8).
  - `asm/src/prg.asm:1689-1709`: `sub_8AE8` (Indexación, reflexión de descenso y signo de velocidad).
  - `asm/src/prg.asm:1719-1728`: `ApplyPlayerXVelocity` (Integrador de posición horizontal Q8.8).
  - `asm/src/prg.asm:1732-1741`: `ApplyPlayerYVelocity` (Integrador de posición vertical Q8.8).
  - `asm/src/prg.asm:2094-2107`: `NegateYYXX` (Negación en complemento a 2 de registro de 16 bits `(Y:X)`).
  - `asm/src/prg.asm:5529-5588`: `sub_A1B9` y `sub_A1EB` (Gestión de flags de suelo y contacto en `PlayerStruct`).
- **Variables de Memoria RAM**:
  - `asm/src/ram.asm:1223-1270`: `PlayerStruct` (`$03CE`), `CollisionFlag` (`$03CF`), `PlayerXVelLo` (`$03D0`), `PlayerXPosLo` (`$03D1`), `PlayerXVelHi` (`$03D2`), `PlayerXPosHi` (`$03D3`), `PlayerYVelLo` (`$03D6`), `PlayerYPosLo` (`$03D7`), `PlayerYVelHi` (`$03D8`), `PlayerYPosHi` (`$03D9`), `PlayerSprite` (`$03DC`), `byte_3DD` (`$03DD`), `byte_3DF` (`$03DF`), `byte_3E6` (`$03E6`), `byte_3E7` (`$03E7`).

---

## 3. Tablas Canónicas Extraídas

### 3.1. Tabla de Trayectoria de Salto de 64 Pasos (`word_C0E3`)

Ubicación: `asm/src/data.asm:187-251`.
Formato original: 64 palabras de 16 bits en formato little-endian (Q8.8 unsigned magnitude).
Unidad física: Subpíxeles por frame ($1 \text{ pixel} = 256 \text{ subpixels} = 0x0100$).

| Paso (Hex) | Paso (Dec) | Valor Hex | Valor Dec (Subpx) | Píxeles / Frame | Fase de Trayectoria |
| :---: | :---: | :---: | :---: | :---: | :--- |
| `$00` | 0 | `$0380` | 896 | 3.500000 | Impulso inicial / Velocidad máxima |
| `$01` | 1 | `$037C` | 892 | 3.484375 | Ascenso rápido |
| `$02` | 2 | `$0378` | 888 | 3.468750 | Ascenso rápido |
| `$03` | 3 | `$0374` | 884 | 3.453125 | Ascenso rápido |
| `$04` | 4 | `$0370` | 880 | 3.437500 | Ascenso rápido |
| `$05` | 5 | `$0368` | 872 | 3.406250 | Ascenso regular |
| `$06` | 6 | `$0360` | 864 | 3.375000 | Ascenso regular |
| `$07` | 7 | `$0358` | 856 | 3.343750 | Ascenso regular |
| `$08` | 8 | `$0350` | 848 | 3.312500 | Ascenso regular |
| `$09` | 9 | `$0348` | 840 | 3.281250 | Ascenso regular |
| `$0A` | 10 | `$0340` | 832 | 3.250000 | Ascenso regular |
| `$0B` | 11 | `$0330` | 816 | 3.187500 | Ascenso gradual |
| `$0C` | 12 | `$0320` | 800 | 3.125000 | Ascenso gradual |
| `$0D` | 13 | `$0310` | 784 | 3.062500 | Ascenso gradual |
| `$0E` | 14 | `$0300` | 768 | 3.000000 | Ascenso gradual |
| `$0F` | 15 | `$02F0` | 752 | 2.937500 | Ascenso intermedio |
| `$10` | 16 | `$02E0` | 736 | 2.875000 | Ascenso intermedio |
| `$11` | 17 | `$02D0` | 720 | 2.812500 | Ascenso intermedio |
| `$12` | 18 | `$02C0` | 704 | 2.750000 | Ascenso intermedio |
| `$13` | 19 | `$02B0` | 688 | 2.687500 | Ascenso intermedio |
| `$14` | 20 | `$02A0` | 672 | 2.625000 | Ascenso sostenido |
| `$15` | 21 | `$0290` | 656 | 2.562500 | Ascenso sostenido |
| `$16` | 22 | `$0280` | 640 | 2.500000 | Ascenso sostenido |
| `$17` | 23 | `$0270` | 624 | 2.437500 | Ascenso sostenido |
| `$18` | 24 | `$0260` | 608 | 2.375000 | Ascenso sostenido |
| `$19` | 25 | `$0250` | 592 | 2.312500 | Ascenso medio |
| `$1A` | 26 | `$0240` | 576 | 2.250000 | Ascenso medio |
| `$1B` | 27 | `$0230` | 560 | 2.187500 | Ascenso medio |
| `$1C` | 28 | `$0220` | 544 | 2.125000 | Ascenso medio |
| `$1D` | 29 | `$0210` | 528 | 2.062500 | Ascenso medio |
| `$1E` | 30 | `$0200` | 512 | 2.000000 | Ascenso medio |
| `$1F` | 31 | `$01F0` | 496 | 1.937500 | Deceleración progresiva |
| `$20` | 32 | `$01E0` | 480 | 1.875000 | Deceleración progresiva |
| `$21` | 33 | `$01D0` | 464 | 1.812500 | Deceleración progresiva |
| `$22` | 34 | `$01C0` | 448 | 1.750000 | Deceleración progresiva |
| `$23` | 35 | `$01B0` | 432 | 1.687500 | Deceleración progresiva |
| `$24` | 36 | `$01A0` | 416 | 1.625000 | Aproximación al ápice |
| `$25` | 37 | `$0190` | 400 | 1.562500 | Aproximación al ápice |
| `$26` | 38 | `$0180` | 384 | 1.500000 | Aproximación al ápice |
| `$27` | 39 | `$0170` | 368 | 1.437500 | Aproximación al ápice |
| `$28` | 40 | `$0160` | 352 | 1.375000 | Aproximación al ápice |
| `$29` | 41 | `$0150` | 336 | 1.312500 | Curvatura media |
| `$2A` | 42 | `$0140` | 320 | 1.250000 | Curvatura media |
| `$2B` | 43 | `$0130` | 304 | 1.187500 | Curvatura media |
| `$2C` | 44 | `$0120` | 288 | 1.125000 | Curvatura media |
| `$2D` | 45 | `$0110` | 272 | 1.062500 | Curvatura media |
| `$2E` | 46 | `$0100` | 256 | 1.000000 | 1 píxel exacto / frame |
| `$2F` | 47 | `$00F0` | 240 | 0.937500 | Deceleración fina |
| `$30` | 48 | `$00E0` | 224 | 0.875000 | Deceleración fina |
| `$31` | 49 | `$00D0` | 208 | 0.812500 | Deceleración fina |
| `$32` | 50 | `$00C0` | 192 | 0.750000 | Deceleración fina |
| `$33` | 51 | `$00B0` | 176 | 0.687500 | Deceleración fina |
| `$34` | 52 | `$00A0` | 160 | 0.625000 | Entrada al ápice |
| `$35` | 53 | `$0090` | 144 | 0.562500 | Entrada al ápice |
| `$36` | 54 | `$0080` | 128 | 0.500000 | Entrada al ápice |
| `$37` | 55 | `$0070` | 112 | 0.437500 | Entrada al ápice |
| `$38` | 56 | `$005E` | 94 | 0.367188 | Zona cuasi-estacionaria |
| `$39` | 57 | `$004C` | 76 | 0.296875 | Zona cuasi-estacionaria |
| `$3A` | 58 | `$003A` | 58 | 0.226562 | Zona cuasi-estacionaria |
| `$3B` | 59 | `$0028` | 40 | 0.156250 | Zona cuasi-estacionaria |
| `$3C` | 60 | `$0010` | 16 | 0.062500 | Suspensión casi total |
| `$3D` | 61 | `$0008` | 8 | 0.031250 | Suspensión casi total |
| `$3E` | 62 | `$0002` | 2 | 0.007812 | Ápice parabólico |
| `$3F` | 63 | `$0001` | 1 | 0.003906 | Cima matemática del salto |

```c
// Definición exacta en C
const uint16_t g_mjk_canonical_jump_table[64] = {
    0x0380, 0x037C, 0x0378, 0x0374, 0x0370, 0x0368, 0x0360, 0x0358,
    0x0350, 0x0348, 0x0340, 0x0330, 0x0320, 0x0310, 0x0300, 0x02F0,
    0x02E0, 0x02D0, 0x02C0, 0x02B0, 0x02A0, 0x0290, 0x0280, 0x0270,
    0x0260, 0x0250, 0x0240, 0x0230, 0x0220, 0x0210, 0x0200, 0x01F0,
    0x01E0, 0x01D0, 0x01C0, 0x01B0, 0x01A0, 0x0190, 0x0180, 0x0170,
    0x0160, 0x0150, 0x0140, 0x0130, 0x0120, 0x0110, 0x0100, 0x00F0,
    0x00E0, 0x00D0, 0x00C0, 0x00B0, 0x00A0, 0x0090, 0x0080, 0x0070,
    0x005E, 0x004C, 0x003A, 0x0028, 0x0010, 0x0008, 0x0002, 0x0001
};
```

---

### 3.2. Tabla de Modulación de Avance (`word_C163`)

Ubicación: `asm/src/data.asm:252-256`.
Indexación: `(Joypad1_Immediate & 0x0C) >> 1` (Offset de byte 0, 2, 4 o 6).
Semántica: Incremento de 16 bits añadido en cada frame al acumulador de salto (`byte_3E7:byte_3E6`).

| Índice | Offset Byte | Condición D-Pad Vertical | Valor Hex | Incremento por Frame | Efecto Físico en Jack |
| :---: | :---: | :--- | :---: | :---: | :--- |
| `0` | `+0` | Neutro (ni Arriba ni Abajo) | `$00F0` | `240/256` = 0.93750 pasos | Salto estándar Tecmo |
| `1` | `+2` | Manteniendo ABAJO (`JP_Down = $04`) | `$0160` | `352/256` = 1.37500 pasos | Salto corto / Caída acelerada |
| `2` | `+4` | Manteniendo ARRIBA (`JP_Up = $08`) | `$00A0` | `160/256` = 0.62500 pasos | Salto muy alto / Planeo prolongado |
| `3` | `+6` | Arriba + Abajo simultáneos (`$0C`) | `$00D0` | `208/256` = 0.81250 pasos | Estado D-Pad simultáneo |

```c
// Definición exacta en C
const uint16_t g_mjk_jump_advance_table[4] = {
    0x00F0, // 0: Neutro
    0x0160, // 1: Abajo
    0x00A0, // 2: Arriba
    0x00D0  // 3: Arriba + Abajo
};
```

---

## 4. Análisis Paso a Paso del Algoritmo Cinemático

### 4.1. Rutina `DoSomethingWithObjYVel` (`asm/src/prg.asm:1658-1685`)

1. **Lectura de Entrada**:
   Recibe en `X` el registro `Joypad1_Immediate`.
   Si Jack no está muerto (`PlayerStruct >= 0`), extrae los bits verticales:
   ```asm
   TXA
   AND #$0C     ; Aisla bits 2 (Down) y 3 (Up)
   LSR A        ; Desplaza a la derecha: valores resultantes {0, 2, 4, 6}
   TAX
   ```
2. **Adición de 16 Bits al Acumulador de Salto**:
   Suma el incremento de `word_C163[X]` al acumulador del objeto (`EnemyStruct_18` y `EnemyStruct_19`, que para Jack son `$03E6` y `$03E7`):
   ```asm
   LDA word_C163,X
   CLC
   LDY #EnemyStruct_18
   ADC (EnemyStructPointer),Y
   STA (EnemyStructPointer),Y   ; accum_lo += advance_lo
   INY
   LDA word_C163+1,X
   ADC (EnemyStructPointer),Y   ; accum_hi += advance_hi + carry
   BPL loc_8AD8
   LDA #$7F                     ; Clamp a 0x7F (127 decimal) si bit 7 se activa
   loc_8AD8:
   STA (EnemyStructPointer),Y
   ```
3. **Cálculo de Velocidad y Almacenamiento**:
   Llama a `sub_8AE8` pasando en `A` el byte alto del acumulador (`accum_hi`, rango `0..127`).
   Almacena el resultado de 16 bits retornado en `(Y:X)` directamente en los campos de velocidad Y del jugador:
   - `EnemyStruct_A_YVelHi` (`PlayerYVelHi`, `$03D8`) $\leftarrow$ `Y`
   - `EnemyStruct_8_YVelLo` (`PlayerYVelLo`, `$03D6`) $\leftarrow$ `X`

---

### 4.2. Rutina `sub_8AE8` (`asm/src/prg.asm:1689-1709`)

```asm
sub_8AE8:
    CMP #$40            ; Compara A con 64 decimal
    PHP                 ; Guarda Carry en la pila (Carry=0: A < 64, Carry=1: A >= 64)
    BCC loc_8AF1        ; Si A < 64, salta (fase de ascenso)
    EOR #$FF            ; Si A >= 64: complemento a 1 (255 - A)
    AND #$3F            ; Máscara de 6 bits (0..63) -> REFLEJO EXACTO

loc_8AF1:
    ASL A               ; Multiplica índice por 2 (palabras de 16 bits)
    TAY
    LDX word_C0E3,Y     ; X = byte bajo de la magnitud
    INY
    LDA word_C0E3,Y
    TAY                 ; Y = byte alto de la magnitud
    PLP                 ; Recupera estado de comparación original
    BCS locret_8B01     ; Si Carry=1 (descenso), retorna magnitud positiva
    JSR NegateYYXX      ; Si Carry=0 (ascenso), niega (Y:X) a complemento a 2

locret_8B01:
    RTS
```

#### Análisis Matemático de la Reflexión `EOR #$FF; AND #$3F`
La rutina convierte simétricamente una única tabla de 64 valores de ascenso en la trayectoria de descenso:
- Para $A \in [0, 63]$ (Ascenso):
  - $\text{índice} = A$.
  - Magnitud extraída: `word_C0E3[A]`.
  - Carry = 0 $\rightarrow$ `NegateYYXX` $\rightarrow$ $V_y = -\text{word\_C0E3}[A]$ (Vector hacia arriba en coordenadas de pantalla NES).
  - Al inicio ($A = 0$): $V_y = -0x0380$ ($-3.5$ px/frame).
  - En el ápice ($A = 63$): $V_y = -0x0001$ ($-0.0039$ px/frame).
- Para $A \in [64, 127]$ (Descenso):
  - $\text{índice} = (A \oplus 0xFF) \ \& \ 0x3F = 63 - (A - 64)$.
  - Al inicio del descenso ($A = 64$): $\text{índice} = (64 \oplus 255) \ \& \ 63 = 191 \ \& \ 63 = 63$. Magnitud: `word_C0E3[63] = 0x0001`.
  - Carry = 1 $\rightarrow$ No niega $\rightarrow$ $V_y = +0x0001$ ($+0.0039$ px/frame).
  - Al final del descenso ($A = 127$): $\text{índice} = (127 \oplus 255) \ \& \ 63 = 128 \ \& \ 63 = 0$. Magnitud: `word_C0E3[0] = 0x0380`.
  - Carry = 1 $\rightarrow$ $V_y = +0x0380$ ($+3.5$ px/frame, velocidad terminal de caída libre).
- Si Jack continúa en el aire tras alcanzar $A = 127$, el clamp `BPL loc_8AD8; LDA #$7F` mantiene $A = 127$ indefinidamente hasta tocar suelo o plataforma, manteniendo la velocidad terminal de $3.5$ px/frame.

---

### 4.3. Rutina `NegateYYXX` (`asm/src/prg.asm:2094-2107`)

Convierte un valor de 16 bits sin signo `(Y:X)` en su representación con signo en complemento a 2:
```asm
NegateYYXX:
    TXA
    EOR #$FF
    TAX
    TYA
    EOR #$FF
    TAY
    INX
    BNE locret_8D2E
    INY
locret_8D2E:
    RTS
```
Equivalente estricto en C:
```c
int16_t Negate16(uint16_t val) {
    return (int16_t)(-(int32_t)val);
}
```

---

## 5. Mecánica del Aleteo Continuo (*Flutter* / Planeo Sostenido)

### 5.1. Detección de Entrada y Disparo de Salto / Aleteo

Ubicación: `asm/src/prg.asm:1397-1447`.
```asm
_MightyLevelAPressCheck:
    LDX #JP_A
    LDA #RF_BombRoom
    BIT a:RoomStatusFlags
    BEQ loc_8901
    LDX #JP_B|JP_A              ; En sala de bombas, tanto A como B saltan

loc_8901:
    TXA
    AND Joypad1_Immediate
    BEQ loc_895A                ; Si no está pulsado A, salta
    AND a:Joypad1_ImmediateCopy
    BNE loc_895A                ; SI YA ESTABA PULSADO EL FRAME ANTERIOR, SALTA
    INC a:APressCounter         ; SOLO SE EJECUTA ANTE NUEVA PULSACIÓN (EDGE-TRIGGERED)
    ...
_NoMightyLevelDown:
    LDA #2
    BIT PlayerStruct            ; ¿Está Jack en el aire o en el suelo?
    SEC                         ; PREPARA CARRY = 1
    BNE _DontAddPoints          ; SI BIT 1 ESTÁ ACTIVO (YA ESTÁ EN EL AIRE), SALTA CON CARRY=1
    ORA PlayerStruct
    STA PlayerStruct            ; Si estaba en suelo, activa bit 1 (entra al aire)
    LDA #Sound_Jump
    STAc JustJumpedFlag
    JSR QueueSound
    CLC                         ; Salto desde suelo: CARRY = 0
    LDA TortureRoomSceneFlag
    BNE _DontAddPoints
    LDA #Score_10
    JSR AddScore
    CLC                         ; CARRY = 0

_DontAddPoints:
    LDA #0
    STA byte_3E6                ; accum_lo = 0
    ROR A                       ; ROTA CARRY DENTRO DEL BIT 7
    ROR A                       ; ROTA BIT 7 DENTRO DEL BIT 6
    STA byte_3E7                ; accum_hi = (Carry ? 0x40 : 0x00)
    LDA #8
    ORA PlayerStruct
    STA PlayerStruct            ; Activa bit 3 (estado de salto/flutter)
```

### 5.2. El Idioma 6502 `ROR A; ROR A` y el Reset del Acumulador

Este fragmento es una de las soluciones más elegantes de la ROM:
1. **Caso 1: Salto Inicial desde el Suelo**:
   - `PlayerStruct & 0x02` era 0 (Jack estaba en el suelo).
   - No salta a `_DontAddPoints`.
   - Limpia el Carry con `CLC` ($C = 0$).
   - Llega a `_DontAddPoints` con $C = 0$.
   - `LDA #0` ($A = 0$).
   - `STA byte_3E6` $\rightarrow$ `accum_lo = 0`.
   - `ROR A; ROR A` con $C=0$ produce $A = 0x00$.
   - `STA byte_3E7` $\rightarrow$ `accum_hi = 0x00`.
   - **Resultado**: El acumulador se fija en `0x0000` (Paso 0 de la tabla: Ascenso a máxima potencia, $-3.5$ px/frame).
2. **Caso 2: Aleteo / Flutter en el Aire**:
   - `PlayerStruct & 0x02` era 1 (Jack ya estaba saltando/en el aire).
   - Ejecuta `SEC` ($C = 1$) y toma la bifurcación `BNE _DontAddPoints` con $C = 1$ intacto.
   - Llega a `_DontAddPoints` con $C = 1$.
   - `LDA #0` ($A = 0$, $C$ no se altera).
   - `STA byte_3E6` $\rightarrow$ `accum_lo = 0`.
   - `ROR A`: El Carry entra en el bit 7 $\rightarrow A = 0x80$, $C = 0$.
   - `ROR A`: El bit 7 entra en el bit 6 $\rightarrow A = 0x40$, $C = 0$.
   - `STA byte_3E7` $\rightarrow$ `accum_hi = 0x40` (64 decimal).
   - **Resultado**: El acumulador se resetea instantáneamente a `0x4000` (Paso 64: Ápice de caída libre).

### 5.3. Efecto Dinámico del Aleteo en el Juego
- En el paso 64 ($0x40$), la velocidad vertical calculada por `sub_8AE8` es exactamente $+0x0001$ ($+0.0039$ px/frame, es decir, virtualmente 0).
- Cada vez que el jugador **pulsa de nuevo el botón A** en el aire, Jack frena inmediatamente su caída y queda suspendido en el aire.
- El jugador no puede simplemente mantener presionado el botón A para flotar indefinidamente: debe pulsar repetidamente (*tap* / aleteo).
- Si el jugador mantiene pulsada la dirección **ARRIBA** mientras aletea, el avance por frame es de solo `$00A0` (160 subpíxeles/frame en lugar de `$00F0` = 240 subpíxeles/frame), permitiendo que cada aleteo lo mantenga flotando durante un 50% más de frames antes de ganar velocidad de caída.

---

## 6. Cinemática Horizontal, Parada Instantánea y Fricción

Ubicación: `asm/src/prg.asm:1533-1636`.

```asm
loc_89EF:
    LDA #0
    STA PlayerXVelLo
    STA PlayerXVelHi            ; POR DEFECTO VELOCIDAD X = 0 (PARADA INSTANTÁNEA)
    LDA PlayerStruct
    BMI loc_8A03                ; Si Jack está muriendo, no responde a D-Pad
    LDA Joypad1_Immediate
    AND #JP_Right|JP_Left       ; ¿Se pulsa Izquierda o Derecha?
    BNE loc_8A06

loc_8A03:
    JMP DrawPlayer              ; Si no se pulsa nada, Jack se detiene en seco

loc_8A06:
    LDX #1                      ; VelHi = +1 (Derecha)
    LDY #1
    LSR A                       ; Desplaza JP_Right al Carry
    BCS loc_8A10
    LDX #$FE                    ; VelHi = -2 (Izquierda, en comp. a 2 $FE.80 = -1.5)
    INY

loc_8A10:
    TYA
    BIT CollisionFlag           ; Chequea colisión con muros laterales
    BNE loc_8A66
    STX PlayerXVelHi
    LDA #$80
    STA PlayerXVelLo            ; VelLo = $80 (+0.5 px)
```

### 6.1. Valores Exactos de Velocidad Horizontal
- **Movimiento a la Derecha**:
  `PlayerXVelHi = 0x01`, `PlayerXVelLo = 0x80` $\rightarrow +0x0180 = +384$ subpíxeles/frame = **$+1.5$ píxeles/frame**.
- **Movimiento a la Izquierda**:
  `PlayerXVelHi = 0xFE`, `PlayerXVelLo = 0x80` $\rightarrow 0xFE80 = -0x0180 = -384$ subpíxeles/frame = **$-1.5$ píxeles/frame**.
- **Sin Botón Pulsado**:
  `PlayerXVelHi = 0x00`, `PlayerXVelLo = 0x00` $\rightarrow 0$ subpíxeles/frame = **$0.0$ píxeles/frame**.

### 6.2. Inercia, Aceleración y Fricción
- **Aceleración**: No existe curva de aceleración. La velocidad conmuta entre 0 y $\pm 1.5$ px/frame en exactamente 1 frame.
- **Fricción**: Es infinita (parada en seco instantánea).
- **Control en el Aire**: Es exactamente idéntico al control en el suelo. Al soltar el D-Pad en el aire, Jack deja de avanzar horizontalmente de inmediato y cae de forma estrictamente vertical. Si se pulsa el D-Pad en el aire, responde instantáneamente a $1.5$ px/frame.

---

## 7. Selección Canónica de Sprites y Animación

Ubicación: `asm/src/prg.asm:1476-1486` y `1605-1636`.

| Estado de Jack | Entrada D-Pad | Acumulador ($03E7) | Frame Counter ($03DF) | Sprite ID | Descripción Canónica |
| :--- | :--- | :--- | :--- | :---: | :--- |
| En Suelo | Quieto | N/A | N/A | `0x00` | De pie frontal (Idle) |
| En Suelo | Caminando | N/A | `(count & 4) == 0` | `0x04` | Caminar Frame 1 |
| En Suelo | Caminando | N/A | `(count & 4) != 0` | `0x05` | Caminar Frame 2 |
| En Aire | Sin D-Pad Horiz. | $< 0x40$ (Ascenso) | N/A | `0x01` | Salto vertical ascendente |
| En Aire | Sin D-Pad Horiz. | $\ge 0x40$ (Descenso/Flutter) | N/A | `0x02` | Planeo/aleteo vertical |
| En Aire | Con D-Pad Horiz. | $< 0x40$ (Ascenso) | N/A | `0x04` | Salto diagonal ascendente |
| En Aire | Con D-Pad Horiz. | $\ge 0x40$ (Descenso/Flutter) | N/A | `0x06` | Planeo/caída diagonal |

Orientación horizontal:
- Si se mueve a la derecha: `byte_3DD = 0x04` (sin flip horizontal).
- Si se mueve a la izquierda: `byte_3DD = 0x00` (con flip horizontal).

---

## 8. Discrepancias Identificadas en `src/core/mjk_player.c`

Al contrastar la implementación actual de C frente al código 6502 canónico se detectan tres divergencias críticas:

1. **Velocidad Horizontal Errónea (`MJK_WALK_SPEED_SUBPIXELS 352`)**:
   En `mjk_player.c:36`, se definió la velocidad de caminar como 352 subpíxeles ($1.375$ px/f) asumiendo que `word_C163[1] = 0x0160` era la velocidad de movimiento horizontal. En el código 6502 canónico, `word_C163` es la tasa de modulación vertical del acumulador de salto, mientras que la velocidad horizontal es de **384 subpíxeles** ($0x0180 = 1.5$ px/f) fija.
2. **Modelo de Caída Ad-Hoc no Canónico**:
   En `mjk_player.c:162-174`, la caída libre y el planeo se implementaron con constantes arbitrarias:
   `MJK_GRAVITY_FALL_ACCEL 32`, `MJK_MAX_FALL_SPEED 768` ($3.0$ px/f) y `MJK_FLUTTER_FALL_SPEED 128` ($0.5$ px/f).
   En 6502, no existe tal aceleración lineal ad-hoc: la fase de caída utiliza la **misma tabla de 64 pasos invertida** (`63 - (step - 64)`), con velocidad terminal máxima de **$3.5$ px/frame** ($0x0380$).
3. **Mecánica de Flutter Errónea (Mantenimiento vs Pulsación)**:
   En `mjk_player.c:165`, el aleteo se activa mientras el botón A está presionado (`inputs->held & MJK_BTN_A`), cayendo a velocidad constante.
   En 6502, el aleteo es **estrictamente de flanco de subida** (`just_pressed & MJK_BTN_A`), reseteando el acumulador a $0x40$ (ápice parabólico con velocidad de $1$ subpíxel/frame), exigiendo pulsar repetidamente el botón para mantenerse en el aire.

---

## 9. Especificación de Implementación para C (`c_implementer`)

### 9.1. Estructura y Variables Canónicas
```c
typedef struct {
    uint16_t jump_accumulator; // byte_3E7 (hi) y byte_3E6 (lo) en formato 8.8
    int16_t vel_x_subpixels;   // Q8.8 con signo (+0x0180, -0x0180, 0)
    int16_t vel_y_subpixels;   // Q8.8 con signo (-0x0380 .. +0x0380)
    bool in_air;               // Bit 1 de PlayerStruct
    bool flutter_active;       // Bit 3 de PlayerStruct
    uint8_t walk_anim_timer;   // byte_3DF
    uint8_t facing_attr;       // byte_3DD: 0x04 (derecha), 0x00 (izquierda)
} MjkJackKinematics;
```

### 9.2. Algoritmo Canónico de Avance Vertical en C
```c
void Jack_StepVerticalPhysics(JackPlayer* jack, const MjkInputState* inputs) {
    // 1. Manejo de pulsación de botón A (Salto o Aleteo)
    if (inputs->just_pressed & MJK_BTN_A) {
        if (!jack->in_air) {
            // Salto inicial desde suelo
            jack->in_air = true;
            jack->jump_accumulator = 0x0000; // Step 0: Impulso inicial
        } else {
            // Aleteo continuo en el aire (Flutter)
            jack->jump_accumulator = 0x4000; // Step 64: Ápice parabólico
        }
        jack->flutter_active = true;
    }

    if (!jack->in_air) {
        jack->vel_y_subpixels = 0;
        return;
    }

    // 2. Modulación de avance según Joypad vertical
    uint8_t joy_vert = 0;
    if (inputs->held & MJK_BTN_DOWN) joy_vert |= 1;
    if (inputs->held & MJK_BTN_UP)   joy_vert |= 2;
    uint16_t advance = g_mjk_jump_advance_table[joy_vert];

    uint32_t next_accum = (uint32_t)jack->jump_accumulator + advance;
    if ((next_accum >> 8) > 0x7F) {
        next_accum = 0x7F00; // Clamp a 127
    }
    jack->jump_accumulator = (uint16_t)next_accum;

    // 3. Consulta de velocidad en tabla simétrica (sub_8AE8)
    uint8_t step = (uint8_t)(jack->jump_accumulator >> 8);
    uint8_t table_idx;
    bool is_descent = (step >= 0x40);

    if (is_descent) {
        table_idx = (step ^ 0xFF) & 0x3F; // 63 - (step - 64)
    } else {
        table_idx = step;
    }

    uint16_t magnitude = g_mjk_canonical_jump_table[table_idx];
    if (is_descent) {
        jack->vel_y_subpixels = (int16_t)magnitude;         // Hacia abajo (+)
    } else {
        jack->vel_y_subpixels = (int16_t)(-(int32_t)magnitude); // Hacia arriba (-)
    }

    // 4. Integración Q8.8
    jack->pos_y += jack->vel_y_subpixels;
}
```

### 9.3. Algoritmo Canónico de Avance Horizontal en C
```c
void Jack_StepHorizontalPhysics(JackPlayer* jack, const MjkInputState* inputs) {
    if (inputs->held & MJK_BTN_RIGHT) {
        jack->vel_x_subpixels = 384; // +0x0180 (1.5 px/f)
        jack->facing_left = false;
        jack->facing_attr = 0x04;
    } else if (inputs->held & MJK_BTN_LEFT) {
        jack->vel_x_subpixels = -384; // -0x0180 (-1.5 px/f)
        jack->facing_left = true;
        jack->facing_attr = 0x00;
    } else {
        jack->vel_x_subpixels = 0; // Parada instantánea
    }

    // Integración Q8.8
    jack->pos_x += jack->vel_x_subpixels;
}
```
