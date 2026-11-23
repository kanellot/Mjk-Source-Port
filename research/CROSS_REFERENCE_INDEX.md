# Índice Maestro de Referencias Cruzadas (Cross-Reference Index)
## Ensamblador 6502 (NES 1987) <---> Source Port C99 (Windows)

**Propósito**: Diccionario canónico bidireccional con números de línea exactos entre las direcciones de memoria RAM, subrutinas y tablas del cartucho original de NES (`asm/src/`) frente a las estructuras, campos y funciones del port en C (`include/` y `src/core/`).  
**Uso Mandatorio**: Todos los agentes y subagentes (`parity_solver`) deben consultar este índice antes de leer archivos para saltar directamente al rango de líneas exacto (`StartLine` / `EndLine`) y minimizar el consumo de tokens.

---

## 1. Mapeo Canónico de Memoria RAM ($0000..$07FF) <---> Campos C (`MjkGameState`)

| Dirección RAM | Símbolo y Línea en `asm/src/ram.asm` | Descripción / Semántica | Campo en C (`MjkGameState`) | Definición Exacta en `include/` |
| :--- | :--- | :--- | :--- | :--- |
| **`$0043`** | `InGameFlag` (`ram.asm:104`) | `0` = Menú/Copyright/Intro, `1` = Partida activa | `game->in_game` (`bool`) | `include/mjk_game.h:49` |
| **`$0044`** | `GameState` (`ram.asm:108`) | Máquina de estados maestra (`0..12`) | `game->current_state` (`GameStateId`) | `include/mjk_game.h:55`, `include/mjk_types.h:65-79` |
| **`$0045`** | `TitleScreenState` (`ram.asm:110`) | Sub-estado de la secuencia de título (`0..5`) | `game->title_screen_state` | `include/mjk_game.h:50`, `include/mjk_types.h:55-63` |
| **`$0048`** | `PlayerScreenX` (`ram.asm:117`) | Coordenada X entera de Jack en pantalla (px) | `FIXED_TO_INT(game->jack.pos_x)` | `include/mjk_types.h:108`, `include/mjk_game.h:73` |
| **`$004A`** | `PlayerScreenY` (`ram.asm:121`) | Coordenada Y entera de Jack en pantalla (px) | `FIXED_TO_INT(game->jack.pos_y)` | `include/mjk_types.h:109`, `include/mjk_game.h:73` |
| **`$004C..$0053`** | `EnemyScreenY/X` (`ram.asm:126-136`) | Coordenadas en pantalla de los 4 enemigos | `game->enemy_mgr.enemies[i].pos_x/y` | `include/mjk_types.h:170-171`, `include/mjk_enemy.h:26` |
| **`$0054`** | `byte_54` (`ram.asm:138`) | Racha/combo de transmutación de monedas (`0..6`) | `game->transmute_streak` | `include/mjk_game.h:136`, `include/mjk_enemy.h:28` |
| **`$006A..$006B`** | `EnemyCoinTimer` (`ram.asm:172`) | Contador de transmutación Power Coin (320f) | `game->enemy_coin_timer` | `include/mjk_game.h:134` |
| **`$0088`** | `CopyrightTimer` (`ram.asm:207`) | Temporizador de pantalla de inicio y copyright | `game->state_timer` | `include/mjk_game.h:57` |
| **`$00AE`** | `FireBombsCollected` (`ram.asm:264`) | Bombas encendidas recogidas en bomb room | `game->fire_bombs_collected` | `include/mjk_game.h:89`, `include/mjk_types.h:245` |
| **`$00AF`** | `FireBombsCollected2` (`ram.asm:266`) | Total de bombas recogidas en bomb room (hasta 23) | `game->fire_bombs_collected_total` | `include/mjk_game.h:99`, `include/mjk_types.h:246` |
| **`$00F0`** | `byte_F0` (`ram.asm:365`) | Coordenada `(Y, X)` del cofre/bloque activo | `game->chest_target_coord` | `include/mjk_game.h:128` |
| **`$00F7`** | `byte_F7` (`ram.asm:379`) | Flag de primera bomba recogida en la sala | `game->room_bomb_picked_flag` | `include/mjk_game.h:96`, `include/mjk_types.h:242` |
| **`$00F8`** | `MaybeBombThing` (`ram.asm:381`) | Flags: b0=starter, b1=broken, b7=bonus 10k | `game->maybe_bomb_thing` | `include/mjk_game.h:95`, `include/mjk_types.h:241` |
| **`$0200..$02FF`** | `OAM DMA Buffer` (`ram.asm:657-912`) | Tabla de 64 sprites de hardware de la PPU | `output->oam_sprites[256]` | `include/mjk_platform.h:81` |
| **`$0210..$0213`** | `OAM Sprite 4` (`ram.asm:673-676`) | Sprite de hardware para la Chispa de Mecha | `game->bomb_fuse.spark_sprite_*` | `include/mjk_types.h:249-254` |
| **`$0301`** | `WaitTimer1` (`ram.asm:916`) | Contador regresivo de micro-animaciones / espera | `game->state_timer` / `jack.anim_timer` | `include/mjk_game.h:57`, `include/mjk_types.h:121` |
| **`$0315`** | `_MaybeBlockSpawningObj` (`ram.asm:950`) | Comando de expulsión de ítem de cofre | `game->chest_spawning_cmd` | `include/mjk_game.h:129` |
| **`$0316..$0325`** | `EnemySlots` (`ram.asm:951-980`) | Ranuras de memoria para los 4 enemigos activos | `game->enemy_mgr.enemies[4]` | `include/mjk_enemy.h:26`, `include/mjk_game.h:79` |
| **`$0335`** | `byte_335` (`ram.asm:996`) | Impactos restantes en bloque secreto (`1..2`) | `game->secret_block_hits_left` | `include/mjk_game.h:126`, `include/mjk_types.h:276` |
| **`$0336`** | `byte_336` (`ram.asm:998`) | Coordenada `(Y, X)` del bloque secreto activo | `game->secret_block_active_coord` | `include/mjk_game.h:127` |
| **`$0337`** | `MaybeDoorReturnTwiceFlag` (`ram.asm:1000`) | Dirección de animación de puerta (`$81`/`$01`) | `game->door_anim_dir` | `include/mjk_game.h:112` |
| **`$0338`** | `byte_338` (`ram.asm:1002`) | Temporizador de animación de apertura puerta | `game->door_anim_timer` | `include/mjk_game.h:110` |
| **`$0339`** | `byte_339` (`ram.asm:1004`) | Fase de apertura de puerta (`0, 1, 2`) | `game->door_anim_phase` | `include/mjk_game.h:111` |
| **`$033B`** | `EntryDoorType` (`ram.asm:1007`) | Tipo de puerta por donde entró Jack (`0..15`) | `game->entry_door_type` | `include/mjk_game.h:109` |
| **`$0356`** | `MaybeTempCollectableFlag2` (`ram.asm:1044`) | Contador acumulador de bombas hacia Power Coin | `game->power_coin_bomb_counter` | `include/mjk_game.h:97`, `include/mjk_types.h:243` |
| **`$0357`** | `MaybeTempCollectableFlag` (`ram.asm:1046`) | Flag de disparo de Power Coin en cofre (`0, 1, 3`) | `game->power_coin_trigger_flag` | `include/mjk_game.h:98`, `include/mjk_types.h:244` |
| **`$0366`** | `GoldCoinsCollected` (`ram.asm:1069`) | Monedas de oro acumuladas (hasta 40) | `game->gold_coins_collected` | `include/mjk_game.h:137` |
| **`$0376`** | `Collected1UPThisRoundFlag` (`ram.asm:1095`) | Flag de vida extra recogida en la ronda actual | `game->collected_1up_this_round` | `include/mjk_game.h:130` |
| **`$038D`** | `StageTimerSeconds` (`ram.asm:1124`) | Temporizador regresivo de tiempo en sala (BCD) | `game->stage_timer_seconds` | `include/mjk_game.h:67` |
| **`$0393`** | `DoorIsAnimatingMaybe` (`ram.asm:1133`) | Flag booleano de puerta en mutación gráfica | `game->door_is_animating` | `include/mjk_game.h:113` |
| **`$0395`** | `DoorCheckFlag_1E_DeathCount` (`ram.asm:1137`) | Contador de muertes para condición puerta `0x1E` | `game->door_check_flag_1e_deaths` | `include/mjk_game.h:117` |
| **`$0396`** | `GreedJumpTimer` (`ram.asm:1140`) | Contador de penalización y codicia (Tortura) | `game->greed_jump_counter` | `include/mjk_game.h:149` |
| **`$0397`** | `DoorCheckFlag_19_1A` (`ram.asm:1143`) | Flag de salto en bloque para puerta `0x19`/`0x1A` | `game->door_check_flag_19_1a` | `include/mjk_game.h:119` |
| **`$0399`** | `DoorCheckFlag_1F` (`ram.asm:1147`) | Flag de paso de tiempo para puerta `0x1F` | `game->door_check_flag_1f` | `include/mjk_game.h:118` |
| **`$039A`** | `DoorCheckFlag_09` (`ram.asm:1149`) | Flag de recogida para condición de puerta `0x09` | `game->door_check_flag_09` | `include/mjk_game.h:116` |
| **`$03BF`** | `CurrentRoomID` (`ram.asm:1194`) | Identificador de la sala activa (`0..256`) | `game->current_room` | `include/mjk_game.h:61`, `include/mjk_camera.h:50` |
| **`$03C2`** | `byte_3C2` / `ScrollX` (`ram.asm:1200`) | Píxel entero de scroll horizontal de cámara | `game->camera_x` / `scroll_x_fixed` | `include/mjk_game.h:152`, `include/mjk_camera.h:54` |
| **`$03D8`** | `PlayerWorldX` (`ram.asm:1243`) | Coordenada X mundo de Jack (8 bits enteros) | `FIXED_TO_INT(game->jack.pos_x)` | `include/mjk_types.h:108` |
| **`$03D9`** | `PlayerWorldY` (`ram.asm:1245`) | Coordenada Y mundo de Jack (8 bits enteros) | `FIXED_TO_INT(game->jack.pos_y)` | `include/mjk_types.h:109` |
| **`$03DA`** | `PlayerSubX` (`ram.asm:1246`) | Acumulador de subpíxel horizontal de Jack | `(game->jack.pos_x & 0x0F)` | `include/mjk_types.h:108` |
| **`$03DB`** | `PlayerSubY` (`ram.asm:1247`) | Acumulador de subpíxel vertical de Jack | `(game->jack.pos_y & 0x0F)` | `include/mjk_types.h:109` |
| **`$03E4`** | `LivesCount` (`ram.asm:1264`) | Contador de vidas del jugador en BCD (`0..9`) | `game->lives_count` | `include/mjk_game.h:62` |
| **`$0600..$0682`** | `RoomDataRAM` (`ram.asm:1843-1973`) | Persistencia de objetos de las salas de sección | `game->room_data_ram.slots[8]` | `include/mjk_room_data.h:61`, `include/mjk_game.h:131` |
| **`$0786..$078D`** | `BombPuffSlots` (`ram.asm:2244-2251`) | Efectos visuales de recogida de bombas (humo) | `game->bomb_puffs[8]` | `include/mjk_game.h:101` |

