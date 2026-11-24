# RES-019: Cinemática de Cámara, Motor de Scroll y Streaming de Seam

- **ID Técnico**: `RES-019`
- **Subsistema**: Cámara, Scroll de Escenarios y Streaming de Nametables PPU
- **Fase del Proyecto**: Fase 6 (Implementación de Source Port en C99)
- **Tarea**: `TASK-F6-024`
- **Estado de Epistemología**: `[CONFIRMADA]` (Verificada exhaustivamente contra `asm/src/prg.asm`, `asm/src/data.asm` y `asm/src/ram.asm`)
- **Autores**: `asm_researcher` (Nivel 3)

---

## 1. Visión General del Sistema de Cámara y Scroll

En *Mighty Bomb Jack* (NES, 1987), el cartucho físico utiliza **Mirroring Horizontal fijo** en hardware (iNES header byte 6 bit 0 = 0). A diferencia de otros juegos de plataformas con scroll libre multidireccional o scrolling suave con mappers MMC3, el motor de *Mighty Bomb Jack* implementa tres tipologías arquitectónicas de pantalla estrictamente diferenciadas según la topología de la sala:

1. **Salas de Pantalla Única (Fixed Single Screen)**:
   - Salas Reales (Palacio / Bomb Rooms), salas de tortura y salas secretas sin scroll.
   - La cámara está fija en `(ScrollX = 0, ScrollY = 0)`.
   - Jack se desplaza libremente en coordenadas de pantalla (`0..255`, `0..239`).
2. **Pasillos de Scroll Horizontal (Horizontal Corridors)**:
   - Pasillos formados por 2 a 6 pantallas contiguas en sentido horizontal (Este-Oeste).
   - Debido al Mirroring Horizontal del cartucho, solo existe un espacio horizontal nativo de 256 píxeles en VRAM (los nametables `$2000` y `$2400` colapsan a la misma memoria física).
   - El motor oculta la columna izquierda de 8 píxeles (`PPUMask_ShowLeft8Pixels` desactivado en `$2001`) y actualiza en tiempo real columnas de metatiles (`$1D` bytes de alto con PPU increment +32) justo en el seam de avance.
   - Jack está anclado en el centro horizontal exacto de la pantalla (`X = 128 / $80`). Jack camina sin moverse en pantalla mientras el scroll mueve el mundo a velocidad constante de `1.5 px/frame`. Al alcanzar los límites del pasillo, Jack se desancla y camina hacia el borde.
3. **Pasajes de Scroll Vertical (Vertical Shafts / Towers)**:
   - Pozos verticales de descenso o ascenso formados por 2 a 8 pantallas contiguas.
   - El Mirroring Horizontal provee dos nametables independientes verticalmente: Nametable 0 (`$2000`) y Nametable 2 (`$2800`).
   - El bit 1 de `$2000` (`PPUCTRL`) se conmuta alternativamente (`EOR #2`) al cruzar el límite de 240 líneas (`$F0`).
   - El motor transmite filas dobles completas de metatiles (`$40` bytes = 64 bytes) en VBlank.
   - Jack está anclado en el centro vertical de la pantalla (`Y = 121 / $79`). Mientras la cámara se desplaza, Jack permanece en `Y = 121` y su velocidad vertical (`PlayerYVel`) acciona el scroll. Al llegar a los límites (techo o fondo), Jack se desancla.

---

## 2. Mapa de Variables de RAM e Invariantes Hardware

### 2.1 Variables de RAM Centrales (`asm/src/ram.asm`)

