# RES-006: Especificación Funcional C x86 — Subsistema 03: Física de Jack, Salto/Aleteo (`word_C0E3`/`word_C163`), Colisiones 9-Probe (`BigBeefySubHandlesBGColliding`) y Scroll (`MainSub_3`)

- **ID de Investigación**: `RES-006`
- **Tarea Vinculada**: `TASK-RES-005` (Subpasos `0.5.1` y `0.5.2`)
- **Área Temática**: 02 - Física, Movimiento y Colisiones (Jack)
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-22
- **Agente Autor**: Antigravity (Orchestrator)

---

## 1. Objetivo y Alcance

Documentar a nivel de especificación ejecutable 1:1 para C x86 el **Subsistema 03** del desmontaje [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm) y [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm):
1. **Cinemática de Salto y Aleteo (*Flutter*)**: Acumulador de fase en coma fija `8.8` (`byte_3E7:byte_3E6`), modulación direccional por `JP_Up`/`JP_Down` (`word_C163`) y muestreo simétrico de la curva parabólica de 64 pasos (`word_C0E3`, `DoSomethingWithObjYVel` `$8ABE`, `sub_8AE8` `$8AE8`).
2. **Motor Polimórfico de Colisión de 9 Sondas (*9-Point Probe*)**: Geometría de sondas (`byte_A3D8`), máscaras de sub-bloques `8x8` (`byte_A3EA`), empaquetado de bits (`byte_A406`, `sub_9965` `$9965`, `sub_9B6A` `$9B6A`) y respuesta física de suelo (`sub_A1EB` `$A1EB`) y techo (`sub_A1B9` `$A1B9`).
3. **Colisión e Interacción con Objetos de Sala (`BigBeefySubHandlesBGColliding` `$9C36`)**: Detección de bloques ocultos (`$60`, `$80..$AF`), cofres naranjas/rojos (`JTPickup_Chest`/`Chest2`), puertas (`JTPickup_Door`) e ítems recolectables (`MaybePickupItem`).
4. **Motor de Cámara, Scroll Bidireccional y Streaming de Costuras PPU (`MainSub_3` `$A40E`)**: Acumuladores de scroll subpíxel `8.8` (`byte_3C2:byte_3C1`, `byte_3C4:byte_3C3`), vectores de compensación de entidades (`byte_98..byte_9B`, `sub_A843`, `sub_A851`) y superposición de objetos dinámicos en bordes recién desplazados (`sub_A85F`, `sub_A875`).

---

## 2. Fuentes del Desmontaje Analizadas (`asm/`)

| Archivo en `asm/` | Rango de Líneas | Dirección ROM | Símbolos / Subrutinas Cubiertas |
| :--- | :--- | :--- | :--- |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1640-L1779) | `1640 - 1779`, `2094 - 2107` | `$8AA0 - $8B68`, `$8D1E - $8D2E` | `DrawPlayer`, `DoSomethingWithObjYVel`, `sub_8AE8`, `ApplyPlayerXAndYVelocity`, `ApplyPlayerXVelocity`, `ApplyPlayerYVelocity`, `UseMightyCoin`, `UpdateMightyPalette`, `NegateYYXX` |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3859-L3955) | `3859 - 3955` | `$97B8 - $9854` | `HandleSpawnsAndMore` (`MainSub_2`), `sub_9813` |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4136-L5860) | `4136 - 5860` | `$9950 - $A3D7` | `CheckIfPlayerWithin8Pixels`, `sub_9965`, `sub_99EA`, `sub_9A03`, `sub_9A7A`, `sub_9B39`, `sub_9B6A`, `BigBeefySubHandlesBGColliding`, `MaybePickupItem`, `JTPickup_*`, `MaybeOpenHiddenBlock`, `MaybeSpawnThingFromObject`, `sub_A18E`, `sub_A1A6`, `sub_A1B9`, `sub_A1EB`, `sub_A270`, `sub_A29C`, `sub_A32B`, `sub_A33E`, `MaybeAward1UP`, `Draw1UPSprite` |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5863-L6655) | `5863 - 6655` | `$A3D8 - $A919` | `byte_A3D8`, `byte_A3EA`, `ObjectPointers`, `byte_A406`, `MainSub_3`, `sub_A843`, `sub_A851`, `sub_A85F`, `sub_A875` |
| [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L187-L257) | `187 - 257`, `306 - 311` | `$C0E3 - $C16A`, `$C202 - $C209` | `word_C0E3` (64 velocidades Y), `word_C163` (4 pasos de fase), `MightyLevelColors`, `MightyLevelAPressesTable` |

---

## 3. Especificación de Símbolos, Contrato ABI 6502 y Grafo de Llamadas

### 3.1 Tabla de Subrutinas del Subsistema 03 y Contrato ABI

