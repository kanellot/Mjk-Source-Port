# Layout de ROM y Mapa de Memoria (`REV_US`)

Distribución física de la ROM `mbj-us.nes` (Mapper 3 CNROM) y mapa de memoria del bus de 16 bits y de la RAM interna (`$0000–$07FF`).

**Fuentes**: `asm/mighty-bomb-jack.asm`, `asm/src/ram.asm`, `asm/src/defs.asm`, `asm/src/registers.asm`.

---

## 1. Cabecera iNES (`mbj-us.nes`, `65.552` bytes)

| Campo iNES | Valor | Interpretación | Referencia |
| :--- | :---: | :--- | :--- |
| **Magic** | `4E 45 53 1A` | Cabecera iNES estándar (16 bytes) | `mighty-bomb-jack.asm:47` |
| **PRG-ROM** | `2` (`32 KB`) | 2 bancos de 16 KB fijos en `$8000–$FFFF` | `INESPRG 2` |
| **CHR-ROM** | `4` (`32 KB`) | 4 bancos de 8 KB conmutables vía CNROM | `INESCHR 4` |
| **Mapper** | `3` (**CNROM**) | Sin PRG-RAM externa ni IRQ por hardware | `NES_MAPPER_NUM = 3` |
| **Mirroring** | `0` (**Horizontal**) | Nametables `$2000=$2400` y `$2800=$2C00` | `INESMIR 0` |
| **SHA-1 (`REV_US`)** | `a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7` | Verificado bit a bit | — |

---

## 2. Espacio de Direccionamiento CPU 6502 (`$0000–$FFFF`)

```text
+------------------+ $FFFF
| Vectores 6502    | $FFFA-$FFFF  (NMI: $8081, RESET: $8000, IRQ: $8081)
+------------------+ $FFFA
| Padding ROM      | $FFF0-$FFF9  (Relleno $FF)
+------------------+ $FFF0
| Bytecode Música  | $F900-$FFEF  (music-data.asm)
+------------------+ $F900
| Motor de Audio   | $F39E-$F8FF  (music-engine.asm)
+------------------+ $F39E
| SFX y Punteros   | $EC00-$F39D  (music-data.asm)
+------------------+ $EC00
| Gráficos / Salas | $C201-$EBFF  (Metatiles, textos, 256 salas RoomData)
+------------------+ $C201
| Tablas Físicas   | $C0E3-$C200  (word_C0E3 salto 64 pasos, word_C163 tasas)
+------------------+ $C0E3
| Código Principal | $8000-$C0E2  (prg.asm: Boot, NMI, FSM, física, IA, HUD)
+------------------+ $8000  (Inicio PRG-ROM, 32 KB)
| Sin uso (CNROM)  | $4020-$7FFF
+------------------+ $4020
| Registros APU/IO | $4000-$4017  (Audio 2A03, OAM DMA $4014, Joypads $4016-$4017)
+------------------+ $4000
| Registros PPU    | $2000-$2007  (Espejados hasta $3FFF)
+------------------+ $2000
| RAM Espejada     | $0800-$1FFF  (Reflejo de $0000-$07FF)
+------------------+ $0800
| RAM Interna 2 KB | $0000-$07FF
+------------------+ $0000
```

- **Bucle principal 100 % dirigido por NMI**: tras inicializar hardware y limpiar RAM en `RESET`, la CPU entra en `LoopForever: JMP LoopForever`; toda la lógica de juego, OAM DMA (`$4014`), actualizaciones PPU, lectura de mandos y audio (`HandleSound`) corren dentro de `NMI` a `60.0988 Hz`.

---

## 3. Bancos de CHR-ROM (`32 KB` = 4 × 8 KB)

| Banco CNROM | Rango PPU | Contenido Principal |
| :---: | :---: | :--- |
| **Banco 0** | `$0000–$1FFF` | Sprites (Jack, enemigos, ítems) y tipografía HUD común. |
| **Banco 1** | `$0000–$1FFF` | Tiles de fondo de pirámide, templos y salas jugables. |
| **Banco 2** | `$0000–$1FFF` | Salas reales / tesoro (palacios). |
| **Banco 3** | `$0000–$1FFF` | Exteriores y secuencias finales. |

---

## 4. Mapa de RAM Interna (`$0000–$07FF`)

| Rango RAM | Tamaño | Zona y Uso Principal |
| :---: | :---: | :--- |
| `$0000–$001F` | 32 B | **Zero-Page Scratchpad**: punteros indirectos y temporales sin persistencia entre frames. |
| `$0020–$00FF` | 224 B | **Variables Críticas ZP**: `InGameFlag` (`$43`), `GameState` (`$44`), `TitleScreenState` (`$45`), `PlayerScore` (`$A0..$A3`), `StageTimer` (`$A4`), `TimerFrames` (`$A5`), `APressCounter` (`$FA`), punteros de audio (`$60..$6B`). |
| `$0100–$01FF` | 256 B | **Pila Hardware 6502**: inicializada a `$FF` en `RESET`. |
| `$0200–$02FF` | 256 B | **Buffer OAM (`SpriteDMAArea`)**: 64 sprites × 4 bytes volcados vía DMA `$4014`. |
| `$0300–$037F` | 128 B | **Entrada, Timers y Eventos**: `Joypad1_Immediate` (`$308`), `GreedyJumpsRemaining` (`$319`), `EntryDoorType` (`$33B`), `EnemySpawnTimer` (`$35E`), `GoldCoinsCollected` (`$367`). |
| `$0380–$03FF` | 128 B | **Estado de Juego, Jugador y Cola PPU**: `PlayerCrystalBallsCollected` (`$391`), `PlayerGDV`/`HighGDV` (`$39B/$39C`), `RoomStatusFlags` (`$3BE`), `CurrentRoomID` (`$3BF`), scroll (`$3C1..$3C4`), `PlayerStruct` (`$3CE..$3DF`), Mighty/Vidas (`$3E0..$3E7`), `PPUUpdateBuffer` (`$3F2+`). |
| `$0432–$0556` | 293 B | **Motor de Audio**: `SoundsQueued` (`$432`), `SoundQueue` (`$433..$451`), 8 canales virtuales (`$452..$511`), stacks de subrutinas (`$512..$551`) y registros espejo APU (`$552..$556`). |
| `$0563–$0602` | 160 B | **Buffers PPU**: `AttributeTableBuffer` (`$563..$5E2`) y `PaletteBuffer` (`$5E3..$602`, 32 bytes). |
| `$0603–$0692` | 144 B | **Persistencia de Salas**: `RoomDataRAM` (`$603..$682`, 8 ranuras × 16 B) + ranura scratch (`$683..$692`). |
| `$06A6–$07FF` | 346 B | **Entidades y Efectos**: `Object0Struct..Object7Struct` (8 ranuras × 28 bytes = `$1C`), `BombPuffSlots` (`$786..$78D`). |

Para el listado completo de las 337 variables RAM y sus lectores/escritores, consulta [`inventories/variable_usage.csv`](inventories/variable_usage.csv).
