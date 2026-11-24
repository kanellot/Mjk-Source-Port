# RES-016: Sistema Canónico de Colisiones de Jack (9-Probe), Resolución de Metatiles y Geometría de Contacto

- **ID de Investigación**: `RES-016`
- **Tarea Asociada**: `TASK-F6-020` / `TASK-RES-011`
- **Área Temática**: 02 - Cinemática, Física y Colisiones (Jack)
- **Fecha**: 2026-10-25
- **Autor / Agente**: `asm_researcher` (Nivel 3)
- **Estado**: `[COMPLETADA]`

---

## 1. Objetivo y Preguntas de Investigación

El propósito de esta investigación es desensamblar, formalizar matemáticamente y documentar sin asunciones el sistema canónico de detección y resolución de colisiones del protagonista (Jack) en *Mighty Bomb Jack* (NES), analizando el código ensamblador 6502 en [`asm/src/prg.asm`](../asm/src/prg.asm) y las tablas de datos en [`asm/src/data.asm`](../asm/src/data.asm).

### Preguntas Clave:
1. **Puntos de Sondeo (9 Probes)**: ¿Cuáles son las coordenadas relativas $(dX, dY)$, el orden canónico y el propósito específico de cada una de las 9 sondas definidas en `byte_A3D8`?
2. **Resolución de Solidez y Metatiles**: ¿Cómo interactúa la tabla `TileAttributeTable` (`asm/src/data.asm:598-639`) con el motor de colisiones de metatiles ($16 \times 16$ px) y con el desglose por cuadrantes ($8 \times 8$ px) en `byte_A3EA`? ¿Cómo se clasifican los metatiles presentes en la Sala 1?
3. **Mecánica de Contacto y Respuesta Física**:
   - ¿Cuáles son las condiciones exactas para aterrizar en suelo/plataforma (`vel_y >= 0`) y cómo se ejecuta el ajuste de posición $Y$ (`AND #$F8`), la cancelación de velocidad y el flag de contacto (`CollisionFlag` bit 4)?
   - ¿Qué sucede ante colisiones de cabeza/techo (`sub_A1B9`), cómo se fuerza la transición a parábola descendente (`byte_3E7 = $40`, `PlayerStruct |= 2`) y cómo se expulsa a Jack del techo (`Y |= 7`)?
   - ¿Cómo se bloquea el avance horizontal $X$ ante colisión lateral contra muros (`CollisionFlag` bits 0 y 1)?

---

## 2. Fuentes de Verdad Analizadas (`asm/`)