| Etiqueta 6502 | Nombre C Propuesto | Dir. ROM | Línea (`prg.asm`) | Inputs (`A, X, Y, C, ZP`) | Outputs (`A, X, Y, Flags`) | Stack Mutation | Callers | Callees |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| [`DrawPlayer`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1640-L1654) | `DrawPlayer` | `$8AA0` | `1640` | `PlayerSprite`, `byte_3DD`, `PlayerXPosHi`, `PlayerYPosHi` | OAM slot 0 (`$0200-$020F`) | `RTS` | `MainSub_1`, `GameState_5_Main`, `MainSub_7`, `MainSub_9`, `MainSub_A` | `WriteSprite` |
| [`DoSomethingWithObjYVel`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1658-L1685) | `UpdateEntityParabolicYVel` | `$8ABE` | `1658` | `X`: joypad/step bits (`& $0C`), `EnemyStructPointer` (`$7A`) | Actualiza `offset $18-$19` (fase) y `offset $08/$0A` (`YVelLo/Hi`) | `RTS` | `MainSub_1`, `sub_BB1C`, `sub_BCCF` | `sub_8AE8` |
| [`sub_8AE8`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1689-L1709) | `SampleParabolicYVelTable` | `$8AE8` | `1689` | `A`: fase alta (`$00-$7F`) | `Y:X` = `YVelHi:YVelLo` con signo (`8.8`) | `RTS` | `DoSomethingWithObjYVel`, `sub_BB1C` | `NegateYYXX` |
| [`ApplyPlayerXAndYVelocity`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1713-L1715) | `ApplyPlayerXAndYVelocity` | `$8B02` | `1713` | `PlayerXVel`, `PlayerYVel` | `PlayerXPos`, `PlayerYPos` | Fallthrough | `MainSub_7`, `MainSub_9` | `ApplyPlayerYVelocity`, `ApplyPlayerXVelocity` |
| [`ApplyPlayerXVelocity`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1719-L1728) | `ApplyPlayerXVelocity` | `$8B05` | `1719` | `PlayerXVelLo/Hi`, `PlayerXPosLo/Hi` | `PlayerXPos += PlayerXVel` (`8.8`) | `RTS` | `MainSub_1`, `ApplyPlayerXAndYVelocity` | — |
| [`ApplyPlayerYVelocity`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1732-L1741) | `ApplyPlayerYVelocity` | `$8B13` | `1732` | `PlayerYVelLo/Hi`, `PlayerYPosLo/Hi` | `PlayerYPos += PlayerYVel` (`8.8`) | `RTS` | `MainSub_1`, `ApplyPlayerXAndYVelocity`, `MainSub_9` | — |
| [`UseMightyCoin`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1745-L1768) | `UseMightyCoin` | `$8B21` | `1745` | `PlayerMightyCoins`, `PlayerMightyLevel` | Incrementa `MightyLevel` (`1..3`), recarga pulsaciones `A`, en nivel 3 llama `TurnEnemiesIntoCoins` | `RTS` | `MainSub_1` | `UpdateMightyPalette`, `QueueSound`, `TurnEnemiesIntoCoins` |
| [`UpdateMightyPalette`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1774-L1779) | `UpdateMightyPalette` | `$8B60` | `1774` | `X`: `PlayerMightyLevel` (`0..3`) | `PaletteBuffer[$17] = MightyLevelColors[X]`, `UpdatePaletteFlag++` | `RTS` | `MainSub_1`, `UseMightyCoin`, `LoadRoom_0` | — |
| [`NegateYYXX`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L2094-L2107) | `NegateYYXX` | `$8D1E` | `2094` | `Y:X` = entero 16 bits | `Y:X = -(Y:X)` en complemento a 2 | `RTS` | `sub_8AE8`, `sub_A843`, `sub_A851`, `sub_BE9E`, `sub_BEDA`, `sub_BF37` | — |
| [`HandleSpawnsAndMore`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3859-L3909) | `MainSub_2_CollisionsAndSpawns` | `$97B8` | `3859` | `PPUUpdateFlag1`, `CurrentRoomID` | Ejecuta sondas de Jack (o de 3 enemigos si no hubo scroll/cambio de celda) | `RTS` | `GameState_5_Main` (`JumpTable #1`) | `HandleEnemySpawnTimer`, `sub_A29C`, `MaybeDrawEnemySprites`, `MaybeLoadRoomFlags`, `sub_99EA`, `sub_9965`, `sub_A1EB`, `sub_A1B9`, `sub_9813`, `sub_B645` |
| [`sub_9813`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L3913-L3955) | `UpdateEnemyBGCollisionsSliced` | `$9813` | `3913` | `A`: máx. enemigos a procesar (`3` o `8`) | Procesa `sub_9965` para hasta `A` enemigos activos en turno rotatorio (`byte_332`) | `RTS` | `HandleSpawnsAndMore`, `loc_B635` | `LoadObjectPointerAPlus1`, `sub_9965`, `sub_BA4E` |
| [`CheckIfPlayerWithin8Pixels`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4136-L4148) | `CheckIfPlayerWithin8Pixels` | `$9950` | `4136` | `Y`: offset coordenada (`$05`=X, `$0B`=Y) | Rota `C = (\|Player[Y] - Enemy[Y]\| >= 8)` dentro de `CollisionTestResult` (`$6E`) | `RTS` | `CheckObjectCollision`, `sub_BA00` | — |
| [`sub_9965`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4152-L4226) | `RunEntityBGCollisionProbes` | `$9965` | `4152` | `A`: índice entidad (`0`=Jack, `1..8`=Enemigo) | Empaqueta 4 bits de colisión (`[Techo\|Suelo\|Izq\|Der]`) en nibble bajo de `(off_78)[1]` | `RTS` | `HandleSpawnsAndMore`, `sub_9813` | `sub_99EA` |
| [`sub_99EA`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4230-L4247) | `RunCollisionProbePair` | `$99EA` | `4230` | `X`: índice sonda inicial (`0`, `1`, `3`, `5`, `7`) | Evalúa sonda `X` (y `X+1` si `X != 0`) según orientación de sala | `RTS` | `HandleSpawnsAndMore`, `sub_9965` | `sub_9A03`, `sub_9A7A` |
| [`sub_9A03`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4251-L4314) | `EvalHorizontalRoomProbe` | `$9A03` | `4251` | `byte_1`: índice sonda (`0..8`), `off_78` | Traduce coordenada con `byte_3C2` a sala/metatile y llama `sub_9B6A` | `RTS` | `sub_99EA` | `LoadRoomStuff`, `sub_9B39`, `HandleDrawHalfRow`, `sub_9B6A` |
| [`sub_9A7A`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4331-L4444) | `EvalVerticalRoomProbe` | `$9A7A` | `4331` | `byte_1`: índice sonda (`0..8`), `off_78` | Traduce coordenada con `byte_3C4` (módulo `240` px) a sala/metatile y llama `sub_9B6A` | `RTS` | `sub_99EA` | `LoadRoomStuff`, `sub_9B39`, `HandleDrawHalfRow`, `sub_9B6A` |
| [`sub_9B39`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4448-L4474) | `SyncPlayerCenterProbeAndCheckObjects` | `$9B39` | `4448` | `A`: `0` (horiz) o `1` (vert), `byte_1` | Si `byte_1 == 0`, guarda `PlayerX_Lo`/`PlayerY_Lo` (`$F1/$F2`) y ejecuta `BigBeefySubHandlesBGColliding` | `RTS` | `sub_9A03`, `sub_9A7A` | `BigBeefySubHandlesBGColliding` |
| [`sub_9B6A`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4480-L4611) | `TestProbeMetatileSolidity` | `$9B6A` | `4480` | `byte_1`, `byte_0`, `byte_9`, `off_4` | Testea fuego mortal (`$5C..$62` en sonda 0) y escribe bit de solidez en `byte_30F` | **`PLA;PLA;RTS` si muere en fuego** | `sub_9A03`, `sub_9A7A` | — |
| [`BigBeefySubHandlesBGColliding`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L4615-L4856) | `BigBeefySubHandlesBGColliding` | `$9C36` | `4615` | `PlayerX_Lo` (`$F1`), `PlayerY_Lo` (`$F2`) | Detecta cambio de metatile (`byte_330`), abre bloques por salto (`JustJumpedFlag`), llena `byte_320`, evalúa `byte_312` y despacha `JumpTable` de ítems | `RTS` | `sub_9B39` | `MaybeOpenHiddenBlock`, `IfNotTitleScreenRoomDoThings`, `sub_9592`, `JumpTable` |
| [`MaybeOpenHiddenBlock`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5392-L5458) | `TryOpenChestOrHiddenBlock` | `$A0D8` | `5392` | `A`: coord `YX` (`byte_F0`), `_MaybeBlockSpawningObj` (`$75`) | Abre cofre o cuenta golpes de bloque oculto (`byte_335`) y revela ítem/enemigo | `RTS` | `BigBeefySubHandlesBGColliding`, `JTPickup_Chest` | `sub_A18E`, `MaybeSpawnThingFromObject`, `sub_A1A6`, `SpawnPowerCoin`, `QueueSound` |
| [`MaybeSpawnThingFromObject`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5462-L5495) | `SpawnEntityFromChest` | `$A153` | `5462` | `A`: `EnemyType` (`>= 9`) | Spawnea entidad (`ExtraCoin`, `SecretCoin`, `Momia`, etc.) en posición del cofre | `RTS` | `MaybeOpenHiddenBlock` | `QueueSound`, `sub_BA00`, `sub_9276`, `InitEnemy` |
| [`sub_A18E`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5499-L5511) | `SetupHiddenBlockPPUCoords` | `$A18E` | `5499` | `X`: coord `YX`, `byte_311` (`$0311`) | Calcula dirección PPU (`sub_92AD`) usando el bit de nametable `byte_311` y limpia `byte_312 = 0` | `RTS` | `MaybeOpenHiddenBlock` | `sub_92AD` |
| [`sub_A1A6`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5515-L5525) | `RevealHiddenBlockMetatile` | `$A1A6` | `5515` | `A`: ID de ítem (`0..$F`), `byte_74` | Escribe `RoomDataRAM[byte_74] = A` y encola tile/atributos PPU (`ItemToTileTable[A]`) | `RTS` | `MaybeOpenHiddenBlock` | `HandleTileAttributes`, `sub_9716` |
| [`sub_A1B9`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5529-L5555) | `HandlePlayerCeilingBonk` | `$A1B9` | `5529` | `byte_314`, `RoomStatusFlags` | Forza `PlayerStruct \|= 2`, `byte_3E7 = $40` (inicio de caída) y alinea `byte_3C4 \|= 7` o `YPosHi \|= 7` | `RTS` | `HandleSpawnsAndMore` | — |
| [`sub_A1EB`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5559-L5609) | `UpdatePlayerFloorState` | `$A1EB` | `5559` | `A`: `CollisionFlag` (`$03CF`) | Si `A & 4`: aterriza Jack (`PlayerStruct &= $F9`, alinea Y a múltiplo de 8); si no: inicia caída (`byte_3E7 = $40`) | `RTS` | `HandleSpawnsAndMore` | — |
| [`sub_A270`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5647-L5667) | `InitBombPickupPopup` | `$A270` | `5647` | `byte_5A`, `byte_70` | Inicializa animación de explosión/puntos de bomba en `$0786-$078D` y cae en `sub_A29C` | Fallthrough | `JTPickup_Bomb` | `LoadPointerTo050`, `sub_9276`, `sub_A29C` |
| [`sub_A29C`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5671-L5747) | `UpdateBombPickupPopupAnd1UP` | `$A29C` | `5671` | `byte_786..byte_78D` | Parpadea mecha de bomba encendida (`OAM+$11`), actualiza popup de bomba/puntos (OAM slot 5) y `1UP` | `RTS` | `HandleSpawnsAndMore`, `sub_A270` | `MaybeAward1UP`, `ClearFourSpritesInY`, `sub_8FB1`, `sub_A32B`, `WriteSprite` |
| [`sub_A32B`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5751-L5764) | `ApplyScrollDeltaToBombPopup` | `$A32B` | `5751` | `RoomStatusFlags`, `byte_98..byte_9B` | Desplaza coordenadas `8.8` del popup (`$0788-$078B`) con el delta de cámara (`byte_98..9B`) | Fallthrough | `sub_A29C` | `sub_A33E` |
| [`sub_A33E`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5768-L5793) | `AddSigned16ScrollDeltaToPopupCoord` | `$A33E` | `5768` | `X`: offset en `$98` (`0` o `2`), `Y`: offset en `$0786` (`2` o `4`) | Suma delta `8.8` con extensión de signo a coordenada de popup; si sale de pantalla desactiva `byte_786 = 0` | `RTS` | `sub_A32B` | — |
| [`MainSub_3`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5906-L6512) | `MainSub_3_ScrollAndSeamStreamer` | `$A40E` | `5906` | `CurrentRoomID`, `RoomStatusFlags`, `PlayerXVel`, `PlayerYVel` | Actualiza scroll `8.8` (`byte_3C1..3C4`), alterna nametable, y vuelca columna (`$1D` bytes) o fila (`$40` bytes) en `PPUUpdateBuffer` | `RTS` | `GameState_5_Main` (`JumpTable #2`) | `MaybeHandleDoorOpening`, `MaybeLoadRoomFlags`, `sub_A843`, `LoadRoomStuff`, `HandleDrawHalfRow`, `LoadPointerTo050`, `HandleTileAttributes`, `LoadLayoutChunk`, `sub_A85F`, `sub_A851`, `sub_92E2` |
| [`sub_A843`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L6516-L6523) | `SetCameraScrollDeltaXNegPlayerVel` | `$A843` | `6516` | `PlayerXVelLo/Hi` | `byte_99:byte_98 = -PlayerXVel` | `RTS` | `MainSub_3` | `NegateYYXX` |
| [`sub_A851`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L6527-L6534) | `SetCameraScrollDeltaYNegPlayerVel` | `$A851` | `6527` | `PlayerYVelLo/Hi` | `byte_9B:byte_9A = -PlayerYVel` | `RTS` | `MainSub_3` | `NegateYYXX` |
| [`sub_A85F`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L6538-L6549) | `OverlayRoomObjectsOnScrolledSeam` | `$A85F` | `6538` | `MaybeRoomIDCopyAgain` (`$03CC`) | Itera objetos de la sala entrante (`sub_9592`) y parchea `PPUUpdateBuffer` con sus tiles (`sub_A875`) | Termina vía `sub_9592` (`PLA;PLA;RTS`) | `MainSub_3` | `IfNotTitleScreenRoomDoThings`, `sub_9592`, `sub_A875` |
| [`sub_A875`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L6553-L6655) | `PatchScrolledSeamWithObjectTile` | `$A875` | `6553` | `X`: `0` (tile principal) o `1` (tile secundario de puerta) | Si la coordenada del objeto coincide con la columna/fila recién decodificada en `PPUUpdateBuffer`, sobrescribe sus tiles y atributos | `RTS` | `sub_A85F` | `sub_96F5`, `HandleTileAttributes`, `LoadLayoutChunk` |

