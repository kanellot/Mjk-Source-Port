# RES-031: Paridad Canónica de Cofres — Paleta/Subpaleta, Comportamiento y Colisiones con Jack (`TASK-F7-004`)

- **ID de Investigación**: `RES-031`
- **Tarea Vinculada**: `TASK-F7-004` (Cofres — Color/Paleta, Comportamiento y Colisiones con Jack)
- **Área Temática**: 03 - Escenarios, Metatiles y Geometría de Salas / 02 - Física y Colisiones / 05 - Renderizado y PPU
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-25
- **Agente Autor**: `asm_researcher` (Nivel 3)
- **Fuentes de Verdad Analizadas**:
  - Desensamblado 6502: [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm), [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm), [`asm/src/roomdata-us.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/roomdata-us.asm).
  - Código C del Port: [`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c), [`src/core/mjk_player.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c), [`src/core/mjk_state_machine.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c), [`src/core/mjk_room_decoder.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c), [`src/core/mjk_room_data.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_data.c), [`include/mjk_room_data.h`](file:///c:/Users/kanel/Documents/mjk/include/mjk_room_data.h).
  - Suite de Pruebas: [`tests/test_chests_secrets.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_chests_secrets.c), [`tests/test_room_objects.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_room_objects.c), [`tests/test_corridor_scroll.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_corridor_scroll.c).

---

## 1. Resumen Ejecutivo (`[CONFIRMADA]`)

El reporte del usuario (*"Revisar los cofres, color de sprite y comportamiento, la interacción de las colisiones con Jack creo que no es exacta"*) queda **100% confirmado** (`[CONFIRMADA]`) tras contrastar el motor 6502 ([`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm) y [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm)) con la implementación en C ([`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c), [`src/core/mjk_player.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c) y [`src/core/mjk_state_machine.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c)):

1. **Color / Subpaleta Incorrecta (Cofres Grises en vez de Rojos y Naranjas)**:
   - En el juego original, los cofres son **metatiles de fondo 16×16 en la Nametable** (`Metatile 0x28` Cofre Rojo cerrado con **Subpaleta `3`**, y `Metatile 0x4A` Cofre Naranja cerrado con **Subpaleta `0`**).
   - En [`src/core/mjk_game.c:586`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L586) y [`src/core/mjk_game.c:681`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L681) (`RenderInGameScreen`, Pasos `3b` y `3b-right`), la subpaleta se fijó a `uint8_t sub = 2;` sin actualizarse en la rama `if (!game->chests[c].is_open)`. Como la Subpaleta 2 en el Set 0 (Salas 1..6) es piedra gris (`{ $0A, $0F, $10, $30 }`), **todos los cofres cerrados se renderizan en color gris** en lugar de **Rojo (`$16`, Subpaleta `3`)** y **Naranja (`$26`, Subpaleta `0`)**.

2. **Desaparición Instantánea en 1 Frame (16 ms) del Ítem Revelado al Abrir un Cofre**:
   - En [`src/core/mjk_game.c:277-302`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L277-L302) (`MjkGame_UpdateChestsAndSecrets`), cuando Jack toca la tapa superior de un cofre (`jy + 16 >= screen_cy && jy + 16 <= screen_cy + 8`), el cofre se abre (`ch->is_open = true`) pero **no se alinea `pos_y` a `screen_cy - 16`**, y en el frame inmediatamente siguiente (`Frame N+1`, 16.6 ms después) la condición de recolección del ítem (`if (h_overlap && jy + 16 >= screen_cy && jy <= screen_cy + 16)` en [`mjk_game.c:278`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L278)) evalúa `true` porque la punta de los pies (`jy + 16`) sigue a `screen_cy + 1..5` mientras Jack despega hacia arriba.
   - En consecuencia, **el ítem dentro del cofre (bolsa de puntos, Mighty Drink, moneda) se consume 1 frame después de abrirse el cofre y jamás llega a verse en pantalla**.
   - En [`asm/src/prg.asm:4616-4625, 4818-4820`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4818-L4820) (`MaybePickupItem`), un ítem expuesto solo se recoge cuando la sonda central de Jack `byte_A` coincide con la celda del ítem (`screen_cx <= jx + 8 < screen_cx + 16` y `screen_cy <= jy + 8 < screen_cy + 16`). Al rebotar desde la tapa superior (`jy = screen_cy - 16`, centro `jy + 8 = screen_cy - 8 < screen_cy`), Jack sube alejándose del ítem, **dejando visible el ítem en el mapa** hasta que vuelve a caer sobre él.

3. **Conflicto de Solidez en `room_metatiles` y `mjk_player_is_tile_solid` ([`src/core/mjk_player.c:335-365`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L335-L365))**:
   - Cuando un cofre se abre en [`src/core/mjk_game.c:132-135`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L132-L135), escribe el metatile del ítem revelado (`0x4D` bolsa 100, `0x4E` bolsa 300, `0x4F` bolsa 1000, `0x47` moneda oro, `0x49` Mighty Drink, `0x48` Tecmo Plate, `0x8F` Mighty Coin) en `game->room_metatiles`.
   - En [`src/core/mjk_player.c:336-352`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L336-L352), `mjk_player_is_tile_solid` solo considera pasables `{ 0x00, 0x27, 0x2F }`, por lo que **clasifica todos los ítems revelados (`0x4D`, `0x4E`, `0x4F`, `0x8F`, etc.) como bloques sólidos 16×16 (o `0x47..0x49` como bloques semi-sólidos)**, haciendo que Jack choque contra la bolsa de puntos o moneda como si fuera un ladrillo.
   - Análogamente, si `0x28` (cofre rojo cerrado) está escrito en `game->room_metatiles` (p. ej. tras revelar un cofre oculto en [`mjk_game.c:96`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L96)), como `Jack_Update` corre en [`mjk_state_machine.c:436`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L436) **antes** que `MjkGame_UpdateChestsAndSecrets` ([`mjk_state_machine.c:456`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L456)), `Jack_Update` frena a Jack (`vel_y = 0; subpixel_vy = 0;`) antes de llegar a `MjkGame_UpdateChestsAndSecrets`, **impidiendo que el cofre revelado pueda abrirse jamás**.

4. **Geometría de Contacto y Solidez Física de Cofres Cerrados**:
   - En [`src/core/mjk_game.c:274, 342`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L274), `h_overlap` usa `(jx + 12 >= screen_cx && jx + 4 <= screen_cx + 16)` (25 píxeles de ancho), mientras que en [`asm/src/prg.asm:4619-4625, 4642-4645`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4619-L4625) las sondas `byte_A` y `byte_B` evalúan exclusivamente la columna del centro de Jack (`(jx + 8 + scroll_x) >> 4 == tile_x`, es decir, `jx + 8 >= screen_cx && jx + 8 < screen_cx + 16`, 16 píxeles exactos).
   - En [`src/core/mjk_game.c:286-325`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L286-L325), los cofres cerrados visibles carecen de colisión lateral (`byte_E` en [`prg.asm:4787-4812`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4787-L4812): muro sólido si `PlayerMightyLevel < 2`, o apertura lateral instantánea si `PlayerMightyLevel >= 2`) y carecen de colisión de techo sólido (`byte_312 |= $C0` -> `sub_A1B9` en [`prg.asm:4780-4782, 5529-5555`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4780-L4782)) cuando Jack los golpea desde abajo sin tener el nivel Mighty requerido.

---

## 2. Ground-Truth 6502: Representación Visual, Metatiles y Subpaletas (`[CONFIRMADA]`)

### 2.1 ¿Son los cofres Metatiles en la Nametable o Sprites OAM?

En *Mighty Bomb Jack* (NES), **todos los cofres cerrados (`$10` Rojo, `$11` Naranja) y todos los ítems estáticos (`$00..$0E`) revelados al abrirlos son 100% Metatiles de Fondo 16×16** escritos en la Nametable PPU (`$2000`/`$2800`) y en la Attribute Table (`$23C0`/`$2BC0`):
- Durante la carga inicial de sala (`DrawFullScreen` -> `sub_9696` -> `sub_9592` -> `sub_96B7`, [`asm/src/prg.asm:3684-3717`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3684-L3717)) y durante el scroll continuo (`loc_A8A7` / `loc_A8DF`, [`asm/src/prg.asm:6586-6645`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L6586-L6645)), el motor lee el estado actual del objeto en `RoomDataRAM,Y`, lo traduce a un ID de Metatile mediante `ItemToTileTable,X` ([`asm/src/data.asm:888-906`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L888-L906)), actualiza sus bits de subpaleta con `HandleTileAttributes` ([`asm/src/prg.asm:7092-7215`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L7092-L7215)) y vuelca sus 4 tiles CHR 8×8 desde `MetatileDefinitions` ([`asm/src/data.asm:1549-1702`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L1549-L1702)).
- Al abrirse un cofre en runtime (`MaybeOpenHiddenBlock` -> `sub_A1A6`, [`asm/src/prg.asm:5515-5525`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5515-L5525)):
  - Si el comando del cofre tiene nibble bajo `0..8`, `sub_A1A6` escribe el ID del ítem (`0..8`) en `RoomDataRAM,Y` y blittea el metatile correspondiente de `ItemToTileTable` en la Nametable mediante `PPUUpdateBuffer` (`PPUUpdateFlag2`).
  - **Único caso en que intervienen Sprites OAM (`lo >= 9`)**: Si el nibble bajo del comando del cofre es `9..15` (`CMP #9; BCC loc_A10C; JSR MaybeSpawnThingFromObject`, [`asm/src/prg.asm:5410-5412, 5462-5495`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5462-L5495)), se invoca `InitEnemy` para spawnear una entidad OAM dinámica (`9` = Extra Coin, `$A` = Secret Coin, `$B` = Bonus Statue, `$C..$F` = Enemigo) en las coordenadas del cofre (`byte_94, byte_92`), y `sub_A1A6` escribe `$0F` en `RoomDataRAM,Y` (metatile vacío `$2F`) — salvo si `lo == 9` y `Collected1UPThisRoundFlag != 0`, en cuyo caso se degrada a ítem estático `8` (Moneda Mighty, Metatile `$8F`).

### 2.2 Tabla Exacta de Bytes en `asm/src/data.asm` (`ItemToTileTable`, `MetatileDefinitions`, `TileAttributeTable`)

| Estado `RoomDataRAM` | Nombre / Función | Metatile ID (`ItemToTileTable`, [`data.asm:888-906`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L888-L906)) | 4 Tiles CHR 8×8 `{ TL, TR, BL, BR }` (`MetatileDefinitions`, [`data.asm:1549-1702`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L1549-L1702)) | Entrada `TileAttributeTable` ([`data.asm:598-638`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L598-L638)) | Subpaleta 2-bit (`0..3`) y Byte Atributo PPU | Colores Reales en Sala 1 (Set 0 + `MainPalette`) |
| :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| **`$10`** | **Cofre Rojo Cerrado** (`red chest`) | **`0x28`** (`data.asm:905`) | **`$E8, $E9, $EA, $EB`** (`data.asm:1590`, offset `$A0`) | Byte `10` (`$0A` = `$EA`): `Attrib 3, 2, 2, 2 ; 28` (`data.asm:609`) | **Subpaleta `3`** (`11b` $\rightarrow$ **`$FF`**) | C0=`$0A` (Verde), C1=`$0F` (Negro), **C2=`$16` (Rojo)**, **C3=`$30` (Blanco)** |
| **`$11`** | **Cofre Naranja Cerrado** (`orange chest`) | **`0x4A`** (`data.asm:906`) | **`$99, $9B, $EA, $EB`** (`data.asm:1624`, offset `$128`) | Byte `18` (`$12` = `$30`): `Attrib 0, 3, 0, 0 ; 48` (`data.asm:617`) | **Subpaleta `0`** (`00b` $\rightarrow$ **`$00`**) | C0=`$0A` (Verde), C1=`$0F` (Negro), **C2=`$26` (Naranja)**, **C3=`$30` (Blanco)** |
| *(N/A)* | Pilar decorativo de fondo *(NO es cofre)* | **`0x29`** | **`$89, $8C, $8B, $8E`** (`data.asm:1591`, offset `$A4`) | Byte `10` (`$0A` = `$EA`): `Attrib 3, 2, 2, 2 ; 28` (`data.asm:609`) | **Subpaleta `2`** (`10b` $\rightarrow$ **`$AA`**) | C0=`$0A`, C1=`$0F`, C2=`$10` (Gris), C3=`$30` (Blanco) |
| **`$00`** | **Bolsa 100 pts** (`100 pt bag`) | **`0x4D`** (`data.asm:889`) | **`$DE, $DF, $F4, $F5`** (`data.asm:1627`, offset `$134`) | Byte `19` (`$13` = `$FF`): `Attrib 3, 3, 3, 3 ; 4C` (`data.asm:618`) | **Subpaleta `3`** (`11b` $\rightarrow$ **`$FF`**) | C0=`$0A`, C1=`$0F`, **C2=`$16` (Rojo)**, **C3=`$30` (Blanco)** |
| **`$01`** | **Bolsa 300 pts** (`300 pt bag`) | **`0x4E`** (`data.asm:890`) | **`$DE, $DF, $F6, $F7`** (`data.asm:1628`, offset `$138`) | Byte `19` (`$13` = `$FF`): `Attrib 3, 3, 3, 3 ; 4C` (`data.asm:618`) | **Subpaleta `3`** (`11b` $\rightarrow$ **`$FF`**) | C0=`$0A`, C1=`$0F`, **C2=`$16` (Rojo)**, **C3=`$30` (Blanco)** |
| **`$02`** | **Bolsa 1000 pts** (`1000 pt bag`) | **`0x4F`** (`data.asm:891`) | **`$DE, $DF, $F8, $F9`** (`data.asm:1629`, offset `$13C`) | Byte `19` (`$13` = `$FF`): `Attrib 3, 3, 3, 3 ; 4C` (`data.asm:618`) | **Subpaleta `3`** (`11b` $\rightarrow$ **`$FF`**) | C0=`$0A`, C1=`$0F`, **C2=`$16` (Rojo)**, **C3=`$30` (Blanco)** |
| **`$03`** | **Moneda de Oro** (`gold coin`) | **`0x47`** (`data.asm:892`) | **`$78, $79, $7A, $7B`** (`data.asm:1621`, offset `$11C`) | Byte `17` (`$11` = `$A8`): `Attrib 2, 2, 2, 0 ; 44` (`data.asm:616`) | **Subpaleta `0`** (`00b` $\rightarrow$ **`$00`**) | C0=`$0A`, C1=`$0F`, **C2=`$26` (Naranja/Oro)**, **C3=`$30` (Blanco)** |
| **`$04`** | **Mighty Drink** (`mighty drink`) | **`0x49`** (`data.asm:893`) | **`$3C, $3D, $3E, $3F`** (`data.asm:1623`, offset `$124`) | Byte `18` (`$12` = `$30`): `Attrib 0, 3, 0, 0 ; 48` (`data.asm:617`) | **Subpaleta `3`** (`11b` $\rightarrow$ **`$FF`**) | C0=`$0A`, C1=`$0F`, **C2=`$16` (Rojo)**, **C3=`$30` (Blanco)** |
| **`$05`** | **Bomba** (`bomb`) | **`0x27`** (`data.asm:894`) | **`$E0, $E1, $E2, $E3`** (`data.asm:1589`, offset `$9C`) | Byte `9` (`$09` = `$57`): `Attrib 1, 1, 1, 3 ; 24` (`data.asm:608`) | **Subpaleta `3`** (`11b` $\rightarrow$ **`$FF`**) | C0=`$0A`, C1=`$0F`, **C2=`$16` (Rojo)**, **C3=`$30` (Blanco)** |
| **`$06`** | **Esfinge** (`sphinx`) | **`0x34`** (`data.asm:895`) | **`$00, $01, $02, $03`** (`data.asm:1602`, offset `$D0`) | Byte `13` (`$0D` = `$00`): `Attrib 0, 0, 0, 0 ; 34` (`data.asm:612`) | **Subpaleta `0`** (`00b` $\rightarrow$ **`$00`**) | C0=`$0A`, C1=`$0F`, **C2=`$26` (Naranja)**, **C3=`$30` (Blanco)** |
| **`$07`** | **Placa Tecmo** (`tecmo plate`) | **`0x48`** (`data.asm:896`) | **`$76, $77, $FA, $FB`** (`data.asm:1622`, offset `$120`) | Byte `18` (`$12` = `$30`): `Attrib 0, 3, 0, 0 ; 48` (`data.asm:617`) | **Subpaleta `0`** (`00b` $\rightarrow$ **`$00`**) | C0=`$0A`, C1=`$0F`, **C2=`$26` (Naranja)**, **C3=`$30` (Blanco)** |
| **`$08`** | **Moneda Mighty** (`mighty coin`) | **`0x8F`** (`data.asm:897`) | **`$08, $09, $3A, $3B`** (`data.asm:1693`, offset `$23C`) | Byte `35` (`$23` = `$80`): `Attrib 2, 0, 0, 0 ; 8C` (`data.asm:634`) | **Subpaleta `0`** (`00b` $\rightarrow$ **`$00`**) | C0=`$0A`, C1=`$0F`, **C2=`$26` (Naranja)**, **C3=`$30` (Blanco)** |
| **`$0F`** | **Vacío (Recogido)** (`nothing`) | **`0x2F`** (`data.asm:904`) | **`$8A, $8A, $8A, $8A`** (`data.asm:1597`, offset `$BC`) | Byte `11` (`$0B` = `$AB`): `Attrib 2, 2, 2, 3 ; 2C` (`data.asm:610`) | **Subpaleta `3`** (`11b` $\rightarrow$ **`$FF`**) | 4 tiles `$8A` de índice 0 = Backdrop puro |

### 2.3 Por qué `src/core/mjk_game.c` Renderiza los Cofres con el Color Incorrecto

En [`src/core/mjk_game.c:577-605`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L577-L605) (Paso `3b`) y [`src/core/mjk_game.c:674-700`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L674-L700) (Paso `3b-right` de costura horizontal):

```c
// src/core/mjk_game.c:585-598 (CÓDIGO ACTUAL CON DEFECTO)
uint8_t tl, tr, bl, br;
uint8_t sub = 2;
if (!game->chests[c].is_open) {
    if (game->chests[c].chest_type == 1) {
        tl = 0x99; tr = 0x9B; bl = 0xEA; br = 0xEB; // Naranja cerrado ($4A)
    } else {
        tl = 0xE8; tr = 0xE9; bl = 0xEA; br = 0xEB; // Rojo cerrado ($28)
    }
} else {
    uint8_t m_id = g_item_to_tile_table[game->chests[c].item_id & 0x1F];
    MjkMetatileDef def = (m_id < MJK_METATILES_COUNT) ? g_metatile_definitions[m_id] : (MjkMetatileDef){0x24, 0x24, 0x24, 0x24};
    tl = def.tl; tr = def.tr; bl = def.bl; br = def.br;
    sub = MjkRoomData_GetMetatileSubpalette(m_id);
}
```

- Cuando el cofre está cerrado (`!game->chests[c].is_open`), `sub` permanece en **`2`** (`Subpaleta 2`, que en el Set 0 de las Salas 1..6 tiene el color gris `$10`), en vez de llamar a `MjkRoomData_GetMetatileSubpalette(0x4A)` (`0` = Naranja `$26`) para cofres naranjas y `MjkRoomData_GetMetatileSubpalette(0x28)` (`3` = Rojo `$16`) para cofres rojos.
- Además, en [`src/core/mjk_state_machine.c:167`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L167) y [`src/core/mjk_room_decoder.c:723`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c#L723), se clasifica `hi == 0x0A` como cofre naranja (`chest_type = 1` / `0x11`), pero en `JT_94AD_multi` ([`asm/src/prg.asm:3404-3417`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3404-L3417)) solo `CMP #$30` (`hi == 3`) y `CMP #$40` (`hi == 4`) incrementan `X` de `$10` a `$11`; los cofres ocultos `$9x` y `$Ax` reciben `$10 | $20 = $30` y al revelarse en `MaybeOpenHiddenBlock` ([`asm/src/prg.asm:5400-5404`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5400-L5404): `LDA a:byte_313; AND #$20; BEQ loc_A0FD; LSR A; BNE loc_A10F`) producen siempre `$10` (`Metatile 0x28`, Cofre Rojo).

---

## 3. Ground-Truth 6502: Solidez, Comportamiento y Colisiones con Jack (`[CONFIRMADA]`)

### 3.1 Separación Arquitectónica entre `sub_9B6A` (Escenario Estático 4-Tier) y `BigBeefySubHandlesBGColliding` (Objetos Dinámicos de `RoomDataRAM`)

En [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm), el motor de colisiones opera en dos capas complementarias dentro de `HandleSpawnsAndMore` ([`prg.asm:3871-3900`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3871-L3900)) y `sub_9965` ([`prg.asm:4152-4226`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4152-L4226)):

1. **Capa 1 — Geometría Estática de Escenario (`HandleDrawHalfRow` + `sub_9B6A`, [`prg.asm:4311-4312, 4441-4442, 4480-4610`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4480-L4610))**:
   - Evalúa las 9 sondas de `byte_A3D8` ([`prg.asm:5863-5872`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5863-L5872)) leyendo `(off_4),Y` directamente desde `LayoutChunks` en ROM (nunca lee la Nametable de la PPU).
   - En `LayoutChunks`, las celdas donde luego se ubican bombas, cofres o ítems contienen el metatile de aire `0x2F`.
   - En `loc_9BBB` / `loc_9BEB` ([`prg.asm:4532-4578`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4532-L4578)):
     - `0x00`, `0x27`, `0x2F`: pasables (`LDA #0`).
     - `0x5C..0x62`: fuego letal en Sonda 0 (`byte_1 == 0`).
     - `< 0x38` o `>= 0x4C`: 100% sólidos (`$FF`).
     - `0x38..0x4B`: máscara de 4 cuadrantes 8×8 en `byte_A3EA` ([`prg.asm:5873-5883`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5873-L5883)).

2. **Capa 2 — Objetos Dinámicos de `RoomDataRAM` (`sub_9B39` $\rightarrow$ `BigBeefySubHandlesBGColliding`, [`prg.asm:4448-4474, 4615-4856`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4615-L4856))**:
   - Se ejecuta cuando `byte_1 == 0` (Sonda 0, centro de Jack `(PlayerX_Lo, PlayerY_Lo)` = `(jx + 8 + scroll_x, jy + 8 + scroll_y)`).
   - Calcula **3 coordenadas de celda Metatile `$YX`** a partir del centro de Jack:
     - **`byte_A` (Centro)** ([`prg.asm:4616-4625`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4616-L4625)):
       $$\text{byte\_A} = (\text{CenterY} \ \& \ \text{0xF0}) \mid (\text{CenterX} \gg 4)$$
     - **`byte_B` (Borde Vertical en la Columna Central)** ([`prg.asm:4626-4656`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4626-L4656)):
       - Si `PlayerYVelHi >= 0` (cayendo o en reposo): usa `CenterY + 8` (planta de los pies `jy + 16`).
       - Si `PlayerYVelHi < 0` (ascendiendo): usa `CenterY - 8` (coronilla `jy`).
       - Mantiene la **misma columna central** que `byte_A` (`CenterX >> 4`):
         $$\text{byte\_B} = ((\text{CenterY} \pm 8) \ \& \ \text{0xF0}) \mid (\text{CenterX} \gg 4)$$
     - **`byte_E` (Borde Horizontal en la Fila Central)** ([`prg.asm:4658-4700`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4658-L4700)):
       - Según la dirección horizontal actual/última en `byte_EE` (`JP_Right` $\rightarrow +8$, `JP_Left` $\rightarrow -8$), mantiene la **misma fila central** que `byte_A` (`CenterY & 0xF0`):
         $$\text{byte\_E} = (\text{CenterY} \ \& \ \text{0xF0}) \mid ((\text{CenterX} \pm 8) \gg 4)$$
   - Recorre los objetos de la sala con `sub_9592` ([`prg.asm:4732-4812`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4732-L4812)):
     - Todo objeto con `(MaybeCollectedThing & $1F) >= $10` (es decir: `$10` Cofre Rojo cerrado, `$11` Cofre Naranja cerrado, `$12` Puerta, `$13` Bloque Secreto) **genera bits de colisión sólida en `byte_312`**:
       - Si coincide con **`byte_B` y `PlayerYVelHi >= 0`** (Jack encima del cofre): incrementa **`byte_310 = 1`** y activa **`byte_312 |= $30`** (bits de suelo de las sondas 3 y 4) ([`prg.asm:4783-4785`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4783-L4785)).
       - Si coincide con **`byte_B` y `PlayerYVelHi < 0`** (Jack golpeando desde abajo): deja **`byte_310 = 0`** y activa **`byte_312 |= $C0`** (bits de techo de las sondas 1 y 2) ([`prg.asm:4780-4782`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4780-L4782)).
       - Si coincide con **`byte_E`** (Jack chocando lateralmente): deja **`byte_310 = 0`** y activa **`byte_312 |= $03`** (muro derecho, sondas 7 y 8) o **`byte_312 |= $0C`** (muro izquierdo, sondas 5 y 6) ([`prg.asm:4798-4811`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4798-L4811)).
     - Finalmente, en `sub_9965` ([`prg.asm:4185-4187`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4185-L4187)), `byte_312` se combina mediante `ORA` con `byte_30F` (`LDA byte_30F; ORA a:byte_312; STA byte_30F`), haciendo que los cofres cerrados actúen como **bloques sólidos 16×16** para Jack (y mediante `byte_320`, [`prg.asm:4502-4529`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4502-L4529), también para los enemigos terrestres).
     - En cambio, cualquier ítem revelado al abrir un cofre tiene `RoomDataRAM` en `$00..$0E` (`(MaybeCollectedThing & $1F) < $10`), por lo que salta en `CMP #$10; BCC loc_9D88` ([`prg.asm:4778-4779, 4801-4802`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4778-L4779)) y **jamás es sólido**.

### 3.2 Reglas Exactas de Apertura de Cofre Rojo (`0x28`) vs Cofre Naranja (`0x4A`) Según Dirección y `PlayerMightyLevel`

Al despachar `JTPickup_Chest` (`$10` Cofre Rojo y `$13` Bloque Secreto, [`prg.asm:5259-5296`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5259-L5296)) y `JTPickup_Chest2` (`$11` Cofre Naranja, [`prg.asm:5300-5308`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5300-L5308)):

```asm
; asm/src/prg.asm:5259-5308
JTPickup_Chest:
	LDAc MaybeCollectedThing
	AND #$20
	BEQ loc_A01F
	LDA byte_310
	BNE loc_A01F
	LDA #0
	STA byte_312
	RTS
loc_A01F:
	LDAc byte_70
	STA a:byte_F0
	LDAc byte_5C
	STA a:_MaybeBlockSpawningObj
	LDAc byte_5A
	STA a:byte_74
	LDA a:byte_F4
	STA byte_311
	LDAc MaybeCollectedThing
	STA byte_313
	LDA byte_310
	BNE locret_A05B
	LDA #0
	STA a:byte_F0
	LDA a:MaybeCollectedThing
	AND #$20
	BNE locret_A05B
	LDA PlayerMightyLevel
	CMP #2
	BCC locret_A05B
	LDAc byte_70
	JSR MaybeOpenHiddenBlock
locret_A05B:
	RTS

JTPickup_Chest2:
	LDA PlayerMightyLevel
	BEQ locret_A064
	JMP JTPickup_Chest
locret_A064:
	RTS
```

| Tipo de Cofre | Estado Oculto (`$20`) | Interacción desde ARRIBA (`byte_310 == 1`, `vel_y >= 0`) | Interacción desde ABAJO (`byte_B`, `vel_y < 0`) o LATERAL (`byte_E`) |
| :--- | :--- | :--- | :--- |
| **Cofre Rojo Visible (`$10` / `0x28`)** | Visible (`bit 5 == 0`) | **Se abre en todos los niveles (`MightyLevel 0, 1, 2, 3`)** | - **`MightyLevel 0, 1`**: **NO se abre**; mantiene `byte_312` activo (**techo sólido** `$C0` o **muro lateral sólido** `$03/$0C`).<br>- **`MightyLevel 2, 3`**: **SÍ se abre** (`MaybeOpenHiddenBlock` $\rightarrow$ `sub_A18E` limpia `byte_312 = 0`). |
| **Cofre Rojo Oculto (`$30` / `$9x`)** | Oculto (`bit 5 == 1`) | - **1er salto encima**: Lo revela a `$10` (`Metatile 0x28` cerrado) en todos los niveles (`0..3`).<br>- **2do salto encima**: Abre el cofre revelado. | **Inmune y pasante en todos los niveles (`0..3`)**: `prg.asm:5260-5267` limpia `byte_312 = 0` y retorna sin revelarlo ni bloquear a Jack. |
| **Cofre Naranja Visible (`$11` / `0x4A`)** | Visible (`bit 5 == 0`) | - **`MightyLevel 0`**: **Inmune y sólido** (`JTPickup_Chest2` retorna en `BEQ locret_A064`, manteniendo `byte_312 = $30` como plataforma sólida sin abrirse).<br>- **`MightyLevel 1, 2, 3`**: **SÍ se abre**. | - **`MightyLevel 0`**: **Inmune y sólido** (techo sólido / muro lateral sólido).<br>- **`MightyLevel >= 1` (en `test_chests_secrets.c:231-236` `can_open_orange_chest`) / `>= 2` lateral**: Se abre si cumple el nivel requerido; si no, bloquea como techo/muro sólido. |

### 3.3 Recolección del Ítem Revelado (`MaybePickupItem`, `prg.asm:4813-4893`)

En [`asm/src/prg.asm:4814-4821`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4814-L4821):

```asm
loc_9D88:
	LDAc MaybeCollectedThing
	AND #$1F
	CMP #$F
	BCS loc_9D99
	LDX byte_A
	CPXc byte_70
	BEQ loc_9D99
	RTS
```

- Un ítem revelado (`0x00..0x0E`) **solo se recoge cuando `byte_A == byte_70`**, es decir, cuando el **centro geométrico de Jack `(jx + 8, jy + 8)`** entra dentro de los 16×16 píxeles de la celda del ítem:
  $$\text{screen\_cx} \le \text{jx} + 8 < \text{screen\_cx} + 16 \quad \land \quad \text{screen\_cy} \le \text{jy} + 8 < \text{screen\_cy} + 16$$

---

## 4. Tabla Comparativa Exhaustiva: `asm/src/prg.asm` vs `src/core/` (`[CONFIRMADA]`)

| # | Subsistema / Archivo y Línea | Comportamiento Actual en C (`src/core/`) | Ground-Truth 6502 (`asm/src/prg.asm` / `data.asm`) | Impacto del Defecto |
| :---: | :--- | :--- | :--- | :--- |
| **1** | [`src/core/mjk_game.c:586-593`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L586-L593) y [`mjk_game.c:681-688`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L681-L688) (`RenderInGameScreen`) | Fija `uint8_t sub = 2;` para todos los cofres cerrados (`!is_open`). | `TileAttributeTable` ([`data.asm:609, 617`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L609)): Cofre Rojo `0x28` usa **Subpaleta `3`** (`MjkRoomData_GetMetatileSubpalette(0x28) == 3`); Cofre Naranja `0x4A` usa **Subpaleta `0`** (`MjkRoomData_GetMetatileSubpalette(0x4A) == 0`). | Todos los cofres cerrados se ven **grises** en lugar de **Rojos** y **Naranjas**. |
| **2** | [`src/core/mjk_game.c:277-282`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L277-L282) y [`mjk_game.c:286-303`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L286-L303) (`MjkGame_UpdateChestsAndSecrets`) | Recoge el ítem si `jy + 16 >= screen_cy && jy <= screen_cy + 16`, y al abrir desde arriba no alinea `pos_y = FIXED_FROM_INT(screen_cy - 16)`. | `loc_9D88` ([`prg.asm:4818-4820`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4818-L4820)): `MaybePickupItem` exige `byte_A == byte_70` (centro `(jx+8, jy+8)` dentro de `[screen_cx..screen_cx+15] × [screen_cy..screen_cy+15]`). | En el frame siguiente a abrir el cofre desde arriba (`Frame N+1`), `jy + 16` sigue en `screen_cy + 1..5`, por lo que **el ítem se recoge instantáneamente en 16 ms sin llegar a verse jamás**. |
| **3** | [`src/core/mjk_player.c:335-352`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L335-L352) (`mjk_player_is_tile_solid`) | Solo ignora `{ 0x00, 0x27, 0x2F }`. Trata `0x28` y los ítems `0x4D..0x50, 0x34, 0x63, 0x8F, 0x90, 0x91` como bloques sólidos 16×16 y `0x47..0x4A` como bloques parciales 8×8. | `sub_9B6A` ([`prg.asm:4480-4610`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4480-L4610)) solo evalúa los chunks estáticos de `LayoutChunks`. Los cofres e ítems de `RoomDataRAM` se evalúan en `BigBeefySubHandlesBGColliding` ([`prg.asm:4615-4856`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4615-L4856)). | 1) Cuando un cofre se abre y escribe `0x4D` / `0x8F` en `room_metatiles`, el ítem actúa como un **muro sólido**.<br>2) Cuando un cofre oculto se revela a `0x28` en `room_metatiles`, `Jack_Update` pone `vel_y = 0` antes de `MjkGame_UpdateChestsAndSecrets`, haciendo que **el cofre revelado no pueda abrirse nunca**. |
| **4** | [`src/core/mjk_game.c:274, 342`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L274) (`MjkGame_UpdateChestsAndSecrets`) | `bool h_overlap = (jx + 12 >= screen_cx && jx + 4 <= screen_cx + 16);` (25 píxeles de ancho). | `byte_A` y `byte_B` ([`prg.asm:4619-4625, 4642-4645`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4619-L4625)) evalúan la columna del centro de Jack: `jx + 8 >= screen_cx && jx + 8 < screen_cx + 16` (16 píxeles exactos). | Jack activa cofres y recoge ítems estando desplazado hasta 12 píxeles fuera del cofre. |
| **5** | [`src/core/mjk_game.c:290-300`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L290-L300) (`MjkGame_UpdateChestsAndSecrets`) | Al caer sobre un Cofre Naranja con `PlayerMightyLevel == 0`, hace rebotar a Jack (`vel_y = -3`) y reproduce `SFX_COLLECT_BOMB`. | `JTPickup_Chest2` ([`prg.asm:5300-5307`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5300-L5307)) retorna manteniendo `byte_312 = $30` (`sub_A1EB`): Jack **aterriza y permanece de pie sobre el cofre naranja como bloque sólido** sin reproducir sonido. | El cofre naranja a Nivel 0 actúa como un trampolín sonoro infinito en vez de un bloque sólido. |
| **6** | [`src/core/mjk_game.c:304-325`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L304-L325) (`MjkGame_UpdateChestsAndSecrets`) | 1) Si golpea un cofre visible desde abajo sin poder abrirlo (`!can_open_below`), no detiene el ascenso de Jack.<br>2) No existe colisión lateral ni apertura lateral con `PlayerMightyLevel >= 2`. | 1) `loc_9D43` ([`prg.asm:4780-4782`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4780-L4782)) activa `byte_312 |= $C0` (`sub_A1B9` techo sólido) cuando el cofre no se abre.<br>2) `loc_9D5A..loc_9D82` y `JTPickup_Chest` ([`prg.asm:4787-4812, 5288-5292`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4787-L4812)) abren el cofre lateralmente si `PlayerMightyLevel >= 2`, o bloquean a Jack como muro lateral (`$03/$0C`) si `< 2`. | Jack atraviesa los cofres cerrados lateralmente y desde abajo cuando no tiene el poder Mighty necesario. |

