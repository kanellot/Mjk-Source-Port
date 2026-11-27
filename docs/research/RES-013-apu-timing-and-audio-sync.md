# RES-013: Secuencia de Inicialización APU 2A03, Sincronización Temporal de Audio y Control de Volumen

- **ID de Investigación**: `RES-013`
- **Área Temática**: 06 - Motor de Audio / APU 2A03
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-25
- **Agente Autor**: asm_researcher (Nivel 3)
- **Fuentes Analizadas**: [`asm/src/prg.asm`](../../asm/src/prg.asm), [`asm/src/music-engine.asm`](../../asm/src/music-engine.asm), [`asm/src/music-data.asm`](../../asm/src/music-data.asm), [`asm/src/defs.asm`](../../asm/src/defs.asm), [`asm/src/ram.asm`](../../asm/src/ram.asm), [`asm/src/registers.asm`](../../asm/src/registers.asm), [`asm/src/data.asm`](../../asm/src/data.asm), [`asm/src/roomdata-us.asm`](../../asm/src/roomdata-us.asm).

---

## 1. Resumen Ejecutivo y Hallazgos Principales

1. **Inicialización de Hardware APU ($4000-$4017)**:
   - En el arranque (`RESET`, [`asm/src/prg.asm:8`](../../asm/src/prg.asm#L8)), el código ejecuta `SEI` ([`prg.asm:9`](../../asm/src/prg.asm#L9)), inicializa la pila e inicializa a cero la RAM completa de `$0000` a `$07FF` ([`prg.asm:47-60`](../../asm/src/prg.asm#L47-L60)).
   - Inicializa el registro de muestra directa DMC (`$4011`) con `#0`: `LDA #0; STA DMC_RAW` ([`prg.asm:65-66`](../../asm/src/prg.asm#L65-L66)).
   - Habilita los 4 canales estándar (Pulse 1, Pulse 2, Triangle, Noise) y deshabilita DMC escribiendo `#$0F` en `$4015`: `LDA #$F; STA SND_CHN` ([`prg.asm:67-68`](../../asm/src/prg.asm#L67-L68)).
   - **Registro `$4017` (Frame Counter)**: **NUNCA se escribe en todo el código del juego**. La interrupción APU Frame IRQ queda inhibida de forma canónica por el flag `I` de la CPU (`SEI`).
2. **Ciclo del Driver de Sonido (`HandleSound`)**:
   - Se invoca una vez por frame a 60 Hz en la interrupción NMI ([`asm/src/prg.asm:112`](../../asm/src/prg.asm#L112)), inmediatamente después de `MainLogicHandler`.
   - La cola de sonido se ubica en RAM: contador `SoundsQueued` en `$0432` ([`asm/src/ram.asm:1352`](../../asm/src/ram.asm#L1352)) y buffer `SoundQueue` en `$0433..$0451` ([`asm/src/ram.asm:1354`](../../asm/src/ram.asm#L1354)).
   - La subrutina `QueueSound` ([`asm/src/prg.asm:2198-2202`](../../asm/src/prg.asm#L2198-L2202)) incrementa `SoundsQueued` y encola el byte recibido en `SoundQueue-1,X`.
3. **Secuencia Temporal de Audio en Pantallas Iniciales**:
   - **Copyright Screen** (`TitleScreen_US_0`/`1`, [`asm/src/prg.asm:7496-7531`](../../asm/src/prg.asm#L7496-L7531)): **SILENCIO TOTAL**. No se encola ningún comando.
   - **Title Screen** (`TitleScreen_0` a `TitleScreen_3`, [`asm/src/prg.asm:7533-7659`](../../asm/src/prg.asm#L7533-L7659)): **SILENCIO TOTAL**. No hay música de título.
   - **Transición Start -> Inicio**: Al pulsar Start, Jack cae de la pantalla. Al entrar en `GameState_0_Init` ([`asm/src/prg.asm:341`](../../asm/src/prg.asm#L341)), se ejecuta `JSR QueueSound` con `A=0` (`Sound_Silence`), forzando el silenciamiento y puesta a cero de los 8 canales.
   - **Round Intro** (`GameState_1_RoundIntro`, [`asm/src/prg.asm:7663-7700`](../../asm/src/prg.asm#L7663-L7700)): Pantalla "Jack x 3" durante 96 frames (`$60`). **SILENCIO TOTAL**.
   - **Carga y Dibujo de Sala** (`GameState_2_LoadRoom` y `GameState_3_DrawScreen`, [`asm/src/prg.asm:3130`](../../asm/src/prg.asm#L3130) y [`379`](../../asm/src/prg.asm#L379)): **SILENCIO TOTAL**. `GameState_2` lee el tipo de música del encabezado de sala (`RoomData_01` byte 1 = `0`, guardado en `RoomMusic = 0` en [`prg.asm:3156`](../../asm/src/prg.asm#L3156)), pero no arranca sonido alguno.
   - **Inicio de Música de Sala (Round 1 Enter Room)**: Ocurre al finalizar la animación de entrada por puerta en `GameState_4_EnterRoom` en la etiqueta `loc_83E6` ([`asm/src/prg.asm:609`](../../asm/src/prg.asm#L609)).
     - Línea 619: `LDX a:RoomMusic` (X = 0).
     - Línea 620: `LDA MusicOptionsTable,X` (carga `MusicOptionsTable[0]` = `Music_MainWithIntro` = `$16` / 22 decimal).
     - **Línea 621**: **`JSR QueueSound`** encola el comando `$16`.
     - En el siguiente frame, `HandleSound` arranca `SoundData_16_Music_MainIntro` en los canales 0, 1 y 2.

---

## 2. Inicialización de Hardware APU ($4000-$4017)

### 2.1 Secuencia de Arranque en `RESET` ([`asm/src/prg.asm:8-81`](../../asm/src/prg.asm#L8-L81))

```asm
; asm/src/prg.asm:8-23
RESET:
    SEI                     ; Línea 9: Deshabilita IRQ CPU
    CLD                     ; Línea 10: Limpia modo decimal
    LDA #$10
    STA PPUCTRL             ; Línea 12
loc_8007:
    LDA PPUSTATUS
    BPL loc_8007            ; Línea 16: Espera primer vblank
loc_800C:
    LDA PPUSTATUS
    BPL loc_800C            ; Línea 20: Espera segundo vblank
    LDA #6
    STA PPUMASK             ; Línea 22
```

A continuación, tras verificar el test de protección de copia (`_CopyProtectionCheck`), inicializa el stack pointer (`TXS`) y efectúa el volcado a cero de toda la memoria RAM:

```asm
; asm/src/prg.asm:45-60
    LDX #$FF                ; Línea 45
    TXS                     ; Línea 46: Pila en $01FF
    LDY #0                  ; Línea 47
    TYA                     ; Línea 48: A = 0
    DEY                     ; Línea 49: Y = $FF
    LDX #7                  ; Línea 50: Página 7 ($0700)
    STXc byte_1             ; Línea 51
    STYc byte_0             ; Línea 52

loc_8041:
    STA (byte_0),Y          ; Línea 55: Escribe $00 en toda la RAM ($0000-$07FF)
    DEY                     ; Línea 56
    BNE loc_8041            ; Línea 57
    DEX                     ; Línea 58
    STXc byte_1             ; Línea 59
    BPL loc_8041            ; Línea 60
```

> [!IMPORTANT]
> Esta limpieza a cero (`$0000-$07FF`) inicializa formalmente a cero los 8 canales virtuales de sonido (`$0452..$0511`), los registros espejo APU (`$07B0..$07EB`), y la cola de sonido (`$0432..$0451`).

Seguidamente, `RESET` configura la APU directamente en las líneas 65 a 68:

```asm
; asm/src/prg.asm:65-68
    LDA #0
    STA DMC_RAW             ; Línea 66: $4011 = $00 (DMC DAC = 0)
    LDA #$F
    STA SND_CHN             ; Línea 68: $4015 = $0F (habilita Pulse 1, Pulse 2, Triangle, Noise)
```

### 2.2 Tratamiento del Registro $4017 (APU Frame Counter)

En el desmontaje completo de Mighty Bomb Jack:
- El símbolo `JOY2 = $4017` se encuentra definido en [`asm/src/registers.asm:53`](../../asm/src/registers.asm#L53) y [`asm/src/ram.asm:2445`](../../asm/src/ram.asm#L2445).
- **No existe ninguna instrucción de escritura (`STA $4017` o `STA JOY2`) en toda la ROM**.
- El hardware de la NES arranca el frame counter en modo 4-step sequence con interrupción APU Frame IRQ armada por defecto. Mighty Bomb Jack enmascara las IRQs de CPU de forma permanente con `SEI` ([`prg.asm:9`](../../asm/src/prg.asm#L9)), y no define ningún vector ni controlador para IRQ aparte de compartir la dirección con NMI ([`prg.asm:90-91`](../../asm/src/prg.asm#L90-L91)).

---

## 3. Cola de Sonido y Arquitectura de `HandleSound`

### 3.1 Estructuras de Memoria RAM

| Dirección RAM | Etiqueta | Tamaño | Descripción Técnica |
| :--- | :--- | :---: | :--- |
| `$0432` | `SoundsQueued` | 1 byte | Contador de comandos pendientes en la cola ([`asm/src/ram.asm:1352`](../../asm/src/ram.asm#L1352)). |
| `$0433..$0451` | `SoundQueue` | 31 bytes | Buffer circular/FIFO de IDs de sonido solicitados ([`asm/src/ram.asm:1354`](../../asm/src/ram.asm#L1354)). |
| `$0452..$04B1` | `SoundChannelA_Mus0..3` | 4 × 24 bytes | 4 canales virtuales de BGM (Pulse 1, Pulse 2, Triangle, Noise). |
| `$04B2..$0511` | `SoundChannelA_Sfx0..3` | 4 × 24 bytes | 4 canales virtuales de SFX (Pulse 1, Pulse 2, Triangle, Noise). |
| `$07B0..$07EB` | `SoundReg_*` | 16 bytes | Registros espejo locales de APU (`SoundReg_Vol`, `SoundReg_Sweep`, `SoundReg_Lo`, `SoundReg_Hi`). |

### 3.2 Subrutina de Encolado: `QueueSound` ([`asm/src/prg.asm:2198-2202`](../../asm/src/prg.asm#L2198-L2202))

```asm
QueueSound:
    INC SoundsQueued            ; Línea 2199: Incrementa contador
    LDX SoundsQueued            ; Línea 2200: X = nuevo contador (1-indexed)
    STA SoundQueue-1,X          ; Línea 2201: Guarda el ID en SoundQueue[X-1]
    RTS                         ; Línea 2202
```

### 3.3 Procesamiento de Cola: `HandleSound` y `HandleSoundQueue`

En cada frame a 60 Hz, la NMI llama a `HandleSound` ([`asm/src/prg.asm:112`](../../asm/src/prg.asm#L112)):

```asm
; asm/src/music-engine.asm:2-13
HandleSound:
    LDA SoundsQueued            ; Línea 3: ¿Hay sonidos en cola?
    BEQ loc_B110                ; Línea 4: Si no, salta a ejecutar canales
    LDA #0
    STA byte_0                  ; Línea 6: Índice de procesamiento = 0

loc_B105:
    JSR HandleSoundQueue        ; Línea 9: Procesa comando en SoundQueue[byte_0]
    INCc byte_0                 ; Línea 10: Avanza al siguiente elemento
    DEC SoundsQueued            ; Línea 11: Decrementa pendientes
    BNE loc_B105                ; Línea 12: Repite hasta vaciar la cola
```

`HandleSoundQueue` ([`asm/src/music-engine.asm:47-130`](../../asm/src/music-engine.asm#L47-L130)) evalúa el comando:

```asm
; asm/src/music-engine.asm:47-68
HandleSoundQueue:
    LDX byte_0                  ; Línea 48
    LDA SoundQueue,X            ; Línea 49: Carga el ID del comando
    BNE loc_B165                ; Línea 50: Si es > 0, es un sonido/música

    ; --- Comando 0: SILENCIO TOTAL ---
    LDX #0                      ; Línea 51
loc_B14D:
    TXA
    ASL A
    TAY
    LDA SoundPointersA,Y        ; Puntero al canal X ($0452 + X * 24)
    STA SoundEnginePointer
    LDA SoundPointersA+1,Y
    STA SoundEnginePointer+1
    LDY #0
    TYA
    STA (SoundEnginePointer),Y  ; Línea 63: status = 0 (apaga canal)
    INX
    CPX #8
    BNE loc_B14D                ; Línea 67: Repite para los 8 canales
    RTS
```

Si el comando es mayor que 0 ([`asm/src/music-engine.asm:71-130`](../../asm/src/music-engine.asm#L71-L130)):
- Decrementa el valor de la cola: `DEC SoundQueue,X` (conversión de 1-indexed a 0-indexed).
- Multiplica por 2 (`ASL A`) e indexa en `SoundPointers` ([`asm/src/music-data.asm:133-170`](../../asm/src/music-data.asm#L133-L170)).
- Recorre la lista de cabeceras de pista terminada en `$FF`: cada entrada asigna un canal virtual (`0..7`), inicializa su estructura de 24 bytes, fija `duration = 1`, `volume_delay = 1` y enlaza el flujo de bytecode musical.

---

## 4. Catálogo Canónico de Comandos de Sonido y Punteros

Definidos en [`asm/src/defs.asm:84-121`](../../asm/src/defs.asm#L84-L121) y enlazados en [`asm/src/music-data.asm:133-170`](../../asm/src/music-data.asm#L133-L170):

| ID Byte (Hex) | ID Dec | Nombre Simbólico (`defs.asm`) | Canales Virtuales | Puntero de Datos (`music-data.asm`) | Función en el Juego |
| :---: | :---: | :--- | :---: | :--- | :--- |
| **`$00`** | `0` | `Sound_Silence` | Todos (0..7) | N/A (Tratamiento especial en `loc_B14D`) | Apaga y silencia los 8 canales de sonido. |
| **`$01`** | `1` | `Sound_Jump` | 5 | `SoundData_01_Jump` ([`L174`](../../asm/src/music-data.asm#L174)) | SFX de salto de Jack. |
| **`$02`** | `2` | `Sound_Death` | 0, 1, 2, 4, 5, 6 | `SoundData_02_Death` ([`L189`](../../asm/src/music-data.asm#L189)) | Secuencia de muerte de Jack. |
| **`$03`** | `3` | `Sound_BonusCoinSpawn` | 5 | `SoundData_03_BonusCoinSpawn` ([`L245`](../../asm/src/music-data.asm#L245)) | Aparición de moneda bonus. |
| **`$04`** | `4` | `Sound_CoinCollected` | 5 | `SoundData_04_CoinCollected` ([`L258`](../../asm/src/music-data.asm#L258)) | Recolección de moneda dorada. |
| **`$05`** | `5` | `Sound_PowerCoinSpawn` | 5 | `SoundData_05_PowerCoinSpawn` ([`L271`](../../asm/src/music-data.asm#L271)) | Aparición de Power Coin (P). |
| **`$06`** | `6` | `Sound_PowerCoinMusic` | 0, 1, 2 | `SoundData_06_PowerCoinMusic` ([`L284`](../../asm/src/music-data.asm#L284)) | Música durante el estado de enemigos convertidos en monedas. |
| **`$07`** | `7` | `Sound_BombCollectedUnlit`| 6 | `SoundData_07_GetNormalBomb` ([`L332`](../../asm/src/music-data.asm#L332)) | Bomba recogida apagada (100 pts). |
| **`$08`** | `8` | `Sound_BombCollectedLit` | 6 | `SoundData_08_GetFireBomb` ([`L349`](../../asm/src/music-data.asm#L349)) | Bomba recogida con mecha encendida (200 pts). |
| **`$09`** | `9` | `Sound_YouAreGreedy` | 0, 1, 2 | `SoundData_09_YouAreGreedy` ([`L366`](../../asm/src/music-data.asm#L366)) | Fanfarria del cartel "YOU ARE GREEDY". |
| **`$0A`** | `10` | `Sound_1UP_Captive` | 4, 5 | `SoundData_0A_Get1UP` ([`L455`](../../asm/src/music-data.asm#L455)) | Vida extra obtenida o cautivo rescatado. |
| **`$0B`** | `11` | `Sound_CrystalBall` | 4, 5 | `SoundData_0B_GetCrystalBall` ([`L484`](../../asm/src/music-data.asm#L484)) | Recolección de bola de cristal. |
| **`$0C`** | `12` | `Sound_C_Null` | — | `SoundData_Null` ([`L171`](../../asm/src/music-data.asm#L171)) | Nulo (inactivo). |
| **`$0D`** | `13` | `Sound_BonusStatueCollected`| 4, 5 | `SoundData_0D_GetBonusStatue` ([`L513`](../../asm/src/music-data.asm#L513)) | Recolección de estatua faraónica. |
| **`$0E`** | `14` | `Sound_SecretBlockRevealed`| 4, 5 | `SoundData_0E_SecretBlockOpened` ([`L542`](../../asm/src/music-data.asm#L542))| Apertura de cofre secreto multi-golpe. |
| **`$0F`** | `15` | `Sound_Door` | 7 | `SoundData_0F_Door` ([`L571`](../../asm/src/music-data.asm#L571)) | Apertura o cierre mecánico de puerta (canal ruido). |
| **`$10`** | `16` | `Music_Main` | 0, 1, 2 | `SoundData_10_Music_Main` ([`L595`](../../asm/src/music-data.asm#L595)) | BGM principal del juego (sin intro). |
| **`$11`** | `17` | `Music_TreasureRoom` | 0, 1, 2 | `SoundData_11_Music_Treasure` ([`L1000`](../../asm/src/music-data.asm#L1000)) | BGM de sala del tesoro. |
| **`$12`** | `18` | `Music_SideRoom` | 0, 1, 2 | `SoundData_12_Music_Side` ([`L1233`](../../asm/src/music-data.asm#L1233)) | BGM de pasillos secundarios. |
| **`$13`** | `19` | `Music_Labyrinth` | 0, 1, 2 | `SoundData_13_Music_Labyrinth` ([`L1414`](../../asm/src/music-data.asm#L1414))| BGM de laberinto piramidal. |
| **`$14`** | `20` | `Sound_14_Null` | — | `SoundData_Null` ([`L171`](../../asm/src/music-data.asm#L171)) | Nulo (inactivo). |
| **`$15`** | `21` | `Music_Outside1` | 0, 1, 2 | `SoundData_15_Music_Outside` ([`L1506`](../../asm/src/music-data.asm#L1506)) | BGM exterior (Ronda 9 / 13). |
| **`$16`** | `22` | `Music_MainWithIntro` | 0, 1, 2 | `SoundData_16_Music_MainIntro` ([`L1650`](../../asm/src/music-data.asm#L1650))| **BGM principal precedida por la fanfarria de inicio de ronda**. |
| **`$17`** | `23` | `Music_GameOver` | 0, 1, 2 | `SoundData_17_Music_GameOver` ([`L1739`](../../asm/src/music-data.asm#L1739)) | Fanfarria fúnebre de Game Over. |
| **`$18`** | `24` | `Music_RoundClear` | 0, 1, 2 | `SoundData_18_Music_RoundClear` ([`L1804`](../../asm/src/music-data.asm#L1804))| Fanfarria de victoria fin de ronda. |
| **`$19`** | `25` | `Sound_BombChestBonus` | 4, 5 | `SoundData_19_BombChestBonus` ([`L1867`](../../asm/src/music-data.asm#L1867))| Notificación de cofre con Power Coin. |
| **`$1A`** | `26` | `Sound_1A` | 4, 5 | `SoundData_1A` ([`L1892`](../../asm/src/music-data.asm#L1892)) | Jingle de transición especial. |
| **`$1B`** | `27` | `Sound_Pause` | 4, 5, 6 | `SoundData_1B_Pause` ([`L1909`](../../asm/src/music-data.asm#L1909)) | Silenciador por bucle SFX de pausa. |
| **`$1C`** | `28` | `Sound_Unpause` | 4, 5, 6 | `SoundData_1C_Unpause` ([`L1944`](../../asm/src/music-data.asm#L1944)) | Desactivador de silencio por pausa. |
| **`$1D`** | `29` | `Sound_UsedMightyCoin` | 4, 5, 6 | `SoundData_1D_UseMightyCoin` ([`L1977`](../../asm/src/music-data.asm#L1977))| Activación de Mighty Coin (cambio de color). |
| **`$1E`** | `30` | `Music_Outside2` | 0, 1, 2 | `SoundData_15_Music_Outside` ([`L1506`](../../asm/src/music-data.asm#L1506)) | Alias idéntico a `Music_Outside1`. |
| **`$1F`** | `31` | `Music_TortureRoom` | 0, 1, 2 | `SoundData_1F_Music_Torture` ([`L2066`](../../asm/src/music-data.asm#L2066)) | BGM de la Sala de Tortura. |
| **`$20`** | `32` | `Sound_BombBonus` | 4, 5 | `SoundData_20_BombBonus` ([`L2162`](../../asm/src/music-data.asm#L2162)) | Bono de 10.000 / 50.000 pts por todas las bombas. |
| **`$21`** | `33` | `Sound_TimePickup` | 5 | `SoundData_21_GetMightyDrink` ([`L2207`](../../asm/src/music-data.asm#L2207))| Recolección de Bebida Mighty (tiempo extra). |
| **`$22`** | `34` | `Music_Ending` | 0, 1, 2 | `SoundData_22_Music_Ending` ([`L2232`](../../asm/src/music-data.asm#L2232)) | Tema de créditos de final estándar. |
| **`$23`** | `35` | `Music_CollectedFullFamily`| 0, 1, 2 | `SoundData_23_Music_BestEnding` ([`L2453`](../../asm/src/music-data.asm#L2453))| Fanfarria al rescatar a la familia real completa. |

---

## 5. Secuencia Temporal de Audio en Pantallas Iniciales

```mermaid
sequenceDiagram
    autonumber
    participant RESET as Boot / RESET
    participant Title as Copyright & Title
    participant GS0 as GameState_0_Init
    participant GS1 as GameState_1_RoundIntro
    participant GS2 as GameState_2_LoadRoom
    participant GS3 as GameState_3_DrawScreen
    participant GS4 as GameState_4_EnterRoom
    participant APU as 2A03 APU Hardware

    RESET->>APU: $4011=0, $4015=$0F (Líneas 66-68)
    Note over Title: Pantalla Copyright y Título (SILENCIO TOTAL)
    Title->>GS0: Start pulsado -> Jack cae
    GS0->>APU: QueueSound(0) [Línea 341] -> Mutea todos los canales
    Note over GS1: Jack x 3 por 96 frames (SILENCIO TOTAL)
    Note over GS2: Lee RoomData_01 (RoomMusic = 0) [Línea 3156] (SILENCIO)
    Note over GS3: Renderiza tiles y sprites de la sala (SILENCIO)
    Note over GS4: Animación apertura/cierre de puerta (~24 frames)
    GS4->>APU: QueueSound(MusicOptionsTable[0] = $16) [Línea 621]
    Note over APU: Inicia Music_MainWithIntro (Fanfarria inicial)
```

### 5.1 Pantalla de Copyright (`TitleScreen_US_0` y `TitleScreen_US_1`)
- **Líneas en `prg.asm`**: [`7496-7531`](../../asm/src/prg.asm#L7496-L7531).
- **Comportamiento**: Escribe texto de copyright de Tecmo en la PPU y espera temporizador o pulsación de Start.
- **Audio**: **Completamente en silencio**. No invoca `QueueSound` ni toca variables de sonido.

### 5.2 Pantalla de Título (`TitleScreen_0`, `TitleScreen_1`, `TitleScreen_2`, `TitleScreen_3`)
- **Líneas en `prg.asm`**: [`7533-7659`](../../asm/src/prg.asm#L7533-L7659).
- **Comportamiento**: Dibuja nametable de título, texto "PUSH START BUTTON", puntuaciones máximas y sprite de Jack. Cuando el jugador pulsa Start (`loc_ADC0`, [`prg.asm:7629`](../../asm/src/prg.asm#L7629)), Jack cae por la parte inferior de la pantalla hasta `PlayerYPosHi >= $D8` (`TitleScreen_3`, [`prg.asm:7648`](../../asm/src/prg.asm#L7648)).
- **Audio**: **Completamente en silencio**. No existe música de título en Mighty Bomb Jack.

### 5.3 Inicialización del Juego (`GameState_0_Init`)
- **Líneas en `prg.asm`**: [`323-361`](../../asm/src/prg.asm#L323-L361).
- **Línea de Encolado**: [`asm/src/prg.asm:341`](../../asm/src/prg.asm#L341):
  ```asm
  LDA #0
  STA PlayerMightyCoins    ; Línea 340
  JSR QueueSound           ; Línea 341: Encola ID #0 (Sound_Silence)
  ```
- **Efecto**: Garantiza que cualquier residuo sonoro previo quede formalmente apagado (`ActiveSoundChannels = 0`).

### 5.4 Introducción de Ronda (`GameState_1_RoundIntro`)
- **Líneas en `prg.asm`**: [`7663-7700`](../../asm/src/prg.asm#L7663-L7700).
- **Comportamiento**: Muestra el sprite de Jack y el contador de vidas (`GameState1WaitTimer = $60`, 96 frames a 60 Hz = 1.6 segundos).
- **Audio**: **Completamente en silencio**. No se encola sonido.

### 5.5 Carga y Renderizado de Sala (`GameState_2_LoadRoom` y `GameState_3_DrawScreen`)
- **Líneas en `prg.asm`**: [`3130-3251`](../../asm/src/prg.asm#L3130-L3251) y [`379-441`](../../asm/src/prg.asm#L379-L441).
- En `GameState_2`, se lee el segundo byte del bloque de sala:
  - Para Ronda 1 (`RoomData_01`, [`asm/src/roomdata-us.asm:5`](../../asm/src/roomdata-us.asm#L5)): `.BYTE $E0, 0, $81...`
  - Byte 1 = `0`.
  - Líneas 3154-3156:
    ```asm
    INY
    LDA (off_58),Y          ; Segundo byte = 0
    AND #7
    STA a:RoomMusic         ; RoomMusic = 0
    ```
- **Audio**: **Completamente en silencio**. No se encola música durante la carga ni durante el volcado PPU de `GameState_3`.

### 5.6 Entrada a Sala 1 y Arranque de Música (`GameState_4_EnterRoom`)
- **Líneas en `prg.asm`**: [`445-623`](../../asm/src/prg.asm#L445-L623).
- Jack realiza la animación de entrada por la puerta (`AnimateDoorOpen`, desplazamiento, `AnimateDoorClose`).
- Cuando la animación de puerta concluye y Jack toma el control interactivo:

```asm
; asm/src/prg.asm:609-623
loc_83E6:
    LDX #0
    STX byte_340
    STX CollisionFlag       ; Línea 612
    INX
    STX PlayerStruct        ; Línea 614
    INCc GameState          ; Línea 615: GameState avanza de 4 a 5 (Main Game)
    LDA a:MaybePauseFlag
    ORA #$40
    STA a:MaybePauseFlag    ; Línea 618
    LDX a:RoomMusic         ; Línea 619: X = RoomMusic (= 0 para Ronda 1)
    LDA MusicOptionsTable,X ; Línea 620: Carga MusicOptionsTable[0]
    JSR QueueSound          ; Línea 621: ¡ENCOLA EL COMANDO DE MÚSICA!
    RTS                     ; Línea 622
```

Consultando la tabla [`asm/src/data.asm:312-320`](../../asm/src/data.asm#L312-L320):
```asm
MusicOptionsTable:
    .BYTE Music_MainWithIntro   ; Índice 0 ($16 / 22 decimal)
    .BYTE Music_Main            ; Índice 1 ($10 / 16 decimal)
    .BYTE Music_TreasureRoom    ; Índice 2 ($11 / 17 decimal)
    .BYTE Music_SideRoom        ; Índice 3 ($12 / 18 decimal)
    .BYTE Music_Labyrinth       ; Índice 4 ($13 / 19 decimal)
    .BYTE Music_Outside1        ; Índice 5 ($15 / 21 decimal)
    .BYTE Music_Outside2        ; Índice 6 ($1E / 30 decimal)
    .BYTE Music_TortureRoom     ; Índice 7 ($1F / 31 decimal)
```

> [!IMPORTANT]
> **Cita de Ground Truth Exacta**:
> - **Línea exacta en `prg.asm`**: [`asm/src/prg.asm:621`](../../asm/src/prg.asm#L621).
> - **Byte / Comando exacto encolado**: `$16` (`Music_MainWithIntro`, 22 decimal).
> - En el frame NMI inmediatamente posterior ([`asm/src/prg.asm:112`](../../asm/src/prg.asm#L112)), `HandleSound` consume `$16` y lanza `SoundData_16_Music_MainIntro` ([`asm/src/music-data.asm:1650`](../../asm/src/music-data.asm#L1650)), que reproduce la fanfarria de inicio e inmediatamente salta mediante `SndJmpF0` ([`music-data.asm:1679`](../../asm/src/music-data.asm#L1679)) al tema cíclico principal `SoundTrack_10_0` (`Music_Main`).

---

## 6. Comandos de Control de Volumen y Silenciamiento Canónicos

El motor de audio de Mighty Bomb Jack implementa un conjunto jerárquico de mecanismos para controlar volumen, silencios y prioridades:

### 6.1 Silenciamiento Maestro Global (`Sound_Silence`, Comando `$00`)
- **Invocación**: `LDA #0; JSR QueueSound`.
- **Rutina**: [`asm/src/music-engine.asm:47-68`](../../asm/src/music-engine.asm#L47-L68).
- **Efecto Técnico**:
  - Escribe `#$00` en el byte de estado (`offset $00`) de los 8 canales virtuales (`SoundPointersA[0..7]`).
  - Al procesar el volcado de hardware en `WriteSoundRegisters` ([`music-engine.asm:712-720`](../../asm/src/music-engine.asm#L712-L720)):
    ```asm
    LDA a:ActiveSoundChannels   ; = 0
    LSR A; LSR A; LSR A; LSR A
    ORA a:ActiveSoundChannels
    AND #$F
    STA SND_CHN                 ; Escribe 0 en $4015 (apaga los 4 canales de hardware)
    ```

### 6.2 Silencio / Silencio de Nota en Canal (`SoundCommand_Cx`, Bytecode `$C0..$CF`)
- **Implementación**: [`asm/src/music-engine.asm:355-367`](../../asm/src/music-engine.asm#L355-L367).
- **Efecto Técnico**:
  - Escribe `opcode & 0x0F` en la parte baja de frecuencia (`offset $11`) y `#8` en la parte alta (`offset $12`).
  - Llama a `sub_B285` para actualizar registros y efectúa doble retorno (`PLA; PLA; JMP loc_B274`) desapilando el frame de llamada para pasar directamente a esperar la duración de la nota sin reproducir audio.

### 6.3 Asignación de Envolvente de Volumen (`SoundCommand_Ax_Bx`, Bytecode `$A0..$BF`)
- **Implementación**: [`asm/src/music-engine.asm:333-349`](../../asm/src/music-engine.asm#L333-L349).
- **Efecto Técnico**:
  - Toma `idx = opcode & 0x1F`.
  - Carga el puntero de 16 bits desde `SoundVolTable,X` ([`asm/src/music-data.asm:1-125`](../../asm/src/music-data.asm#L1-L125)) en los offsets `$05/$06` del canal virtual.
  - Reinicia el índice de paso de envolvente (`offset $07 = 0`).
  - Llama a `MaybeResetVolDelay` ([`music-engine.asm:646`](../../asm/src/music-engine.asm#L646)), cargando el primer byte de volumen y duración de fase (`offset $0E`).

### 6.4 Atenuación de Volumen por Canal (`SoundCommand_Ex`, Bytecode `$E0..$EF`)
- **Implementación**: [`asm/src/music-engine.asm:410-416`](../../asm/src/music-engine.asm#L410-L416).
- **Efecto Técnico**:
  - Almacena `opcode & 0x0F` en el `offset $0D` del canal virtual.
  - Al evaluar la envolvente en `MaybeResetVolDelay` ([`music-engine.asm:676-685`](../../asm/src/music-engine.asm#L676-L685)), si el bit 4 del byte de envolvente está activo, se resta este nivel de atenuación:
    ```asm
    LDA off_4
    AND #$F
    LDY #$D
    SEC
    SBC (SoundEnginePointer),Y  ; Resta atenuación
    BCS loc_B4AD
    LDA #0                      ; Clampa a 0 si hay desbordamiento negativo
    ```

### 6.5 Configuración de Duty Cycle (`SoundCommand_Dx`, Bytecode `$D0..$D7`)
- **Implementación**: [`asm/src/music-engine.asm:371-387`](../../asm/src/music-engine.asm#L371-L387).
- **Efecto Técnico**:
  - Si el argumento es `< 8`, desplaza bits a las posiciones 6 y 7 del registro de volumen del canal (`offset $0F`), configurando los 4 posibles ciclos de trabajo de los generadores Pulse (12.5%, 25%, 50%, 75%).

### 6.6 Finalización de Pista y Restauración de BGM (`SoundCommand_F6_F7`, Bytecode `$F6`, `$F7`)
- **Implementación**: [`asm/src/music-engine.asm:605-630`](../../asm/src/music-engine.asm#L605-L630).
- **Efecto Técnico**:
  - Pone `status = 0` en el canal activo.
  - Si el canal activo es un canal SFX (`CurrentSoundChannel >= 4`):
    - Localiza el canal hermano de BGM (`CurrentSoundChannel - 4`).
    - Si el canal hermano BGM sigue activo (`status & 1`), ejecuta `SoundCommand_F6_F7_sub1` ([`music-engine.asm:273`](../../asm/src/music-engine.asm#L273)), restaurando inmediatamente sus registros de volumen, sweep y frecuencia en el hardware de la APU.

### 6.7 Mecanismo Canónico de Pausa mediante Loops SFX Silenciosos
- **Pausa (`Sound_Pause = $1B`)**:
  - [`asm/src/music-data.asm:1909-1943`](../../asm/src/music-data.asm#L1909-L1943).
  - Instancia los canales SFX 4, 5 y 6 (Pulse 1, Pulse 2, Triangle). Tras una breve fanfarria de aviso de pausa, entra en bucles infinitos de silencios:
    ```asm
    SoundTrack_1B_4L:
        .BYTE $80, $0C
        SndJmpF0 SoundTrack_1B_4L
    ```
  - Al estar activos los canales SFX 4, 5 y 6, la comprobación `CheckIfSoundChannelIsActive` ([`music-engine.asm:751`](../../asm/src/music-engine.asm#L751)) bloquea la escritura de los canales BGM 0, 1 y 2 en el hardware, silenciando la música sin detener sus contadores lógicos de tempo.
- **Despausa (`Sound_Unpause = $1C`)**:
  - [`asm/src/music-data.asm:1944-1976`](../../asm/src/music-data.asm#L1944-L1976).
  - Emite un jingle de despausa en los canales SFX 4, 5 y 6, y finaliza cada pista con `.BYTE $FF` ([`L1958, L1968, L1976`](../../asm/src/music-data.asm#L1958)).
  - La terminación de los canales SFX dispara la reactivación instantánea de los canales BGM hermanos.

---

## 7. Verificación Cruzada y Trazabilidad

| Pregunta de Investigación | Veredicto 6502 | Cita Fuente Exacta |
| :--- | :--- | :--- |
| ¿Dónde se inicializa APU $4015? | `RESET`, `LDA #$F; STA SND_CHN` | [`asm/src/prg.asm:67-68`](../../asm/src/prg.asm#L67-L68) |
| ¿Dónde se inicializa APU $4017? | **NUNCA**. No se escribe en $4017. IRQ enmascarada por `SEI`. | [`asm/src/prg.asm:9`](../../asm/src/prg.asm#L9) |
| ¿Dónde se limpian los canales al boot? | Bucle `RESET` borra RAM `$0000-$07FF` a cero. | [`asm/src/prg.asm:47-60`](../../asm/src/prg.asm#L47-L60) |
| ¿Dónde se encola silencio explícito? | `GameState_0_Init`, `JSR QueueSound` con `A=0` | [`asm/src/prg.asm:341`](../../asm/src/prg.asm#L341) |
| ¿Suena algo en Copyright Screen? | **No**, silencio total. | [`asm/src/prg.asm:7496-7531`](../../asm/src/prg.asm#L7496-L7531) |
| ¿Suena algo en Title Screen? | **No**, silencio total. | [`asm/src/prg.asm:7533-7659`](../../asm/src/prg.asm#L7533-L7659) |
| ¿Suena algo en Round Intro (GameState_1)? | **No**, silencio total (duración 96 frames). | [`asm/src/prg.asm:7663-7700`](../../asm/src/prg.asm#L7663-L7700) |
| ¿En qué línea exacta arranca la música de Round 1? | `prg.asm:621` en `GameState_4_EnterRoom` tras animación de puerta | [`asm/src/prg.asm:621`](../../asm/src/prg.asm#L621) |
| ¿Cuál es el byte/comando exacto encolado? | `$16` (`Music_MainWithIntro`, 22 dec) | [`asm/src/data.asm:313`](../../asm/src/data.asm#L313), [`defs.asm:107`](../../asm/src/defs.asm#L107) |