---

## 2. Mapeo de Subrutinas 6502 (`asm/src/prg.asm`) <---> Funciones en C (`src/core/`)

### 2.1. Núcleo, Flujo Principal y Máquina de Estados (CTR-01)
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C Equivalente | Archivo y Líneas en `src/core/` |
| :--- | :--- | :--- | :--- |
| `RESET` | `prg.asm:8-83` | `MjkGame_Init` | `src/core/mjk_game.c:1366-1435` |
| `NMI` | `prg.asm:91-126` | `MjkGame_SimulateFrame` | `src/core/mjk_game.c:1499-1652` |
| `HandlePPUUpdates` | `prg.asm:127-240` | `RenderInGameScreen` / `MjkPPU_BlitMetatile` | `src/core/mjk_game.c:751-1364`, `src/core/mjk_ppu.c:90-100` |
| `MainLogicHandler` | `prg.asm:290-299` | Despacho `in_game` en `MjkGame_SimulateFrame` | `src/core/mjk_game.c:1505-1620` |
| `GameHandler` | `prg.asm:300-322` | `MjkGame_ExecuteState` | `src/core/mjk_state_machine.c:883-908` |
| `GameState_0_Init` | `prg.asm:323-364` | `State_0_Init` | `src/core/mjk_state_machine.c:23-36` |
| `InitPlayerStruct` | `prg.asm:365-378` | `Jack_Init` | `src/core/mjk_player.c:69-101` |
| `GameState_1_RoundIntro` | `prg.asm:7663-7681` | `State_1_RoundIntro` | `src/core/mjk_state_machine.c:38-52` |
| `GameState_2_LoadRoom` | `prg.asm:3130-3256` | `State_2_LoadRoom` | `src/core/mjk_state_machine.c:355-436` |
| `GameState_3_DrawScreen` | `prg.asm:379-444` | `State_3_DrawScreen` | `src/core/mjk_state_machine.c:438-451` |
| `GameState_4_EnterRoom` | `prg.asm:445-626` | `State_4_EnterRoom` | `src/core/mjk_state_machine.c:453-474` |
| `GameState_5_Main` | `prg.asm:647-671` | `State_5_MainLoop` | `src/core/mjk_state_machine.c:476-698` |
| `GameState_6_TimeOver` | `prg.asm:672-697` | `State_6_TimeOver` | `src/core/mjk_state_machine.c:700-712` |
| `GameState_7_GoToTortureRoom` | `prg.asm:698-775` | `State_7_TortureRoom` | `src/core/mjk_state_machine.c:714-755` |
| `GameState_8_RoundClear` | `prg.asm:776-1037` | `State_8_RoundClear` | `src/core/mjk_state_machine.c:757-775` |
| `GameState_9_ExitRoom` | `prg.asm:7294-7375` | `State_9_ExitRoom` | `src/core/mjk_state_machine.c:777-814` |
| `GameState_A_GameOver` | `prg.asm:1038-1094` | `State_A_GameOver` | `src/core/mjk_state_machine.c:816-835` |
| `GameState_B_AnimateDoorOpen` | `prg.asm:1122-1134` | `State_B_AnimateDoorOpen` | `src/core/mjk_state_machine.c:837-861` |
| `GameState_C_Ending` | `prg.asm:7706-7927` | `State_C_Ending` | `src/core/mjk_state_machine.c:863-881` |
| `TitleScreenHandler` | `prg.asm:7473-7662` | `RenderTitleScreen` / Bloque Título | `src/core/mjk_game.c:502-559`, `src/core/mjk_game.c:1506-1590` |
| `DrawFullScreen` | `prg.asm:2724-2768` | `MjkRoom_BuildNametable` / Renderizado de sala | `src/core/mjk_room_decoder.c:80-133` |