| Dirección RAM | Símbolo Ensamblador | Tipo | Descripción Canónica |
|---|---|---|---|
| `$00BC` / `$03BE` | `RoomStatusFlags` | `uint8_t` | Registro de estado y topología de la sala activa (flags `RF_*`). |
| `$03BF` | `CurrentRoomID` | `uint8_t` | Identificador de la sala principal visible. |
| `$03C0` | `CheckpointRoomIDMaybe` | `uint8_t` | ID de sala de checkpoint o respaldo. |
| `$03C1` | `byte_3C1` | `uint8_t` | Subpíxel fraccionario de Scroll X (`8.8` fixed point, parte baja). |
| `$03C2` | `byte_3C2` | `uint8_t` | Píxel entero de Scroll X (`0..255`), escrito en `$2005` (1º write). |
| `$03C3` | `byte_3C3` | `uint8_t` | Subpíxel fraccionario de Scroll Y (`8.8` fixed point, parte baja). |
| `$03C4` | `byte_3C4` | `uint8_t` | Píxel entero de Scroll Y (`0..239`), escrito en `$2005` (2º write). |
| `$03C5` | `CurrentRoomIDBackup` | `uint8_t` | Copia de respaldo de la sala para validación de seam. |
| `$03C6` | `byte_3C6` | `uint8_t` | Coordenada Y temporal / seam line index para cálculo de fila. |
| `$03C7` | `MaybeRoomIDCopyAgain` | `uint8_t` | ID de la sala adyacente cargada en el seam streamer. |
| `$03C8` | `byte_3C8` | `int8_t` | Caché de dirección de scroll X (`PlayerXVelHi`) para dirty tracking. |
| `$03C9` | `byte_3C9` | `int8_t` | Caché de dirección de scroll Y (`PlayerYVelHi`) para dirty tracking. |
| `$03CA` | `byte_3CA` | `uint8_t` | Columna X previa o PPUADDR Hi previo para evitar streams redundantes. |
| `$03CB` | `byte_3CB` | `uint8_t` | PPUADDR Lo previo de fila vertical. |
| `$0098` / `$0099` | `byte_98`, `byte_99` | `int16_t` | Delta de desplazamiento X de cámara (`-PlayerXVel`), aplicado a enemigos/objetos. |
| `$009A` / `$009B` | `byte_9A`, `byte_9B` | `int16_t` | Delta de desplazamiento Y de cámara (`-PlayerYVel`), aplicado a enemigos/objetos. |
| `$00F2` (US) | `PPUUpdateFlag1` | `uint8_t` | Flag de petición de actualización VRAM en NMI (0 = buffer listo, >0 = omitir). |
| `$03EE` | `byte_3EE` | `uint8_t` | Máscara de control PPUCTRL (bit 2 = 4 para incremento vertical +32, 0 para +1). |
| `$03EF` | `byte_3EF` | `uint8_t` | Byte alto de dirección destino VRAM (`PPUADDR`). |
| `$03F0` | `byte_3F0` | `uint8_t` | Byte bajo de dirección destino VRAM (`PPUADDR`). |
| `$03F1` | `byte_3F1` | `uint8_t` | Longitud en bytes de la transferencia VRAM (`$1D` = 29 col, `$40` = 64 row). |
| `$03F2..$0431` | `PPUUpdateBuffer` | `uint8_t[64]` | Buffer temporal en RAM de tiles a transferir a VRAM durante el VBlank. |

---

## 3. Topología de Salas: Estructura de `AdjacentRoomsTable`

En `asm/src/data.asm:1704-1800`, la tabla `AdjacentRoomsTable` define la conectividad y tipología de cada sala mediante bloques de **5 bytes por sala**:

```
Offset 0: Flags de sala (Topología y límites)
Offset 1: ID de Sala Adyacente ARRIBA (North / Up)
Offset 2: ID de Sala Adyacente ABAJO (South / Down)
Offset 3: ID de Sala Adyacente DERECHA (East / Right)
Offset 4: ID de Sala Adyacente IZQUIERDA (West / Left)
```

### 3.1 Decodificación de Flags del Byte 0 de la Sala (`asm/src/prg.asm:2602-2625`)

La rutina `MaybeLoadRoomFlags` (`asm/src/prg.asm:2579-2626`) mapea los bits del byte 0 de la tabla a `RoomStatusFlags`:

```assembly
; asm/src/prg.asm:2602-2615
	LDY #0
	LDX #0
	LDA (off_3D),Y
	LSR A
	BCC loc_9009
	LDX #RF_SingleScreen     ; Bit 0 original ($01) -> RF_SingleScreen ($10)

loc_9009:
	AND #2                   ; Bit 2 original ($04) -> RF_ScrollStop ($20)
	BEQ loc_9011
	TXA
	ORA #RF_ScrollStop
	TAX
```