---

## 4. Cinemática de Salto y Aleteo (*Flutter*): Algoritmo Exacto y Tablas ROM

### 4.1 Tablas ROM `word_C163` y `word_C0E3` ([`data.asm:187-257`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L187-L257))

```c
// Paso de avance de fase (8.8 fixed-point) por frame según botones Up/Down:
// Índice = (Joypad1_Immediate & 0x0C) >> 2:
//   0: Neutro        -> +0x00F0 (240/256 pasos/frame)
//   1: Down ($04)    -> +0x0160 (352/256 pasos/frame -> salto corto / caída rápida)
//   2: Up   ($08)    -> +0x00A0 (160/256 pasos/frame -> súper salto / caída flotante)
//   3: Up+Down ($0C) -> +0x00D0 (208/256 pasos/frame)
static const uint16_t kJumpPhaseStepTable_C163[4] = {
    0x00F0, 0x0160, 0x00A0, 0x00D0
};

// Curva parabólica de magnitud de velocidad vertical (8.8 fixed-point, 64 entradas):
// Índice 0 = velocidad máxima ($0380 = 3.50 px/frame), Índice 63 = ápice ($0001 = 1/256 px/frame).
static const uint16_t kParabolicYVelTable_C0E3[64] = {
    0x0380, 0x037C, 0x0378, 0x0374, 0x0370, 0x0368, 0x0360, 0x0358, // 0x00..0x07
    0x0350, 0x0348, 0x0340, 0x0330, 0x0320, 0x0310, 0x0300, 0x02F0, // 0x08..0x0F
    0x02E0, 0x02D0, 0x02C0, 0x02B0, 0x02A0, 0x0290, 0x0280, 0x0270, // 0x10..0x17
    0x0260, 0x0250, 0x0240, 0x0230, 0x0220, 0x0210, 0x0200, 0x01F0, // 0x18..0x1F
    0x01E0, 0x01D0, 0x01C0, 0x01B0, 0x01A0, 0x0190, 0x0180, 0x0170, // 0x20..0x27
    0x0160, 0x0150, 0x0140, 0x0130, 0x0120, 0x0110, 0x0100, 0x00F0, // 0x28..0x2F
    0x00E0, 0x00D0, 0x00C0, 0x00B0, 0x00A0, 0x0090, 0x0080, 0x0070, // 0x30..0x37
    0x005E, 0x004C, 0x003A, 0x0028, 0x0010, 0x0008, 0x0002, 0x0001  // 0x38..0x3F
};
```