### 2.2. Cinemática, Física de Jack y Colisiones 9-Probe (CTR-02)
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C Equivalente | Archivo y Líneas en `src/core/` |
| :--- | :--- | :--- | :--- |
| `MainSub_1` | `prg.asm:1304-1639` | `Jack_Update` / `Jack_UpdateMultiRoom` | `src/core/mjk_player.c:513-538` |
| `SetPlayerVelocityFromX` | `prg.asm:627-646` | `Jack_UpdateHorizontalVelocity` | `src/core/mjk_player.c:103-155` |
| `ApplyPlayerXAndYVelocity` | `prg.asm:1713-1744` | `Jack_UpdateVerticalPhysics` | `src/core/mjk_player.c:157-246` |
| `BigBeefySubHandlesBGColliding` | `prg.asm:4615-4859` | `mjk_player_update_collision` / `Jack_ResolveCollisions` | `src/core/mjk_player.c:373-511` |
| `DrawPlayer` | `prg.asm:1640-1657` | `Jack_DetermineSprite` / `Jack_AssembleMetasprite` | `src/core/mjk_player.c:540-649` |
| `ClearFourSpritesInY` | `prg.asm:2033-2049` | Ocultar Jack de OAM en salida de sala | `src/core/mjk_game.c:1215-1230` |
| `UseMightyCoin` | `prg.asm:1745-1773` | `mjk_player_use_mighty_coin` / `mjk_game_use_mighty_coin` | `src/core/mjk_player.c:693-708`, `src/core/mjk_game.c:1654-1667` |
| `UpdateMightyPalette` | `prg.asm:1774-1782` | `MjkGame_UpdateJackPalette` | `src/core/mjk_game.c:47-64` |

