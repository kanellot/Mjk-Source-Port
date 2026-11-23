# RES-035: Bloqueo de Movimiento de Jack al Retroceder hacia la Derecha en Pasillos Horizontales

- **ID de Tarea**: `TASK-F7-008`
- **Área Temática**: 02 - Física y Colisiones / 03 - Escenarios y Salas / Cinemática de Cámara
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-25
- **Autor**: asm_researcher (Nivel 3)
- **Fuentes de Verdad Primarias**:
  - `asm/src/prg.asm`: líneas 1530-1605 (`loc_89EF`, `loc_8A06`, `loc_8A10`, `loc_8A3E`, `loc_8A5B`), 4251-4314 (`sub_9A03`), 5906-6130 (`MainSub_3`, `loc_A454`, `loc_A47B`, `loc_A4DD`, `loc_A505`, `loc_A554`, `loc_A55E`, `loc_A57A`).
  - `asm/src/data.asm`: líneas 1704-1820 (`AdjacentRoomsTable`), 322-339 (`DoorEntryXYPositionTable`).
  - `asm/src/defs.asm`: líneas 215-222 (`RF_*`), 43-44 (`JP_Right`, `JP_Left`).
  - Implementación en C: `src/core/mjk_player.c` (líneas 247-309, 372-436, 483-506), `src/core/mjk_camera.c` (líneas 134-175, 240-305), `src/core/mjk_state_machine.c` (líneas 432-440, 514-535).

---

## 1. Resumen Ejecutivo del Bug y Causa Raíz

### 1.1 Síntoma Reportado
> *"Cuando Jack vuelve hacia atrás (se mueve hacia la derecha durante un rato) hay un momento en que se para y ya no se puede mover más, hay que investigar eso."*