### 4.2 Implementación C 1:1 de `DoSomethingWithObjYVel` (`$8ABE`) y `sub_8AE8` (`$8AE8`)

```c
int16_t SampleParabolicYVelTable_8AE8(uint8_t phase_hi) {
    // Si phase_hi < 0x40 (ascenso): idx = phase_hi, signo negativo (hacia arriba).
    // Si phase_hi >= 0x40 (descenso): idx = (phase_hi ^ 0xFF) & 0x3F = 0x7F - phase_hi, signo positivo.
    bool is_descending = (phase_hi >= 0x40);
    uint8_t idx = is_descending ? ((phase_hi ^ 0xFF) & 0x3F) : phase_hi;
    uint16_t mag = kParabolicYVelTable_C0E3[idx];
    return is_descending ? (int16_t)mag : -(int16_t)mag;
}

void UpdateEntityParabolicYVel_8ABE(uint8_t *entity_base, uint8_t joypad_or_mode) {
    uint8_t step_idx = (joypad_or_mode & 0x0C) >> 2;
    uint16_t step = kJumpPhaseStepTable_C163[step_idx];

    uint16_t phase = (uint16_t)entity_base[0x18] | ((uint16_t)entity_base[0x19] << 8);
    phase += step;
    entity_base[0x18] = (uint8_t)(phase & 0xFF);

    uint8_t phase_hi = (uint8_t)(phase >> 8);
    if (phase_hi & 0x80) {
        phase_hi = 0x7F; // Clamp en velocidad terminal descendente (+0x0380)
    }
    entity_base[0x19] = phase_hi;

    int16_t y_vel = SampleParabolicYVelTable_8AE8(phase_hi);
    entity_base[0x08] = (uint8_t)((uint16_t)y_vel & 0xFF);        // YVelLo
    entity_base[0x0A] = (uint8_t)(((uint16_t)y_vel >> 8) & 0xFF); // YVelHi
}
```