### 2.3. Decodificador de Salas, Puertas, Bombas y Cofres (CTR-03)
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C Equivalente | Archivo y Líneas en `src/core/` |
| :--- | :--- | :--- | :--- |
| `LoadRoomStuff` / `HandleDrawHalfRow` | `prg.asm:2630-2723` | `MjkRoom_DecodeGeometry` | `src/core/mjk_room_decoder.c:14-57` |
| `ReadRoomData` / `LoadRoomData` | `prg.asm:3257-3540` | `MjkRoom_ReadRoomData` / `MjkRoom_LoadWithRAM` | `src/core/mjk_room_decoder.c:683-820`, `src/core/mjk_room_decoder.c:946-1068` |
| `CheckAndMaybeOpenDoor` | `prg.asm:6741-7009` | `MjkRoom_CheckDoorCondition` | `src/core/mjk_room_decoder.c:280-369` |
| `HandleOpenDoorMaybe` | `prg.asm:7030-7091` | `MjkRoom_HandleOpenDoorMaybe` | `src/core/mjk_room_decoder.c:240-273` |
| `OpenBombRoomDoor` | `prg.asm:5352-5368` | `MjkRoom_OpenBombRoomDoor` | `src/core/mjk_room_decoder.c:275-278` |
| `AnimateDoor` / `AnimateDoorOpen` | `prg.asm:2157-2179`, `7219-7293` | `State_B_AnimateDoorOpen` / `BlitCanonicalRoomDoors` | `src/core/mjk_state_machine.c:837-861`, `src/core/mjk_game.c:561-650` |
| `JTPickup_Bomb` | `prg.asm:4970-5167` | `MjkRoom_CollectBombCanonical` | `src/core/mjk_room_decoder.c:473-681` |
| `MaybeOpenHiddenBlock` | `prg.asm:5392-5461` | `MjkGame_OpenChest` | `src/core/mjk_game.c:80-139` |
| `JTPickup_Chest` / `JTPickup_Chest2` | `prg.asm:5259-5311` | `MjkGame_HitChestFromBelow` / `MjkGame_HitSecretBlock` | `src/core/mjk_game.c:141-188` |
| `MaybePickupItem` / `HandleCollectedItem` | `prg.asm:3999-4135`, `4860-5258` | `MjkGame_CollectChestItem` | `src/core/mjk_game.c:190-292` |
| `sub_A270` / `sub_A29C` | `prg.asm:5647-5750` | `MjkGame_SpawnBombPuff` / `MjkGame_UpdateBombPuffs` | `src/core/mjk_room_decoder.c:425-467` |

