# Índice Maestro de Referencias Cruzadas (Cross-Reference Index)
## Ensamblador 6502 (NES 1987 `REV_US`) <---> Source Port C11 Clean-Slate

**Propósito**: Diccionario canónico bidireccional entre las direcciones RAM, subrutinas y tablas del cartucho original de NES (`asm/src/`) y los módulos C11 puros (`include/` y `src/core/`) de la arquitectura limpia multiplataforma (Windows & Linux, Clean-Room BYOR v2).

---

## 1. Mapeo Canónico de Memoria RAM (`$0000..$07FF`) <---> Campos C11 (`MjkGame`)

| Dirección RAM | Símbolo y Línea en `asm/src/ram.asm` | Descripción / Semántica | Campo en C11 (`MjkGame`) | Header de Definición |
| :--- | :--- | :--- | :--- | :--- |
| **`$0043`** | `InGameFlag` (`ram.asm:104`) | `0` = Título/Copyright, `1` = Partida activa | `game->top_mode` (`MjkTopMode`) | `include/mjk_game.h:34`, `include/mjk_types.h:157-160` |
| **`$0044`** | `GameState` (`ram.asm:108`) | Máquina de estados maestra (`0..$C`) | `game->state` (`MjkGameState`) | `include/mjk_game.h:35`, `include/mjk_types.h:163-177` |
| **`$0045`** | `TitleScreenState` (`ram.asm:110`) | Subestado de Copyright/Título/Caída (`REV_US`) | `game->title.state` (`MjkTitleState`) | `include/mjk_title.h`, `include/mjk_types.h:180-186` |
| **`$0088`** | `CopyrightTimer` (`ram.asm:207`) | Temporizador Copyright (256f) / Título (512f) | `game->title.timer` (`uint16_t`) | `include/mjk_title.h` |
| **`$009B`/`$009C`** | `HighGDV` / `PlayerGDV` (`ram.asm:233-235`) | GDV récord y actual en BCD (inicia `$47`) | `game->high_gdv_bcd` / `game->gdv_bcd` | `include/mjk_game.h:60-61` |
| **`$00A0..$00A3`** | `PlayerScore` (`ram.asm:240-244`) | Puntuación BCD little-endian (4 bytes, 8 dígitos) | `game->score_bcd[4]` | `include/mjk_game.h:62` |
| **`$00A4`** | `StageTimer` (`ram.asm:245-247`) | Temporizador de fase en BCD (inicia `$60`) | `game->stage_timer_bcd` | `include/mjk_game.h:64` |
| **`$00A5`** | `TimerFrames` (`ram.asm:248-250`) | Divisor de reloj de 256 frames | `game->timer_frames` (`uint16_t`) | `include/mjk_game.h:65` |
| **`$00AE`/`$00AF`** | `FireBombsCollected` / `2` (`ram.asm:264-266`) | Bombas encendidas y total en Bomb Room (23) | `game->fire_bombs_collected` / `objects.fire_bombs_total` | `include/mjk_game.h:73`, `include/mjk_objects.h` |
| **`$00B1`** | `AliveEnemyCount` (`ram.asm:268-269`) | Monstruos vivos simultáneos (máx. 4) | `game->enemies.alive_count` | `include/mjk_enemy.h` |
| **`$00BC`** | `RoomStatusFlags` (`ram.asm:283`) | Flags `RF_*` (Horizontal, SingleScreen, BombRoom, ScrollEdge) | `game->camera.room_flags` | `include/mjk_camera.h`, `include/mjk_types.h:189-198` |
| **`$00F7`/`$00F8`** | `byte_F7` / `MaybeBombThing` (`ram.asm:379-381`) | Cadena de bombas y bonus perfecto 10.000 pts | `game->objects.first_bomb_picked` / `maybe_bomb_thing` | `include/mjk_objects.h` |
| **`$00FA`** | `APressCounter` (`ram.asm:386`) | Contador de pulsaciones `A` para mutación `(A%6)+2` | `game->player.a_press_counter` | `include/mjk_player.h` |
| **`$006A..$006B`** | `EnemyCoinTimer` (`ram.asm:172`) | Temporizador de transmutación Power Coin (320f) | `game->enemies.transmute_timer` | `include/mjk_enemy.h` |
| **`$0200..$02FF`** | `SpriteDMAArea` (`ram.asm:656-912`) | Tabla OAM de 64 sprites hardware (4 B/sprite) | `game->ppu.oam[64]` (`MjkOamEntry`) | `include/mjk_ppu.h` |
| **`$033B`/`$033D`/`$033F`** | `EntryDoorType` + backups (`ram.asm:1006-1014`) | Puerta de entrada activa y respaldos de checkpoint/Torture | `game->entry_door_type` / `entry_door_backup` / `torture_return_door_type` | `include/mjk_game.h:55-57` |
| **`$0356`/`$0357`** | `MaybeTempCollectableFlag2/1` (`ram.asm:1044-1046`) | Acumulador de 20 bombas y bandera Power Coin armada | `game->objects.power_coin_acc` / `power_coin_armed` | `include/mjk_objects.h` |
| **`$0366`** | `GoldCoinsCollected` (`ram.asm:1069`) | Monedas de oro acumuladas (40 -> Extra Coin 1UP) | `game->gold_coins` | `include/mjk_game.h:77` |
| **`$0376`** | `Collected1UPThisRoundFlag` (`ram.asm:1095`) | Flag de 1UP obtenido en la ronda | `game->collected_1up` | `include/mjk_game.h:78` |
| **`$0391`** | `PlayerCrystalBallsCollected` (`ram.asm:1128`) | Máscara de Bolas de Cristal (bit 0/1 -> Finales 1/2) | `game->crystal_balls_mask` | `include/mjk_game.h:75` |
| **`$0395`/`$0399`/`$039A`** | `DoorCheckFlag_1E/1F/09` (`ram.asm:1137-1149`) | Flags persistentes de puertas warp (`1E`, `1F`, `09`) | `game->door_1e_deaths` / `door_flag_1f` / `door_flag_09` | `include/mjk_game.h:48-50` |
| **`$03BF`/`$03C0`/`$033E`** | `CurrentRoomID` / `Checkpoint` (`ram.asm:1194-1196`) | Sala activa (`0x00..0xFF`), checkpoint y retorno Torture | `game->current_room_id` / `checkpoint_room_id` / `torture_return_room_id` | `include/mjk_game.h:52-54` |
| **`$03C1..$03C4`** | `ScrollX/Y` Q8.8 (`ram.asm:1198-1203`) | Coordenadas de scroll horizontal/vertical de cámara | `game->camera.scroll_x_fixed` / `scroll_y_fixed` | `include/mjk_camera.h` |
| **`$03CE..$03DF`** | `PlayerStruct` (`ram.asm:1223-1254`) | Posición/velocidad Q8.8, colisiones 9-probe, sprite y dirección | `game->player` (`MjkPlayer`) | `include/mjk_player.h` |
| **`$03E0..$03E4`** | `MightyLevel/Presses/Coins/Mult/Lives` (`ram.asm:1255-1265`) | Poder Mighty `0..3`, monedas, multiplicador `x2..x5` y vidas | `game->mighty_level`, `mighty_presses_left`, `mighty_coins`, `score_multiplier`, `lives` | `include/mjk_game.h:59, 68-71` |
| **`$03E5`** | `PlayerSecretCoins` (`ram.asm:1266`) | Secret Coins acumuladas (`>= 5` -> Final 3) | `game->secret_coins` | `include/mjk_game.h:76` |
| **`$03E6..$03E7`** | `byte_3E6`/`byte_3E7` (`ram.asm:1268-1270`) | Acumulador Q8.8 de la tabla de salto de 64 pasos | `game->player.jump_accum` | `include/mjk_player.h` |
| **`$0452..$0511`** | `SoundChannelA_Mus0..Sfx3` (`music-engine.asm:793-801`) | 8 canales virtuales de audio (4 BGM + 4 SFX) | `game->apu.channels[8]` (`MjkApuChannel`) | `include/mjk_apu.h:61-80` |
| **`$05E3..$0602`** | `PaletteBuffer` (`ram.asm:1813-1845`) | 32 bytes de paletas PPU BG + Sprites | `game->ppu.palette_ram[32]` | `include/mjk_ppu.h` |
| **`$0603..$0692`** | `RoomDataRAM` + `byte_683` (`ram.asm:1846-1990`) | Persistencia de 8 slots de sección + 1 slot scratch | `game->persist` (`MjkRoomPersistRam`) | `include/mjk_room_decoder.h` |
| **`$0786..$078D`** | `BombPuffSlots` (`ram.asm:2244-2251`) | Animación puff de bomba en 2 fases (32f + 32f) | `game->objects.puff` (`MjkBombPuff`) | `include/mjk_objects.h` |

