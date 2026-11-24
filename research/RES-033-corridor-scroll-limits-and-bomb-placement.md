# RES-033: Límites de Scroll en Pasillos Horizontales y Posicionamiento de Bombas en Frontera

- **ID de Investigación**: `RES-033`
- **Tarea Asociada**: `TASK-F7-006`
- **Área Temática**: 03-Escenarios y Cámara / 05-Renderizado
- **Fecha**: 2026-10-25
- **Autor / Agente**: `asm_researcher` (Nivel 3)
- **Estado**: `[COMPLETADA]`

---

## 1. Objetivo y Preguntas de Investigación

El propósito de esta investigación es determinar el comportamiento canónico exacto del motor de NES (*Mighty Bomb Jack*, Tecmo 1986/1987) respecto a los límites de desplazamiento horizontal en pasillos continuos y la decodificación/renderizado de objetos interactivos (bombas y cofres), con el fin de resolver el bug reportado en la Fase 7:
> *"Hay incorrecciones con la generación de pantalla y bombas, por ejemplo cuando se llega al final del pasillo en el muro de la derecha se ven bombas dentro de la pared, hay que investigar cómo se comporta el scroll original cuando se llega a los límites de los pasillos."*

### Preguntas Clave a Responder:
1. **Comportamiento del Scroll en Pasillos Horizontales**:
   - ¿Cómo funciona la cinemática de la cámara y el scroll en pasillos (`MainSub_3`, `sub_A843`, `sub_B7B4`, `sub_B7BF`)?
   - ¿Cuánto mide exactamente un pasillo (por ejemplo, el Round 1: Salas 1 a 6 de `Section_1_1`)?
   - ¿Se detiene el scroll al alcanzar la última sala del pasillo (`max_scroll`), o la cámara se desplaza a lo largo de ella? ¿Cómo y cuándo se desancla Jack para caminar hacia el muro derecho?
   - ¿Cuál es la coordenada máxima de scroll y la posición de la pared derecha en la última sala (`Room 6` en el Round 1)?
2. **Generación y Posicionamiento de Bombas**:
   - ¿Cómo se calculan y proyectan las coordenadas $(X, Y)$ de las bombas en cada sub-sala (`RoomData_01..06`)?
   - ¿Por qué en `src/core/mjk_game.c`, `src/core/mjk_room_decoder.c` y `src/core/mjk_state_machine.c` aparecen bombas incrustadas en el muro derecho?
   - ¿Se están renderizando bombas de la sala adyacente derecha fuera del área visible? ¿Es Sala `$90` un vecino de scroll o una puerta de transición?

---

## 2. Fuentes de Verdad Analizadas (`asm/`)