---

## 5. Especificación Quirúrgica de Corrección para `c_implementer` (`TASK-F7-004`)

### 5.1 Corrección en [`src/core/mjk_player.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c) (`mjk_player_is_tile_solid`, líneas 335-339)

En [`src/core/mjk_player.c:335-339`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_player.c#L335-L339), excluir de la solidez estática de 9 sondas los metatiles de objetos dinámicos de `ItemToTileTable` (cofres `0x28`, `0x4A` e ítems revelados `0x34`, `0x47`, `0x48`, `0x49`, `0x4C`, `0x4D`, `0x4E`, `0x4F`, `0x50`, `0x63`, `0x8F`, `0x90`, `0x91`), ya que en la NES (`sub_9B6A` vs `BigBeefySubHandlesBGColliding`) el mapa estático 4-tier tiene `0x2F` en dichas celdas y toda su colisión/apertura se resuelve en `MjkGame_UpdateChestsAndSecrets`:

```c
    // 1. Casos Especiales Pasantes (Vacío y Objetos Dinámicos de ItemToTileTable)
    // En NES (prg.asm:4480-4610 vs 4615-4856), sub_9B6A solo evalúa LayoutChunks estáticos
    // mientras que cofres e ítems de RoomDataRAM se gestionan en BigBeefySubHandlesBGColliding.
    if (tile_id == 0x00 || tile_id == 0x27 || tile_id == 0x2F ||
        tile_id == 0x28 || tile_id == 0x4A || tile_id == 0x34 ||
        (tile_id >= 0x47 && tile_id <= 0x50) ||
        tile_id == 0x63 || tile_id == 0x8F || tile_id == 0x90 || tile_id == 0x91) {
        return false;
    }
```

*(Nota: Esta lista no toca ningún metatile usado en `tests/test_player_physics.c` (`0x00, 0x27, 0x2F, 0x2D, 0x5D, 0x38, 0x3D, 0x46`) ni los muros/suelos de `tests/test_corridor_scroll.c`).*

### 5.2 Corrección de Subpaleta de Cofres Cerrados en [`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c) (`RenderInGameScreen`, líneas 585-598 y 680-693)

En [`src/core/mjk_game.c:585-598`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L585-L598) y [`src/core/mjk_game.c:680-693`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L680-L693), obtener la subpaleta canónica desde `MjkRoomData_GetMetatileSubpalette`:

```c
                uint8_t tl, tr, bl, br;
                uint8_t sub;
                if (!game->chests[c].is_open) {
                    uint8_t m_id = (game->chests[c].chest_type == 1) ? 0x4A : 0x28;
                    MjkMetatileDef def = g_metatile_definitions[m_id];
                    tl = def.tl; tr = def.tr; bl = def.bl; br = def.br;
                    sub = MjkRoomData_GetMetatileSubpalette(m_id); // 0x4A -> Sub 0 (Naranja), 0x28 -> Sub 3 (Rojo)
                } else {
                    uint8_t m_id = g_item_to_tile_table[game->chests[c].item_id & 0x1F];
                    MjkMetatileDef def = (m_id < MJK_METATILES_COUNT) ? g_metatile_definitions[m_id] : (MjkMetatileDef){0x24, 0x24, 0x24, 0x24};
                    tl = def.tl; tr = def.tr; bl = def.bl; br = def.br;
                    sub = MjkRoomData_GetMetatileSubpalette(m_id);
                }
```

### 5.3 Corrección de Colisiones e Interacción con Cofres en [`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c) (`MjkGame_UpdateChestsAndSecrets`, líneas 252-369)

Reemplazar la lógica de interacción con cofres en `MjkGame_UpdateChestsAndSecrets` ([`src/core/mjk_game.c:252-326`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L252-L326)) para implementar con exactitud `BigBeefySubHandlesBGColliding` ([`prg.asm:4615-4856`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4615-L4856)), `JTPickup_Chest` ([`prg.asm:5259-5296`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5259-L5296)) y `JTPickup_Chest2` ([`prg.asm:5300-5308`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5300-L5308)), preservando el 100% de compatibilidad con [`tests/test_chests_secrets.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_chests_secrets.c):

```c
        // Sonda central canónica byte_A / byte_B (prg.asm:4616-4645):
        // Columna del centro de Jack (jx + 8) dentro de [screen_cx, screen_cx + 16)
        int center_x = jx + 8;
        int center_y = jy + 8;
        bool h_overlap = (center_x >= screen_cx && center_x < screen_cx + 16);

        // A) Si el cofre ya está abierto pero el ítem no ha sido recogido:
        // En NES (loc_9D88, prg.asm:4818-4820), MaybePickupItem exige byte_A == byte_70,
        // es decir, el centro de Jack (jx + 8, jy + 8) dentro de la celda 16x16 del ítem.
        if (ch->is_open && !ch->collected) {
            if (h_overlap && center_y >= screen_cy && center_y < screen_cy + 16) {
                MjkGame_CollectChestItem(game, c);
            }
            continue;
        }

        if (ch->is_open) continue;

        // B) Salto / aterrizaje sobre la tapa superior (byte_B con PlayerYVelHi >= 0, prg.asm:4781-4785)
        if (h_overlap && jy + 16 >= screen_cy && jy + 16 <= screen_cy + 8 &&
            (game->jack.vel_y >= 0 && game->jack.subpixel_vy >= 0)) {

            if (ch->chest_type == 1 && !can_open_orange_chest(&game->jack)) {
                // JTPickup_Chest2 (prg.asm:5300-5307): Cofre naranja es un bloque sólido inmune a Nivel 0
                if (!ch->is_hidden) {
                    jy = screen_cy - 16;
                    game->jack.pos_y = FIXED_FROM_INT(jy);
                    game->jack.vel_y = 0;
                    game->jack.subpixel_vy = 0;
                    game->jack.jump_accumulator = 0;
                    game->jack.jump_step_index = 0;
                    game->jack.flutter_active = false;
                    game->jack.on_ground = true;
                    game->jack.state = JACK_STATE_ON_GROUND;
                    game->jack.collision_flags |= CF_FLOOR | CF_ON_GROUND;
                }
            } else if (game->jack.vel_y > 0 || game->jack.subpixel_vy > 0) {
                // Alinear pos_y exactamente sobre el tope del cofre (screen_cy - 16) antes de despegar,
                // evitando que en el frame siguiente el centro de Jack consuma el ítem recién revelado
                jy = screen_cy - 16;
                game->jack.pos_y = FIXED_FROM_INT(jy);
                game->jack.vel_y = -3;
                game->jack.subpixel_vy = -768;
                game->jack.jump_accumulator = 0x2000;
                game->jack.jump_step_index = 32;
                game->jack.state = JACK_STATE_JUMPING;
                game->jack.on_ground = false;

                MjkGame_OpenChest(game, c);
            }
        }
        // C) Golpe con la cabeza desde abajo (byte_B con PlayerYVelHi < 0, prg.asm:4780-4782)
        else if (h_overlap && jy <= screen_cy + 16 && jy >= screen_cy + 8 &&
                 (game->jack.vel_y < 0 || game->jack.subpixel_vy < 0)) {

            // Cofres ocultos ($30/$31) son intangibles desde abajo (prg.asm:5260-5267)
            if (!ch->is_hidden) {
                bool can_open_below = (ch->chest_type == 1)
                                    ? can_open_orange_chest(&game->jack)
                                    : can_headbutt_red_chest(&game->jack);
                if (can_open_below) {
                    MjkGame_OpenChest(game, c);
                } else {
                    // Si no tiene el nivel Mighty requerido, el cofre cerrado bloquea como techo sólido (sub_A1B9)
                    jy = screen_cy + 16;
                    game->jack.pos_y = FIXED_FROM_INT(jy);
                }
                game->jack.vel_y = 0;
                game->jack.subpixel_vy = 0;
                game->jack.jump_accumulator = 0x4000;
                game->jack.state = JACK_STATE_FALLING;
                game->jack.ceiling_hit = true;
                game->jack.collision_flags |= CF_CEILING;
            }
        }
        // D) Colisión / apertura lateral (byte_E: fila central jy + 8 en [screen_cy, screen_cy + 16), prg.asm:4787-4812)
        else if (!ch->is_hidden && center_y >= screen_cy && center_y < screen_cy + 16) {
            // Borde derecho de Jack (jx + 16) o borde izquierdo (jx) entra en el cofre
            bool hit_from_left  = (jx + 16 > screen_cx && jx + 8 < screen_cx &&
                                   (game->jack.vel_x > 0 || game->jack.subpixel_vx > 0));
            bool hit_from_right = (jx < screen_cx + 16 && jx + 8 >= screen_cx + 16 &&
                                   (game->jack.vel_x < 0 || game->jack.subpixel_vx < 0));
            if (hit_from_left || hit_from_right) {
                // JTPickup_Chest (prg.asm:5288-5292): con PlayerMightyLevel >= 2 abre el cofre lateralmente
                if (can_headbutt_red_chest(&game->jack)) {
                    MjkGame_OpenChest(game, c);
                } else {
                    // Con PlayerMightyLevel < 2, actúa como muro sólido lateral (byte_312 |= $03 / $0C)
                    if (hit_from_left) {
                        jx = screen_cx - 16;
                        game->jack.collision_flags |= CF_RIGHT_WALL;
                    } else {
                        jx = screen_cx + 16;
                        game->jack.collision_flags |= CF_LEFT_WALL;
                    }
                    game->jack.pos_x = FIXED_FROM_INT(jx);
                    game->jack.vel_x = 0;
                    game->jack.subpixel_vx = 0;
                }
            }
        }
```

---

## 6. Verificación de Impacto en Tests Existentes (`ctest`)

- [`tests/test_chests_secrets.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_chests_secrets.c):
  - **TEST 1** (Cofre rojo visible, salto encima): `jx = 6*16` $\rightarrow$ `center_x = 6*16 + 8` (dentro de `[6*16, 6*16+16)`), `jy + 16 = 12*16 + 1`, `vel_y = 2 > 0` $\rightarrow$ abre cofre, `vel_y = -3 < 0`, `room_metatiles == 0x4D`. **PASA 100%**.
  - **TEST 2** (Cabezazo desde abajo en cofre rojo): Nivel 0 y Nivel 1 no abren (`is_open == false`, `vel_y = 0`); Nivel 2 abre (`is_open == true`, `room_metatiles == 0x4E`, `vel_y == 0 && state == JACK_STATE_FALLING`). **PASA 100%**.
  - **TEST 3** (Cofre rojo oculto): Inmune desde abajo a Nivel 3; 1er salto revela a `0x28`/`$10`; 2do salto abre a `0x8F`/`$08`. **PASA 100%**.
  - **TEST 4** (Cofre naranja): Nivel 0 arriba y abajo no abren (`is_open == false`); Nivel 1 (`MIGHTY_POWER_BLUE`) abre desde abajo y desde arriba (`is_open == true`, `room_metatiles == 0x4F`). **PASA 100%**.
  - **TEST 5, 6, 7**: Sin cambios. **PASAN 100%**.
  - **TEST 8** (Recolección de ítem expuesto): `pos_x = 6*16, pos_y = 12*16` $\rightarrow$ `center_x = 6*16 + 8`, `center_y = 12*16 + 8` (ambos dentro de `[6*16, 6*16+16) × [12*16, 12*16+16)`) $\rightarrow$ recoge Mighty Drink, `room_metatiles == 0x2F`. **PASA 100%**.