---

## 2. Mapeo de Subrutinas 6502 (`asm/src/`) <---> Módulos C11 (`src/core/`)

### 2.1. Núcleo, Título y Máquina de Estados
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C11 Equivalente | Archivo en `src/core/` |
| :--- | :--- | :--- | :--- |
| `RESET` / `NMI` | `prg.asm:8-126` | `MjkGame_Init` / `MjkGame_StepFrame` / `MjkGame_Render` | `src/core/mjk_state_machine.c` |
| `TitleScreenHandler` (`US_0/1`, `0..3`) | `prg.asm:7473-7660` | `MjkTitle_Init` / `MjkTitle_Step` | `src/core/mjk_title.c` |
| `GameState_0_Init`..`5_Main`, `6`, `7`, `9`, `B` | `prg.asm:323-775, 1122-1163, 7294-7372` | `MjkGame_StepFrame`, `MjkGame_ExitRoom`, `MjkGame_LoseLife`, `MjkGame_TriggerTortureRoom` | `src/core/mjk_state_machine.c` |
| `GameState_8_RoundClear`, `A_GameOver`, `C_Ending` | `prg.asm:776-1091, 7706-7924` | `MjkState_StepRoundClear`, `MjkState_StepGameOver`, `MjkState_StepEnding` | `src/core/mjk_end_screens.c` |