### 2.4. IA de Enemigos, Spawns y Power Coins (CTR-04)
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C Equivalente | Archivo y Líneas en `src/core/` |
| :--- | :--- | :--- | :--- |
| `HandleEnemySpawnTimer` / `InitEnemy` | `prg.asm:8171-8312` | `MjkEnemy_Init` / `MjkEnemy_TickSpawning` | `src/core/mjk_enemy.c:104-118`, `src/core/mjk_enemy.c:150-192` |
| `ChooseEnemySpawnPoint` | `prg.asm:8689-8827` | `MjkEnemy_Spawn` | `src/core/mjk_enemy.c:124-148` |
| `RunEnemyLogic` | `prg.asm:8458-8659` | `MjkEnemy_UpdateAll` | `src/core/mjk_enemy.c:214-484` |
| `TransformMummy` | `prg.asm:9362-9381` | `MjkEnemy_MutateMummy` | `src/core/mjk_enemy.c:194-206` |
| `CheckObjectCollision` | `prg.asm:3959-3998` | `MjkEnemy_CheckJackCollisions` | `src/core/mjk_enemy.c:486-540` |
| `SpawnPowerCoin` / `TurnEnemiesIntoCoins` | `prg.asm:8978-9046` | `MjkEnemy_ActivatePowerCoin` / `mjk_game_activate_power_coin` | `src/core/mjk_enemy.c:208-212`, `src/core/mjk_game.c:1669-1702` |