| Bit Original en Tabla | Máscara | Significado en Tabla | Mapeo a `RoomStatusFlags` |
|---|---|---|---|
| Bit 0 | `$01` | Sala de Pantalla Única (Fixed) | `RF_SingleScreen` (`%00010000 = $10`) |
| Bit 1 | `$02` | Fin de sección (Derecha o Abajo) | Utilizado en inicialización de scroll |
| Bit 2 | `$04` | Orientación: `1` = Horizontal, `0` = Vertical | `RF_ScrollStop` (`%00100000 = $20`) |
| Bit 3 | `$08` | Inicio de sección (Izquierda o Arriba) | Utilizado en inicialización de scroll |

### 3.2 Semántica Completa de `RoomStatusFlags` (`asm/src/defs.asm:215-222`)

```assembly
RF_1            = 1         ; %00000001: Flag interno / misceláneo
RF_BombRoom     = %10       ; %00000010 ($02): Sala Real / Bomb Room (A y B aletean)
RF_4            = %100      ; %00000100 ($04): Configuración de puerta de entrada
RF_AtScrollEdge = %1000     ; %00001000 ($08): Cámara bloqueada en el extremo; Jack se mueve libre en pantalla
RF_SingleScreen = %10000    ; %00010000 ($10): Pantalla fija, scroll desactivado (0, 0)
RF_ScrollStop   = %100000   ; %00100000 ($20): Modo de Scroll (1 = Horizontal, 0 = Vertical si SingleScreen=0)
RF_Horizontal   = %1000000  ; %01000000 ($40): Borde Este (Horizontal) o Borde Sur (Vertical) pendiente
RF_SectionStart = %10000000 ; %10000000 ($80): Borde Oeste (Horizontal) o Borde Norte (Vertical) pendiente
```

> **NOTA CRÍTICA SOBRE NOMENCLATURA**:
> En el desmontaje histórico, el bit 5 fue bautizado como `RF_ScrollStop`. En realidad, cuando `RF_SingleScreen` es 0, `RF_ScrollStop == 1` define un **Pasillo Horizontal**, mientras que `RF_ScrollStop == 0` define un **Pozo Vertical**.

---

## 4. Cinemática de Jack vs Cámara (Deadzones y Anclaje)

La interacción entre Jack y el desplazamiento de cámara se gobierna en dos puntos críticos:
- **Eje Y**: `asm/src/prg.asm:1487-1518`.
- **Eje X**: `asm/src/prg.asm:1548-1604`.

```
                        ESPACIO DE PANTALLA (256 x 240)
       (0,0) +---------------------------------------------------+
             |                                                   |
             |                   Y = 121 ($79)                   |
             | - - - - - - - - - - - - - - - - - - - - - - - - - | <-- Deadzone Vertical
             |                    |   Jack   |                   |     (Ancla vertical)
             |                    |  Anclado |                   |
             |                    +----------+                   |
             |                          |                        |
             |                          v                        |
             |                    X = 128 ($80)                  |
             |                 (Deadzone Horizontal)             |
             |                                                   |
             +---------------------------------------------------+ (255, 239)
```

### 4.1 Cinemática en Pasillos Horizontales (`RF_ScrollStop = $20`)

1. **Estado de Scroll Activo (`(RoomStatusFlags & RF_AtScrollEdge) == 0`)**:
   - Jack está anclado fijamente en `PlayerXPosHi = 128` (`$80`).
   - El código en `asm/src/prg.asm:1578` ejecuta `BEQ loc_8A66`, **omitiendo la rutina `ApplyPlayerXVelocity`**.
   - Jack ejecuta su animación de correr en el sitio sobre `X = 128`.
   - `PlayerXVel` (`+1.5 px/frame` a la derecha, `-1.5 px/frame` a la izquierda) se aplica a `byte_3C1:byte_3C2` en `MainSub_3` (`loc_A498` / `loc_A531`).
   - La cámara avanza exactamente a `1.5 píxeles/frame`.
2. **Llegada al Límite del Pasillo**:
   - Al alcanzar el extremo izquierdo (`ScrollX <= $F8`) o derecho (`ScrollX == 0` sin sala contigua), `MainSub_3` activa el flag `RoomStatusFlags |= RF_AtScrollEdge`.
   - Una vez activo `RF_AtScrollEdge`, `asm/src/prg.asm:1581` **ejecuta `ApplyPlayerXVelocity`**: Jack se desancla de `X = 128` y se desplaza físicamente por la pantalla hacia la pared.
