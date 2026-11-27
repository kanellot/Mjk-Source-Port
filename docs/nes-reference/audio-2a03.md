# Audio 2A03 y Motor Musical de 8 Canales (`REV_US`)

Especificación del driver de audio 6502 (`HandleSound`), la cola de comandos, la máquina virtual de bytecode y la sincronización temporal con las pantallas del juego.

**Fuentes**: `asm/src/music-engine.asm`, `asm/src/music-data.asm`, `asm/src/prg.asm` (`65-68`, `112`, `609-621`, `2198-2202`), `asm/src/defs.asm` (`84-121`).

---

## 1. Inicialización Hardware APU y Timing de Arranque

- En `RESET` (`prg.asm:65-68`): escribe `$00` en `$4011` (`DMC_RAW`) y `$0F` en `$4015` (`SND_CHN`), habilitando **Pulse 1, Pulse 2, Triangle y Noise** y deshabilitando DMC. El registro `$4017` nunca se escribe (las IRQs quedan inhibidas con `SEI`).
- **Ciclo NMI**: `HandleSound` (`music-engine.asm:2-13`) se ejecuta una vez por frame (60.0988 Hz) al final de la NMI.
- **Silencio en arranque**:
  - `Copyright Screen`, `Title Screen`, `Round Intro`, `LoadRoom` y `DrawScreen` operan en **silencio total** (`GameState_0_Init` encola `Sound_Silence = $00`).
  - La música de la sala arranca al terminar `GameState_4_EnterRoom` (`prg.asm:619-621`): `QueueSound(MusicOptionsTable[RoomMusic])` (en Sala 1 encola `$16`, `Music_MainWithIntro`).

---

## 2. Arquitectura de 8 Canales Virtuales y Prioridad SFX → BGM

| Canales Virtuales | Tipo | Hardware APU | RAM Estado (24 B/canal) | Stack Subrutinas (8 B/canal) |
| :---: | :---: | :--- | :---: | :---: |
| `0..3` (`Mus0..3`) | **BGM** | Pulse 1, Pulse 2, Triangle, Noise | `$0452–$04B1` | `$0512–$0531` |
| `4..7` (`Sfx0..3`) | **SFX** | Pulse 1, Pulse 2, Triangle, Noise | `$04B2–$0511` | `$0532–$0551` |

- **Multiplexado (`CheckIfSoundChannelIsActive`, `music-engine.asm:751-771`)**: si un canal SFX (`ch + 4`) está activo, inhibe la escritura de registros APU de su canal BGM hermano (`ch`) sin detener el avance temporal de la música.
- **Restauración**: al terminar el SFX (`$F6/$F7`), el canal BGM hermano restaura inmediatamente sus registros de hardware.
- **Cola FIFO**: `QueueSound` (`prg.asm:2198-2202`) encola IDs en `SoundQueue` (`$0433..$0451`) usando el contador `SoundsQueued` (`$0432`).

---

## 3. Catálogo de Comandos de Sonido (`$00..$23`)

| ID | Símbolo (`defs.asm`) | Canales | Función |
| :---: | :--- | :---: | :--- |
| `$00` | `Sound_Silence` | `0..7` | Apaga los 8 canales. |
| `$01` | `Sound_Jump` | `5` | Salto de Jack. |
| `$02` | `Sound_Death` | `0,1,2,4,5,6` | Muerte de Jack / Time Over. |
| `$03` / `$04` | `Sound_BonusCoinSpawn` / `Sound_CoinCollected` | `5` | Aparición y recogida de moneda. |
| `$05` / `$06` | `Sound_PowerCoinSpawn` / `Sound_PowerCoinMusic` | `5` / `0,1,2` | Power Coin y música de transmutación. |
| `$07` / `$08` | `Sound_BombCollectedUnlit` / `Lit` | `6` | Bomba apagada (100) / encendida (200). |
| `$09` | `Sound_YouAreGreedy` | `0,1,2` | Fanfarria de codicia (Torture Room). |
| `$0A` / `$0B` | `Sound_1UP_Captive` / `Sound_CrystalBall` | `4,5` | 1UP / cautivo / Bola de Cristal. |
| `$0D` / `$0E` | `Sound_BonusStatueCollected` / `SecretBlockRevealed` | `4,5` | Estatua / bloque secreto revelado. |
| `$0F` | `Sound_Door` | `7` | Apertura/cierre de puerta (ruido). |
| `$10`..`$13` | `Music_Main`, `TreasureRoom`, `SideRoom`, `Labyrinth` | `0,1,2` | Temas BGM principales. |
| `$15` / `$1E` | `Music_Outside1` / `Music_Outside2` | `0,1,2` | BGM exterior (alias). |
| `$16` | `Music_MainWithIntro` | `0,1,2` | Tema principal con fanfarria inicial. |
| `$17` / `$18` | `Music_GameOver` / `Music_RoundClear` | `0,1,2` | Fanfarrias de fin de partida y ronda. |
| `$19` / `$1A` | `Sound_BombChestBonus` / `Sound_1A` | `4,5` | Bonus de cofre / transición especial. |
| `$1B` / `$1C` | `Sound_Pause` / `Sound_Unpause` | `4,5,6` | Silencio y reanudación de pausa. |
| `$1D` | `Sound_UsedMightyCoin` | `4,5,6` | Activación de Mighty Power. |
| `$1F` | `Music_TortureRoom` | `0,1,2` | Tema de la Sala de Tortura. |
| `$20` / `$21` | `Sound_BombBonus` / `Sound_TimePickup` | `4,5` / `5` | Bonus 10k/50k / Mighty Drink. |
| `$22` / `$23` | `Music_Ending` / `Music_CollectedFullFamily` | `0,1,2` | Temas de finales estándar y mejor final. |