---

## 5. Motor Polimórfico de Colisión de 9 Sondas (*9-Point Probe*)

### 5.1 Geometría de las 9 Sondas (`byte_A3D8`, [`prg.asm:5863-5872`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5863-L5872))

Cada entidad (`PlayerStruct` o `EnemyStruct`) se evalúa como un *bounding box* de $16 \times 16$ píxeles centrado en `(XPosHi, YPosHi)` mediante **1 sonda central** (Sonda `0`) y **4 pares de sondas periféricas** (Sondas `1..8`):

```c
typedef struct {
    int8_t dx;
    int8_t dy;
} ProbeOffset;

static const ProbeOffset kCollisionProbes_A3D8[9] = {
    {  0,  0 }, // Probe 0: Centro (interacción con ítems, bombas, cofres, puertas y fuego mortal $5C..$62)
    { -4, -8 }, // Probe 1: Techo Izquierda   (Par 1: Techo)
    { +4, -8 }, // Probe 2: Techo Derecha     (Par 1: Techo)
    { -4, +8 }, // Probe 3: Suelo Izquierda   (Par 2: Suelo)
    { +4, +8 }, // Probe 4: Suelo Derecha     (Par 2: Suelo)
    { -8, -4 }, // Probe 5: Pared Izq Arriba  (Par 3: Pared Izquierda)
    { -8, +4 }, // Probe 6: Pared Izq Abajo   (Par 3: Pared Izquierda)
    { +8, -4 }, // Probe 7: Pared Der Arriba  (Par 4: Pared Derecha)
    { +8, +4 }  // Probe 8: Pared Der Abajo   (Par 4: Pared Derecha)
};
```

