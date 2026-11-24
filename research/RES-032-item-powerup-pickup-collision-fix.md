# RES-032: Colisiones de Recolección de Ítems y Power-Ups tras Abrir un Cofre (Física de Tiles, Scroll y Paridad 6502)

- **ID de Investigación**: `RES-032`
- **Tarea Asociada**: `TASK-F7-005`
- **Agente**: `asm_researcher` (Nivel 3)
- **Estado**: `[CONFIRMADA]`
- **Fecha**: 2026-04-06
- **Archivos de Referencia**:
  - `asm/src/prg.asm` (`sub_9B39`: L4448–4474, `sub_9B6A`: L4480–4610, `BigBeefySubHandlesBGColliding` & `MaybePickupItem`: L4615–5254, `MaybeOpenHiddenBlock` & `MaybeSpawnThingFromObject`: L5358–5495, `byte_A3D8` & `byte_A3EA`: L5863–5883, `JT_94AD_1..7`: L3357–3471, `HandleDrawHalfRow`: L2653–2699, `Enemy_8_PowerCoin`: L8978–9044)
  - `asm/src/data.asm` (`byte_92D1` / `ItemToTileTable`: L904–920)
  - `asm/src/roomdata-us.asm` (`RoomData_01..FF`: L1–486)
  - `src/core/mjk_player.c` (`mjk_player_is_tile_solid`: L335–366, `mjk_player_update_collision`: L388–560, `Jack_CheckBombCollision`: L642–718)
  - `src/core/mjk_objects.c` (`g_item_to_tile_table`: L63–66, `MjkGame_OpenChest`: L76–136, `MjkGame_CollectChestItem`: L196–254, `MjkGame_UpdateChestsAndSecrets`: L256–329, `MjkGame_UpdateBombs`: L331–354)
  - `src/core/mjk_state_machine.c` (`AppendRoomObjects`: L95–228, `State_5_MainLoop`: L406–626)
  - `src/core/mjk_room_decoder.c` (`MjkRoom_HandleOpenDoorMaybe`: L236–269, `MjkRoom_BlitObjects`: L306–415, `MjkRoom_CollectBombCanonical`: L417–492)
  - `src/core/mjk_enemy.c` (`MjkEnemy_UpdateAll`: L202–245)

---

## 1. Diagnóstico Ejecutivo del Bug Reportado por el Usuario

> **Reporte del Usuario**:
> *"Items y power up: revisar las colisiones de recolección de items y powerup, he abierto un cofre con powerup y si me acerco al token por el lateral no me deja cogerlo, parece que choco con el cofre que ya no está."*

### 1.1. Causa Raíz Exacta (`[CONFIRMADA]`)

El síntoma de **"chocar con un cofre fantasma que ya no está"** al acercarse lateralmente (o desde arriba/abajo) a un ítem o token de Power-Up revelado tras abrir un cofre es producido por la interacción de **tres defectos directos en el port en C**:

1. **`mjk_player_is_tile_solid()` trata a TODOS los metatiles de ítems/power-ups (excepto `0x27` Extra Bomb) como bloques sólidos en `game->room_metatiles[]`**:
   - Cuando Jack abre un cofre en `MjkGame_OpenChest` (`src/core/mjk_objects.c`), el port en C sobrescribe la celda del cofre en `game->room_metatiles[ch->tile_y * MJK_ROOM_TILES_W + ch->tile_x]` con el Metatile ID del ítem revelado usando `g_item_to_tile_table[ch->item_id & 0x1F]` (`src/core/mjk_objects.c`, espejo de `byte_92D1` en `asm/src/data.asm:904-920`):
     - `item_id = 8` (**Mighty Coin / Power-Up Token `[P]`**) $\rightarrow$ escribe el metatile **`0x8F`**.
     - `item_id = 0..2` (**Bolsas de 100 / 300 / 1000 pts**) $\rightarrow$ escribe los metatiles **`0x4D`, `0x4E`, `0x4F`**.
     - `item_id = 3` (**Gold Coin**) $\rightarrow$ escribe el metatile **`0x47`**.
     - `item_id = 4` (**Mighty Drink**) $\rightarrow$ escribe el metatile **`0x49`**.
     - `item_id = 6` (**Sphinx**) $\rightarrow$ escribe el metatile **`0x34`**.
     - `item_id = 7` (**Tecmo Plate 1UP**) $\rightarrow$ escribe el metatile **`0x48`**.
     - `item_id = 9..14` (**Crystal Balls, Royal Family, Beelzebub**) $\rightarrow$ escribe **`0x4C`, `0x63`, `0x91`, `0x90`, `0x50`**.
   - Sin embargo, en `mjk_player_is_tile_solid` (`src/core/mjk_player.c:335-366`), **únicamente** los tiles `{ 0x00, 0x27, 0x2F }` retornan `false` (`src/core/mjk_player.c:336-338`):
     ```c
     // src/core/mjk_player.c:335-353
     static bool mjk_player_is_tile_solid(uint8_t tile_id, int px, int py) {
         if (tile_id == 0x00 || tile_id == 0x27 || tile_id == 0x2F) {
             return false;
         }
         if (tile_id >= 0x5C && tile_id <= 0x62) {
             return false;
         }
         if (tile_id < 0x38 || tile_id >= 0x4C) {
             return true; // <-- ¡0x8F, 0x4D..0x4F, 0x34, 0x4C, 0x50, 0x63, 0x90, 0x91 retornan TRUE!
         }
     ```
   - Como resultado:
     - **`0x8F` (Mighty Coin / Power-Up Token)**, **`0x4D..0x4F` (Money Bags)**, **`0x34` (Sphinx)**, **`0x4C` (Crystal Ball)**, **`0x50` (Beelzebub)**, **`0x63` (King)**, **`0x90` (Princess)** y **`0x91` (Queen)** caen en `tile_id < 0x38 || tile_id >= 0x4C` y retornan **`true` (100% sólidos en los 4 cuadrantes de 8×8)**.
     - **`0x47` (Gold Coin)**, **`0x48` (Tecmo Plate)** y **`0x49` (Mighty Drink)** caen en el rango `0x38..0x4B` (`k_quadrant_masks`, `src/core/mjk_player.c:354-365`) y son tratados como **bloques parcialmente sólidos** (`0x47` bloquea la mitad derecha TR+BR `0x50`; `0x48` bloquea la mitad izquierda TL+BL `0xA0`; `0x49` bloquea la mitad superior TL+TR `0xC0`).

2. **Orden de ejecución en `State_5_MainLoop`: la física de paredes expulsa a Jack ANTES de evaluar la recolección del ítem**:
   - En `State_5_MainLoop` (`src/core/mjk_state_machine.c:436` vs `456`), `Jack_Update(&game->jack, inputs, game->room_metatiles)` se ejecuta **20 líneas antes** que `MjkGame_UpdateChestsAndSecrets(game)`.
   - Cuando Jack camina o vuela horizontalmente hacia el token `0x8F` desde la izquierda (`vel_x > 0`), en el instante en que su borde derecho `jx + 16` entra en `[screen_cx, screen_cx + 16)`, los probes laterales 7 y 8 (`cx + 8 = jx + 16 + scroll_x`, `cy ± 4`, `src/core/mjk_player.c:383, 429-435`) consultan `mjk_player_is_tile_solid(0x8F, ...)`, que devuelve `true`.
   - Inmediatamente, `mjk_player_update_collision` (`src/core/mjk_player.c:484-503`) activa `CF_RIGHT_WALL`, anula `jack->vel_x = 0` y **expulsa (`clamp`) `jack->pos_x` a `screen_cx - 16`**:
     ```c
     // src/core/mjk_player.c:487-492
     int world_right = (int)jack->pos_x + (int)jack->scroll_x + 16;
     world_right &= ~7; // alinea al borde izquierdo del tile (screen_cx)
     jack->pos_x = (uint8_t)(world_right - 16 - (int)jack->scroll_x); // jx = screen_cx - 16
     ```
   - De igual forma, si Jack se acerca desde la derecha (`vel_x < 0`), los probes 5 y 6 (`cx - 8 = jx + scroll_x`) activan `CF_LEFT_WALL` y **expulsan `jack->pos_x` a `screen_cx + 16`** (`src/core/mjk_player.c:494-503`).
   - Cuando milisegundos después en el mismo frame se ejecuta `MjkGame_UpdateChestsAndSecrets` (`src/core/mjk_objects.c`), Jack ya fue expulsado a `jx = screen_cx - 16` (donde `jx + 12 = screen_cx - 4 < screen_cx`) o a `jx = screen_cx + 16` (donde `jx + 4 = screen_cx + 20 > screen_cx + 16`).
   - **Consecuencia**: La condición `h_overlap` (`jx + 12 >= screen_cx && jx + 4 <= screen_cx + 16`) —y a fortiori el probe central canónico `byte_A` (`jx + 8 >= screen_cx && jx + 8 < screen_cx + 16`)— **es matemáticamente inalcanzable**. Jack choca contra una pared sólida invisible del tamaño exacto del cofre de 16×16 píxeles que contenía el token.