3. **Reanudación del Scroll (Salida del Borde)**:
   - Mientras Jack se mueve con `RF_AtScrollEdge`, se evalúa continuamente si Jack se da la vuelta y cruza el centro de pantalla (`$80`):
     ```assembly
     ; asm/src/prg.asm:1588-1603
     LDA PlayerXPosHi
     CMP #$80
     PHP
     TXA
     BPL loc_8A5B            ; Si va a la derecha, Carry si X >= $80
     PLA
     EOR #1                  ; Si va a la izquierda, invierte Carry: Carry si X < $80
     PHA
     loc_8A5B:
     PLP
     BCC loc_8A66
     LDA a:RoomStatusFlags
     AND #~RF_AtScrollEdge
     STA a:RoomStatusFlags   ; Limpia RF_AtScrollEdge: ¡Jack vuelve a anclarse!
     ```
   - Al cruzar `$80` hacia el interior, se limpia `RF_AtScrollEdge`, Jack queda anclado en `$80` y el scroll reanuda el movimiento.

### 4.2 Cinemática en Pozos Verticales (`RF_ScrollStop = 0`, `RF_SingleScreen = 0`)

1. **Estado de Scroll Activo (`(RoomStatusFlags & RF_AtScrollEdge) == 0`)**:
   - Jack está anclado en `PlayerYPosHi = 121` (`$79`).
   - El código en `asm/src/prg.asm:1493` ejecuta `BEQ loc_89D6`, **omitiendo `ApplyPlayerYVelocity`**.
   - La velocidad vertical del jugador (`PlayerYVel`, calculada por la gravedad, saltos y aleteos) se transfiere directamente a `byte_3C3:byte_3C4` (Scroll Y) en `loc_A66F` (subiendo) o `loc_A716` (bajando).
2. **Llegada al Límite de la Torre**:
   - Al llegar al techo (`ScrollY == 0` y no hay sala superior) o al fondo (`ScrollY == 0` y no hay sala inferior), `MainSub_3` fija `RoomStatusFlags |= RF_AtScrollEdge`.
   - Se habilita `ApplyPlayerYVelocity`: Jack abandona `Y = 121` y sube hacia el techo o cae hacia el suelo (`Y = 224 / $E0`).
3. **Reanudación del Scroll Vertical**:
   - Cuando Jack revierte la marcha y cruza `Y = 121` (`$79`), se invierte el carry según el signo de `PlayerYVelHi` (`asm/src/prg.asm:1501-1517`).
   - Al cruzar `$79` hacia el interior, se ejecuta `RoomStatusFlags &= ~RF_AtScrollEdge` y Jack vuelve a quedar anclado a la cámara.

---

## 5. Algoritmo de Avance de Scroll y Streaming de Nametables (`MainSub_3`)

La función `MainSub_3` (`asm/src/prg.asm:5906-6512`) es el núcleo del motor de scroll. Se ejecuta en cada frame durante `GameState_5_Main`.

### 5.1 Despacho de Modos (`prg.asm:5906-5940`)

```c
void MainSub_3(void) {
    MaybeHandleDoorOpening();
    delta_cam_x = 0;
    delta_cam_y = 0;
    
    MaybeLoadRoomFlags(CurrentRoomID);
    PPUUpdateFlag1 = 1; // Por defecto no hay buffer pendiente
    
    if (RoomStatusFlags & RF_SingleScreen) {
        PPUMaskMirror |= 0x06; // Muestra 8px izquierdos (BG y sprites)
        ScrollX = 0;
        ScrollY = 0;
        return;
    }
    
    if (RoomStatusFlags & RF_AtScrollEdge) {
        return; // Extremo alcanzado: la cámara no se mueve
    }
    
    if (RoomStatusFlags & RF_ScrollStop) {
        MainSub_3_HorizontalScroll();
    } else {
        MainSub_3_VerticalScroll();
    }
}
```

### 5.2 Desplazamiento Horizontal y Streaming de Columnas (`loc_A454-A631`)

