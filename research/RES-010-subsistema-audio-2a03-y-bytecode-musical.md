# RES-010: Especificación Funcional C x86 — Subsistema 07: Motor de Audio 2A03 y Bytecode Musical

- **ID de Investigación**: `RES-010`
- **Tarea Vinculada**: `TASK-RES-009` (Subpasos `0.9.1` y `0.9.2`)
- **Área Temática**: 06 - Motor de Audio, Secuencias APU y SFX
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-22
- **Agente Autor**: Antigravity (Orchestrator)

---

## 1. Objetivo y Alcance

Documentar con total rigor de ingeniería inversa y especificación ejecutable para C x86 el **Subsistema 07** correspondiente al motor de sonido original de *Mighty Bomb Jack* en [`asm/src/music-engine.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm) y las partituras/efectos en [`asm/src/music-data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-data.asm):
1. **Arquitectura de 8 Canales Virtuales y Prioridad SFX sobre BGM**:
   - 4 Canales BGM (Pulse 1, Pulse 2, Triangle, Noise en `$0452..$04B1`).
   - 4 Canales SFX (Pulse 1, Pulse 2, Triangle, Noise en `$04B2..$0511`).
   - Mecanismo de multiplexado y restauración automática de BGM al finalizar un efecto de sonido (`SoundCommand_F6_F7`).
2. **Máquina Virtual de Bytecode Musical e Intérprete de Opcodes**:
   - Rango de notas musicales `$00..$7F` (nota de 4 bits + octava por desplazamientos `LSR`/`ROR`).
   - Opcodes de control `$80..$FF` (duración, envolvente de volumen, silencios, duty cycle, vibrato, bucles anidados, subrutinas de llamada/retorno y barridos sweep).
3. **Catálogo de 35 Pistas y Efectos de Sonido (`SoundPointers`)**:
   - Mapeo completo de identificadores (`Sound_*`, `Music_*`), canales asignados y punteros de datos.
4. **Mapeo a Registros de Hardware APU 2A03 de la NES**:
   - Puertos `$4000-$4015`, espejos de registros en RAM (`$07B0-$07EB`) y ciclo de actualización a 60 Hz (`HandleSound`).

---

## 2. Fuentes del Desmontaje Analizadas (`asm/`)

