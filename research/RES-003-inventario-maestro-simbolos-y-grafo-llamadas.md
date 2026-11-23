# Informe Técnico: Inventario Maestro de Símbolos, Tablas de Salto (`JumpTable`) y Grafo de Llamadas (`asm/`)

- **ID de Investigación**: `RES-003`
- **Tarea Asociada**: `TASK-RES-002`
- **Área Temática**: `01 - Núcleo, Memoria y Arquitectura General`
- **Fecha**: 2026-10-22
- **Autor / Agente**: Antigravity (Orchestrator) + `tools/extract_asm_inventory.py`
- **Estado**: `[COMPLETADA]`

---

## 1. Objetivo y Alcance

Proporcionar el **Índice Maestro Determinista** de todo el código ensamblador 6502 en `asm/`, extraído automáticamente mediante `asm/tools/asm6f_64.exe` y el analizador estático [extract_asm_inventory.py](file:///c:/Users/kanel/Documents/mjk/tools/extract_asm_inventory.py) (cuyo volcado consultable en JSON reside en [asm_inventory.json](file:///c:/Users/kanel/Documents/mjk/tools/asm_inventory.json)).

- **Total de Símbolos Exportados por `asm6f_64.exe`**: **1764**
- **Total de Subrutinas Indexadas (`prg.asm` + `music-engine.asm`)**: **294**
- **Total de Tablas de Despacho Indirecto (`JSR JumpTable`)**: **15**
- **Total de Variables RAM Etiquetadas (`ram.asm`)**: **338**

---

## 2. Catálogo Completo de Tablas de Despacho Indirecto (`JSR JumpTable`)

En *Mighty Bomb Jack*, las máquinas de estado principales se despachan mediante `JSR JumpTable`. La siguiente tabla documenta las **12 tablas de salto indirecto** con su variable selectora y sus funciones destino:

| # | Archivo y Línea | Subrutina Padre | Variable / Selector (`A`) | # Destinos | Lista Ordenada de Funciones Destino (`0..N`) |
| :---: | :--- | :--- | :--- | :---: | :--- |
| `1` | `prg.asm:292` | `MainLogicHandler` | `InGameFlag` | `2` | `0:TitleScreenHandler`, `1:GameHandler` |
| `2` | `prg.asm:302` | `GameHandler` | `GameState` | `13` | `0:GameState_0_Init`, `1:GameState_1_RoundIntro`, `2:GameState_2_LoadRoom`, `3:GameState_3_DrawScreen`, `4:GameState_4_EnterRoom`, `5:GameState_5_Main`, `6:GameState_6_TimeOver`, `7:GameState_7_GoToTortureRoom`, `8:GameState_8_RoundClear`, `9:GameState_9_ExitRoom`, `10:GameState_A_GameOver`, `11:GameState_B_AnimateDoorOpen`, `12:GameState_C_Ending` |
| `3` | `prg.asm:786` | `GameState_8_RoundClear` | `RoundClearState` | `4` | `0:RoundClear_0`, `1:RoundClear_1`, `2:RoundClear_2`, `3:RoundClear_3` |
| `4` | `prg.asm:3357` | `HandleRoomDataObject` | `A` | `15` | `0:JT_94AD_0`, `1:JT_94AD_multi`, `2:JT_94AD_multi`, `3:JT_94AD_multi`, `4:JT_94AD_multi`, `5:JT_94AD_5_6`, `6:JT_94AD_5_6`, `7:JT_94AD_7`, `8:JT_94AD_8_Door`, `9:JT_94AD_multi`, `10:JT_94AD_multi`, `11:JT_94AD_rts`, `12:JT_94AD_rts`, `13:JT_94AD_rts`, `14:JT_94AD_E` |
| `5` | `prg.asm:4009` | `CheckObjectCollision` | `A` | `17` | `0:JT_98A4_multi`, `1:JT_98A4_multi`, `2:JT_98A4_multi`, `3:JT_98A4_multi`, `4:JT_98A4_multi`, `5:JT_98A4_multi`, `6:JT_98A4_multi`, `7:JT_98A4_multi`, `8:JT_98A4_8_PowerCoin`, `9:JT_98A4_9_ExtraCoin`, `10:JT_98A4_A_SecretCoin`, `11:JT_98A4_B_BonusStatue`, `12:JT_98A4_C`, `13:JT_98A4_multi`, `14:JT_98A4_E`, `15:JT_98A4_multi`, `16:JT_98A4_10_Brother` |
| `6` | `prg.asm:4834` | `BigBeefySubHandlesBGColliding` | `MaybeCollectedThing` | `20` | `0:MaybePickupItem`, `1:MaybePickupItem`, `2:MaybePickupItem`, `3:MaybePickupItem`, `4:MaybePickupItem`, `5:JTPickup_Bomb`, `6:MaybePickupItem`, `7:MaybePickupItem`, `8:MaybePickupItem`, `9:MaybePickupItem`, `10:MaybePickupItem`, `11:JTPickup_Family`, `12:JTPickup_Family`, `13:JTPickup_Family`, `14:JTPickup_Beelzebub`, `15:MaybePickupItem`, `16:JTPickup_Chest`, `17:JTPickup_Chest2`, `18:JTPickup_Door`, `19:JTPickup_Chest` |
| `7` | `prg.asm:4874` | `MaybePickupItem` | `A` | `16` | `0:JTPickup_Bag100`, `1:JTPickup_Bag300`, `2:JTPickup_Bag1000`, `3:JTPickup_GoldCoin`, `4:JTPickup_MightyDrink`, `5:JT_9DF2_rts`, `6:JTPickup_Sphinx`, `7:JTPickup_TecmoPlate`, `8:JTPickup_MightyCoin`, `9:JTPickup_CrystalBall1`, `10:JTPickup_CrystalBall2`, `11:JT_9DF2_rts`, `12:JT_9DF2_rts`, `13:JT_9DF2_rts`, `14:JT_9DF2_rts`, `15:JT_9DF2_F_rts` |
| `8` | `prg.asm:6743` | `MaybeHandleDoorOpening` | `MaybeDoorTypeOrSphinx` | `65` | `0:DoorCond_rts`, `1:DoorCond_rts`, `2:DoorCond_02_JumpedOn`, `3:DoorCond_rts`, `4:DoorCond_rts`, `5:DoorCond_rts`, `6:DoorCond_rts`, `7:DoorCond_XX_Mighty3`, `8:DoorCond_rts`, `9:DoorCond_09_FBWarp2`, `10:DoorCond_rts`, `11:DoorCond_rts`, `12:DoorCond_0C_AllFireBombs`, `13:DoorCond_rts`, `14:DoorCond_XX_Mighty3`, `15:DoorCond_rts`, `16:DoorCond_rts`, `17:DoorCond_rts`, `18:DoorCond_rts`, `19:DoorCond_rts`, `20:DoorCond_rts`, `21:DoorCond_rts`, `22:DoorCond_rts`, `23:DoorCond_XX_Mighty3`, `24:DoorCond_XX_Mighty3`, `25:DoorCond_1D_FBWarp1`, `26:DoorCond_1A_US_rts`, `27:DoorCond_XX_Mighty3`, `28:DoorCond_rts`, `29:DoorCond_1D_FBWarp1`, `30:DoorCond_1E_R5R11Warp`, `31:DoorCond_1F_R11WarpBack`, `32:DoorCond_20_US`, `33:DoorCond_rts`, `34:DoorCond_rts`, `35:DoorCond_02_JumpedOn`, `36:DoorCond_rts`, `37:DoorCond_rts`, `38:DoorCond_rts`, `39:DoorCond_rts`, `40:DoorCond_XX_Mighty3`, `41:DoorCond_rts`, `42:DoorCond_09_FBWarp2`, `43:DoorCond_0A_Timer30`, `44:DoorCond_rts`, `45:DoorCond_0C_AllFireBombs`, `46:DoorCond_rts`, `47:DoorCond_0E_Timer60`, `48:DoorCond_rts`, `49:DoorCond_rts`, `50:DoorCond_rts`, `51:DoorCond_rts`, `52:DoorCond_rts`, `53:DoorCond_rts`, `54:DoorCond_rts`, `55:DoorCond_rts`, `56:DoorCond_XX_Mighty3`, `57:DoorCond_XX_Mighty3`, `58:DoorCond_19_TensScore30`, `59:DoorCond_1A_TensScore70`, `60:DoorCond_XX_Mighty3`, `61:DoorCond_rts`, `62:DoorCond_1D_FBWarp1`, `63:DoorCond_1E_R5R11Warp`, `64:DoorCond_1F_R11WarpBack` |
| `9` | `prg.asm:7380` | `EndingDoorChecks` | `A` | `4` | `0:_EndCheckCrystalBall1`, `1:_EndCheckCrystalBall2`, `2:_EndCheck5SecretCoins`, `3:_EndGame` |
| `10` | `prg.asm:7477` | `TitleScreenHandler` | `TitleScreenState` | `7` | `0:TitleScreen_US_0`, `1:TitleScreen_US_1`, `2:TitleScreen_0`, `3:TitleScreen_1`, `4:BypassStart`, `5:TitleScreen_2`, `6:TitleScreen_3` |
| `11` | `prg.asm:7708` | `GameState_C_Ending` | `EndingState` | `5` | `0:EndingState_0`, `1:EndingState_1`, `2:EndingState_2`, `3:EndingState_3`, `4:EndingState_4` |
| `12` | `prg.asm:8266` | `RunEnemyInit` | `EnemySpeedModifier` | `17` | `0:ObjInit_0_Mummy`, `1:Obj_null`, `2:ObjInit_2_Hanezo`, `3:ObjCode_3_GejiShogun`, `4:ObjInit_4_Gameido`, `5:ObjInit_5_Dokuron`, `6:ObjInit_6_Desufa`, `7:ObjInit_7_Horus`, `8:ObjInit_8_PowerCoin`, `9:ObjInit_9_E_ExtraBonusCoin`, `10:Obj_null`, `11:Obj_null`, `12:ObjInit_C_Balloon`, `13:ObjInit_D_F_MummyAlt`, `14:ObjInit_9_E_ExtraBonusCoin`, `15:ObjInit_D_F_MummyAlt`, `16:ObjInit_10_Brother` |
| `13` | `prg.asm:8465` | `RunEnemyLogic` | `A` | `17` | `0:ObjCode_0_Mummy`, `1:Obj_null`, `2:ObjCode_2_Hanezo`, `3:ObjCode_3_GejiShogun`, `4:ObjCode_4_Gameido`, `5:ObjCode_5_Dokuron`, `6:ObjCode_6_Desufa`, `7:ObjCode_7_Horus`, `8:ObjCode_8_PowerCoin`, `9:ObjCode_9_E_ExtraBonusCoin`, `10:Obj_null`, `11:Obj_null`, `12:Obj_null`, `13:ObjCode_0_Mummy`, `14:ObjCode_9_E_ExtraBonusCoin`, `15:ObjCode_0_Mummy`, `16:ObjCode_10_Brother` |
| `14` | `music-engine.asm:301` | `HandleSoundCommand` | `A` | `8` | `0:SoundCommand_8x_9x`, `1:SoundCommand_8x_9x`, `2:SoundCommand_Ax_Bx`, `3:SoundCommand_Ax_Bx`, `4:SoundCommand_Cx`, `5:SoundCommand_Dx`, `6:SoundCommand_Ex`, `7:SoundCommand_Fx` |
| `15` | `music-engine.asm:425` | `SoundCommand_Fx` | `byte_6` | `8` | `0:SoundCommand_F0`, `1:SoundCommand_F1`, `2:SoundCommand_F2`, `3:SoundCommand_F3`, `4:SoundCommand_F4`, `5:SoundCommand_F5`, `6:SoundCommand_F6_F7`, `7:SoundCommand_F6_F7` |

---

## 3. Subrutinas con Mutación de Pila (`_DoubleReturn` / `PLA; PLA`)

Las siguientes subrutinas alteran la pila de llamadas 6502 para abortar la ejecución de su función padre (crítico al traducir a C x86):

| Subrutina | Dir. ROM | Archivo y Líneas | Tipo de Mutación | Callers |
| :--- | :---: | :--- | :--- | :--- |
| `GameState_7_GoToTortureRoom` | `$8474` | `prg.asm:698-772` | `PLA;PLA (Stack Pop)` | `GameHandler`, `GameState_5_Main` |
| `MainSub_1` | `$8845` | `prg.asm:1304-1636` | `PLA;PLA (Stack Pop)` | `GameState_5_Main` |
| `AnimateDoorClose` | `$8D6B` | `prg.asm:2165-2176` | `PLA;PLA (Stack Pop)` | `GameState_4_EnterRoom`, `GameState_9_ExitRoom` |
| `IfNotTitleScreenRoomDoThings` | `$92F8` | `prg.asm:3077-3126` | `PLA;PLA (Stack Pop)` | `sub_9696`, `BigBeefySubHandlesBGColliding`, `sub_A85F`, `HandleOpenDoorMaybe`, `GameState_9_ExitRoom` |
| `sub_9592` | `$9592` | `prg.asm:3541-3680` | `PLA;PLA (Stack Pop)` | `sub_9696`, `BigBeefySubHandlesBGColliding`, `sub_A85F`, `HandleOpenDoorMaybe`, `GameState_9_ExitRoom` |
| `sub_96F5` | `$96F5` | `prg.asm:3746-3769` | `PLA;PLA (Stack Pop)` | `sub_96B7`, `sub_A875` |
| `sub_9B6A` | `$9B6A` | `prg.asm:4480-4611` | `PLA;PLA (Stack Pop)` | `sub_9A03`, `sub_9A7A` |
| `MaybeDontOpenDoors` | `$AAB4` | `prg.asm:7010-7026` | `PLA;PLA (Stack Pop)` | `DoorCond_02_JumpedOn`, `DoorCond_0A_Timer30`, `DoorCond_0E_Timer60`, `DoorCond_XX_Mighty3` |
| `MaybeConsiderOffscreen` | `$B781` | `prg.asm:8548-8566` | `PLA;PLA (Stack Pop)` | `MaybeRemoveIfOffscreen` |
| `sub_BA00` | `$BA00` | `prg.asm:8995-9015` | `PLA;PLA (Stack Pop)` | `AddScore`, `MaybeSpawnThingFromObject`, `HandleEnemySpawnTimer`, `SpawnPowerCoin` |
| `SoundCommand_Cx` | `$B2F0` | `music-engine.asm:355-367` | `PLA;PLA (Stack Pop)` | `HandleSoundCommand` |
| `SoundCommand_F6_F7` | `$B432` | `music-engine.asm:605-630` | `PLA;PLA (Stack Pop)` | `SoundCommand_Fx` |

---

## 4. Inventario de las 59 Subrutinas Anónimas (`sub_XXXX`) y 22 Tentativas (`Maybe...`) con Pre-Diagnóstico

Esta tabla concentra las **81 subrutinas pendientes de bautizo/confirmación semántica**, ya cruzadas con quién las llama (`Callers`), a quién llaman (`Callees`), qué registros reciben y qué variables de RAM leen/escriben:

| Subrutina | Categoría | Dir. ROM | Líneas (`prg.asm`) | In Regs | Callers | Callees | ZP `$00-$14` | RAM Leída (`R`) | RAM Escrita (`W`) |
| :--- | :---: | :---: | :---: | :---: | :--- | :--- | :--- | :--- | :--- |
| `LoseALifeMaybeGameOver` | `TENTATIVE` | `$8757` | `1135-1151` | `-` | `GameState_6_TimeOver`, `MainSub_1` | `MaybeGoBackToOldRoom` | - | `PlayerLives` | `GameState`, `PlayerLives`, `PlayerMightyLevel`, `PlayerMightyLevelPressesLeft`, `TimerStatusMaybe` |
| `MaybeGoBackToOldRoom` | `TENTATIVE` | `$8777` | `1155-1163` | `-` | `GameState_7_GoToTortureRoom`, `LoseALifeMaybeGameOver` | - | - | `CheckpointRoomIDMaybe`, `EntryDoorTypeBackup` | `CurrentRoomID`, `EntryDoorType`, `GameState` |
| `MaybePause` | `TENTATIVE` | `$8789` | `1167-1187` | `-` | `GameState_5_Main` | `QueueSound` | - | `Joypad1_Immediate`, `Joypad1_ImmediateCopy`, `MaybePauseFlag` | `MaybePauseFlag` |
| `sub_8AE8` | `AUTO_IDA` | `$8AE8` | `1689-1709` | `A` | `DoSomethingWithObjYVel`, `sub_B024` | `NegateYYXX` | - | - | - |
| `DrawScoreDigitMaybe` | `TENTATIVE` | `$8BA9` | `1829-1865` | `A,X` | `DrawScore` | - | `byte_1` | `CurrentSpriteIndex`, `TempSpriteX`, `TempSpriteY` | `CurrentSpriteIndex`, `SpriteDMAArea`, `TempSpriteX` |
| `sub_8FB1` | `AUTO_IDA` | `$8FB1` | `2551-2573` | `A` | `sub_A29C` | - | - | `RoomStatusFlags`, `ScoreMultiplier` | `byte_30B`, `byte_30C`, `byte_30D`, `byte_30E` |
| `MaybeLoadRoomFlags` | `TENTATIVE` | `$8FD7` | `2579-2626` | `A` | `GameState_3_DrawScreen`, `HandleSpawnsAndMore`, `MainSub_3`, `GameState_9_ExitRoom` | `LoadPointerTo050` | - | `RoomStatusFlags`, `off_3D`, `word_50` | `RoomStatusFlags`, `off_3D` |
| `LoadLayoutChunk` | `TENTATIVE` | `$908A` | `2703-2720` | `A` | `DrawFullScreen`, `sub_9716`, `MainSub_3`, `sub_A875` | `LoadPointerTo050` | `byte_6`, `byte_7` | `word_50` | - |
| `sub_9276` | `AUTO_IDA` | `$9276` | `2988-3018` | `A` | `MaybeSpawnThingFromObject`, `sub_A270`, `InitPowerCoin`, `SpawnLocFunc1` | - | `byte_F` | `byte_3C2`, `byte_3C4` | `byte_92`, `byte_94` |
| `sub_92AD` | `AUTO_IDA` | `$92AD` | `3022-3054` | `A,X` | `sub_96B7`, `MaybePickupItem`, `sub_A18E`, `AnimateDoor` | `sub_92E2` | `byte_9` | `RoomStatusFlags`, `byte_3EF`, `byte_3F0`, `byte_F4`, `off_3D` | `byte_3EF`, `byte_3F0` |
| `sub_92E2` | `AUTO_IDA` | `$92E2` | `3058-3071` | `A` | `sub_92AD`, `MainSub_3` | - | - | `byte_3EF` | `byte_3EF`, `byte_3F0` |
| `sub_9592` | `AUTO_IDA` | `$9592` | `3541-3680` | `-` | `sub_9696`, `BigBeefySubHandlesBGColliding`, `sub_A85F`, `HandleOpenDoorMaybe` | - | `byte_0`, `byte_14` | `MaybeSphinxFlags`, `RoomDataRAM`, `RoomStatusFlags`, `UnknownDoorFlag` | `MaybeCollectedThing`, `MaybeTileCollected`, `RoomDataRAM`, `SphinxCollectedMaybe`, `UnknownDoorFlag` |
| `sub_9696` | `AUTO_IDA` | `$9696` | `3684-3699` | `-` | `DrawFullScreen` | `IfNotTitleScreenRoomDoThings`, `sub_9592`, `sub_96B7` | - | `CurrentRoomID`, `PPUCtrlMirror` | `PPUCTRL`, `word_50` |
| `sub_96B7` | `AUTO_IDA` | `$96B7` | `3703-3717` | `X` | `sub_9696` | `sub_96F5`, `sub_92AD`, `HandleTileAttributes`, `sub_9716` | - | `MaybeTileCollected`, `byte_70` | - |
| `sub_96F5` | `AUTO_IDA` | `$96F5` | `3746-3769` | `X` | `sub_96B7`, `sub_A875` | `sub_A270` | - | `MaybeCollectedThing`, `byte_5C`, `byte_70` | - |
| `sub_9716` | `AUTO_IDA` | `$9716` | `3773-3785` | `-` | `DrawFullScreen`, `sub_96B7`, `MaybePickupItem`, `sub_A1A6` | `LoadLayoutChunk` | `byte_6` | `PPUUpdateFlag2` | `PPUUpdateBuffer`, `PPUUpdateFlag2` |
| `sub_9729` | `AUTO_IDA` | `$9729` | `3789-3820` | `-` | `HandlePPUUpdates`, `DrawFullScreen`, `sub_96B7` | - | - | `PPUSTATUS`, `PPUUpdateBuffer`, `byte_3EF`, `byte_3F0` | `PPUADDR`, `PPUDATA`, `PPUUpdateFlag2` |
| `sub_9813` | `AUTO_IDA` | `$9813` | `3913-3955` | `A` | `HandleSpawnsAndMore` | `LoadObjectPointerAPlus1`, `sub_9965`, `sub_BA4E` | `byte_10`, `byte_11` | `EnemyStructPointer`, `byte_332`, `off_78` | `byte_332` |
| `sub_9965` | `AUTO_IDA` | `$9965` | `4152-4226` | `A` | `HandleSpawnsAndMore`, `sub_9813` | `sub_99EA` | `byte_0` | `CollisionFlag`, `byte_30F`, `byte_312`, `off_78` | `CollisionFlag`, `byte_30F`, `byte_314`, `off_78` |
| `sub_99EA` | `AUTO_IDA` | `$99EA` | `4230-4247` | `X` | `HandleSpawnsAndMore`, `sub_9965` | `sub_9A03` | `byte_1` | `RoomStatusFlags` | - |
| `sub_9A03` | `AUTO_IDA` | `$9A03` | `4251-4314` | `-` | `sub_99EA` | `LoadRoomStuff`, `sub_9B39`, `HandleDrawHalfRow`, `sub_9B6A` | `byte_0`, `byte_1`, `byte_7`, `byte_8`, `byte_9` | `CurrentRoomID`, `RoomStatusFlags`, `byte_3C2` | `CurrentRoomIDBackup2` |
| `sub_9A7A` | `AUTO_IDA` | `$9A7A` | `4331-4444` | `-` | - | `LoadRoomStuff`, `sub_9B39`, `HandleDrawHalfRow`, `sub_9B6A` | `byte_0`, `byte_1`, `byte_7`, `byte_8`, `byte_9` | `CurrentRoomID`, `CurrentRoomIDBackup`, `CurrentRoomIDBackup2`, `RoomStatusFlags` | `CurrentRoomIDBackup`, `CurrentRoomIDBackup2` |
| `sub_9B39` | `AUTO_IDA` | `$9B39` | `4448-4474` | `A` | `sub_9A03`, `sub_9A7A` | `BigBeefySubHandlesBGColliding` | `byte_0`, `byte_1`, `byte_9` | `CurrentRoomIDBackup2`, `CurrentRoomIDBackup3`, `CurrentRoomIDBackup4` | `CurrentRoomIDBackup3`, `CurrentRoomIDBackup4`, `MaybeHandleDoorFlag`, `PlayerX_Lo` |
| `sub_9B6A` | `AUTO_IDA` | `$9B6A` | `4480-4611` | `-` | `sub_9A03`, `sub_9A7A` | - | `byte_0`, `byte_1`, `byte_6`, `byte_9`, `byte_A`, `off_4` | `CurrentRoomIDBackup2`, `CurrentRoomIDBackup3`, `PlayerStruct`, `RoomStatusFlags`, `TilePlayerCollidedWith` | `FireTilePlayerDiedTo`, `PlayerStruct`, `TilePlayerCollidedWith`, `byte_30F` |
| `MaybePickupItem` | `TENTATIVE` | `$9DD3` | `4860-4893` | `-` | `BigBeefySubHandlesBGColliding`, `JTPickup_Bomb`, `JTPickup_Family` | `sub_92AD`, `HandleTileAttributes`, `sub_9716`, `JTPickup_Bag100` | - | `RoomDataRAM`, `byte_5A`, `byte_70` | `RoomDataRAM` |
| `MaybeDrawFireBombSprite` | `TENTATIVE` | `$A0C4` | `5369-5388` | `A` | `JTPickup_Bomb` | - | - | - | `SpriteDMAArea` |
| `MaybeOpenHiddenBlock` | `TENTATIVE` | `$A0E3` | `5392-5458` | `A` | `BigBeefySubHandlesBGColliding`, `JTPickup_Chest` | `sub_A18E`, `MaybeSpawnThingFromObject`, `sub_A1A6`, `SpawnPowerCoin` | - | `Collected1UPThisRoundFlag`, `MaybeTempCollectableFlag`, `_MaybeBlockSpawningObj`, `byte_313`, `byte_335` | `_MaybeBlockSpawningObj`, `byte_312`, `byte_335`, `byte_336`, `byte_F0` |
| `MaybeSpawnThingFromObject` | `TENTATIVE` | `$A152` | `5462-5495` | `-` | `MaybeOpenHiddenBlock` | `QueueSound`, `sub_BA00`, `sub_9276`, `InitEnemy` | `byte_12` | `Collected1UPThisRoundFlag`, `byte_92`, `byte_94`, `byte_F0` | `EnemyStructPointer`, `_MaybeBlockSpawningObj` |
| `sub_A18E` | `AUTO_IDA` | `$A18E` | `5499-5511` | `X` | `MaybeOpenHiddenBlock` | `sub_92AD` | - | `byte_311`, `byte_F4` | `byte_312`, `byte_F4` |
| `sub_A1A6` | `AUTO_IDA` | `$A1A6` | `5515-5525` | `A` | `MaybeOpenHiddenBlock` | `HandleTileAttributes`, `sub_9716` | - | `byte_74` | `RoomDataRAM` |
| `sub_A1B9` | `AUTO_IDA` | `$A1B9` | `5529-5555` | `-` | `HandleSpawnsAndMore` | - | - | `PlayerStruct`, `RoomStatusFlags`, `byte_314`, `byte_3C4`, `off_78` | `PlayerStruct`, `byte_3C4`, `byte_3E7`, `off_78` |
| `sub_A1EB` | `AUTO_IDA` | `$A1EB` | `5559-5609` | `-` | `HandleSpawnsAndMore` | - | - | `CollisionFlag`, `PlayerStruct`, `PlayerYPosHi`, `RoomStatusFlags`, `byte_3C4` | `CollisionFlag`, `PlayerSprite`, `PlayerStruct`, `PlayerYPosHi`, `byte_333` |
| `unusedMaybe_A248` | `TENTATIVE` | `$A248` | `5613-5626` | `-` | - | - | - | `CollisionFlag` | `CollisionFlag` |
| `unusedMaybe_A25C` | `TENTATIVE` | `$A25C` | `5630-5643` | `-` | - | - | - | `CollisionFlag` | `CollisionFlag` |
| `sub_A270` | `AUTO_IDA` | `$A270` | `5647-5667` | `-` | `sub_96F5`, `JTPickup_Bomb` | `LoadPointerTo050`, `sub_9276` | - | `RoomDataRAM`, `byte_5A`, `byte_70`, `byte_92`, `byte_94` | `byte_786`, `byte_787`, `byte_789`, `byte_78B` |
| `sub_A29C` | `AUTO_IDA` | `$A29C` | `5671-5747` | `-` | `HandleSpawnsAndMore` | `MaybeAward1UP`, `ClearFourSpritesInY`, `sub_8FB1`, `sub_A32B` | - | `SpriteDMAArea`, `byte_786`, `byte_787`, `byte_789`, `byte_78B` | `CurrentSpriteIndex`, `SpriteDMAArea`, `TempSpriteAttributesish`, `TempSpriteTile`, `TempSpriteX` |
| `sub_A32B` | `AUTO_IDA` | `$A32B` | `5751-5764` | `-` | `sub_A29C` | `sub_A33E` | - | `RoomStatusFlags` | - |
| `sub_A33E` | `AUTO_IDA` | `$A33E` | `5768-5793` | `X,Y` | `sub_A32B` | - | - | `byte_786`, `byte_98`, `byte_99` | `byte_786` |
| `MaybeAward1UP` | `TENTATIVE` | `$A365` | `5797-5834` | `-` | `sub_A29C` | `QueueSound`, `Draw1UPSprite` | - | `Collected1UPThisRoundFlag`, `ExtraLivesFound`, `MaybeCollectedThing2`, `PlayerLives`, `PlayerXPosHi` | `Collected1UPThisRoundFlag`, `ExtraLivesFound`, `MaybeCollectedThing2`, `PlayerLives`, `Show1UPSpriteTimer` |
| `sub_A843` | `AUTO_IDA` | `$A843` | `6516-6523` | `-` | `MainSub_3` | `NegateYYXX` | - | `PlayerXVelHi`, `PlayerXVelLo` | `byte_98`, `byte_99` |
| `sub_A851` | `AUTO_IDA` | `$A851` | `6527-6534` | `-` | `MainSub_3` | `NegateYYXX` | - | `PlayerYVelHi`, `PlayerYVelLo` | `byte_9A`, `byte_9B` |
| `sub_A85F` | `AUTO_IDA` | `$A85F` | `6538-6549` | `-` | `MainSub_3` | `IfNotTitleScreenRoomDoThings`, `sub_9592`, `sub_A875` | - | `MaybeRoomIDCopyAgain` | - |
| `sub_A875` | `AUTO_IDA` | `$A875` | `6553-6655` | `X` | `sub_A85F` | `sub_96F5`, `HandleTileAttributes`, `LoadLayoutChunk` | `byte_6`, `byte_9`, `byte_A` | `MaybeTileCollected`, `PlayerXVelHi`, `RoomStatusFlags`, `byte_3C2`, `byte_3C6` | `PPUUpdateBuffer`, `unk_3F3` |
| `MaybeHandleDoorOpening` | `TENTATIVE` | `$A953` | `6696-6817` | `-` | `MainSub_3` | `LoadRoomData`, `DoorCond_rts`, `DoorCond_02_JumpedOn`, `DoorCond_XX_Mighty3` | - | `CurrentRoomIDBackup4`, `MaybeDoorTypeOrSphinx`, `MaybeHandleDoorFlag`, `off_58` | `MaybeDoorTypeOrSphinx`, `MaybeHandleDoorFlag` |
| `MaybeDontOpenDoors` | `TENTATIVE` | `$AAB4` | `7010-7026` | `-` | `DoorCond_02_JumpedOn`, `DoorCond_0A_Timer30`, `DoorCond_0E_Timer60`, `DoorCond_XX_Mighty3` | - | - | `DoorIsAnimatingMaybe`, `MaybeDoorTypeOrSphinx`, `UnknownDoorFlagCopy` | - |
| `HandleOpenDoorMaybe` | `TENTATIVE` | `$AAC7` | `7030-7088` | `-` | `DoorCond_02_JumpedOn`, `DoorCond_09_FBWarp2`, `DoorCond_0A_Timer30`, `DoorCond_0C_AllFireBombs` | `QueueSound`, `IfNotTitleScreenRoomDoThings`, `sub_9592` | `byte_10` | `CurrentRoomIDBackup4`, `MaybeDoorTypeOrSphinx`, `MaybeSphinxFlags`, `byte_5A` | `EntryDoorType`, `GameState`, `MaybeSphinxFlags`, `RoomDataRAM` |
| `sub_AFA9` | `AUTO_IDA` | `$AFA9` | `7928-7994` | `-` | `EndingState_2` | - | `byte_0` | `byte_373`, `byte_374`, `byte_375`, `byte_376` | `EnemyStructPointer`, `PPUUpdateBuffer`, `PPUUpdateFlag1`, `byte_373` |
| `sub_B024` | `AUTO_IDA` | `$B024` | `7998-8054` | `-` | `EndingState_2` | `sub_8AE8`, `sub_B080` | `byte_0`, `byte_1` | `EnemyStructPointer` | `EnemyStructPointer` |
| `sub_B080` | `AUTO_IDA` | `$B080` | `8058-8070` | `-` | `sub_B024` | `sub_B094` | `byte_0` | `EnemyStructPointer` | - |
| `sub_B094` | `AUTO_IDA` | `$B094` | `8076-8090` | `A` | `sub_B080` | - | `byte_1`, `byte_11` | - | `SpriteDMAArea` |
| `sub_B0AF` | `AUTO_IDA` | `$B0AF` | `8096-8108` | `A` | `TitleScreen_3`, `EndingState_4`, `EndingState_1` | `LoadObjectPointerA`, `DoSomethingWithObjYVel`, `ApplyPlayerXAndYVelocity`, `DrawPlayer` | - | - | `byte_3DD` |
| `sub_B645` | `AUTO_IDA` | `$B645` | `8326-8365` | `-` | `HandleSpawnsAndMore` | `LoadObjectPointerAPlus1`, `DrawEnemies`, `RunEnemyLogic`, `CheckObjectCollision` | `byte_12`, `byte_13` | `EnemyStructPointer`, `UnknownFlag_0FE`, `byte_BA` | `UnknownFlag_0FE`, `byte_BA` |
| `MaybeDrawEnemySprites` | `TENTATIVE` | `$B682` | `8369-8454` | `-` | `HandleSpawnsAndMore` | `LoadObjectPointerAPlus1`, `sub_B7B4`, `CopyEnemyToTempSprite`, `WriteSprite` | - | `CurrentSpriteIndex`, `EnemyStructPointer`, `MaybeTempSpriteIndex`, `PPUUpdateFlag1`, `PlayerStruct` | `CurrentSpriteIndex`, `MaybeTempSpriteIndex`, `byte_56` |
| `sub_B765` | `AUTO_IDA` | `$B765` | `8524-8534` | `-` | `ObjInit_2_Hanezo`, `ObjInit_5_Dokuron`, `ObjInit_6_Desufa`, `ObjInit_7_Horus` | - | - | `EnemyStructPointer` | `Mod_Number` |
| `MaybeRemoveIfOffscreen` | `TENTATIVE` | `$B776` | `8538-8544` | `-` | `sub_B645` | `MaybeConsiderOffscreen` | - | - | - |
| `MaybeConsiderOffscreen` | `TENTATIVE` | `$B781` | `8548-8566` | `Y` | `MaybeRemoveIfOffscreen` | `RemoveEnemy` | - | `EnemyStructPointer` | `MaybeTempCollectableFlag`, `MaybeTempCollectableFlag2` |
| `sub_B7B4` | `AUTO_IDA` | `$B7B4` | `8588-8594` | `-` | `MaybeDrawEnemySprites` | `sub_B7BF` | - | - | - |
| `sub_B7BF` | `AUTO_IDA` | `$B7BF` | `8598-8656` | `X,Y` | `sub_B7B4` | - | `byte_0`, `byte_1` | `EnemyStructPointer`, `byte_98`, `byte_99` | `EnemyStructPointer` |
| `sub_BA00` | `AUTO_IDA` | `$BA00` | `8995-9015` | `-` | `AddScore`, `MaybeSpawnThingFromObject`, `HandleEnemySpawnTimer`, `SpawnPowerCoin` | `LoadObjectPointerAPlus1` | - | `EnemyStructPointer` | - |
| `sub_BA48` | `AUTO_IDA` | `$BA48` | `9052-9056` | `-` | `ObjCode_0_Mummy` | `sub_BA7B` | - | - | - |
| `sub_BA4E` | `AUTO_IDA` | `$BA4E` | `9060-9085` | `-` | `sub_9813` | `ResetEnemyVelocity` | - | `EnemyStructPointer`, `RoomStatusFlags` | `EnemyStructPointer` |
| `sub_BA7B` | `AUTO_IDA` | `$BA7B` | `9092-9111` | `Y` | `sub_BA48` | `NegateYYXX` | `byte_11` | `EnemyStructPointer` | `EnemyStructPointer` |
| `sub_BA9E` | `AUTO_IDA` | `$BA9E` | `9128-9140` | `-` | `ObjCode_0_Mummy` | - | - | `EnemyStructPointer` | `EnemyStructPointer` |
| `sub_BB1C` | `AUTO_IDA` | `$BB1C` | `9224-9241` | `-` | `TitleScreen_2`, `ObjCode_9_E_ExtraBonusCoin`, `ObjInit_9_E_ExtraBonusCoin` | `ResetEnemyVelocity` | - | `EnemyStructPointer` | `EnemyStructPointer` |
| `sub_BB39` | `AUTO_IDA` | `$BB39` | `9245-9262` | `-` | `ObjCode_9_E_ExtraBonusCoin`, `ObjCode_0_Mummy` | - | - | `EnemyStructPointer`, `PlayerXPosHi` | `EnemyStructPointer` |
| `sub_BD64` | `AUTO_IDA` | `$BD64` | `9669-9685` | `-` | `JustCallBD64` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` | - | `TempPointer_0C0` | - |
| `sub_BD81` | `AUTO_IDA` | `$BD81` | `9696-9712` | `-` | `JustCallBD81` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` | - | `TempPointer_0C0` | - |
| `sub_BDA0` | `AUTO_IDA` | `$BDA0` | `9723-9747` | `X` | `ObjInitCode_3a` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` | - | `TempPointer_0C0` | - |
| `sub_BDC2` | `AUTO_IDA` | `$BDC2` | `9751-9754` | `-` | `ObjCode_6_Desufa` | `sub_BE2F` | - | - | - |
| `sub_BDC7` | `AUTO_IDA` | `$BDC7` | `9758-9782` | `X` | `ObjInitCode_3a` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` | - | `TempPointer_0C0` | - |
| `sub_BDE9` | `AUTO_IDA` | `$BDE9` | `9788-9814` | `Y` | `ObjCode_4a` | `CalculateModulus` | - | `EnemyStructPointer`, `PlayerStruct` | `Mod_Modulus`, `Mod_Number` |
| `sub_BE2F` | `AUTO_IDA` | `$BE2F` | `9845-9877` | `Y` | `ObjCode_5a`, `sub_BDC2` | - | - | `EnemyStructPointer`, `PlayerStruct`, `TempY` | `EnemyStructPointer`, `TempY` |
| `sub_BEE7` | `AUTO_IDA` | `$BEE7` | `9973-9995` | `-` | `ObjCode_7a` | - | - | `EnemyStructPointer`, `byte_C7` | `EnemyStructPointer` |
| `SubtractPlayerAndEnemyVarMaybe` | `TENTATIVE` | `$BF06` | `9999-10013` | `Y` | `ObjCode_7a` | - | - | `EnemyStructPointer`, `PlayerStruct` | - |
| `sub_BF17` | `AUTO_IDA` | `$BF17` | `10017-10049` | `-` | `ObjCode_7a` | - | - | `byte_C2`, `byte_C3` | `byte_CA`, `byte_CB`, `byte_CC`, `byte_CD` |
| `sub_BF4E` | `AUTO_IDA` | `$BF4E` | `10053-10085` | `-` | `ObjCode_7a` | - | - | `byte_C2`, `byte_C3` | `byte_CA`, `byte_CB`, `byte_CC`, `byte_CD` |
| `sub_BF85` | `AUTO_IDA` | `$BF85` | `10089-10126` | `-` | `ObjCode_7a` | - | - | `EnemyStructPointer`, `TempPointer_0C0`, `byte_C7`, `byte_CA` | `byte_C7` |
| `sub_BFBC` | `AUTO_IDA` | `$BFBC` | `10130-10145` | `-` | `ObjCode_7a` | `Negate_Mod_Number` | - | `byte_C7` | - |
| `sub_B285` | `AUTO_IDA` | `$B285` | `266-269` | `-` | `HandleSoundChannel`, `SoundCommand_Cx` | `CheckIfSoundChannelIsActive` | - | - | - |
| `MaybeUpdateVolDelay` | `TENTATIVE` | `$B45F` | `634-642` | `-` | `HandleSoundChannel` | - | - | `SoundEnginePointer` | `SoundEnginePointer` |
| `MaybeResetVolDelay` | `TENTATIVE` | `$B46B` | `646-708` | `-` | `HandleSoundChannel`, `SoundCommand_Ax_Bx` | `CheckIfSoundChannelIsActive` | `off_4` | `SoundChannelA_Mus3`, `SoundEnginePointer`, `SoundRegIndex` | `SoundEnginePointer`, `SoundReg_Vol` |

---

## 5. Catálogo Maestro de las 295 Rutinas Ejecutables (`prg.asm` + `music-engine.asm`)

| # | Nombre en `asm/` | Dir. ROM | Bytes | Archivo:Líneas | In Regs | Stack | Callers | Callees |
| :---: | :--- | :---: | :---: | :--- | :---: | :---: | :--- | :--- |
| 1 | `RESET` | `N/A` | - | `prg.asm:8-81` | `-` | `RTS` | - | `ClearNametable`, `ClearAllSprites`, `RestoreDefaultPalettes` (+2) |
| 2 | `NMI` | `$8073` | 84 | `prg.asm:91-123` | `A,X,Y` | `RTS` | - | `SetPPUCtrl`, `SetPPUMask`, `SetPPUScroll` (+4) |
| 3 | `HandlePPUUpdates` | `$80C7` | 172 | `prg.asm:127-237` | `-` | `RTS` | `NMI` | `CopyPaletteToPPU`, `sub_9729`, `LoadPointerTo050` (+1) |
| 4 | `CopyPaletteToPPU` | `$8173` | 18 | `prg.asm:241-249` | `-` | `RTS` | `HandlePPUUpdates` | `CopyPaletteToPPU2` |
| 5 | `CopyPaletteToPPU2` | `$8185` | 42 | `prg.asm:259-281` | `X,Y` | `RTS` | `CopyPaletteToPPU` | - |
| 6 | `MainLogicHandler` | `$81AF` | 10 | `prg.asm:290-296` | `-` | `RTS` | `NMI` | `TitleScreenHandler`, `GameHandler`, `JumpTable` |
| 7 | `GameHandler` | `$81B9` | 32 | `prg.asm:300-317` | `-` | `RTS` | `MainLogicHandler` | `GameState_0_Init`, `GameState_1_RoundIntro`, `GameState_2_LoadRoom` (+11) |
| 8 | `GameState_0_Init` | `$81D9` | 91 | `prg.asm:323-361` | `-` | `RTS` | `GameHandler` | `QueueSound`, `ClearNametable` |
| 9 | `InitPlayerStruct` | `$8234` | 12 | `prg.asm:365-375` | `-` | `RTS` | `TitleScreen_1`, `EndingState_0` | - |
| 10 | `GameState_3_DrawScreen` | `$8240` | 130 | `prg.asm:379-441` | `-` | `RTS` | `GameHandler` | `ClearAllSprites`, `MaybeLoadRoomFlags`, `LoadRoomStuff` (+3) |
| 11 | `GameState_4_EnterRoom` | `$82C2` | 325 | `prg.asm:445-623` | `-` | `RTS` | `GameHandler` | `SetPlayerVelocityFromX`, `DrawPlayer`, `LoadRoomPalette` (+5) |
| 12 | `SetPlayerVelocityFromX` | `$8407` | 25 | `prg.asm:627-636` | `X` | `RTS` | `GameState_4_EnterRoom` | - |
| 13 | `GameState_5_Main` | `$8420` | 42 | `prg.asm:647-668` | `-` | `RTS` | `GameHandler` | `MaybePause`, `MainSub_1`, `GameState_7_GoToTortureRoom` (+4) |
| 14 | `GameState_6_TimeOver` | `$844A` | 42 | `prg.asm:672-694` | `-` | `RTS` | `GameHandler` | `ClearNametable`, `WriteStringToPPU`, `QueueSound` (+2) |
| 15 | `GameState_7_GoToTortureRoom` | `$8474` | 130 | `prg.asm:698-772` | `-` | `PLA;PLA (Stack Pop)` | `GameHandler`, `GameState_5_Main` | `ClearAllSprites`, `ClearNametable`, `WriteStringToPPU` (+3) |
| 16 | `GameState_8_RoundClear` | `$84F6` | 427 | `prg.asm:776-1004` | `-` | `RTS` | `GameHandler` | `RoundClear_0`, `RoundClear_1`, `RoundClear_2` (+15) |
| 17 | `DrawTimeBonus` | `$86A1` | 22 | `prg.asm:1008-1018` | `-` | `RTS` | `GameState_8_RoundClear` | `WriteBCDDigitsSprites` |
| 18 | `DrawFireBombBonus` | `$86B7` | 22 | `prg.asm:1022-1032` | `-` | `RTS` | `GameState_8_RoundClear` | `WriteBCDDigitsSprites` |
| 19 | `GameState_A_GameOver` | `$86CD` | 102 | `prg.asm:1038-1091` | `-` | `RTS` | `GameHandler` | `ClearAllSprites`, `ClearNametable`, `WriteStringToPPU` (+5) |
| 20 | `AddAToGDV` | `$8733` | 14 | `prg.asm:1095-1102` | `A` | `RTS` | `GameState_A_GameOver`, `JT_98A4_A_SecretCoin`, `JTPickup_CrystalBall2` (+1) | `BCDCheckFirstDigit` |
| 21 | `BCDCheckFirstDigit` | `$8741` | 13 | `prg.asm:1106-1118` | `A` | `RTS` | `AddAToGDV` | - |
| 22 | `GameState_B_AnimateDoorOpen` | `$874E` | 9 | `prg.asm:1122-1127` | `-` | `RTS` | `GameHandler` | `AnimateDoorOpen` |
| 23 | `LoseALifeMaybeGameOver` | `$8757` | 32 | `prg.asm:1135-1151` | `-` | `RTS` | `GameState_6_TimeOver`, `MainSub_1` | `MaybeGoBackToOldRoom` |
| 24 | `MaybeGoBackToOldRoom` | `$8777` | 18 | `prg.asm:1155-1163` | `-` | `RTS` | `GameState_7_GoToTortureRoom`, `LoseALifeMaybeGameOver` | - |
| 25 | `MaybePause` | `$8789` | 32 | `prg.asm:1167-1187` | `-` | `RTS` | `GameState_5_Main` | `QueueSound` |
| 26 | `HandleStageTimer` | `$87A9` | 67 | `prg.asm:1191-1232` | `-` | `RTS` | `GameState_5_Main` | `BCDSub1FromA`, `DrawTimerOrTortureJumps` |
| 27 | `InitXSprites` | `$87EC` | 45 | `prg.asm:1238-1264` | `X` | `RTS` | `GameState_8_RoundClear` | - |
| 28 | `UpdateJoypads` | `$8819` | 26 | `prg.asm:1271-1283` | `-` | `RTS` | `NMI` | `ReadJoypad` |
| 29 | `ReadJoypad` | `$8833` | 18 | `prg.asm:1287-1300` | `X` | `RTS` | `UpdateJoypads` | - |
| 30 | `MainSub_1` | `$8845` | 599 | `prg.asm:1304-1636` | `-` | `PLA;PLA (Stack Pop)` | `GameState_5_Main` | `QueueSound`, `LoseALifeMaybeGameOver`, `RestoreDefaultPalettesLimited` (+8) |
| 31 | `DrawPlayer` | `$8A9C` | 35 | `prg.asm:1640-1654` | `-` | `RTS` | `GameState_4_EnterRoom`, `MainSub_1`, `TitleScreen_1` (+1) | `WriteSprite` |
| 32 | `DoSomethingWithObjYVel` | `$8ABF` | 41 | `prg.asm:1658-1685` | `X` | `RTS` | `MainSub_1`, `sub_B0AF`, `ObjCode_9_E_ExtraBonusCoin` (+1) | `sub_8AE8` |
| 33 | `sub_8AE8` | `$8AE8` | 26 | `prg.asm:1689-1709` | `A` | `RTS` | `DoSomethingWithObjYVel`, `sub_B024` | `NegateYYXX` |
| 34 | `ApplyPlayerXAndYVelocity` | `$8B02` | 3 | `prg.asm:1713-1715` | `-` | `RTS` | `GameState_4_EnterRoom`, `sub_B0AF` | `ApplyPlayerYVelocity` |
| 35 | `ApplyPlayerXVelocity` | `$8B05` | 20 | `prg.asm:1719-1728` | `-` | `RTS` | `MainSub_1` | - |
| 36 | `ApplyPlayerYVelocity` | `$8B19` | 20 | `prg.asm:1732-1741` | `-` | `RTS` | `MainSub_1`, `ApplyPlayerXAndYVelocity` | - |
| 37 | `UseMightyCoin` | `$8B2D` | 51 | `prg.asm:1745-1768` | `-` | `RTS` | `MainSub_1` | `UpdateMightyPalette`, `QueueSound`, `TurnEnemiesIntoCoins` |
| 38 | `UpdateMightyPalette` | `$8B60` | 10 | `prg.asm:1774-1779` | `X` | `RTS` | `MainSub_1`, `UseMightyCoin` | - |
| 39 | `DrawScoreAndCoinCount` | `$8B6A` | 18 | `prg.asm:1783-1795` | `-` | `RTS` | `GameState_5_Main` | `DrawScore`, `DrawMightyCoinCountOrMultiplier` |
| 40 | `DrawScore` | `$8B7C` | 45 | `prg.asm:1801-1823` | `-` | `RTS` | `GameState_8_RoundClear`, `DrawScoreAndCoinCount`, `TitleScreen_US_0` | `CopyNext5BytesToTempSprite`, `DrawScoreDigitMaybe` |
| 41 | `DrawScoreDigitMaybe` | `$8BA9` | 62 | `prg.asm:1829-1865` | `A,X` | `RTS` | `DrawScore` | - |
| 42 | `DrawTimerOrTortureJumps` | `$8BE7` | 55 | `prg.asm:1871-1897` | `-` | `RTS` | `GameState_7_GoToTortureRoom`, `HandleStageTimer` | `CopyNext5BytesToTempSprite`, `WriteSprite`, `WriteBCDDigitsSprites` |
| 43 | `DrawMightyCoinCountOrMultiplier` | `$8C1E` | 56 | `prg.asm:1903-1933` | `-` | `RTS` | `DrawScoreAndCoinCount` | `CopyNext5BytesToTempSprite`, `WriteSprite` |
| 44 | `RestoreDefaultPalettes` | `$8C56` | 22 | `prg.asm:1937-1951` | `-` | `RTS` | `RESET`, `GameState_A_GameOver`, `_EndGame` | - |
| 45 | `SetPPUCtrl` | `$8C6C` | 7 | `prg.asm:1955-1959` | `A` | `RTS` | `RESET`, `NMI`, `HandlePPUUpdates` (+3) | - |
| 46 | `SetPPUMask` | `$8C73` | 7 | `prg.asm:1963-1967` | `A` | `RTS` | `NMI` | - |
| 47 | `SetPPUScroll` | `$8C7A` | 7 | `prg.asm:1971-1975` | `A` | `RTS` | `NMI`, `ClearNametable`, `WriteStringToPPU` | - |
| 48 | `ClearNametable` | `$8C81` | 72 | `prg.asm:1979-2022` | `-` | `RTS` | `RESET`, `GameState_0_Init`, `GameState_6_TimeOver` (+6) | `SetPPUCtrl`, `SetPPUScroll` |
| 49 | `ClearAllSprites` | `$8CC9` | 20 | `prg.asm:2026-2046` | `-` | `RTS` | `RESET`, `GameState_3_DrawScreen`, `GameState_7_GoToTortureRoom` (+6) | - |
| 50 | `WriteStringToPPU` | `$8CDD` | 69 | `prg.asm:2050-2087` | `A` | `RTS` | `GameState_6_TimeOver`, `GameState_7_GoToTortureRoom`, `GameState_8_RoundClear` (+3) | `SetPPUScroll`, `SetPPUCtrl` |
| 51 | `NegateYYXX` | `$8D22` | 13 | `prg.asm:2094-2107` | `X,Y` | `RTS` | `sub_8AE8`, `sub_A843`, `sub_A851` (+2) | - |
| 52 | `BCDSub1FromA` | `$8D2F` | 16 | `prg.asm:2111-2125` | `A` | `RTS` | `GameState_8_RoundClear`, `HandleStageTimer` | - |
| 53 | `WriteBCDDigitsSprites` | `$8D3F` | 41 | `prg.asm:2129-2153` | `A` | `RTS` | `GameState_8_RoundClear`, `DrawTimeBonus`, `DrawFireBombBonus` (+4) | `WriteSprite` |
| 54 | `AnimateDoorOpen` | `$8D68` | 3 | `prg.asm:2157-2161` | `-` | `RTS` | `GameState_4_EnterRoom`, `GameState_B_AnimateDoorOpen` | - |
| 55 | `AnimateDoorClose` | `$8D6B` | 15 | `prg.asm:2165-2176` | `-` | `PLA;PLA (Stack Pop)` | `GameState_4_EnterRoom`, `GameState_9_ExitRoom` | `AnimateDoor` |
| 56 | `JumpTable` | `$8D7A` | 21 | `prg.asm:2180-2194` | `A` | `RTS` | `MainLogicHandler`, `GameHandler`, `GameState_8_RoundClear` (+12) | - |
| 57 | `QueueSound` | `$8D8F` | 10 | `prg.asm:2198-2203` | `A` | `RTS` | `GameState_0_Init`, `GameState_4_EnterRoom`, `GameState_6_TimeOver` (+28) | - |
| 58 | `CalculateModulus` | `$8D99` | 46 | `prg.asm:2207-2234` | `A,X` | `RTS` | `ObjCode_0_Mummy`, `sub_BDE9` | - |
| 59 | `LoadPointerTo050` | `$8DC7` | 43 | `prg.asm:2238-2263` | `A` | `RTS` | `HandlePPUUpdates`, `WriteSprite`, `LoadRoomPalette` (+11) | - |
| 60 | `WriteSprite` | `$8DF2` | 193 | `prg.asm:2267-2374` | `-` | `RTS` | `DrawPlayer`, `DrawTimerOrTortureJumps`, `DrawMightyCoinCountOrMultiplier` (+4) | `LoadPointerTo050` |
| 61 | `LoadRoomPalette` | `$8EB3` | 65 | `prg.asm:2389-2432` | `A` | `RTS` | `GameState_4_EnterRoom` | `LoadPointerTo050` |
| 62 | `AddScore` | `$8EF4` | 189 | `prg.asm:2436-2545` | `A` | `RTS` | `GameState_8_RoundClear`, `MainSub_1`, `JT_98A4_multi` (+13) | `sub_BA00`, `InitEnemy`, `ChooseEnemySpawnPoint` |
| 63 | `sub_8FB1` | `$8FB1` | 38 | `prg.asm:2551-2573` | `A` | `RTS` | `sub_A29C` | - |
| 64 | `MaybeLoadRoomFlags` | `$8FD7` | 74 | `prg.asm:2579-2626` | `A` | `RTS` | `GameState_3_DrawScreen`, `HandleSpawnsAndMore`, `MainSub_3` (+1) | `LoadPointerTo050` |
| 65 | `LoadRoomStuff` | `$9021` | 29 | `prg.asm:2630-2647` | `A` | `RTS` | `GameState_3_DrawScreen`, `DrawFullScreen`, `sub_9A03` (+3) | `LoadPointerTo050` |
| 66 | `HandleDrawHalfRow` | `$903E` | 76 | `prg.asm:2653-2699` | `Y` | `RTS` | `DrawFullScreen`, `sub_9A03`, `sub_9A7A` (+1) | `LoadPointerTo050` |
| 67 | `LoadLayoutChunk` | `$908A` | 28 | `prg.asm:2703-2720` | `A` | `RTS` | `DrawFullScreen`, `sub_9716`, `MainSub_3` (+1) | `LoadPointerTo050` |
| 68 | `DrawFullScreen` | `$90A6` | 464 | `prg.asm:2724-2984` | `-` | `RTS` | `GameState_3_DrawScreen`, `DrawTitleScreen` | `SetPPUCtrl`, `HandleDrawHalfRow`, `HandleTileAttributes` (+6) |
| 69 | `sub_9276` | `$9276` | 55 | `prg.asm:2988-3018` | `A` | `RTS` | `MaybeSpawnThingFromObject`, `sub_A270`, `InitPowerCoin` (+1) | - |
| 70 | `sub_92AD` | `$92AD` | 53 | `prg.asm:3022-3054` | `A,X` | `RTS` | `sub_96B7`, `MaybePickupItem`, `sub_A18E` (+1) | `sub_92E2` |
| 71 | `sub_92E2` | `$92E2` | 22 | `prg.asm:3058-3071` | `A` | `RTS` | `sub_92AD`, `MainSub_3` | - |
| 72 | `IfNotTitleScreenRoomDoThings` | `$92F8` | 72 | `prg.asm:3077-3126` | `-` | `PLA;PLA (Stack Pop)` | `sub_9696`, `BigBeefySubHandlesBGColliding`, `sub_A85F` (+2) | `LoadRoomData`, `ReadRoomData` |
| 73 | `GameState_2_LoadRoom` | `$9340` | 208 | `prg.asm:3130-3251` | `-` | `RTS` | `GameHandler` | `LoadPointerTo050`, `LoadRoomData`, `ReadRoomData` |
| 74 | `ReadRoomData` | `$9410` | 146 | `prg.asm:3257-3341` | `Y` | `RTS` | `IfNotTitleScreenRoomDoThings`, `GameState_2_LoadRoom` | `HandleRoomDataObject` |
| 75 | `HandleRoomDataObject` | `$94A2` | 44 | `prg.asm:3347-3377` | `A` | `RTS` | `ReadRoomData` | `JT_94AD_0`, `JT_94AD_multi`, `JT_94AD_5_6` (+5) |
| 76 | `JT_94AD_0` | `$94CE` | 27 | `prg.asm:3381-3398` | `-` | `RTS` | `HandleRoomDataObject` | - |
| 77 | `JT_94AD_multi` | `$94E9` | 51 | `prg.asm:3402-3434` | `-` | `RTS` | `HandleRoomDataObject` | - |
| 78 | `JT_94AD_5_6` | `$951C` | 9 | `prg.asm:3438-3443` | `-` | `RTS` | `HandleRoomDataObject` | - |
| 79 | `JT_94AD_7` | `$9525` | 38 | `prg.asm:3447-3471` | `-` | `RTS` | `HandleRoomDataObject` | - |
| 80 | `JT_94AD_8_Door` | `$954B` | 34 | `prg.asm:3475-3498` | `-` | `RTS` | `HandleRoomDataObject` | - |
| 81 | `JT_94AD_rts` | `$956D` | 1 | `prg.asm:3502-3504` | `-` | `RTS` | `HandleRoomDataObject` | - |
| 82 | `JT_94AD_E` | `$956E` | 4 | `prg.asm:3508-3511` | `-` | `RTS` | `HandleRoomDataObject` | - |
| 83 | `LoadRoomData` | `$9572` | 32 | `prg.asm:3515-3535` | `A` | `RTS` | `IfNotTitleScreenRoomDoThings`, `GameState_2_LoadRoom`, `MaybeHandleDoorOpening` | `LoadPointerTo050` |
| 84 | `sub_9592` | `$9592` | 260 | `prg.asm:3541-3680` | `-` | `PLA;PLA (Stack Pop)` | `sub_9696`, `BigBeefySubHandlesBGColliding`, `sub_A85F` (+2) | - |
| 85 | `sub_9696` | `$9696` | 33 | `prg.asm:3684-3699` | `-` | `RTS` | `DrawFullScreen` | `IfNotTitleScreenRoomDoThings`, `sub_9592`, `sub_96B7` |
| 86 | `sub_96B7` | `$96B7` | 27 | `prg.asm:3703-3717` | `X` | `RTS` | `sub_9696` | `sub_96F5`, `sub_92AD`, `HandleTileAttributes` (+2) |
| 87 | `CopyAttributeTableToPPU` | `$96D2` | 35 | `prg.asm:3724-3742` | `-` | `RTS` | `DrawFullScreen` | - |
| 88 | `sub_96F5` | `$96F5` | 33 | `prg.asm:3746-3769` | `X` | `PLA;PLA (Stack Pop)` | `sub_96B7`, `sub_A875` | `sub_A270` |
| 89 | `sub_9716` | `$9716` | 19 | `prg.asm:3773-3785` | `-` | `RTS` | `DrawFullScreen`, `sub_96B7`, `MaybePickupItem` (+2) | `LoadLayoutChunk` |
| 90 | `sub_9729` | `$9729` | 143 | `prg.asm:3789-3820` | `-` | `RTS` | `HandlePPUUpdates`, `DrawFullScreen`, `sub_96B7` | - |
| 91 | `HandleSpawnsAndMore` | `$97B8` | 91 | `prg.asm:3859-3909` | `-` | `RTS` | `GameState_5_Main` | `HandleEnemySpawnTimer`, `sub_A29C`, `MaybeDrawEnemySprites` (+7) |
| 92 | `sub_9813` | `$9813` | 66 | `prg.asm:3913-3955` | `A` | `RTS` | `HandleSpawnsAndMore` | `LoadObjectPointerAPlus1`, `sub_9965`, `sub_BA4E` |
| 93 | `CheckObjectCollision` | `$9855` | 116 | `prg.asm:3959-4028` | `-` | `RTS` | `sub_B645` | `CheckIfPlayerWithin8Pixels`, `RemoveEnemy`, `JT_98A4_multi` (+8) |
| 94 | `JT_98A4_multi` | `$98C9` | 51 | `prg.asm:4032-4060` | `-` | `RTS` | `CheckObjectCollision` | `QueueSound`, `AddScore` |
| 95 | `JT_98A4_9_ExtraCoin` | `$98FC` | 3 | `prg.asm:4064-4066` | `-` | `RTS` | `CheckObjectCollision` | - |
| 96 | `JT_98A4_10_Brother` | `$98FF` | 11 | `prg.asm:4070-4077` | `-` | `RTS` | `CheckObjectCollision` | `AddScore` |
| 97 | `JT_98A4_A_SecretCoin` | `$990A` | 29 | `prg.asm:4081-4096` | `-` | `RTS` | `CheckObjectCollision` | `RemoveEnemy`, `AddScore`, `QueueSound` (+1) |
| 98 | `JT_98A4_B_BonusStatue` | `$9927` | 11 | `prg.asm:4100-4106` | `-` | `RTS` | `CheckObjectCollision` | `AddScore`, `QueueSound` |
| 99 | `JT_98A4_C` | `$9932` | 11 | `prg.asm:4110-4116` | `-` | `RTS` | `CheckObjectCollision` | `QueueSound`, `AddScore` |
| 100 | `JT_98A4_E` | `$993D` | 22 | `prg.asm:4120-4132` | `-` | `RTS` | `CheckObjectCollision` | `AddScore`, `QueueSound` |
| 101 | `CheckIfPlayerWithin8Pixels` | `$9953` | 18 | `prg.asm:4136-4148` | `Y` | `RTS` | `CheckObjectCollision` | - |
| 102 | `sub_9965` | `$9965` | 133 | `prg.asm:4152-4226` | `A` | `RTS` | `HandleSpawnsAndMore`, `sub_9813` | `sub_99EA` |
| 103 | `sub_99EA` | `$99EA` | 25 | `prg.asm:4230-4247` | `X` | `RTS` | `HandleSpawnsAndMore`, `sub_9965` | `sub_9A03` |
| 104 | `sub_9A03` | `$9A03` | 119 | `prg.asm:4251-4314` | `-` | `RTS` | `sub_99EA` | `LoadRoomStuff`, `sub_9B39`, `HandleDrawHalfRow` (+1) |
| 105 | `sub_9A7A` | `$9A7A` | 191 | `prg.asm:4331-4444` | `-` | `RTS` | - | `LoadRoomStuff`, `sub_9B39`, `HandleDrawHalfRow` (+1) |
| 106 | `sub_9B39` | `$9B39` | 49 | `prg.asm:4448-4474` | `A` | `RTS` | `sub_9A03`, `sub_9A7A` | `BigBeefySubHandlesBGColliding` |
| 107 | `sub_9B6A` | `$9B6A` | 204 | `prg.asm:4480-4611` | `-` | `PLA;PLA (Stack Pop)` | `sub_9A03`, `sub_9A7A` | - |
| 108 | `BigBeefySubHandlesBGColliding` | `$9C36` | 413 | `prg.asm:4615-4856` | `-` | `RTS` | `sub_9B39` | `MaybeOpenHiddenBlock`, `IfNotTitleScreenRoomDoThings`, `sub_9592` (+8) |
| 109 | `MaybePickupItem` | `$9DD3` | 66 | `prg.asm:4860-4893` | `-` | `RTS` | `BigBeefySubHandlesBGColliding`, `JTPickup_Bomb`, `JTPickup_Family` | `sub_92AD`, `HandleTileAttributes`, `sub_9716` (+13) |
| 110 | `JT_9DF2_rts` | `$9E15` | 1 | `prg.asm:4897-4899` | `-` | `RTS` | `MaybePickupItem` | - |
| 111 | `JTPickup_Bag100` | `$9E16` | 4 | `prg.asm:4903-4906` | `-` | `RTS` | `MaybePickupItem` | - |
| 112 | `JTPickup_Bag300` | `$9E1A` | 3 | `prg.asm:4910-4912` | `-` | `RTS` | `MaybePickupItem` | - |
| 113 | `JTPickup_Bag1000` | `$9E1D` | 11 | `prg.asm:4919-4927` | `-` | `RTS` | `MaybePickupItem` | `AddScore`, `QueueSound` |
| 114 | `JTPickup_GoldCoin` | `$9E28` | 14 | `prg.asm:4931-4938` | `-` | `RTS` | `MaybePickupItem` | `AddScore`, `QueueSound` |
| 115 | `JTPickup_MightyDrink` | `$9E36` | 23 | `prg.asm:4942-4953` | `-` | `RTS` | `MaybePickupItem` | `AddScore`, `QueueSound` |
| 116 | `GoToTheTortureRoom` | `$9E4D` | 14 | `prg.asm:4957-4966` | `-` | `RTS` | `JTPickup_MightyCoin`, `GameState_9_ExitRoom` | - |
| 117 | `JTPickup_Bomb` | `$9E5B` | 339 | `prg.asm:4970-5164` | `-` | `RTS` | `BigBeefySubHandlesBGColliding` | `SpawnPowerCoin`, `QueueSound`, `AddScore` (+4) |
| 118 | `JTPickup_Sphinx` | `$9FAE` | 11 | `prg.asm:5168-5173` | `-` | `RTS` | `MaybePickupItem` | `AddScore`, `HandleSphinxEvent` |
| 119 | `JTPickup_TecmoPlate` | `$9FB9` | 11 | `prg.asm:5177-5183` | `-` | `RTS` | `MaybePickupItem` | `AddScore` |
| 120 | `JTPickup_MightyCoin` | `$9FC4` | 24 | `prg.asm:5187-5201` | `-` | `RTS` | `MaybePickupItem` | `AddScore`, `QueueSound`, `GoToTheTortureRoom` |
| 121 | `JTPickup_CrystalBall1` | `$9FDC` | 3 | `prg.asm:5205-5207` | `-` | `RTS` | `MaybePickupItem` | - |
| 122 | `JTPickup_CrystalBall2` | `$9FDF` | 19 | `prg.asm:5214-5223` | `-` | `RTS` | `MaybePickupItem` | `QueueSound`, `AddAToGDV` |
| 123 | `JTPickup_Family` | `$9FF2` | 18 | `prg.asm:5227-5237` | `-` | `RTS` | `BigBeefySubHandlesBGColliding`, `JTPickup_Beelzebub` | `MaybePickupItem` |
| 124 | `JTPickup_Beelzebub` | `$A004` | 8 | `prg.asm:5241-5249` | `-` | `RTS` | `BigBeefySubHandlesBGColliding` | `QueueSound`, `AddScore`, `JTPickup_Family` |
| 125 | `JT_9DF2_F_rts` | `$A00C` | 1 | `prg.asm:5253-5255` | `-` | `RTS` | `MaybePickupItem` | - |
| 126 | `JTPickup_Chest` | `$A00D` | 79 | `prg.asm:5259-5296` | `-` | `RTS` | `BigBeefySubHandlesBGColliding`, `JTPickup_Chest2` | `MaybeOpenHiddenBlock` |
| 127 | `JTPickup_Chest2` | `$A05C` | 9 | `prg.asm:5300-5308` | `-` | `RTS` | `BigBeefySubHandlesBGColliding` | `JTPickup_Chest` |
| 128 | `JTPickup_Door` | `$A065` | 69 | `prg.asm:5312-5346` | `-` | `RTS` | `BigBeefySubHandlesBGColliding` | `QueueSound` |
| 129 | `OpenBombRoomDoor` | `$A0AA` | 26 | `prg.asm:5352-5365` | `-` | `RTS` | `JTPickup_Bomb` | - |
| 130 | `MaybeDrawFireBombSprite` | `$A0C4` | 31 | `prg.asm:5369-5388` | `A` | `RTS` | `JTPickup_Bomb` | - |
| 131 | `MaybeOpenHiddenBlock` | `$A0E3` | 111 | `prg.asm:5392-5458` | `A` | `RTS` | `BigBeefySubHandlesBGColliding`, `JTPickup_Chest` | `sub_A18E`, `MaybeSpawnThingFromObject`, `sub_A1A6` (+2) |
| 132 | `MaybeSpawnThingFromObject` | `$A152` | 60 | `prg.asm:5462-5495` | `-` | `RTS` | `MaybeOpenHiddenBlock` | `QueueSound`, `sub_BA00`, `sub_9276` (+1) |
| 133 | `sub_A18E` | `$A18E` | 24 | `prg.asm:5499-5511` | `X` | `RTS` | `MaybeOpenHiddenBlock` | `sub_92AD` |
| 134 | `sub_A1A6` | `$A1A6` | 19 | `prg.asm:5515-5525` | `A` | `RTS` | `MaybeOpenHiddenBlock` | `HandleTileAttributes`, `sub_9716` |
| 135 | `sub_A1B9` | `$A1B9` | 50 | `prg.asm:5529-5555` | `-` | `RTS` | `HandleSpawnsAndMore` | - |
| 136 | `sub_A1EB` | `$A1EB` | 93 | `prg.asm:5559-5609` | `-` | `RTS` | `HandleSpawnsAndMore` | - |
| 137 | `unusedMaybe_A248` | `$A248` | 20 | `prg.asm:5613-5626` | `-` | `RTS` | - | - |
| 138 | `unusedMaybe_A25C` | `$A25C` | 20 | `prg.asm:5630-5643` | `-` | `RTS` | - | - |
| 139 | `sub_A270` | `$A270` | 44 | `prg.asm:5647-5667` | `-` | `RTS` | `sub_96F5`, `JTPickup_Bomb` | `LoadPointerTo050`, `sub_9276` |
| 140 | `sub_A29C` | `$A29C` | 143 | `prg.asm:5671-5747` | `-` | `RTS` | `HandleSpawnsAndMore` | `MaybeAward1UP`, `ClearFourSpritesInY`, `sub_8FB1` (+2) |
| 141 | `sub_A32B` | `$A32B` | 19 | `prg.asm:5751-5764` | `-` | `RTS` | `sub_A29C` | `sub_A33E` |
| 142 | `sub_A33E` | `$A33E` | 39 | `prg.asm:5768-5793` | `X,Y` | `RTS` | `sub_A32B` | - |
| 143 | `MaybeAward1UP` | `$A365` | 70 | `prg.asm:5797-5834` | `-` | `RTS` | `sub_A29C` | `QueueSound`, `Draw1UPSprite` |
| 144 | `Draw1UPSprite` | `$A3AB` | 99 | `prg.asm:5838-5860` | `A` | `RTS` | `MaybeAward1UP`, `ObjCode_10_Brother` | - |
| 145 | `MainSub_3` | `$A40E` | 1077 | `prg.asm:5906-6512` | `-` | `RTS` | `GameState_4_EnterRoom`, `GameState_5_Main` | `MaybeHandleDoorOpening`, `MaybeLoadRoomFlags`, `sub_A843` (+8) |
| 146 | `sub_A843` | `$A843` | 14 | `prg.asm:6516-6523` | `-` | `RTS` | `MainSub_3` | `NegateYYXX` |
| 147 | `sub_A851` | `$A851` | 14 | `prg.asm:6527-6534` | `-` | `RTS` | `MainSub_3` | `NegateYYXX` |
| 148 | `sub_A85F` | `$A85F` | 22 | `prg.asm:6538-6549` | `-` | `RTS` | `MainSub_3` | `IfNotTitleScreenRoomDoThings`, `sub_9592`, `sub_A875` |
| 149 | `sub_A875` | `$A875` | 165 | `prg.asm:6553-6655` | `X` | `RTS` | `sub_A85F` | `sub_96F5`, `HandleTileAttributes`, `LoadLayoutChunk` |
| 150 | `SpawnBrotherRoomObjects` | `$A91A` | 37 | `prg.asm:6659-6676` | `-` | `RTS` | `GameState_3_DrawScreen` | `LoadObjectPointerAPlus1`, `InitEnemy`, `SetBrotherRoomObjectPosition` |
| 151 | `SetBrotherRoomObjectPosition` | `$A93F` | 20 | `prg.asm:6680-6688` | `X` | `RTS` | `SpawnBrotherRoomObjects` | - |
| 152 | `MaybeHandleDoorOpening` | `$A953` | 129 | `prg.asm:6696-6817` | `-` | `RTS` | `MainSub_3` | `LoadRoomData`, `DoorCond_rts`, `DoorCond_02_JumpedOn` (+13) |
| 153 | `DoorCond_unused` | `$A9D4` | 10 | `prg.asm:6818-6821` | `-` | `RTS` | - | - |
| 154 | `DoorCond_02_JumpedOn` | `$A9DE` | 11 | `prg.asm:6832-6837` | `-` | `RTS` | `MaybeHandleDoorOpening` | `MaybeDontOpenDoors`, `HandleOpenDoorMaybe` |
| 155 | `DoorCond_09_FBWarp2` | `$A9E9` | 16 | `prg.asm:6845-6853` | `-` | `RTS` | `MaybeHandleDoorOpening` | `HandleOpenDoorMaybe` |
| 156 | `DoorCond_0A_Timer30` | `$A9F9` | 13 | `prg.asm:6858-6864` | `-` | `RTS` | `MaybeHandleDoorOpening` | `MaybeDontOpenDoors`, `HandleOpenDoorMaybe` |
| 157 | `DoorCond_0C_AllFireBombs` | `$AA06` | 13 | `prg.asm:6871-6877` | `-` | `RTS` | `MaybeHandleDoorOpening` | `HandleOpenDoorMaybe` |
| 158 | `DoorCond_rts` | `$AA13` | 1 | `prg.asm:6883-6885` | `-` | `RTS` | `MaybeHandleDoorOpening` | - |
| 159 | `DoorCond_0E_Timer60` | `$AA14` | 13 | `prg.asm:6890-6896` | `-` | `RTS` | `MaybeHandleDoorOpening` | `MaybeDontOpenDoors`, `HandleOpenDoorMaybe` |
| 160 | `DoorCond_XX_Mighty3` | `$AA21` | 13 | `prg.asm:6903-6909` | `-` | `RTS` | `MaybeHandleDoorOpening` | `MaybeDontOpenDoors`, `HandleOpenDoorMaybe` |
| 161 | `DoorCond_19_TensScore30` | `$AA2E` | 18 | `prg.asm:6914-6922` | `-` | `RTS` | `MaybeHandleDoorOpening` | `HandleOpenDoorMaybe` |
| 162 | `DoorCond_1D_FBWarp1` | `$AA40` | 37 | `prg.asm:6929-6945` | `-` | `RTS` | `MaybeHandleDoorOpening` | `HandleOpenDoorMaybe` |
| 163 | `DoorCond_1E_R5R11Warp` | `$AA65` | 50 | `prg.asm:6951-6974` | `-` | `RTS` | `MaybeHandleDoorOpening` | - |
| 164 | `DoorCond_1F_R11WarpBack` | `$AA97` | 10 | `prg.asm:6980-6985` | `-` | `RTS` | `MaybeHandleDoorOpening` | - |
| 165 | `DoorCond_1A_TensScore70` | `$AAA1` | 19 | `prg.asm:6990-7004` | `-` | `RTS` | `MaybeHandleDoorOpening` | `HandleOpenDoorMaybe` |
| 166 | `MaybeDontOpenDoors` | `$AAB4` | 19 | `prg.asm:7010-7026` | `-` | `PLA;PLA (Stack Pop)` | `DoorCond_02_JumpedOn`, `DoorCond_0A_Timer30`, `DoorCond_0E_Timer60` (+1) | - |
| 167 | `HandleOpenDoorMaybe` | `$AAC7` | 82 | `prg.asm:7030-7088` | `-` | `RTS` | `DoorCond_02_JumpedOn`, `DoorCond_09_FBWarp2`, `DoorCond_0A_Timer30` (+6) | `QueueSound`, `IfNotTitleScreenRoomDoThings`, `sub_9592` |
| 168 | `HandleTileAttributes` | `$AB19` | 181 | `prg.asm:7092-7215` | `A` | `RTS` | `DrawFullScreen`, `sub_96B7`, `MaybePickupItem` (+4) | `LoadPointerTo050` |
| 169 | `AnimateDoor` | `$ABCE` | 126 | `prg.asm:7219-7290` | `X` | `RTS` | `AnimateDoorClose` | `QueueSound`, `sub_92AD`, `sub_9716` (+1) |
| 170 | `GameState_9_ExitRoom` | `$AC4C` | 147 | `prg.asm:7294-7372` | `-` | `RTS` | `GameHandler` | `ClearFourSpritesInY`, `AnimateDoorClose`, `MaybeLoadRoomFlags` (+4) |
| 171 | `EndingDoorChecks` | `$ACDF` | 17 | `prg.asm:7376-7386` | `X` | `RTS` | `GameState_9_ExitRoom` | `_EndCheckCrystalBall1`, `_EndCheckCrystalBall2`, `_EndCheck5SecretCoins` (+2) |
| 172 | `_EndCheckCrystalBall1` | `$ACF0` | 6 | `prg.asm:7390-7394` | `-` | `RTS` | `EndingDoorChecks` | - |
| 173 | `_EndCheckCrystalBall2` | `$ACF6` | 8 | `prg.asm:7398-7403` | `-` | `RTS` | `EndingDoorChecks` | - |
| 174 | `_EndCheck5SecretCoins` | `$ACFE` | 8 | `prg.asm:7407-7412` | `-` | `RTS` | `EndingDoorChecks` | - |
| 175 | `_EndGame` | `$AD06` | 55 | `prg.asm:7416-7425` | `-` | `RTS` | `EndingDoorChecks` | `RestoreDefaultPalettes`, `AddAToGDV` |
| 176 | `TitleScreenHandler` | `$AD3D` | 55 | `prg.asm:7473-7492` | `-` | `RTS` | `MainLogicHandler` | `TitleScreen_US_0`, `TitleScreen_US_1`, `TitleScreen_0` (+5) |
| 177 | `TitleScreen_US_0` | `N/A` | - | `prg.asm:7496-7569` | `-` | `RTS` | `TitleScreenHandler` | `ClearAllSprites`, `ClearNametable`, `WriteStringToPPU` (+4) |
| 178 | `DrawTitleScreen` | `$AD74` | 43 | `prg.asm:7573-7591` | `-` | `RTS` | `TitleScreen_US_0`, `EndingState_0` | `ClearAllSprites`, `LoadPointerTo050`, `LoadRoomStuff` (+1) |
| 179 | `TitleScreen_1` | `$AD9F` | 20 | `prg.asm:7595-7604` | `-` | `RTS` | `TitleScreenHandler` | `InitPlayerStruct`, `DrawPlayer` |
| 180 | `TitleScreen_2` | `$ADB3` | 37 | `prg.asm:7608-7640` | `-` | `RTS` | `TitleScreenHandler` | `LoadObjectPointerA`, `sub_BB1C` |
| 181 | `TitleScreen_3` | `$ADD8` | 30 | `prg.asm:7644-7659` | `-` | `RTS` | `TitleScreenHandler` | `sub_B0AF`, `ClearNametable`, `ClearAllSprites` |
| 182 | `GameState_1_RoundIntro` | `$ADF6` | 76 | `prg.asm:7663-7700` | `-` | `RTS` | `GameHandler` | `ClearAllSprites`, `ClearNametable`, `DrawRoundIntroSprites` (+3) |
| 183 | `GameState_C_Ending` | `$AE42` | 16 | `prg.asm:7706-7715` | `-` | `RTS` | `GameHandler` | `EndingState_0`, `EndingState_1`, `EndingState_2` (+3) |
| 184 | `EndingState_0` | `$AE52` | 58 | `prg.asm:7719-7751` | `-` | `RTS` | `GameState_C_Ending` | `DrawTitleScreen`, `InitPlayerStruct` |
| 185 | `EndingState_3` | `$AE8C` | 53 | `prg.asm:7755-7781` | `-` | `RTS` | `GameState_C_Ending` | `WriteStringToPPU`, `QueueSound` |
| 186 | `EndingState_4` | `$AEC1` | 40 | `prg.asm:7791-7814` | `-` | `RTS` | `GameState_C_Ending` | `sub_B0AF` |
| 187 | `EndingState_1` | `$AEE9` | 57 | `prg.asm:7818-7844` | `-` | `RTS` | `GameState_C_Ending` | `sub_B0AF` |
| 188 | `EndingState_2` | `$AF22` | 135 | `prg.asm:7848-7924` | `-` | `RTS` | `GameState_C_Ending` | `LoadObjectPointerA`, `sub_B024`, `ClearFourSpritesInY` (+2) |
| 189 | `sub_AFA9` | `$AFA9` | 123 | `prg.asm:7928-7994` | `-` | `RTS` | `EndingState_2` | - |
| 190 | `sub_B024` | `$B024` | 92 | `prg.asm:7998-8054` | `-` | `RTS` | `EndingState_2` | `sub_8AE8`, `sub_B080` |
| 191 | `sub_B080` | `$B080` | 20 | `prg.asm:8058-8070` | `-` | `RTS` | `sub_B024` | `sub_B094` |
| 192 | `sub_B094` | `$B094` | 27 | `prg.asm:8076-8090` | `A` | `RTS` | `sub_B080` | - |
| 193 | `sub_B0AF` | `$B0AF` | 23 | `prg.asm:8096-8108` | `A` | `RTS` | `TitleScreen_3`, `EndingState_4`, `EndingState_1` | `LoadObjectPointerA`, `DoSomethingWithObjYVel`, `ApplyPlayerXAndYVelocity` (+1) |
| 194 | `CopyNext5BytesToTempSprite` | `$B0C6` | 54 | `prg.asm:8119-8151` | `-` | `RTS` | `GameState_8_RoundClear`, `GameState_A_GameOver`, `DrawScore` (+3) | - |
| 195 | `HandleEnemySpawnTimer` | `$B570` | 0 | `prg.asm:8171-8172` | `-` | `RTS` | - | - |
| 196 | `HandleEnemySpawnTimer` | `$B570` | 72 | `prg.asm:8171-8218` | `-` | `RTS` | `HandleSpawnsAndMore` | `LoadDifficultySettings`, `sub_BA00`, `InitEnemy` (+2) |
| 197 | `InitEnemy` | `$B5B8` | 22 | `prg.asm:8224-8240` | `A` | `RTS` | `AddScore`, `MaybeSpawnThingFromObject`, `SpawnBrotherRoomObjects` (+4) | `RunEnemyInit` |
| 198 | `RunEnemyInit` | `$B5CE` | 73 | `prg.asm:8246-8286` | `A` | `RTS` | `InitEnemy`, `ObjCode_0_Mummy` | `ObjInit_0_Mummy`, `Obj_null`, `ObjInit_2_Hanezo` (+11) |
| 199 | `LoadDifficultySettings` | `$B617` | 31 | `prg.asm:8290-8307` | `-` | `RTS` | `HandleEnemySpawnTimer` | - |
| 200 | `OrEnemyStatusWith08` | `$B636` | 15 | `prg.asm:8313-8322` | `-` | `RTS` | `HandleEnemySpawnTimer`, `ObjCode_0_Mummy`, `ObjInit_D_F_MummyAlt` | - |
| 201 | `sub_B645` | `$B645` | 61 | `prg.asm:8326-8365` | `-` | `RTS` | `HandleSpawnsAndMore` | `LoadObjectPointerAPlus1`, `DrawEnemies`, `RunEnemyLogic` (+2) |
| 202 | `MaybeDrawEnemySprites` | `$B682` | 140 | `prg.asm:8369-8454` | `-` | `RTS` | `HandleSpawnsAndMore` | `LoadObjectPointerAPlus1`, `sub_B7B4`, `CopyEnemyToTempSprite` (+2) |
| 203 | `RunEnemyLogic` | `$B70E` | 49 | `prg.asm:8458-8484` | `-` | `RTS` | `sub_B645` | `ObjCode_0_Mummy`, `Obj_null`, `ObjCode_2_Hanezo` (+9) |
| 204 | `LoadObjectPointerAPlus1` | `$B73F` | 16 | `prg.asm:8487-8499` | `A` | `RTS` | `sub_9813`, `SpawnBrotherRoomObjects`, `sub_B645` (+2) | - |
| 205 | `RemoveEnemy` | `$B74F` | 22 | `prg.asm:8503-8518` | `-` | `RTS` | `CheckObjectCollision`, `JT_98A4_A_SecretCoin`, `MaybeConsiderOffscreen` (+2) | - |
| 206 | `sub_B765` | `$B765` | 17 | `prg.asm:8524-8534` | `-` | `RTS` | `ObjInit_2_Hanezo`, `ObjInit_5_Dokuron`, `ObjInit_6_Desufa` (+7) | - |
| 207 | `MaybeRemoveIfOffscreen` | `$B776` | 11 | `prg.asm:8538-8544` | `-` | `RTS` | `sub_B645` | `MaybeConsiderOffscreen` |
| 208 | `MaybeConsiderOffscreen` | `$B781` | 26 | `prg.asm:8548-8566` | `Y` | `PLA;PLA (Stack Pop)` | `MaybeRemoveIfOffscreen` | `RemoveEnemy` |
| 209 | `CopyEnemyToTempSprite` | `$B79B` | 25 | `prg.asm:8570-8584` | `-` | `RTS` | `MaybeDrawEnemySprites` | - |
| 210 | `sub_B7B4` | `$B7B4` | 11 | `prg.asm:8588-8594` | `-` | `RTS` | `MaybeDrawEnemySprites` | `sub_B7BF` |
| 211 | `sub_B7BF` | `$B7BF` | 82 | `prg.asm:8598-8656` | `X,Y` | `RTS` | `sub_B7B4` | - |
| 212 | `InitPowerCoin` | `$B811` | 42 | `prg.asm:8660-8685` | `-` | `RTS` | `SpawnPowerCoin` | `sub_9276` |
| 213 | `ChooseEnemySpawnPoint` | `$B83B` | 104 | `prg.asm:8689-8754` | `-` | `RTS` | `AddScore`, `HandleEnemySpawnTimer` | `LoadPointerTo050`, `RemoveEnemy`, `SpawnLocFunc1` (+1) |
| 214 | `SpawnLocFunc1` | `$B8A3` | 48 | `prg.asm:8758-8788` | `A` | `RTS` | `ChooseEnemySpawnPoint` | `sub_9276` |
| 215 | `SpawnLocFunc2` | `$B8D3` | 50 | `prg.asm:8794-8824` | `X` | `RTS` | `ChooseEnemySpawnPoint` | - |
| 216 | `DrawEnemies` | `$B905` | 225 | `prg.asm:8828-8974` | `-` | `RTS` | `sub_B645` | - |
| 217 | `SpawnPowerCoin` | `$B9E6` | 26 | `prg.asm:8978-8991` | `-` | `RTS` | `JTPickup_Bomb`, `MaybeOpenHiddenBlock` | `sub_BA00`, `InitEnemy`, `InitPowerCoin` |
| 218 | `sub_BA00` | `$BA00` | 24 | `prg.asm:8995-9015` | `-` | `PLA;PLA (Stack Pop)` | `AddScore`, `MaybeSpawnThingFromObject`, `HandleEnemySpawnTimer` (+1) | `LoadObjectPointerAPlus1` |
| 219 | `JT_98A4_8_PowerCoin` | `$BA18` | 48 | `prg.asm:9019-9044` | `-` | `RTS` | `CheckObjectCollision` | `RemoveEnemy`, `QueueSound` |
| 220 | `sub_BA48` | `$BA48` | 6 | `prg.asm:9052-9056` | `-` | `RTS` | `ObjCode_0_Mummy` | `sub_BA7B` |
| 221 | `sub_BA4E` | `$BA4E` | 45 | `prg.asm:9060-9085` | `-` | `RTS` | `sub_9813` | `ResetEnemyVelocity` |
| 222 | `sub_BA7B` | `$BA7B` | 26 | `prg.asm:9092-9111` | `Y` | `RTS` | `sub_BA48` | `NegateYYXX` |
| 223 | `ResetEnemyVelocity` | `$BA95` | 9 | `prg.asm:9117-9124` | `Y` | `RTS` | `sub_BA4E`, `ObjCode_9_E_ExtraBonusCoin`, `sub_BB1C` (+2) | - |
| 224 | `sub_BA9E` | `$BA9E` | 16 | `prg.asm:9128-9140` | `-` | `RTS` | `ObjCode_0_Mummy` | - |
| 225 | `ObjCode_9_E_ExtraBonusCoin` | `$BAAE` | 87 | `prg.asm:9144-9198` | `-` | `RTS` | `RunEnemyLogic` | `DoSomethingWithObjYVel`, `sub_BB1C`, `sub_BB39` (+2) |
| 226 | `Obj_null` | `$BB05` | 1 | `prg.asm:9202-9204` | `-` | `RTS` | `RunEnemyInit`, `RunEnemyLogic` | - |
| 227 | `ObjInit_0_Mummy` | `$BB06` | 22 | `prg.asm:9208-9220` | `-` | `RTS` | `RunEnemyInit`, `ObjCode_0_Mummy` | - |
| 228 | `sub_BB1C` | `$BB1C` | 29 | `prg.asm:9224-9241` | `-` | `RTS` | `TitleScreen_2`, `ObjCode_9_E_ExtraBonusCoin`, `ObjInit_9_E_ExtraBonusCoin` | `ResetEnemyVelocity` |
| 229 | `sub_BB39` | `$BB39` | 25 | `prg.asm:9245-9262` | `-` | `RTS` | `ObjCode_9_E_ExtraBonusCoin`, `ObjCode_0_Mummy` | - |
| 230 | `ObjCode_0_Mummy` | `$BB52` | 192 | `prg.asm:9266-9378` | `-` | `RTS` | `RunEnemyLogic` | `sub_BA9E`, `ResetEnemyVelocity`, `sub_BB39` (+6) |
| 231 | `ObjInit_2_Hanezo` | `$BC12` | 13 | `prg.asm:9382-9388` | `-` | `RTS` | `RunEnemyInit` | `sub_B765`, `Store_Mod_Number_ToEnemy` |
| 232 | `ObjCode_2_Hanezo` | `$BC1F` | 7 | `prg.asm:9392-9396` | `-` | `RTS` | `RunEnemyLogic` | `JustCallBD64`, `JustCallBD81` |
| 233 | `ObjCode_3_GejiShogun` | `$BC26` | 4 | `prg.asm:9400-9403` | `-` | `RTS` | `RunEnemyInit`, `RunEnemyLogic` | `ObjInitCode_3a` |
| 234 | `ObjInit_4_Gameido` | `$BC2A` | 11 | `prg.asm:9407-9413` | `-` | `RTS` | `RunEnemyInit` | - |
| 235 | `ObjCode_4_Gameido` | `$BC35` | 16 | `prg.asm:9417-9426` | `-` | `RTS` | `RunEnemyLogic` | `ObjCode_4a` |
| 236 | `ObjInit_5_Dokuron` | `$BC45` | 8 | `prg.asm:9430-9434` | `-` | `RTS` | `RunEnemyInit` | `sub_B765`, `Store_Mod_Number_ToEnemy` |
| 237 | `ObjCode_5_Dokuron` | `$BC4D` | 7 | `prg.asm:9438-9442` | `-` | `RTS` | `RunEnemyLogic` | `ObjCode_5a`, `JustCallBD81` |
| 238 | `ObjInit_6_Desufa` | `$BC54` | 8 | `prg.asm:9446-9450` | `-` | `RTS` | `RunEnemyInit` | `sub_B765`, `Store_Mod_Number_ToEnemy` |
| 239 | `ObjCode_6_Desufa` | `$BC5C` | 7 | `prg.asm:9454-9458` | `-` | `RTS` | `RunEnemyLogic` | `sub_BDC2`, `JustCallBD64` |
| 240 | `ObjInit_7_Horus` | `$BC63` | 9 | `prg.asm:9462-9467` | `-` | `RTS` | `RunEnemyInit` | `sub_B765` |
| 241 | `ObjCode_7_Horus` | `$BC6C` | 4 | `prg.asm:9471-9474` | `-` | `RTS` | `RunEnemyLogic` | `ObjCode_7a` |
| 242 | `ObjInit_8_PowerCoin` | `$BC70` | 40 | `prg.asm:9478-9501` | `-` | `RTS` | `RunEnemyInit` | `sub_B765`, `Store_Mod_Number_ToEnemy` |
| 243 | `ObjCode_8_PowerCoin` | `$BC98` | 34 | `prg.asm:9505-9528` | `-` | `RTS` | `RunEnemyLogic` | `JustCallBD64`, `JustCallBD81` |
| 244 | `ObjInit_9_E_ExtraBonusCoin` | `$BCBA` | 25 | `prg.asm:9532-9548` | `-` | `RTS` | `RunEnemyInit` | `QueueSound`, `sub_BB1C` |
| 245 | `ObjInit_C_Balloon` | `$BCD3` | 13 | `prg.asm:9552-9560` | `-` | `RTS` | `RunEnemyInit` | - |
| 246 | `ObjInit_D_F_MummyAlt` | `$BCE0` | 19 | `prg.asm:9564-9573` | `-` | `RTS` | `RunEnemyInit` | `InitEnemy`, `OrEnemyStatusWith08` |
| 247 | `ObjInit_10_Brother` | `$BCF3` | 11 | `prg.asm:9577-9585` | `-` | `RTS` | `RunEnemyInit` | `InitEnemy` |
| 248 | `ObjCode_10_Brother` | `$BCFE` | 25 | `prg.asm:9589-9604` | `-` | `RTS` | `RunEnemyLogic` | `Draw1UPSprite` |
| 249 | `ObjCode_4a` | `$BD17` | 73 | `prg.asm:9608-9658` | `X` | `RTS` | `ObjCode_4_Gameido` | `sub_BDE9`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` |
| 250 | `JustCallBD64` | `$BD60` | 4 | `prg.asm:9662-9665` | `-` | `RTS` | `ObjCode_9_E_ExtraBonusCoin`, `ObjCode_2_Hanezo`, `ObjCode_6_Desufa` (+1) | `sub_BD64` |
| 251 | `sub_BD64` | `$BD64` | 25 | `prg.asm:9669-9685` | `-` | `RTS` | `JustCallBD64` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` |
| 252 | `JustCallBD81` | `$BD7D` | 4 | `prg.asm:9689-9692` | `-` | `RTS` | `ObjCode_2_Hanezo`, `ObjCode_5_Dokuron`, `ObjCode_8_PowerCoin` | `sub_BD81` |
| 253 | `sub_BD81` | `$BD81` | 26 | `prg.asm:9696-9712` | `-` | `RTS` | `JustCallBD81` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` |
| 254 | `ObjCode_5a` | `$BD9B` | 5 | `prg.asm:9716-9719` | `-` | `RTS` | `ObjCode_5_Dokuron` | `sub_BE2F` |
| 255 | `sub_BDA0` | `$BDA0` | 34 | `prg.asm:9723-9747` | `X` | `RTS` | `ObjInitCode_3a` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` |
| 256 | `sub_BDC2` | `$BDC2` | 5 | `prg.asm:9751-9754` | `-` | `RTS` | `ObjCode_6_Desufa` | `sub_BE2F` |
| 257 | `sub_BDC7` | `$BDC7` | 34 | `prg.asm:9758-9782` | `X` | `RTS` | `ObjInitCode_3a` | `sub_B765`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy` |
| 258 | `sub_BDE9` | `$BDE9` | 41 | `prg.asm:9788-9814` | `Y` | `RTS` | `ObjCode_4a` | `CalculateModulus` |
| 259 | `Negate_Mod_Number` | `$BE12` | 16 | `prg.asm:9818-9825` | `-` | `RTS` | `ObjCode_4a`, `sub_BD64`, `sub_BD81` (+3) | `NegateYYXX` |
| 260 | `Store_Mod_Number_ToEnemy` | `$BE22` | 13 | `prg.asm:9831-9839` | `Y` | `RTS` | `ObjInit_2_Hanezo`, `ObjInit_5_Dokuron`, `ObjInit_6_Desufa` (+7) | - |
| 261 | `sub_BE2F` | `$BE2F` | 43 | `prg.asm:9845-9877` | `Y` | `RTS` | `ObjCode_5a`, `sub_BDC2` | - |
| 262 | `ObjInitCode_3a` | `$BE5A` | 48 | `prg.asm:9881-9909` | `-` | `RTS` | `ObjCode_3_GejiShogun` | `SetXIfPlayerAndEnemyVarMatch`, `sub_BDA0`, `sub_BDC7` |
| 263 | `SetXIfPlayerAndEnemyVarMatch` | `$BE8A` | 11 | `prg.asm:9913-9922` | `Y` | `RTS` | `ObjInitCode_3a` | - |
| 264 | `ObjCode_7a` | `$BE95` | 82 | `prg.asm:9926-9969` | `X` | `RTS` | `ObjCode_7_Horus` | `sub_B765`, `SubtractPlayerAndEnemyVarMaybe`, `sub_BF17` (+6) |
| 265 | `sub_BEE7` | `$BEE7` | 31 | `prg.asm:9973-9995` | `-` | `RTS` | `ObjCode_7a` | - |
| 266 | `SubtractPlayerAndEnemyVarMaybe` | `$BF06` | 17 | `prg.asm:9999-10013` | `Y` | `RTS` | `ObjCode_7a` | - |
| 267 | `sub_BF17` | `$BF17` | 55 | `prg.asm:10017-10049` | `-` | `RTS` | `ObjCode_7a` | - |
| 268 | `sub_BF4E` | `$BF4E` | 55 | `prg.asm:10053-10085` | `-` | `RTS` | `ObjCode_7a` | - |
| 269 | `sub_BF85` | `$BF85` | 55 | `prg.asm:10089-10126` | `-` | `RTS` | `ObjCode_7a` | - |
| 270 | `sub_BFBC` | `$BFBC` | 22 | `prg.asm:10130-10145` | `-` | `RTS` | `ObjCode_7a` | `Negate_Mod_Number` |
| 271 | `ResetEitherXOrYVelocity` | `$BFD2` | 46 | `prg.asm:10149-10159` | `-` | `RTS` | `ObjCode_7a` | `ResetEnemyVelocity` |
| 272 | `HandleSound` | `$B0FC` | 72 | `music-engine.asm:2-43` | `-` | `RTS` | `NMI` | `HandleSoundQueue`, `HandleSoundChannel`, `WriteSoundRegisters` |
| 273 | `HandleSoundQueue` | `$B144` | 133 | `music-engine.asm:47-131` | `-` | `RTS` | `HandleSound` | - |
| 274 | `HandleSoundChannel` | `$B1C9` | 188 | `music-engine.asm:135-262` | `-` | `RTS` | `HandleSound` | `MaybeUpdateVolDelay`, `MaybeResetVolDelay`, `HandleSoundCommand` (+1) |
| 275 | `sub_B285` | `$B285` | 5 | `music-engine.asm:266-269` | `-` | `RTS` | `HandleSoundChannel`, `SoundCommand_Cx` | `CheckIfSoundChannelIsActive` |
| 276 | `SoundCommand_F6_F7_sub1` | `$B28A` | 31 | `music-engine.asm:273-290` | `-` | `RTS` | `SoundCommand_F6_F7` | - |
| 277 | `HandleSoundCommand` | `$B2A9` | 27 | `music-engine.asm:294-311` | `A` | `RTS` | `HandleSoundChannel` | `SoundCommand_8x_9x`, `SoundCommand_Ax_Bx`, `SoundCommand_Cx` (+4) |
| 278 | `SoundCommand_8x_9x` | `$B2C4` | 16 | `music-engine.asm:317-327` | `-` | `RTS` | `HandleSoundCommand` | - |
| 279 | `SoundCommand_Ax_Bx` | `$B2D4` | 28 | `music-engine.asm:333-349` | `-` | `RTS` | `HandleSoundCommand` | `MaybeResetVolDelay` |
| 280 | `SoundCommand_Cx` | `$B2F0` | 22 | `music-engine.asm:355-367` | `-` | `PLA;PLA (Stack Pop)` | `HandleSoundCommand` | `sub_B285` |
| 281 | `SoundCommand_Dx` | `$B306` | 46 | `music-engine.asm:371-404` | `-` | `RTS` | `HandleSoundCommand` | - |
| 282 | `SoundCommand_Ex` | `$B334` | 9 | `music-engine.asm:410-416` | `-` | `RTS` | `HandleSoundCommand` | - |
| 283 | `SoundCommand_Fx` | `$B33D` | 23 | `music-engine.asm:422-435` | `-` | `RTS` | `HandleSoundCommand` | `SoundCommand_F0`, `SoundCommand_F1`, `SoundCommand_F2` (+5) |
| 284 | `SoundCommand_F0` | `$B354` | 27 | `music-engine.asm:441-459` | `-` | `RTS` | `SoundCommand_Fx` | - |
| 285 | `SoundCommand_F1` | `$B36F` | 27 | `music-engine.asm:465-481` | `-` | `RTS` | `SoundCommand_Fx` | - |
| 286 | `SoundCommand_F2` | `$B38A` | 36 | `music-engine.asm:487-511` | `-` | `RTS` | `SoundCommand_Fx` | - |
| 287 | `SoundCommand_F3` | `$B3AE` | 76 | `music-engine.asm:515-559` | `-` | `RTS` | `SoundCommand_Fx` | - |
| 288 | `SoundCommand_F4` | `$B3FA` | 45 | `music-engine.asm:563-590` | `-` | `RTS` | `SoundCommand_Fx` | - |
| 289 | `SoundCommand_F5` | `$B427` | 11 | `music-engine.asm:594-601` | `-` | `RTS` | `SoundCommand_Fx` | - |
| 290 | `SoundCommand_F6_F7` | `$B432` | 45 | `music-engine.asm:605-630` | `-` | `PLA;PLA (Stack Pop)` | `SoundCommand_Fx` | `SoundCommand_F6_F7_sub1` |
| 291 | `MaybeUpdateVolDelay` | `$B45F` | 12 | `music-engine.asm:634-642` | `-` | `RTS` | `HandleSoundChannel` | - |
| 292 | `MaybeResetVolDelay` | `$B46B` | 101 | `music-engine.asm:646-708` | `-` | `RTS` | `HandleSoundChannel`, `SoundCommand_Ax_Bx` | `CheckIfSoundChannelIsActive` |
| 293 | `WriteSoundRegisters` | `$B4D0` | 61 | `music-engine.asm:712-747` | `-` | `RTS` | `HandleSound` | - |
| 294 | `CheckIfSoundChannelIsActive` | `$B50D` | 99 | `music-engine.asm:751-771` | `-` | `RTS` | `sub_B285`, `MaybeResetVolDelay` | - |

---

## 6. Referencias Cruzadas de Variables RAM Anónimas (`byte_XX` / `off_XX` / `Maybe...`)

Para acelerar `TASK-RES-004`, la siguiente tabla muestra qué funciones leen (`R`) y escriben (`W`) cada variable RAM aún no documentada o tentativa:

| Dir. RAM | Etiqueta Actual | Tamaño | Categoría | Funciones que Leen (`R`) | Funciones que Escriben (`W`) | Comentario en `ram.asm` |
| :---: | :--- | :---: | :---: | :--- | :--- | :--- |
| `$0000` | `byte_0` | `1` | `AUTO_IDA` | `HandlePPUUpdates`, `ReadJoypad`, `MainSub_1`, `DrawScore`, `sub_9592` (+15) | `RESET`, `HandlePPUUpdates`, `ReadJoypad`, `MainSub_1`, `DrawScore` (+15) | - |
| `$0001` | `byte_1` | `1` | `AUTO_IDA` | `DrawScoreDigitMaybe`, `DrawFullScreen`, `sub_99EA`, `sub_9A03`, `sub_9A7A` (+13) | `RESET`, `HandlePPUUpdates`, `DrawScore`, `DrawScoreDigitMaybe`, `DrawFullScreen` (+14) | - |
| `$0002` | `off_2` | `2` | `AUTO_IDA` | `HandleDrawHalfRow`, `HandleSoundChannel`, `SoundCommand_F0`, `SoundCommand_F1`, `SoundCommand_F3` (+1) | `LoadRoomStuff`, `HandleSoundChannel`, `SoundCommand_F0`, `SoundCommand_F2`, `SoundCommand_F3` (+1) | - |
| `$0004` | `off_4` | `2` | `AUTO_IDA` | `HandleDrawHalfRow`, `DrawFullScreen`, `sub_9B6A`, `MainSub_3`, `HandleSoundChannel` (+1) | `HandleDrawHalfRow`, `HandleSoundChannel`, `MaybeResetVolDelay` | - |
| `$0006` | `byte_6` | `1` | `AUTO_IDA` | `DrawFullScreen`, `sub_9716`, `sub_9B6A`, `MainSub_3`, `sub_A875` (+6) | `LoadLayoutChunk`, `sub_9B6A`, `HandleSoundCommand` | - |
| `$0007` | `byte_7` | `1` | `AUTO_IDA` | `LoadLayoutChunk`, `sub_9A03`, `sub_9A7A` | `LoadLayoutChunk`, `sub_9A03`, `sub_9A7A`, `MainSub_3` | - |
| `$0008` | `byte_8` | `1` | `AUTO_IDA` | `HandleDrawHalfRow`, `MainSub_3`, `ChooseEnemySpawnPoint`, `SoundCommand_F4` | `DrawFullScreen`, `sub_9A03`, `sub_9A7A`, `MainSub_3`, `SpawnLocFunc2` (+2) | - |
| `$0009` | `byte_9` | `1` | `AUTO_IDA` | `DrawFullScreen`, `sub_9A03`, `sub_9A7A`, `sub_9B39`, `sub_9B6A` (+4) | `DrawFullScreen`, `sub_92AD`, `sub_9A03`, `sub_9A7A`, `sub_9B6A` (+5) | - |
| `$000A` | `byte_A` | `1` | `AUTO_IDA` | `sub_9B6A`, `BigBeefySubHandlesBGColliding`, `sub_A875`, `CheckIfSoundChannelIsActive` | `sub_9B6A`, `BigBeefySubHandlesBGColliding`, `sub_A875`, `CheckIfSoundChannelIsActive` | - |
| `$000B` | `byte_B` | `1` | `AUTO_IDA` | `ReadRoomData`, `JT_94AD_0`, `BigBeefySubHandlesBGColliding`, `HandleTileAttributes` | `GameState_2_LoadRoom`, `ReadRoomData`, `BigBeefySubHandlesBGColliding`, `HandleTileAttributes`, `CheckIfSoundChannelIsActive` | - |
| `$000C` | `byte_C` | `1` | `AUTO_IDA` | `ReadRoomData`, `HandleTileAttributes`, `ChooseEnemySpawnPoint`, `SpawnLocFunc2` | `ReadRoomData`, `HandleTileAttributes`, `SpawnLocFunc2` | - |
| `$000D` | `byte_D` | `1` | `AUTO_IDA` | `JT_94AD_multi` | `ReadRoomData`, `HandleTileAttributes` | - |
| `$000E` | `byte_E` | `1` | `AUTO_IDA` | `ReadRoomData`, `HandleRoomDataObject`, `JT_94AD_0`, `JT_94AD_multi`, `JT_94AD_5_6` (+5) | `ReadRoomData`, `HandleRoomDataObject`, `BigBeefySubHandlesBGColliding`, `MainSub_3` | - |
| `$000F` | `byte_F` | `1` | `AUTO_IDA` | `sub_9276`, `IfNotTitleScreenRoomDoThings`, `ReadRoomData`, `JT_94AD_multi`, `JT_94AD_7` (+2) | `sub_9276`, `IfNotTitleScreenRoomDoThings`, `ReadRoomData`, `JT_94AD_multi`, `JT_94AD_E` | - |
| `$0010` | `byte_10` | `1` | `AUTO_IDA` | `AddScore`, `sub_9813`, `HandleOpenDoorMaybe`, `EndingState_2`, `HandleSoundChannel` | `AddScore`, `sub_9813`, `HandleOpenDoorMaybe`, `EndingState_2`, `HandleSoundChannel` | - |
| `$0011` | `byte_11` | `1` | `AUTO_IDA` | `sub_9813`, `EndingState_2`, `sub_B094`, `sub_BA7B`, `HandleSoundChannel` | `sub_9813`, `EndingState_2`, `sub_B094`, `sub_BA7B`, `HandleSoundChannel` | - |
| `$0012` | `byte_12` | `1` | `AUTO_IDA` | `MaybeSpawnThingFromObject`, `sub_B645` | `MaybeSpawnThingFromObject`, `sub_B645` | - |
| `$0013` | `byte_13` | `1` | `AUTO_IDA` | `EndingState_2`, `sub_B645` | `EndingState_2`, `sub_B645` | - |
| `$0014` | `byte_14` | `1` | `AUTO_IDA` | `sub_9592` | `sub_9592` | - |
| `$0031` | `byte_31` | `1` | `AUTO_IDA` | - | `JumpTable` | - |
| `$003D` | `off_3D` | `2` | `AUTO_IDA` | `MaybeLoadRoomFlags`, `DrawFullScreen`, `sub_92AD`, `sub_9A03`, `sub_9A7A` (+3) | `MaybeLoadRoomFlags`, `DrawTitleScreen` | - |
| `$0040` | `MaybePauseFlag` | `1` | `TENTATIVE` | `GameState_4_EnterRoom`, `GameState_5_Main`, `MaybePause` | `GameState_4_EnterRoom`, `MaybePause` | - |
| `$0048` | `byte_48` | `1` | `AUTO_IDA` | `WriteStringToPPU` | `WriteStringToPPU` | - |
| `$0049` | `byte_49` | `1` | `AUTO_IDA` | - | `WriteStringToPPU` | - |
| `$004B` | `byte_4B` | `1` | `AUTO_IDA` | `WriteSprite` | `WriteSprite` | - |
| `$004C` | `byte_4C` | `1` | `AUTO_IDA` | `WriteSprite` | `WriteSprite` | - |
| `$004D` | `byte_4D` | `1` | `AUTO_IDA` | `WriteSprite` | `WriteSprite` | - |
| `$004E` | `byte_4E` | `1` | `AUTO_IDA` | `WriteSprite` | `WriteSprite` | - |
| `$004F` | `byte_4F` | `1` | `AUTO_IDA` | `WriteSprite` | `WriteSprite` | - |
| `$0050` | `word_50` | `2` | `AUTO_IDA` | `HandlePPUUpdates`, `WriteSprite`, `LoadRoomPalette`, `MaybeLoadRoomFlags`, `LoadRoomStuff` (+7) | `LoadPointerTo050`, `WriteSprite`, `LoadRoomPalette`, `LoadRoomStuff`, `LoadRoomData` (+2) | - |
| `$0054` | `byte_54` | `1` | `AUTO_IDA` | `JT_98A4_multi` | `JT_98A4_multi`, `JT_98A4_8_PowerCoin` | - |
| `$0055` | `unk_55` | `1` | `AUTO_IDA` | - | - | - |
| `$0056` | `byte_56` | `1` | `AUTO_IDA` | `MaybeDrawEnemySprites` | `MaybeDrawEnemySprites` | - |
| `$0057` | `UnknownDoorFlag` | `1` | `TENTATIVE` | `sub_9592`, `JTPickup_Door` | `sub_9592` | - |
| `$0058` | `off_58` | `2` | `AUTO_IDA` | `GameState_2_LoadRoom`, `ReadRoomData`, `JT_94AD_7`, `JT_94AD_8_Door`, `sub_9592` (+1) | `LoadRoomData`, `sub_9592` | - |
| `$005A` | `byte_5A` | `1` | `AUTO_IDA` | `GameState_2_LoadRoom`, `sub_9592`, `BigBeefySubHandlesBGColliding`, `MaybePickupItem`, `JTPickup_Bomb` (+5) | `IfNotTitleScreenRoomDoThings`, `GameState_2_LoadRoom`, `sub_9592`, `BigBeefySubHandlesBGColliding` | - |
| `$005B` | `byte_5B` | `1` | `AUTO_IDA` | - | `GameState_2_LoadRoom` | - |
| `$005C` | `byte_5C` | `1` | `AUTO_IDA` | `sub_9592`, `sub_96F5`, `BigBeefySubHandlesBGColliding`, `JTPickup_Chest`, `JTPickup_Door` (+2) | `sub_9592` | - |
| `$005E` | `MaybeCollectedThing` | `1` | `TENTATIVE` | `sub_96F5`, `BigBeefySubHandlesBGColliding`, `JTPickup_Chest`, `JTPickup_Door`, `GameState_9_ExitRoom` | `sub_9592` | - |
| `$005F` | `byte_5F` | `1` | `AUTO_IDA` | `IfNotTitleScreenRoomDoThings`, `GameState_2_LoadRoom`, `ReadRoomData` | `GameState_2_LoadRoom` | - |
| `$0070` | `byte_70` | `1` | `AUTO_IDA` | `sub_96B7`, `sub_96F5`, `BigBeefySubHandlesBGColliding`, `MaybePickupItem`, `JTPickup_Chest` (+2) | `sub_9592`, `BigBeefySubHandlesBGColliding` | - |
| `$0071` | `byte_71` | `1` | `AUTO_IDA` | `BigBeefySubHandlesBGColliding` | `sub_9592` | - |
| `$0072` | `MaybeTileCollected` | `1` | `TENTATIVE` | `sub_96B7`, `sub_A875` | `sub_9592` | - |
| `$0073` | `byte_73` | `1` | `AUTO_IDA` | - | `sub_9592` | - |
| `$0074` | `byte_74` | `1` | `AUTO_IDA` | `sub_A1A6` | `JTPickup_Chest` | - |
| `$0078` | `off_78` | `2` | `AUTO_IDA` | `sub_9813`, `sub_9965`, `sub_9A03`, `sub_9A7A`, `sub_9B6A` (+1) | `HandleSpawnsAndMore`, `sub_9965`, `sub_A1B9` | - |
| `$0090` | `byte_90` | `1` | `AUTO_IDA` | `JTPickup_Bomb` | `ReadRoomData` | - |
| `$0091` | `byte_91` | `1` | `AUTO_IDA` | - | `ReadRoomData` | - |
| `$0092` | `byte_92` | `1` | `AUTO_IDA` | `MaybeSpawnThingFromObject`, `sub_A270`, `InitPowerCoin`, `SpawnLocFunc2` | `sub_9276`, `InitPowerCoin`, `SpawnLocFunc1` | - |
| `$0094` | `byte_94` | `1` | `AUTO_IDA` | `MaybeSpawnThingFromObject`, `sub_A270`, `InitPowerCoin`, `SpawnLocFunc2` | `sub_9276`, `InitPowerCoin`, `SpawnLocFunc1` | - |
| `$0098` | `byte_98` | `1` | `AUTO_IDA` | `sub_A33E`, `sub_B7BF` | `MainSub_3`, `sub_A843` | - |
| `$0099` | `byte_99` | `1` | `AUTO_IDA` | `sub_A33E`, `sub_B7BF` | `MainSub_3`, `sub_A843` | - |
| `$009A` | `byte_9A` | `1` | `AUTO_IDA` | - | `MainSub_3`, `sub_A851` | - |
| `$009B` | `byte_9B` | `1` | `AUTO_IDA` | - | `MainSub_3`, `sub_A851` | - |
| `$00A8` | `TimerStatusMaybe` | `1` | `TENTATIVE` | `GameState_5_Main`, `GameState_6_TimeOver`, `GameState_8_RoundClear`, `HandleStageTimer`, `GameState_2_LoadRoom` | `GameState_5_Main`, `GameState_6_TimeOver`, `GameState_8_RoundClear`, `LoseALifeMaybeGameOver`, `HandleStageTimer` (+2) | - |
| `$00AC` | `byte_AC` | `1` | `AUTO_IDA` | `ReadRoomData`, `JT_94AD_0`, `sub_9592` | `IfNotTitleScreenRoomDoThings`, `ReadRoomData`, `LoadRoomData`, `sub_9592` | - |
| `$00BA` | `byte_BA` | `1` | `AUTO_IDA` | `sub_B645` | `sub_B645` | - |
| `$00C2` | `byte_C2` | `1` | `AUTO_IDA` | `DrawEnemies`, `sub_BF17`, `sub_BF4E` | `DrawEnemies`, `ObjCode_7a` | - |
| `$00C3` | `byte_C3` | `1` | `AUTO_IDA` | `sub_BF17`, `sub_BF4E` | `ObjCode_7a` | - |
| `$00C5` | `byte_C5` | `1` | `AUTO_IDA` | `ObjCode_7a` | `ObjCode_7a` | - |
| `$00C7` | `byte_C7` | `1` | `AUTO_IDA` | `ObjCode_7a`, `sub_BEE7`, `sub_BF85`, `sub_BFBC`, `ResetEitherXOrYVelocity` | `sub_BF85` | - |
| `$00CA` | `byte_CA` | `1` | `AUTO_IDA` | `sub_BF85` | `sub_BF17`, `sub_BF4E` | - |
| `$00CB` | `byte_CB` | `1` | `AUTO_IDA` | - | `ObjCode_7a`, `sub_BF17`, `sub_BF4E` | - |
| `$00CC` | `byte_CC` | `1` | `AUTO_IDA` | - | `sub_BF17`, `sub_BF4E` | - |
| `$00CD` | `byte_CD` | `1` | `AUTO_IDA` | - | `sub_BF17`, `sub_BF4E` | - |
| `$00CF` | `byte_CF` | `1` | `AUTO_IDA` | `DrawEnemies` | `DrawEnemies` | - |
| `$00EE` | `byte_EE` | `1` | `AUTO_IDA` | `BigBeefySubHandlesBGColliding` | `BigBeefySubHandlesBGColliding` | - |
| `$00F0` | `byte_F0` | `1` | `AUTO_IDA` | `BigBeefySubHandlesBGColliding`, `MaybeSpawnThingFromObject`, `InitPowerCoin` | `BigBeefySubHandlesBGColliding`, `JTPickup_Chest`, `MaybeOpenHiddenBlock` | - |
| `$00F1` | `_MaybeBlockSpawningObj` | `1` | `TENTATIVE` | `MaybeOpenHiddenBlock` | `JTPickup_Chest`, `MaybeOpenHiddenBlock`, `MaybeSpawnThingFromObject` | - |
| `$00F4` | `byte_F4` | `1` | `AUTO_IDA` | `sub_92AD`, `sub_9A7A`, `JTPickup_Chest`, `sub_A18E` | `DrawFullScreen`, `sub_9A7A`, `sub_A18E` | - |
| `$00F7` | `byte_F7` | `1` | `AUTO_IDA` | `JTPickup_Bomb` | `GameState_3_DrawScreen`, `JTPickup_Bomb` | - |
| `$00F8` | `MaybeBombThing` | `1` | `TENTATIVE` | `JTPickup_Bomb`, `DoorCond_0C_AllFireBombs` | `GameState_2_LoadRoom`, `JTPickup_Bomb`, `DoorCond_0C_AllFireBombs` | - |
| `$00FE` | `UnknownFlag_0FE` | `1` | `TENTATIVE` | `CheckObjectCollision`, `sub_B645` | `MainSub_1`, `sub_B645` | - |
| `$00FF` | `unk_FF` | `1` | `AUTO_IDA` | - | - | - |
| `$030A` | `unk_30A` | `1` | `AUTO_IDA` | - | - | - |
| `$030B` | `byte_30B` | `1` | `AUTO_IDA` | - | `sub_8FB1` | - |
| `$030C` | `byte_30C` | `1` | `AUTO_IDA` | - | `sub_8FB1` | - |
| `$030D` | `byte_30D` | `1` | `AUTO_IDA` | - | `sub_8FB1` | - |
| `$030E` | `byte_30E` | `1` | `AUTO_IDA` | - | `sub_8FB1` | - |
| `$030F` | `byte_30F` | `1` | `AUTO_IDA` | `sub_9965`, `sub_9B6A` | `sub_9965`, `sub_9B6A` | - |
| `$0310` | `byte_310` | `1` | `AUTO_IDA` | `BigBeefySubHandlesBGColliding`, `JTPickup_Chest` | `BigBeefySubHandlesBGColliding` | - |
| `$0311` | `byte_311` | `1` | `AUTO_IDA` | `sub_A18E` | `JTPickup_Chest` | - |
| `$0312` | `byte_312` | `1` | `AUTO_IDA` | `sub_9965`, `BigBeefySubHandlesBGColliding` | `BigBeefySubHandlesBGColliding`, `JTPickup_Chest`, `JTPickup_Door`, `MaybeOpenHiddenBlock`, `sub_A18E` | - |
| `$0313` | `byte_313` | `1` | `AUTO_IDA` | `MaybeOpenHiddenBlock` | `JTPickup_Chest` | - |
| `$0314` | `byte_314` | `1` | `AUTO_IDA` | `sub_9A7A`, `sub_A1B9` | `HandleSpawnsAndMore`, `sub_9965` | - |
| `$0320` | `byte_320` | `1` | `AUTO_IDA` | `sub_9B6A`, `BigBeefySubHandlesBGColliding` | `BigBeefySubHandlesBGColliding` | - |
| `$0330` | `byte_330` | `1` | `AUTO_IDA` | `HandleSpawnsAndMore`, `BigBeefySubHandlesBGColliding` | `HandleSpawnsAndMore`, `BigBeefySubHandlesBGColliding` | - |
| `$0332` | `byte_332` | `1` | `AUTO_IDA` | `sub_9813` | `sub_9813` | - |
| `$0333` | `byte_333` | `1` | `AUTO_IDA` | `BigBeefySubHandlesBGColliding` | `UseMightyCoin`, `BigBeefySubHandlesBGColliding`, `sub_A1EB` | - |
| `$0334` | `byte_334` | `1` | `AUTO_IDA` | `BigBeefySubHandlesBGColliding` | `BigBeefySubHandlesBGColliding` | - |
| `$0335` | `byte_335` | `1` | `AUTO_IDA` | `MaybeOpenHiddenBlock` | `MaybeOpenHiddenBlock` | - |
| `$0336` | `byte_336` | `1` | `AUTO_IDA` | `MaybeOpenHiddenBlock` | `MaybeOpenHiddenBlock` | - |
| `$0337` | `MaybeDoorReturnTwiceFlag` | `1` | `TENTATIVE` | `AnimateDoorClose`, `AnimateDoor` | `AnimateDoor` | DATA XREF: AnimateDoorClose+7r ; $0337 |
| `$0338` | `byte_338` | `1` | `AUTO_IDA` | `AnimateDoor` | `AnimateDoor` | - |
| `$0339` | `byte_339` | `1` | `AUTO_IDA` | `AnimateDoor` | `AnimateDoor` | - |
| `$033C` | `byte_33C` | `1` | `AUTO_IDA` | `GameState_4_EnterRoom` | `GameState_0_Init`, `GameState_9_ExitRoom` | - |
| `$033E` | `byte_33E` | `1` | `AUTO_IDA` | `GameState_7_GoToTortureRoom` | `GameState_2_LoadRoom` | - |
| `$0340` | `byte_340` | `1` | `AUTO_IDA` | `GameState_4_EnterRoom`, `GameState_9_ExitRoom` | `GameState_3_DrawScreen`, `GameState_4_EnterRoom`, `GameState_9_ExitRoom` | - |
| `$0341` | `byte_341` | `1` | `AUTO_IDA` | `GameState_4_EnterRoom` | `GameState_4_EnterRoom` | - |
| `$0343` | `byte_343` | `1` | `AUTO_IDA` | `GameState_4_EnterRoom` | `GameState_4_EnterRoom` | - |
| `$0346` | `MaybeTempSpriteIndex` | `1` | `TENTATIVE` | `MaybeDrawEnemySprites` | `MaybeDrawEnemySprites` | - |
| `$0356` | `MaybeTempCollectableFlag2` | `1` | `TENTATIVE` | `JTPickup_Bomb` | `GameState_0_Init`, `GameState_3_DrawScreen`, `MainSub_1`, `JTPickup_Bomb`, `MaybeConsiderOffscreen` | - |
| `$0357` | `MaybeTempCollectableFlag` | `1` | `TENTATIVE` | `GameState_3_DrawScreen`, `JTPickup_Bomb`, `MaybeOpenHiddenBlock`, `SpawnPowerCoin` | `GameState_0_Init`, `GameState_3_DrawScreen`, `MainSub_1`, `JTPickup_Bomb`, `MaybeConsiderOffscreen` (+1) | DATA XREF: GameState_0_Init+2w ; $0357 |
| `$0358` | `MaybeCollectedThing2` | `1` | `TENTATIVE` | `MaybeAward1UP` | `JT_98A4_10_Brother`, `JTPickup_TecmoPlate`, `MaybeAward1UP` | - |
| `$0364` | `byte_364` | `1` | `AUTO_IDA` | `GameState_3_DrawScreen`, `AddScore`, `JT_98A4_multi`, `JT_98A4_E` | `GameState_0_Init`, `GameState_3_DrawScreen`, `AddScore`, `JT_98A4_multi`, `JT_98A4_E` | - |
| `$0368` | `SphinxCollectedMaybe` | `1` | `TENTATIVE` | `JTPickup_Sphinx` | `sub_9592` | - |
| `$0369` | `MaybeHandleDoorFlag` | `1` | `TENTATIVE` | `MaybeHandleDoorOpening` | `GameState_3_DrawScreen`, `sub_9B39`, `MaybeHandleDoorOpening` | - |
| `$036B` | `MaybeDoorTypeOrSphinx` | `1` | `TENTATIVE` | `MaybeHandleDoorOpening`, `MaybeDontOpenDoors`, `HandleOpenDoorMaybe` | `MaybeHandleDoorOpening` | - |
| `$0372` | `byte_372` | `1` | `AUTO_IDA` | `EndingState_2` | `EndingState_1`, `EndingState_2` | - |
| `$0373` | `byte_373` | `1` | `AUTO_IDA` | `sub_AFA9` | `EndingState_1`, `sub_AFA9` | - |
| `$0374` | `byte_374` | `1` | `AUTO_IDA` | `sub_AFA9` | `EndingState_1`, `sub_AFA9` | - |
| `$0375` | `byte_375` | `1` | `AUTO_IDA` | `sub_AFA9` | `EndingState_1`, `sub_AFA9` | - |
| `$0376` | `byte_376` | `1` | `AUTO_IDA` | `sub_AFA9` | `EndingState_1`, `sub_AFA9` | - |
| `$0379` | `byte_379` | `1` | `AUTO_IDA` | `EndingState_2` | `EndingState_1`, `EndingState_2` | - |
| `$037A` | `byte_37A` | `1` | `AUTO_IDA` | `EndingState_2` | `EndingState_2` | - |
| `$037B` | `byte_37B` | `1` | `AUTO_IDA` | `EndingState_4` | `EndingState_3` | - |
| `$037C` | `byte_37C` | `1` | `AUTO_IDA` | `EndingState_2` | `EndingState_1`, `EndingState_2` | - |
| `$0393` | `DoorIsAnimatingMaybe` | `1` | `TENTATIVE` | `JTPickup_Door`, `MaybeDontOpenDoors` | `BigBeefySubHandlesBGColliding`, `JTPickup_Door` | - |
| `$0394` | `UnknownDoorFlagCopy` | `1` | `TENTATIVE` | `MaybeDontOpenDoors` | `JTPickup_Door` | - |
| `$0396` | `DoorCheckFlag_1E_Unk` | `1` | `TENTATIVE` | `DoorCond_1E_R5R11Warp` | `GameState_8_RoundClear`, `DoorCond_1E_R5R11Warp` | - |
| `$039E` | `MaybeSphinxFlags` | `1` | `TENTATIVE` | `sub_9592`, `HandleOpenDoorMaybe` | `JT_94AD_8_Door`, `HandleOpenDoorMaybe` | - |
| `$03C0` | `CheckpointRoomIDMaybe` | `1` | `TENTATIVE` | `MaybeGoBackToOldRoom` | `GameState_0_Init`, `GameState_7_GoToTortureRoom`, `GameState_2_LoadRoom` | - |
| `$03C1` | `byte_3C1` | `1` | `AUTO_IDA` | `MainSub_3` | `DrawFullScreen`, `MainSub_3` | - |
| `$03C2` | `byte_3C2` | `1` | `AUTO_IDA` | `HandlePPUUpdates`, `GameState_4_EnterRoom`, `sub_9276`, `sub_9A03`, `MainSub_3` (+2) | `WriteStringToPPU`, `DrawFullScreen`, `MainSub_3` | - |
| `$03C3` | `byte_3C3` | `1` | `AUTO_IDA` | `MainSub_3` | `DrawFullScreen`, `MainSub_3` | - |
| `$03C4` | `byte_3C4` | `1` | `AUTO_IDA` | `HandlePPUUpdates`, `sub_9276`, `sub_9A7A`, `sub_A1B9`, `sub_A1EB` (+1) | `WriteStringToPPU`, `DrawFullScreen`, `sub_A1B9`, `sub_A1EB`, `MainSub_3` | - |
| `$03C6` | `byte_3C6` | `1` | `AUTO_IDA` | `MainSub_3`, `sub_A875` | `MainSub_3` | - |
| `$03C7` | `MaybeRoomIDCopyAgain` | `1` | `TENTATIVE` | `MainSub_3`, `sub_A85F` | `MainSub_3` | - |
| `$03C8` | `byte_3C8` | `1` | `AUTO_IDA` | `MainSub_3` | `MainSub_3` | - |
| `$03C9` | `byte_3C9` | `1` | `AUTO_IDA` | `MainSub_3` | `MainSub_3` | - |
| `$03CA` | `byte_3CA` | `1` | `AUTO_IDA` | `MainSub_3` | `MainSub_3` | - |
| `$03CB` | `byte_3CB` | `1` | `AUTO_IDA` | `MainSub_3` | `MainSub_3` | - |
| `$03D5` | `MaybeDeathTimer` | `1` | `TENTATIVE` | `MainSub_1` | `MainSub_1` | - |
| `$03DD` | `byte_3DD` | `1` | `AUTO_IDA` | `DrawPlayer` | `GameState_4_EnterRoom`, `MainSub_1`, `sub_B0AF` | - |
| `$03DF` | `byte_3DF` | `1` | `AUTO_IDA` | `MainSub_1` | `MainSub_1` | - |
| `$03E6` | `byte_3E6` | `1` | `AUTO_IDA` | - | `MainSub_1` | - |
| `$03E7` | `byte_3E7` | `1` | `AUTO_IDA` | `MainSub_1`, `EndingState_1` | `MainSub_1`, `sub_A1B9`, `sub_A1EB`, `EndingState_3`, `EndingState_4` | - |
| `$03EE` | `byte_3EE` | `1` | `AUTO_IDA` | `HandlePPUUpdates` | `HandlePPUUpdates`, `MainSub_3`, `sub_AFA9` | - |
| `$03EF` | `byte_3EF` | `1` | `AUTO_IDA` | `HandlePPUUpdates`, `sub_92AD`, `sub_92E2`, `sub_9729`, `MainSub_3` (+1) | `HandlePPUUpdates`, `DrawFullScreen`, `sub_92AD`, `sub_92E2`, `MainSub_3` (+1) | - |
| `$03F0` | `byte_3F0` | `1` | `AUTO_IDA` | `HandlePPUUpdates`, `DrawFullScreen`, `sub_92AD`, `sub_9729`, `MainSub_3` (+1) | `HandlePPUUpdates`, `DrawFullScreen`, `sub_92AD`, `sub_92E2`, `MainSub_3` (+1) | - |
| `$03F1` | `byte_3F1` | `1` | `AUTO_IDA` | `HandlePPUUpdates` | `HandlePPUUpdates`, `MainSub_3`, `sub_AFA9` | - |
| `$03F3` | `unk_3F3` | `1` | `AUTO_IDA` | - | `MainSub_3`, `sub_A875` | - |
| `$05A3` | `byte_5A3` | `1` | `AUTO_IDA` | - | - | - |
| `$0683` | `byte_683` | `1` | `AUTO_IDA` | - | `IfNotTitleScreenRoomDoThings` | - |
| `$0786` | `byte_786` | `1` | `AUTO_IDA` | `sub_A29C`, `sub_A33E` | `sub_A270`, `sub_A29C`, `sub_A33E` | - |
| `$0787` | `byte_787` | `1` | `AUTO_IDA` | `sub_A29C` | `sub_A270` | - |
| `$0789` | `byte_789` | `1` | `AUTO_IDA` | `sub_A29C` | `sub_A270` | - |
| `$078B` | `byte_78B` | `1` | `AUTO_IDA` | `sub_A29C` | `sub_A270` | - |
| `$078C` | `byte_78C` | `1` | `AUTO_IDA` | `sub_A29C` | `sub_A29C` | - |
| `$078D` | `byte_78D` | `1` | `AUTO_IDA` | `sub_A29C` | `sub_A29C` | - |

---

## 7. Referencias Cruzadas

- **Script Generador**: [tools/extract_asm_inventory.py](file:///c:/Users/kanel/Documents/mjk/tools/extract_asm_inventory.py)
- **Base de Datos JSON**: [tools/asm_inventory.json](file:///c:/Users/kanel/Documents/mjk/tools/asm_inventory.json)
- **Investigaciones Relacionadas**: [`RES-001`](file:///c:/Users/kanel/Documents/mjk/research/RES-001-mapa-memoria-y-subsistemas-nes.md), [`RES-002`](file:///c:/Users/kanel/Documents/mjk/research/RES-002-analisis-naturaleza-desensamblado-asm.md)