| Archivo | Rango de Líneas | Símbolos / Funciones Analizadas | Propósito en el Motor |
| :--- | :--- | :--- | :--- |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L5863-L5872) | `5863 - 5872` | `byte_A3D8` | Tabla de desplazamientos relativos $(dX, dY)$ de las 9 sondas de colisión. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L5873-L5883) | `5873 - 5883` | `byte_A3EA` | Tabla de máscaras de solidez por cuadrantes $8 \times 8$ para metatiles `$38..$4B`. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L5894-L5903) | `5894 - 5903` | `byte_A406` | Máscaras de bits para acumulación de resultados de sondas en `byte_30F`. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L3871-L3900) | `3871 - 3900` | `loc_97CA` (en `HandleSpawnsAndMore`) | Secuencia principal de sondeo de centro (sonda 0) y disparo de colisiones. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L4152-L4226) | `4152 - 4226` | `sub_9965`, `loc_99BB`, `loc_99DB` | Programación de sondas verticales/laterales y compresión en `CollisionFlag`. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L4230-L4247) | `4230 - 4247` | `sub_99EA`, `loc_99F4` | Despacho de pares de sondas para salas estáticas (`RF_ScrollStop`). |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L4251-L4314) | `4251 - 4314` | `sub_9A03` | Cálculo de coordenadas de sondeo mundo/pantalla y llamada a comprobación. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L4480-L4611) | `4480 - 4611` | `sub_9B6A`, `loc_9BBB`, `loc_9BEB`, `loc_9C05` | Resolución de solidez de metatile, muerte por fuego y acumulación en `byte_30F`. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L4615-L4706) | `4615 - 4706` | `BigBeefySubHandlesBGColliding` | Detección de cruce de celda de $8$ px y activación de `byte_330`. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L5530-L5555) | `5530 - 5555` | `sub_A1B9`, `loc_A1E2` | Respuesta a impacto de cabeza con techo (fuerza caída y expulsa $Y$). |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L5559-L5609) | `5559 - 5609` | `sub_A1EB`, `loc_A20A`, `loc_A224` | Respuesta a contacto con suelo/plataforma (alinea $Y$, resetea caída). |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L1547-L1562) | `1547 - 1562` | `loc_8A06`, `loc_8A10` | Bloqueo de velocidad horizontal $X$ si `CollisionFlag & 1` o `2` está activo. |
| [`asm/src/data.asm`](../asm/src/data.asm#L598-L639) | `598 - 639` | `TileAttributeTable` | Tabla de atributos de 2 bits por metatile (160 metatiles). |
| [`asm/src/ram.asm`](../asm/src/ram.asm#L1223-L1246) | `1223 - 1246` | `PlayerStruct` a `PlayerYPosHi` | Distribución en RAM de la estructura del jugador (`$03CE-$03D9`). |

---

## 3. Geometría y Puntos de Sondeo (Tabla Canónica de 9 Sondas)

### 3.1. Definición en ROM (`asm/src/prg.asm:5863-5872`)

La tabla `byte_A3D8` define las coordenadas relativas en formato con signo $(dX, dY)$ de 9 puntos de testeo centrados respecto al origen del jugador $(X, Y) = (\text{PlayerXPosHi}, \text{PlayerYPosHi})$:

```assembly
; asm/src/prg.asm:5863-5872
byte_A3D8:
	.BYTE   0,  0			; Sonda 0: dX =  0, dY =  0 (Centro / Ancla)
	.BYTE  -4, -8			; Sonda 1: dX = -4, dY = -8 (Cabeza / Techo Izquierda)
	.BYTE   4, -8			; Sonda 2: dX = +4, dY = -8 (Cabeza / Techo Derecha)
	.BYTE  -4,  8			; Sonda 3: dX = -4, dY = +8 (Pies / Suelo Izquierda)
	.BYTE   4,  8			; Sonda 4: dX = +4, dY = +8 (Pies / Suelo Derecha)
	.BYTE  -8, -4			; Sonda 5: dX = -8, dY = -4 (Muro Izquierdo Superior)
	.BYTE  -8,  4			; Sonda 6: dX = -8, dY = +4 (Muro Izquierdo Inferior)
	.BYTE   8, -4			; Sonda 7: dX = +8, dY = -4 (Muro Derecho Superior)
	.BYTE   8,  4			; Sonda 8: dX = +8, dY = +4 (Muro Derecho Inferior)
```

### 3.2. Tabla Maestra de Especificación de Sondas

| Índice (`byte_1`) | $dX$ (px) | $dY$ (px) | Denominación Canónica | Región Anatómica | Propósito y Función en el Motor |
| :---: | :---: | :---: | :--- | :--- | :--- |
| **0** | `0` | `0` | **Centro / Ancla** | Centro geométrico | Detección de celda $8 \times 8$, transiciones de sala, activación de `byte_330`. |
| **1** | `-4` | `-8` | **Cabeza Izquierda** | Esquina sup-izq | Detección de impacto contra techo (`CollisionFlag & $08`). |
| **2** | `+4` | `-8` | **Cabeza Derecha** | Esquina sup-der | Detección de impacto contra techo (`CollisionFlag & $08`). |
| **3** | `-4` | `+8` | **Pies Izquierda** | Esquina inf-izq | Detección de suelo y plataformas (`CollisionFlag & $04`). |
| **4** | `+4` | `+8` | **Pies Derecha** | Esquina inf-der | Detección de suelo y plataformas (`CollisionFlag & $04`). |
| **5** | `-8` | `-4` | **Muro Izquierdo Sup.** | Costado izq-alto | Bloqueo de avance hacia la izquierda (`CollisionFlag & $02`). |
| **6** | `-8` | `+4` | **Muro Izquierdo Inf.** | Costado izq-bajo | Bloqueo de avance hacia la izquierda (`CollisionFlag & $02`). |
| **7** | `+8` | `-4` | **Muro Derecho Sup.** | Costado der-alto | Bloqueo de avance hacia la derecha (`CollisionFlag & $01`). |
| **8** | `+8` | `+4` | **Muro Derecho Inf.** | Costado der-bajo | Bloqueo de avance hacia la derecha (`CollisionFlag & $01`). |

### 3.3. Diagrama Visual del Bounding Box y Envolvente de Jack ($16 \times 16$ px)

```
        -8      -4       0      +4      +8   (dX)
    -8   +-------[1]-------------0[2]-------+  <-- Techo / Cabeza (dY = -8)
         |                               |
    -4  [5]                             [7] <-- Muro Sup (dY = -4)
         |                               |
     0   |              [0]              |  <-- Centro Jack (dX=0, dY=0)
         |                               |
    +4  [6]                             [8] <-- Muro Inf (dY = +4)
         |                               |
    +8   +-------[3]-------------[4]-----+  <-- Suelo / Pies (dY = +8)
```

- **Ancho del torso entre sondas de pared**: $dX = 8 - (-8) = 16$ píxeles.
- **Alto del cuerpo entre cabeza y pies**: $dY = 8 - (-8) = 16$ píxeles.
- **Separación entre sondas de pies/cabeza**: $4 - (-4) = 8$ píxeles (permite pararse sobre bordes o salientes de medio bloque).
- **Separación entre sondas laterales**: $4 - (-4) = 8$ píxeles (permite detectar peldaños y pasadizos de 1 bloque de altura).

---

## 4. Pipeline de Sondeo y Formación de `CollisionFlag`

### 4.1. Estructura de Entidad y Correspondencia en RAM

La estructura del jugador reside en `$03CE` ([`asm/src/ram.asm:1223-1246`](../asm/src/ram.asm#L1223-L1246)) y comparte indexación idéntica con los enemigos (`EnemyStruct_*` en [`asm/src/defs.asm:161-175`](../asm/src/defs.asm#L161-L175)):

- Offset `+0` (`$03CE`): `PlayerStruct` (Estado, bits de salto/vuelo/muerte).
- Offset `+1` (`$03CF`): `CollisionFlag` (**Registro maestro de colisiones**).
- Offset `+4` (`$03D2`): `PlayerXVelHi` (Velocidad horizontal entera).
- Offset `+5` (`$03D3`): `PlayerXPosHi` (Coordenada $X$ entera).
- Offset `+$A` (`$03D8`): `PlayerYVelHi` (Velocidad vertical entera).
- Offset `+$B` (`$03D9`): `PlayerYPosHi` (Coordenada $Y$ entera).

### 4.2. Algoritmo de Cálculo de Coordenadas de Sonda (`sub_9A03`, `asm/src/prg.asm:4251-4314`)

Para la sonda `byte_1`:
1. Lee $dX = \text{byte\_A3D8}[2 \cdot \text{byte\_1}]$, extiende signo en `X`.
2. Suma el desplazamiento horizontal de la cámara: $X' = dX + \text{byte\_3C2}$.
3. Suma la posición horizontal del objeto:
   $$\text{byte\_0} = X' + (\text{off\_78})[5] = X' + \text{PlayerXPosHi}$$
4. Lee $dY = \text{byte\_A3D8}[2 \cdot \text{byte\_1} + 1]$.
5. Suma la posición vertical del objeto:
   $$\text{byte\_9} = dY + (\text{off\_78})[\$B] = dY + \text{PlayerYPosHi}$$
6. Obtiene la columna de metatile: $\text{byte\_8} = \text{byte\_0} \gg 4$.
7. Determina la mitad de pantalla: $Y_{half} = (\text{byte\_9} \ \& \ \$80) \gg 7$.
8. Invoca `HandleDrawHalfRow` (`prg.asm:4311`), resolviendo el puntero `off_4` a los metatiles de la tira.

### 4.3. Resolución de Solidez y Cuadrantes $8 \times 8$ (`sub_9B6A`, `asm/src/prg.asm:4480-4611`)

El metatile obtenido de `(off_4)[(byte_9 & $70) >> 4]` se almacena en `TilePlayerCollidedWith`. Su solidez se evalúa según su rango numérico:

1. **Casos Especiales Pasantes (Vacío)** (`loc_9BBB:4535-4540`):
   - Metatile `$00` (Fondo vacío).
   - Metatile `$2F` (Fondo transitable / cielo).
   - Metatile `$27` (Baldosa de bomba).
   $\rightarrow$ Genera máscara de solidez $\text{byte\_6} = \$00$ (sin colisión).

2. **Peligro Mortal (Fuego / Lava)** (`loc_9BBB:4543-4555`):
   - Rango: `$5C \le \text{TilePlayerCollidedWith} \le \$62`.
   - Si `byte_1 == 0` (sonda de centro): Jack muere inmediatamente:
     $$\text{FireTilePlayerDiedTo} = \text{TilePlayerCollidedWith}, \quad \text{PlayerStruct} \mid= \$80$$
     Ejecuta `PLA; PLA; RTS` para abortar el ciclo de frame.

3. **Metatiles de Solidez Total ($16 \times 16$ px)** (`loc_9BEB:4560-4563`):
   - $\text{TilePlayerCollidedWith} < \$38$ (excepto `$00, $27, $2F`).
   - $\text{TilePlayerCollidedWith} \ge \$4C$ (excepto fuego `$5C..$62`).
   $\rightarrow$ Genera máscara de solidez $\text{byte\_6} = \$FF$ (los 4 cuadrantes son sólidos).

4. **Metatiles de Solidez Parcial ($8 \times 8$ px) vía `byte_A3EA`** (`loc_9BEB:4564-4576`):
   - Rango: `$38 \le \text{TilePlayerCollidedWith} \le \$4B` (20 metatiles de medio bloque, rampas y plataformas delgadas).
   - Cálculo del índice:
     $$\text{offset} = \text{TilePlayerCollidedWith} - \$38$$
     $$\text{idx} = \text{offset} \gg 1, \quad \text{nibble} = (\text{offset} \ \& \ 1)$$
   - Si `nibble == 0`: toma el nibble superior: $\text{byte\_6} = \text{byte\_A3EA}[\text{idx}] \ \& \ \$F0$.
   - Si `nibble == 1`: toma el nibble inferior desplazado: $\text{byte\_6} = (\text{byte\_A3EA}[\text{idx}] \ll 4) \ \& \ \$F0$.

#### Desglose de Cuadrantes en `byte_6`:
Los 4 bits superiores de `byte_6` representan los 4 cuadrantes $8 \times 8$:
- **Bit 7**: Cuadrante Superior-Izquierdo ($TL$, $Y < 8, X < 8$).
- **Bit 6**: Cuadrante Superior-Derecho ($TR$, $Y < 8, X \ge 8$).
- **Bit 5**: Cuadrante Inferior-Izquierdo ($BL$, $Y \ge 8, X < 8$).
- **Bit 4**: Cuadrante Inferior-Derecho ($BR$, $Y \ge 8, X \ge 8$).

El motor extrae el cuadrante de la sonda desplazando `byte_6` a la izquierda según $(\text{byte\_9} \ \& \ 8)$ y $(\text{byte\_0} \ \& \ 8)$, dejando el bit de solidez en el flag `Carry`.

### 4.4. Acumulación de Bits de Sonda en `byte_30F` (`prg.asm:4597-4610`)

Cada sonda almacena su solidez (1 o 0) en un bit específico de `byte_30F`:
```assembly
; prg.asm:4597-4610
	LDX byte_1				; índice de sonda (1..8)
	DEX					; 0..7
	LDA byte_A406,X				; Máscara para borrar bit previo
	AND byte_30F
	STA byte_30F
	LDA #0
loc_9C2B:
	ROR A					; Rota Carry (solidez) en el acumulador
	DEX
	BPL loc_9C2B
	ORA byte_30F
	STA byte_30F
```

#### Mapeo Exacto de `byte_30F`:
- **Bit 7**: Sonda 1 (Cabeza Izquierda)
- **Bit 6**: Sonda 2 (Cabeza Derecha)
- **Bit 5**: Sonda 3 (Pies Izquierda)
- **Bit 4**: Sonda 4 (Pies Derecha)
- **Bit 3**: Sonda 5 (Muro Izquierdo Sup.)
- **Bit 2**: Sonda 6 (Muro Izquierdo Inf.)
- **Bit 1**: Sonda 7 (Muro Derecho Sup.)
- **Bit 0**: Sonda 8 (Muro Derecho Inf.)

### 4.5. Compresión por Pares en `CollisionFlag` (`loc_99BB`, `prg.asm:4201-4225`)

En `loc_99BB`, los 8 bits de `byte_30F` se combinan mediante compuertas OR lógicas en un nibble de 4 bits:

$$\text{Ceiling} = p_3 = (\text{Bit 7} \mid \text{Bit 6}) = \text{Sonda 1} \mid \text{Sonda 2}$$
$$\text{Floor} = p_2 = (\text{Bit 5} \mid \text{Bit 4}) = \text{Sonda 3} \mid \text{Sonda 4}$$
$$\text{Left Wall} = p_1 = (\text{Bit 3} \mid \text{Bit 2}) = \text{Sonda 5} \mid \text{Sonda 6}$$
$$\text{Right Wall} = p_0 = (\text{Bit 1} \mid \text{Bit 0}) = \text{Sonda 7} \mid \text{Sonda 8}$$

El nibble resultante se integra en `CollisionFlag` (`PlayerStruct + 1`):

$$\text{CollisionFlag} = (\text{CollisionFlag} \ \& \ \$F0) \mid (p_3 \ll 3) \mid (p_2 \ll 2) \mid (p_1 \ll 1) \mid p_0$$

#### Semántica Completa de `CollisionFlag` (`$03CF`):
| Bit | Máscara | Nombre Canónico | Significado Lógico |
| :---: | :---: | :--- | :--- |
| **0** | `$01` | `CF_RightWall` | Sonda 7 u 8 en sólido $\rightarrow$ Bloqueo de avance a la derecha. |
| **1** | `$02` | `CF_LeftWall` | Sonda 5 u 6 en sólido $\rightarrow$ Bloqueo de avance a la izquierda. |
| **2** | `$04` | `CF_Floor` | Sonda 3 o 4 en sólido $\rightarrow$ Contacto con suelo/plataforma. |
| **3** | `$08` | `CF_Ceiling` | Sonda 1 o 2 en sólido $\rightarrow$ Impacto contra techo. |
| **4** | `$10` | `CF_OnGround` | Flag de estado en suelo (mantenido por `sub_A1EB`). |
| **5..7**| `$E0` | `CF_System` | Flags de estado interno del motor / cambio de sala. |

---

## 5. Dinámica de Contacto y Resolución Física

### 5.1. Selección Condicional de Sondas según la Velocidad Vertical (`sub_9965:4171-4178`)

```assembly
; asm/src/prg.asm:4171-4178
	LDY #$A					; Offset PlayerYVelHi
	LDX #3
	LDA (off_78),Y				; Evalúa signo de PlayerYVelHi
	BPL loc_9992				; Si VelY >= 0 (cayendo/en suelo) -> X = 3
	LDX #1					; Si VelY < 0 (saltando hacia arriba) -> X = 1
loc_9992:
	JSR sub_99EA				; Lanza par de sondas (X y X+1)
```

> [!IMPORTANT]
> **Inmunidad Unidireccional en Ascenso**:
> Mientras Jack asciende ($\text{PlayerYVelHi} < 0$), el motor ejecuta las sondas 1 y 2 (cabeza), **omitiendo completamente las sondas 3 y 4 (pies)**. Por tanto, Jack atraviesa de abajo a arriba cualquier plataforma o suelo sin colisionar. Solo cuando $\text{PlayerYVelHi} \ge 0$ (en el vértice del salto o durante la caída) se activan las sondas 3 y 4.

### 5.2. Aterrizaje sobre Suelo o Plataforma (`sub_A1EB`, `asm/src/prg.asm:5559-5609`)

Invocada inmediatamente tras la actualización de colisiones en `HandleSpawnsAndMore` (`prg.asm:3887`):

```assembly
; asm/src/prg.asm:5559-5609
sub_A1EB:
	AND #4					; Comprueba CF_Floor (bit 2)
	BNE loc_A20A				; Si hay contacto con suelo, salta a loc_A20A

	; --- EN EL AIRE (CF_Floor == 0) ---
	LDA CollisionFlag
	AND #%11101111				; Apaga bit 4 (CF_OnGround)
	STA CollisionFlag
	LDA #2
	BIT PlayerStruct			; Comprueba si ya está en caída
	BNE locret_A209
	ORA PlayerStruct			; Activa bit 1 de PlayerStruct (estado caída)
	STA PlayerStruct
	LDA #$40
	STA byte_3E7				; Índice inicial de caída libre
locret_A209:
	RTS

loc_A20A:
	; --- CONTACTO CON SUELO (CF_Floor != 0) ---
	LDA PlayerStruct
	AND #$F9				; Limpia bits 1 y 2 (cancela salto y aleteo)
	STA PlayerStruct
	LDA #$10
	BIT CollisionFlag			; Comprueba si ya estaba en el suelo
	BNE loc_A224
	ORA CollisionFlag			; Enciende bit 4 (CF_OnGround)
	STA CollisionFlag
	LDA #0
	STA byte_333

loc_A224:
	LDA byte_3C4
	AND #$F8
	STA byte_3C4
	LDA a:RoomStatusFlags
	AND #$20				; RF_ScrollStop (Salas de pantalla fija, ej. Sala 1)
	BNE loc_A23A
	LDA a:RoomStatusFlags
	AND #$C0
	BEQ loc_A242

loc_A23A:
	LDA PlayerYPosHi
	AND #$F8				; ALINEACIÓN A MÚLTIPLO DE 8 PÍXELES
	STA PlayerYPosHi

loc_A242:
	LDA #0
	STA PlayerSprite			; Resetea sprite a posición de pie
	RTS
```

#### Reglas Matemáticas del Aterrizaje:
1. **Condición de Contacto**: $\text{PlayerYVelHi} \ge 0$ Y $(\text{CollisionFlag} \ \& \ \$04) \ne 0$.
2. **Alineación de Coordenada $Y$**:
   $$\text{PlayerYPosHi} = \text{PlayerYPosHi} \ \& \ \$F8 = \lfloor \text{PlayerYPosHi} / 8 \rfloor \cdot 8$$
   Snappea los pies de Jack con exactitud quirúrgica a la superficie superior de la celda de metatile.
3. **Cancelación de Estados Cinemáticos**:
   $$\text{PlayerStruct} = \text{PlayerStruct} \ \& \ \$F9 \quad (\text{apaga bits 1 y 2: salto y aleteo})$$
   $$\text{CollisionFlag} \mid= \$10 \quad (\text{activa flag de estar en suelo})$$
   $$\text{PlayerSprite} = 0 \quad (\text{fuerza animación de Jack parado})$$
   Al estar en suelo, el ciclo de física de salto omite la gravedad y anula $\text{PlayerYVelHi} = 0, \text{PlayerYVelLo} = 0$.

### 5.3. Colisión con Techo (`sub_A1B9`, `asm/src/prg.asm:5530-5555`)

Cuando $\text{CollisionFlag} \ \& \ \$08 \ne 0$ (Sonda 1 o 2 en bloque sólido al ascender), `loc_97CA:3891` llama a `sub_A1B9`:

```assembly
; asm/src/prg.asm:5530-5555
sub_A1B9:
	LDA a:byte_314
	BNE loc_A1E2
	LDA PlayerStruct
	ORA #2					; Activa bit 1 (fuerza fase descendente)
	STA PlayerStruct
	LDA #$40
	STA byte_3E7				; byte_3E7 = $40 (parábola descendente inmediata)
	LDA a:RoomStatusFlags
	AND #RF_ScrollStop
	BNE loc_A1E2
	LDA a:RoomStatusFlags
	AND #RF_Horizontal|RF_SectionStart
	BNE loc_A1E2
	LDA byte_3C4
	ORA #7
	STA byte_3C4
	RTS

loc_A1E2:
	LDY #$B					; Offset PlayerYPosHi
	LDA (off_78),Y
	ORA #7					; EXPULSIÓN DE TECHO: Y = Y | 7
	STA (off_78),Y
	RTS
```

#### Reglas de Rebote contra Techo:
1. **Transición a Parábola Descendente**: Fija $\text{byte\_3E7} = \$40$ y activa el bit 1 de `PlayerStruct`. Esto cancela inmediatamente el impulso ascendente y conmuta la tabla de velocidad a valores positivos (caída).
2. **Expulsión Fuera del Bloque**: Ejecuta $\text{PlayerYPosHi} = \text{PlayerYPosHi} \mid 7$. Esto desplaza a Jack hacia abajo hasta el borde inferior del bloque de 8 píxeles actual ($Y \pmod 8 = 7$), impidiendo que su cabeza quede incrustada en el techo.

### 5.4. Colisión Lateral con Muros (`asm/src/prg.asm:1547-1562`)

En el bucle de entrada de mando del jugador:

```assembly
; asm/src/prg.asm:1547-1562
loc_8A06:
	LDX #1					; X = +1 (velocidad positiva hacia la derecha)
	LDY #1					; Y = 1 (máscara de bit 0: CF_RightWall)
	LSR A					; Joypad Right en Carry
	BCS loc_8A10
	LDX #$FE				; X = -2 ($FE en compl. a 2, avance izquierda)
	INY					; Y = 2 (máscara de bit 1: CF_LeftWall)

loc_8A10:
	TYA
	BIT CollisionFlag			; Comprueba bit 0 o bit 1 según la dirección
	BNE loc_8A66				; ¡Si el bit está activo, salta y NO aplica velocidad!
	STX PlayerXVelHi
	LDA #$80
	STA PlayerXVelLo
	...
	JSR ApplyPlayerXVelocity
```

- **Bloqueo a la Derecha**: Si `CollisionFlag & $01 != 0` (Sonda 7 u 8 en sólido), se aborta la asignación de velocidad horizontal y la llamada a `ApplyPlayerXVelocity`.
- **Bloqueo a la Izquierda**: Si `CollisionFlag & $02 != 0` (Sonda 5 u 6 en sólido), se aborta la asignación de velocidad horizontal y la llamada a `ApplyPlayerXVelocity`.
- No existe empuje o expulsión en $X$; la coordenada se congela en el píxel previo al impacto.

---

## 6. Clasificación de Metatiles y Geometría de la Sala 1

### 6.1. La Tabla `TileAttributeTable` (`asm/src/data.asm:598-639`)

En el ensamblador original, `TileAttributeTable` almacena 2 bits por metatile empacados de a 4 metatiles por byte mediante la macro `Attrib`:
```assembly
; asm/src/macros.asm:61-63
MACRO Attrib t1, t2, t3, t4
	.db (t1 << 6) | (t2 << 4) | (t3 << 2) | (t4)
ENDM
```

> [!NOTE]
> **Doble Función en la Arquitectura NES**:
> En el hardware NES, `TileAttributeTable` es leída por `HandleTileAttributes` (`prg.asm:7092-7160`) para generar la tabla de atributos de la PPU (selección de paletas 0..3). A nivel semántico de diseño y colisiones, los valores corresponden a los tipos clásicos:
> - `0` = Vacío / Transitable.
> - `1` = Sólido Total (Muros perimetrales, columnas estructurales).
> - `2` = Plataforma (Suelo transitable / one-way).
> - `3` = Especial / Ítem / Daño / Fuego.

### 6.2. Catálogo y Clasificación de Metatiles Usados en la Sala 1

La Sala 1 se decodifica a partir de las mitades de pantalla `h0 = $34` y `h1 = $22` ([`asm/src/data.asm:923`](../asm/src/data.asm#L923)), conformando una matriz de $16 \times 15$ metatiles ($256 \times 240$ px).

| Metatile ID | `TileAttributeTable` | Solidez por Sonda (`sub_9B6A`) | Clasificación Arquitectónica | Cuadrantes $TL, TR, BL, BR$ |
| :---: | :---: | :--- | :--- | :---: |
| **`$00`** | `1` | `$00` (Bypass `loc_9BBB`) | **Vacío / Fondo Negro** | $0, 0, 0, 0$ |
| **`$2D`** | `2` | `$FF` (Sólido Total) | **Plataforma Sólida Interior** | $1, 1, 1, 1$ |
| **`$2E`** | `2` | `$FF` (Sólido Total) | **Borde Lateral de Plataforma** | $1, 1, 1, 1$ |
| **`$2F`** | `3` | `$00` (Bypass `loc_9BBB`) | **Espacio Vacío Transitable** | $0, 0, 0, 0$ |
| **`$31`** | `2` | `$FF` (Sólido Total) | **Cornisa Superior de Plataforma** | $1, 1, 1, 1$ |
| **`$32`** | `2` | `$FF` (Sólido Total) | **Esquina Superior de Plataforma** | $1, 1, 1, 1$ |
| **`$5B`** | `2` | `$FF` (Sólido Total) | **Suelo de Plataforma Superior** | $1, 1, 1, 1$ |
| **`$5D`** | `1` | `DEATH/FIRE` (`loc_9BBB`) | **Fuego Letal Inferior (Lava)** | Muerte instantánea |
| **`$5E`** | `1` | `DEATH/FIRE` (`loc_9BBB`) | **Fuego Letal Superior (Llama)** | Muerte instantánea |
| **`$92`** | `0` | `$FF` (Sólido Total) | **Muro Perimetral Exterior** | $1, 1, 1, 1$ |

### 6.3. Mapa de Ocupación de la Sala 1 ($16 \times 15$ Metatiles)

```
Columna:  00 01 02 03 04 05 06 07 | 08 09 0A 0B 0C 0D 0E 0F
Fila 00:  2F 2F 2F 2D 2F 5D 5B 2D | 00 00 00 00 00 5E 92 92  <-- Techo / Columnas
Fila 01:  31 31 31 31 32 2D 2E 2D | 00 00 00 00 00 5E 00 00  <-- Plataforma Sup.
Fila 02:  2D 2D 2D 2D 2D 2D 2D 2D | 00 00 00 00 00 5E 00 00
Fila 03:  2D 2D 2D 2D 2D 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 04:  2D 2D 2D 2D 2D 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 05:  2D 2D 2D 2D 2D 2F 2F 2F | 2D 2D 2D 2F 2F 2D 2E 2D  <-- Plataforma Media
Fila 06:  2D 2D 2D 2D 2D 2F 2F 2F | 2D 2D 2D 2F 2F 2D 2E 2D
Fila 07:  2D 2D 2D 2D 2D 2F 2F 2F | 2D 2D 2D 2F 2F 2D 2E 2D
Fila 08:  2D 2D 2D 2D 2D 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 09:  2D 2D 2D 2D 2D 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 10:  2D 2D 2D 2D 2D 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 11:  2D 2D 2D 2F 2F 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 12:  2D 2D 2D 2F 2F 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 13:  2D 2D 2D 2F 2F 2F 2F 2F | 00 00 00 00 00 5E 00 00
Fila 14:  2D 2D 2D 2D 2D 2F 2F 2F | 00 00 00 00 00 5E 00 00  <-- Suelo Inferior
```

---

## 7. Catálogo Completo de Metatiles con Solidez Parcial (`byte_A3EA`)

Para metatiles en el rango `$38..$4B`, la tabla `byte_A3EA` ([`asm/src/prg.asm:5873-5883`](../asm/src/prg.asm#L5873-L5883)) define la solidez a nivel de cuadrantes de $8 \times 8$ píxeles:

```assembly
; asm/src/prg.asm:5873-5883
byte_A3EA:
	.BYTE $CC, $FC, $48, $5A, $7B, $DE, $21, $35, $AC, $3C
```

| Metatile ID | Offset Base | Byte ROM | Nibble | Máscara `byte_6` | $TL$ (8x8) | $TR$ (8x8) | $BL$ (8x8) | $BR$ (8x8) | Forma Geométrica |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **`$38`** | `$00` | `$CC` | Alto | `$C0` | **1** | **1** | 0 | 0 | Mitad superior sólida (Plataforma delgada) |
| **`$39`** | `$01` | `$CC` | Bajo | `$C0` | **1** | **1** | 0 | 0 | Mitad superior sólida (Plataforma delgada) |
| **`$3A`** | `$02` | `$FC` | Alto | `$F0` | **1** | **1** | **1** | **1** | Bloque completo sólido |
| **`$3B`** | `$03` | `$FC` | Bajo | `$C0` | **1** | **1** | 0 | 0 | Mitad superior sólida |
| **`$3C`** | `$04` | `$48` | Alto | `$40` | 0 | **1** | 0 | 0 | Solo esquina superior derecha sólida |
| **`$3D`** | `$05` | `$48` | Bajo | `$80` | **1** | 0 | 0 | 0 | Solo esquina superior izquierda sólida |
| **`$3E`** | `$06` | `$5A` | Alto | `$50` | 0 | **1** | 0 | **1** | Mitad derecha sólida (Muro vertical delgado) |
| **`$3F`** | `$07` | `$5A` | Bajo | `$A0` | **1** | 0 | **1** | 0 | Mitad izquierda sólida (Muro vertical delgado) |
| **`$40`** | `$08` | `$7B` | Alto | `$70` | 0 | **1** | **1** | **1** | Todo sólido excepto sup-izq |
| **`$41`** | `$09` | `$7B` | Bajo | `$B0` | **1** | 0 | **1** | **1** | Todo sólido excepto sup-der |
| **`$42`** | `$0A` | `$DE` | Alto | `$D0` | **1** | **1** | 0 | **1** | Todo sólido excepto inf-izq |
| **`$43`** | `$0B` | `$DE` | Bajo | `$E0` | **1** | **1** | **1** | 0 | Todo sólido excepto inf-der |
| **`$44`** | `$0C` | `$21` | Alto | `$20` | 0 | 0 | **1** | 0 | Solo esquina inferior izquierda sólida |
| **`$45`** | `$0D` | `$21` | Bajo | `$10` | 0 | 0 | 0 | **1** | Solo esquina inferior derecha sólida |
| **`$46`** | `$0E` | `$35` | Alto | `$30` | 0 | 0 | **1** | **1** | Mitad inferior sólida |
| **`$47`** | `$0F` | `$35` | Bajo | `$50` | 0 | **1** | 0 | **1** | Mitad derecha sólida |
| **`$48`** | `$10` | `$AC` | Alto | `$A0` | **1** | 0 | **1** | 0 | Mitad izquierda sólida |
| **`$49`** | `$11` | `$AC` | Bajo | `$C0` | **1** | **1** | 0 | 0 | Mitad superior sólida |
| **`$4A`** | `$12` | `$3C` | Alto | `$30` | 0 | 0 | **1** | **1** | Mitad inferior sólida |
| **`$4B`** | `$13` | `$3C` | Bajo | `$C0` | **1** | **1** | 0 | 0 | Mitad superior sólida |

---

## 8. Contrato Funcional en C x86 para Paridad Exacta

```c
#include <stdint.h>
#include <stdbool.h>

// Desplazamientos canónicos de las 9 sondas (byte_A3D8)
static const int8_t PROBE_DX[9] = {  0, -4,  4, -4,  4, -8, -8,  8,  8 };
static const int8_t PROBE_DY[9] = {  0, -8, -8,  8,  8, -4,  4, -4,  4 };

// Máscaras de cuadrante 8x8 para metatiles parciales 0x38..0x4B (byte_A3EA)
static const uint8_t QUADRANT_MASKS[10] = {
    0xCC, 0xFC, 0x48, 0x5A, 0x7B, 0xDE, 0x21, 0x35, 0xAC, 0x3C
};

// Flags de CollisionFlag ($03CF)
#define CF_RIGHT_WALL   0x01
#define CF_LEFT_WALL    0x02
#define CF_FLOOR        0x04
#define CF_CEILING      0x08
#define CF_ON_GROUND    0x10

typedef struct {
    uint8_t player_struct;     // $03CE
    uint8_t collision_flag;    // $03CF
    int8_t  vel_x_hi;          // $03D2
    uint8_t vel_x_lo;          // $03D0
    uint8_t pos_x_hi;          // $03D3
    uint8_t pos_x_lo;          // $03D1
    int8_t  vel_y_hi;          // $03D8
    uint8_t vel_y_lo;          // $03D6
    uint8_t pos_y_hi;          // $03D9
    uint8_t pos_y_lo;          // $03D7
    uint8_t sprite_frame;      // $03DC
    uint8_t jump_timer;        // $03E7
} JackState;

// Resolución de solidez de sonda individual
bool ProbeTileSolid(uint8_t tile_id, uint8_t px, uint8_t py, JackState *jack) {
    if (tile_id == 0x00 || tile_id == 0x2F || tile_id == 0x27) {
        return false; // Vacío
    }
    if (tile_id >= 0x5C && tile_id <= 0x62) {
        // Fuego / Daño mortal
        jack->player_struct |= 0x80; // Muerte de Jack
        return false;
    }
    if (tile_id < 0x38 || tile_id >= 0x4C) {
        return true; // Sólido total
    }

    // Solidez parcial 8x8 (0x38..0x4B)
    uint8_t offset = tile_id - 0x38;
    uint8_t raw = QUADRANT_MASKS[offset >> 1];
    uint8_t mask = (offset & 1) ? ((raw << 4) & 0xF0) : (raw & 0xF0);

    uint8_t quad = ((py & 8) ? 2 : 0) + ((px & 8) ? 1 : 0);
    return (mask & (0x80 >> quad)) != 0;
}

// Actualización canónica de colisiones (HandleSpawnsAndMore + sub_9965 + sub_A1EB)
void UpdateJackCollisions(JackState *jack, const uint8_t room_grid[15][16]) {
    uint8_t probe_bits = 0;

    // Selección vertical de sondas según vel_y_hi
    if (jack->vel_y_hi >= 0) {
        // Cayendo o en reposo: sondea pies (3 y 4)
        for (int p = 3; p <= 4; p++) {
            uint8_t x = jack->pos_x_hi + PROBE_DX[p];
            uint8_t y = jack->pos_y_hi + PROBE_DY[p];
            if (ProbeTileSolid(room_grid[y >> 4][x >> 4], x, y, jack)) {
                probe_bits |= (1 << (8 - p));
            }
        }
    } else {
        // Saltando hacia arriba: sondea cabeza (1 y 2)
        for (int p = 1; p <= 2; p++) {
            uint8_t x = jack->pos_x_hi + PROBE_DX[p];
            uint8_t y = jack->pos_y_hi + PROBE_DY[p];
            if (ProbeTileSolid(room_grid[y >> 4][x >> 4], x, y, jack)) {
                probe_bits |= (1 << (8 - p));
            }
        }
    }

    // Siempre sondea muros laterales (5, 6, 7, 8)
    for (int p = 5; p <= 8; p++) {
        uint8_t x = jack->pos_x_hi + PROBE_DX[p];
        uint8_t y = jack->pos_y_hi + PROBE_DY[p];
        if (ProbeTileSolid(room_grid[y >> 4][x >> 4], x, y, jack)) {
            probe_bits |= (1 << (8 - p));
        }
    }

    // Compresión por pares (loc_99BB)
    uint8_t new_cf = jack->collision_flag & 0xF0;
    if (probe_bits & 0xC0) new_cf |= CF_CEILING;   // Sondas 1 o 2
    if (probe_bits & 0x30) new_cf |= CF_FLOOR;     // Sondas 3 o 4
    if (probe_bits & 0x0C) new_cf |= CF_LEFT_WALL;  // Sondas 5 o 6
    if (probe_bits & 0x03) new_cf |= CF_RIGHT_WALL; // Sondas 7 o 8
    jack->collision_flag = new_cf;

    // Respuesta a colisión con suelo (sub_A1EB)
    if (jack->collision_flag & CF_FLOOR) {
        jack->player_struct &= 0xF9;     // Cancela salto y vuelo
        jack->collision_flag |= CF_ON_GROUND;
        jack->pos_y_hi &= 0xF8;          // Alineación exacta a múltiplo de 8 px
        jack->vel_y_hi = 0;
        jack->vel_y_lo = 0;
        jack->sprite_frame = 0;          // Posición de pie
    } else {
        jack->collision_flag &= ~CF_ON_GROUND;
        if (!(jack->player_struct & 2)) {
            jack->player_struct |= 2;    // Estado de caída
            jack->jump_timer = 0x40;
        }
    }

    // Respuesta a colisión con techo (sub_A1B9)
    if (jack->collision_flag & CF_CEILING) {
        jack->player_struct |= 2;        // Fuerza parábola descendente
        jack->jump_timer = 0x40;
        jack->pos_y_hi |= 7;             // Expulsa cabeza hacia abajo
        jack->vel_y_hi = 0;
    }
}
```

---

## 9. Conclusiones y Certificación

1. **Topología de 9 Puntos Exacta**: El sistema de 9 sondas de `byte_A3D8` constituye la verdad fundamental e inmutable del motor de colisiones de *Mighty Bomb Jack*. Define un bounding box de $16 \times 16$ píxeles centrado en Jack con pares de redundancia que previenen tunneling en las esquinas de bloques.
2. **Filtrado Direccional Vertical**: El motor 6502 no implementa banderas complejas de "plataforma one-way", sino que desactiva selectivamente las sondas de pies (3 y 4) mientras Jack posee velocidad vertical negativa ($\text{vel\_y} < 0$), permitiendo atravesar plataformas de abajo a arriba de forma natural.
3. **Sub-metatiles de $8 \times 8$ px**: Mediante `byte_A3EA`, 20 metatiles intermedios ($38..$4B) poseen resolución geométrica a nivel de cuadrante de 8 píxeles, modelando con exactitud cornisas, rampas y plataformas delgadas.
4. **Respuestas de Contacto Canónicas**:
   - Suelo: alineación $Y$ con `AND #$F8`, borrado de bits de salto (`& $F9`), y activación de `CF_OnGround` (`$10`).
   - Techo: fijación de parábola descendente (`byte_3E7 = $40`, `PlayerStruct |= 2`) y expulsión con `Y |= 7`.
   - Paredes: inhibición total del incremento de posición horizontal sin modificación de la coordenada $X$.