1. **Ocultación de Borde Izquierdo**:
   `PPUMaskMirror &= ~(PPUMask_ShowLeft8Pixels_BG | PPUMask_ShowLeft8Pixels_SPR)` (`asm/src/prg.asm:5944`).
   Evita que los artefactos visuales de escritura en el seam sean visibles en el borde izquierdo del televisor.
2. **Cálculo de Delta y Negación**:
   La velocidad del jugador se invierte mediante `sub_A843` (`byte_98 = -PlayerXVelLo`, `byte_99 = -PlayerXVelHi`).
3. **Actualización de Posición `8.8`**:
   `ScrollX_Fixed += PlayerXVel`.
4. **Detección de Cruce de Pantalla (Overflow / Underflow de 256 px)**:
   - **Moviendo a la Izquierda (`PlayerXVel < 0`)**:
     - Si hay underflow (`Borrow`), consulta `AdjacentRoomsTable[CurrentRoomID].left`:
       - Si existe sala izquierda (`!= 0`): `CurrentRoomID = room_left`.
       - Si no existe: `RoomStatusFlags |= RF_SectionStart`. Al llegar a `ScrollX <= $F8`, fija `ScrollX = $F8` y activa `RF_AtScrollEdge`.
   - **Moviendo a la Derecha (`PlayerXVel > 0`)**:
     - Si hay overflow (`Carry`), consulta `AdjacentRoomsTable[CurrentRoomID].right`:
       - Si existe sala derecha (`!= 0`): `CurrentRoomID = room_right`.
       - Si no existe: fija `ScrollX = 0` y activa `RF_AtScrollEdge`.
5. **Streaming de la Columna VRAM (`loc_A58C-A631`)**:
   - Columna destino: `tile_x = (ScrollX + (PlayerXVelHi < 0 ? 8 : 0)) >> 3`.
   - Parámetros PPU:
     - `byte_3EE = 4` (PPUCTRL bit 2 = 1 -> incremento VRAM de +32 en cada escritura).
     - `byte_3EF = $20` (Nametable base $2000).
     - `byte_3F0 = tile_x` (Offset de columna $00..$1F).
     - `byte_3F1 = $1D` (29 metatiles de alto = 232 líneas jugables).
   - Dirty check (`byte_3C8`, `byte_3CA`): si la dirección de marcha no cambió y `tile_x == last_tile_x`, se aborta el streaming (`PPUUpdateFlag1 = 1`).
   - Carga y desempaquetado de metatiles: lee 15 metatiles (30 tiles verticales de 8x8) y los escribe en `PPUUpdateBuffer` (`$03F2`).

### 5.3 Desplazamiento Vertical y Alternancia de Nametables (`loc_A63C-A7E0`)

1. **Restauración de Pantalla Completa**:
   `PPUMaskMirror |= 0x06` (`asm/src/prg.asm:6232`). En scroll vertical no se requiere clipping de 8 píxeles.
2. **Cálculo de Delta y Negación**:
   `sub_A851` fija `byte_9A = -PlayerYVelLo`, `byte_9B = -PlayerYVelHi`.
3. **Aritmética Módulo 240 Líneas**:
   El scroll vertical del NES solo admite valores de 0 a 239 (`$00..$EF`). Los valores `$F0..$FF` se consideran fuera de rango.
   - Si `ScrollY >= 240`: `ScrollY += 15` (salta las 16 líneas no visibles para envolver a 0).
4. **Alternancia de Nametable Vertical (`PPUCTRL` bit 1)**:
   Al rebasar el límite superior o inferior y pasar a la siguiente sala contigua:
   ```assembly
   ; asm/src/prg.asm:6310, 6381
   LDA a:PPUCtrlMirror
   EOR #2
   STA a:PPUCtrlMirror
   ```
   Conmuta el bit 1 de `$2000` entre 0 (Nametable `$2000`) y 1 (Nametable `$2800`), permitiendo un recorrido vertical continuo entre las dos pantallas físicas.
5. **Streaming de Fila Doble de Metatiles (`loc_A784-A839`)**:
   - Parámetros PPU:
     - `byte_3EE = 0` (incremento horizontal estándar de +1).
     - `byte_3EF = ($20 | (nametable_bit << 3)) | (row_hi)`.
     - `byte_3F1 = $40` (64 bytes = 2 filas completas de 32 tiles de 8x8 = 1 fila de metatiles de 16x16).