### 2.2. PPU 2C02, Metasprites y Decodificador 4-Tier de 256 Salas
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C11 Equivalente | Archivo en `src/core/` |
| :--- | :--- | :--- | :--- |
| `RestoreDefaultPalettes` / `LoadRoomPalette` | `prg.asm:1937-1950, 2389-2432` | `MjkPpu_RestoreDefaultPalettes` / `MjkPpu_LoadRoomPalette` | `src/core/mjk_ppu.c` |
| `WriteSprite` / `WriteStringToPPU` | `prg.asm:2050-2087, 2267-2374` | `MjkPpu_WriteMetasprite` / `MjkPpu_WriteString` / `MjkPpu_RenderFrame` | `src/core/mjk_ppu.c` |
| `LoadRoomStuff` / `HandleDrawHalfRow` / `DrawFullScreen` | `prg.asm:2630-2925` | `MjkRoom_DecodeGeometry` / `MjkRoom_BlitToPpu` | `src/core/mjk_room_decoder.c` |
| `GameState_2_LoadRoom` / `ReadRoomData` / `sub_9592` | `prg.asm:3130-3680` | `MjkRoom_PreloadSection` / `MjkRoom_DecodeObjects` | `src/core/mjk_room_decoder.c` |

### 2.3. Física de Jack, Colisiones 9-Probe y Cámara
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C11 Equivalente | Archivo en `src/core/` |
| :--- | :--- | :--- | :--- |
| `MainSub_1` / `DoSomethingWithObjYVel` / `sub_8AE8` | `prg.asm:1304-1744` | `MjkPlayer_Init` / `MjkPlayer_Update` | `src/core/mjk_player.c` |
| `sub_9965` / `sub_9A03` / `sub_9B6A` / `sub_A1B9` / `sub_A1EB` | `prg.asm:4152-4611, 5530-5609` | `is_probe_solid` (sobre `static_metatiles`) en `MjkPlayer_Update` | `src/core/mjk_player.c` |
| `MainSub_3` / `sub_A843` | `prg.asm:5906-6534` | `MjkCamera_InitForRoom` / `MjkCamera_Update` | `src/core/mjk_camera.c` |