### 5.2 Máscaras de Sub-bloques $8 \times 8$ (`byte_A3EA`, [`prg.asm:5873-5883`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L5873-L5883))

Cuando una sonda cae sobre un metatile `tile_id` (`TilePlayerCollidedWith` `$030E`):
- Si `tile_id == 0x00`, `0x2F` o `0x27`: **100% vacío** (`mask4 = 0x0`).
- Si `0x5C <= tile_id < 0x63` (baldosas de fuego/lava):
  - En la **Sonda 0** (centro de Jack) si Jack está vivo: mata a Jack (`PlayerStruct |= 0x80`, `FireTilePlayerDiedTo = tile_id`) y aborta mediante `PLA; PLA; RTS`.
  - En las **Sondas 1..8** (periféricas): es sólido (`mask4 = 0xF`).
- Si `0x38 <= tile_id < 0x4C` (20 metatiles de colisión parcial de medio bloque):
  - Se extrae el nibble correspondiente de `byte_A3EA[10]` (`idx = tile_id - 0x38`; si `idx` es par se toma el nibble alto, si `idx` es impar se toma el nibble bajo):
    ```c
    static const uint8_t kPartialMetatileMasks_A3EA[10] = {
        0xCC, // $38: 1100 (TL,TR) | $39: 1100 (TL,TR)
        0xFC, // $3A: 1111 (Full)  | $3B: 1100 (TL,TR)
        0x48, // $3C: 0100 (TR)    | $3D: 1000 (TL)
        0x5A, // $3E: 0101 (TR,BR) | $3F: 1010 (TL,BL)
        0x7B, // $40: 0111 (!TL)   | $41: 1011 (!TR)
        0xDE, // $42: 1101 (!BL)   | $43: 1110 (!BR)
        0x21, // $44: 0010 (BL)    | $45: 0001 (BR)
        0x35, // $46: 0011 (BL,BR) | $47: 0101 (TR,BR)
        0xAC, // $48: 1010 (TL,BL) | $49: 1100 (TL,TR)
        0x3C  // $4A: 0011 (BL,BR) | $4B: 1100 (TL,TR)
    };
    ```
  - Los 4 bits del nibble representan los 4 cuadrantes de $8 \times 8$ píxeles del metatile de $16 \times 16$:
    - **Bit 3 (`0x8`)**: Cuadrante Superior-Izquierdo (`(y & 8) == 0, (x & 8) == 0`)
    - **Bit 2 (`0x4`)**: Cuadrante Superior-Derecho (`(y & 8) == 0, (x & 8) != 0`)
    - **Bit 1 (`0x2`)**: Cuadrante Inferior-Izquierdo (`(y & 8) != 0, (x & 8) == 0`)
    - **Bit 0 (`0x1`)**: Cuadrante Inferior-Derecho (`(y & 8) != 0, (x & 8) != 0`)