---

## 6. Sistema de Coordenadas de Entidades y Proyección a Pantalla

Un descubrimiento arquitectónico fundamental verificado en `asm/src/prg.asm:5751-5780` (`sub_A32B`) y `prg.asm:8620-8655` (`loc_B7E4`) es que **todos los enemigos, monedas flotantes y popups de puntuación residen internamente en COORDENADAS DE PANTALLA**:

```
Coordenada_Enemigo_Pantalla(t) = Coordenada_Enemigo_Pantalla(t-1) + Velocidad_Propia + Delta_Camara
```

Donde:
- `Delta_Camara_X = byte_98` (lo) / `byte_99` (hi) = `-ScrollDeltaX`.
- `Delta_Camara_Y = byte_9A` (lo) / `byte_9B` (hi) = `-ScrollDeltaY`.

```assembly
; asm/src/prg.asm:8626-8654 (loc_B7E4 - Actualización de posición de enemigo)
	LDA a:byte_98,X          ; Delta de cámara X (-PlayerXVelLo)
	CLC
	ADC byte_0               ; Posición X del enemigo
	STA byte_0
	LDA a:byte_99,X          ; Delta de cámara X (-PlayerXVelHi)
	ADC byte_1
	STA byte_1
	...
	ADC (EnemyStructPointer),Y
	STA (EnemyStructPointer),Y
```

### Consecuencias para la Arquitectura C99:
1. Las entidades en pantalla no necesitan calcular costosas proyecciones `world_x - camera_x` cada frame.
2. Cada vez que la cámara avanza en una dirección, todas las entidades en pantalla se desplazan en sentido opuesto exactamente por el delta de la cámara (`delta_cam_x`, `delta_cam_y`).
3. Si la coordenada de pantalla de un enemigo sale de los márgenes visibles (`X < 0` o `X >= 256`, `Y < 0` o `Y >= 240`), el motor ejecuta el culling y reciclaje del slot en `EnemyStruct`.

---

## 7. Fórmulas Canónicas y Pseudocódigo C99 Determinista

A continuación se detalla el pseudocódigo C11 formal para la integración en `src/core/mjk_camera.c`:

