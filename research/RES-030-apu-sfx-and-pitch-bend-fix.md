# RES-030: Diagnóstico Forense de Efectos de Sonido (SFX), Salto, Pitch Bend Inducido en Música y Ruido Blanco (`TASK-F7-003`)

- **ID de Investigación**: `RES-030`
- **Tarea Vinculada**: `TASK-F7-003` (Efectos de Sonido, Salto, Pitch Bend en Música y White Noise)
- **Área Temática**: 06 - Motor de Audio 2A03 y Bytecode Musical
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-25
- **Agente Autor**: `asm_researcher` (Nivel 3)

---

## 1. Resumen Ejecutivo y Síntomas Reportados

El usuario reportó el siguiente comportamiento anómalo en el subsistema de audio durante el gameplay:
> *"La música funciona pero los sonidos de salto y fx no se oyen, cuando se triggerean estos sonidos parece que hacen pitch bend en la música y esta suena desafinada, en alguna ocasión también generan white noise, creo que los fx no están bien mapeados en la extracción de assets."*

La auditoría forense cruzada entre el código C actual ([`src/core/mjk_apu.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c), [`include/mjk_apu.h`](file:///c:/Users/kanel/Documents/mjk/include/mjk_apu.h), [`src/core/mjk_state_machine.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c), [`src/core/mjk_game.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c), [`src/core/mjk_room_decoder.c`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c), [`tools/extract_assets.py`](file:///c:/Users/kanel/Documents/mjk/tools/extract_assets.py)) y el desmontaje original 6502 ([`asm/src/music-engine.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm), [`asm/src/music-data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-data.asm), [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm)) confirma **cuatro causas raíz exactas** que explican el 100% de los síntomas observados:

1. **Causa Raíz 1 — Sobrescritura por frame de `p1_period` y Desafinación/Pitch Bend de la melodía BGM al saltar (`src/core/mjk_apu.c:606-685`)**:
   - `MjkAPU_PlaySfx` no utiliza canales de bytecode dedicados para SFX (`channels[4..7]`), sino que intenta escribir parámetros sintéticos directamente sobre `apu->p1_period = 350` (Canal Pulse 1).
   - Sin embargo, en cada frame a 60 Hz, `MjkAPU_UpdateFrame` ([`src/core/mjk_apu.c:653-663`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L653-L663)) **sobrescribe incondicionalmente** `apu->p1_period = apu->channels[0].period`, `apu->p1_volume = apu->channels[0].duty_and_vol & 0x0F` y `apu->p1_duty` con el estado actual de la melodía principal de la música (`channels[0]`).
   - Inmediatamente después, las líneas [`src/core/mjk_apu.c:678-679`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L678-L679) ejecutan `if (apu->active_sfx == SFX_JUMP) { if (apu->p1_period > 100) apu->p1_period -= 14; }`.
   - Como `p1_period` acaba de ser recargado con la nota actual de la melodía BGM (`channels[0].period`), no se produce ningún barrido de salto: en su lugar, durante 18 frames (300 ms), **la melodía principal en Pulse 1 suena con su período reducido en 14 unidades (`channels[0].period - 14`)**, elevando su afinación entre +40 y +80 cents respecto a la armonía (Pulse 2) y el bajo (Triangle). El efecto de salto jamás llega a oírse y la música sufre un *pitch bend* desafinado cada vez que Jack salta.
   - Del mismo modo, `SFX_COLLECT_BOMB` y `SFX_FIRE_BOMB` ([`src/core/mjk_apu.c:615-623`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L615-L623)) fijan `p1_period = 180`, que es machacado en el mismo frame por `apu->channels[0].period` sin modulación ulterior, haciéndolos 100% inaudibles cuando hay música activa.

2. **Causa Raíz 2 — Ruido Blanco Espurio y Enganche Infinito del Canal Noise (`src/core/mjk_apu.c:625-711` y `782-791`)**:
   - En el código C actual, `SFX_OPEN_CHEST` activa el generador LFSR de ruido blanco (`apu->noise_enabled = true; apu->noise_period = 8; apu->noise_volume = 15; apu->sfx_frames_left = 25;` en [`src/core/mjk_apu.c:625-631`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L625-L631)) y se invoca erróneamente al entrar/salir de salas ([`src/core/mjk_state_machine.c:389, 401, 694`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L389-L401)), al abrir cofres ([`src/core/mjk_game.c:100, 108`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L100-L108)), al revelar bloques secretos ([`src/core/mjk_game.c:185`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L185)) y al recoger ítems ([`src/core/mjk_game.c:222-243`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L222-L243)). En la ROM original de NES, **ninguno** de esos eventos usa el canal de ruido.
   - **Bug de White Noise Infinito**: Si mientras `SFX_OPEN_CHEST` está activo (25 frames) se dispara cualquier otro SFX (por ejemplo, `SFX_JUMP` al saltar tras entrar a la sala o al rebotar sobre un cofre), `apu->active_sfx` cambia a `SFX_JUMP` sin apagar `apu->noise_enabled` ni poner `apu->noise_volume = 0`. A partir de ese instante, `MjkAPU_UpdateFrame` deja de decrementar `apu->noise_volume` ([`src/core/mjk_apu.c:680-681`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L680-L681)), el fin de `SFX_JUMP` no apaga `noise_enabled` ([`src/core/mjk_apu.c:687-695`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L687-L695)), e incluso cuando `SFX_OPEN_CHEST` expira normalmente con música activa, la línea [`src/core/mjk_apu.c:697`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L697) (`if (!apu->music_enabled) apu->noise_enabled = false;`) **no apaga `noise_enabled`**. En consecuencia, `MjkAPU_GenerateFrameSamples` ([`src/core/mjk_apu.c:787-791`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L787-L791)) mantiene el ruido blanco encendido indefinidamente.

3. **Causa Raíz 3 — Ausencia de los 4 Canales Virtuales SFX (`channels[4..7]`), Hardware Sweep (`$F5`) y Pitch Bend por Software (`$D8..$DF`) en `src/core/mjk_apu.c`**:
   - El motor original Tecmo (`HandleSound`, [`asm/src/music-engine.asm:19-42`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L19-L42)) opera sobre **8 canales virtuales**: `0..3` para BGM y `4..7` para SFX (`SoundPointersA`, [`asm/src/music-engine.asm:793-802`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L793-L802)).
   - Los SFX son secuencias de bytecode reales en `SoundPointers` ([`asm/src/music-data.asm:133-170`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-data.asm#L133-L170)), donde el salto (`SoundData_01_Jump`), la bomba encendida (`SoundData_08_GetFireBomb`) y la muerte (`SoundData_02_Death`) dependen de la unidad **Hardware Sweep (`$F5`)** del 2A03 (`$4001/$4005`), y efectos como puertas (`SoundData_0F_Door`), muerte (`SoundData_02_Death`), Power Coin (`SoundData_05_PowerCoinSpawn`) y Mighty Coin (`SoundData_1D_UseMightyCoin`) dependen del **Pitch Bend de bytecode (`$D8..$DF`)** en [`asm/src/music-engine.asm:181-218`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L181-L218).
   - En [`src/core/mjk_apu.c:478-481`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L478-L481) y [`527-530`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L527-L530), `Channel_Tick` lee `$D8..$DF` (`pitch_bend_step`) y `$F5` (`sweep_reg`), pero **nunca aplica su lógica acústica**.

4. **Causa Raíz 4 — Truncamiento en `tools/extract_assets.py`, Desconexión de Assets y Llamadas SFX Omitidas o Erróneas en Gameplay**:
   - [`tools/extract_assets.py:69-71`](file:///c:/Users/kanel/Documents/mjk/tools/extract_assets.py#L69-L71) extrae el bloque de audio desde `0xF000` hasta `0xFD00`, truncando los últimos 672 bytes de `music-data.asm` (`$FD00..$FFA0`), donde residen `SoundData_1D_UseMightyCoin`, `SoundData_1F_Music_Torture`, `SoundData_20_BombBonus`, `SoundData_21_GetMightyDrink`, `SoundData_22_Music_Ending` y `SoundData_23_Music_BestEnding`.
   - `MjkRoom_CollectBombCanonical` ([`src/core/mjk_room_decoder.c:423-611`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c#L423-L611)) **no invoca ninguna función de sonido** al recoger bombas normales (`Sound_BombCollectedUnlit` `$07`), bombas encendidas (`Sound_BombCollectedLit` `$08`), bono de 20 bombas (`Sound_BombChestBonus` `$19` / `Sound_PowerCoinSpawn` `$05`) ni bono de 10.000 puntos (`Sound_BombBonus` `$20`).

---

## 2. Arquitectura Ground Truth del Motor de Sonido 6502 (`asm/src/music-engine.asm` y `asm/src/music-data.asm`)

### 2.1 Coexistencia BGM vs SFX: 8 Canales Virtuales y Multiplexado sin Desfase

En [`asm/src/music-engine.asm:19-42`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L19-L42) (`HandleSound`) y [`793-811`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L793-L811) (`SoundPointersA` / `SoundPointersB`), el motor mantiene 8 estructuras de canal de 24 bytes cada una:

| Índice (`CurrentSoundChannel`) | Símbolo 6502 (`ram.asm`) | Dirección RAM | Rol | Canal Hardware 2A03 (`SoundRegIndex = (ch & 3) * 4`) |
| :---: | :--- | :---: | :---: | :--- |
| `0` | `SoundChannelA_Mus0` | `$0452-$0469` | BGM Voz 0 | Pulse 1 (`$4000-$4003`, `X = $00`) |
| `1` | `SoundChannelA_Mus1` | `$046A-$0481` | BGM Voz 1 | Pulse 2 (`$4004-$4007`, `X = $04`) |
| `2` | `SoundChannelA_Mus2` | `$0482-$0499` | BGM Voz 2 | Triangle (`$4008-$400B`, `X = $08`) |
| `3` | `SoundChannelA_Mus3` | `$049A-$04B1` | BGM Voz 3 | Noise (`$400C-$400F`, `X = $0C`) |
| `4` | `SoundChannelA_Sfx0` | `$04B2-$04C9` | SFX Voz 0 | Pulse 1 (`$4000-$4003`, `X = $00`) |
| `5` | `SoundChannelA_Sfx1` | `$04CA-$04E1` | SFX Voz 1 | Pulse 2 (`$4004-$4007`, `X = $04`) |
| `6` | `SoundChannelA_Sfx2` | `$04E2-$04F9` | SFX Voz 2 | Triangle (`$4008-$400B`, `X = $08`) |
| `7` | `SoundChannelA_Sfx3` | `$04FA-$0511` | SFX Voz 3 | Noise (`$400C-$400F`, `X = $0C`) |

#### Reglas de Prioridad y Restauración (`CheckIfSoundChannelIsActive` y `SoundCommand_F6_F7`)
1. **Avance continuo de BGM en segundo plano**: En cada NMI (`HandleSound`, [`asm/src/music-engine.asm:19-40`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L19-L40)), los 8 canales activos ejecutan `HandleSoundChannel`. Los canales BGM (`0..3`) nunca detienen su reloj ni pierden el compás cuando suena un SFX.
2. **Inhibición de escritura a registros de hardware (`CheckIfSoundChannelIsActive`, [`asm/src/music-engine.asm:751-771`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L751-L771))**:
   - Cuando un canal actualiza su envolvente (`MaybeResetVolDelay`, línea 699) o emite una nota (`sub_B285`, línea 266), llama a `CheckIfSoundChannelIsActive`:
   - Si `CurrentSoundChannel >= 4` (es un canal SFX), retorna `A = 0` (`Z = 1`), permitiendo volcar volumen, sweep y período a los registros espejo `SoundReg_Vol/Sweep/Lo/Hi[X]`.
   - Si `CurrentSoundChannel < 4` (es un canal BGM), inspecciona `channels[CurrentSoundChannel | 4].status & 1`. Si el canal SFX hermano está activo (`!= 0`, `Z = 0`), **inhibe** la escritura en `SoundReg_*[X]`, pero conserva intactos `duty_and_vol` (`+$0F`), `sweep_reg` (`+$10`), `freq_lo` (`+$11`) y `freq_hi` (`+$12`) dentro de la estructura privada del canal BGM.
3. **Restauración instantánea al terminar el SFX (`SoundCommand_F6_F7`, [`asm/src/music-engine.asm:605-630`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L605-L630))**:
   - Cuando un stream de bytecode lee `$F6`..`$FF` (en la práctica `$FF`), apaga su propio canal (`status = 0`).
   - Si `CurrentSoundChannel & 4` es no nulo (terminó un canal SFX `4..7`), carga el puntero del canal BGM hermano `CurrentSoundChannel & 3` y ejecuta `SoundCommand_F6_F7_sub1` + `ORA #$20` sobre `SoundReg_Vol,X`.
   - Esto copia inmediatamente `sweep_reg` (`+$10`), `freq_lo` (`+$11`), `freq_hi` (`+$12`) y `duty_and_vol | 0x20` (`+$0F`) del canal BGM de vuelta a los registros del hardware APU sin esperar al siguiente cambio de nota de la canción.

---

### 2.2 Unidad Hardware Sweep (`$F5` -> `$4001 / $4005`)

En [`asm/src/music-engine.asm:160-162`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L160-L162), al expirar la duración de una nota y antes de leer el siguiente comando/nota, el motor resetea el registro de barrido del canal a `0x08` (barrido desactivado, flag negate activo para evitar silenciamiento por carry de sumador):
```asm
	LDY #$10
	LDA #8
	STA (SoundEnginePointer),Y	; sweep_reg = $08
```
Si la secuencia incluye el comando `$F5 <byte>` (`SoundCommand_F5`, [`asm/src/music-engine.asm:594-601`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L594-L601)), el valor `<byte>` se guarda en `sweep_reg` (`+$10`) y se escribe en `$4001` (Pulse 1) o `$4005` (Pulse 2) al emitir la nota (`WriteSoundRegisters`, [`asm/src/music-engine.asm:730-738`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L730-L738)).

#### Semántica Hardware del Registro Sweep 2A03 (`EPPP NSSS`)
- **Bit 7 (`E = 0x80`)**: Enable. Si es `1`, el barrido de hardware está activo.
- **Bits 6..4 (`P = (sweep >> 4) & 7`)**: Período del divisor de barrido en unidades de medio frame (120 Hz NTSC). El divisor actualiza el período del oscilador cada $P + 1$ medios frames.
  - En *Mighty Bomb Jack*, todos los comandos `$F5` usan $P = 1$ (`$96`, `$9D`, `$9C`, `$9B`, `$97` tienen bits 6..4 = `001`), lo que equivale a $P + 1 = 2$ medios frames = **1 actualización por cada frame de 60 Hz** (o 2 ticks de divisor por frame con recarga en $P$).
- **Bit 3 (`N = 0x08`)**: Negate. Si es `1`, resta el desplazamiento $\Delta$ al período (tono ascendente / más agudo). Si es `0`, suma $\Delta$ al período (tono descendente / más grave).
- **Bits 2..0 (`S = sweep & 7`)**: Shift count. Si $E = 1$ y $S > 0$ y el período actual $T \ge 8$:
  $$\Delta = T \gg S$$
  $$T_{\text{nuevo}} = \begin{cases} T - \Delta & \text{si } N = 1 \\ T + \Delta & \text{si } N = 0 \end{cases}$$

---

### 2.3 Modulación de Tono / Pitch Bend por Software (`$D8..$DF`, `loc_B214`)

En [`asm/src/music-engine.asm:390-403`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L390-L403) (`SoundCommand_Dx` cuando `(cmd & 0x0F) >= 8`):
- Lee el paso de semitonos con signo desde `byte_B52C[cmd & 3]` ([`asm/src/music-engine.asm:774-775`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L774-L775)):
  - `$D8` (índice 0): `+1`
  - `$D9` (índice 1): `+2`
  - `$DA` (índice 2): `-1` (`$FF`)
  - `$DB` (índice 3): `-2` (`$FE`)
- **Detalle crítico de estado (`music-engine.asm:395-400`)**: Compara el nuevo `pitch_bend_step` con el valor ya almacenado en `+$14`. **Solo si es distinto** resetea `pitch_bend_acc` (`+$15`) a `0`. Si el comando `$D8..$DF` se repite dentro de un bucle `$F1..$F2` con el mismo valor (como ocurre en `SoundTrack_02_0` con `$DB` y en `SoundTrack_0F_4/5` con `$D8`), `pitch_bend_acc` **no se resetea**, sino que sigue incrementándose en cada nota emitida (`1, 2, 3, ...`), generando una escala progresiva ascendente o descendente a lo largo de las iteraciones del bucle.

En [`asm/src/music-engine.asm:181-218`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L181-L218) (`loc_B214`), cada vez que se emite un byte de nota `note_byte` (`0x00..0x7F`) y `pitch_bend_step != 0`:
1. Incrementa `pitch_bend_acc` en 1 (`ch->pitch_bend_acc++`).
2. Ejecuta un bucle `count = ch->pitch_bend_acc` veces sobre `note_byte`:
   ```c
   uint8_t count = ++ch->pitch_bend_acc;
   uint8_t n = note_byte;
   for (uint8_t i = 0; i < count; ++i) {
       n = (uint8_t)(n + (uint8_t)ch->pitch_bend_step);
       if ((n & 0x0F) >= 0x0B) {
           if (ch->pitch_bend_step > 0) {
               n = (uint8_t)(n + 5); // Carry=1 de CMP #$B + ADC #4 -> salta de semitono >=11 a octava siguiente
           } else {
               n = (uint8_t)(n - 4); // Carry=1 de CMP #$B + SBC #4 -> ajusta wrap negativo ($xF -> $xB)
           }
       }
   }
   ```
3. Con el `n` resultante, extrae `semitone = n & 0x0F` y `octave = (n >> 4) & 0x07`, indexando `NoteFrequencyTable[semitone] >> octave`.

---

### 2.4 Frecuencia Directa / Ruido (`$C0..$CF`, `SoundCommand_Cx`)

En [`asm/src/music-engine.asm:355-367`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L355-L367):
- Cuando el intérprete lee un opcode `$C0..$CF` (usado por el canal 7 de ruido en `SoundTrack_20_3`: `$CF` y `$CD`, [`asm/src/music-data.asm:2101-2108`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-data.asm#L2101-L2108)):
- Fija `freq_lo = cmd & 0x0F` (`0..15`, índice de período del temporizador de ruido 2A03 en `$400E`) y `freq_hi = 0x08` (`$400F`).
- Llama a `sub_B285` y hace `PLA; PLA; JMP loc_B274` (termina la lectura de bytecode del frame actual de la misma forma que tras emitir una nota, esperando a que `duration_counter` se agote).

---

## 3. Mapeo Canónico de los 35 IDs de Sonido (`SoundPointers`) vs Eventos de Gameplay

En [`asm/src/defs.asm:84-121`](file:///c:/Users/kanel/Documents/mjk/asm/src/defs.asm#L84-L121) y [`asm/src/music-data.asm:133-170`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-data.asm#L133-L170), el motor indexa `SoundPointers[sound_id - 1]` (para `sound_id == 0`, `HandleSoundQueue` silencia los 8 canales en [`asm/src/music-engine.asm:49-68`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-engine.asm#L49-L68)):

| ID (`QueueSound`) | Símbolo `defs.asm` | Etiqueta `music-data.asm` | Canales Virtuales | Cita en `asm/src/prg.asm` | Evento Exacto que lo Dispara | Estado Actual en C (`src/`) |
| :---: | :--- | :--- | :---: | :--- | :--- | :--- |
| `$00` | `Sound_Silence` | — (limpia 0..7) | `0..7` | `prg.asm:341, 712, 1001, 1321, 5330` | Init, Torture Intro, Fin Round Clear, Muerte, Entrar en Puerta (`JTPickup_Door`) | Parcial (`MjkAPU_StopMusic`) |
| `$01` | `Sound_Jump` | `SoundData_01_Jump` (`:174`) | `5` (SFX P2) | `prg.asm:1428-1430` | Salto de Jack **solo desde el suelo** (el aleteo/flutter aéreo NO dispara sonido) | **ROTO** (usa P1 falso y desafina BGM; en Torture Room sonaba en el aire) |
| `$02` | `Sound_Death` | `SoundData_02_Death` (`:189`) | `0` (y apaga `1,2,4,5,6`) | `prg.asm:682, 1322` | Inicio de muerte de Jack (`loc_8874`) e inicio de `GameState_6_TimeOver` | **ROTO** (usaba ruido falso + P1; faltaba en Time Over y muerte por lava) |
| `$03` | `Sound_BonusCoinSpawn` | `SoundData_03_BonusCoinSpawn` (`:209`) | `4, 5` (SFX P1, P2) | `prg.asm:5475, 9533` | Aparición de Extra Coin (`$09`) o Bonus Coin (`$0E`) | **AUSENTE** |
| `$04` | `Sound_CoinCollected` | `SoundData_04_CoinCollected` (`:229`) | `5` (SFX P2) | `prg.asm:4033, 4088, 4129, 4935, 5190` | Recoger Moneda Dorada, Enemigo convertido en moneda, Extra/Bonus Coin, Secret Coin o Mighty Coin | **ERRÓNEO** (llamaba a `SFX_COLLECT_BOMB` inaudible) |
| `$05` | `Sound_PowerCoinSpawn` | `SoundData_05_PowerCoinSpawn` (`:243`) | `0` (y apaga `1,2`) | `prg.asm:4994-4998` | Spawn de Power Coin en Bomb Room al llegar a 20 puntos de bomba | **AUSENTE** |
| `$06` | `Sound_PowerCoinMusic` | `SoundData_06_PowerCoinMusic` (`:265`) | `0, 1, 2` (BGM) | `prg.asm:9027-9028` | `TurnEnemiesIntoCoins` (dura 320 frames; al expirar en `prg.asm:1392` restaura BGM de sala) | **AUSENTE** (`SFX_POWER_COIN` caía en `default: break;`) |
| `$07` | `Sound_BombCollectedUnlit`| `SoundData_07_GetNormalBomb` (`:405`) | `5` (SFX P2) | `prg.asm:5003-5006` | Recoger bomba apagada (100 pts) en `JTPickup_Bomb` | **AUSENTE** (`MjkRoom_CollectBombCanonical` no llamaba al APU) |
| `$08` | `Sound_BombCollectedLit` | `SoundData_08_GetFireBomb` (`:391`) | `5` (SFX P2) | `prg.asm:4111, 4924, 5003-5006` | Recoger bomba encendida (200 pts), bolsas de 100/300/1000 pts (`JTPickup_Bag*`) y globo (`JT_98A4_C`) | **AUSENTE** en bombas; inaudible en bolsas |
| `$09` | `Sound_YouAreGreedy` | `SoundData_09_YouAreGreedy` (`:416`) | `0, 1` (apaga `2`) | `prg.asm:713-714` | Fanfarria de pantalla `"YOU ARE GREEDY"` (`GameState_7`) | **AUSENTE** |
| `$0A` | `Sound_1UP_Captive` | `SoundData_0A_Get1UP` (`:437`) | `4, 5, 6` | `prg.asm:5810-5811` | `MaybeAward1UP` (Tecmo Plate / Cautivo / Extra Coin) | **ROTO** (`SFX_1UP` caía en `default: break;`) |
| `$0B` | `Sound_CrystalBall` | `SoundData_0B_GetCrystalBall` (`:474`) | `4, 5, 6` | `prg.asm:5218-5219` | Recoger Crystal Ball 1 o 2 (`JTPickup_CrystalBall1/2`) | **ERRÓNEO** (disparaba `SFX_OPEN_CHEST` = ruido blanco) |
| `$0C` | `Sound_C_Null` | `SoundData_Null` (`:171`) | Ninguno | No usado | Entrada vacía (`$FF`) | — |
| `$0D` | `Sound_BonusStatueCollected`| `SoundData_0D_GetBonusStatue` (`:526`) | `5` (SFX P2) | `prg.asm:4103-4104` | Recoger estatua de bonificación (`JT_98A4_B_BonusStatue`) | **AUSENTE** |
| `$0E` | `Sound_SecretBlockRevealed`| `SoundData_0E_SecretBlockOpened` (`:542`)| `5` (SFX P2) | `prg.asm:5455-5456` | Descubrir bloque secreto multi-golpe (`$6N`) en `MaybeOpenHiddenBlock` (NOTA: abrir cofres normales/ocultos NO emite sonido en NES) | **ERRÓNEO** (disparaba `SFX_OPEN_CHEST` = ruido blanco) |
| `$0F` | `Sound_Door` | `SoundData_0F_Door` (`:564`) | `4, 5, 6` | `prg.asm:7041, 7080, 7230` | `HandleOpenDoorMaybe`, `HandleSphinxEvent` y `AnimateDoor` (abrir/cerrar puerta al entrar a sala o al cumplir condición) | **ERRÓNEO** (disparaba `SFX_OPEN_CHEST` = ruido blanco; `SFX_DOOR_OPEN` era no-op) |
| `$10` | `Music_Main` | `SoundData_10_Music_Main` (`:595`) | `0, 1, 2` (BGM) | `data.asm:314`, `prg.asm:1392` | Tema principal sin intro (al restaurar música tras Power Coin) | Soportado |
| `$11` | `Music_TreasureRoom` | `SoundData_11_Music_Treasure` (`:964`) | `0, 1, 2` (BGM) | `data.asm:315` | Tema de salas de bombas reales / tesoro (`RoomMusic == 2`) | No conectado en `PlayMusic` |
| `$12` | `Music_SideRoom` | `SoundData_12_Music_Side` (`:1349`) | `0, 1, 2` (BGM) | `data.asm:316` | Tema de salas laterales (`RoomMusic == 3`) | No conectado en `PlayMusic` |
| `$13` | `Music_Labyrinth` | `SoundData_13_Music_Labyrinth` (`:1427`) | `0, 1, 2` (BGM) | `data.asm:317` | Tema de laberinto (`RoomMusic == 4`) | No conectado en `PlayMusic` |
| `$14` | `Sound_14_Null` | `SoundData_Null` (`:171`) | Ninguno | No usado | Entrada vacía (`$FF`) | — |
| `$15` | `Music_Outside1` | `SoundData_15_Music_Outside` (`:1468`) | `0, 1, 2` (BGM) | `data.asm:318` | Tema exterior (`RoomMusic == 5`) | No conectado en `PlayMusic` |
| `$16` | `Music_MainWithIntro` | `SoundData_16_Music_MainIntro` (`:1650`) | `0, 1, 2` (BGM) | `data.asm:313`, `prg.asm:621` | Tema principal con intro al iniciar sala (`RoomMusic == 0`) | Soportado |
| `$17` | `Music_GameOver` | `SoundData_17_Music_GameOver` (`:1733`) | `0, 1, 2` (BGM) | `prg.asm:1046-1047` | Pantalla de Game Over (`GameState_A_GameOver`) | **AUSENTE** |
| `$18` | `Music_RoundClear` | `SoundData_18_Music_RoundClear` (`:1789`) | `0, 1, 2` (BGM) | `prg.asm:868-869` | Fanfarria de fin de ronda (`GameState_8_RoundClear`) | **AUSENTE** |
| `$19` | `Sound_BombChestBonus` | `SoundData_19_BombChestBonus` (`:1846`) | `4, 5, 6` | `prg.asm:4989-4998` | Alcanzar 20 puntos de bombas en sala normal (habilita Power Coin en el próximo cofre) | **AUSENTE** |
| `$1A` | `Sound_1A` | `SoundData_1A` (`:1886`) | `0, 1` (apaga `2`) | No referenciado | Efecto interno de prueba | — |
| `$1B` | `Sound_Pause` | `SoundData_1B_Pause` (`:1909`) | `4, 5, 6` | `prg.asm:1179-1183` | Pausar partida | **AUSENTE** |
| `$1C` | `Sound_Unpause` | `SoundData_1C_Unpause` (`:1944`) | `4, 5, 6` | `prg.asm:1176-1183` | Reanudar partida | **AUSENTE** |
| `$1D` | `Sound_UsedMightyCoin` | `SoundData_1D_UseMightyCoin` (`:1977`) | `4, 5, 6` | `prg.asm:1757-1758` | Usar moneda Mighty con botón B (`UseMightyCoin`) | **ROTO** (`SFX_MIGHTY_POWER` caía en `default: break;`) |
| `$1E` | `Music_Outside2` | `SoundData_15_Music_Outside` (`:1468`) | `0, 1, 2` (BGM) | `data.asm:319` | Duplicado de `$15` (`RoomMusic == 6`) | — |
| `$1F` | `Music_TortureRoom` | `SoundData_1F_Music_Torture` (`:2025`) | `0, 1, 2` (BGM) | `data.asm:320` | Música de la Sala de Tortura (`Room $AC`, `RoomMusic == 7`) | No conectado en `PlayMusic` |
| `$20` | `Sound_BombBonus` | `SoundData_20_BombBonus` (`:2046`) | `4, 5, 6, 7` | `prg.asm:5056, 7917` | Bono de 10.000 pts por recoger todas las bombas en orden (`loc_9EDB`) y excavación en Ending (`EndingState_2`) | **AUSENTE** |
| `$21` | `Sound_TimePickup` | `SoundData_21_GetMightyDrink` (`:2109`) | `4, 5, 6` | `prg.asm:4945-4946` | Recoger Mighty Drink (`JTPickup_MightyDrink`) | **ROTO** (`SFX_MIGHTY_POWER` caía en `default: break;`) |
| `$22` | `Music_Ending` | `SoundData_22_Music_Ending` (`:2140`) | `0, 1, 2` (BGM) | `prg.asm:7777-7778` | Música de secuencia final (`EndingState_3`) | **AUSENTE** |
| `$23` | `Music_CollectedFullFamily`| `SoundData_23_Music_BestEnding` (`:2437`)| `0, 1, 2` (BGM) | `prg.asm:5242-5243` | Recoger máscara de Beelzebub (`JTPickup_Beelzebub`) | **ERRÓNEO** (disparaba `SFX_OPEN_CHEST` = ruido blanco) |

---

## 4. Bytecode Exacto de Todos los Efectos de Sonido (`asm/src/music-data.asm`)

A continuación se documentan los flujos de bytecode exactos verificados línea por línea en [`asm/src/music-data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/music-data.asm):

### 4.1 `Sound_Jump` (`$01` — `asm/src/music-data.asm:174-188`)
- **Canal**: `5` (SFX Pulse 2).
- **Stream `SoundTrack_01_5`** (11 bytes):
  ```c
  // $A0: Env 0 | $D1: Duty 25% | $F5 $96: Sweep(E=1,P=1,N=1,S=6) | $84: Len 4 (4f) | $32: D3
  // $F5 $9D: Sweep(E=1,P=1,N=1,S=5) | $8A: Len 10 (32f) | $30: C3 | $FF: End
  static const uint8_t k_sfx_01_jump_ch5[] = {
      0xA0, 0xD1, 0xF5, 0x96, 0x84, 0x32, 0xF5, 0x9D, 0x8A, 0x30, 0xFF
  };
  ```

### 4.2 `Sound_Death` (`$02` — `asm/src/music-data.asm:189-208`)
- **Canales**: `0` (BGM Pulse 1) ejecuta `SoundTrack_02_0`; `1, 2, 4, 5, 6` reciben `SoundData_Null` (`0xFF`, apagándolos inmediatamente).
- **Stream `SoundTrack_02_0`** (11 bytes):
  ```c
  // $A0: Env 0 | $D2: Duty 50% | $84: Len 4 (4f) | $F1 $13: Loop 19 veces
  //   $F5 $9C: Sweep(E=1,P=1,N=1,S=4) | $40: C4 | $DB: PitchBend -2 semitonos/iter
  // $F2: Loop End | $FF: End
  static const uint8_t k_sfx_02_death_ch0[] = {
      0xA0, 0xD2, 0x84, 0xF1, 0x13, 0xF5, 0x9C, 0x40, 0xDB, 0xF2, 0xFF
  };
  ```

### 4.3 `Sound_BonusCoinSpawn` (`$03` — `asm/src/music-data.asm:209-228`)
- **Canales**: `4` (SFX Pulse 1) y `5` (SFX Pulse 2).
- **Streams**:
  ```c
  static const uint8_t k_sfx_03_bonus_coin_ch4[] = { 0xA0, 0xD1, 0x86, 0x57, 0x8A, 0x57, 0xFF };
  static const uint8_t k_sfx_03_bonus_coin_ch5[] = { 0xA0, 0xD1, 0x86, 0x56, 0x8A, 0x56, 0xFF };
  ```

### 4.4 `Sound_CoinCollected` (`$04` — `asm/src/music-data.asm:229-242`)
- **Canal**: `5` (SFX Pulse 2).
- **Stream `SoundTrack_04_5`** (10 bytes):
  ```c
  static const uint8_t k_sfx_04_coin_collected_ch5[] = {
      0xA0, 0xD1, 0x81, 0x30, 0x34, 0x38, 0x3C, 0x8A, 0x40, 0xFF
  };
  ```

### 4.5 `Sound_PowerCoinSpawn` (`$05` — `asm/src/music-data.asm:243-264`)
- **Canales**: `0` (BGM Pulse 1) y apaga `1, 2`.
- **Stream `SoundTrack_05_0`** (18 bytes, bucle en offset 4):
  ```c
  static const uint8_t k_sfx_05_power_coin_spawn_ch0[] = {
      0xA0, 0xD1, 0xE8, 0x81,
      /* offset 4: */ 0xF1, 0x1F, 0x20, 0xD8, 0xF2, 0xF1, 0x1F, 0xDA, 0x47, 0xF2,
      0xF0, 0x04, 0x00
  };
  ```

### 4.6 `Sound_BombCollectedUnlit` (`$07` — `asm/src/music-data.asm:405-415`)
- **Canal**: `5` (SFX Pulse 2).
- **Stream `SoundTrack_07_5`** (7 bytes):
  ```c
  static const uint8_t k_sfx_07_bomb_unlit_ch5[] = {
      0xA0, 0xD1, 0x82, 0x47, 0x0C, 0x48, 0xFF
  };
  ```

### 4.7 `Sound_BombCollectedLit` (`$08` — `asm/src/music-data.asm:391-404`)
- **Canal**: `5` (SFX Pulse 2).
- **Stream `SoundTrack_08_5`** (10 bytes):
  ```c
  static const uint8_t k_sfx_08_bomb_lit_ch5[] = {
      0xA0, 0xD1, 0x84, 0xF5, 0x9B, 0x34, 0xF5, 0x9B, 0x37, 0xFF
  };
  ```

### 4.8 `Sound_1UP_Captive` (`$0A` — `asm/src/music-data.asm:437-473`)
- **Canales**: `4, 5, 6` (SFX Pulse 1, Pulse 2, Triangle).
- **Streams**:
  ```c
  static const uint8_t k_sfx_0a_1up_ch4[] = {
      0xA0, 0xD1, 0x84, 0x40, 0x37, 0x39, 0x34, 0x39, 0x3B, 0x8A, 0x40, 0xFF
  };
  static const uint8_t k_sfx_0a_1up_ch5[] = {
      0xA0, 0xD1, 0x84, 0x37, 0x34, 0x34, 0x30, 0x35, 0x37, 0x8A, 0x37, 0xFF
  };
  static const uint8_t k_sfx_0a_1up_ch6[] = {
      0x89, 0x0C, 0x8A, 0x0C, 0xFF
  };
  ```

### 4.9 `Sound_CrystalBall` (`$0B` — `asm/src/music-data.asm:474-525`)
- **Canales**: `4, 5, 6`.
- **Streams**:
  ```c
  static const uint8_t k_sfx_0b_crystal_ch4[] = {
      0xA1, 0xD1, 0x96, 0x34, 0x0C, 0x34, 0x37, 0x40, 0x0C, 0x37, 0x0C, 0x9A, 0x40, 0x0C, 0xFF
  };
  static const uint8_t k_sfx_0b_crystal_ch5[] = {
      0xA1, 0xD1, 0x96, 0x30, 0x0C, 0x30, 0x34, 0x37, 0x0C, 0x34, 0x0C, 0x9A, 0x37, 0x0C, 0xFF
  };
  static const uint8_t k_sfx_0b_crystal_ch6[] = {
      0xA4, 0x96, 0x27, 0x0C, 0x27, 0x30, 0x34, 0x0C, 0x30, 0x0C, 0x9A, 0x34, 0x0C, 0xFF
  };
  ```

### 4.10 `Sound_BonusStatueCollected` (`$0D` — `asm/src/music-data.asm:526-541`)
- **Canal**: `5` (SFX Pulse 2).
- **Stream**:
  ```c
  static const uint8_t k_sfx_0d_statue_ch5[] = {
      0xA0, 0xD1, 0x82, 0x30, 0x36, 0x33, 0x39, 0x36, 0x40, 0x39, 0x43, 0xFF
  };
  ```

### 4.11 `Sound_SecretBlockRevealed` (`$0E` — `asm/src/music-data.asm:542-563`)
- **Canal**: `5` (SFX Pulse 2).
- **Stream**:
  ```c
  static const uint8_t k_sfx_0e_secret_block_ch5[] = {
      0xA0, 0xD1, 0x81, 0x40, 0x36, 0x39, 0x33, 0x36, 0x30, 0x33, 0x29, 0x30, 0x26, 0x29, 0x23, 0x26, 0x20, 0xFF
  };
  ```

### 4.12 `Sound_Door` (`$0F` — `asm/src/music-data.asm:564-594`)
- **Canales**: `4, 5, 6` (SFX Pulse 1, Pulse 2, Triangle).
- **Streams**:
  ```c
  static const uint8_t k_sfx_0f_door_ch4[] = {
      0xA0, 0xD1, 0x82, 0xF1, 0x0B, 0x30, 0x32, 0xD8, 0xF2, 0xFF
  };
  static const uint8_t k_sfx_0f_door_ch5[] = {
      0xA0, 0xD1, 0x82, 0xF1, 0x0B, 0x27, 0x29, 0xD8, 0xF2, 0xFF
  };
  static const uint8_t k_sfx_0f_door_ch6[] = {
      0x99, 0x0C, 0xFF
  };
  ```

### 4.13 `Sound_BombChestBonus` (`$19` — `asm/src/music-data.asm:1846-1885`)
- **Canales**: `4, 5, 6`.
- **Streams**:
  ```c
  static const uint8_t k_sfx_19_bomb_chest_ch4[] = {
      0xA0, 0xD1, 0x84, 0x44, 0x40, 0x45, 0x42, 0x47, 0x44, 0x49, 0x45, 0x8A, 0x4B, 0xFF
  };
  static const uint8_t k_sfx_19_bomb_chest_ch5[] = {
      0xA0, 0xD1, 0x84, 0x40, 0x37, 0x42, 0x39, 0x44, 0x3B, 0x45, 0x40, 0x8A, 0x47, 0xFF
  };
  static const uint8_t k_sfx_19_bomb_chest_ch6[] = {
      0xA4, 0x8C, 0x0C, 0xFF
  };
  ```

### 4.14 `Sound_UsedMightyCoin` (`$1D` — `asm/src/music-data.asm:1977-2024`)
- **Canales**: `4, 5, 6`. (Con subrutina compartida `SoundTrack_FD04`: `0x81, 0x30, 0x34, 0x32, 0x36, 0x34, 0x38, 0x36, 0x3A, 0xF4`).

### 4.15 `Sound_BombBonus` (`$20` — `asm/src/music-data.asm:2046-2108`)
- **Canales (REV_US)**: `4, 5, 6, 7`.
- `SoundTrack_20_3` (Canal 7 = SFX Noise): `0xA5, 0x86, 0xCF, 0xA7, 0x8E, 0xCD, 0xFF`.

### 4.16 `Sound_TimePickup` / Mighty Drink (`$21` — `asm/src/music-data.asm:2109-2139`)
- **Canales**: `4, 5, 6`.
- **Streams**:
  ```c
  static const uint8_t k_sfx_21_mighty_drink_ch4[] = {
      0xA7, 0xD1, 0x85, 0x40, 0x44, 0x87, 0x47, 0x89, 0x40, 0xFF
  };
  static const uint8_t k_sfx_21_mighty_drink_ch5[] = {
      0xA7, 0xD1, 0x85, 0x37, 0x40, 0x87, 0x44, 0x89, 0x37, 0xFF
  };
  static const uint8_t k_sfx_21_mighty_drink_ch6[] = {
      0x8B, 0x0C, 0xFF
  };
  ```

---

## 5. Especificación Quirúrgica de Corrección (`TASK-F7-003`)

### 5.1 Actualización de `include/mjk_apu.h` y `src/core/mjk_apu.c`
1. **Ampliar `apu->channels[8]`**:
   - Cambiar `MjkSoundChannel channels[4]` a `MjkSoundChannel channels[8]` en `MjkApuState` ([`include/mjk_apu.h:69`](file:///c:/Users/kanel/Documents/mjk/include/mjk_apu.h#L69)), donde `0..3` son los canales BGM y `4..7` son los canales SFX.
   - Añadir `uint8_t sweep_divider;` a `MjkSoundChannel` para el divisor de hardware sweep.
2. **Implementar Pitch Bend por Software (`$D8..$DF`) y Hardware Sweep (`$F5`) en `Channel_Tick` (`src/core/mjk_apu.c`)**:
   - En `case 5` (`0xD0..0xDF`, [`src/core/mjk_apu.c:474-482`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L474-L482)): cuando `(b & 0x0F) >= 8`, obtener `int8_t step = k_pitch_bend_steps[b & 3]`. Solo si `step != ch->pitch_bend_step`, asignar `ch->pitch_bend_step = step` y resetear `ch->pitch_bend_acc = 0` (fiel a `asm/src/music-engine.asm:395-400`).
   - En la emisión de nota (`b < 0x80`, [`src/core/mjk_apu.c:420-430`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_apu.c#L420-L430)): si `ch->pitch_bend_step != 0` y `(b & 0x0F) != 0x0C`, incrementar `ch->pitch_bend_acc` e iterar `ch->pitch_bend_acc` veces aplicando la aritmética exacta de `loc_B214` (`asm/src/music-engine.asm:181-218`) antes de extraer `semitone` y `octave`.
   - En cada tick de frame (`Channel_Tick`), antes o después de evaluar duración, si el canal es Pulse (`0, 1, 4, 5`) y `(ch->sweep_reg & 0x80) != 0` y `(ch->sweep_reg & 0x07) != 0` y `ch->period >= 8`:
     - Aplicar el desplazamiento de hardware sweep 2A03: `uint16_t delta = ch->period >> (ch->sweep_reg & 7);`
     - Si `ch->sweep_reg & 0x08` (Negate): `ch->period = (ch->period > delta) ? (ch->period - delta) : 0;`
     - Si no: `ch->period += delta;`
3. **Ejecutar los SFX en `channels[4..7]` mediante Bytecode Canónico (`MjkAPU_PlaySfx` y `MjkAPU_UpdateFrame`)**:
   - En `MjkAPU_PlaySfx`, inicializar los canales SFX correspondientes (`channels[4..7]`, o `channels[0]` en `SFX_PLAYER_DIE` apagando el resto) con las secuencias de bytecode auténticas de `asm/src/music-data.asm`.
   - En `MjkAPU_UpdateFrame`:
     - Avanzar `Channel_Tick(&apu->channels[i])` para `i = 0..7`.
     - Para cada una de las 4 voces de hardware (`v = 0..3`: Pulse 1, Pulse 2, Triangle, Noise), seleccionar el canal SFX `apu->channels[v + 4]` si `apu->channels[v + 4].status != 0`; de lo contrario, seleccionar el canal BGM `apu->channels[v]` (si `apu->music_enabled && apu->channels[v].status != 0`).
     - Eliminar por completo la resta ad-hoc `apu->p1_period -= 14` y el encendido espurio de ruido blanco (`apu->noise_enabled = true`).
     - Actualizar `apu->sfx_frames_left` y `apu->active_sfx` reflejando si alguno de los canales SFX (`channels[4..7]`) sigue activo (`status != 0`), manteniendo 100% de compatibilidad con `tests/test_apu.c`.

### 5.2 Corrección de Disparadores SFX en Gameplay (`src/core/`)
1. **Recogida de Bombas (`MjkRoom_CollectBombCanonical`, [`src/core/mjk_room_decoder.c:431-512`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c#L431-L512))**:
   - Reproducir `SFX_FIRE_BOMB` (`Sound_BombCollectedLit` `$08`) cuando `val & MJK_BOMB_FLAG_FUSE_LIT`, o `SFX_COLLECT_BOMB` (`Sound_BombCollectedUnlit` `$07`) cuando está apagada (`asm/src/prg.asm:5003-5006`).
   - Cuando se alcanza el umbral de 20 puntos de bombas (`game->power_coin_bomb_counter >= MJK_POWER_COIN_THRESHOLD`, líneas 438-445), reproducir `SFX_BOMB_CHEST_BONUS` (`$19`) en salas normales o `SFX_POWER_COIN` (`$05`) en Bomb Rooms (`asm/src/prg.asm:4989-4998`).
   - Cuando se otorga el bono de 10.000 puntos por recoger todas las bombas en orden (líneas 505-510), reproducir `SFX_BOMB_BONUS` (`$20`, `asm/src/prg.asm:5056-5057`).
2. **Puertas y Transiciones (`src/core/mjk_state_machine.c` y `src/core/mjk_room_decoder.c`)**:
   - En `State_4_EnterRoom` ([`src/core/mjk_state_machine.c:389, 401`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L389-L401)) y `MjkRoom_HandleOpenDoorMaybe` ([`src/core/mjk_room_decoder.c:239-271`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_room_decoder.c#L239-L271)): cambiar `SFX_OPEN_CHEST` por `SFX_DOOR_OPEN` (`Sound_Door` `$0F`, `asm/src/prg.asm:7041, 7230`).
   - En `State_9_ExitRoom` ([`src/core/mjk_state_machine.c:694`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L694)): silenciar la música (`MjkAPU_StopMusic(&game->apu)`) como hace `JTPickup_Door` (`LDA #0; JSR QueueSound` en `asm/src/prg.asm:5329-5330`) en lugar de disparar `SFX_OPEN_CHEST`.
   - En `State_7_TortureRoom` ([`src/core/mjk_state_machine.c:651-655`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_state_machine.c#L651-L655)): disparar `SFX_JUMP` y decrementar `torture_jumps_left` solo cuando el salto se inicia desde el suelo (`prg.asm:743, 1422-1430`).
3. **Cofres, Bloques Secretos e Ítems (`src/core/mjk_game.c`)**:
   - En `MjkGame_OpenChest` ([`src/core/mjk_game.c:100, 108`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L100-L108)): eliminar la llamada espuria a `SFX_OPEN_CHEST` al revelar o abrir un cofre normal/naranja (`asm/src/prg.asm:5392-5431` no emite sonido al abrir cofres).
   - En `MjkGame_HitSecretBlock` ([`src/core/mjk_game.c:185`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L185)): usar el bytecode de `Sound_SecretBlockRevealed` (`$0E`, `asm/src/prg.asm:5455-5456`).
   - En `MjkGame_CollectChestItem` ([`src/core/mjk_game.c:211-244`](file:///c:/Users/kanel/Documents/mjk/src/core/mjk_game.c#L211-L244)): mapear bolsas de 100/300/1000 (`0, 1, 2`) a `Sound_BombCollectedLit` (`$08`), Gold Coin (`3`) y Mighty Coin (`8`) a `Sound_CoinCollected` (`$04`), Mighty Drink (`4`) a `Sound_TimePickup` (`$21`), Sphinx (`6`) a `Sound_Door` (`$0F`), Tecmo Plate (`7`) a `Sound_1UP_Captive` (`$0A`), Crystal Ball (`9, 10`) a `Sound_CrystalBall` (`$0B`), Royal Family (`11, 12, 13`) a `Sound_Door` (`$0F`) y Beelzebut (`14`) a `Music_CollectedFullFamily` (`$23`).

### 5.3 Corrección del Rango de Audio en `tools/extract_assets.py`
- En [`tools/extract_assets.py:69-71`](file:///c:/Users/kanel/Documents/mjk/tools/extract_assets.py#L69-L71), ampliar `audio_end` de `0xFD00 - 0x8000` a `0xFFFA - 0x8000` (o `0xFFA0 - 0x8000`) para cubrir la totalidad de `asm/src/music-data.asm` (`$F39E..$FFA0`) sin truncar las pistas y efectos situados entre `$FD00` y `$FFA0`.