- Para cualquier otro `tile_id`: **100% sólido** (`mask4 = 0xF`).

### 5.3 Empaquetado Final en `(off_78)[1]` (`sub_9965` `$9965`) y Respuesta Física (`sub_A1EB`, `sub_A1B9`)

Al concluir las sondas periféricas (`byte_30F` unificado con `byte_312` para Jack), `sub_9965` colapsa cada par de sondas con un `OR` lógico y escribe el resultado en los 4 bits bajos de `(off_78)[1]` (`CollisionFlag` `$03CF` para Jack):
- **Bit 3 (`0x08`)**: `(Probe1 || Probe2) || (byte_312 & 0xC0)` $\rightarrow$ **Colisión de Techo**.
  - Si se activa para Jack (`HandleSpawnsAndMore:3888`), se invoca `sub_A1B9` (`HandlePlayerCeilingBonk`): activa `PlayerStruct |= 0x02`, fija `byte_3E7 = 0x40` (rebota e inicia caída inmediata desde el ápice) y alinea `byte_3C4 |= 7` (en scroll vertical activo) o `PlayerYPosHi |= 7` (en pantalla fija/horizontal).
- **Bit 2 (`0x04`)**: `(Probe3 || Probe4) || (byte_312 & 0x30)` $\rightarrow$ **Colisión de Suelo**.
  - Evaluado en cada frame por `sub_A1EB` (`UpdatePlayerFloorState`):
    - Si `(CollisionFlag & 0x04) == 0`: limpia el bit `0x10` de `CollisionFlag`; si Jack estaba en tierra (`(PlayerStruct & 0x02) == 0`), lo pone en estado aéreo (`PlayerStruct |= 0x02`) con `byte_3E7 = 0x40` (caída inmediata al caminar fuera de una cornisa).
    - Si `(CollisionFlag & 0x04) != 0`: aterriza a Jack (`PlayerStruct &= 0xF9`), activa `CollisionFlag |= 0x10` (forzando `byte_333 = 0` en el primer frame de contacto), alinea `byte_3C4 &= 0xF8` y `PlayerYPosHi &= 0xF8` (encaje exacto a rejilla de 8 píxeles), y pone `PlayerSprite = 0`.
- **Bit 1 (`0x02`)**: `(Probe5 || Probe6) || (byte_312 & 0x0C)` $\rightarrow$ **Colisión de Pared Izquierda** (bloquea movimiento a la izquierda en `MainSub_1`).
- **Bit 0 (`0x01`)**: `(Probe7 || Probe8) || (byte_312 & 0x03)` $\rightarrow$ **Colisión de Pared Derecha** (bloquea movimiento a la derecha en `MainSub_1`).

---

## 6. Interacción con Objetos de Sala (`BigBeefySubHandlesBGColliding` `$9C36`)

1. **Disparo por Cambio de Celda (`byte_330` `$0330`)**:
   - Calcula la coordenada de metatile central de Jack `byte_A = (PlayerY_Lo & 0xF0) | (PlayerX_Lo >> 4)`, la celda adyacente vertical `byte_B` (`Y ± 8` según signo de `PlayerYVelHi`) y la celda adyacente horizontal `byte_E` (`X ± 8` según dirección `byte_EE`).
   - Sólo ejecuta el escaneo de `RoomDataRAM` si `byte_B != byte_333` o `byte_E != byte_334` (`byte_330 > 0`). Si `byte_330 == 0` (Jack no cruzó frontera de 16 píxeles en este frame), `HandleSpawnsAndMore` aprovecha el frame para evaluar las colisiones de fondo de hasta **3 enemigos activos** (`LDA #3; JSR sub_9813`) y la IA enemiga (`JSR sub_B645`)!
