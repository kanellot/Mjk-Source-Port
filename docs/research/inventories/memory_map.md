# Mapa Exhaustivo de Memoria RAM ($0000-$07FF): Mighty Bomb Jack

- **Fase**: Fase 1 — Inventario Estático Exhaustivo
- **Total Variables Catalogadas**: 337
- **Fuente de Verdad**: `asm/src/ram.asm` y `asm/src/defs.asm`

## Distribución de Zonas de Memoria

| Rango | Zona | Tamaño | Descripción Funcional |
| :---: | :--- | :---: | :--- |
| `$0000 - $001F` | Zero-Page Scratchpad | 32 B | Variables temporales y punteros indirectos |
| `$0020 - $00FF` | Zero-Page Crítica | 224 B | Coordenadas, timers, físicas, estado de Jack |
| `$0100 - $01FF` | Stack Hardware 6502 | 256 B | Pila de llamadas y saltos indirectos |
| `$0200 - $02FF` | Buffer OAM Sprites | 256 B | 64 sprites volcados vía DMA $4014 |
| `$0300 - $037F` | Joypads y Timers | 128 B | Control de entrada, flancos y temporizadores |
| `$0380 - $03F0` | Buffer VRAM PPU | 113 B | Cola lineal para volcado durante VBlank |
| `$0400 - $04FF` | Estructura Jugador/Sala | 256 B | PlayerStruct, Room ID, cofres, puertas |
| `$0500 - $05FF` | Atributos y Baldosas | 256 B | Buffers de Nametables y paletas |
| `$0600 - $07FF` | Slots de Enemigos | 512 B | Arreglos de EnemyStruct de 28 bytes |

## Catálogo Detallado de Variables RAM