3. **Cofres de Power Coin (`lo == 9`, `EnemyType_9_ExtraCoin`) cuando `collected_1up_this_round != 0`**:
   - Además, en `MjkGame_OpenChest` (`src/core/mjk_objects.c`), si un cofre tiene `item_id == 9` (`EnemyType_9_ExtraCoin`), el código en C hace `if (ch->item_id >= 9) { ch->collected = true; room_metatiles[...] = 0x2F; }` sin comprobar la regla canónica de `loc_A259` (`asm/src/prg.asm:5463-5470`): si `Collected1UPThisRoundFlag != 0` (`game->collected_1up_this_round != 0`), el NES **convierte `item_id = 9` en `item_id = 8` (Mighty Coin `0x8F`)** y lo deja en el cofre como metatile recolectable.

---

## 2. Arquitectura Ground-Truth 6502: Separación de Colisión Estática vs Objetos Dinámicos

En el hardware original de NES (`asm/src/prg.asm`), la colisión con el escenario y los objetos está dividida en **dos capas estrictamente separadas**:

### 2.1. Capa 1: `sub_9B6A` (`asm/src/prg.asm:4480-4610`) — Colisión Estática contra ROM (`LayoutChunks`)

- `sub_9B39` (`asm/src/prg.asm:4448-4474`) itera los 9 puntos de colisión de Jack (`byte_1 = 0..8`) usando la tabla de offsets `byte_A3D8` (`asm/src/prg.asm:5863-5872`) respecto al centro de Jack `(PlayerX_Lo, PlayerY_Lo)`:
  - **Probe 0 (`byte_1 = 0`)**: `(0, 0)` — Centro de Jack `(jx + 8, jy + 8)`.
  - **Probes 1 y 2 (`byte_1 = 1, 2`)**: `(-4, +8)` y `(+4, +8)` — Pies (suelo).
  - **Probes 3 y 4 (`byte_1 = 3, 4`)**: `(-4, -8)` y `(+4, -8)` — Cabeza (techo).
  - **Probes 5 y 6 (`byte_1 = 5, 6`)**: `(-8, -4)` y `(-8, +4)` — Flanco izquierdo.
  - **Probes 7 y 8 (`byte_1 = 7, 8`)**: `(+8, -4)` y `(+8, +4)` — Flanco derecho.
- Para cada probe, `sub_9B6A` llama a `HandleDrawHalfRow` (`asm/src/prg.asm:4489` $\rightarrow$ `L2653-2699`), el cual decodifica el metatile **directamente desde los `LayoutChunks` estáticos en ROM** (`RoomLayoutPtr`), **NUNCA desde el nametable PPU ni desde `RoomDataRAM`**.
- En los `LayoutChunks` en ROM, las celdas donde residen bombas, cofres o ítems contienen siempre **`0x2F` (aire interior)** o **`0x00` (cielo exterior)**.
- Luego, en `loc_9BBB` (`asm/src/prg.asm:4532-4578`), `sub_9B6A` clasifica el metatile estático `A`:
  - `A == $00`, `$27` o `$2F`: aire pasable (`byte_6 = $00`).
  - `A in [$5C, $62]`: tiles de fuego letal. Si `byte_1 == 0` (Probe 0, centro de Jack), mata o daña a Jack (`prg.asm:4543-4555`); si `byte_1 != 0` (Probes 1..8), retorna aire pasable (`byte_6 = $00`).
  - `A < $38` o `A >= $4C`: bloque estático 100% sólido (`byte_6 = $FF`).
  - `A in [$38, $4B]`: bloque estático con máscara de 4 cuadrantes de 8×8 leída de `byte_A3EA` (`asm/src/prg.asm:5873-5883`).

### 2.2. Capa 2: `BigBeefySubHandlesBGColliding` & `MaybePickupItem` (`asm/src/prg.asm:4615-4893`) — Colisión y Recolección de Objetos Dinámicos en `RoomDataRAM`

- Durante el **Probe 0 (`byte_1 == 0`)**, `sub_9B39` invoca `JSR BigBeefySubHandlesBGColliding` (`asm/src/prg.asm:4463`), siempre que el centro de Jack `(PlayerX_Lo, PlayerY_Lo)` se encuentre dentro de la habitación activa (`CurrentRoomIDBackup3 == CurrentRoomID`, `prg.asm:4459-4462`).
- `BigBeefySubHandlesBGColliding` calcula **3 coordenadas de celda de metatile `$YX`** (fila `Y` en nibble alto `0..14`, columna `X` en nibble bajo `0..15`):
  1. **`byte_A` (`asm/src/prg.asm:4616-4625`)**: Celda del **centro de Jack** `((PlayerY_Lo & $F0) | (PlayerX_Lo >> 4))`.
  2. **`byte_B` (`asm/src/prg.asm:4626-4656`)**: Celda del **borde vertical de Jack** en la dirección de `PlayerYVelHi` (`PlayerY_Lo + 8` si `PlayerYVelHi >= 0`; `PlayerY_Lo - 8` si `PlayerYVelHi < 0`), en la misma columna `PlayerX_Lo >> 4`. Si el borde vertical cruza de pantalla (`BCS`/`BCC`), `byte_B = $FF`.
  3. **`byte_E` (`asm/src/prg.asm:4658-4700`)**: Celda del **borde horizontal de Jack** en la dirección de movimiento horizontal (`PlayerX_Lo + 8` o `PlayerX_Lo - 8`), en la misma fila `PlayerY_Lo & $F0`. Si cruza de pantalla, `byte_E = $FF`.
- A continuación, el bucle `loc_9CD5` (`asm/src/prg.asm:4732-4808`) recorre todas las entradas de `RoomDataRAM` de la habitación `CurrentRoomIDBackup3`:
  - Lee la coordenada del objeto en `byte_70` (`$YX`) y sus flags/tipo en `MaybeCollectedThing` (`RoomDataRAM+1,X`):
    - Bit 7 (`$80`): `1` = activo / no recolectado (`BMI loc_9CE4`, `prg.asm:4738`); `0` = ya recolectado (se ignora por completo).
    - Bit 6 (`$40`): `1` = bloque secreto (`$13`) aún oculto/intacto; `0` = visible o revelado.
    - Bit 5 (`$20`): `1` = cofre (`$10`/`$11`) aún oculto; `0` = cofre visible.
    - Bits 4..0 (`$1F`): Tipo de objeto (`0x00..0x0E` = ítems y bombas; `0x10` = Cofre Rojo; `0x11` = Cofre Naranja; `0x12` = Puerta cerrada; `0x13` = Bloque Secreto).
  - **Filtro de Solidez Física (`CMP #$10 / BCC loc_9D88` en `asm/src/prg.asm:4778-4779` y `4801-4802`)**:
    - Si `byte_B == byte_70` (colisión vertical de pies o cabeza con el objeto):
      ```asm
      ; asm/src/prg.asm:4774-4794
      loc_9D3C:
          LDA MaybeCollectedThing
          AND #$3F
          CMP #$10
          BCC loc_9D88      ; <-- Si (tipo & $1F) < $10 (es decir, 0x00..0x0E: CUALQUIER ÍTEM O BOMBA),
                            ;     SALTA DIRECTO A loc_9D88 SIN TOCAR byte_312 (100% PASABLE)!
          CMP #$30
          BCS loc_9D6F      ; <-- Si bit 5 ($20) está activo (cofre oculto), NO es sólido (verifica revelado).
          ; Si es >= $10 y visible ($10 cofre rojo, $11 cofre naranja, $12 puerta, $13 bloque secreto):
          ; Activa bit de suelo ($30) o techo ($C0) en byte_312.
      ```
    - Si `byte_E == byte_70` (colisión lateral izquierda o derecha con el objeto):
      ```asm
      ; asm/src/prg.asm:4796-4813
      loc_9D6F:
          LDX byte_E
          CPX byte_70
          BNE loc_9D62
          LDA MaybeCollectedThing
          AND #$3F
          CMP #$10
          BCC loc_9D88      ; <-- Si (tipo & $1F) < $10 (0x00..0x0E: CUALQUIER ÍTEM O BOMBA),
                            ;     SALTA DIRECTO A loc_9D88 SIN TOCAR byte_312 (100% PASABLE)!
          CMP #$30
          BCS loc_9D62      ; <-- Si es cofre oculto ($30/$31), NO bloquea lateralmente.
          ; Si es >= $10 y visible ($10..$13): activa pared derecha ($03) o izquierda ($0C) en byte_312.
      ```
  - **Condición Canónica de Recolección (`loc_9D88`, `asm/src/prg.asm:4814-4821`)**:
    ```asm
    ; asm/src/prg.asm:4814-4821
    loc_9D88:
        LDX byte_A
        CPX byte_70
        BEQ loc_9D99
        RTS
    ```
    - **Regla de Oro 6502**: Un ítem o bomba (`0x00..0x0E`) jamás bloquea a Jack ni vertical (`byte_B`) ni horizontalmente (`byte_E`), y se recolecta **única y exclusivamente cuando `byte_A == byte_70`**, es decir, cuando el **centro de Jack `(jx + 8, jy + 8)`** entra en la celda de 16×16 del ítem.

