# RES-029: Diagnóstico Ground-Truth y Eliminación de "Calaveras Amarillas" (Puertas Fallidas) en el Mapa (`TASK-F7-002`)

- **ID de Investigación**: `RES-029`
- **Tarea Vinculada**: `TASK-F7-002` (Eliminación de sprites/tiles glitcheados "Calaveras Amarillas" en el mapa)
- **Área Temática**: 03 - Escenarios, Metatiles y Geometría de Salas / 05 - Sistema de Renderizado y PPU
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-25
- **Agente Autor**: `asm_researcher` (Nivel 3)
- **Fuentes de Verdad Analizadas**:
  - Desensamblado 6502 y CHR-ROM: [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm), [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm), [`asm/src/roomdata-us.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/roomdata-us.asm), [`asm/src/mbj-us.chr`](file:///c:/Users/kanel/Documents/mjk/asm/src/mbj-us.chr).
  - Código C del Port: [`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c), [`src/core/mjk_state_machine.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c), [`src/core/mjk_room_decoder.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c), [`src/core/mjk_room_data.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_data.c), [`include/mjk_room_data.h`](file:///c:/Users/kanel/Documents/mjk/include/mjk_room_data.h), [`include/mjk_room_decoder.h`](file:///c:/Users/kanel/Documents/mjk/include/mjk_room_decoder.h).
  - Suite de Pruebas: [`tests/test_room_decoder.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_room_decoder.c), [`tests/test_state_machine.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_state_machine.c), [`tests/test_bomb_chain.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_bomb_chain.c), [`tests/test_room_objects.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_room_objects.c).

---

## 1. Resumen Ejecutivo (`[CONFIRMADA]`)

La hipótesis del usuario es **100% exacta** (`[CONFIRMADA]`): las "calaveras amarillas/naranjas" que aparecen flotando por el mapa (en la coordenada de metatile `(X=14, Y=8)`, píxeles `(224, 128)`, justo debajo de la plataforma derecha en la Sala 1 y repetidas en cada sala del pasillo) **son basura visual de una implementación temprana y fallida de puertas de salida**.

El defecto se produce por la combinación de tres errores en [`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c) y [`src/core/mjk_state_machine.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c):
1. **[`src/core/mjk_game.c:621-635`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L621-L635)** (`RenderInGameScreen`, Paso `3c. Colocar Puerta de salida`): Sobrescribe el nametable en `(game->door_tile_x, game->door_tile_y)` con los tiles CHR crudos de 8×8 **`0x94, 0x95, 0x96, 0x97`** (cuando está cerrada) o **`0x98, 0x99, 0x9A, 0x9B`** (cuando está abierta), forzando la **Subpaleta de Fondo 1** (`subpalettes = 1`).
2. En [`asm/src/mbj-us.chr`](file:///c:/Users/kanel/Documents/mjk/asm/src/mbj-us.chr) (Banco 1, Pattern Table 1, offset `4096`), los cuatro tiles 8×8 `{ 0x94, 0x95, 0x96, 0x97 }` **NO son una puerta**, sino los 4 cuadrantes del **`Metatile 0x61`** ([`asm/src/data.asm:1645`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L1645), [`src/core/mjk_room_data.c:761`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_data.c#L761)), que es un **relieve decorativo de calavera** empleado únicamente con Subpaleta 2 (piedra gris) en muros subterráneos (`Chunk 0xA8` y `Chunk 0xA9`). Al dibujarlo con **Subpaleta 1** (`BackgroundPaletteSets[0][0..3] = { $0A, $16, $26, $38 }` = Verde oscuro, Rojo, Naranja, Amarillo brillante), se renderiza literalmente como una **calavera amarilla/naranja**.
3. **[`src/core/mjk_state_machine.c:301-321`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L301-L321)** (`State_2_LoadRoom`), **[`src/core/mjk_state_machine.c:544-559`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L544-L559)** (`State_5_MainLoop`) y **[`src/core/mjk_game.c:981-984, 1044-1047`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L981-L984)** fijan artificialmente `game->door_tile_x = 14; game->door_tile_y = 8;` en todas las salas con adyacencia derecha (`adj->right != 0`), plantando esa calavera amarilla en mitad de la pared de ladrillos verdes (`Metatile 0x2F`) en las Salas 1, 2, 3, 4, 5 y 6.

---

## 2. Demostración Bit a Bit sobre CHR-ROM y Tablas 6502 (`[CONFIRMADA]`)

### 2.1 Confusión entre IDs de Metatile (`$95..$98`) e IDs de Tile 8×8 CHR (`0x94..0x9B`)

En [`asm/src/prg.asm:3823-3839`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3823-L3839) (`DoorPositionTileTable`), las entradas definen los índices de **Metatile de 16×16** para las puertas:
- Puertas horizontales (tipos `0..7`): Metatiles base **`$87`** (`{ $38, $39, $38, $39 }`) y **`$95`** (`{ $38, $39, $38, $39 }`).
- Puertas verticales laterales (tipos `8..15`): Metatiles base **`$8B`** (`{ $38, $38, $39, $39 }`) y **`$97`** (`{ $38, $38, $39, $39 }`).

Todos los metatiles reales de puerta (`$87..$8E` y `$95..$98` en [`asm/src/data.asm:1685-1702`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L1685-L1702) y [`src/core/mjk_room_data.c:799-817`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_data.c#L799-L817)) están construidos **exclusivamente con los tiles 8×8 CHR `$38`, `$39` (barrotes de piedra) y `$24` (hueco negro vacío)**.

Sin embargo, en [`src/core/mjk_game.c:621-635`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L621-L635) se escribieron los números `0x94..0x97` y `0x98..0x9B` directamente en `nametable[][]` como si fueran tiles 8×8 de CHR-ROM:

```c
// src/core/mjk_game.c:621-635 (CÓDIGO ERRÓNEO ACTUAL)
    // 3c. Colocar Puerta de salida (TASK-F6-021)
    if (game->door_tile_x > 0 && game->door_tile_y > 0) {
        int dy = game->door_tile_y;
        int dx = game->door_tile_x;
        if (dy < 15 && dx < 16) {
            uint8_t p_tl = game->door_is_open ? 0x98 : 0x94;
            uint8_t p_tr = game->door_is_open ? 0x99 : 0x95;
            uint8_t p_bl = game->door_is_open ? 0x9A : 0x96;
            uint8_t p_br = game->door_is_open ? 0x9B : 0x97;
            nametable[dy * 2][dx * 2]         = p_tl; subpalettes[dy * 2][dx * 2]         = 1;
            nametable[dy * 2][dx * 2 + 1]     = p_tr; subpalettes[dy * 2][dx * 2 + 1]     = 1;
            nametable[dy * 2 + 1][dx * 2]     = p_bl; subpalettes[dy * 2 + 1][dx * 2]     = 1;
            nametable[dy * 2 + 1][dx * 2 + 1] = p_br; subpalettes[dy * 2 + 1][dx * 2 + 1] = 1;
        }
    }
```

### 2.2 Decodificación 2bpp de los Tiles `0x94, 0x95, 0x96, 0x97` en `asm/src/mbj-us.chr`

Al decodificar los 16×16 píxeles formados por `{ TL=0x94, TR=0x95, BL=0x96, BR=0x97 }` en Pattern Table 1 del Banco 1 (`offset = 8192 + 4096 + tile * 16`), se obtiene exactamente el relieve de calavera de `Metatile 0x61`:

```text
1.......1.......
1.......1.......
1.....1.1.......
1111111111111111  <-- Borde superior de ladrillo
...1..2...321...
...121....112...
...1332.21321...
.23123..2111...3
.131.23.21332122
21222121.333321.  <-- Cráneo (color 3 = $38 Amarillo brillante)
1233221131131121  <-- Cuencas oculares
2323321131121121  <-- Cuencas + fosa nasal
2311221123333211  <-- Maxilar superior
2311211212121112  <-- Dientes
1233113213131332
1111121111111221
```

Y cuando `game->door_is_open == true`, los tiles `{ 0x98, 0x99, 0x9A, 0x9B }` mezclan la mitad derecha de una columna rota (`0x98, 0x9A` de `Metatile 0x2B`) con la tapa superior de un cofre naranja (`0x99, 0x9B` de `Metatile 0x4A`).

---

## 3. Auditoría Completa de Todos los Puntos Afectados en `src/core/` (`[CONFIRMADA]`)

| # | Archivo y Líneas | Problema Detectado | Ground-Truth 6502 (`asm/src/prg.asm` / `roomdata-us.asm`) |
| :---: | :--- | :--- | :--- |
| **1** | [`src/core/mjk_game.c:621-635`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L621-L635) | Bloque `3c. Colocar Puerta de salida`: estampa la calavera amarilla (`0x94..0x97` / `0x98..0x9B`, subpaleta 1) en `(door_tile_x, door_tile_y)`. | En `sub_9696` / `sub_9592` ([`prg.asm:3635-3663`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3635-L3663)), las puertas solo existen donde el bytecode `RoomData_XX` contiene un comando `$80..$8F`, y se blittean como **2 metatiles contiguos** de `DoorPositionTileTable` (`$87..$8E` o `$95..$98`). |
| **2** | [`src/core/mjk_state_machine.c:301-321`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L301-L321) (`State_2_LoadRoom`) y [`mjk_state_machine.c:544-559`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L544-L559) (`State_5_MainLoop`) | Inventa coordenadas de puerta (`(14,8)`, `(1,8)`, `(14,12)`, `(14,2)`) basándose en `g_adjacent_rooms[room_id]` y escribe `METATILE_DOOR_CLOSED` (`0x04`) en `game->room_metatiles`. | En el Corredor 1-1 (`RoomData_01..RoomData_06`, [`src/core/mjk_room_data.c:1081-1086`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_data.c#L1081-L1086)):<br>- **Sala 1** solo tiene la puerta de entrada izquierda cerrada `$8F, $00` en `(Y=11..12, X=0)`. En `(14,8)` hay pared normal `$2F`.<br>- **Salas 2, 3, 4 y 5** no tienen ninguna puerta (son pasillo continuo).<br>- **Sala 6** tiene la puerta de salida derecha abierta `$8A, $01` en `(Y=2..3, X=15)` (`DoorPositionTileTable[10]`: `$2F, $3F`, estado inicial `$52` con bit 6 activo). |
| **3** | [`src/core/mjk_state_machine.c:469-473`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L469-L473) (`State_5_MainLoop`, Paso 7) | La rama `else { if (game->bomb_count > 0 && game->total_bombs_collected >= game->bomb_count) should_open = true; }` abre puertas falsas en cualquier sala normal al recoger sus bombas y congela el juego en `GAME_STATE_B_ANIMATE_DOOR_OPEN`. | En `MaybeHandleDoorOpening` ([`prg.asm:6696-6740`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L6696-L6740)), si una sala normal no tiene condición de puerta en su cabecera (`UUUUU == 0`, `door_condition_active == false`), `_ClearDoorFlagAndExit` desactiva la evaluación. Las puertas de salida normales al final de los pasillos ya vienen abiertas en ROM (`arg = $01` -> `$52`). |
| **4** | [`src/core/mjk_game.c:981-984`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L981-L984) (`MjkGame_Init`) y [`mjk_game.c:1044-1047`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L1044-L1047) (`MjkGame_StartNewGame`) | Inicializa `game->door_tile_x = 14; game->door_tile_y = 8;`. | Debe inicializarse a `0` (`game->door_tile_x = 0; game->door_tile_y = 0; game->door_is_open = false;`) ya que la Sala 1 no tiene puerta de salida. |
| **5** | [`src/core/mjk_game.c:586`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L586) y [`mjk_game.c:681`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L681) (`RenderInGameScreen`, Paso 3b Cofres) | *(Hallazgo adicional visible en la captura del usuario)*: Fija `uint8_t sub = 2;` para todos los cofres cerrados, haciendo que los Cofres Rojos (`0x28`) se vean grises en vez de rojos. | En `TileAttributeTable` (`MjkRoomData_GetMetatileSubpalette`), el cofre rojo cerrado `0x28` usa **Subpaleta 3** (`sub = 3`, rojo `$16` y blanco `$30`), y el cofre naranja cerrado `0x4A` usa **Subpaleta 0** (`sub = 0`, naranja `$26` y blanco `$30`). |

---

## 4. Especificación Quirúrgica de Corrección para `c_implementer` (`TASK-F7-002`)

### 4.1 Corrección en [`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c)