| Dirección | Símbolo | Tamaño | Categoría | Lectores | Escritores | Descripción |
| :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| `$0000` | `byte_0` | 1 | AUTO_IDA | 18 | 27 | - |
| `$0001` | `byte_1` | 1 | AUTO_IDA | 17 | 25 | - |
| `$0002` | `off_2` | 2 | AUTO_IDA | 6 | 6 | - |
| `$0004` | `off_4` | 2 | AUTO_IDA | 6 | 5 | - |
| `$0006` | `byte_6` | 1 | AUTO_IDA | 10 | 4 | - |
| `$0007` | `byte_7` | 1 | AUTO_IDA | 3 | 6 | - |
| `$0008` | `byte_8` | 1 | AUTO_IDA | 4 | 7 | - |
| `$0009` | `byte_9` | 1 | AUTO_IDA | 9 | 12 | - |
| `$000A` | `byte_A` | 1 | AUTO_IDA | 15 | 4 | - |
| `$000B` | `byte_B` | 1 | AUTO_IDA | 19 | 6 | - |
| `$000C` | `byte_C` | 1 | AUTO_IDA | 17 | 5 | - |
| `$000D` | `byte_D` | 1 | AUTO_IDA | 4 | 2 | - |
| `$000E` | `byte_E` | 1 | AUTO_IDA | 15 | 4 | - |
| `$000F` | `byte_F` | 1 | AUTO_IDA | 45 | 8 | - |
| `$0010` | `byte_10` | 1 | AUTO_IDA | 34 | 7 | - |
| `$0011` | `byte_11` | 1 | AUTO_IDA | 5 | 7 | - |
| `$0012` | `byte_12` | 1 | AUTO_IDA | 2 | 3 | - |
| `$0013` | `byte_13` | 1 | AUTO_IDA | 2 | 4 | - |
| `$0014` | `byte_14` | 1 | AUTO_IDA | 4 | 1 | - |
| `$001F` | `BrotherSpawnerCounter` | 1 | NAMED | 11 | 2 | - |
| `$0028` | `Mod_Number` | 2 | NAMED | 5 | 5 | - |
| `$002A` | `Mod_Modulus` | 1 | NAMED | 2 | 2 | - |
| `$002B` | `Mod_Remainder` | 1 | NAMED | 2 | 3 | - |
| `$002D` | `NMITemp_A` | 1 | NAMED | 1 | 1 | - |
| `$002E` | `NMITemp_X` | 1 | NAMED | 1 | 1 | - |
| `$002F` | `NMITemp_Y` | 1 | NAMED | 3 | 1 | - |
| `$0030` | `JumpTablePointer` | 1 | NAMED | 10 | 1 | - |
| `$0031` | `byte_31` | 1 | AUTO_IDA | 0 | 1 | - |
| `$0032` | `JumpTableTarget` | 2 | NAMED | 0 | 1 | - |
| `$0038` | `TempSpriteTile` | 1 | NAMED | 3 | 8 | - |
| `$0039` | `TempSpriteAttributesish` | 1 | NAMED | 1 | 5 | - |
| `$003A` | `TempSpriteX` | 1 | NAMED | 7 | 14 | - |
| `$003B` | `TempSpriteY` | 1 | NAMED | 4 | 10 | - |
| `$003C` | `CurrentSpriteIndex` | 1 | NAMED | 6 | 9 | - |
| `$003D` | `off_3D` | 2 | AUTO_IDA | 8 | 3 | - |
| `$003F` | `EndingState` | 1 | NAMED | 5 | 5 | - |
| `$0040` | `MaybePauseFlag` | 1 | TENTATIVE | 27 | 2 | - |
| `$0043` | `InGameFlag` | 1 | NAMED | 1 | 2 | - |
| `$0044` | `GameState` | 1 | NAMED | 3 | 19 | - |
| `$0045` | `TitleScreenState` | 1 | NAMED | 1 | 7 | - |
| `$0046` | `UpdatePaletteFlag` | 1 | NAMED | 1 | 5 | - |
| `$0047` | `RoundClearState` | 1 | NAMED | 2 | 2 | - |
| `$0048` | `byte_48` | 1 | AUTO_IDA | 2 | 1 | - |
| `$0049` | `byte_49` | 1 | AUTO_IDA | 1 | 1 | - |
| `$004A` | `SpriteOffset` | 1 | NAMED | 1 | 1 | - |
| `$004B` | `byte_4B` | 1 | AUTO_IDA | 1 | 2 | - |
| `$004C` | `byte_4C` | 1 | AUTO_IDA | 2 | 1 | - |
| `$004D` | `byte_4D` | 1 | AUTO_IDA | 1 | 1 | - |
| `$004E` | `byte_4E` | 1 | AUTO_IDA | 1 | 2 | - |
| `$004F` | `byte_4F` | 1 | AUTO_IDA | 0 | 2 | - |
| `$0050` | `word_50` | 2 | AUTO_IDA | 14 | 8 | - |
| `$0054` | `byte_54` | 1 | AUTO_IDA | 2 | 2 | - |
| `$0055` | `unk_55` | 1 | AUTO_IDA | 1 | 0 | - |
| `$0056` | `byte_56` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0057` | `UnknownDoorFlag` | 1 | TENTATIVE | 2 | 1 | - |
| `$0058` | `off_58` | 2 | AUTO_IDA | 7 | 2 | - |
| `$005A` | `byte_5A` | 1 | AUTO_IDA | 9 | 4 | - |
| `$005B` | `byte_5B` | 1 | AUTO_IDA | 0 | 1 | - |
| `$005C` | `byte_5C` | 1 | AUTO_IDA | 8 | 1 | - |
| `$005E` | `MaybeCollectedThing` | 1 | TENTATIVE | 5 | 1 | - |
| `$005F` | `byte_5F` | 1 | AUTO_IDA | 4 | 2 | - |
| `$0060` | `SoundEnginePointer` | 2 | NAMED | 18 | 17 | - |
| `$0062` | `SoundPointerTemp` | 2 | NAMED | 1 | 1 | - |
| `$0064` | `CurrentSoundChannel` | 1 | NAMED | 4 | 2 | - |
| `$0065` | `ActiveSoundChannels` | 1 | NAMED | 1 | 2 | - |
| `$0068` | `RoomMusic` | 1 | NAMED | 2 | 1 | - |
| `$006A` | `EnemyCoinTimer` | 2 | NAMED | 3 | 3 | - |
| `$0070` | `byte_70` | 1 | AUTO_IDA | 10 | 2 | - |
| `$0071` | `byte_71` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0072` | `MaybeTileCollected` | 1 | TENTATIVE | 2 | 1 | - |
| `$0073` | `byte_73` | 1 | AUTO_IDA | 0 | 1 | - |
| `$0074` | `byte_74` | 1 | AUTO_IDA | 2 | 1 | - |
| `$0078` | `off_78` | 2 | AUTO_IDA | 10 | 3 | - |
| `$007A` | `EnemyStructPointer` | 2 | NAMED | 36 | 38 | - |
| `$0088` | `CopyrightScreenTimer` | 1 | NAMED | 0 | 2 | - |
| `$0089` | `TitleScreenTimer` | 1 | NAMED | 1 | 2 | - |
| `$0090` | `byte_90` | 1 | AUTO_IDA | 6 | 1 | - |
| `$0091` | `byte_91` | 1 | AUTO_IDA | 0 | 1 | - |
| `$0092` | `byte_92` | 1 | AUTO_IDA | 4 | 3 | - |
| `$0094` | `byte_94` | 1 | AUTO_IDA | 4 | 3 | - |
| `$0098` | `byte_98` | 1 | AUTO_IDA | 2 | 2 | - |
| `$0099` | `byte_99` | 1 | AUTO_IDA | 2 | 2 | - |
| `$009A` | `byte_9A` | 1 | AUTO_IDA | 0 | 2 | - |
| `$009B` | `byte_9B` | 1 | AUTO_IDA | 0 | 2 | - |
| `$00A0` | `PlayerScore` | 1 | NAMED | 9 | 2 | - |
| `$00A4` | `StageTimer` | 1 | NAMED | 8 | 2 | - |
| `$00A5` | `TimerFrames` | 1 | NAMED | 0 | 2 | - |
| `$00A6` | `Unread_0A6` | 1 | NAMED | 0 | 1 | - |
| `$00A8` | `TimerStatusMaybe` | 1 | TENTATIVE | 5 | 7 | - |
| `$00A9` | `TimeOverTimer` | 1 | NAMED | 0 | 2 | - |
| `$00AC` | `byte_AC` | 1 | AUTO_IDA | 7 | 6 | - |
| `$00AE` | `FireBombsCollected` | 1 | NAMED | 2 | 3 | - |
| `$00AF` | `FireBombsCollected2` | 1 | NAMED | 1 | 2 | - |
| `$00B1` | `AliveEnemyCount` | 1 | NAMED | 1 | 4 | - |
| `$00B3` | `EnemySpeedModifier` | 1 | NAMED | 1 | 1 | - |
| `$00BA` | `byte_BA` | 1 | AUTO_IDA | 1 | 1 | - |
| `$00C0` | `TempPointer_0C0` | 2 | NAMED | 20 | 5 | - |
| `$00C2` | `byte_C2` | 1 | AUTO_IDA | 3 | 2 | - |
| `$00C3` | `byte_C3` | 1 | AUTO_IDA | 2 | 1 | - |
| `$00C4` | `TempY` | 1 | NAMED | 2 | 1 | - |
| `$00C5` | `byte_C5` | 1 | AUTO_IDA | 1 | 1 | - |
| `$00C7` | `byte_C7` | 1 | AUTO_IDA | 5 | 1 | - |
| `$00CA` | `byte_CA` | 1 | AUTO_IDA | 1 | 2 | - |
| `$00CB` | `byte_CB` | 1 | AUTO_IDA | 0 | 3 | - |
| `$00CC` | `byte_CC` | 1 | AUTO_IDA | 1 | 2 | - |
| `$00CD` | `byte_CD` | 1 | AUTO_IDA | 0 | 2 | - |
| `$00CF` | `byte_CF` | 1 | AUTO_IDA | 1 | 1 | - |
| `$00E0` | `NMIRunFlag` | 1 | NAMED | 5 | 1 | - |
| `$00E1` | `PPUCtrlMirror` | 1 | NAMED | 8 | 2 | - |
| `$00E2` | `PPUMaskMirror` | 1 | NAMED | 4 | 4 | - |
| `$00E6` | `Joypad1_ImmediateCopy` | 1 | NAMED | 4 | 1 | - |
| `$00E7` | `TilePlayerCollidedWith` | 1 | NAMED | 1 | 1 | - |
| `$00E9` | `CurrentRoomIDBackup2` | 1 | NAMED | 3 | 2 | - |
| `$00EA` | `CurrentRoomIDBackup3` | 1 | NAMED | 5 | 1 | - |
| `$00EC` | `PlayerX_Lo` | 1 | NAMED | 1 | 1 | - |
| `$00ED` | `PlayerY_Lo` | 1 | NAMED | 1 | 0 | - |
| `$00EE` | `byte_EE` | 1 | AUTO_IDA | 1 | 1 | - |
| `$00EF` | `RoundSelectRound` | 1 | NAMED | 2 | 0 | - |
| `$00F0` | `byte_F0` | 1 | AUTO_IDA | 24 | 3 | - |
| `$00F1` | `_MaybeBlockSpawningObj` | 1 | TENTATIVE | 1 | 3 | - |
| `$00F2` | `PPUUpdateFlag1` | 1 | NAMED | 4 | 5 | - |
| `$00F3` | `JustJumpedFlag` | 1 | NAMED | 3 | 2 | - |
| `$00F4` | `byte_F4` | 1 | AUTO_IDA | 4 | 3 | - |
| `$00F5` | `RoomPalette` | 1 | NAMED | 1 | 2 | - |
| `$00F6` | `Collected1UPThisRoundFlag` | 1 | NAMED | 3 | 2 | - |
| `$00F7` | `byte_F7` | 1 | AUTO_IDA | 4 | 2 | - |
| `$00F8` | `MaybeBombThing` | 1 | TENTATIVE | 11 | 3 | - |
| `$00F9` | `GS8_Status` | 1 | NAMED | 5 | 2 | - |
| `$00FA` | `APressCounter` | 1 | NAMED | 2 | 1 | - |
| `$00FB` | `DrawScoreAndCoinsFlag` | 1 | NAMED | 4 | 1 | - |
| `$00FC` | `PPUUpdateFlag2` | 1 | NAMED | 3 | 2 | - |
| `$00FE` | `UnknownFlag_0FE` | 1 | TENTATIVE | 5 | 2 | - |
| `$00FF` | `unk_FF` | 1 | AUTO_IDA | 19 | 0 | - |
| `$0100` | `StackArea` | 1 | NAMED | 0 | 0 | ; $0100 |
| `$0200` | `SpriteDMAArea` | 1 | NAMED | 1 | 9 | DATA XREF: InitXSprites+9w ; $0200 |
| `$0300` | `GameState1Flag` | 1 | NAMED | 1 | 2 | - |
| `$0301` | `GameState1WaitTimer` | 1 | NAMED | 0 | 1 | - |
| `$0308` | `Joypad1_Immediate` | 1 | NAMED | 8 | 2 | - |
| `$0309` | `Joypad2_Immediate` | 1 | NAMED | 1 | 0 | - |
| `$030A` | `unk_30A` | 1 | AUTO_IDA | 0 | 0 | - |
| `$030B` | `byte_30B` | 1 | AUTO_IDA | 0 | 1 | - |
| `$030C` | `byte_30C` | 1 | AUTO_IDA | 0 | 1 | - |
| `$030D` | `byte_30D` | 1 | AUTO_IDA | 0 | 1 | - |
| `$030E` | `byte_30E` | 1 | AUTO_IDA | 0 | 1 | - |
| `$030F` | `byte_30F` | 1 | AUTO_IDA | 2 | 2 | - |
| `$0310` | `byte_310` | 1 | AUTO_IDA | 1 | 2 | - |
| `$0311` | `byte_311` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0312` | `byte_312` | 1 | AUTO_IDA | 2 | 5 | - |
| `$0313` | `byte_313` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0314` | `byte_314` | 1 | AUTO_IDA | 2 | 2 | - |
| `$0315` | `CollisionTestResult` | 1 | NAMED | 1 | 2 | - |
| `$0317` | `TortureRoomSceneFlag` | 1 | NAMED | 4 | 4 | - |
| `$0318` | `TortureRoomMessageTimer` | 1 | NAMED | 1 | 2 | - |
| `$0319` | `GreedyJumpsRemaining` | 1 | NAMED | 2 | 1 | - |
| `$031B` | `StageTimerCopy` | 1 | NAMED | 2 | 1 | - |
| `$031C` | `FireBombBonusPoints` | 1 | NAMED | 2 | 1 | - |
| `$031D` | `RoundClearCountdownTimer` | 1 | NAMED | 1 | 2 | DATA XREF: GameState_8_RoundClear+2Fw ; $031d |
| `$031E` | `GameOverInitFlag` | 1 | NAMED | 1 | 2 | - |
| `$031F` | `GameOverTimer` | 1 | NAMED | 0 | 1 | - |
| `$0320` | `byte_320` | 1 | AUTO_IDA | 2 | 1 | - |
| `$0330` | `byte_330` | 1 | AUTO_IDA | 2 | 2 | - |
| `$0332` | `byte_332` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0333` | `byte_333` | 1 | AUTO_IDA | 1 | 3 | - |
| `$0334` | `byte_334` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0335` | `byte_335` | 1 | AUTO_IDA | 0 | 2 | - |
| `$0336` | `byte_336` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0337` | `MaybeDoorReturnTwiceFlag` | 1 | TENTATIVE | 2 | 1 | DATA XREF: AnimateDoorClose+7r ; $0337 |
| `$0338` | `byte_338` | 1 | AUTO_IDA | 0 | 2 | - |
| `$0339` | `byte_339` | 1 | AUTO_IDA | 1 | 2 | - |
| `$033B` | `EntryDoorType` | 1 | NAMED | 4 | 7 | - |
| `$033C` | `byte_33C` | 1 | AUTO_IDA | 1 | 2 | - |
| `$033D` | `EntryDoorTypeBackup` | 1 | NAMED | 1 | 2 | - |
| `$033E` | `byte_33E` | 1 | AUTO_IDA | 1 | 1 | - |
| `$033F` | `EntryDoorTypeBackup2` | 1 | NAMED | 1 | 1 | - |
| `$0340` | `byte_340` | 1 | AUTO_IDA | 2 | 3 | - |
| `$0341` | `byte_341` | 1 | AUTO_IDA | 1 | 2 | - |
| `$0342` | `DirectionOffsetTemp` | 1 | NAMED | 1 | 1 | - |
| `$0343` | `byte_343` | 1 | AUTO_IDA | 1 | 2 | - |
| `$0346` | `MaybeTempSpriteIndex` | 1 | TENTATIVE | 1 | 2 | - |
| `$0356` | `MaybeTempCollectableFlag2` | 1 | TENTATIVE | 1 | 5 | - |
| `$0357` | `MaybeTempCollectableFlag` | 1 | TENTATIVE | 4 | 6 | DATA XREF: GameState_0_Init+2w ; $0357 |
| `$0358` | `MaybeCollectedThing2` | 1 | TENTATIVE | 1 | 3 | - |
| `$0359` | `Show1UPSpriteTimer` | 1 | NAMED | 0 | 2 | - |
| `$035C` | `InitFlag_LoadedDifficulty` | 1 | NAMED | 1 | 2 | - |
| `$035D` | `DifficultyModifier` | 1 | NAMED | 1 | 1 | - |
| `$035E` | `EnemySpawnTimer` | 1 | NAMED | 0 | 2 | - |
| `$035F` | `EnemyTransformTimer` | 1 | NAMED | 2 | 1 | - |
| `$0364` | `byte_364` | 1 | AUTO_IDA | 4 | 5 | - |
| `$0365` | `PlayerScoreThousandsCopy` | 1 | NAMED | 1 | 1 | DATA XREF: AddScore+5Fr ; $0365 |
| `$0367` | `GoldCoinsCollected` | 1 | NAMED | 1 | 4 | - |
| `$0368` | `SphinxCollectedMaybe` | 1 | TENTATIVE | 1 | 1 | - |
| `$0369` | `MaybeHandleDoorFlag` | 1 | TENTATIVE | 1 | 3 | - |
| `$036A` | `CurrentRoomIDBackup4` | 1 | NAMED | 3 | 1 | - |
| `$036B` | `MaybeDoorTypeOrSphinx` | 1 | TENTATIVE | 3 | 1 | - |
| `$0372` | `byte_372` | 1 | AUTO_IDA | 1 | 2 | - |
| `$0373` | `byte_373` | 1 | AUTO_IDA | 1 | 3 | - |
| `$0374` | `byte_374` | 1 | AUTO_IDA | 1 | 2 | - |
| `$0375` | `byte_375` | 1 | AUTO_IDA | 1 | 2 | - |
| `$0376` | `byte_376` | 1 | AUTO_IDA | 1 | 2 | - |
| `$0379` | `byte_379` | 1 | AUTO_IDA | 0 | 3 | - |
| `$037A` | `byte_37A` | 1 | AUTO_IDA | 1 | 2 | - |
| `$037B` | `byte_37B` | 1 | AUTO_IDA | 1 | 1 | - |
| `$037C` | `byte_37C` | 1 | AUTO_IDA | 0 | 3 | - |
| `$0381` | `EndingType` | 1 | NAMED | 2 | 1 | - |
| `$0391` | `PlayerCrystalBallsCollected` | 1 | NAMED | 4 | 2 | - |
| `$0392` | `FireTilePlayerDiedTo` | 1 | NAMED | 1 | 3 | - |
| `$0393` | `DoorIsAnimatingMaybe` | 1 | TENTATIVE | 1 | 2 | - |
| `$0394` | `UnknownDoorFlagCopy` | 1 | TENTATIVE | 1 | 1 | - |
| `$0395` | `DoorCheckFlag_1E_DeathCount` | 1 | NAMED | 1 | 3 | - |
| `$0396` | `DoorCheckFlag_1E_Unk` | 1 | TENTATIVE | 1 | 3 | - |
| `$0397` | `DoorCheckFlag_19_1A` | 1 | NAMED | 2 | 3 | - |
| `$0398` | `SomeLoadFlag` | 1 | NAMED | 1 | 2 | - |
| `$0399` | `DoorCheckFlag_1F` | 1 | NAMED | 1 | 2 | - |
| `$039A` | `DoorCheckFlag_09` | 1 | NAMED | 1 | 3 | - |
| `$039B` | `PlayerGDV` | 1 | NAMED | 2 | 2 | - |
| `$039C` | `HighGDV` | 1 | NAMED | 2 | 1 | - |
| `$039D` | `ExtraLivesFound` | 1 | NAMED | 1 | 2 | - |
| `$039E` | `MaybeSphinxFlags` | 1 | TENTATIVE | 2 | 2 | - |
| `$03BE` | `RoomStatusFlags` | 1 | NAMED | 31 | 8 | - |
| `$03BF` | `CurrentRoomID` | 1 | NAMED | 10 | 7 | - |
| `$03C0` | `CheckpointRoomIDMaybe` | 1 | TENTATIVE | 1 | 3 | - |
| `$03C1` | `byte_3C1` | 1 | AUTO_IDA | 1 | 2 | - |
| `$03C2` | `byte_3C2` | 1 | AUTO_IDA | 7 | 3 | - |
| `$03C3` | `byte_3C3` | 1 | AUTO_IDA | 1 | 2 | - |
| `$03C4` | `byte_3C4` | 1 | AUTO_IDA | 6 | 5 | - |
| `$03C5` | `CurrentRoomIDBackup` | 1 | NAMED | 1 | 2 | - |
| `$03C6` | `byte_3C6` | 1 | AUTO_IDA | 2 | 1 | - |
| `$03C7` | `MaybeRoomIDCopyAgain` | 1 | TENTATIVE | 2 | 1 | - |
| `$03C8` | `byte_3C8` | 1 | AUTO_IDA | 1 | 1 | - |
| `$03C9` | `byte_3C9` | 1 | AUTO_IDA | 1 | 1 | - |
| `$03CA` | `byte_3CA` | 1 | AUTO_IDA | 1 | 1 | - |
| `$03CB` | `byte_3CB` | 1 | AUTO_IDA | 1 | 1 | - |
| `$03CC` | `LastBombRoomCleared` | 1 | NAMED | 3 | 2 | - |
| `$03CE` | `PlayerStruct` | 1 | NAMED | 15 | 7 | - |
| `$03CF` | `CollisionFlag` | 1 | NAMED | 6 | 5 | - |
| `$03D0` | `PlayerXVelLo` | 1 | NAMED | 3 | 3 | - |
| `$03D1` | `PlayerXPosLo` | 1 | NAMED | 1 | 1 | - |
| `$03D2` | `PlayerXVelHi` | 1 | NAMED | 5 | 3 | - |
| `$03D3` | `PlayerXPosHi` | 1 | NAMED | 7 | 4 | - |
| `$03D5` | `MaybeDeathTimer` | 1 | TENTATIVE | 0 | 2 | - |
| `$03D6` | `PlayerYVelLo` | 1 | NAMED | 3 | 2 | - |
| `$03D7` | `PlayerYPosLo` | 1 | NAMED | 1 | 1 | - |
| `$03D8` | `PlayerYVelHi` | 1 | NAMED | 6 | 3 | - |
| `$03D9` | `PlayerYPosHi` | 1 | NAMED | 8 | 6 | - |
| `$03DC` | `PlayerSprite` | 1 | NAMED | 3 | 5 | - |
| `$03DD` | `byte_3DD` | 1 | AUTO_IDA | 1 | 3 | - |
| `$03DF` | `byte_3DF` | 1 | AUTO_IDA | 1 | 1 | - |
| `$03E0` | `PlayerMightyLevel` | 1 | NAMED | 6 | 4 | - |
| `$03E1` | `PlayerMightyLevelPressesLeft` | 1 | NAMED | 1 | 4 | - |
| `$03E2` | `PlayerMightyCoins` | 1 | NAMED | 4 | 4 | - |
| `$03E3` | `ScoreMultiplier` | 1 | NAMED | 2 | 2 | - |
| `$03E4` | `PlayerLives` | 1 | NAMED | 1 | 3 | - |
| `$03E5` | `PlayerSecretCoins` | 1 | NAMED | 1 | 2 | - |
| `$03E6` | `byte_3E6` | 1 | AUTO_IDA | 0 | 1 | - |
| `$03E7` | `byte_3E7` | 1 | AUTO_IDA | 2 | 5 | - |
| `$03EE` | `byte_3EE` | 1 | AUTO_IDA | 1 | 3 | - |
| `$03EF` | `byte_3EF` | 1 | AUTO_IDA | 5 | 7 | - |
| `$03F0` | `byte_3F0` | 1 | AUTO_IDA | 6 | 7 | - |
| `$03F1` | `byte_3F1` | 1 | AUTO_IDA | 1 | 3 | - |
| `$03F2` | `PPUUpdateBuffer` | 1 | NAMED | 1 | 4 | - |
| `$03F3` | `unk_3F3` | 1 | AUTO_IDA | 0 | 2 | - |
| `$0432` | `SoundsQueued` | 1 | NAMED | 2 | 2 | - |
| `$0433` | `SoundQueue` | 1 | NAMED | 1 | 2 | - |
| `$0452` | `SoundChannelA_Mus0` | 1 | NAMED | 0 | 0 | - |
| `$046A` | `SoundChannelA_Mus1` | 1 | NAMED | 0 | 0 | - |
| `$0482` | `SoundChannelA_Mus2` | 1 | NAMED | 0 | 0 | - |
| `$049A` | `SoundChannelA_Mus3` | 1 | NAMED | 1 | 0 | - |
| `$04B2` | `SoundChannelA_Sfx0` | 1 | NAMED | 0 | 0 | - |
| `$04CA` | `SoundChannelA_Sfx1` | 1 | NAMED | 0 | 0 | - |
| `$04E2` | `SoundChannelA_Sfx2` | 1 | NAMED | 0 | 0 | - |
| `$04FA` | `SoundChannelA_Sfx3` | 1 | NAMED | 0 | 0 | - |
| `$0512` | `SoundChannelB_Mus0` | 1 | NAMED | 0 | 0 | - |
| `$051A` | `SoundChannelB_Mus1` | 1 | NAMED | 0 | 0 | - |
| `$0522` | `SoundChannelB_Mus2` | 1 | NAMED | 0 | 0 | - |
| `$052A` | `SoundChannelB_Mus3` | 1 | NAMED | 0 | 0 | - |
| `$0532` | `SoundChannelB_Sfx0` | 1 | NAMED | 0 | 0 | - |
| `$053A` | `SoundChannelB_Sfx1` | 1 | NAMED | 0 | 0 | - |
| `$0542` | `SoundChannelB_Sfx2` | 1 | NAMED | 0 | 0 | - |
| `$054A` | `SoundChannelB_Sfx3` | 1 | NAMED | 0 | 0 | - |
| `$0552` | `SoundRegIndex` | 1 | NAMED | 2 | 1 | - |
| `$0553` | `SoundReg_Vol` | 1 | NAMED | 2 | 4 | - |
| `$0554` | `SoundReg_Sweep` | 1 | NAMED | 1 | 1 | - |
| `$0555` | `SoundReg_Lo` | 1 | NAMED | 1 | 1 | - |
| `$0556` | `SoundReg_Hi` | 1 | NAMED | 1 | 1 | - |
| `$0563` | `AttributeTableBuffer` | 1 | NAMED | 0 | 2 | - |
| `$05A3` | `byte_5A3` | 1 | AUTO_IDA | 0 | 0 | - |
| `$05E3` | `PaletteBuffer` | 1 | NAMED | 2 | 5 | - |
| `$0603` | `RoomDataRAM` | 1 | NAMED | 9 | 15 | - |
| `$0683` | `byte_683` | 1 | AUTO_IDA | 0 | 1 | - |
| `$06A6` | `Object0Struct` | 1 | NAMED | 1 | 1 | - |
| `$06C2` | `Object1Struct` | 1 | NAMED | 0 | 0 | DATA XREF: ROM:ObjectPointerso ; $06c2 |
| `$06DE` | `Object2Struct` | 1 | NAMED | 0 | 0 | DATA XREF: ROM:ObjectPointerso ; $06de |
| `$06FA` | `Object3Struct` | 1 | NAMED | 0 | 0 | DATA XREF: ROM:ObjectPointerso ; $06fa |
| `$0716` | `Object4Struct` | 1 | NAMED | 0 | 0 | DATA XREF: ROM:ObjectPointerso ; $0716 |
| `$0732` | `Object5Struct` | 1 | NAMED | 0 | 0 | DATA XREF: ROM:ObjectPointerso ; $0732 |
| `$074E` | `Object6Struct` | 1 | NAMED | 0 | 0 | DATA XREF: ROM:ObjectPointerso ; $074e |
| `$076A` | `Object7Struct` | 1 | NAMED | 0 | 0 | DATA XREF: ROM:ObjectPointerso ; $076a |
| `$0786` | `byte_786` | 1 | AUTO_IDA | 2 | 3 | - |
| `$0787` | `byte_787` | 1 | AUTO_IDA | 1 | 1 | - |
| `$0789` | `byte_789` | 1 | AUTO_IDA | 1 | 1 | - |
| `$078B` | `byte_78B` | 1 | AUTO_IDA | 1 | 1 | - |
| `$078C` | `byte_78C` | 1 | AUTO_IDA | 0 | 2 | - |
| `$078D` | `byte_78D` | 1 | AUTO_IDA | 0 | 2 | - |
| `$07F8` | `Obj7_Timer` | 1 | NAMED | 1 | 2 | - |
