# Mapa de Correspondencia: ASM 6502 (`REV_US`) ↔ Source Port C11

Diccionario bidireccional entre las direcciones RAM y subrutinas del cartucho original (`asm/src/`) y las estructuras/funciones actuales del motor en C11 (`include/mjk/` y `src/`).

*(Este archivo es consultado también por `python tools/asm_query.py --xref <término>`.)*

---

## 1. Memoria RAM 6502 (`$0000–$07FF`) ↔ Campos C11 (`MjkGame`)

| Dirección RAM | Símbolo (`asm/src/ram.asm`) | Semántica | Campo C11 Actual | Header (`include/mjk/`) |
| :---: | :--- | :--- | :--- | :--- |
| **`$0043`** | `InGameFlag` (`ram.asm:104`) | `0` = Título/Copyright, `1` = Partida activa | `game->mode` (`MjkScreenMode`) | `game.h`, `types.h` |
| **`$0044`** | `GameState` (`ram.asm:108`) | Máquina de estados maestra (`0..$C`) | `game->state` (`MjkGameState`) | `game.h`, `types.h` |
| **`$0045`** | `TitleScreenState` (`ram.asm:110`) | Subestado de Copyright/Título/Caída | `game->title.state` (`MjkTitleState`) | `game.h`, `types.h` |
| **`$006A..$006B`** | `EnemyCoinTimer` (`ram.asm:172`) | Temporizador Power Coin (`320f`) | `game->enemies.transmute_timer` | `enemies.h` |
| **`$0088/$0089`** | `CopyrightScreenTimer` / `TitleScreenTimer` | Timers de Copyright (`256f`) y Título (`512f`) | `game->title.timer` | `game.h` |
| **`$00A0..$00A3`** | `PlayerScore` (`ram.asm:240-244`) | Puntuación (entero `uint32_t` en C11) | `game->scoring.score` | `scoring.h` |
| **`$00A4`** | `StageTimer` (`ram.asm:245-247`) | Temporizador de fase (`60` s) | `game->scoring.timer_seconds` | `scoring.h` |
| **`$00A5`** | `TimerFrames` (`ram.asm:248-250`) | Divisor de reloj de `256` frames | `game->scoring.timer_frames` | `scoring.h` |
| **`$00AE`/`$00AF`** | `FireBombsCollected` / `2` (`ram.asm:264-266`) | Bombas encendidas y total en Bomb Room (`23`) | `game->fire_bombs_collected`, `game->objects` | `game.h`, `objects.h` |
| **`$00B1`** | `AliveEnemyCount` (`ram.asm:268-269`) | Enemigos vivos simultáneos (máx. 4) | `game->enemies.alive_count` | `enemies.h` |
| **`$00BC`/`$03BE`** | `RoomStatusFlags` (`ram.asm:283`) | Flags de topología (`RF_*`) | `game->world.camera.room_flags` | `world.h` |
| **`$00FA`** | `APressCounter` (`ram.asm:386`) | Contador de saltos para mutación `(A % 6) + 2` | `game->player.jump_press_count` | `player.h` |
| **`$0319`** | `GreedyJumpsRemaining` | Saltos restantes en Torture Room (`50` US) | `game->greedy_jumps_left` | `game.h` |
| **`$033B`/`$033D`/`$033F`** | `EntryDoorType` + backups | Puerta de entrada y respaldos | `game->entry_door_type`, `entry_door_backup`, `torture_return_door_type` | `game.h` |
| **`$0367`** | `GoldCoinsCollected` | Monedas de oro (`40` → Extra Coin `1UP`) | `game->scoring.gold_coins` | `scoring.h` |
| **`$0391`** | `PlayerCrystalBallsCollected` | Máscara de Bolas de Cristal (bits 0/1) | `game->crystal_balls_mask` | `game.h` |
| **`$0395`/`$0399`/`$039A`** | `DoorCheckFlag_1E/1F/09` | Flags persistentes de puertas warp | `game->door_1e_deaths`, `door_flag_1f`, `door_flag_09` | `game.h` |
| **`$039B`/`$039C`** | `PlayerGDV` / `HighGDV` | GDV actual (`47..99`) y récord | `game->scoring.gdv`, `game->scoring.high_gdv` | `scoring.h` |
| **`$03BF`/`$03C0`** | `CurrentRoomID` / `Checkpoint` | Sala activa (`0x00..0xFF`) y checkpoint | `game->current_room_id`, `game->checkpoint_room_id` | `game.h` |
| **`$03C1..$03C4`** | `ScrollX` / `ScrollY` | Desplazamiento de cámara en `float` | `game->world.camera.scroll_x`, `scroll_y` | `world.h` |
| **`$03CE..$03DF`** | `PlayerStruct` (`ram.asm:1223-1254`) | Posición/velocidad `float`, colisiones y sprite | `game->player` (`MjkPlayer`) | `player.h` |
| **`$03E0..$03E4`** | `MightyLevel`, `PressesLeft`, `Coins`, `Mult`, `Lives` | Poder Mighty `0..3`, monedas, multiplicador y vidas | `game->mighty_level`, `mighty_presses_left`, `mighty_coins`, `scoring.score_multiplier`, `scoring.lives` | `game.h`, `scoring.h` |
| **`$03E5`** | `PlayerSecretCoins` | *Secret Coins* (`>= 5` → Final 3) | `game->secret_coins` | `game.h` |
| **`$03E6..$03E7`** | `byte_3E6`/`byte_3E7` | Progreso de curva de salto de 64 pasos | `game->player.jump_progress` | `player.h` |
| **`$0432..$0556`** | `SoundQueue` + 8 canales | Cola de sonido y 8 canales virtuales (4 BGM + 4 SFX) | `game->sound_queue`, `MjkAudioEngine` | `game.h`, `audio.h` |
| **`$0603..$0692`** | `RoomDataRAM` + `byte_683` | Persistencia de 8 ranuras + scratch | `game->world.persist` (`MjkSectionPersist`) | `world.h` |
| **`$06A6..$0785`** | `Object0Struct..Object7Struct` | 8 ranuras de enemigos y coleccionables | `game->enemies` (`MjkEnemySystem`) | `enemies.h` |