### 2.4. Bombas, Cofres, Puertas, Enemigos y HUD
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C11 Equivalente | Archivo en `src/core/` |
| :--- | :--- | :--- | :--- |
| `JTPickup_Bomb` / `sub_A270` / `sub_A29C` | `prg.asm:4970-5165, 5647-5747` | `collect_bomb` / `MjkObjects_Update` / `MjkObjects_RenderOam` | `src/core/mjk_objects.c` |
| `JTPickup_Chest` / `MaybeOpenHiddenBlock` / `UseMightyCoin` | `prg.asm:1745-1782, 4860-5461` | `open_chest` / `pickup_item` / `MjkObjects_UseMightyCoin` | `src/core/mjk_objects.c` |
| `MaybeHandleDoorOpening` / `CheckAndMaybeOpenDoor` / `JTPickup_Door` | `prg.asm:5312-5368, 6696-7091` | `MjkDoors_EvaluateCondition` / `MjkDoors_OpenRoomDoor` / `MjkDoors_CheckPlayerEntry` | `src/core/mjk_doors.c` |
| `HandleEnemySpawnTimer` / `RunEnemyLogic` / `TransformMummy` / `TurnEnemiesIntoCoins` | `prg.asm:8171-9575` | `MjkEnemy_InitForRoom` / `MjkEnemy_UpdateAll` / `MjkEnemy_ActivatePowerCoin` / `MjkEnemy_RenderOam` | `src/core/mjk_enemy.c` |
| `AddScore` / `HandleStageTimer` / `DrawScoreAndCoinCount` / `sub_8FB1` | `prg.asm:1191-1237, 1783-1933, 2436-2573` | `MjkHud_AddScore` / `MjkHud_TickStageTimer` / `MjkHud_SpawnFloatingScore` / `MjkHud_RenderOam` | `src/core/mjk_hud.c` |

### 2.5. Audio APU 2A03 y Verificación Externa (Capa 2)
| Subrutina 6502 / Capa | Referencia | Función C11 Equivalente | Archivo en `src/core/` |
| :--- | :--- | :--- | :--- |
| Hardware 2A03 (`$4000-$4015`) | NTSC `1789772.5 Hz` | `MjkApuSynth_Init`, `MjkApuSynth_ClockSweep`, `MjkApuSynth_RenderSamples` | `src/core/mjk_apu_synth.c` |
| `HandleSound` / `HandleSoundQueue` / `HandleSoundChannel` | `asm/src/music-engine.asm:1-810` | `MjkApu_Init`, `MjkApu_QueueSound`, `MjkApu_StepFrame` | `src/core/mjk_apu.c` |
| API de Verificación Headless (`ctypes` Capa 2) | `tools/verify_scenarios.py` | `MjkVerify_Create`, `MjkVerify_StepFrame`, `MjkVerify_GetSnapshot` | `src/core/mjk_verify_api.c` |

---

## 3. Resolución Clean-Room BYOR v2 de Tablas ROM (`assets.dat` v2)

Ninguna tabla con datos propietarios de Tecmo se incluye en el código fuente C. Todos los bloques se extraen en tiempo de ejecución desde `mbj-us.nes` (`SHA-1: a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`) mediante `src/core/mjk_rom_extractor.c` hacia `assets/assets.dat` v2 y se consultan por dirección CPU original mediante `MjkAssets_ResolveCpuPtr` (`include/mjk_assets.h`):