### 1.2 Diagnóstico Forense y Reproducción Empírica
Al ejecutar una simulación determinista frame a frame con `MjkGameState`, se observó el siguiente fenómeno reproducible:
1. **Fase de Avance**: Jack camina hacia la derecha desde el spawn inicial de la Sala 1 ($X=40 \rightarrow 128$), se ancla en el centro ($X=128$), el scroll horizontal avanza a la derecha ($ScrollX: 0 \rightarrow 255$), transiciona a la Sala 2 (`loc_A55E`) y continúa.
2. **Fase de Retroceso (Izquierda)**: Jack gira y camina hacia la izquierda ($vx < 0$). La cámara retrocede en Sala 2 hasta que $ScrollX$ subdesborda por debajo de 0 ($new\_x < 0$). La cámara detecta la sala contigua izquierda (`left_room = 1`), envuelve $ScrollX$ a `0xFF..` (255 px) y cambia `current_room_id = 1`.
3. **El Disparador Fatal (Bug 1 - Inyección Ilegítima de `MJK_RF_SECTION_START`)**: En [`src/core/mjk_camera.c:268-270`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_camera.c#L268-L270), el port ejecuta:
   ```c
   if (prev_adj->left == 0 || (prev_adj->flags & 0x08)) {
       cam->room_flags |= MJK_RF_SECTION_START;
   }
   ```
   Como la Sala 1 es el inicio del pasillo (`prev_adj->left == 0` y `flags & 0x08`), el código inyecta el flag `MJK_RF_SECTION_START` ($0x80) **a pesar de que la cámara acaba de entrar por el extremo derecho de la sala ($ScrollX = 255$)**.
4. **El Bloqueo Definitivo (Bug 2 - Pared Fantasma en `mjk_player_is_tile_solid`)**:
   En [`src/core/mjk_player.c:258-269`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L258-L269):
   ```c
   if (target_x >= 256) {
       if (!(player->room_flags & 0x90)) {
           // ...
       } else {
           return true; // <-- PARED FANTASMA SÓLIDA ARTIFICIAL
       }
   }
   ```
   Con Jack anclado en pantalla en $X=128$ y $ScrollX=255$, el centro espacial continuo para las 9 sondas es:
   $$cx = jx + 8 + ScrollX = 128 + 8 + 255 = 391$$
   - Sonda 7 y 8 (Muro Derecho, $dx = +8$): $target\_x = 391 + 8 = 399 \ge 256$.
   - Sonda 5 y 6 (Muro Izquierdo, $dx = -8$): $target\_x = 391 - 8 = 383 \ge 256$.
   
   Al estar activo `MJK_RF_SECTION_START` ($0x80 \in 0x90$), `mjk_player_is_tile_solid` evalúa la rama `else` y retorna `true` (**sólido**) incondicionalmente para **ambas sondas laterales**.
5. **Colapso Cinematográfico Total**:
   Jack recibe simultáneamente `CF_RIGHT_WALL` y `CF_LEFT_WALL` (`collision_flags = 0x17`).
   En [`src/core/mjk_player.c:484-503`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L484-L503):
   Cualquier intento de caminar a la derecha o a la izquierda anula inmediatamente `subpixel_vx = 0` y `vel_x = 0`.
   En [`src/core/mjk_camera.c:245`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_camera.c#L245):
   Al ser `vx == 0`, `camera_update_kinematics` retorna inmediatamente sin mover la cámara.
   **Jack queda permanentemente atrapado en una jaula invisible de 0 píxeles de tolerancia en $ScrollX=255$, sin poder moverse nunca más.**

---

## 2. Comparativa Exhaustiva: Desmontaje 6502 (`asm/`) vs Source Port (`src/`)

### 2.1 Lógica de Avance y Detección de Borde en Desensamblado Canónico

En [`asm/src/prg.asm:5984-6018`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5984-L6018) (`loc_A498` - Retroceso a la izquierda con subdesborde):
```asm
loc_A498:
	JSR sub_A843
	LDA CurrentRoomID
	STA MaybeRoomIDCopyAgain
	LDA PlayerXVelLo
	CLC
	ADC byte_3C1
	STA byte_3C1
	LDA PlayerXVelHi
	ADC byte_3C2
	STA byte_3C2
	BCS loc_A4E3            ; Carry=1 -> No hubo subdesborde por debajo de 0
	LDAc RoomStatusFlags
	AND #~RF_Horizontal     ; Limpia fin de sección este
	STAc RoomStatusFlags
	LDY #0
	LDA (off_3D),Y          ; Lee flags de la sala activa (off_3D)
	AND #2
	BEQ loc_A4CC
	LDA (off_3D),Y
	AND #8
	BNE loc_A4D2            ; Solo si la sala actual YA ERA sección inicial ($08 | $02)

loc_A4CC:
	LDY #4
	LDA (off_3D),Y          ; Lee campo LEFT de AdjacentRoomsTable
	BNE loc_A4DD            ; Si hay sala izquierda -> salta a loc_A4DD

loc_A4D2:
	LDAc RoomStatusFlags
	ORA #RF_SectionStart    ; Activa RF_SectionStart ÚNICAMENTE si no hay sala izquierda
	STA a:RoomStatusFlags

loc_A4DA:
	JMP loc_A58C

loc_A4DD:
	STA CurrentRoomID       ; Cambia CurrentRoomID a la sala izquierda
	STA MaybeRoomIDCopyAgain
loc_A4E3:
```

#### Revelación Crítica del Código ASM:
1. Cuando la cámara subdesborda hacia la izquierda y transiciona a una sala contigua (`left_room`), el flujo salta a `loc_A4DD`.
2. **`loc_A4D2` (`ORA #RF_SectionStart`) SE OMITE POR COMPLETO.**
3. En el NES, `RF_SectionStart` **NO SE ACTIVA** al entrar a la Sala 1 desde la Sala 2 en $ScrollX=255$.
4. `RF_SectionStart` se activa en NES **única y exclusivamente cuando estando YA en la Sala 1, el scroll vuelve a subdesbordar por debajo de 0 y no existe sala izquierda a la que saltar (`off_3D[4] == 0`)**.
5. Y una vez activado `RF_SectionStart`, en [`asm/src/prg.asm:5971-5979`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5971-L5979) (`loc_A47B`):
   ```asm
   loc_A47B:
       LDA a:RoomStatusFlags
       BPL loc_A498
       LDA byte_3C2
       CMP #$F9
       BCS loc_A498
       LDA #$F8
       STA byte_3C2
       LDAc RoomStatusFlags
       ORA #RF_AtScrollEdge    ; Clampa scroll en $F8 y desancla a Jack
       STAc RoomStatusFlags
   ```
   El NES solo clampa la cámara y desancla a Jack cuando $ScrollX$ desciende por debajo de $\$F9$, clavando el scroll en $\$F8$ para que Jack camine libremente hacia el extremo izquierdo de la pantalla visible.

### 2.2 Sondas de Colisión Espacial en `sub_9A03` (NES vs Port)

En [`asm/src/prg.asm:4260-4289`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4260-L4289) (`sub_9A03`):
```asm
loc_9A0F:
	CLC
	ADC byte_3C2          ; Accum = ProbeDx + ScrollX
	TAY
	TXA
	ADC #0
	STA byte_7           ; byte_7 = carry acumulado
	TYA
	CLC
	LDY #5               ; PlayerXPosHi (coordenada en pantalla de Jack)
	ADC (off_78),Y       ; byte_0 = ProbeDx + ScrollX + ScreenX
	STA byte_0           ; byte_0 truncado a 8 bits (0..255)
	LDA byte_7
	ADC #0
	BEQ loc_9A39         ; Si no superó 256 -> carga CurrentRoomID
	LDA #$10             ; Si superó 256:
	BIT a:RoomStatusFlags ; Si es SingleScreen ($10) -> salta a loc_9A39
	BNE loc_9A39
	LDX a:RoomStatusFlags
	BMI loc_9A39         ; Si es SectionStart ($80) -> salta a loc_9A39
	LDY #3
	LDA (off_3D),Y       ; Lee sala DERECHA (AdjacentRoomsTable[3])
	BNE loc_9A3C         ; Si existe sala derecha -> carga sus metatiles

loc_9A39:
	LDA CurrentRoomID    ; <-- CARGA CurrentRoomID (¡WRAP MÓDULO 256!)

loc_9A3C:
	STA a:CurrentRoomIDBackup2
	JSR LoadRoomStuff    ; Desempaqueta metatiles de la sala seleccionada
```

#### Análisis de la Discrepancia Fatal:
- **Comportamiento NES**: Si la sonda supera el píxel 256 y la sala no permite cruzar a la vecina derecha (o no tiene vecina derecha), el código **salta a `loc_9A39` y consulta los metatiles de `CurrentRoomID`**. La posición en tile se calcula en línea 4301 con:
  ```asm
  LDA byte_0
  LSR A
  LSR A
  LSR A
  LSR A
  STA byte_8           ; Col = (byte_0 & 0xFF) >> 4
  ```
  `byte_0` es un registro de 8 bits sin signo ($0..255$). El NES realiza un wrap circular natural módulo 256. **Bajo ninguna circunstancia el NES inyecta una pared sólida artificial.**
- **Comportamiento Erróneo del Port C**: En [`src/core/mjk_player.c:268`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L268), el programador asumió erróneamente que "si no puede saltar a la sala contigua, debe ser una pared perimetral insuperable" y colocó `return true;`. Esta pared fantasma es la que encierra a Jack cuando $target\_x \ge 256$.

### 2.3 Reseteo Anómalo de Cámara a Cero en Borde Derecho

En [`src/core/mjk_camera.c:298-303`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_camera.c#L298-L303):
```c
} else {
    // Borde terminal absoluto del pasillo: bloquear cámara y desanclar a Jack (loc_A554)
    cam->room_flags |= (MJK_RF_HORIZONTAL | MJK_RF_AT_SCROLL_EDGE);
    cam->at_scroll_edge = true;
    cam->scroll_x_fixed = 0; // <-- ERROR: RESETEA EL SCROLL A CERO
}
```
En el NES original ([`asm/src/prg.asm:6085-6090`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L6085-L6090), `loc_A554`):
```asm
loc_A554:
	LDAc RoomStatusFlags
	ORA #RF_Horizontal
	STAc RoomStatusFlags
	BNE loc_A58C
```
El NES **no altera `byte_3C2`** al alcanzar el borde derecho; simplemente marca `RF_Horizontal` ($0x40) y en el frame siguiente clampa a 0 si procede, pero jamás resetea $ScrollX$ bruscamente en pleno paso de subpíxel.

---

## 3. Topología de Salas y Direccionalidad del Scroll en Mighty Bomb Jack

### 3.1 Inventario de Salas Horizontales en `AdjacentRoomsTable`
Mediante análisis del desensamblado [`asm/src/data.asm:1704-1820`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L1704-L1820):

| Sala (Hex) | Sala (Dec) | Flags | UP | DOWN | RIGHT | LEFT | Tipo de Sección y Conectividad |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| `0x01` | 1 | `$0E` | `0` | `0` | `2` | `0` | **Inicio de Pasillo 1 (Borde Izquierdo absoluto)** |
| `0x02` | 2 | `$04` | `0` | `0` | `3` | `1` | Pasillo continuo intermedio |
| `0x03` | 3 | `$04` | `0` | `0` | `4` | `2` | Pasillo continuo intermedio |
| `0x04` | 4 | `$04` | `0` | `0` | `5` | `3` | Pasillo continuo intermedio |
| `0x05` | 5 | `$04` | `0` | `0` | `6` | `4` | Pasillo continuo intermedio |
| `0x06` | 6 | `$06` | `0` | `0` | `$90` | `5` | **Fin de Pasillo 1 (Borde Derecho hacia Sala Real $0x90)** |
| `0x0C` | 12 | `$0E` | `0` | `0` | `$0D` | `$91` | Inicio Pasillo 2 (proviene de Sala Real $0x91) |
| `0x0F` | 15 | `$06` | `0` | `$A9` | `$92` | `$0E` | Fin Pasillo 2 (hacia Sala Real $0x92) |
| `0x12` | 18 | `$0E` | `0` | `0` | `$13` | `$11` | Inicio Pasillo 3 |
| `0x17` | 23 | `$06` | `0` | `0` | `$27` | `$16` | Fin Pasillo 3 |

**Conclusión Topológica Canónica**:
- En Mighty Bomb Jack, **todos los pasillos horizontales avanzan de izquierda a derecha** (`RIGHT = next_room`, `LEFT = prev_room`).
- El jugador avanza hacia la derecha. "Retroceder" o "volver hacia atrás" en la terminología del usuario corresponde a girar a la izquierda, y luego intentar reanudar la marcha hacia la derecha ("se mueve hacia la derecha durante un rato").
- El juego original **permite el retroceso bidireccional fluido** dentro del pasillo; no existe scroll forzado unidireccional irrecuperable.

---

## 4. Comparativa de Sistemas de Coordenadas

```
+-----------------------------------------------------------------------------------+
| MODELO CANÓNICO NES (Hardware PPU 2C02 + 6502)                                    |
+-----------------------------------------------------------------------------------+
| 1. JackScreenX: Coordenada de sprite en OAM ($0305 / $0203), rango 0..255.       |
| 2. PPU ScrollX: Registro $2005 / byte_3C2, rango 0..255.                          |
| 3. En Pasillo Horizontal:                                                         |
|    - Si RF_AtScrollEdge == 0: JackScreenX CLAVADO en 128 ($80). ScrollX avanza.   |
|    - Si RF_AtScrollEdge == 1: ScrollX CLAVADO en $00 o $F8. JackScreenX varía.     |
| 4. Sonda de Metatiles (sub_9A03):                                                 |
|    - ProbeX_8bit = (JackScreenX + ScrollX + ProbeDx) & 0xFF                       |
|    - Overflow bit (Carry): Determina si se lee de CurrentRoom o AdjacentRoom.     |
|    - Metatile siempre se lee de la matriz 16x15; NUNCA genera sólidos sintéticos. |
+-----------------------------------------------------------------------------------+

+-----------------------------------------------------------------------------------+
| MODELO ACTUAL DEL PORT EN C (Discrepancias Identificadas)                         |
+-----------------------------------------------------------------------------------+
| 1. Jack_UpdateHorizontalVelocity modifica jack->pos_x ANTES de chequear cámara.   |
| 2. mjk_player_update_collision calcula cx = jx + 8 + scroll_x.                    |
| 3. Si target_x >= 256 y RF_SECTION_START está encendido:                          |
|    - Port devuelve "true" (Pared sólida), bloqueando a Jack en seco.              |
| 4. camera_update_kinematics asignó prematuramente RF_SECTION_START al entrar     |
|    a Room 1 desde Room 2 en scroll_x = 255.                                       |
+-----------------------------------------------------------------------------------+
```

---

## 5. Especificación Técnica de la Solución para C

Para solucionar definitivamente el congelamiento de Jack al retroceder y avanzar hacia la derecha, se deben aplicar las siguientes correcciones arquitectónicas:

### Corrección 1: Supresión de la Pared Fantasma en `mjk_player_is_tile_solid`
**Archivo**: [`src/core/mjk_player.c:258-283`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L258-L283)
- Cuando `target_x >= 256`:
  Si no se puede cruzar a la sala derecha (porque `player->room_flags & 0x90` está activo, o `adj->right == 0`), **no retornar `true` incondicionalmente**. En su lugar, emular `loc_9A39` de NES:
  ```c
  if (target_x >= 256) {
      if (!(player->room_flags & 0x90) && adj->right != 0) {
          target_room = adj->right;
          target_x -= 256;
      } else {
          // Paridad loc_9A39: Wrap modular dentro de la sala actual sin pared sintética
          target_x &= 0xFF;
      }
  } else if (target_x < 0) {
      if (!(player->room_flags & 0x10) && adj->left != 0) {
          target_room = adj->left;
          target_x += 256;
      } else {
          target_x &= 0xFF;
      }
  }
  ```

### Corrección 2: Asignación Estricta de `MJK_RF_SECTION_START` en `camera_update_kinematics`
**Archivo**: [`src/core/mjk_camera.c:260-277`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_camera.c#L260-L277)
- Al retroceder hacia la izquierda (`vx < 0`) y transicionar de sala (`new_x < 0`):
  ```c
  uint8_t left_room = adj ? adj->left : 0;
  if (left_room != 0) {
      cam->scroll_x_fixed = (uint16_t)(new_x + 0x10000);
      cam->current_room_id = left_room;
      const MjkAdjacentRoom* prev_adj = &adj_table[left_room];
      cam->seam_room_id = prev_adj->left ? prev_adj->left : left_room;
      // ELIMINADO: No inyectar MJK_RF_SECTION_START en scroll_x = 255
      cam->room_flags &= ~MJK_RF_SECTION_START;
  } else {
      // Borde inicial absoluto: solo aquí se detiene el scroll y se desancla a Jack
      cam->room_flags |= (MJK_RF_SECTION_START | MJK_RF_AT_SCROLL_EDGE);
      cam->at_scroll_edge = true;
      cam->scroll_x_fixed = 0;
  }
  ```

### Corrección 3: Limpieza Limpia de Flags en Re-anclaje Bidireccional
**Archivo**: [`src/core/mjk_camera.c:143-169`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_camera.c#L143-L169)
- Al re-anclar a Jack cuando camina hacia la derecha (`vx > 0 && jx >= MJK_JACK_CENTER_X`):
  Limpiar explícitamente tanto `MJK_RF_AT_SCROLL_EDGE` como `MJK_RF_SECTION_START`, garantizando que la cámara asuma el control inmediato de la cinemática de scroll:
  ```c
  if (vx > 0 && jx >= MJK_JACK_CENTER_X) {
      cam->room_flags &= ~(MJK_RF_AT_SCROLL_EDGE | MJK_RF_SECTION_START);
      cam->at_scroll_edge = false;
      jack->pos_x = FIXED_FROM_INT(MJK_JACK_CENTER_X);
  }
  ```

### Corrección 4: Eliminación del Reseteo Espurio `cam->scroll_x_fixed = 0` en Fin de Sección
**Archivo**: [`src/core/mjk_camera.c:298-304`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_camera.c#L298-L304)
- Al alcanzar el fin de sección derecha sin sala contigua pasante, preservar el valor de scroll de final de nametable en lugar de sobreescribirlo con 0.

---

## 6. Verificación y Plan de Certificación de la Tarea

1. **Test Determinista de Regresión**:
   - Crear / actualizar test unitario para simular la secuencia de eventos reportada:
     - Caminar derecha de Sala 1 a Sala 2 (250 frames).
     - Retroceder izquierda de Sala 2 a Sala 1 cruzando frontera (200 frames).
     - Volver a caminar a la derecha dentro de Sala 1 hasta cruzar a Sala 2 nuevamente (300 frames).
     - **Criterio de Aceptación**: `jack.subpixel_vx == 384` sostenido, $ScrollX$ avanza de 0 a 255 sin trabarse, ningún frame con `CF_RIGHT_WALL` espurio en espacio abierto, transición limpia `1 -> 2 -> 1 -> 2`.
2. **Control de Invariantes**:
   - Compilación limpia con MSVC x64 (`build.bat test`).
   - Todos los 17 tests unitarios existentes continúan pasando al 100%.