---

## 2. Subrutinas 6502 (`asm/src/`) ↔ Módulos C11 (`src/`)

| Subrutina 6502 | Ubicación ASM | Equivalente C11 | Archivo C11 |
| :--- | :--- | :--- | :--- |
| `RESET` / `NMI` / `GameHandler` | `prg.asm:8-126, 300-775` | `MjkGame_Init`, `MjkGame_StepFrame` | `src/game/game.c` |
| `TitleScreenHandler` (`US_0/1`, `0..3`) | `prg.asm:7473-7660` | `MjkScreens_StepTitle` | `src/game/screens.c` |
| `GameState_8_RoundClear`, `A_GameOver`, `C_Ending` | `prg.asm:776-1091, 7706-7924` | `MjkScreens_StepRoundClear`, `StepGameOver`, `StepEnding` | `src/game/screens.c` |
| `AddScore`, `HandleStageTimer`, `sub_8FB1`, `AddAToGDV` | `prg.asm:1050-1117, 1220-1232, 2436-2573` | `MjkScoring_AddPoints`, `TickTimer`, `SpawnPopup`, `AddGdv` | `src/game/scoring.c` |
| `ReadRoomData`, `LoadLayoutChunk`, `AdjacentRoomsTable` | `prg.asm:2579-3535` | `MjkAssets_Load`, `MjkWorld_LoadRoom`, `UpdateCamera` | `src/assets/asset_bank.c`, `src/game/world.c` |
| `DoSomethingWithObjYVel`, `sub_8AE8`, `sub_9B6A` | `prg.asm:1397-1741, 4480-4611` | `MjkPlayer_Update`, `MjkWorld_IsPointSolid` | `src/game/player.c`, `src/game/world.c` |
| `JTPickup_Bomb`, `JTPickup_Chest`, `UseMightyCoin` | `prg.asm:1745-1779, 4860-5495` | `MjkObjects_Update` | `src/game/objects.c` |
| `MaybeHandleDoorOpening`, `CheckAndMaybeOpenDoor`, `OpenBombRoomDoor` | `prg.asm:5312-5365, 6696-7425` | `MjkDoors_Update`, `MjkGame_ExitRoom` | `src/game/doors.c`, `src/game/game.c` |
| `HandleEnemySpawnTimer`, `TransformMummy`, `TurnEnemiesIntoCoins` | `prg.asm:8150-9575` | `MjkEnemies_Update`, `TurnIntoCoins` | `src/game/enemies.c` |
| `WriteSprite`, `DrawScoreAndCoinCount`, `RestoreDefaultPalettes` | `prg.asm:1783-1950, 2267-2374` | `MjkRenderer_DrawFrame` | `src/render/renderer.c` |
| `HandleSound`, `HandleSoundChannel`, `WriteSoundRegisters` | `music-engine.asm:1-811` | `MjkAudio_StepFrame`, `MjkSynth_Step` | `src/audio/audio_engine.c`, `src/audio/synth_2a03.c` |