```c
#include <stdint.h>
#include <stdbool.h>

#define RF_BOMB_ROOM     0x02
#define RF_AT_SCROLL_EDGE 0x08
#define RF_SINGLE_SCREEN 0x10
#define RF_SCROLL_STOP   0x20
#define RF_HORIZONTAL    0x40
#define RF_SECTION_START 0x80

#define JACK_CENTER_X    128
#define JACK_CENTER_Y    121

typedef struct {
    uint8_t flags;
    uint8_t up;
    uint8_t down;
    uint8_t right;
    uint8_t left;
} RoomAdjacency;

typedef struct {
    uint8_t room_flags;
    uint8_t current_room_id;
    uint8_t seam_room_id;
    
    uint16_t scroll_x_fixed; // 8.8 (byte_3C1:byte_3C2)
    uint16_t scroll_y_fixed; // 8.8 (byte_3C3:byte_3C4)
    
    int16_t delta_cam_x;     // byte_98:byte_99
    int16_t delta_cam_y;     // byte_9A:byte_9B
    
    uint8_t ppu_ctrl;        // PPUCTRL mirror (bit 1: nametable vertical)
    uint8_t ppu_mask;        // PPUMASK mirror (bits 1, 2: left 8px clipping)
} CameraSystem;

// ----------------------------------------------------------------------------
// Manejo de Deadzone de Jack al caminar/saltar
// ----------------------------------------------------------------------------
void Camera_UpdatePlayerKinematics(CameraSystem* cam, uint8_t* player_x, uint8_t* player_y, 
                                   int16_t vel_x, int16_t vel_y) {
    // Eje Y (Vertical)
    if ((cam->room_flags & (RF_SINGLE_SCREEN | RF_SCROLL_STOP)) != 0 ||
        (cam->room_flags & RF_AT_SCROLL_EDGE) != 0) {
        // En salas fijas, pasillos horizontales o en límites de torre: Jack se mueve libre
        *player_y += (vel_y >> 8);
        
        // Comprobar cruce de retorno al centro (Y = 121)
        if ((cam->room_flags & (RF_SINGLE_SCREEN | RF_SCROLL_STOP)) == 0) {
            bool crossing = (vel_y >= 0) ? (*player_y >= JACK_CENTER_Y) 
                                         : (*player_y < JACK_CENTER_Y);
            if (crossing) {
                cam->room_flags &= ~RF_AT_SCROLL_EDGE;
            }
        }
    } else {
        // En pozo vertical activo: Jack anclado a Y = 121
        *player_y = JACK_CENTER_Y;
    }
    
    // Eje X (Horizontal)
    if ((cam->room_flags & RF_SCROLL_STOP) == 0 ||
        (cam->room_flags & RF_SINGLE_SCREEN) != 0 ||
        (cam->room_flags & RF_AT_SCROLL_EDGE) != 0) {
        // En salas fijas, pozos verticales o límites horizontales: Jack se mueve libre
        *player_x += (vel_x >> 8);
        
        // Comprobar cruce de retorno al centro (X = 128)
        if ((cam->room_flags & RF_SCROLL_STOP) != 0 && (cam->room_flags & RF_SINGLE_SCREEN) == 0) {
            bool crossing = (vel_x >= 0) ? (*player_x >= JACK_CENTER_X) 
                                         : (*player_x < JACK_CENTER_X);
            if (crossing) {
                cam->room_flags &= ~RF_AT_SCROLL_EDGE;
            }
        }
    } else {
        // En pasillo horizontal activo: Jack anclado a X = 128
        *player_x = JACK_CENTER_X;
    }
}

// ----------------------------------------------------------------------------
// Actualización de Scroll de Cámara (MainSub_3)
// ----------------------------------------------------------------------------
void Camera_Tick(CameraSystem* cam, const RoomAdjacency* adj_table, int16_t vel_x, int16_t vel_y) {
    cam->delta_cam_x = 0;
    cam->delta_cam_y = 0;
    
    if (cam->room_flags & RF_SINGLE_SCREEN) {
        cam->ppu_mask |= 0x06;
        cam->scroll_x_fixed = 0;
        cam->scroll_y_fixed = 0;
        return;
    }
    
    if (cam->room_flags & RF_AT_SCROLL_EDGE) {
        return;
    }
    
    const RoomAdjacency* adj = &adj_table[cam->current_room_id];
    
    if (cam->room_flags & RF_SCROLL_STOP) {
        // --- SCROLL HORIZONTAL ---
        cam->ppu_mask &= ~0x06; // Oculta columna de 8px izquierda
        
        int8_t vel_hi = (int8_t)(vel_x >> 8);
        if (vel_hi == 0) return;
        
        // Delta de cámara para entidades
        cam->delta_cam_x = -vel_x;
        
        if (vel_hi < 0) {
            // Moviendo a la izquierda
            if (cam->room_flags & RF_SECTION_START) {
                uint8_t coarse_x = (cam->scroll_x_fixed >> 8) & 0xFF;
                if (coarse_x < 0xF9) {
                    cam->scroll_x_fixed = (0xF8 << 8);
                    cam->room_flags |= RF_AT_SCROLL_EDGE;
                    return;
                }
            }
            
            uint16_t old_x = cam->scroll_x_fixed;
            cam->scroll_x_fixed += vel_x;
            
            if (cam->scroll_x_fixed > old_x) { // Underflow por debajo de 0
                cam->room_flags &= ~RF_HORIZONTAL;
                if (adj->left != 0) {
                    cam->current_room_id = adj->left;
                    cam->seam_room_id = adj->left;
                } else {
                    cam->room_flags |= RF_SECTION_START;
                }
            }
        } else {
            // Moviendo a la derecha
            if (cam->room_flags & RF_HORIZONTAL) {
                cam->scroll_x_fixed = 0;
                cam->room_flags |= RF_AT_SCROLL_EDGE;
                return;
            }
            
            if (adj->right == 0) {
                cam->room_flags |= RF_HORIZONTAL;
                return;
            }
            
            uint16_t old_x = cam->scroll_x_fixed;
            cam->scroll_x_fixed += vel_x;
            
            if (cam->scroll_x_fixed < old_x) { // Overflow por encima de 255
                if (adj->right != 0) {
                    cam->current_room_id = adj->right;
                    // Actualiza flags con la nueva sala
                } else {
                    cam->room_flags |= RF_HORIZONTAL;
                }
            }
        }
    } else {
        // --- SCROLL VERTICAL ---
        cam->ppu_mask |= 0x06; // Borde completo visible
        
        int8_t vel_hi = (int8_t)(vel_y >> 8);
        if (vel_hi == 0 && (vel_y & 0xFF) == 0) return;
        
        cam->delta_cam_y = -vel_y;
        
        if (vel_hi < 0) {
            // Subiendo
            if (cam->room_flags & RF_SECTION_START) {
                cam->scroll_y_fixed = 0;
                cam->room_flags |= RF_AT_SCROLL_EDGE;
                return;
            }
            
            cam->room_flags &= ~RF_HORIZONTAL;
            cam->scroll_y_fixed += vel_y;
            uint8_t coarse_y = (cam->scroll_y_fixed >> 8) & 0xFF;
            
            if (coarse_y >= 240) {
                coarse_y += 15; // Módulo 240
                cam->scroll_y_fixed = (coarse_y << 8) | (cam->scroll_y_fixed & 0xFF);
            }
            
            if (adj->up != 0) {
                cam->current_room_id = adj->up;
                cam->ppu_ctrl ^= 0x02; // Alterna nametable vertical
            } else {
                cam->room_flags |= RF_SECTION_START;
                cam->scroll_y_fixed = 0;
            }
        } else {
            // Bajando
            if (cam->room_flags & RF_HORIZONTAL) {
                cam->scroll_y_fixed = 0;
                cam->room_flags |= RF_AT_SCROLL_EDGE;
                return;
            }
            
            cam->room_flags &= ~RF_SECTION_START;
            cam->scroll_y_fixed += vel_y;
            uint8_t coarse_y = (cam->scroll_y_fixed >> 8) & 0xFF;
            
            if (coarse_y >= 240) {
                coarse_y += 15; // Módulo 240
                cam->scroll_y_fixed = (coarse_y << 8) | (cam->scroll_y_fixed & 0xFF);
                cam->ppu_ctrl ^= 0x02;
                
                if (adj->down != 0) {
                    cam->current_room_id = adj->down;
                } else {
                    cam->room_flags |= RF_HORIZONTAL;
                    cam->scroll_y_fixed = 0;
                }
            }
        }
    }
}
```