| Archivo en `asm/` | Rango de Líneas | Dirección ROM | Componentes Cubiertos |
| :--- | :--- | :--- | :--- |
| [`asm/src/music-engine.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L1-L810) | `1 - 810` | `$B100 - $B592` | `HandleSound`, `HandleSoundQueue`, `HandleSoundChannel`, `sub_B285`, `SoundCommand_F6_F7_sub1`, `HandleSoundCommand`, `SoundCommand_8x_9x..Fx`, `MaybeUpdateVolDelay`, `MaybeResetVolDelay`, `WriteSoundRegisters`, `CheckIfSoundChannelIsActive`, `NoteFrequencyTable`, `SoundPointersA`, `SoundPointersB` |
| [`asm/src/music-data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-data.asm#L1-L200) | `1 - 200` | `$F251 - $F420` | `NoteLengthTable` (32 duraciones), `SoundVolTable` (8 envolventes de volumen), `SoundPointers` (35 pistas y efectos) |
| [`asm/src/ram.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/ram.asm#L110-L160) | `110 - 160` | `$0452 - $0511`, `$07B0 - $07EB` | Estructuras de 24 bytes por canal (`SoundChannelA_Mus0..3`, `SoundChannelA_Sfx0..3`), cola `SoundQueue` (`$0440`) y espejos APU |

---

## 3. Catálogo de Subrutinas y ABI del Motor de Sonido

| Etiqueta 6502 | Nombre C Propuesto | Dir. ROM | Línea (`music-engine.asm`) | Inputs / Registros | Función / Contrato |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `HandleSound` | `UpdateAudioEngineTick` | `$B100` | `2` | `SoundsQueued`, `SoundQueue` | Procesa encolados y avanza los 8 canales virtuales, volcando a registros APU (`$4000-$4015`). |
| `HandleSoundQueue` | `ProcessQueuedAudioCommands` | `$B145` | `47` | `SoundQueue[byte_0]` | Si es 0 silencia los 8 canales; si > 0 instancia los canales del sonido desde `SoundPointers`. |
| `HandleSoundChannel` | `ExecuteChannelBytecodeTick` | `$B1D5` | `135` | `CurrentSoundChannel`, `SoundEnginePointer` | Decrementa temporizador de nota; si expira, lee y ejecuta el flujo de notas y comandos de bytecode. |
| `sub_B285` | `CheckChannelMutedBySFX` | `$B285` | `266` | `CurrentSoundChannel` | Comprueba si el canal BGM actual está silenciado por un SFX activo. Si no, actualiza registros. |
| `SoundCommand_F6_F7_sub1` | `FlushChannelHardwareRegisters` | `$B28E` | `273` | `SoundRegIndex`, `SoundEnginePointer` | Copia volumen, sweep y frecuencia de 16 bits del canal a los registros espejo APU. |
| `HandleSoundCommand` | `DispatchBytecodeCommand` | `$B2A9` | `294` | `A`: opcode (`$80..$FF`), `byte_6` | Despacha el comando según sus 3 bits superiores mediante `JumpTable` de 8 entradas. |
| `SoundCommand_8x_9x` | `CmdSetNoteDuration` | `$B2C6` | `317` | `byte_6 & 0x1F` | Asigna la duración de nota desde `NoteLengthTable` al canal activo (`offset $01` y `$02`). |
| `SoundCommand_Ax_Bx` | `CmdSetVolumeEnvelope` | `$B2D6` | `333` | `byte_6 & 0x1F` | Asigna el puntero de envolvente de volumen desde `SoundVolTable` (`offset $05/$06`). |
| `SoundCommand_Cx` | `CmdSetRestOrSilence` | `$B2EF` | `355` | `byte_6 & 0x0F` | Fija frecuencia silenciosa/apagada y fuerza retorno doble para procesar siguiente comando. |
| `SoundCommand_Dx` | `CmdSetDutyOrVibrato` | `$B307` | `371` | `byte_6 & 0x0F` | Si `< 8`: configura duty cycle (12.5%, 25%, 50%, 75%); si $\ge 8$: configura paso de vibrato/pitch bend. |
| `SoundCommand_Ex` | `CmdSetVolumeAttenuation` | `$B334` | `410` | `byte_6 & 0x0F` | Configura atenuación o transposición de volumen en `offset $0D`. |
| `SoundCommand_Fx` | `DispatchSubCommand` | `$B33E` | `422` | `byte_6 & 0x07` | Despacha sub-comandos `F0..F7` (saltos, bucles, llamadas a subrutina y finalización). |
| `SoundCommand_F0` | `CmdJumpAddress` | `$B34E` | `441` | `(off_2)`: dirección 16-bit | Salto incondicional en el flujo de bytecode a la dirección especificada. |
| `SoundCommand_F1` | `CmdLoopStart` | `$B36F` | `465` | `(off_2)`: contador de repeticiones | Inicia bloque de bucle: guarda contador en `offset $13` y puntero de retorno en `offset $09/$0A`. |
| `SoundCommand_F2` | `CmdLoopEnd` | `$B38E` | `487` | `offset $13` (contador) | Decrementa repeticiones; si $> 0$, salta al inicio del bucle guardado en `offset $09/$0A`. |
| `SoundCommand_F3` | `CmdCallSubroutine` | `$B3AE` | `515` | `(off_2)`: dirección 16-bit | Apila el puntero de retorno en el stack interno del canal (`offset $0B`) y salta a la subrutina. |
| `SoundCommand_F4` | `CmdReturnSubroutine` | `$B36C` | `563` | `offset $0B` (stack de canal) | Desapila la dirección de retorno del stack interno y continúa la ejecución del bytecode. |
| `SoundCommand_F5` | `CmdSetSweepRegister` | `$B396` | `594` | `(off_2)`: valor sweep | Carga el valor en el registro sweep de hardware (`offset $10`). |
| `SoundCommand_F6_F7` | `CmdEndTrack` | `$B3A5` | `605` | `CurrentSoundChannel` | Apaga el canal (`status = 0`). Si era canal SFX (4..7), restaura el canal BGM correspondiente (0..3). |
| `MaybeUpdateVolDelay` | `UpdateVolumeEnvelopeTick` | `$B460` | `634` | `offset $0E` (delay volumen) | Decrementa el temporizador de la fase actual de la envolvente de volumen. |
| `MaybeResetVolDelay` | `AdvanceVolumeEnvelopePhase` | `$B46B` | `646` | `offset $05/$06` (puntero envolvente) | Lee el siguiente paso de volumen y duración de la envolvente y actualiza `SoundReg_Vol`. |
| `WriteSoundRegisters` | `FlushAPUHardwareRegisters` | `$B4D0` | `712` | `ActiveSoundChannels`, `SoundReg_*` | Escribe máscara a `$4015` y vuelca registros modificados a los puertos APU `$4000-$400F`. |
| `CheckIfSoundChannelIsActive` | `IsSFXChannelOverridingBGM` | `$B50E` | `751` | `CurrentSoundChannel` | Verifica si el canal SFX hermano está encendido para inhibir la escritura de BGM. |

---

## 4. Tabla de Frecuencias de Notas y Opcodes del Bytecode

### 4.1 Tabla de Frecuencias Base (`NoteFrequencyTable`, [`music-engine.asm:776-792`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L776-L792))

Frecuencias de 11 bits para las 12 notas de la escala cromática en la octava más grave:

```c
static const uint16_t kNoteFrequencyTable[12] = {
    0x0591, // 0: C  (Do)
    0x0541, // 1: C# (Do#)
    0x04F5, // 2: D  (Re)
    0x04AE, // 3: D# (Re#)
    0x046B, // 4: E  (Mi)
    0x042B, // 5: F  (Fa)
    0x03EF, // 6: F# (Fa#)
    0x03B7, // 7: G  (Sol)
    0x0381, // 8: G# (Sol#)
    0x034F, // 9: A  (La)
    0x031F, // A: A# (La#)
    0x02F2  // B: B  (Si)
};
```

Para una nota en el flujo de bytecode con byte `B`:
- `nota = B & 0x0F` (índice 0..11).
- `octava = B >> 4` (desplazamiento a la derecha: `freq = kNoteFrequencyTable[nota] >> octava`).

### 4.2 Matriz de Opcodes del Bytecode Musical (`$80..$FF`)

| Rango de Opcode | Comando | Argumentos | Efecto Técnico en el Canal |
| :---: | :--- | :---: | :--- |
| **`0x00 .. 0x7F`** | `PLAY_NOTE` | — | Emite la nota `(B & 0x0F)` con octava `(B >> 4)` en el canal activo. |
| **`0x80 .. 0x9F`** | `SET_DURATION` | `idx = B & 0x1F` | Fija la duración de las notas siguientes desde `NoteLengthTable[idx]` (en frames a 60 Hz). |
| **`0xA0 .. 0xBF`** | `SET_ENVELOPE` | `idx = B & 0x1F` | Asigna el puntero de envolvente de volumen desde `SoundVolTable[idx]`. |
| **`0xC0 .. 0xCF`** | `REST` | — | Silencia el canal durante el tiempo actual de nota. |
| **`0xD0 .. 0xD7`** | `SET_DUTY` | `duty = B & 0x07` | Fija el duty cycle del canal Pulse: `00=12.5%`, `01=25%`, `10=50%`, `11=75%`. |
| **`0xD8 .. 0xDF`** | `SET_VIBRATO` | `mode = B & 0x03` | Configura el modo de vibrato / modulación de tono desde `byte_B52C[mode]`. |
| **`0xE0 .. 0xEF`** | `SET_ATTENUATION` | `vol = B & 0x0F` | Fija nivel de atenuación o transposición de volumen en `offset $0D`. |
| **`0xF0`** | `JUMP` | `uint16_t addr` | Salto incondicional a la dirección de memoria especificada. |
| **`0xF1`** | `LOOP_START` | `uint8_t count` | Inicia bucle con `count` iteraciones; guarda dirección de retorno. |
| **`0xF2`** | `LOOP_END` | — | Decrementa iteraciones y salta al inicio mientras `count > 0`. |
| **`0xF3`** | `CALL` | `uint16_t addr` | Llama a subrutina musical apilando dirección en stack del canal. |
| **`0xF4`** | `RET` | — | Retorna de subrutina musical desapilando dirección. |
| **`0xF5`** | `SET_SWEEP` | `uint8_t sweep` | Carga el valor en el registro de hardware sweep de los canales Pulse. |
| **`0xF6, 0xF7`** | `END_TRACK` | — | Finaliza la reproducción del canal y restaura el canal BGM si correspondía a un SFX. |

---

## 5. Catálogo de 35 Pistas y Efectos de Sonido (`SoundPointers`)

| ID | Nombre Simbólico | Tipo | Canales Utilizados | Descripción en Juego |
| :---: | :--- | :---: | :---: | :--- |
| `0x01` | `Sound_Jump` | SFX | Canal 5 (Pulse 2) | Salto estándar de Jack. |
| `0x02` | `Sound_Death` | SFX | Canales 0, 1, 2, 4, 5, 6 | Sonido y secuencia de muerte de Jack. |
| `0x03` | `Sound_BonusCoinSpawn` | SFX | Canal 5 (Pulse 2) | Aparición de moneda bonus o extra. |
| `0x04` | `Sound_CoinCollected` | SFX | Canal 5 (Pulse 2) | Recolección de moneda dorada. |
| `0x05` | `Sound_PowerCoinSpawn` | SFX | Canal 5 (Pulse 2) | Aparición de la Power Coin (P). |
| `0x06` | `Sound_PowerCoinMusic` | BGM | Canales 0, 1, 2 (Pulse 1, 2, Triangle) | Fanfarria activa durante la transformación a monedas. |
| `0x07` | `Sound_BombCollectedUnlit` | SFX | Canal 6 (Triangle) | Recolección de bomba apagada (100 pts). |
| `0x08` | `Sound_BombCollectedLit` | SFX | Canal 6 (Triangle) | Recolección de bomba encendida (200 pts). |
| `0x09` | `Sound_YouAreGreedy` | BGM | Canales 0, 1, 2 | Fanfarria del cartel "YOU ARE GREEDY". |
| `0x0A` | `Sound_1UP_Captive` | SFX | Canales 4, 5 | Sonido al obtener vida extra o rescatar cautivo. |
| `0x0B` | `Sound_CrystalBall` | SFX | Canales 4, 5 | Recolección de bola de cristal. |
| `0x0D` | `Sound_BonusStatueCollected`| SFX | Canales 4, 5 | Recolección de estatua faraónica. |
| `0x0E` | `Sound_SecretBlockRevealed` | SFX | Canales 4, 5 | Apertura de bloque secreto multi-golpe. |
| `0x0F` | `Sound_Door` | SFX | Canal 7 (Noise) | Apertura o cierre mecánico de puerta. |
| `0x10` | `Music_Main` | BGM | Canales 0, 1, 2, 3 | Tema musical principal de escenario. |
| `0x11` | `Music_TreasureRoom` | BGM | Canales 0, 1, 2, 3 | Tema musical de sala del tesoro. |
| `0x12` | `Music_SideRoom` | BGM | Canales 0, 1, 2, 3 | Tema musical de pasillos secundarios. |
| `0x13` | `Music_Labyrinth` | BGM | Canales 0, 1, 2, 3 | Tema musical del laberinto. |
| `0x15` | `Music_Outside1` | BGM | Canales 0, 1, 2, 3 | Tema de escenarios al aire libre (versión 1). |
| `0x16` | `Music_MainWithIntro` | BGM | Canales 0, 1, 2, 3 | Tema principal precedido de fanfarria de inicio. |
| `0x17` | `Music_GameOver` | BGM | Canales 0, 1, 2 | Fanfarria fúnebre de Game Over. |
| `0x18` | `Music_RoundClear` | BGM | Canales 0, 1, 2, 3 | Fanfarria de victoria de fin de ronda. |
| `0x19` | `Sound_BombChestBonus` | SFX | Canales 4, 5 | Notificación de cofre con moneda de poder. |
| `0x1B` | `Sound_Pause` | SFX | Canal 5 | Silbido de pausa. |
| `0x1C` | `Sound_Unpause` | SFX | Canal 5 | Reanudación de pausa. |
| `0x1D` | `Sound_UsedMightyCoin` | SFX | Canales 4, 5 | Activación de moneda de poder Mighty. |
| `0x1E` | `Music_Outside2` | BGM | Canales 0, 1, 2, 3 | Tema de escenarios al aire libre (versión 2). |
| `0x1F` | `Music_TortureRoom` | BGM | Canales 0, 1, 2, 3 | Tema tenso de la Sala de Tortura. |
| `0x20` | `Sound_BombBonus` | SFX | Canales 4, 5 | Fanfarria de bono de 10.000 pts por todas las bombas. |
| `0x21` | `Sound_TimePickup` | SFX | Canal 5 | Recolección de Bebida Mighty / tiempo. |
| `0x22` | `Music_Ending` | BGM | Canales 0, 1, 2, 3 | Melodía completa de los créditos finales. |
| `0x23` | `Music_CollectedFullFamily` | BGM | Canales 0, 1, 2, 3 | Fanfarria épica al rescatar a la familia real completa. |

---

## 6. Estructura de Datos en C x86

```c
// Estructura de 24 bytes de un canal de audio virtual (SoundChannelA_Mus/Sfx)
typedef struct {
    uint8_t status;             // +0x00: Bit 0 = Activo
    uint8_t duration_counter;   // +0x01: Temporizador regresivo en frames de la nota actual
    uint8_t duration_reload;    // +0x02: Valor de recarga de duración (desde NoteLengthTable)
    const uint8_t *data_ptr;    // +0x03/+0x04: Puntero al flujo de bytecode musical
    const uint8_t *envelope_ptr;// +0x05/+0x06: Puntero a la tabla de envolvente de volumen
    uint8_t envelope_index;     // +0x07: Índice de paso dentro de la envolvente de volumen
    uint8_t loop_counter;       // +0x08: Contador de repeticiones de bucle
    const uint8_t *loop_ptr;    // +0x09/+0x0A: Puntero de inicio de bucle
    const uint8_t *call_stack;  // +0x0B/+0x0C: Puntero de retorno de subrutina (stack de canal)
    uint8_t volume_attenuation; // +0x0D: Nivel de atenuación de volumen
    uint8_t envelope_delay;     // +0x0E: Temporizador regresivo de la fase de envolvente
    uint8_t volume_register;    // +0x0F: Espejo del registro de volumen/duty para hardware
    uint8_t sweep_register;     // +0x10: Espejo del registro sweep
    uint16_t frequency_11bit;   // +0x11/+0x12: Frecuencia de 11 bits para registros LO/HI
    uint8_t loop_repeat_count;  // +0x13: Contador de repetición del comando F1/F2
    uint8_t vibrato_speed;      // +0x14: Velocidad de vibrato / modulación de tono
    uint8_t vibrato_counter;    // +0x15: Acumulador de fase de vibrato
} MBJ_SoundChannel;

typedef struct {
    uint8_t sound_queue[16];    // Cola de IDs de sonido solicitados
    uint8_t sounds_queued;      // Cantidad de sonidos en cola
    uint8_t active_channels_mask;// Máscara para registro $4015
    MBJ_SoundChannel channels[8];// 0..3: BGM, 4..7: SFX
    
    // Registros espejo de hardware APU ($4000..$400F)
    uint8_t reg_vol[4];
    uint8_t reg_sweep[4];
    uint8_t reg_lo[4];
    uint8_t reg_hi[4];
} MBJ_AudioEngine;
```

---

## 7. Plan de Pruebas Unitarias y Verificación

1. **`test_note_frequency_shift_octaves`**:
   - Para cada una de las 12 notas y octavas 0 a 5, verificar que la frecuencia de 11 bits calculada en C coincide exactamente con `kNoteFrequencyTable[nota] >> octava`.
2. **`test_sfx_bgm_precedence_and_restoration`**:
   - Activar una pista BGM en el canal 0 (Pulse 1); luego encolar un SFX en el canal 4 (Pulse 1). Verificar que mientras el canal 4 esté activo, el canal 0 no escribe en los registros de hardware y que, al recibir `0xF6/0xF7`, el canal 0 reanuda inmediatamente su flujo y restaura su volumen y frecuencia.
3. **`test_bytecode_loops_and_subroutines`**:
   - Interpretar una secuencia sintética con comandos `0xF1/0xF2` (bucles) y `0xF3/0xF4` (llamadas anidadas a subrutina), validando que las direcciones de retorno y contadores de repetición se desapilan correctamente sin desbordamiento.