| Tabla ROM (`asm/src/`) | Dirección CPU | Macro en `include/mjk_assets.h` | Módulo Consumidor |
| :--- | :---: | :--- | :--- |
| `NoteFrequencyTable` (`music-engine.asm:776`) | `$B3FD` | `MJK_ADDR_NOTE_FREQ_TABLE` | `src/core/mjk_apu.c` |
| `SpriteAnimationTable` (`data.asm:6`) | `$BEAE` | `MJK_ADDR_SPRITE_ANIM_TABLE` | `src/core/mjk_enemy.c` |
| `DifficultyTable` (`data.asm:116`) | `$BF58` | `MJK_ADDR_DIFFICULTY_TABLE` | `src/core/mjk_enemy.c` |
| `word_C0E3` / `word_C163` (`data.asm:187-256`) | `$BFB0` / `$C030` | `MJK_ADDR_JUMP_TABLE_C0E3` / `MJK_ADDR_VEL_X_TABLE_C163` | `src/core/mjk_player.c`, `mjk_title.c` |
| `MainPalette` / `BackgroundPaletteSets` (`data.asm:258-305`) | `$C038` / `$C058` | `MJK_ADDR_MAIN_PALETTE` / `MJK_ADDR_BG_PALETTE_SETS` | `src/core/mjk_ppu.c` |
| `MightyLevelColors` / `MightyLevelAPressesTable` (`data.asm:306-310`) | `$C0C4` / `$C0C8` | `MJK_ADDR_MIGHTY_COLORS` / `MJK_ADDR_MIGHTY_A_PRESSES` | `src/core/mjk_objects.c`, `mjk_player.c` |
| `MusicOptionsTable` / `DoorEntryXYPositionTable` / `VectorTable` | `$C0CC` / `$C0D4` / `$C0F4` | `MJK_ADDR_MUSIC_OPTIONS` / `MJK_ADDR_DOOR_ENTRY_XY` / `MJK_ADDR_VECTOR_TABLE` | `src/core/mjk_state_machine.c` |
| `SpritesTable` / `SpriteAttributeTable` (`data.asm:364, 588`) | `$C11C` / `$C4D9` | `MJK_ADDR_SPRITES_TABLE` / `MJK_ADDR_SPRITE_ATTR_TABLE` | `src/core/mjk_ppu.c` |
| `StringPointerTable` (`data.asm:414`) | `$C202` | `MJK_ADDR_STRING_PTR_TABLE` | `src/core/mjk_assets.c`, `mjk_ppu.c` |
| `TileAttributeTable` / `SectionRoomsTable` (`data.asm:598, 640`) | `$C4E1` / `$C509` | `MJK_ADDR_TILE_ATTR_TABLE` / `MJK_ADDR_SECTION_ROOMS_TBL` | `src/core/mjk_room_decoder.c` |
| `FireBombBonus` / `MultipliedScoreTable` / `ScoreAddTable` | `$C646` / `$C64A` / `$C66A` | `MJK_ADDR_FIRE_BOMB_BONUS` / `MJK_ADDR_MULTIPLIED_SCORE` / `MJK_ADDR_SCORE_ADD_TABLE` | `src/core/mjk_hud.c`, `mjk_end_screens.c` |
| `ItemToTileTable` / `RoomHalfScreens` / `HalfScreenLayouts` | `$C6AA` / `$C6CA` / `$C8CA` | `MJK_ADDR_ITEM_TO_TILE` / `MJK_ADDR_ROOM_HALF_SCREENS` / `MJK_ADDR_HALF_SCREEN_LAYTS` | `src/core/mjk_room_decoder.c`, `mjk_objects.c` |
| `LayoutChunks` / `MetatileDefinitions` / `AdjacentRoomsTable` | `$D0EA` / `$D7AA` / `$DA0E` | `MJK_ADDR_LAYOUT_CHUNKS` / `MJK_ADDR_METATILE_DEFS` / `MJK_ADDR_ADJACENT_ROOMS` | `src/core/mjk_room_decoder.c`, `mjk_camera.c` |
| `EnemySpawnPositionTable` / `RoomDataPointers` (`data.asm:1963, 2227`) | `$DF0E` / `$F1A0` | `MJK_ADDR_ENEMY_SPAWN_POS` / `MJK_ADDR_ROOM_DATA_PTRS` | `src/core/mjk_enemy.c`, `mjk_assets.c` |
| `NoteLengthTable` / `SoundVolTable` / `SoundPointers` (`music-data.asm`) | `$F39E` / `$F3BE` / `$F45C` | `MJK_ADDR_NOTE_LENGTH_TABLE` / `MJK_ADDR_SOUND_VOL_TABLE` / `MJK_ADDR_SOUND_POINTERS` | `src/core/mjk_apu.c` |
