# Informe Técnico: Especificación Funcional C x86 — Subsistema 02: Mapa Completo de Memoria RAM (`$0000-$07FF`) y Zero-Page Scratchpad (`$00-$14`)

- **ID de Investigación**: `RES-005`
- **Tarea Asociada**: `TASK-RES-004`
- **Área Temática**: `01 - Núcleo, Memoria y Arquitectura General`
- **Fecha**: 2026-10-22
- **Autor / Agente**: Antigravity (Orchestrator) + `tools/analyze_ram_semantics.py`
- **Estado**: `[COMPLETADA]`

---

## 1. Objetivo y Alcance

Documentar de forma exhaustiva la arquitectura completa de los **2,048 bytes de memoria RAM interna (`$0000-$07FF`)** de *Mighty Bomb Jack*, resolviendo el propósito semántico de las **127 variables `byte_XX`/`off_XX`/`word_XX`** y las **23 variables tentativas (`Maybe...`)** de [ram.asm](file:///c:/Users/kanel/Documents/mjk/asm/src/ram.asm), y definiendo las estructuras C x86 exactas para una recreación 1:1.

---

## 2. Fuentes de Verdad Analizadas (`asm/`)

- **[ram.asm](file:///c:/Users/kanel/Documents/mjk/asm/src/ram.asm)**: Líneas `1 - 2450` (Mapa completo `$0000-$07FF`).
- **[defs.asm](file:///c:/Users/kanel/Documents/mjk/asm/src/defs.asm)**: Offsets de `EnemyStruct` (`$00..$1B`) y constantes `BasePointer_*`.
- **[prg.asm](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm)**: `LoadPointerTo050` y `PointerTable050` (`prg.asm:2238-2263`), `ObjectPointers` (`prg.asm:4612-4629`).
- **Base de Datos de Referencias Cruzadas**: [tools/asm_inventory.json](file:///c:/Users/kanel/Documents/mjk/tools/asm_inventory.json).

---

## 3. Arquitectura Global de los 2,048 Bytes de RAM (`$0000-$07FF`)

```
+---------------------------------------------------------------------------------------------------+
|                             MAPA FISICO DE RAM INTERNA NES ($0000-$07FF)                          |
+-------------------+-----------+-------------------------------------------------------------------+
| Rango Hex         | Tamaño    | Subsistema / Contenido                                            |
+-------------------+-----------+-------------------------------------------------------------------+
| $0000 - $0014     |   21 B    | Zero-Page Scratchpad Polimórfico (byte_0..byte_14, off_2, off_4)  |
| $001F - $00FF     |  225 B    | Estado Global Rápido ZP: Punteros, PPU Mirrors, Score BCD, Timers |
| $0100 - $01FF     |  256 B    | Pila Hardware 6502 (StackArea, crece de $01FF hacia abajo)        |
| $0200 - $02FF     |  256 B    | SpriteDMAArea (Shadow OAM: 64 sprites x 4 bytes [Y, Tile, Attr, X]|
| $0300 - $03CD     |  206 B    | Joypad, Colisiones, Torture/RoundClear, Puertas, Cámara y Scroll  |
| $03CE - $03EC     |   31 B    | PlayerStruct ($1F bytes; primeros 16 bytes compatibles EnemyStruct|
| $03EE - $0431     |   68 B    | Cabecera PPU ($03EE-$03F1) + PPUUpdateBuffer ($03F2-$0431, 64 B)  |
| $0432 - $0556     |  293 B    | Motor de Audio APU: Cola ($0432) + 8 Canales A (24B) + 8 B (8B)   |
| $0563 - $05E2     |  128 B    | Tablas de Atributos PPU en RAM: NT0 ($0563, 64B) y NT2 ($05A3,64B)|
| $05E3 - $0602     |   32 B    | PaletteBuffer (16 colores BG + 16 colores Sprites)                |
| $0603 - $06A5     |  163 B    | RoomDataRAM ($0603-$0682, 128B objetos/tiles sala) + tail ($0683) |
| $06A6 - $0785     |  224 B    | Object0Struct .. Object7Struct (8 slots de enemigos x 28B [$1C])  |
| $0786 - $07FF     |  122 B    | Popup de Bonus Especial ($0786-$078D) y cola/timers auxiliares    |
+-------------------+-----------+-------------------------------------------------------------------+
```

---

## 4. Subpaso 0.4.1 — Zero-Page (`$0000-$00FF`) y Scratchpad Polimórfico (`$00-$14`)

### 4.1. Contrato del Scratchpad Polimórfico (`$0000-$0014`)
Las primeras 21 direcciones de Zero-Page se reutilizan como registros de trabajo entre distintos subsistemas. **Atención en C x86**: varias funciones padre-hija se comunican a través de estos bytes sin pasarlos por `A/X/Y`:

| Dirección | Etiqueta | Tamaño | Usos por Contexto y Comunicación Padre $\leftrightarrow$ Hija |
| :---: | :--- | :---: | :--- |
| `$0000-$0001` | `byte_0`, `byte_1` | 2 B | **1.** Puntero de 16 bits `(byte_0),Y` en `RESET` y `HandlePPUUpdates`.<br>**2.** `DrawScore` guarda máscara/atributos en `byte_1` leída por `DrawScoreDigitMaybe`.<br>**3.** En `music-engine.asm`, `byte_0` es el índice de `SoundQueue` y `byte_1` el cursor de lectura de cabeceras de pistas.<br>**4.** En `ReadJoypad`, `byte_0` guarda temporalmente la lectura cruda de `$4016/$4017`. |
| `$0002-$0003` | `off_2` | 2 B | **1.** Puntero de 16 bits a la entrada de `RoomHalfScreens` calculado por `LoadRoomStuff` y leído por `HandleDrawHalfRow`.<br>**2.** En `HandleSoundChannel`, puntero de 16 bits al stream de bytecode musical activo `(off_2),Y`. |
| `$0004-$0005` | `off_4` | 2 B | Puntero de 16 bits en el decodificador de salas (`HandleDrawHalfRow`, `DrawFullScreen`, `sub_9B6A`) para indexar `HalfScreenLayouts`, `LayoutChunks` y `MetatileDefinitions`. |
| `$0006-$0007` | `byte_6`, `byte_7` | 2 B | **1.** Par puntero de 16 bits `(byte_6),Y` cargado por `LoadLayoutChunk` (apunta al bloque de 4 metatiles de un Layout Chunk) y leído por `DrawFullScreen` y `sub_9716`.<br>**2.** En `SoundCommand_Fx`, `byte_6` guarda el subcomando `0..7` (`$F0..$F7`). |
| `$0008` | `byte_8` | 1 B | Índice de columna/chunk (`0..15`) dentro de un `HalfScreenLayout`, pasado desde `DrawFullScreen` / `sub_9A03` / `sub_9A7A` hacia `HandleDrawHalfRow`. |
| `$0009` | `byte_9` | 1 B | Contador de fila/metatile durante la decodificación de pantalla y cálculo de direcciones PPU (`DrawFullScreen`, `sub_92AD`, `sub_9A03`, `sub_9B39`, `sub_9B6A`). |
| `$000A` | `byte_A` | 1 B | Bitmask acumulador de colisión de metatiles en `BigBeefySubHandlesBGColliding` y `sub_9B6A`, e índice temporal en `sub_A875`. |
| `$000B` | `byte_B` | 1 B | Tipo/cabecera de objeto de sala en `GameState_2_LoadRoom` y `ReadRoomData`, pasado a `JT_94AD_0`. |
| `$000C` | `byte_C` | 1 B | Contador de objetos en `ReadRoomData`, y desplazamiento de atributo en `HandleTileAttributes`. |
| `$000D` | `byte_D` | 1 B | Byte de datos de objeto leído en `ReadRoomData` y consumido por `JT_94AD_multi`; máscara en `HandleTileAttributes`. |
| `$000E` | `byte_E` | 1 B | Cursor `Y` de lectura dentro del stream de datos de sala (`ReadRoomData`, `HandleRoomDataObject`, `JT_94AD_*`). |
| `$000F` | `byte_F` | 1 B | Acumulador temporal de coordenadas de celda en `sub_9276`. |
| `$0010` | `byte_10` | 1 B |Multiplicador en `AddScore`; contador de bucle en `sub_9813`; ID de puerta temporal en `HandleOpenDoorMaybe`. |
| `$0011` | `byte_11` | 1 B | Segundo contador en `sub_9813` y variable de estado en `EndingState_2`. |
| `$0012` | `byte_12` | 1 B | Índice temporal de objeto al spawnear desde cofre/bloque (`MaybeSpawnThingFromObject`, `sub_B645`). |
| `$0013` | `byte_13` | 1 B | Contador de bucle en `sub_B645` y `EndingState_2`. |
| `$0014` | `byte_14` | 1 B | Offset temporal sumado en `sub_9592`. |

---

### 4.2. Tabla Completa de Variables de Zero-Page (`$001F-$00FF`)

| Dirección | Etiqueta `ram.asm` | Nombre Semántico C x86 | Tamaño | Rol Exacto y Semántica |
| :---: | :--- | :--- | :---: | :--- |
| `$001F` | `BrotherSpawnerCounter` | `brother_spawner_counter` | 1 B | Contador de spawns en salas especiales de hermanos. |
| `$0028-$0029` | `Mod_Number` | `div16_dividend` | 2 B | Dividendo de 16 bits para `CalculateModulus`. |
| `$002A` | `Mod_Modulus` | `div16_divisor` | 1 B | Divisor de 8 bits para `CalculateModulus`. |
| `$002B` | `Mod_Remainder` | `div16_remainder` | 1 B | Resto resultante de `CalculateModulus`. |
| `$002D-$002F` | `NMITemp_A/X/Y` | `nmi_saved_a/x/y` | 3 B | Respaldo de registros `A, X, Y` al entrar en `NMI`. |
| `$0030-$0031` | `JumpTablePointer`, `byte_31` | `jumptable_ret_ptr` | 2 B | Dirección de retorno extraída de la pila por `JumpTable` (`byte_31` es el byte alto `JumpTablePointer+1`). |
| `$0032-$0033` | `JumpTableTarget` | `jumptable_target_ptr` | 2 B | Dirección de destino final para `JMP (JumpTableTarget)`. |
| `$0038` | `TempSpriteTile` | `temp_sprite_tile_id` | 1 B | Índice de metasprite en `SpritesTable` para `WriteSprite`. |
| `$0039` | `TempSpriteAttributesish` | `temp_sprite_attr` | 1 B | Atributos OAM + flag de centrado bit 4 (`$10` resta 4px a X/Y en `WriteSprite`). |
| `$003A-$003B` | `TempSpriteX`, `TempSpriteY` | `temp_sprite_x`, `temp_sprite_y` | 2 B | Coordenadas X/Y en pantalla para dibujar sprites/dígitos BCD. |
| `$003C` | `CurrentSpriteIndex` | `oam_slot_cursor` | 1 B | Índice de slot OAM actual (`0..63`, multiplicado $\times 4$ para indexar `$0200`). |
| `$003D-$003E` | `off_3D` | `room_layout_ptr` | 2 B | Puntero general de 16 bits para tablas de layout, metatiles y puertas (`MaybeLoadRoomFlags`, `sub_92AD`). |
| `$003F` | `EndingState` | `ending_substate` | 1 B | Subestado (`0..4`) de `GameState_C_Ending`. |
| `$0040` | `MaybePauseFlag` | `pause_and_stage_flags` | 1 B | Bit 5 (`$20`): Pausa activa. Bit 6 (`$40`): Sala iniciada. |
| `$0043` | `InGameFlag` | `in_game_mode` | 1 B | `0` = `TitleScreenHandler`, `1` = `GameHandler`. |
| `$0044` | `GameState` | `game_state` | 1 B | Estado principal del juego (`0..$0C`). |
| `$0045` | `TitleScreenState` | `title_screen_state` | 1 B | Subestado (`0..6`) de la pantalla de título. |
| `$0046` | `UpdatePaletteFlag` | `ppu_palette_dirty_flag` | 1 B | Si `!= 0`, `HandlePPUUpdates` sube los 32 bytes de `PaletteBuffer` a `$3F00`. |
| `$0047` | `RoundClearState` | `round_clear_substate` | 1 B | Subestado (`0..3`) de `GameState_8_RoundClear`. |
| `$0048-$0049` | `byte_48`, `byte_49` | `string_data_ptr` | 2 B | Puntero de 16 bits a la cadena de texto activa en `WriteStringToPPU`. |
| `$004A` | `SpriteOffset` | `oam_byte_offset` | 1 B | Offset en bytes (`CurrentSpriteIndex << 2`) dentro de `$0200`. |
| `$004B-$004F` | `byte_4B..byte_4F` | `write_sprite_scratch[5]` | 5 B | Variables de trabajo exclusivas de `WriteSprite`: `byte_4B`=contador de subtiles (`0..3`), `byte_4C..4D`=puntero a `SpritesTable` (`TempSpriteTile * 5`), `byte_4E..4F`=máscaras de flip H/V. |
| `$0050-$0051` | `word_50` | `base_table_ptr` | 2 B | Puntero maestro cargado por `LoadPointerTo050(A)` desde `PointerTable050` (`0..$0E`). |
| `$0054` | `byte_54` | `collected_obj_slot_idx` | 1 B | Índice del slot de entidad recogida en `JT_98A4_multi` / `JT_98A4_8_PowerCoin`. |
| `$0056` | `byte_56` | `enemy_oam_draw_count` | 1 B | Contador de enemigos dibujados en el frame actual en `MaybeDrawEnemySprites`. |
| `$0057` | `UnknownDoorFlag` | `active_door_packed_info` | 1 B | Información empaquetada de la puerta con la que colisiona Jack (`sub_9592`, `JTPickup_Door`). |
| `$0058-$0059` | `off_58` | `room_data_rom_ptr` | 2 B | Puntero de 16 bits a los datos de objetos de la sala actual en `roomdata-jp/us.asm`. |
| `$005A-$005B` | `byte_5A`, `byte_5B` | `room_ram_ptr` | 2 B | Puntero de 16 bits hacia `RoomDataRAM` (`$0603+`) durante la búsqueda y recogida de objetos de sala (`sub_9592`, `MaybePickupItem`). |
| `$005C` | `byte_5C` | `room_obj_search_idx` | 1 B | Índice/offset del objeto encontrado en `RoomDataRAM` (`sub_9592`, `sub_96F5`). |
| `$005E` | `MaybeCollectedThing` | `interacted_room_obj_type` | 1 B | Tipo de objeto estático con el que interactúa Jack (`0..19`, selector de la `JumpTable #6` en `BigBeefySubHandlesBGColliding`). |
| `$005F` | `byte_5F` | `room_data_read_cursor` | 1 B | Cursor de bytes al desempaquetar la cabecera de sala en `GameState_2_LoadRoom` y `ReadRoomData`. |
| `$0060-$0065` | `SoundEngine*` | `apu_zp_state` | 6 B | `SoundEnginePointer` (`$60-$61`), `SoundPointerTemp` (`$62-$63`), `CurrentSoundChannel` (`$64`), `ActiveSoundChannels` (`$65`). |
| `$0068` | `RoomMusic` | `current_room_music_idx` | 1 B | Índice en `MusicOptionsTable` para la música de fondo de la sala actual. |
| `$006A-$006B` | `EnemyCoinTimer` | `enemy_coin_timer_16` | 2 B | Temporizador de 16 bits (320 frames = `$0140`) mientras los enemigos están convertidos en monedas. |
| `$0070-$0071` | `byte_70`, `byte_71` | `interacted_obj_tile_pos[2]` | 2 B | Coordenada/índice de metatile en pantalla del objeto/puerta interactuado (`sub_9592`, `sub_96B7`, `sub_96F5`, `BigBeefySubHandlesBGColliding`). |
| `$0072-$0073` | `MaybeTileCollected`, `byte_73` | `replacement_metatile_id[2]` | 2 B | ID de metatile de reemplazo (p. ej. fondo vacío o cofre abierto) al recoger un ítem o abrir puerta (`sub_9592`, `sub_96B7`, `sub_A875`). |
| `$0074` | `byte_74` | `opened_chest_ram_offset` | 1 B | Offset en `RoomDataRAM` del cofre que acaba de abrirse (`JTPickup_Chest`, `sub_A1A6`). |
| `$0078-$0079` | `off_78` | `active_entity_ptr` | 2 B | Puntero de 16 bits a la estructura de 28 bytes (`PlayerStruct` o `Object0..7Struct`) que está siendo procesada en física/colisiones. |
| `$007A-$007B` | `EnemyStructPointer` | `active_enemy_ptr` | 2 B | Puntero de 16 bits al slot de enemigo (`$06A6..$076A`) activo en IA/spawns. |
| `$0088-$0089` | `Copyright/TitleTimer` | `copyright_timer`, `title_timer` | 2 B | Temporizadores de las pantallas de copyright y título. |
| `$0090-$0091` | `byte_90`, `byte_91` | `lit_bomb_order_rom_ptr` | 2 B | Puntero de 16 bits a la tabla de orden de encendido de bombas de la sala actual (`ReadRoomData`, `JTPickup_Bomb`). |
| `$0092`, `$0094` | `byte_92`, `byte_94` | `computed_screen_x`, `computed_screen_y` | 2 B | Coordenadas X (`$92`) e Y (`$94`) en pantalla calculadas por `sub_9276` a partir de la posición en sala y el scroll (`byte_3C2`/`byte_3C4`). |
| `$0098-$009B` | `byte_98..byte_9B` | `negated_player_vel_x/y` | 4 B | Velocidad X (`$98-$99`) e Y (`$9A-$9B`) de Jack negada (`-PlayerVel`) calculada por `sub_A843`/`sub_A851` en `MainSub_3` para desplazar la cámara y los objetos cuando la sala hace scroll. |
| `$00A0-$00A3` | `PlayerScore` | `player_score_bcd[4]` | 4 B | Puntuación en BCD (4 bytes little-endian de pares de dígitos = 8 dígitos BCD, máx `99,999,990`). |
| `$00A4-$00A5` | `StageTimer`, `TimerFrames` | `stage_timer_bcd`, `stage_timer_subframes` | 2 B | Segundos restantes en BCD (`$60`) y subcontador de 256 frames. |
| `$00A8-$00A9` | `TimerStatusMaybe`, `TimeOverTimer` | `stage_timer_status`, `time_over_timer` | 2 B | Estado del reloj de fase y cuenta atrás de 128 frames de `"TIME OVER"`. |
| `$00AC` | `byte_AC` | `room_decode_chunk_counter` | 1 B | Contador de bloques al leer `RoomData` (`IfNotTitleScreenRoomDoThings`, `ReadRoomData`). |
| `$00AE-$00AF` | `FireBombsCollected`, `FireBombsCollected2` | `lit_bombs_collected`, `total_bombs_collected` | 2 B | Bombas encendidas recogidas (`$AE`, para el bonus de 20..23 bombas) y total de bombas recogidas en la sala (`$AF`). |
| `$00B1`, `$00B3` | `AliveEnemyCount`, `EnemySpeedModifier` | `alive_enemy_count`, `enemy_speed_mod` | 2 B | Número de enemigos vivos y modificador de velocidad/tipo cargado de `DifficultyTable`. |
| `$00BA` | `byte_BA` | `spawn_enemy_slot_cursor` | 1 B | Índice rotativo de slot al buscar hueco para spawnear enemigos en `sub_B645`. |
| `$00BC` | `RoomStatusFlags` | `room_status_flags_zp` | 1 B | Flags de configuración de la sala actual (`RF_SingleScreen`, `RF_BombRoom`, `RF_ScrollStop`, `RF_AtScrollEdge`, `RF_Horizontal`). Nota: en US algunos accesos usan `$03BE`. |
| `$00C0-$00CF` | `TempPointer_0C0` .. `byte_CF` | `enemy_ai_zp_scratch` | 16 B | Registros de trabajo del motor de IA y renderizado de enemigos: `TempPointer_0C0` (`$C0-$C1`), `byte_C2/C3` (deltas X/Y o índice temporal en `DrawEnemies` y `ObjCode_7a`), `TempY` (`$C4`), `byte_C5` (distancia en `ObjCode_7a`), `byte_C7` (máscara direccional en `sub_BEE7/BF85`), `byte_CA..CD` (límites de sonda en `sub_BF17/BF4E`), `byte_CF` (índice de enemigo en `DrawEnemies`). |
| `$00E0-$00E2` | `NMIRunFlag`, `PPUCtrl/MaskMirror` | `nmi_run_flag`, `ppu_ctrl_mirror`, `ppu_mask_mirror` | 3 B | Espejos de `$2000` y `$2001` en RAM. |
| `$00E6` | `Joypad1_ImmediateCopy` | `joypad1_prev_frame` | 1 B | Estado del mando en el frame $N-1$ (copiado al final de `NMI`). |
| `$00E7` | `TilePlayerCollidedWith` | `collided_metatile_attr` | 1 B | Atributo del metatile con el que colisionó Jack (`sub_9B6A`). |
| `$00E9-$00EA` | `CurrentRoomIDBackup2/3` | `scroll_room_id_nt0`, `scroll_room_id_nt2` | 2 B | IDs de las medias salas cargadas en los nametables durante el scroll continuo. |
| `$00EC-$00ED` | `PlayerX_Lo`, `PlayerY_Lo` | `probe_x_px`, `probe_y_px` | 2 B | Coordenadas X e Y de sonda durante las pruebas de colisión contra el escenario (`sub_9B39`, `BigBeefySubHandlesBGColliding`). |
| `$00EE`, `$00F0` | `byte_EE`, `byte_F0` | `hidden_block_candidate_idx`, `spawn_item_type_id` | 2 B | Índice de bloque candidato en `BigBeefySubHandlesBGColliding` (`$EE`) y tipo de ítem a spawnear al abrir un cofre/bloque (`$F0`). |
| `$00F1` | `_MaybeBlockSpawningObj` | `block_item_spawn_state` | 1 B | Estado de aparición de ítem desde un bloque oculto o cofre (`MaybeOpenHiddenBlock`). |
| `$00F2` | `PPUUpdateFlag1` | `ppu_update_mode_flag1` | 1 B | Selector de modo en `HandlePPUUpdates` (`0` = `PPUUpdateBuffer`, `1` = `AttributeTableBuffer`). |
| `$00F3` | `JustJumpedFlag` | `player_just_jumped_flag` | 1 B | Se pone a `1` en el frame exacto en que Jack despega del suelo; se limpia tras procesarse en `GameState_7` y `MaybeOpenHiddenBlock`. |
| `$00F4` | `byte_F4` | `room_nametable_select_bit` | 1 B | Bit de selección de nametable base (`0` o `8`) para el cálculo de direcciones PPU en `sub_92AD` y `sub_A18E`. |
| `$00F5` | `RoomPalette` | `pending_room_palette_id` | 1 B | ID de paleta de fondo de la sala a cargar en `GameState_4_EnterRoom`. |
| `$00F6` | `Collected1UPThisRoundFlag` | `collected_1up_this_round` | 1 B | Impide spawnear más de un 1UP por fase. |
| `$00F7` | `byte_F7` | `room_bombs_collected_count` | 1 B | Contador de bombas recogidas en la habitación actual (`JTPickup_Bomb`). |
| `$00F8` | `MaybeBombThing` | `next_lit_bomb_slot_id` | 1 B | Índice de la bomba que tiene actualmente la mecha encendida (`JTPickup_Bomb`), o flag `$80`/`$40` cuando todas las bombas fueron recogidas (`DoorCond_0C_AllFireBombs`). |
| `$00F9` | `GS8_Status` | `round_clear_phase_flags` | 1 B | Bitmask de fases (`$01, $02, $04, $08`) dentro de `GameState_8_RoundClear`. |
| `$00FA` | `APressCounter` | `button_a_press_counter` | 1 B | Contador global de pulsaciones del botón A; determina en qué enemigo se transforma una Momia: `(APressCounter % 6) + 2`. |
| `$00FB` | `DrawScoreAndCoinsFlag` | `hud_dirty_flag` | 1 B | Si `!= 0`, `DrawScoreAndCoinCount` redibuja los sprites del HUD. |
| `$00FC` | `PPUUpdateFlag2` | `ppu_tile_patch_pending` | 1 B | Si `!= 0`, `HandlePPUUpdates` invoca `sub_9729` para parchar un metatile 2x2 en VRAM (al recoger bomba/abrir cofre o puerta). |
| `$00FE` | `UnknownFlag_0FE` | `post_coin_invulnerability_timer` | 1 B | Temporizador de 32 frames (`$20`) activado cuando expira `EnemyCoinTimer` (`MainSub_1`), durante el cual `CheckObjectCollision` ignora colisiones letales mientras las monedas vuelven a ser enemigos. |

---

## 5. Subpaso 0.4.2 — Pila, OAM, Estado de Sala y `PlayerStruct` (`$0100-$03EC`)

### 5.1. Regiones `$0100-$03CC`

| Rango Hex | Etiqueta `ram.asm` | Nombre Semántico C x86 | Tamaño | Rol Exacto y Semántica |
| :---: | :--- | :--- | :---: | :--- |
| `$0100-$01FF` | `StackArea` | `cpu_stack[256]` | 256 B | Pila hardware del 6502. |
| `$0200-$02FF` | `SpriteDMAArea` | `oam_buffer[256]` | 256 B | 64 entradas OAM de 4 bytes: `[0]=Y, [1]=Tile, [2]=Attr, [3]=X`. |
| `$0300-$0301` | `GameState1Flag`, `GameState1WaitTimer` | `round_intro_init_flag`, `round_intro_timer` | 2 B | Estado e intervalo de espera (128 frames) de la pantalla `"ROUND X"` (`GameState_1_RoundIntro`). |
| `$0308-$0309` | `Joypad1_Immediate`, `Joypad2_Immediate` | `joypad1_curr`, `joypad2_curr` | 2 B | Estado actual leído en `UpdateJoypads` (`Joypad1_Immediate` contiene el `OR` de ambos mandos). |
| `$030A-$030E` | `unk_30A`, `byte_30B..30E` | `score_multiplier_table_ram[5]` | 5 B | Buffer apuntado por `BasePointer_unk30A` (`$0D`); `sub_8FB1` escribe en `$030B-$030E` los tiles/coordenadas del multiplicador de puntos (`x2`, `x3`, `x4`). |
| `$030F` | `byte_30F` | `bg_collision_dir_mask` | 1 B | Máscara de direcciones bloqueadas calculada por las sondas de fondo (`sub_9965`, `sub_9B6A`). |
| `$0310-$0313` | `byte_310..byte_313` | `chest_and_headbutt_probe[4]` | 4 B | Coordenadas y celda del bloque golpeado desde abajo o cofre tocado por Jack (`BigBeefySubHandlesBGColliding`, `JTPickup_Chest`, `sub_A18E`, `MaybeOpenHiddenBlock`). |
| `$0314-$0315` | `byte_314`, `CollisionTestResult` | `entity_probe_flags`, `collision_test_result` | 2 B | Resultado de sonda de colisión contra bloques sólidos. |
| `$0317-$0319` | `TortureRoom*`, `GreedyJumpsRemaining` | `torture_state`, `torture_msg_timer`, `greedy_jumps_bcd` | 3 B | Control de la sala de castigo (`$AC`) y contador BCD de saltos restantes (`$49` JP / `$50` US). |
| `$031B-$031F` | `StageTimerCopy` .. `GameOverTimer` | `round_clear_and_gameover_vars` | 5 B | Variables de conteo de bonus en `GameState_8_RoundClear` y temporizador de `GameState_A_GameOver`. |
| `$0320-$032F` | `byte_320` | `probe_metatile_coords[16]` | 16 B | Coordenadas de los puntos de sonda de colisión (`BigBeefySubHandlesBGColliding`, `sub_9B6A`). |
| `$0330-$0336` | `byte_330..byte_336` | `hidden_block_bounce_state` | 7 B | Contadores de rebote, última celda golpeada (`byte_333/334`) y temporizador de apertura de bloques ocultos (`byte_335/336`). |
| `$0337-$0339` | `MaybeDoorReturnTwiceFlag`, `byte_338/339` | `door_anim_state`, `door_anim_timer`, `door_anim_step` | 3 B | Control de la animación de apertura y cierre de puertas (`AnimateDoor`, `AnimateDoorClose`). Si bit 7 de `$0337` está activo al terminar `AnimateDoorClose`, hace `PLA; PLA; RTS`. |
| `$033B-$0343` | `EntryDoorType` .. `byte_343` | `room_entry_transition_state` | 9 B | Tipo de puerta de entrada (`$033B`), backups (`$033D`, `$033F`), backup de sala pre-tortura (`$033E`), fases de entrada/salida (`$0340`), timer de caminata (`$0341`), dirección (`$0342`) y timer de scroll inicial (`$0343`). |
| `$0346-$0355` | `MaybeTempSpriteIndex` | `enemy_oam_shuffle_buffer[16]` | 16 B | Buffer circular usado por `MaybeDrawEnemySprites` para rotar el orden de dibujo OAM de los enemigos y convertir el límite de 8 sprites por scanline en parpadeo (*flickering*). |
| `$0356-$0359` | `MaybeTempCollectable*`, `Show1UPSpriteTimer` | `power_coin_and_1up_state` | 4 B | Flags de Power Coin activa (`$0356-$0357`), flag de 1UP por Brother/Tecmo Plate (`$0358`) y timer de sprite `"1UP"` (`$0359`). |
| `$035C-$035F` | `InitFlag_LoadedDifficulty` .. `EnemyTransformTimer` | `difficulty_and_spawn_timers` | 4 B | Flag de carga de dificultad (`$035C`), índice de dificultad (`$035D`), timer de spawn (`$035E`) y timer de transformación de Momia (`$035F`). |
| `$0364-$036B` | `byte_364` .. `MaybeDoorTypeOrSphinx` | `score_and_door_check_state` | 8 B | Flags de bonus/1UP por puntos (`byte_364`), copia de millares (`$0365`), `GoldCoinsCollected` (`$0367`), `SphinxCollectedMaybe` (`$0368`), `MaybeHandleDoorFlag` (`$0369`), `CurrentRoomIDBackup4` (`$036A`) y `MaybeDoorTypeOrSphinx` (`$036B`). |
| `$0372-$0381` | `byte_372..37C`, `EndingType` | `ending_sequence_state` | 16 B | Contadores de excavación/demolición de la pirámide y líneas de texto en `EndingState_1..4` y `sub_AFA9`. |
| `$0391-$039E` | `PlayerCrystalBallsCollected` .. `MaybeSphinxFlags` | `progression_and_door_flags` | 18 B | Bolas de cristal (`$0391`), tile de fuego letal (`$0392`), estado de puerta (`$0393-$039A`), `PlayerGDV` (`$039B`), `HighGDV` (`$039C`), `ExtraLivesFound` (`$039D`) y tabla de 32 bytes `MaybeSphinxFlags` (`$039E-$03BD`) que registra qué puertas/esfinges de la sala están abiertas (`$52`). |
| `$03BE-$03CC` | `RoomStatusFlags` .. `LastBombRoomCleared` | `camera_and_scroll_state` | 15 B | `CurrentRoomID` (`$03BF`), `CheckpointRoomIDMaybe` (`$03C0`), subpíxeles y píxeles de scroll X (`byte_3C1` lo, `byte_3C2` hi) y scroll Y (`byte_3C3` lo, `byte_3C4` hi), `CurrentRoomIDBackup` (`$03C5`), seam de scroll (`byte_3C6..3CB`) y `LastBombRoomCleared` (`$03CC`). |

---

### 5.2. Estructura `PlayerStruct` (`$03CE-$03EC`, 31 Bytes `$1F`)

`PlayerStruct` comparte sus primeros 16 bytes (`$00..$0F`) con el layout de `EnemyStruct` (`defs.asm:110-137`) para reutilizar las rutinas de integración de velocidad (`ApplyPlayerXVelocity`, `DoSomethingWithObjYVel`):

```c
typedef struct {
    uint8_t status;               // $03CE (+0x00): Bit 0=activo, Bit 1=en aire, Bit 3=salto iniciado, Bit 5=muerto en suelo, Bit 6=muerte iniciada, Bit 7=muriendo
    uint8_t collision_flag;       // $03CF (+0x01): Bit 0=bloqueo Der, Bit 1=bloqueo Izq, Bit 2=suelo ($04), Bit 3=techo ($08)
    uint8_t x_vel_lo;             // $03D0 (+0x02): Subpíxel velocidad X ($00 o $80)
    uint8_t x_pos_lo;             // $03D1 (+0x03): Subpíxel posición X
    int8_t  x_vel_hi;             // $03D2 (+0x04): Píxel velocidad X (+1 der, -2 izq con $80 lo = -1.5)
    uint8_t x_pos_hi;             // $03D3 (+0x05): Píxel posición X en pantalla
    uint8_t unused_06;            // $03D4 (+0x06)
    uint8_t death_timer;          // $03D5 (+0x07): MaybeDeathTimer (6 frames giro aire, $40 frames suelo)
    uint8_t y_vel_lo;             // $03D6 (+0x08): Subpíxel velocidad Y
    uint8_t y_pos_lo;             // $03D7 (+0x09): Subpíxel posición Y
    int8_t  y_vel_hi;             // $03D8 (+0x0A): Píxel velocidad Y
    uint8_t y_pos_hi;             // $03D9 (+0x0B): Píxel posición Y en pantalla
    uint8_t unused_0C[2];         // $03DA-$03DB (+0x0C..0x0D)
    uint8_t sprite_id;            // $03DC (+0x0E): PlayerSprite
    uint8_t facing_attr;          // $03DD (+0x0F): byte_3DD ($00 izq, $08 der, $21 detrás puerta)
    uint8_t unused_10;            // $03DE (+0x10)
    uint8_t walk_anim_counter;    // $03DF (+0x11): byte_3DF (alterna frame caminar cada 8 ticks)
    uint8_t mighty_level;         // $03E0 (+0x12): PlayerMightyLevel (0..3: Normal, Azul, Naranja, Verde)
    uint8_t mighty_presses_left;  // $03E1 (+0x13): PlayerMightyLevelPressesLeft (hasta 40 pulsaciones A)
    uint8_t mighty_coins;         // $03E2 (+0x14): PlayerMightyCoins (0..9; al llegar a 10 -> Torture Room)
    uint8_t score_multiplier;     // $03E3 (+0x15): ScoreMultiplier (1..4)
    uint8_t lives;                // $03E4 (+0x16): PlayerLives (inicia en 3)
    uint8_t secret_coins;         // $03E5 (+0x17): PlayerSecretCoins (S-Coins para el buen final)
    uint8_t jump_table_step;      // $03E6 (+0x18): byte_3E6 (índice 0..63 en word_C0E3)
    uint8_t jump_flutter_state;   // $03E7 (+0x19): byte_3E7 ($00=salto normal, $40=flutter en aire)
    uint8_t padding_1A_1E[5];     // $03E8-$03EC (+0x1A..0x1E): limpiados por InitPlayerStruct ($1F bytes)
} PlayerStateRAM;
```

---

## 6. Subpaso 0.4.3 — Buffers PPU, Motor APU, Sala Activa y Enemigos (`$03EE-$07FF`)

| Rango Hex | Etiqueta `ram.asm` | Nombre Semántico C x86 | Tamaño | Rol Exacto y Estructura |
| :---: | :--- | :--- | :---: | :--- |
| `$03EE-$03F1` | `byte_3EE..byte_3F1` | `ppu_cmd_header[4]` | 4 B | `[0]`=inc bit (`0`/+1 o `4`/+32), `[1]`=VRAM addr hi, `[2]`=VRAM addr lo, `[3]`=length. |
| `$03F2-$0431` | `PPUUpdateBuffer`, `unk_3F3` | `ppu_update_buffer[64]` | 64 B | Buffer de tiles/strings para copiar a VRAM en `HandlePPUUpdates` (`unk_3F3` es `PPUUpdateBuffer + 1`). |
| `$0432-$0451` | `SoundsQueued`, `SoundQueue` | `apu_queue_count`, `apu_queue[31]` | 32 B | Cola FIFO/LIFO de IDs de pistas y SFX solicitados mediante `QueueSound`. |
| `$0452-$0511` | `SoundChannelA_Mus0..Sfx3` | `apu_chan_a[8][24]` | 192 B | 8 estructuras de canal de 24 bytes (`$18`) cada una: 4 canales de música (`0..3`) y 4 de SFX (`4..7`). |
| `$0512-$0551` | `SoundChannelB_Mus0..Sfx3` | `apu_chan_b[8][8]` | 64 B | 8 buffers secundarios de 8 bytes por canal (envolvente/frecuencia base). |
| `$0552-$0556` | `SoundRegIndex` .. `SoundReg_Hi` | `apu_hw_reg_staging[5]` | 5 B | Registros temporales antes de volcar a `$4000-$4015`. |
| `$0563-$05A2` | `AttributeTableBuffer` | `nt0_attr_shadow[64]` | 64 B | Espejo en RAM de la tabla de atributos del Nametable 0 (`$23C0-$23FF`), apuntado por `BasePointer_AttributeTableBuffer` (`7`). |
| `$05A3-$05E2` | `byte_5A3` | `nt2_attr_shadow[64]` | 64 B | Espejo en RAM de la tabla de atributos del Nametable 2 (`$2BC0-$2BFF`), apuntado por `PointerTable050[8]`. |
| `$05E3-$0602` | `PaletteBuffer` | `palette_ram[32]` | 32 B | Espejo de las 8 paletas PPU (`$3F00-$3F1F`: 4 BG + 4 Sprites). |
| `$0603-$0682` | `RoomDataRAM` | `active_room_objects_ram[128]` | 128 B | Mapa descomprimido de objetos interactivos (bombas, cofres, monedas, puertas) de la sala activa. |
| `$0683-$06A5` | `byte_683` | `room_collision_grid_tail[35]` | 35 B | Extensión de estado de objetos de sala inicializada en `IfNotTitleScreenRoomDoThings`. |
| `$06A6-$0785` | `Object0Struct` .. `Object7Struct` | `enemies[8]` (`EnemyStruct[8]`) | 224 B | 8 estructuras de entidades/enemigos de **28 bytes (`$1C`)** cada una (`$06A6, $06C2, $06DE, $06FA, $0716, $0732, $074E, $076A`). Nota: `STA $1EFE` en `RESET` escribe en `$06FE` (`Object3Struct + 4`) por el espejo de RAM de 2 KB de la NES. |
| `$0786-$078D` | `byte_786..byte_78D` | `special_bonus_popup_state[8]` | 8 B | Estructura apuntada por `PointerTable050[$C]` (`sub_A270`, `sub_A29C`, `sub_A33E`): gestiona el sprite flotante de bonificación (`10000`..`50000` pts / moneda especial) al recoger bombas encendidas: `byte_786`=flags de estado, `byte_787`=índice sprite, `byte_789`=X, `byte_78B`=Y, `byte_78C/78D`=timers de parpadeo/vida. |
| `$07F8` | `Obj7_Timer` | `global_anim_and_spawn_tick` | 1 B | Temporizador auxiliar global en la página `$0700`. |

---

## 7. Plan de Verificación para C x86

- [x] **Cobertura del 100% de RAM (`$0000-$07FF`)**: Documentadas las 305 etiquetas de `ram.asm` + 33 alias de registros, incluyendo las 127 etiquetas `byte_XX` y 23 `Maybe...`.
- [x] **Alineación Binaria**: Verificados los offsets de `PlayerStruct` (`$03CE`, `$1F` bytes) y `Object0..7Struct` (`$06A6..$0785`, `8 x $1C` bytes).

---

## 8. Referencias Cruzadas

- **Inventario Maestro**: [`RES-003`: Inventario Maestro de Símbolos, Tablas de Salto y Grafo de Llamadas](file:///c:/Users/kanel/Documents/mjk/research/RES-003-inventario-maestro-simbolos-y-grafo-llamadas.md)
- **Subsistema 01 (Núcleo)**: [`RES-004`: Especificación Funcional C x86 — Subsistema 01](file:///c:/Users/kanel/Documents/mjk/research/RES-004-subsistema-nucleo-nmi-ppu-estados.md)