---

## 8. Conclusiones y Guía de Implementación para C Port (`Fase 6`)

1. **Paridad Absoluta de Velocidad**:
   - Velocidad de avance de scroll: constante e idéntica a la velocidad horizontal de Jack (`1.5 px/frame` = `0x0180` en fixed-point `8.8`).
   - El scroll nunca es forzado ni automático: el avance de la cámara está gobernado al 100% por los comandos direccionales aplicados a Jack.
2. **Deadzones Canónicos**:
   - Pasillo horizontal: Jack se ancla a `X = 128` (`$80`).
   - Pozo vertical: Jack se ancla a `Y = 121` (`$79`).
   - Al tocar el límite (`RF_AtScrollEdge`), Jack se desancla y se desplaza hasta el borde de pantalla. Al retornar y cruzar el centro, se re-ancla inmediatamente.
3. **Manejo de Coordenadas de Entidades**:
   - Los enemigos y objetos flotantes se procesan en **coordenadas de pantalla**.
   - Cada frame de scroll aplica `delta_cam_x` y `delta_cam_y` directamente a la posición de las entidades activas, preservando la arquitectura original del 6502 sin necesidad de coordenadas mundiales pesadas.
4. **Emulación de Hardware NES en Frontend**:
   - En el renderizado por software / SDL:
     - En horizontal, enmascarar los 8 píxeles izquierdos (`X < 8`) si `PPUMask_ShowLeft8Pixels` está apagado.
     - En vertical, alternar el origen de nametable entre 0 y 240 píxeles según el bit 1 de `PPUCTRL`.