*(Los IDs `$0C` y `$14` apuntan a `SoundData_Null`.)*

---

## 4. Tablas Musicales

### Períodos Base de 11 bits (`NoteFrequencyTable`, `music-engine.asm:776-792`)
`semitone = byte & 0x0F`, `octave = byte >> 4`. El período se divide entre $2^{\text{octave}}$ mediante desplazamientos `LSR/ROR`:

| Semitono | `0` C | `1` C# | `2` D | `3` D# | `4` E | `5` F | `6` F# | `7` G | `8` G# | `9` A | `A` A# | `B` B | `C` Rest |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Período** | `$591` | `$541` | `$4F5` | `$4AE` | `$46B` | `$42B` | `$3EF` | `$3B7` | `$381` | `$34F` | `$31F` | `$2F2` | `$000` |

### Duraciones (`NoteLengthTable`, `music-data.asm:5-37`, en frames NTSC)
`{ 0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 80, 7, 14, 28, 5, 10, 60, 6, 9, 12, 18, 24, 36, 48, 72, 96, 144 }`

### Envolventes (`SoundVolTable`, `music-data.asm:38-125`)
8 tablas de pares `(vol_flags, frames)` terminadas en `$FF` (hold):
- **0**: decay lineal `15→0` (1f/paso).
- **1**: attack `13→15` + hold.
- **2**: decay rápido de 2 en 2.
- **3**: crescendo lento + hold `48f` + decrescendo.
- **4**: volumen máximo constante (`$1F, $FF`).
- **5**: staccato percusivo de 2 frames (`$1F, 2, $10, $FF`).
- **6**: sostenido pleno Triangle/Noise (`$7F, $FF`).
- **7**: decay progresivo lento.

### Pasos de Pitch Bend (`byte_B52C`, `music-engine.asm:774-775`)
`{ +1, +2, -1, -2 }`.

---

## 5. Gramática del Bytecode (`HandleSoundChannel`)

- **`$00..$7F` (Nota / Rest)**: `[Octave:3][Semitone:4]`. Emite nota (o silencio si semitono = `$C`), aplica pitch bend si está activo y espera `duration_reload` frames.
- **`$80..$9F`**: fija duración `NoteLengthTable[byte & 0x1F]`.
- **`$A0..$BF`**: selecciona envolvente `SoundVolTable[byte & 0x1F]`.
- **`$C0..$CF`**: carga frecuencia directa `freq_lo = byte & 0x0F`, `freq_hi = $08`.
- **`$D0..$D7`**: fija *duty cycle* (`0`=12.5%, `1`=25%, `2`=50%, `3`=75%).
- **`$D8..$DF`**: fija paso de pitch bend `byte_B52C[byte & 3]`.
- **`$E0..$EF`**: fija atenuación de volumen (`0..15`).
- **`$F0 <addr16>`**: salto incondicional.
- **`$F1 <count>` / `$F2`**: inicio y fin de bucle (`count` repeticiones).
- **`$F3 <addr16>` / `$F4`**: llamada y retorno de subrutina musical (pila de hasta 4 niveles en `SoundChannelB`).
- **`$F5 <val>`**: configura registro *hardware sweep* (`+0x10`, se reinicia a `$08` en cada nota).
- **`$F6` / `$F7`**: fin de pista (`status = 0`; si es SFX, restaura canal BGM hermano).