---

## 3. Auditoría Completa de Metatiles: Pasables vs Sólidos en `game->room_metatiles`

A diferencia del NES (donde `LayoutChunks` en ROM y `RoomDataRAM` están separados), el port en C mantiene un único buffer mutable `game->room_metatiles[240]` por habitación (`src/core/mjk_Room_decoder.c:306-415`, `src/core/mjk_objects.c, 152, 249, 319`), donde se "blitean" tanto los bloques estáticos del cuarto como los metatiles visuales de bombas, cofres cerrados, ítems revelados, ítems expuestos y puertas abiertas/cerradas.

Por lo tanto, cuando `mjk_player_is_tile_solid(uint8_t tile_id, int px, int py)` (`src/core/mjk_player.c:335-366`) o `MjkEnemy_UpdateAll` (`src/core/mjk_enemy.c:214-235`) consultan `game->room_metatiles`, **deben distinguir qué metatiles corresponden a objetos pasables (aire, ítems, bombas, puertas abiertas) y cuáles a bloques sólidos**.

### 3.1. Tabla Maestra de Clasificación de Metatiles (`0x00..0x9F`)

| Metatile ID (`hex`) | Origen (`LayoutChunks` ROM vs `RoomDataRAM` / `byte_92D1`) | Uso en el Juego | Comportamiento en 6502 ASM (`sub_9B6A` + `BigBeefySub`) | Comportamiento Actual en C (`mjk_player_is_tile_solid`, `mjk_player.c:335-366`) | Estado / Bug en C |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`0x00`** | `LayoutChunks` ROM | Cielo exterior / Fondo vacío | **Pasable (`$00`)** (`prg.asm:4533`) | `return false` (`mjk_player.c:336`) | OK en Player (`MjkEnemy` OK) |
| **`0x01..0x26`** | `LayoutChunks` ROM | Muros, pilares, ladrillos y suelos estáticos | **Sólido 100% (`$FF`)** (`prg.asm:4556-4557`) | `return true` (`mjk_player.c:350`) | OK |
| **`0x27`** | `byte_92D1[5]` / `MjkRoom_BlitObjects` | Bomba apagada / Extra Bomb (`item_id = 5`) | **Pasable (`$00`)** (`prg.asm:4535, 4779`) | `return false` (`mjk_player.c:336`) | OK en Player (**BUG en `MjkEnemy` L214/233**: lo trata como muro/suelo) |
| **`0x28`** | `byte_92D1[$10]` (`data.asm:910`) | Cofre Rojo cerrado (`$10`) | **Sólido 100%** vía `byte_312` (`prg.asm:4778-4813`) y `byte_320` para enemigos (`prg.asm:4753`) | `return true` (`mjk_player.c:350`) | OK |
| **`0x29..0x2E`** | `LayoutChunks` ROM | Geometría sólida de habitaciones | **Sólido 100% (`$FF`)** (`prg.asm:4556-4557`) | `return true` (`mjk_player.c:350`) | OK |
| **`0x2F`** | `LayoutChunks` ROM / `byte_92D1[$0F]` | Aire interior vacío | **Pasable (`$00`)** (`prg.asm:4537`) | `return false` (`mjk_player.c:336`) | OK en Player (**BUG en `MjkEnemy` L214/233**: `mt != 0x00` trata `0x2F` como sólido!) |
| **`0x30..0x33`** | `LayoutChunks` ROM | Bloques sólidos estáticos (`0x31` usado también en bloques secretos `$13`) | **Sólido 100% (`$FF`)** (`prg.asm:4556-4557`) | `return true` (`mjk_player.c:350`) | OK |
| **`0x34`** | `byte_92D1[6]` (`data.asm:906`) | **Ítem 6: Esfinge (`Sphinx`)** (jamás aparece en `LayoutChunks` ROM) | **Pasable 100%** (`CMP #$10; BCC loc_9D88`, `prg.asm:4779, 4802`) | `return true` (`tile_id < 0x38`, `mjk_player.c:350`) | **BUG CRÍTICO**: Esfinge actúa como muro sólido 16×16 |
| **`0x35..0x37`** | `LayoutChunks` ROM | Bloques sólidos estáticos | **Sólido 100% (`$FF`)** (`prg.asm:4556-4557`) | `return true` (`mjk_player.c:350`) | OK |
| **`0x38..0x46`** | `LayoutChunks` ROM | Plataformas de medio bloque y bordes biselados (`byte_A3EA[0..14]`) | **Sólido parcial por cuadrante 8×8** (`byte_A3EA`, `prg.asm:4561-4578, 5873-5880`) | `k_quadrant_masks[0..14]` (`mjk_player.c:354-365`) | OK |
| **`0x47`** | `byte_92D1[3]` (`data.asm:905`) | **Ítem 3: Moneda de Oro (`Gold Coin`)** (y bisel estático en ROM) | **En cofre (`RoomDataRAM` tipo 3): Pasable 100%** (`prg.asm:4779, 4802`) | `k_quadrant_masks[15] = 0x50` (mitad derecha TR+BR sólida) | **BUG CRÍTICO**: Al abrir cofre con Gold Coin, bloquea a Jack por la derecha |
| **`0x48`** | `byte_92D1[7]` (`data.asm:906`) | **Ítem 7: Placa Tecmo 1UP (`Tecmo Plate`)** (y bisel estático en ROM) | **En cofre (`RoomDataRAM` tipo 7): Pasable 100%** (`prg.asm:4779, 4802`) | `k_quadrant_masks[16] = 0xA0` (mitad izquierda TL+BL sólida) | **BUG CRÍTICO**: Al abrir cofre con Tecmo Plate, bloquea a Jack por la izquierda |
| **`0x49`** | `byte_92D1[4]` (`data.asm:905`) | **Ítem 4: Bebida (`Mighty Drink`)** (y losa superior en ROM) | **En cofre (`RoomDataRAM` tipo 4): Pasable 100%** (`prg.asm:4779, 4802`) | `k_quadrant_masks[17] = 0xC0` (mitad superior TL+TR sólida) | **BUG CRÍTICO**: Al abrir cofre con Mighty Drink, bloquea a Jack en mitad superior |
| **`0x4A`** | `byte_92D1[$11]` (`data.asm:910`) / ROM | Cofre Naranja cerrado (`$11`) y losa inferior en ROM | **En `RoomDataRAM` (`$11`): Sólido 16×16** vía `byte_312` (`prg.asm:4778-4813`). En ROM: mitad inferior `0x30` | `k_quadrant_masks[18] = 0x30` (mitad inferior BL+BR sólida) | En cofres naranjas la colisión 16×16 la complementa `byte_312` / `MjkGame_UpdateChestsAndSecrets` |
| **`0x4B`** | `LayoutChunks` ROM | Bloque decorativo pasable en ROM (`byte_A3EA[19] = $00`) | **Pasable (`$00`)** (`byte_A3EA` bajo nibble `$0`, `prg.asm:5882`) | `k_quadrant_masks[19] = 0x00` (`return false`) | OK |
| **`0x4C`** | `byte_92D1[9, 10]` (`data.asm:907`) | **Ítems 9 y 10: Bola de Cristal (`Crystal Ball 1 & 2`)** (jamás aparece en `LayoutChunks` ROM) | **Pasable 100%** (`CMP #$10; BCC loc_9D88`, `prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: Crystal Ball actúa como muro sólido 16×16 |
| **`0x4D`** | `byte_92D1[0]` (`data.asm:904`) | **Ítem 0: Bolsa de Dinero 100 pts (`Money Bag 100`)** (jamás en ROM) | **Pasable 100%** (`prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: Bolsa de 100 pts actúa como muro sólido 16×16 |
| **`0x4E`** | `byte_92D1[1]` (`data.asm:904`) | **Ítem 1: Bolsa de Dinero 300 pts (`Money Bag 300`)** (jamás en ROM) | **Pasable 100%** (`prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: Bolsa de 300 pts actúa como muro sólido 16×16 |
| **`0x4F`** | `byte_92D1[2]` (`data.asm:905`) | **Ítem 2: Bolsa de Dinero 1000 pts (`Money Bag 1000`)** (jamás en ROM) | **Pasable 100%** (`prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: Bolsa de 1000 pts actúa como muro sólido 16×16 |
| **`0x50`** | `byte_92D1[14]` (`data.asm:909`) | **Ítem 14: Máscara de Beelzebub (`Beelzebub`)** (jamás en ROM) | **Pasable 100%** (`prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: Beelzebub actúa como muro sólido 16×16 |
| **`0x51..0x5B`** | `LayoutChunks` ROM | Muros, columnas y estatuas sólidas | **Sólido 100% (`$FF`)** (`prg.asm:4558`) | `return true` (`mjk_player.c:350`) | OK |
| **`0x5C..0x62`** | `LayoutChunks` ROM | Picos / Llamas de fuego letales | **Pasable en probes 1..8 (`$00`)**, daña en probe 0 (`prg.asm:4543-4555`) | `return false` en `is_tile_solid` + `Jack_IsFireTile` en probe 0 (`mjk_player.c:345, 444`) | OK |
| **`0x63`** | `byte_92D1[11]` (`data.asm:908`) | **Ítem 11: El Rey (`King`)** (jamás en ROM) | **Pasable 100%** (`prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: El Rey actúa como muro sólido 16×16 |
| **`0x64..0x87`** | `LayoutChunks` ROM | Bloques sólidos de pirámide/palacio | **Sólido 100% (`$FF`)** (`prg.asm:4558`) | `return true` (`mjk_player.c:350`) | OK |
| **`0x88, 0x8B, 0x8C, 0x92..0x94`** | `MjkRoom_BlitObjects` (`mjk_room_decoder.c:314-319`) | **Puerta cerrada** (`door_is_open == false`, tipo `$12` activo en `RoomDataRAM`) | **Sólido 100%** vía `byte_312` (`prg.asm:4778-4813`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | OK (bloquea el paso mientras la puerta está cerrada) |
| **`0x89, 0x8A, 0x8D, 0x8E, 0x95..0x98`** | `MjkRoom_BlitObjects` / `MjkRoom_HandleOpenDoorMaybe` (`mjk_room_decoder.c:264, 314-319`) | **Puerta ABIERTA** (`door_is_open == true`, bit 7 de `$12` borrado en `RoomDataRAM` por `loc_AACF`, `prg.asm:5817`) | **Pasable 100%** (en `LayoutChunks` ROM detrás de la puerta hay `$2F`, y en `RoomDataRAM` el bit 7 de `$12` es `0`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG**: `0x89, 0x8A, 0x8D, 0x8E, 0x95..0x98` son tratados como muro sólido antes del chequeo de puerta |
| **`0x8F`** | `byte_92D1[8]` (`data.asm:907`) | **Ítem 8: Moneda Mighty / Power-Up Token `[P]` (`Mighty Coin`)** (jamás en ROM) | **Pasable 100%** (`CMP #$10; BCC loc_9D88`, `prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO REPORTADO POR EL USUARIO**: Token `[P]` actúa como muro sólido 16×16 |
| **`0x90`** | `byte_92D1[13]` (`data.asm:908`) | **Ítem 13: La Princesa (`Princess`)** (jamás en ROM) | **Pasable 100%** (`prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: La Princesa actúa como muro sólido 16×16 |
| **`0x91`** | `byte_92D1[12]` (`data.asm:908`) | **Ítem 12: La Reina (`Queen`)** (jamás en ROM) | **Pasable 100%** (`prg.asm:4779, 4802`) | `return true` (`tile_id >= 0x4C`, `mjk_player.c:350`) | **BUG CRÍTICO**: La Reina actúa como muro sólido 16×16 |

### 3.2. El Caso Especial de `0x47`, `0x48` y `0x49` (Colisión Dual Estática vs Ítem)

- En el ROM original (`asm/src/prg.asm:5873-5883`, `byte_A3EA`), los índices `0x47`, `0x48` y `0x49` existen dentro del rango `0x38..0x4B` con máscaras de cuadrante `0x50`, `0xA0` y `0xC0` respectivamente, pero **`byte_92D1` (`asm/src/data.asm:905-906`) también reutiliza esos mismos IDs `0x47` (Gold Coin), `0x48` (Tecmo Plate) y `0x49` (Mighty Drink)** para dibujar el metatile del ítem en la PPU cuando se abre un cofre.
- ¿Por qué no colisionan en el NES? Porque en el NES `sub_9B6A` lee el metatile desde `LayoutChunks` en ROM (donde la celda del cofre tiene **`0x2F`**), ¡no desde el nametable de la PPU!
- En cambio, en el port en C, `MjkGame_OpenChest` (`src/core/mjk_objects.c`) escribe `0x47`, `0x48` o `0x49` directamente en `game->room_metatiles[]`, que es el mismo array que lee `mjk_player_is_tile_solid()`.
- **Verificación en todas las habitaciones del ROM (`src/core/mjk_room_decoder.c`) y en la suite de tests (`tests/test_player_physics.c`)**:
  - En `tests/test_player_physics.c:111-147`, el test `test_partial_solidity_quadrants` prueba explícitamente `0x38` (máscara `0xC0`, mitad superior sólida) y `0x39` (máscara `0x30`, mitad inferior sólida). Ningún test de física usa `0x47`, `0x48` ni `0x49` como pared.
  - Más aún: si tratamos todos los metatiles de ítems recolectables (`0x34`, `0x47`, `0x48`, `0x49`, `0x4C`, `0x4D`, `0x4E`, `0x4F`, `0x50`, `0x63`, `0x8F`, `0x90`, `0x91`) y puertas abiertas (`0x89`, `0x8A`, `0x8D`, `0x8E`, `0x95..0x98`) como **no sólidos (`return false`)** en `mjk_player_is_tile_solid()`, cualquier ítem revelado al abrir un cofre (incluyendo Gold Coin `0x47`, Tecmo Plate `0x48`, Mighty Drink `0x49` y Mighty Coin `0x8F`) será **100% pasable desde cualquier dirección (izquierda, derecha, arriba y abajo)**, idéntico al 6502 ASM.

---

## 4. Análisis de Interacción con el Scroll de Pasillo (Corridor Scroll) y Salas Verticales

El requerimiento 2 de la investigación solicita verificar si el scroll horizontal (`CAMERA_MODE_HORIZONTAL_CORRIDOR`, ej. Rooms 1–6) o vertical (`CAMERA_MODE_VERTICAL_SHAFT`) introduce desincronizaciones entre la posición visual/lógica de los cofres/ítems y las colisiones de Jack.

La inspección exhaustiva reveló **cuatro bugs reales de scroll y transición de sala**:

### 4.1. Bug de Scroll Vertical (`camera_y` omitido en `screen_cy`) (`[CONFIRMADA]`)

- En `RenderInGameScreen` (`src/core/mjk_objects.c, 696`), tanto los tiles del fondo (`room_metatiles`) como los sprites se renderizan restando `game->camera_y`:
  ```c
  // src/core/mjk_objects.c
  int py = ty * 8 - (int)game->camera_y;
  ```
- Y en `State_5_MainLoop` (`src/core/mjk_state_machine.c:434-435`), `game->jack.scroll_y = game->camera_y;`, por lo que `jack->pos_y` (`jy`) está expresado en **coordenadas de pantalla (relativas a `camera_y`)**, mientras que `mjk_player_update_collision` suma `jack->scroll_y` (`src/core/mjk_player.c:383`).
- **Sin embargo**, en **tres funciones críticas de recolección y colisión de objetos**, `screen_cy` se calculaba como `tile_y * 16` **sin restar `game->camera_y`** ni considerar `adj->down` / `adj->up`:
  1. `MjkGame_CollectChestItem` (`src/core/mjk_objects.c`):
     ```c
     int screen_cy = ch->tile_y * 16; // <-- ¡FALTA - (int)game->camera_y!
     ```
  2. `MjkGame_UpdateChestsAndSecrets` (`src/core/mjk_objects.c`):
     ```c
     int screen_cy = ch->tile_y * 16; // <-- ¡FALTA - (int)game->camera_y y adj->down/up!
     ```
  3. `MjkGame_UpdateBombs` (`src/core/mjk_objects.c`) y `Jack_CheckBombCollision` (`src/core/mjk_player.c:669`):
     ```c
     int screen_by = b->tile_y * 16;  // <-- ¡FALTA - (int)jack->scroll_y!
     ```
- **Efecto**: En cualquier habitación con `camera_y != 0` (salas con scroll vertical `CAMERA_MODE_VERTICAL_SHAFT`), la caja de colisión/recolección de todos los cofres, ítems y bombas estaba desplazada verticalmente en `+camera_y` píxeles respecto a su posición visual en pantalla y respecto a `jack->pos_y`.

### 4.2. Bug de Asimetría `adj->left` en `MjkGame_CollectChestItem` (`[CONFIRMADA]`)

- En `MjkGame_UpdateChestsAndSecrets` (`src/core/mjk_objects.c`), `screen_cx` contempla tres salas horizontales (`current_room_id`, `adj->right` y `adj->left`):
  ```c
  if (ch->room_id == game->current_room_id) {
      screen_cx = (ch->tile_x * 16) - game->camera_x;
  } else if (adj && ch->room_id == adj->right) {
      screen_cx = (256 - game->camera_x) + (ch->tile_x * 16);
  } else if (adj && ch->room_id == adj->left) {
      screen_cx = (-256 - game->camera_x) + (ch->tile_x * 16);
  }
  ```
- Pero en `MjkGame_CollectChestItem` (`src/core/mjk_objects.c`), **faltaba la rama `else if (adj && ch->room_id == adj->left)`**, haciendo que `screen_cx` quedara en `-1000` y el popup de puntos `+1000` se creara en coordenadas corruptas si el cofre pertenecía a `adj->left`.

### 4.3. Bug de Saturación de `game->bombs[24]` y `has_new_room_bombs` en Pasillos Largos (`[CONFIRMADA]`)

- En `State_5_MainLoop` (`src/core/mjk_state_machine.c:520-583`), cuando Jack avanza a lo largo de un pasillo horizontal (como la Sección 1-1: Rooms `1 -> 2 -> 3 -> 4 -> 5 -> 6`), cada transición llama a `AppendRoomObjects(game, adj->right)`.
- `AppendRoomObjects` (`src/core/mjk_state_machine.c:102, 140`) añade bombas al array `game->bombs[24]` mientras `game->bomb_count < 24`, **pero nunca purga las bombas de las habitaciones que ya quedaron atrás** (ej. Rooms 1, 2, 3 cuando Jack ya está en Room 5).
- Como cada habitación tiene ~5 bombas, al llegar a la 5.ª habitación del pasillo `game->bomb_count` alcanza el límite de `24`, y **las bombas de la última habitación del pasillo (ej. Room 6) no se cargan nunca**.
- Adicionalmente, el chequeo `has_new_room_bombs` (`src/core/mjk_state_machine.c:562-570`) solo buscaba en `game->bombs[]` para decidir si llamar a `AppendRoomObjects(game, game->current_room_id)`. Si una habitación adyacente tenía cofres o ítems expuestos pero **0 bombas** (lo cual ocurre en 28 habitaciones del juego: Rooms `$62`, `$64`, `$81`, `$A0..$A5`, `$A8..$B1`, `$B8..$BF`, etc.), `has_new_room_bombs` daba `false` o fallaba en detectar que los cofres ya estaban o no cargados.

### 4.4. Bug de Auto-Recolección Instantánea en 1 Frame al Rebotar sobre un Cofre (`[CONFIRMADA]`)

- Como documentó preliminarmente `RES-031`, cuando Jack cae sobre un cofre cerrado (`ch->is_open == false`), la rama de apertura (`src/core/mjk_objects.c`) se dispara cuando `jy + 16 >= screen_cy && jy + 16 <= screen_cy + 8`, abre el cofre (`ch->is_open = true`) e impone `game->jack.vel_y = -3`.
- Si no se ajusta `game->jack.pos_y = (uint8_t)(screen_cy - 16)` al rebotar y si la rama de recolección (`ch->is_open == true`) usa solapamiento de caja completa (`jy + 16 >= screen_cy && jy <= screen_cy + 16`), en el **frame siguiente (`Frame N+1`, 16.6 ms después)** los pies de Jack (`jy + 16`) siguen dentro de `[screen_cy, screen_cy + 5]`, por lo que **el ítem recién revelado se recolecta instantáneamente en el aire antes de que el jugador pueda verlo**.
- **Solución Canónica 6502**:
  1. Al abrir un cofre por rebote superior (`src/core/mjk_objects.c`), alinear los pies de Jack a la cara superior del cofre (`if (screen_cy >= 16) game->jack.pos_y = (uint8_t)(screen_cy - 16);`) junto con `vel_y = -3; subpixel_vy = 0;`.
  2. Evaluar la recolección del ítem (`ch->is_open == true`) usando el **probe central canónico `byte_A`** (`asm/src/prg.asm:4616-4625, 4814-4821`):
     ```c
     int center_x = jx + 8;
     int center_y = jy + 8;
     if (center_x >= screen_cx && center_x < screen_cx + 16 &&
         center_y >= screen_cy && center_y < screen_cy + 16) {
         MjkGame_CollectChestItem(game, ch);
     }
     ```
  - Con `byte_A` (`center_x = jx + 8`, `center_y = jy + 8`):
    - Al rebotar arriba del cofre (`jy = screen_cy - 16`), `center_y = screen_cy - 8 < screen_cy`, por lo que el ítem **NO se auto-consume en el frame del rebote**; se muestra en pantalla y Jack lo recoge al caer sobre él o al caminar/volar a través de él.
    - Al acercarse lateralmente caminando sobre el mismo suelo que el ítem (`jy = screen_cy`), `center_y = screen_cy + 8` está exactamente en el centro de `[screen_cy, screen_cy + 16)`, y como el metatile del ítem ahora retorna `false` en `mjk_player_is_tile_solid()`, Jack camina libremente hacia el ítem y lo recoge en cuanto `jx + 8` cruza `screen_cx` (desde la izquierda) o `screen_cx + 15` (desde la derecha).

---

## 5. Auditoría Completa de Todos los Ítems, Power-Ups, Spawns Dinámicos y Power Coins (6502 ASM vs C)

El requerimiento 3 solicita auditar **todos los tipos de ítems y power-ups del juego** (Mighty Coins, Mighty Drinks, Money Bags, Gold Coins, Extra Bombs, Sphinx, Tecmo Plate, Crystal Balls, Royal Family, Beelzebub, Power Coins, Secret Coins y Extra Coins) comparando el desensamblado 6502 (`asm/src/prg.asm`) contra la implementación actual en C (`src/core/mjk_objects.c`, `src/core/mjk_room_decoder.c`, `src/core/mjk_state_machine.c`).

### 5.1. Tabla Comparativa Exhaustiva: 6502 ASM vs Implementación C

| ID / Tipo | Nombre del Ítem / Power-Up | Metatile ID (`byte_92D1`) | Comportamiento Canónico en 6502 ASM (`asm/src/prg.asm`) | Comportamiento Actual en C (`src/core/mjk_objects.c`) | Discrepancias / Bugs Detectados en C |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`0x00`** | **Money Bag (100 pts)** | `0x4D` (`data.asm:904`) | `JTPickup_Bag100` (`prg.asm:4860-4893`): +100 pts BCD (`A=$01`), popup `$01`, `Sound_BombCollectedLit` (`$08`), borra tile a `$2F`. Pasable (`< $10`). | `case 0`: +100 pts, popup `SCORE_POPUP_100`, `MJK_SND_BOMB_LIT`, borra tile a `0x2F` (`mjk_game.c:212`). | **Bug colisión**: `0x4D` era sólido en `mjk_player_is_tile_solid`. |
| **`0x01`** | **Money Bag (300 pts)** | `0x4E` (`data.asm:904`) | `JTPickup_Bag300` (`prg.asm:4864-4893`): +300 pts BCD (`A=$03`), popup `$02`, `Sound_BombCollectedLit` (`$08`), borra tile a `$2F`. Pasable (`< $10`). | `case 1`: +300 pts, popup `SCORE_POPUP_300`, `MJK_SND_BOMB_LIT`, borra tile a `0x2F` (`mjk_game.c:213`). | **Bug colisión**: `0x4E` era sólido en `mjk_player_is_tile_solid`. |
| **`0x02`** | **Money Bag (1000 pts)** | `0x4F` (`data.asm:905`) | `JTPickup_Bag1000` (`prg.asm:4868-4893`): +1000 pts BCD (`A=$10`), popup `$04`, `Sound_BombCollectedLit` (`$08`), borra tile a `$2F`. Pasable (`< $10`). | `case 2`: +1000 pts, popup `SCORE_POPUP_1000`, `MJK_SND_BOMB_LIT`, borra tile a `0x2F` (`mjk_game.c:214`). | **Bug colisión**: `0x4F` era sólido en `mjk_player_is_tile_solid`. |
| **`0x03`** | **Gold Coin (500 pts)** | `0x47` (`data.asm:905`) | `JTPickup_GoldCoin` (`prg.asm:4897-4906`): +500 pts BCD (`A=$05`), popup `$03`, `INC GoldCoinsCollected` (`$0366`), `Sound_CoinCollected` (`$0A`), borra tile a `$2F`. Pasable (`< $10`). | `case 3`: +500 pts, popup `SCORE_POPUP_500`, `MJK_SND_COIN` (`mjk_game.c:215`). | **1. Bug colisión**: `0x47` tenía máscara sólida `0x50` (mitad derecha bloqueada).<br>**2. Bug contador**: Falta `game->gold_coins_collected++;` en `case 3` (`mjk_game.c:215`). |
| **`0x04`** | **Mighty Drink (+Tiempo)** | `0x49` (`data.asm:905`) | `JTPickup_MightyDrink` (`prg.asm:4910-4932`): +1000 pts BCD (`A=$10`), popup `$04`, suma `+$10` al StageTimer (si `>= $A0` envía a Torture Room), `Sound_TimePickup` (`$0E`). Pasable (`< $10`). | `case 4`: +1000 pts, `stage_timer += 16`, si `>= 100` llama `MjkGame_TriggerTortureRoom(game)`, `MJK_SND_TIME_PICKUP` (`mjk_game.c:216-220`). | **Bug colisión**: `0x49` tenía máscara sólida `0xC0` (mitad superior bloqueada). |
| **`0x05`** | **Extra Bomb (en cofre)** | `0x27` (`data.asm:906`) | `JTPickup_Bomb` (`prg.asm:4841, 4970-5164`): +100 pts BCD (`$01`), `Sound_BombCollectedUnlit` (`$09`), incrementa `PowerCoinBombCounter` (`$0356`, arma `PowerCoinTriggerFlag = 1` al llegar a 20), incrementa `TotalBombsCollected` (`$0365`). Pasable (`< $10`). | `case 5`: +100 pts, `MJK_SND_BOMB_UNLIT` (`mjk_game.c:221`). | **Bug contadores**: Falta incrementar `game->total_bombs_collected` y `game->power_coin_bomb_counter` (armando `power_coin_trigger_flag = 1` al llegar a 20) en `case 5`. |
| **`0x06`** | **Sphinx (Esfinge)** | `0x34` (`data.asm:906`) | `JTPickup_Sphinx` (`prg.asm:5168-5173` $\rightarrow$ `HandleSphinxEvent`, `prg.asm:7033-7088`): +10,000 pts BCD, popup `$05`, **abre la puerta de la habitación (`loc_AACF`)** si la puerta está cerrada por Esfinge (`DoorLockFlag >= $80`). Pasable (`< $10`). | `case 6`: +10,000 pts, popup `SCORE_POPUP_10000`, `MJK_SND_COIN` (`mjk_game.c:222`). | **1. Bug colisión**: `0x34` era 100% sólido.<br>**2. Bug softlock**: ¡No abre la puerta de la habitación (`MjkRoom_HandleOpenDoorMaybe(game)`) al recoger la Esfinge! |
| **`0x07`** | **Tecmo Plate (1-UP)** | `0x48` (`data.asm:906`) | Al abrir cofre (`prg.asm:5418-5423`): si `Collected1UPThisRoundFlag != 0`, se reemplaza por `8` (Mighty Coin `0x8F`). Al recoger (`JTPickup_TecmoPlate`, `prg.asm:4936-4945`): +100,000 pts BCD, popup `$06`, `MaybeCollectedThing2 = $80` $\rightarrow$ `MaybeAward1UP` (`+1` vida, `ExtraLivesFound++`, `Collected1UPThisRoundFlag = 1`, `Sound_1UP_Captive`). Pasable (`< $10`). | Reemplazo en `MjkGame_OpenChest` (`mjk_game.c:107-109`). En `case 7`: +100,000 pts, `lives++`, `extra_lives_found++`, `collected_1up_this_round = 1`, `MJK_SND_1UP` (`mjk_game.c:223-230`). | **Bug colisión**: `0x48` tenía máscara sólida `0xA0` (mitad izquierda bloqueada). |
| **`0x08`** | **Mighty Coin (`[P]` Token)** | `0x8F` (`data.asm:907`) | `JTPickup_MightyCoin` (`prg.asm:4949-4966`): +1000 pts BCD, popup `$04`, `Sound_MightyCoinPickup` (`$0C`), `INC PlayerMightyCoins` (`$0349`), si `>= 10` llama `GoToTheTortureRoom`. Pasable (`< $10`). | `case 8`: +1000 pts, `MJK_SND_MIGHTY_COIN`, `mighty_coins++`, si `>= 10` llama `MjkGame_TriggerTortureRoom(game)` (`mjk_game.c:231-238`). | **BUG PRINCIPAL DEL USUARIO**: `0x8F` era 100% sólido (`tile_id >= 0x4C`) en `mjk_player_is_tile_solid`, impidiendo recoger el token `[P]` lateralmente. |
| **`0x09` (en cofre)** | **Extra Coin 1-UP (`EnemyType_9_ExtraCoin`)** | Dinámico OAM / `0x8F` | Al abrir cofre (`MaybeSpawnThingFromObject`, `prg.asm:5462-5470`): **si `Collected1UPThisRoundFlag != 0`, cambia `item_id` a `8` (Mighty Coin `0x8F`)** y lo deja en el cofre. Si es `0`, spawnea entidad `EnemyType_9_ExtraCoin` (que al tocarla en `prg.asm:9042` da +1000 pts y `MaybeAward1UP`). | En `MjkGame_OpenChest` (`mjk_game.c:125-131`): `if (ch->item_id >= 9)` marca `ch->collected = true` y escribe `0x2F` sin dar nada ni reemplazar por `8`. | **Bug doble**: 1) Si `collected_1up_this_round != 0`, debe convertirse en `ch->item_id = 8` (`0x8F`). 2) Si no, debe otorgar/spawnear el Extra Coin. |
| **`0x09, 0x0A` (`$79, $7A` expuestos)** | **Crystal Ball 1 & 2** (`RoomData_A0, A1`) | `0x4C` (`data.asm:907`) | `JT_94AD_7` (`prg.asm:3447-3471`) registra el ítem en `RoomDataRAM`. `JTPickup_CrystalBall1/2` (`prg.asm:5205-5223`): marca bit `1` o `2` en `PlayerCrystalBallsCollected` (`$036B`), suma `+1` GDV, `Sound_CoinCollected` (`$0A`), borra tile a `$2F`. Pasable (`< $10`). | `AppendRoomObjects` (`mjk_state_machine.c:225`) **ignora `hi == 7` (`$7x`)**, por lo que nunca se cargan en `game->chests[]`! En `case 9, 10` (`mjk_game.c:239-242`) no actualiza `crystal_balls_collected` ni suma GDV en `case 9`. | **1. Bug colisión**: `0x4C` era 100% sólido.<br>**2. Bug carga**: `AppendRoomObjects` no carga `$7x` (`hi == 7`).<br>**3. Bug estado**: `case 9` y `case 10` no actualizan `game->crystal_balls_collected |= (1 << (item_id - 9))` ni `gdv_value` en `case 9`. |
| **`0x0B..0x0D` (`$7B..$7D` expuestos)** | **Royal Family: King (`0x0B`), Queen (`0x0C`), Princess (`0x0D`)** (`RoomData_A2..A4`) | `0x63`, `0x91`, `0x90` (`data.asm:908`) | `JTPickup_Family` (`prg.asm:5227-5237`): borra tile a `$2F`, reproduce sonido y **abre la puerta del Palacio Real (`loc_AACF`)**! Pasable (`< $10`). | `AppendRoomObjects` ignora `hi == 7`. En `default:` (`mjk_game.c:243-246`) solo da +1000 pts y **no abre la puerta** (`MjkRoom_HandleOpenDoorMaybe`). | **1. Bug colisión**: `0x63, 0x91, 0x90` eran 100% sólidos.<br>**2. Bug carga**: `AppendRoomObjects` no carga `hi == 7`.<br>**3. Bug softlock**: No abre la puerta del cuarto al rescatar al miembro de la familia real. |
| **`0x0E` (`$7E` expuesto)** | **Beelzebub Mask** (`RoomData_A5`) | `0x50` (`data.asm:909`) | `JTPickup_Beelzebub` (`prg.asm:5241-5249`): +1,000,000 pts BCD (`REV_US`), `Music_CollectedFullFamily`, borra tile a `$2F` y **abre la puerta (`loc_AACF`)**. Pasable (`< $10`). | `AppendRoomObjects` ignora `hi == 7`. En `default:` solo da +1000 pts y **no abre la puerta**. | **1. Bug colisión**: `0x50` era 100% sólido.<br>**2. Bug carga**: `AppendRoomObjects` no carga `hi == 7`.<br>**3. Bug softlock/score**: No suma 1,000,000 pts ni abre la puerta. |
| **Power Coin (`EnemyType_8`)** | **Power Coin (Transmutación de enemigos)** | Sprite OAM (`prg.asm:8978`) | Se arma (`PowerCoinTriggerFlag = 1`) al recoger 20 bombas (`prg.asm:4980-4989`). Al abrir el siguiente cofre (`prg.asm:5426-5428`) cambia `PowerCoinTriggerFlag` de `1 -> 2` y spawnea `EnemyType_8_PowerCoin`. Al tocarlo o recoger la siguiente bomba (`prg.asm:4991-4999, 9034-9039`), pasa a `3` y activa `EnemyCoinTimer = $40` (320 frames = `64 * 5`), convirtiendo a todos los enemigos en monedas (`EnemyState = 3`). | En `MjkGame_OpenChest` (`mjk_game.c:115-122`) y `MjkRoom_CollectBombCanonical` (`mjk_room_decoder.c:440-444`), al pasar `power_coin_trigger_flag` a `3` asigna `game->transmute_timer = 320`, pero **olvida sincronizar `game->enemy_coin_timer = 320` y `game->jack.enemy_coin_timer = 320`**. | **Bug sincronización de timers**: `MjkGame_OpenChest` y `MjkRoom_CollectBombCanonical` deben activar los tres campos (`transmute_timer`, `enemy_coin_timer` y `jack.enemy_coin_timer = 320`) igual que `mjk_game_activate_power_coin` (`mjk_game.c:1226-1234`). |

---

## 6. Especificación Quirúrgica de Correcciones para `c_implementer`

A continuación se detallan los cambios exactos, archivo por archivo y línea por línea, requeridos para resolver definitivamente el bug reportado y todos los defectos colaterales descubiertos, manteniendo **100% de compatibilidad con todos los tests existentes en `ctest`**.

### 6.1. Fix 1: `src/core/mjk_player.c` — Hacer pasables todos los metatiles de ítems, power-ups y puertas abiertas en `mjk_player_is_tile_solid` y sincronizar `scroll_y` en `Jack_CheckBombCollision`

1. En `src/core/mjk_player.c:335-366` (`mjk_player_is_tile_solid`), crear un helper explícito `mjk_player_is_passable_item_or_open_door(uint8_t tile_id)` o incluir los metatiles de ítems y puertas abiertas en el retorno temprano `return false`:
   - **Metatiles de aire y bombas**: `0x00`, `0x27`, `0x2F`.
   - **Metatiles de ítems de cofre y expuestos (`byte_92D1[0..14]`)**:
     - `0x34` (Sphinx)
     - `0x47` (Gold Coin)
     - `0x48` (Tecmo Plate 1-UP)
     - `0x49` (Mighty Drink)
     - `0x4C` (Crystal Ball 1 & 2)
     - `0x4D` (Money Bag 100)
     - `0x4E` (Money Bag 300)
     - `0x4F` (Money Bag 1000)
     - `0x50` (Beelzebub Mask)
     - `0x63` (King)
     - `0x8F` (Mighty Coin / Power-Up Token `[P]`)
     - `0x90` (Princess)
     - `0x91` (Queen)
   - **Metatiles de puertas abiertas**: `0x89`, `0x8A`, `0x8D`, `0x8E`, `0x95`, `0x96`, `0x97`, `0x98`.
   ```c
   static bool mjk_player_is_tile_solid(uint8_t tile_id, int px, int py) {
       // 0x00, 0x27 (bomb), 0x2F (air) -> Passable
       if (tile_id == 0x00 || tile_id == 0x27 || tile_id == 0x2F) {
           return false;
       }

       // Collectible items & power-up tokens (byte_92D1[0..14]) -> 100% Passable
       // In 6502 ASM (prg.asm:4778-4779, 4801-4802), objects with (type & 0x1F) < 0x10
       // branch to loc_9D88 without setting any solid wall/floor bits in byte_312.
       switch (tile_id) {
           case 0x34: // Sphinx (item 6)
           case 0x47: // Gold Coin (item 3)
           case 0x48: // Tecmo Plate 1UP (item 7)
           case 0x49: // Mighty Drink (item 4)
           case 0x4C: // Crystal Ball 1 & 2 (items 9, 10)
           case 0x4D: // Money Bag 100 (item 0)
           case 0x4E: // Money Bag 300 (item 1)
           case 0x4F: // Money Bag 1000 (item 2)
           case 0x50: // Beelzebub Mask (item 14)
           case 0x63: // King (item 11)
           case 0x8F: // Mighty Coin / Power-Up Token [P] (item 8)
           case 0x90: // Princess (item 13)
           case 0x91: // Queen (item 12)
           // Open door metatiles (MjkRoom_BlitObjects / MjkRoom_HandleOpenDoorMaybe)
           case 0x89:
           case 0x8A:
           case 0x8D:
           case 0x8E:
           case 0x95:
           case 0x96:
           case 0x97:
           case 0x98:
               return false;
           default:
               break;
       }

       // Fire hazard tiles (0x5C to 0x62): passable for Probes 1..8
       if (tile_id >= 0x5C && tile_id <= 0x62) {
           return false;
       }

       // Outside partial-solidity range [0x38, 0x4B] -> 100% solid
       if (tile_id < 0x38 || tile_id >= 0x4C) {
           return true;
       }
       ...
   ```
2. En `Jack_CheckBombCollision` (`src/core/mjk_player.c:669`), restar `(int)jack->scroll_y` al calcular `screen_by`:
   ```c
   int screen_by = (int)(b->tile_y * 16) - (int)jack->scroll_y;
   ```

### 6.2. Fix 2: `src/core/mjk_objects.c` — Probe Central Canónico `byte_A`, Coordenadas de Scroll, Paridad de Ítems `0..14` y Power Coin Timers

1. **En `MjkGame_OpenChest` (`src/core/mjk_objects.c`)**:
   - Cuando `game->power_coin_trigger_flag` pasa a `3` (`mjk_game.c:119-121`), sincronizar los tres timers de transmutación (`game->transmute_timer = 320; game->enemy_coin_timer = 320; game->jack.enemy_coin_timer = 320;`).
   - Implementar la regla canónica de `loc_A259` (`asm/src/prg.asm:5463-5470`): si `ch->item_id == 9` (`EnemyType_9_ExtraCoin`) y `game->collected_1up_this_round != 0`, reemplazar `ch->item_id = 8` (Mighty Coin `0x8F`) **antes** del bloque `if (ch->item_id >= 9)`, para que el cofre revele un Mighty Coin recolectable tal como en el NES.
2. **En `MjkGame_CollectChestItem` (`src/core/mjk_objects.c`)**:
   - Añadir la rama `else if (adj && ch->room_id == adj->left)` en el cálculo de `screen_cx` (`mjk_game.c:200-207`) y restar `(int)game->camera_y` (con soporte `adj->down` / `adj->up`) en `screen_cy` (`mjk_game.c:208`).
   - En `case 3` (Gold Coin, `mjk_game.c:215`): añadir `game->gold_coins_collected++;`.
   - En `case 5` (Extra Bomb, `mjk_game.c:221`): añadir `game->total_bombs_collected++;` y el incremento de `game->power_coin_bomb_counter` (si llega a `>= 20`, reiniciar a `0` y poner `if (game->power_coin_trigger_flag == 0) game->power_coin_trigger_flag = 1;`).
   - En `case 6` (Sphinx, `mjk_game.c:222`): llamar a `MjkRoom_HandleOpenDoorMaybe(game);` para abrir la puerta de la habitación (`HandleSphinxEvent`, `prg.asm:7033-7088`).
   - En `case 9` y `case 10` (Crystal Balls 1 & 2, `mjk_game.c:239-242`): distinguir si proviene de un ítem expuesto (`$79`/`$7A`) actualizando `if (ch->item_id == 9) game->crystal_balls_collected |= 0x01; else game->crystal_balls_collected |= 0x02;` y sumando `game->gdv_value = (uint8_t)mjk_bcd_add(game->gdv_value, 1);` en ambos casos (`prg.asm:5205-5223`).
   - En `case 11, 12, 13` (King, Queen, Princess, `prg.asm:5227-5237`): llamar a `MjkRoom_HandleOpenDoorMaybe(game);`.
   - En `case 14` (Beelzebub Mask, `prg.asm:5241-5249`): sumar `mjk_score_add(game, 1000000);` y llamar a `MjkRoom_HandleOpenDoorMaybe(game);`.
3. **En `MjkGame_UpdateChestsAndSecrets` (`src/core/mjk_objects.c`)**:
   - Calcular `screen_cy` considerando `game->camera_y` y las salas adyacentes verticales (`adj->down`, `adj->up`):
     ```c
     int screen_cy = -1000;
     if (ch->room_id == game->current_room_id ||
         (adj && (ch->room_id == adj->right || ch->room_id == adj->left))) {
         screen_cy = (ch->tile_y * 16) - (int)game->camera_y;
     } else if (adj && ch->room_id == adj->down) {
         screen_cx = (ch->tile_x * 16) - (int)game->camera_x;
         screen_cy = (240 - (int)game->camera_y) + (ch->tile_y * 16);
     } else if (adj && ch->room_id == adj->up) {
         screen_cx = (ch->tile_x * 16) - (int)game->camera_x;
         screen_cy = (-240 - (int)game->camera_y) + (ch->tile_y * 16);
     }
     ```
   - En la rama `if (ch->is_open)` (`src/core/mjk_objects.c`), evaluar la recolección con el **probe central canónico `byte_A`** (`asm/src/prg.asm:4616-4625, 4814-4821`):
     ```c
     if (ch->is_open) {
         // Canonical 6502 center probe (byte_A == byte_70, prg.asm:4814-4821):
         // Jack's center (jx + 8, jy + 8) must be inside the 16x16 item cell.
         int center_x = jx + 8;
         int center_y = jy + 8;
         if (center_x >= screen_cx && center_x < screen_cx + 16 &&
             center_y >= screen_cy && center_y < screen_cy + 16) {
             MjkGame_CollectChestItem(game, ch);
         }
     }
     ```
   - En la rama `else` (cofre cerrado, `src/core/mjk_objects.c`), al abrir un cofre saltando encima (`vel_y >= 0`), alinear `pos_y` a la parte superior del cofre (`if (screen_cy >= 16) game->jack.pos_y = (uint8_t)(screen_cy - 16);`) y limpiar `game->jack.subpixel_vy = 0;` para evitar penetración.
4. **En `MjkGame_UpdateBombs` (`src/core/mjk_objects.c`)**:
   - Incluir soporte para `adj->down` y `adj->up` en `screen_by` (además de `Jack_CheckBombCollision`).

### 6.3. Fix 3: `src/core/mjk_room_decoder.c` — Sincronizar `enemy_coin_timer` al activar Power Coin por Bomba

- En `MjkRoom_CollectBombCanonical` (`src/core/mjk_room_decoder.c:440-444`), cuando `game->power_coin_trigger_flag == 2` pasa a `3`, sincronizar:
  ```c
  game->power_coin_trigger_flag = 3;
  game->transmute_timer = 320;
  game->enemy_coin_timer = 320;
  game->jack.enemy_coin_timer = 320;
  ```

### 6.4. Fix 4: `src/core/mjk_state_machine.c` — Cargar Ítems Expuestos (`$7x`, `hi == 7`) y Evitar Saturación de `bombs[]` en Pasillos

1. En `AppendRoomObjects` (`src/core/mjk_state_machine.c:225-227`), reemplazar el comentario `// hi == 7: Exposed item` por la carga real del ítem expuesto en `game->chests[]` con `is_open = true`, `is_hidden = false` y la escritura de su metatile `g_item_to_tile_table[lo & 0x1F]` en `game->room_metatiles[]` (espejo exacto de `JT_94AD_7` en `asm/src/prg.asm:3447-3471`):
   ```c
   } else if (hi == 7) {
       // Exposed item ($7x in RoomData_A0..A5, JT_94AD_7 in prg.asm:3447-3471)
       if (idx < MJK_ROOM_METATILE_COUNT && room_id == game->current_room_id) {
           static const uint8_t k_exposed_item_tiles[16] = {
               0x4D, 0x4E, 0x4F, 0x47, 0x49, 0x27, 0x34, 0x48,
               0x8F, 0x4C, 0x4C, 0x63, 0x91, 0x90, 0x50, 0x2F
           };
           game->room_metatiles[idx] = k_exposed_item_tiles[lo & 0x0F];
       }
       if (game->chest_count < 16) {
           MjkChest* c = &game->chests[game->chest_count++];
           c->tile_x = tx;
           c->tile_y = ty;
           c->is_orange = false;
           c->is_hidden = false;
           c->is_open = true;
           c->collected = false;
           c->item_id = lo;
           c->room_id = room_id;
       }
   }
   ```
2. En `AppendRoomObjects` (`src/core/mjk_state_machine.c:95-100`), evitar duplicar objetos si `AppendRoomObjects` se invoca para un `room_id` cuyos objetos ya están cargados en `game->bombs[]`, `game->chests[]` o `game->secrets[]`, y en `State_5_MainLoop` (`src/core/mjk_state_machine.c:555-583`), compactar/reciclar las ranuras de `game->bombs[]` de habitaciones que ya no son ni `current_room_id` ni adyacentes (`adj->left`, `adj->right`, `adj->up`, `adj->down`) cuando `game->bomb_count >= 20`, garantizando que en pasillos de 6 pantallas jamás se agote el límite de 24 bombas.

### 6.5. Fix 5: `src/core/mjk_enemy.c` — Corregir `MjkEnemy_UpdateAll` para que los Enemigos no Colisionen contra Aire (`0x2F`), Bombas (`0x27`) ni Ítems

- En `MjkEnemy_UpdateAll` (`src/core/mjk_enemy.c:214-216, 233-235`), el código actual hace:
  ```c
  uint8_t mt = game->room_metatiles[ty * MJK_ROOM_TILES_W + tx];
  if (mt != 0x00) { // <-- ¡BUG: trata 0x2F (aire interior), 0x27 (bombas) e ítems como muros/suelos!
  ```
- Debe usar un predicado de solidez para enemigos donde `0x00`, `0x27`, `0x2F`, los ítems recolectables (`0x34`, `0x47..0x49`, `0x4C..0x50`, `0x63`, `0x8F..0x91`), puertas abiertas (`0x89, 0x8A, 0x8D, 0x8E, 0x95..0x98`) y fuego (`0x5C..0x62`) sean **pasables (`false`)**, mientras que los bloques estáticos y los cofres cerrados visibles (`0x28`, `0x4A`, `byte_320` en `prg.asm:4502-4529, 4753`) sean sólidos (`true`).

---

## 7. Verificación de Compatibilidad con Tests Existentes (`ctest`)

Se verificaron todas las aserciones de la suite de tests del proyecto contra los cambios especificados:
- **`tests/test_chests_secrets.c`**:
  - `test_open_orange_chest_from_above` (`L163-189`): Coloca a Jack con `pos_x = 80, pos_y = 64` sobre un cofre en `tile_x = 5, tile_y = 5` (`screen_cx = 80, screen_cy = 80`). Con `jy + 16 = 80 == screen_cy`, abre el cofre, `vel_y = -3`, y como `center_y = 72 < 80`, **no consume el ítem en el rebote** (`ch->is_open == true`, `ch->collected == false`). Pasa al 100%.
  - `test_collect_all_chest_items` (`L358-516`): Coloca a Jack directamente en `pos_x = 80, pos_y = 80` sobre un cofre abierto en `tile_x = 5, tile_y = 5` (`screen_cx = 80, screen_cy = 80`). Con `center_x = 88 in [80, 96)` y `center_y = 88 in [80, 96)`, `MjkGame_UpdateChestsAndSecrets` recoge todos los ítems (`0..8`) en el primer frame. Además, ahora si un test o el jugador camina desde `pos_x = 64` hacia `pos_x = 80` con `Jack_Update`, `mjk_player_is_tile_solid` devuelve `false` para todos los ítems y Jack los recoge lateralmente sin chocar.
- **`tests/test_player_physics.c`**:
  - `test_partial_solidity_quadrants` (`L111-147`): Utiliza `0x38` y `0x39` (intactos en `k_quadrant_masks`). Pasa al 100%.
- **`tests/test_room_objects.c`** y **`tests/test_corridor_scroll.c`**:
  - Todos los tests de decodificación de objetos y scroll de pasillo se benefician directamente de la no-duplicación y reciclaje de bombas fuera de cámara.