| Archivo | Rango de Líneas | Identificador / Etiqueta | Descripción Funcional |
| :--- | :--- | :--- | :--- |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L1548-L1604) | `1548 - 1604` | `loc_8A06`..`loc_8A66` | Anclaje de Jack a $X=128$, desanclaje en `RF_AtScrollEdge` y re-anclaje bidireccional. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L2579-L2626) | `2579 - 2626` | `MaybeLoadRoomFlags` | Carga de `AdjacentRoomsTable` y extracción de flags de sala a `RoomStatusFlags`. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L3130-L3251) | `3130 - 3251` | `GameState_2_LoadRoom` | Carga de sección vía `SectionRoomsTable` e inicialización de `RoomDataRAM`. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L3257-L3341) | `3257 - 3341` | `ReadRoomData` | Decodificación del bytecode de objetos por sala (bombas, cofres, bloques). |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L5906-L6125) | `5906 - 6125` | `MainSub_3`, `loc_A454`, `loc_A505` | Motor de scroll horizontal, detección de bordes, avance de nametables y costura. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L6522-L6534) | `6522 - 6534` | `sub_A843` | Negación de velocidad horizontal (`byte_98:byte_99 = -PlayerXVel`). |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L6538-L6611) | `6538 - 6611` | `sub_A85F`, `sub_A875` | Inyección de metatiles de objetos de `MaybeRoomIDCopyAgain` en el buffer de costura. |
| [`asm/src/prg.asm`](../asm/src/prg.asm#L8588-L8656) | `8588 - 8656` | `sub_B7B4`, `sub_B7BF` | Traslación de actores y enemigos en coordenadas de pantalla aplicando deltas de cámara. |
| [`asm/src/data.asm`](../asm/src/data.asm#L640-L700) | `640 - 700` | `SectionRoomsTable`, `Section_1_1` | Tabla de composición de secciones y salas pertenecientes al pasillo inicial. |
| [`asm/src/data.asm`](../asm/src/data.asm#L1704-L1720) | `1704 - 1720` | `AdjacentRoomsTable` | Topología de salas (flags, arriba, abajo, derecha, izquierda). |
| [`asm/src/roomdata-us.asm`](../asm/src/roomdata-us.asm#L4-L16) | `4 - 16` | `RoomData_01` a `RoomData_06` | Bytecode de salas 1 a 6 de la revisión US. |
| [`asm/src/roomdata-us.asm`](../asm/src/roomdata-us.asm#L262-L264) | `262 - 264` | `RoomData_90` | Bytecode de la Sala Real de Bombas del Round 1 (Sala `$90`). |

---

## 3. Hallazgos Técnicos y Extracción de Datos

### 3.1. Arquitectura Topológica de Pasillos en NES

En el motor original de *Mighty Bomb Jack*, cada "Pasillo" (corredor horizontal) se compone de un conjunto finito de salas contiguas indexadas en `SectionRoomsTable` ([`asm/src/data.asm:640-686`](../asm/src/data.asm#L640-L686)). Para el Pasillo del Round 1 (`Section_1_1`):

```assembly
; asm/src/data.asm:684-686
Section_1_1:
	.BYTE    1,   2,   3,   4,   5,   6
	.BYTE    0
```

El pasillo contiene **exactamente 6 salas** (Salas 1, 2, 3, 4, 5 y 6). La Sala `$90` (la Sala de Bombas del Palacio Real) **NO forma parte del pasillo**. Es una sala de pantalla única (`RF_SingleScreen`, `RF_BombRoom`) a la que se accede exclusivamente a través de una puerta.

#### Dimensiones Exactas del Pasillo:
- Cada sala mide exactamente **16 metatiles de ancho** ($16 \times 16\text{ px} = 256\text{ px}$ de ancho) por **15 metatiles de alto** ($15 \times 16\text{ px} = 240\text{ px}$ de alto).
- El ancho total continuo del pasillo es:
  $$\text{Ancho Mundo} = 6 \times 256\text{ px} = 1536\text{ px}$$
- La coordenada X de la pared derecha en la última sala (`Room 6`) está en la columna metatile 15 ($15 \times 16 = 240\text{ px}$). En espacio global continuo de mundo:
  $$X_{\text{pared}} = (5 \times 256) + 240 = 1280 + 240 = 1520\text{ px}$$
  (los píxeles $1520..1535$ conforman el bloque sólido terminal).

---

### 3.2. Banderas de Topología en `AdjacentRoomsTable`

En [`asm/src/data.asm:1704-1720`](../asm/src/data.asm#L1704-L1720), cada sala define una entrada de 5 bytes:
`[Flags, Up, Down, Right, Left]`.

```assembly
; asm/src/data.asm:1706-1712
AdjacentRoomsTable:
	;    flag  [U]  [D]  [R]  [L]
	.BYTE    1,   0,   0,   0,   0	; Room 00 (Título / demo)
	.BYTE   $E,   0,   0,   2,   0	; Room 01: Inicio sección horizontal, derecha -> 2
	.BYTE    4,   0,   0,   3,   1	; Room 02: Interior pasillo horizontal
	.BYTE    4,   0,   0,   4,   2	; Room 03: Interior pasillo horizontal
	.BYTE    4,   0,   0,   5,   3	; Room 04: Interior pasillo horizontal
	.BYTE    4,   0,   0,   6,   4	; Room 05: Interior pasillo horizontal
	.BYTE    6,   0,   0, $90,   5	; Room 06: Fin sección horizontal, puerta -> $90
```

#### Decodificación Canónica del Byte 0 (`asm/src/prg.asm:2602-2615`):
- `Bit 0 ($01)`: `1` = `RF_SingleScreen` ($10 en `RoomStatusFlags`), sala fija sin scroll.
- `Bit 1 ($02)`: `1` = **Borde terminal de sección** (parada de scroll en avance Este o Sur).
- `Bit 2 ($04)`: `1` = Pasillo horizontal (`RF_ScrollStop` $20 en `RoomStatusFlags`), `0` = Pozo vertical.
- `Bit 3 ($08)`: `1` = **Borde inicial de sección** (parada de scroll en retroceso Oeste o Norte).

Análisis de las salas terminales:
1. **Sala 1 (`Flags = $0E`)**:
   - `Bit 3 = 1` y `Bit 1 = 1`: Borde inicial de sección horizontal. Jack inicia desanclado; al avanzar a la derecha y alcanzar $X=128$, se ancla y activa el scroll.
2. **Salas 2, 3, 4, 5 (`Flags = $04`)**:
   - `Bit 1 = 0` y `Bit 3 = 0`: Salas interiores normales. El scroll avanza libremente a $1.5\text{ px/frame}$.
3. **Sala 6 (`Flags = $06`)**:
   - `Bit 1 = 1` y `Bit 3 = 0`: **Borde terminal absoluto de sección horizontal**.
   - El campo `Right = $90` especifica el **destino de la puerta** al cruzar el umbral, **NO un vecino de scroll**.

---

### 3.3. Algoritmo de Bloqueo de Cámara y Desanclaje en Sala 6

El desmontaje de [`asm/src/prg.asm:6045-6111`](../asm/src/prg.asm#L6045-L6111) demuestra con certeza matemática el flujo de parada de scroll:

```
[Jack anclado en X=128 cruzando Sala 5 a la Derecha]
                      |
                      v
      ScrollX desborda 255 -> 0 (Carry = 1)
                      |
                      v
       CurrentRoomID pasa a ser 6 (loc_A55E)
                      |
                      v
     Carga flags de Sala 6: byte 0 = $06
    (Bit 1 activo: stop scrolling bottom/right)
    (Bit 3 inactivo: no es inicio de sección)
                      |
                      v
       loc_A554: RoomStatusFlags |= RF_Horizontal ($40)
                      |
                      v
 [Frame N: Seam Streamer dibuja última columna de Sala 6]
                      |
                      v
[Frame N+1: Jack camina a la derecha (PlayerXVelHi > 0)]
                      |
                      v
               Entrada a loc_A505:
            LDA RoomStatusFlags; ASL A
          (Bit 6 RF_Horizontal pasa a Bit 7 Negativo)
                      |
                      v
          BPL NO se cumple (es negativo)
                      |
                      v
          LDA #0; STA byte_3C2  --> ScrollX = 0 fijo!
          RoomStatusFlags |= RF_AtScrollEdge ($08)
          INC PPUUpdateFlag1; RTS
                      |
                      v
   ¡SCROLL DETENIDO TOTALMENTE EN ScrollX = 0!
   Jack se DESANCLA (ApplyPlayerXVelocity activo)
   Jack camina de X=128 a X=240 en pantalla.
```

#### Cita Textual de Ensamblador (`asm/src/prg.asm`):

1. **Detección del Fin de Sección al cruzar de Sala 5 a 6**:
```assembly
; asm/src/prg.asm:6078-6111
	LDA PlayerXVelHi
	ADC byte_3C2
	STA byte_3C2
	BCC loc_A581           ; Si no hay overflow, continúa
	LDAc RoomStatusFlags
	BMI loc_A57A           ; Si RF_SectionStart activo, salta
	LDY #3
	LDA (off_3D),Y         ; Lee vecino derecho (Sala 6)
	BNE loc_A55E

loc_A554:
	LDAc RoomStatusFlags
	ORA #RF_Horizontal     ; ACTIVA RF_Horizontal ($40)
	STAc RoomStatusFlags
	BNE loc_A58C

loc_A55E:
	STA CurrentRoomID       ; CurrentRoomID = 6
	JSR MaybeLoadRoomFlags  ; off_3D apunta a AdjacentRoomsTable[6]
	LDY #0
	LDA (off_3D),Y         ; A = $06
	AND #2                 ; Bit 1 activo? (Fin de sección)
loc_A56A:
	BEQ loc_A572
	LDA (off_3D),Y
	AND #8                 ; Bit 3 activo? (Inicio de sección)
	BEQ loc_A554           ; Bit 1=1 y Bit 3=0 -> SALTA A loc_A554!
```

2. **Bloqueo Inmediato de Scroll en el siguiente frame (`loc_A505`)**:
```assembly
; asm/src/prg.asm:6045-6055
loc_A505:
	LDA a:RoomStatusFlags
	ASL A                  ; Bit 6 (RF_Horizontal) pasa a Bit 7 (signo)
	BPL loc_A51C           ; Si RF_Horizontal está limpio, continúa scroll
	LDA #0
	STA byte_3C2           ; FUERZA ScrollX = 0 !
	LDA a:RoomStatusFlags
	ORA #RF_AtScrollEdge   ; ACTIVA RF_AtScrollEdge ($08) !
	STAc RoomStatusFlags
	INCc PPUUpdateFlag1    ; Señaliza buffer PPU inactivo
	RTS                    ; RETORNA SIN MOVER LA CÁMARA !
```

3. **Inhibición Total del Scroll mientras `RF_AtScrollEdge` está activo**:
```assembly
; asm/src/prg.asm:5929-5933
loc_A442:
	LDA #RF_AtScrollEdge
	BITc RoomStatusFlags
	BEQ loc_A44A
	RTS                    ; Si RF_AtScrollEdge activo, MainSub_3 SALE DE INMEDIATO !
```

4. **Desanclaje de Jack para caminar libremente hacia la pared**:
```assembly
; asm/src/prg.asm:1570-1587
	LDA #RF_ScrollStop
	BIT a:RoomStatusFlags
	BEQ loc_8A3E
	LSR A
	BIT a:RoomStatusFlags
	BNE loc_8A3E
	LDA #RF_AtScrollEdge   ; Bit 3 ($08)
	BIT a:RoomStatusFlags
	BEQ loc_8A66           ; Si RF_AtScrollEdge == 0: OMITE ApplyPlayerXVelocity (Jack ANCLADO)

loc_8A3E:
	JSR ApplyPlayerXVelocity ; Jack DESANCLADO: camina libremente en pantalla hacia X=240 !
```

#### Respuestas a las Preguntas 2b y 2c:
- **¿Cuánto mide el pasillo?**: 6 pantallas de 256 píxeles = $1536\text{ px}$.
- **¿Se detiene el scroll?**: **SÍ**. El scroll se detiene exactamente al entrar en la Sala 6.
- **¿Cuál es la coordenada máxima de scroll?**: `CurrentRoomID = 6`, `ScrollX = 0`. La cámara **NUNCA** hace scroll a través de la Sala 6; la Sala 6 completa permanece fija en pantalla.
- **Posición de Jack y la pared**: La pared derecha se encuentra en la columna 15 ($X = 240..255$) de la Sala 6. La puerta está en la columna 14 ($X = 224$). Jack se desancla en el centro de la pantalla ($X = 128$) y camina libremente hacia la derecha hasta tocar la puerta o la pared.

---

### 3.4. Cinemática de Actores y Enemigos (`sub_B7B4` / `sub_B7BF`)

El código en [`asm/src/prg.asm:8588-8656`](../asm/src/prg.asm#L8588-L8656) confirma la arquitectura descubierta en `RES-019`:
**Todos los actores, enemigos y monedas residen internamente en coordenadas de pantalla**.

```assembly
; asm/src/prg.asm:8588-8594
sub_B7B4:
	LDY #2                 ; Offset X en EnemyStruct
	LDX #0                 ; byte_98:byte_99 = Delta Cam X
	JSR sub_B7BF
	LDX #2                 ; byte_9A:byte_9B = Delta Cam Y
	LDY #8                 ; Offset Y en EnemyStruct (fallthrough)

; asm/src/prg.asm:8626-8655 (sub_B7BF)
loc_B7E4:
	LDA a:byte_98,X        ; Suma -PlayerXVelLo / -PlayerYVelLo
	CLC
	ADC byte_0
	STA byte_0
	LDA a:byte_99,X        ; Suma -PlayerXVelHi / -PlayerYVelHi
	ADC byte_1
	STA byte_1
	...
	ADC (EnemyStructPointer),Y
	STA (EnemyStructPointer),Y ; Actualiza posición en pantalla del enemigo
```

- Cuando la cámara se desplaza hacia la derecha a $+1.5\text{ px/frame}$, `sub_A843` fija `byte_98:byte_99 = -0x0180`. `sub_B7B4` aplica este delta negativo a todos los enemigos activos en pantalla, desplazándolos hacia la izquierda a $-1.5\text{ px/frame}$.
- Cuando la cámara se bloquea en Sala 6 (`RF_AtScrollEdge` activo), `byte_98` y `byte_99` permanecen en $0$. Los enemigos se desplazan exclusivamente por su propia velocidad, sin delta de cámara.

---

### 3.5. Generación y Posicionamiento de Bombas (`RoomData_01..06`)

Las bombas se codifican en el bytecode de sala como un byte de conteo seguido de pares empacados $YX$ (`byte >> 4` = fila $Y$, `byte & 0x0F` = columna $X$):

| Sala | Archivo y Línea | Bytecode de Bombas | Coordenadas $(Y, X)$ Metatile | Píxeles $(Y, X)$ en Pantalla |
| :--- | :--- | :--- | :--- | :--- |
| **Sala 1** | `roomdata-us.asm:5` | `5, $85, $87, $39, $6B, $6D` | $(8, 5), (8, 7), (3, 9), (6, 11), (6, 13)$ | $(128, 80), (128, 112), (48, 144), (96, 176), (96, 208)$ |
| **Sala 2** | `roomdata-us.asm:7` | `6, $41, $43, $67, $69, $8D, $8F` | $(4, 1), (4, 3), (6, 7), (6, 9), (8, 13), (8, 15)$ | $(64, 16), (64, 48), (96, 112), (96, 144), (128, 208), (128, 240)$ |
| **Sala 3** | `roomdata-us.asm:9` | `4, $43, $47, $64, $66` | $(4, 3), (4, 7), (6, 4), (6, 6)$ | $(64, 48), (64, 112), (96, 64), (96, 96)$ |
| **Sala 4** | `roomdata-us.asm:11` | `5, $65, $67, $69, $4B, $4D` | $(6, 5), (6, 7), (6, 9), (4, 11), (4, 13)$ | $(96, 80), (96, 112), (96, 144), (64, 176), (64, 208)$ |
| **Sala 5** | `roomdata-us.asm:13` | `6, $B5, $95, $75, $55, $8B, $8E` | $(11, 5), (9, 5), (7, 5), (5, 5), (8, 11), (8, 14)$ | $(176, 80), (144, 80), (112, 80), (80, 80), (128, 176), (128, 224)$ |
| **Sala 6** | `roomdata-us.asm:15` | `6, $22, $44, $26, $48, $92, $94` | $(2, 2), (4, 4), (2, 6), (4, 8), (9, 2), (9, 4)$ | $(32, 32), (64, 64), (32, 96), (64, 128), (144, 32), (144, 64)$ |

#### Hallazgo Fundamental sobre la Sala 6:
En `RoomData_06`:
- Las coordenadas de columna $X$ de las 6 bombas son: $X = 2, 4, 6, 8, 2, 4$.
- **La coordenada $X$ máxima es 8 ($128\text{ px}$)**.
- **NO EXISTE NINGUNA BOMBA en las columnas 9 a 15 de la Sala 6**.
- La pared derecha está en la columna 15 ($X = 240$).
- **Por tanto, en el juego original es IMPOSIBLE que existan bombas dentro del muro derecho ni cerca de él**.

---

## 4. Diagnóstico Forense del Bug en el Código C

Al contrastar la arquitectura original contra el código fuente C de `src/core/`, se identificó una cadena de **cuatro defectos lógicos acumulativos** que causan la aparición de bombas dentro de la pared derecha:

### Defecto 1: Precarga de la Sala de Bombas (`$90`) como vecino contiguo de pasillo
- **Ubicación**: [`src/core/mjk_state_machine.c:250-255`](../src/core/mjk_state_machine.c#L250-L255) y [`lines 571-580`](../src/core/mjk_state_machine.c#L571-L580):
  ```c
  if (room_id < MJK_TOTAL_ROOMS_COUNT) {
      const MjkAdjacentRoom* adj = &g_adjacent_rooms[room_id];
      if (adj->right != 0) {
          AppendRoomObjects(game, adj->right);
      }
  }
  ```
- **Mecanismo de Falla**:
  Cuando `room_id == 6`, `g_adjacent_rooms[6].right == 0x90` ($144$ decimal).
  El código interpretó ciegamente que `adj->right != 0` indicaba una sala contigua de pasillo y llamó a `AppendRoomObjects(game, 0x90)`.
  Esto decodificó `RoomData_90` (la Sala Real del Round 1) e insertó sus **24 bombas** dentro del array `game->bombs` con `room_id = 0x90`.
  Dado que `bombs[24]` tiene capacidad para 24 elementos y Sala 6 aportó 6 bombas, las siguientes 18 bombas de la Sala `$90` saturaron el array.

### Defecto 2: Omisión del chequeo de `RF_Horizontal` al desplazarse a la derecha
- **Ubicación**: [`src/core/mjk_camera.c:280-286`](../src/core/mjk_camera.c#L280-L286):
  ```c
  } else {
      // Moviendo a la derecha (prg.asm:6070-6111):
      // Limpia flag de inicio de sección (loc_A57A)
      cam->room_flags &= ~MJK_RF_SECTION_START;

      uint32_t total_x = (uint32_t)cam->scroll_x_fixed + (uint16_t)vx;
      if (total_x < 0x10000) {
          cam->scroll_x_fixed = (uint16_t)total_x;
      }
  ```
- **Mecanismo de Falla**:
  El código C omitió la condición canónica de [`asm/src/prg.asm:6045-6055`](../asm/src/prg.asm#L6045-L6055) (`loc_A505`):
  ```c
  if (cam->room_flags & MJK_RF_HORIZONTAL) {
      cam->scroll_x_fixed = 0;
      cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
      cam->at_scroll_edge = true;
      return;
  }
  ```
  Al no tener esta guarda, cuando el scroll desbordó de Sala 5 a Sala 6 y activó `MJK_RF_HORIZONTAL`, en los fotogramas siguientes la cámara **continuó desplazándose a lo largo de la Sala 6** (`cam->scroll_x_fixed` creció de $0$ a $255$), mientras Jack permanecía anclado erróneamente en $X=128$.

### Defecto 3: Blitteo indebido del nametable de la Sala `$90` sobre la Sala 6
- **Ubicación**: [`src/core/mjk_game.c:654-672`](../src/core/mjk_game.c#L654-L672):
  ```c
  if (game->camera_x > 0 && adj->right != 0) {
      uint8_t right_nametable[30][32];
      MjkRoom_BuildNametable(adj->right, right_nametable, ...);

      for (uint8_t b = 0; b < game->bomb_count; ++b) {
          if (game->bombs[b].room_id == adj->right && !game->bombs[b].collected) {
              // Estampa bombas de adj->right en right_nametable
          }
      }

      for (int ty = 0; ty < 30; ++ty) {
          for (int tx = 0; tx < 32; ++tx) {
              int px = (256 - game->camera_x) + tx * 8;
              MjkPPU_BlitTile(fb, px, ty * 8, ...);
          }
      }
  }
  ```
- **Mecanismo de Falla**:
  Como `game->camera_x > 0` ocurrió indebidamente en Sala 6 y `adj->right == 0x90`, el renderer construyó el nametable de la Sala `$90`, le inyectó las bombas de la Sala `$90`, y lo dibujó proyectado con offset `(256 - game->camera_x)`.
  A medida que `camera_x` aumentaba, las bombas de la Sala `$90` (que tienen coordenadas $X = 2, 3, 4, 11, 13$) se deslizaron sobre el borde derecho de la pantalla, **superponiéndose y estampándose directamente sobre los metatiles del muro terminal de la Sala 6**.

### Defecto 4: Proyección errónea en recolección y colisiones
- **Ubicación**: [`src/core/mjk_room_decoder.c:454-455`](../src/core/mjk_room_decoder.c#L454-L455) y [`src/core/mjk_player.c:662`](../src/core/mjk_player.c#L662):
  ```c
  } else if (game->jack.room_id < MJK_TOTAL_ROOMS_COUNT && g_adjacent_rooms[game->jack.room_id].right == bomb->room_id) {
      screen_bx = (256 - game->camera_x) + (bomb->tile_x * 16);
  }
  ```
- **Mecanismo de Falla**:
  Se asume que cualquier bomba con `bomb->room_id == adj->right` es visible en pantalla a `(256 - camera_x)`, ignorando si `adj->right` es una sala de pantalla única o si la sala actual ya alcanzó el límite de scroll de sección.

---

## 5. Especificación de Implementación y Criterios de Paridad

Para alcanzar paridad 100% canónica con el 6502 de NES, el desarrollador debe implementar los siguientes cambios en la arquitectura:

### 5.1. Regla Canónica de Adyacencia para Pasillos
Una sala adyacente `adj->right` solo es un vecino de scroll horizontal si:
1. `adj->right != 0`.
2. `adj->right < 0x90` (no es una Sala Real de Bombas `$90..$9F` ni sala secreta especial `$A0..$FF`).
3. La sala actual no es terminal de avance horizontal: `(g_adjacent_rooms[room_id].flags & 0x02) == 0`.

```c
static inline bool MjkRoom_HasHorizontalScrollNeighborRight(uint8_t room_id) {
    if (room_id >= MJK_TOTAL_ROOMS_COUNT) return false;
    const MjkAdjacentRoom* adj = &g_adjacent_rooms[room_id];
    if (adj->right == 0 || adj->right >= 0x90) return false;
    // Si bit 1 está activo y bit 3 inactivo, es terminal Este (fin de sección)
    if ((adj->flags & 0x02) && !(adj->flags & 0x08)) return false;
    return true;
}
```

### 5.2. Corrección en `mjk_camera.c` (`camera_update_kinematics`)
Restaurar la guarda canónica de `loc_A505` al desplazarse a la derecha:

```c
        } else {
            // Moviendo a la derecha (prg.asm:6045-6055, loc_A505):
            if (cam->room_flags & MJK_RF_HORIZONTAL) {
                cam->scroll_x_fixed = 0;
                cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
                cam->at_scroll_edge = true;
                return;
            }

            cam->room_flags &= ~MJK_RF_SECTION_START;
            uint32_t total_x = (uint32_t)cam->scroll_x_fixed + (uint16_t)vx;
            ...
```

Y al desbordar 256 píxeles al entrar a la nueva sala (`loc_A55E`):
```c
            if (next_adj->right == 0 || (next_adj->flags & 0x02)) {
                cam->room_flags |= MJK_RF_HORIZONTAL;
                // En el frame exacto de entrada a sala terminal, el scroll queda anclado en 0
                cam->scroll_x_fixed = 0;
            }
```

### 5.3. Corrección en `mjk_state_machine.c` (`LoadRoomEntities`)
Pre-cargar objetos únicamente si la sala contigua es un vecino de scroll válido (nunca para salas `$90..$9F` ni extremos de sección):

```c
    // 2. Si es pasillo con scroll continuo hacia la derecha, precargar objetos de sala contigua válida
    if (room_id < MJK_TOTAL_ROOMS_COUNT && MjkRoom_HasHorizontalScrollNeighborRight(room_id)) {
        const MjkAdjacentRoom* adj = &g_adjacent_rooms[room_id];
        AppendRoomObjects(game, adj->right);
    }
```

### 5.4. Corrección en `mjk_game.c` (`RenderInGameScreen`)
La costura derecha sólo debe generarse si la sala vecina es de scroll válido:

```c
    // Costura horizontal derecha (CurrentRoomID -> RightRoom):
    if (game->camera_x > 0 && MjkRoom_HasHorizontalScrollNeighborRight(room_id)) {
        ...
```

---

## 6. Plan de Pruebas Unitarias / Aceptación

Para certificar la resolución del defecto, se recomienda crear la suite `tests/test_corridor_limits.c`:

- [ ] **Test 1 (`test_sala_6_scroll_stop`)**:
  - Inicializar cámara en Sala 5 con $X=250$.
  - Avanzar con Jack hacia la derecha.
  - Verificar que al transitar a Sala 6 (`CurrentRoomID = 6`), `ScrollX` queda exactamente en $0$.
  - Verificar que `cam.at_scroll_edge == true` y `MJK_RF_AT_SCROLL_EDGE` está activo.
  - Comprobar que en frames sucesivos con Jack moviéndose a la derecha, `ScrollX` permanece inmutablemente en $0$.
- [ ] **Test 2 (`test_sala_6_jack_unanchored_to_wall`)**:
  - En Sala 6 con `ScrollX = 0`, Jack avanza a la derecha.
  - Verificar que Jack no está anclado en $X=128$, sino que su coordenada de pantalla avanza: $128 \to 160 \to 200 \to 224$ (puerta) $\to 240$ (muro).
- [ ] **Test 3 (`test_no_bomb_room_preload_in_corridor`)**:
  - Ejecutar `LoadRoomEntities` para Sala 6.
  - Verificar que `game.bomb_count == 6` (solo las 6 bombas de `RoomData_06`).
  - Verificar que ninguna bomba tiene `room_id == 0x90`.
  - Comprobar que todas las bombas tienen $X \le 8$ ($128\text{ px}$) y que no existe ninguna entidad en el muro derecho ($X = 15$).

---

## 7. Referencias Cruzadas

- **Investigación Base de Scroll**: [`RES-019: Cinemática de Cámara, Motor de Scroll y Streaming de Seam`](RES-019-scroll-engine-and-camera.md)
- **Investigación de Seam Parity**: [`RES-019B: Ensamblado Continuo de Pasillos y Cinemática de Desanclaje`](RES-019B-corridor-world-assembly-and-seam-parity.md)
- **Investigación de Objetos y Bytecode**: [`RES-020: Bytecode de RoomData y Mapeo de Objetos`](RES-020-roomdata-object-mapping-and-emulator-parity.md)
- **Investigación del Sistema de Puertas**: [`RES-017: Sistema de Puertas y Transiciones`](RES-017-door-system-and-transitions.md)