### 2.5. HUD, Aritmética BCD, Codicia y Finales (CTR-05)
| Subrutina 6502 | Líneas en `asm/src/prg.asm` | Función C Equivalente | Archivo y Líneas en `src/core/` |
| :--- | :--- | :--- | :--- |
| `AddAToGDV` | `prg.asm:1095-1105` | `MjkGame_AddAToGDV` / `MjkHUD_UpdateGDV` | `src/core/mjk_game.c:1758-1769`, `src/core/mjk_hud.c:81-94` |
| `BCDCheckFirstDigit` / `AddScore` | `prg.asm:1106-1121`, `2436-2550` | `MjkBCD_AddByte` / `MjkBCD_AddScore` | `src/core/mjk_hud.c:13-31`, `src/core/mjk_hud.c:49-79` |
| `BCDSub1FromA` / `HandleStageTimer` | `prg.asm:1191-1237`, `2111-2128` | `MjkBCD_Sub1` | `src/core/mjk_hud.c:33-47` |
| `EndingDoorChecks` | `prg.asm:7376-7427` | `MjkGame_EvaluateEnding` / `MjkHUD_EvaluateEnding` | `src/core/mjk_game.c:1805-1822`, `src/core/mjk_hud.c:96-111` |
| `GoToTheTortureRoom` | `prg.asm:4957-4969` | `MjkGame_TriggerTortureRoom` | `src/core/mjk_game.c:1824-1840` |

---

## 3. Mapeo de Tablas de Datos ROM (`asm/src/data.asm` y `prg.asm`) <---> Tablas en C (`src/core/`)

| Tabla en Ensamblador | Archivo y Líneas ASM | Tabla en C Equivalente | Archivo y Líneas en `src/core/` | Descripción |
| :--- | :--- | :--- | :--- | :--- |
| `SpriteAnimationTable` | `asm/src/data.asm:6-95` | Tablas de sprites de enemigos y Jack | `src/core/mjk_enemy.c:15-75`, `src/core/mjk_player.c:15-67` | Secuencias de tiles CHR y atributos de animación |
| `DifficultyTable` | `asm/src/data.asm:116-168` | `s_difficulty_table[16]` | `src/core/mjk_enemy.c:77-102` | Cadencias de spawn, velocidad y mutación por ronda |
| `MainPalette` / `BackgroundPaletteSets` | `asm/src/data.asm:258-305` | `MjkRoom_LoadAuthenticPalettes` | `src/core/mjk_room_decoder.c:135-165` | Paletas de hardware PPU de fondo y sprites |
| `DoorEntryXYPositionTable` | `asm/src/data.asm:322-339` | `g_door_entry_xy_table[16]` | `src/core/mjk_room_data.c:1601-1662` | Coordenadas mundo `(X, Y)` de spawn de Jack al entrar |
| `DoorPositionTileTable` | `asm/src/prg.asm:3823-3839` | `g_door_position_tile_table[16]` | `src/core/mjk_room_data.c:1581-1599` | Coordenadas en rejilla y metatiles cerrado/abierto |
| `AdjacentRoomsTable` | `asm/src/data.asm:1704-1962` | `g_adjacent_rooms[257]` | `src/core/mjk_room_data.c:820-1319` | Matriz topológica 257x4 de conexión entre salas |
| `EnemySpawnPositionTable` | `asm/src/data.asm:1963-2226` | Tabla de puntos de spawn | `src/core/mjk_enemy.c:124-188` | Puntos canónicos de nacimiento de momias y pájaros |
| `RoomDataPointers` | `asm/src/data.asm:2227-2485` | `g_room_data_table[256]` | `src/core/mjk_room_data.c:1321-1579` | Punteros al bytecode de objetos de las salas |
| `MetatileDefinitions` | `asm/src/data.asm:1100-1703` | `g_metatile_definitions[154]` | `src/core/mjk_room_data.c:663-818` | Matriz 2x2 de tiles CHR por cada metatile 16x16 |

---

## 4. Protocolo Obligatorio para Subagentes (`parity_solver`)

Para evitar divagaciones y ahorrar tokens en las iteraciones de la Fase 7:

1. **Paso 1 Obligatorio**: Al recibir un reporte de divergencia con una variable RAM (ej. `$03BF`, `$0044`, `$03D9`), el agente **NO debe ejecutar búsquedas generales en el disco**.
2. **Paso 2**: Abrir **únicamente** este archivo (`research/CROSS_REFERENCE_INDEX.md`) y consultar la fila de la variable o subrutina en conflicto.
3. **Paso 3**: Usar `read_file` con `StartLine` y `EndLine` exactos sobre `asm/src/prg.asm` y el archivo de `src/core/` o `include/` indicado en las tablas.
4. **Paso 4**: Aplicar el parche, recompilar con `cmd.exe /c "build.bat build"` y validar con `tools/hierarchical_parity_oracle.py`.