1. **Eliminar el bloque `3c. Colocar Puerta de salida` (`líneas 621-635`) en `RenderInGameScreen` y reemplazarlo por el blitteo canónico de puertas reales de `g_room_data_table[room_id]` (y `adj->right` en la costura horizontal)**:
   - Función auxiliar estática en `src/core/mjk_game.c` para superponer las puertas reales (`$80..$8F`) de cualquier `room_id` sobre `nametable[30][32]` y `subpalettes[30][32]` siguiendo exactamente `sub_9592` ([`asm/src/prg.asm:3635-3663`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3635-L3663)) y `MjkRoom_BlitObjects` ([`src/core/mjk_room_decoder.c:838-860`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c#L838-L860)):

```c
static void BlitCanonicalRoomDoors(const MjkGameState* game, uint8_t room_id,
                                   uint8_t nametable[30][32], uint8_t subpalettes[30][32]) {
    if (!game || room_id == 0 || room_id >= MJK_TOTAL_ROOMS_COUNT) return;
    const uint8_t* rdata = g_room_data_table[room_id];
    if (!rdata) return;

    int slot_idx = MjkRoom_FindSlot(&game->room_data_ram, room_id);
    const uint8_t* ram_raw = NULL;
    if (slot_idx >= 0) {
        ram_raw = (slot_idx < MJK_ROOMDATA_SECTION_SLOTS)
                ? game->room_data_ram.slots[slot_idx].raw
                : game->room_data_ram.scratch.raw;
    }

    size_t rom_pos = 0;
    uint8_t byte_AC = 0;
    uint8_t ram_obj_idx = 1;

    while (rom_pos < 512) {
        uint8_t b = rdata[rom_pos];
        if (b == 0xFF) break;
        if ((b & 0xF0) == 0xE0) {
            rom_pos += 3;
            continue;
        }
        if (byte_AC != 0) {
            byte_AC--;
            rom_pos++;
            ram_obj_idx++;
            continue;
        }

        rom_pos++;
        uint8_t hi = (uint8_t)(b >> 4);
        uint8_t lo = (uint8_t)(b & 0x0F);
        if (hi < 0x0B) ram_obj_idx++;

        if (hi == 0) {
            byte_AC = (lo == 0) ? 0 : (uint8_t)(lo - 1);
            if (room_id >= 0x90 && room_id < 0xA0) byte_AC = 0x18;
            rom_pos++;
        } else if (hi == 8) {
            uint8_t door_arg = rdata[rom_pos++];
            uint8_t state = g_door_state_tile_table[door_arg & 7];
            if (ram_raw && ram_obj_idx < MJK_ROOMDATA_SLOT_SIZE && ram_raw[ram_obj_idx] != 0 && ram_raw[ram_obj_idx] != 0xFF) {
                state = ram_raw[ram_obj_idx];
            }
            if (room_id == game->current_room && game->door_is_open &&
                lo == (game->entry_door_type & 0x0F)) {
                state |= MJK_OBJ_STATE_DOOR_OPEN;
            }

            const MjkDoorPositionTile* dpos = &g_door_position_tile_table[lo];
            uint8_t base_tile = dpos->closed_tile;
            uint8_t y_offset = 0;
            if (state & MJK_OBJ_STATE_DOOR_OPEN) {
                y_offset = 2;
            } else if (state & 0x80) {
                base_tile = dpos->open_tile;
            }
            uint8_t m_ids[2] = { (uint8_t)(base_tile + y_offset), (uint8_t)(base_tile + y_offset + 1) };
            uint8_t coords[2] = { dpos->y_tile, dpos->x_tile };

            for (int k = 0; k < 2; ++k) {
                uint8_t my = (uint8_t)(coords[k] >> 4);
                uint8_t mx = (uint8_t)(coords[k] & 0x0F);
                uint8_t m_id = m_ids[k];
                if (my < 15 && mx < 16 && m_id < MJK_METATILES_COUNT) {
                    MjkMetatileDef def = g_metatile_definitions[m_id];
                    uint8_t sub = MjkRoomData_GetMetatileSubpalette(m_id);
                    nametable[my * 2][mx * 2]         = def.tl; subpalettes[my * 2][mx * 2]         = sub;
                    nametable[my * 2][mx * 2 + 1]     = def.tr; subpalettes[my * 2][mx * 2 + 1]     = sub;
                    nametable[my * 2 + 1][mx * 2]     = def.bl; subpalettes[my * 2 + 1][mx * 2]     = sub;
                    nametable[my * 2 + 1][mx * 2 + 1] = def.br; subpalettes[my * 2 + 1][mx * 2 + 1] = sub;
                }
            }
        } else {
            rom_pos++;
            if (lo == 6 && (hi == 1 || hi == 2 || hi == 3 || hi == 4 || hi == 9 || hi == 0x0A)) {
                rom_pos++;
            }
        }
    }
}
```

   - Llamar a `BlitCanonicalRoomDoors(game, room_id, nametable, subpalettes);` en el Paso 3c de `RenderInGameScreen` (`src/core/mjk_game.c:621`) y a `BlitCanonicalRoomDoors(game, adj->right, right_nametable, right_subpalettes);` en la costura derecha (`src/core/mjk_game.c:714`).
2. **Corregir la subpaleta de los cofres cerrados en [`src/core/mjk_game.c:586-593`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L586-L593) y [`src/core/mjk_game.c:681-688`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L681-L688)**:
   - En lugar de `uint8_t sub = 2;`, usar `sub = MjkRoomData_GetMetatileSubpalette(0x4A);` (`0`) para cofres naranjas cerrados y `sub = MjkRoomData_GetMetatileSubpalette(0x28);` (`3`) para cofres rojos cerrados.
3. **Limpiar la inicialización de puertas en [`src/core/mjk_game.c:981-984`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L981-L984) (`MjkGame_Init`) y [`src/core/mjk_game.c:1044-1047`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L1044-L1047) (`MjkGame_StartNewGame`)**:
   - Cambiar `game->door_tile_x = 14; game->door_tile_y = 8;` por `game->door_tile_x = 0; game->door_tile_y = 0; game->door_is_open = false;`.

---

### 4.2 Corrección en [`src/core/mjk_state_machine.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c)

1. **Reemplazar la colocación inventada de puertas en `State_2_LoadRoom` (`líneas 301-321`) y en la transición de pasillo de `State_5_MainLoop` (`líneas 544-559`) por la lectura real desde `g_room_data_table[room_id]`**:
   - Crear una función auxiliar `ConfigureRoomDoorFromBytecode(MjkGameState* game, uint8_t room_id)` que recorra `g_room_data_table[room_id]` buscando comandos `$80..$8F`:
     - Si la sala tiene una puerta abierta inicialmente (`(g_door_state_tile_table[door_arg & 7] & MJK_OBJ_STATE_DOOR_OPEN) != 0`, como `$8A, $01` en la Sala 6) o una puerta de salida cerrable en sala con condición / Bomb Room, asigna `game->door_tile_x = dpos->y_tile & 0x0F`, `game->door_tile_y = dpos->y_tile >> 4`, `game->door_is_open = (state & MJK_OBJ_STATE_DOOR_OPEN) != 0`, y actualiza `game->entry_door_type = door_idx` cuando corresponda a la puerta de salida.
     - Si la sala no tiene puerta de salida (como la Sala 1, que solo tiene la puerta de entrada cerrada `$8F, $00` en `X=0` sin condición de apertura, o las Salas 2..5 que no tienen ningún comando `$8x`), establece `game->door_tile_x = 0; game->door_tile_y = 0; game->door_is_open = false;` y **NO** escribe `METATILE_DOOR_CLOSED` (`0x04`) sobre `game->room_metatiles`.
2. **Eliminar la rama `else` espuria en `State_5_MainLoop` (`líneas 469-473`)**:
   - En [`src/core/mjk_state_machine.c:460-474`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L460-L474), cambiar:
     ```c
     if (!game->door_is_open) {
         bool should_open = false;
         if (game->is_bomb_room) {
             if (game->total_bombs_collected >= 23) {
                 should_open = true;
             }
         } else if (game->door_condition_active) {
             if (MjkRoom_CheckDoorCondition(game->door_condition_id, &game->jack, game)) {
                 should_open = true;
             }
         }
     ```
     eliminando el bloque `else { if (game->bomb_count > 0 && game->total_bombs_collected >= game->bomb_count) should_open = true; }`.
3. **Proteger la comprobación de colisión con la puerta abierta en `State_5_MainLoop` (`líneas 482-490`)**:
   - Comprobar `else if (game->door_tile_x > 0 || game->door_tile_y > 0)` y calcular la posición en pantalla de la puerta restando el scroll (`int dx = game->door_tile_x * 16 - game->camera_x; int dy = game->door_tile_y * 16 - game->camera_y;`), lo cual mantiene 100% de compatibilidad con [`tests/test_state_machine.c:120-137`](file:///c:/Users/kanel/Documents/mjk/tests/test_state_machine.c#L120-L137) (donde `camera_x == 0`, `door_tile_x == 14`, `door_tile_y == 8`, `door_is_open == true`) y a la vez permite cruzar puertas reales al final de pasillos con scroll como la Sala 6 (`(X=15, Y=2..3)`).