2. **Apertura de Cofres y Bloques Ocultos por Salto (`JustJumpedFlag` `$00EF` y `MaybeOpenHiddenBlock` `$A0D8`)**:
   - Si Jack está apoyado sobre o contra un cofre/bloque oculto, `JTPickup_Chest` guarda su coordenada en `byte_F0` (`$00F0`).
   - Si en ese instante el jugador pulsa `A` para saltar (`JustJumpedFlag != 0`), `BigBeefySubHandlesBGColliding` consume `JustJumpedFlag = 0` y llama a `MaybeOpenHiddenBlock(byte_F0)`:
     - Si el objeto es un cofre (`$80..$AF`), lo abre inmediatamente en el primer salto (`sub_A18E` + `sub_A1A6` / `MaybeSpawnThingFromObject`).
     - Si el objeto es un **bloque oculto de múltiples cabezazos (`$60..$6F`)**, su nibble bajo (`_MaybeBlockSpawningObj & 0x0F`) indica el **número de saltos necesarios** (`byte_335` `$0335`) antes de revelarse y sonar `Sound_SecretBlockRevealed`!
     - Además, si `PlayerMightyLevel >= 2` (Jack Verde o superior), `JTPickup_Chest` llama directamente a `MaybeOpenHiddenBlock` al tocar el cofre lateralmente sin necesidad de saltar (`prg.asm:5288-5292`), y `JTPickup_Chest2` (cofre naranja) permite abrirlo con `PlayerMightyLevel >= 1` (Jack Azul).

---

## 7. Motor de Scroll y Streaming de Costuras (`MainSub_3` `$A40E`)

- **Salas de Pantalla Única (`RF_SingleScreen = $20`)**: Fuerza `byte_3C2 = 0`, `byte_3C4 = 0`, muestra los 8 píxeles izquierdos (`PPUMaskMirror |= 0x06`) y retorna.
- **Salas de Scroll Horizontal (`RF_ScrollStop = $40`, `RF_SingleScreen = 0`)**:
  - Oculta la columna izquierda de 8 píxeles (`PPUMaskMirror &= ~0x06`).
  - Mientras `RF_AtScrollEdge ($08)` está limpio, la posición en pantalla de Jack se mantiene fija en `$80` y `PlayerXVel` se suma al acumulador de cámara `byte_3C2:byte_3C1`, mientras que `byte_99:byte_98 = -PlayerXVel` (`sub_A843`) desplaza en sentido opuesto a todos los enemigos y popups.
  - Cada vez que `(byte_3C2 [+ 8 si XVel < 0]) >> 3` cambia respecto a `byte_3CA`, decodifica una columna vertical de 15 metatiles (`$1D` bytes, `byte_3EE = 4` -> incremento PPU `+32` vertical) en `PPUUpdateBuffer` y superpone los objetos dinámicos de la sala (`sub_A85F`).
- **Salas de Scroll Vertical (`RF_ScrollStop = 0`, `RF_SingleScreen = 0`)**:
  - Acumula `PlayerYVel` en `byte_3C4:byte_3C3` con aritmética módulo `240` (`$F0`, altura de una nametable NES) y conmuta el bit vertical de nametable (`PPUCtrlMirror ^= 0x02`) al envolver `0x00 <-> 0xEF`.
  - Fija `byte_9B:byte_9A = -PlayerYVel` (`sub_A851`) y, al cambiar de fila de 8 píxeles (`byte_3CA`/`byte_3CB`), decodifica una tira horizontal completa de 16 metatiles (`$40` = 64 bytes = 2 filas de 32 tiles PPU, `byte_3EE = 0` -> incremento PPU `+1` horizontal) en `PPUUpdateBuffer` y superpone objetos (`sub_A85F`).

---

## 8. Plan de Pruebas Unitarias y Verificación (Paridad C vs 6502)

1. **`test_jack_parabolic_jump_and_flutter`**:
   - Verificar que un salto neutro desde el suelo (`byte_3E7=0, byte_3E6=0`, `step=$00F0`) alcanza el ápice (`byte_3E7 >= $40`) exactamente en el frame `ceil(0x4000 / 0x00F0) = 69`, mientras que manteniendo `JP_Up` (`step=$00A0`) alcanza el ápice en el frame `ceil(0x4000 / 0x00A0) = 103`.
2. **`test_9probe_collision_and_partial_metatiles`**:
   - Verificar la extracción de los 4 cuadrantes `8x8` para los 20 metatiles parciales (`$38..$4B`) contra `byte_A3EA`, el aterrizaje alineado a 8 píxeles en `sub_A1EB` y el rebote de techo (`byte_3E7 = $40`) en `sub_A1B9`.
3. **`test_load_balancing_player_vs_enemies`**:
   - Confirmar que cuando `byte_330 != 0` se ejecutan las sondas de Jack (`sub_9965(0)`), y cuando `byte_330 == 0` se ejecutan hasta 3 enemigos por frame (`sub_9813(3)`) más la IA enemiga (`sub_B645`).
