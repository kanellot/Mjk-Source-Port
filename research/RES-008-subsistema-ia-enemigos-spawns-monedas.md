# RES-008: Especificación Funcional C x86 — Subsistema 05: IA de Enemigos, Ciclo de Spawns, Transformación de Momias y Conversión a Monedas

- **ID de Investigación**: `RES-008`
- **Tarea Vinculada**: `TASK-RES-007` (Subpasos `0.7.1` y `0.7.2`)
- **Área Temática**: 04 - Enemigos, Inteligencia Artificial y Spawns
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-22
- **Agente Autor**: Antigravity (Orchestrator)

---

## 1. Objetivo y Alcance

Documentar a nivel de especificación ejecutable 1:1 en C x86 el **Subsistema 05** del desmontaje [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm) (`líneas 8200-10162`) y tablas asociadas en [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm):
1. **Ciclo de Generación (Spawning) y Selección de Punto de Aparición**: `HandleEnemySpawnTimer` (`$B593`), tabla de dificultad escalada por ronda (`DifficultyTable`), límite estricto de 4 enemigos vivos simultáneos (`AliveEnemyCount`) y cálculo geométrico de la posición de spawn (`ChooseEnemySpawnPoint` `$B830`, `SpawnLocFunc1`/`2`).
2. **Ciclo de Vida y Transformación Determinista de Momias**: Estado inicial de Momia (`Mummy`), cuenta atrás de transformación (`EnemyTransformTimer`), y la fórmula determinista pseudo-aleatoria basada en pulsaciones del botón A: $\text{Type} = (\text{APressCounter} \pmod 6) + 2$ (`TransformMummy` `$BC01`).
3. **Máquinas de Estado de los 6 Arquetipos Enemigos**:
   - Tipo 0: Momia terrestre con gravedad y detección de cornisas (`ObjCode_0_Mummy`).
   - Tipo 2: Hanezo (rebotador diagonal elástico) (`ObjCode_2_Hanezo`).
   - Tipo 3: GejiShogun (rastreador de perímetros / oruga) (`ObjCode_3_GejiShogun`).
   - Tipo 4: Gameido (tortuga teledirigida con aproximación suavizada) (`ObjCode_4_Gameido`).
   - Tipo 5: Dokuron (calavera saltadora con seguimiento en X) (`ObjCode_5_Dokuron`).
   - Tipo 6: Desufa (bola de fuego flotante con oscilación senoidal en Y) (`ObjCode_6_Desufa`).
   - Tipo 7: Horus (pájaro cazador con IA de persecución en 4 ejes y evasión de obstáculos) (`ObjCode_7_Horus`).
4. **Mecánica de la Power Coin y Transmutación a Monedas**: `TurnEnemiesIntoCoins` (`$BA23`), temporizador de 320 frames (`EnemyCoinTimer` = `0x0140`, 5.33 s), animación de parpadeo de paleta de advertencia (`DrawEnemies` `$B908`) y recolección con multiplicador en cadena (`JT_98A4_multi`).
5. **Entidades Especiales y de Bonificación**: Monedas de salto/suelo (`ExtraCoin`, `BonusCoin`), Hermano cautivo (`Brother`), globo (`Balloon`) y estatuas de faraón.

---

## 2. Fuentes del Desmontaje Analizadas (`asm/`)

| Archivo en `asm/` | Rango de Líneas | Dirección ROM | Símbolos / Subrutinas Cubiertas |
| :--- | :--- | :--- | :--- |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L8200-L8455) | `8200 - 8455` | `$B593 - $B718` | `HandleEnemySpawnTimer`, `InitEnemy`, `RunEnemyInit`, `LoadDifficultySettings`, `OrEnemyStatusWith08`, `sub_B645` (Scheduler de enemigos en lotes de 4), `MaybeDrawEnemySprites` |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L8458-L8656) | `8458 - 8656` | `$B719 - $B820` | `RunEnemyLogic` (`JumpTable`), `LoadObjectPointerA`, `LoadObjectPointerAPlus1`, `RemoveEnemy`, `sub_B765`, `MaybeRemoveIfOffscreen`, `MaybeConsiderOffscreen`, `CopyEnemyToTempSprite`, `sub_B7B4`, `sub_B7BF` |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L8660-L9112) | `8660 - 9112` | `$B821 - $BA94` | `InitPowerCoin`, `ChooseEnemySpawnPoint`, `SpawnLocFunc1`, `SpawnLocFunc2`, `DrawEnemies`, `SpawnPowerCoin`, `sub_BA00` (Búsqueda de slot libre), `TurnEnemiesIntoCoins`, `sub_BA48`, `sub_BA4E`, `sub_BA7B`, `ResetEnemyVelocity` |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L9128-L9605) | `9128 - 9605` | `$BA9E - $BD18` | `sub_BA9E`, `ObjCode_9_E_ExtraBonusCoin`, `ObjInit_0_Mummy`, `sub_BB1C`, `sub_BB39`, `ObjCode_0_Mummy`, `TransformMummy`, `ObjInit_2_Hanezo`, `ObjCode_2_Hanezo`, `ObjCode_3_GejiShogun`, `ObjInit_4_Gameido`, `ObjCode_4_Gameido`, `ObjInit_5_Dokuron`, `ObjCode_5_Dokuron`, `ObjInit_6_Desufa`, `ObjCode_6_Desufa`, `ObjInit_7_Horus`, `ObjCode_7_Horus`, `ObjInit_8_PowerCoin`, `ObjCode_8_PowerCoin`, `ObjInit_9_E_ExtraBonusCoin`, `ObjInit_C_Balloon`, `ObjInit_D_F_MummyAlt`, `ObjInit_10_Brother`, `ObjCode_10_Brother` |
| [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L9608-L10162) | `9608 - 10162` | `$BD19 - $BFDF` | `ObjCode_4a`, `JustCallBD64`, `sub_BD64`, `JustCallBD81`, `sub_BD81`, `ObjCode_5a`, `sub_BDA0`, `sub_BDC2`, `sub_BDC7`, `sub_BDE9`, `Negate_Mod_Number`, `Store_Mod_Number_ToEnemy`, `sub_BE2F`, `ObjInitCode_3a`, `SetXIfPlayerAndEnemyVarMatch`, `ObjCode_7a`, `sub_BEE7`, `SubtractPlayerAndEnemyVarMaybe`, `sub_BF17`, `sub_BF4E`, `sub_BF85`, `sub_BFBC`, `ResetEitherXOrYVelocity` |
| [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L1-L186) | `1 - 186` | `$C000 - $C0E2` | `SpriteAnimationTable` (animaciones de frames 0..16), `EnemyInitialStatus`, `DifficultyTable` (configuración por ronda), `byte_C0CB` (rango Gameido), `word_C0D3` (velocidades de enemigos 0..7) |
| [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L1963-L2220) | `1963 - 2220` | `$F08F - $F250` | `EnemySpawnPositionTable` (puntos de spawn por sala) |

---

## 3. Catálogo de Subrutinas y Resolución de `sub_XXXX`

| Etiqueta 6502 | Nombre C Propuesto | Dir. ROM | Línea (`prg.asm`) | Inputs / Registros | Función / Contrato |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `HandleEnemySpawnTimer` | `UpdateEnemySpawnTimer` | `$B593` | `8200` | `EnemySpawnTimer`, `AliveEnemyCount` | Si el timer llega a cero y hay `< 4` enemigos vivos, genera una Momia. |
| `InitEnemy` | `InitEnemySlot` | `$B5BA` | `8224` | `A`: `EnemyType` (`0..$10`), `EnemyStructPointer` | Limpia los 28 bytes del slot enemigo, inicializa sprite base `$25` y llama `RunEnemyInit`. |
| `RunEnemyInit` | `DispatchEnemyInit` | `$B5D2` | `8246` | `A`: `EnemyType`, `EnemySpeedModifier` | Carga `EnemyInitialStatus`, resetea offsets de animación y salta por `JumpTable` al inicializador del tipo. |
| `LoadDifficultySettings` | `RefreshDifficultySettings` | `$B60B` | `8290` | `DifficultyModifier` (ronda), `RoomStatusFlags` | Carga velocidad, timer de spawn y timer de transformación según sala normal o del palacio. |
| `OrEnemyStatusWith08` | `TriggerEnemySpawnFlash` | `$B636` | `8313` | `EnemyStructPointer` | Fija `status |= 0x08` y temporizador de parpadeo de aparición `offset $12 = 0x20` (32 frames). |
| `sub_B645` | `RunActiveEnemiesSliced` | `$B645` | `8326` | `byte_BA` (cursor round-robin) | Procesa la lógica de hasta 4 enemigos activos por frame (`DrawEnemies`, `RunEnemyLogic`, colisión con Jack y offscreen). |
| `MaybeDrawEnemySprites` | `BufferEnemySpritesOAM` | `$B683` | `8369` | `byte_56`, `CurrentSpriteIndex` | Escribe los sprites de enemigos en OAM (`$0200`) aplicando multiplexado/parpadeo de sprites si hay más de los soportados. |
| `RunEnemyLogic` | `DispatchEnemyLogic` | `$B719` | `8458` | `EnemyStruct_13_Type`, `EnemyStructPointer` | Despacha la máquina de estados del tipo de enemigo mediante `JumpTable` de 17 entradas. |
| `RemoveEnemy` | `DespawnEnemy` | `$B744` | `8503` | `EnemyStructPointer` | Si el enemigo estaba activo (`status & 1`), lo desactiva y decrementa `AliveEnemyCount` (si tipo `< 8`). |
| `sub_B765` | `LoadEnemySpeedVector` | `$B765` | `8524` | `EnemyStruct_SpeedMod` | Carga la velocidad 8.8 correspondiente de `word_C0D3[speed_idx]` en `Mod_Number`. |
| `MaybeRemoveIfOffscreen` | `CheckEnemyOffscreenDespawn` | `$B78B` | `8538` | `EnemyStruct_6`, `EnemyStruct_C` | Despawnea el enemigo si sobrepasa los márgenes de la pantalla activa. |
| `ChooseEnemySpawnPoint` | `ComputeEnemySpawnCoords` | `$B830` | `8689` | `CurrentRoomIDBackup3`, `EnemySpawnPositionTable` | Lee el par de coordenadas de spawn de la sala y selecciona el punto óptimo según distancia al jugador. |
| `DrawEnemies` | `UpdateEnemyAnimationAndCoinState` | `$B908` | `8828` | `EnemyCoinTimer`, `SpriteAnimationTable` | Si la Power Coin está activa, transmuta al enemigo en moneda dorada giratoria; si no, avanza el ciclo de animación estándar. |
| `sub_BA00` | `FindFreeEnemySlot` | `$BA00` | `8995` | Array de 8 slots de enemigos | Retorna en `EnemyStructPointer` el primer slot inactivo (`status & 1 == 0`). Si no hay ninguno, aborta llamador con `PLA;PLA;RTS`. |
| `TurnEnemiesIntoCoins` | `ActivatePowerCoinEffect` | `$BA23` | `9022` | — | Fija `EnemyCoinTimer = 0x0140` (320 frames), encola música `Sound_PowerCoinMusic` y actualiza la paleta de sprites. |
| `sub_BA48` | `ReverseEnemyHorizontalVel` | `$BA48` | `9052` | `EnemyStructPointer` | Invierte el signo de `XVel` (`offset $02/$04`) mediante `NegateYYXX`. |
| `sub_BA4E` | `HandleEnemyLandingOnFloor` | `$BA4E` | `9060` | `EnemyStruct_0_Status` | Aterriza al enemigo al tocar suelo: limpia bit 1 (`en aire`), anula `YVel` y alinea `YPos` a la rejilla de 8 px. |
| `sub_BA7B` | `NegateEnemyVelocityField` | `$BA7B` | `9092` | `Y`: offset del campo 16-bit | Aplica `NegateYYXX` al vector 8.8 ubicado en `(ESP + Y)` y lo guarda de vuelta. |
| `sub_BA9E` | `UpdateEnemyFacingDirection` | `$BA9E` | `9128` | `EnemyStruct_4_XVelHi` | Fija `offset $14 = 0` (mira a la izquierda si `XVel < 0`) o `4` (mira a la derecha si `XVel >= 0`). |
| `sub_BB1C` | `SetEnemyFallingFromApex` | `$BB1C` | `9224` | `EnemyStructPointer` | Inicia la caída de un enemigo desde el ápice: activa bit 1 de status, fija fase de salto `$18=0, $19=$40` y anula `XVel`. |
| `sub_BB39` | `AimEnemyTowardsPlayerX` | `$BB39` | `9245` | `PlayerXPosHi`, `EnemyStruct_5_XPosHi` | Fija `XVel = +0x0080` (si el jugador está a la derecha) o `-0x0080` (si está a la izquierda). |
| `TransformMummy` | `EvolveMummyDeterministically` | `$BC01` | `9362` | `APressCounter` | Transforma la Momia en un enemigo tipo 2 a 7 según `(APressCounter % 6) + 2`. |
| `sub_BD64` | `BounceEnemyHorizontalWall` | `$BD64` | `9669` | `TempPointer_0C0+1` (bits de colisión) | Si colisiona con pared izquierda/derecha, invierte la velocidad en X usando `word_C0D3`. |
| `sub_BD81` | `BounceEnemyVerticalCeilingFloor` | `$BD81` | `9695` | `TempPointer_0C0+1` (bits de colisión) | Si colisiona con techo/suelo, invierte la velocidad en Y usando `word_C0D3`. |
| `sub_BDE9` | `ComputeHomingDistanceVector` | `$BDE9` | `9788` | `Y`: offset coord (`X` o `Y`), `EnemyStruct_7` | Calcula `|PlayerPos - EnemyPos| * 256 / TargetRange` para suavizado de persecución. |
| `sub_BE2F` | `TrackPlayerCoordDelta16` | `$BE2F` | `9845` | `Y`: coord, `offset $0D` (contador oscilación) | Evalúa si el enemigo está dentro de un rango de ±16 píxeles del jugador para alternar patrones. |
| `ObjCode_7a` | `ExecuteHorusHuntingAI` | `$BEA8` | `9940` | `EnemyStructPointer`, `PlayerStruct` | Calcula las distancias Manhattan en X e Y, selecciona el eje dominante libre de colisión y reorienta a Horus. |

---

## 4. Algoritmos Detallados de los Arquetipos de IA

### 4.1 Ciclo de Dificultad y Velocidades de Enemigos (`word_C0D3`, [`data.asm:178-186`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L178-L186))

Las velocidades 8.8 asignadas a los enemigos provienen de `word_C0D3[speed_idx]` (donde `speed_idx` es determinado por la ronda actual en `DifficultyTable`):

```c
// Velocidades fijas 8.8 asignadas a enemigos según nivel de dificultad (0..7):
static const uint16_t kEnemySpeedTable_C0D3[8] = {
    0x0058, // Nivel 0: 0.344 px/frame
    0x0070, // Nivel 1: 0.437 px/frame
    0x0088, // Nivel 2: 0.531 px/frame
    0x00A0, // Nivel 3: 0.625 px/frame
    0x00B8, // Nivel 4: 0.718 px/frame
    0x00D0, // Nivel 5: 0.812 px/frame
    0x00E8, // Nivel 6: 0.906 px/frame
    0x0100  // Nivel 7: 1.000 px/frame
};
```

### 4.2 Transformación Determinista de Momias (`TransformMummy` `$BC01`)

Una momia inicia con el temporizador `offset $15` cargado con `EnemyTransformTimer`. Cada frame en tierra decrementa este contador:
- Al llegar a 0 (o si cae por debajo de `YPos >= 0xD8` en salas del palacio), se ejecuta la fórmula de transmutación:

$$\text{NuevoTipo} = (\text{APressCounter} \pmod 6) + 2$$

Donde:
- `2`: **Hanezo** (pájaro/saltador rojo con rebotes elásticos).
- `3`: **GejiShogun** (oruga rastreadora que recorre las plataformas perimetrales).
- `4`: **Gameido** (tortuga teledirigida con persecución proporcional suavizada).
- `5`: **Dokuron** (calavera saltarina con persecución horizontal directa).
- `6`: **Desufa** (bola de fuego flotante que oscila verticalmente hacia la altura de Jack).
- `7`: **Horus** (halcón cazador de alta velocidad con navegación selectiva en 4 direcciones).

Tras asignar el nuevo tipo, se invoca `OrEnemyStatusWith08`, que inicia 32 frames de animación de parpadeo (`offset $12 = 0x20`) antes de que el nuevo enemigo pueda dañar a Jack o interactuar con las colisiones.

### 4.3 IA de Horus (Halcón Cazador, `ObjCode_7a` `$BEA8`)

Horus ejecuta el algoritmo de persecución más sofisticado del juego:
1. Calcula la distancia absoluta en X ($|\Delta X| = |X_{\text{Jack}} - X_{\text{Horus}}|$) y en Y ($|\Delta Y| = |Y_{\text{Jack}} - Y_{\text{Horus}}|$).
2. Compara $|\Delta X|$ vs $|\Delta Y|$:
   - Si $|\Delta X| > |\Delta Y|$: el **eje dominante** es horizontal. El vector prioritario se dirige hacia Jack en X; el secundario en Y.
   - Si $|\Delta Y| \ge |\Delta X|$: el **eje dominante** es vertical. El vector prioritario se dirige hacia Jack en Y; el secundario en X.
3. Evalúa los bits de colisión de fondo del frame anterior (`TempPointer_0C0+1`): si el vector primario choca contra un obstáculo sólido, conmuta automáticamente al vector secundario libre.
4. Actualiza la animación de sprite (`sub_BEE7`):
   - Movimiento horizontal hacia la izquierda: animación con orientación izquierda (`anim_offset = 0`).
   - Movimiento horizontal hacia la derecha: animación volteada horizontalmente (`anim_offset = 7`).
   - Movimiento vertical puro (hacia arriba o hacia abajo): vista frontal (`anim_offset = 14`).

### 4.4 Mecánica de la Power Coin y Monedas Doradas (`TurnEnemiesIntoCoins`)

Al recoger la Power Coin (o alcanzar el nivel 3 de Mighty Power con monedas activas):
1. Fija el temporizador global de monedas `EnemyCoinTimer = 0x0140` (320 frames = 5.33 segundos a 60 Hz).
2. Carga en la paleta de sprites secundaria los colores de moneda dorada: `{0x00, 0x08, 0x26, 0x30}`.
3. Durante el ciclo `DrawEnemies`:
   - Todo enemigo estándar (tipos 0..7) con bit 4 de status apagado se convierte en moneda dorada: fija bit 5 de status (`status |= 0x20`) y fuerza el tile a `$21` (moneda rotatoria).
   - En los últimos 64 frames (`EnemyCoinTimer < 0x40`), la paleta de la moneda parpadea en cada frame para advertir su reversión inminente.
4. Al colisionar Jack con una moneda transmutada:
   - Se procesa en `JT_98A4_multi`: reproduce `Sound_CoinCollected` y otorga puntos basados en la cadena continua de recolección (`byte_54`):
     - Moneda 1: 100 pts.
     - Moneda 2: 200 pts.
     - Moneda 3: 300 pts.
     - Moneda 4: 500 pts.
     - Moneda 5: 800 pts.
     - Moneda 6+: 1200 / 2000 pts.
   - Incrementa el contador global `GoldCoinsCollected` (cada 40 monedas otorga bonificación especial).

---

## 5. Especificación de Estructura de Datos en C x86

```c
// Estructura de 28 bytes de un slot de enemigo (paridad 1:1 con EnemyStruct en $06A6-$0785)
typedef struct {
    uint8_t status;         // +0x00: Bit 0=Activo, Bit 1=En aire/salto, Bit 3=Spawn flash, Bit 5=Es moneda
    uint8_t collision_flags;// +0x01: Bits 0..3: [Techo|Suelo|Pared_Izq|Pared_Der]
    uint8_t x_vel_lo;       // +0x02: Velocidad X subpíxel (8.8)
    uint8_t x_vel_hi;       // +0x04: Velocidad X píxeles con signo
    uint8_t x_pos_lo;       // +0x03: Posición X subpíxel
    uint8_t x_pos_hi;       // +0x05: Posición X píxeles en pantalla
    uint8_t offscreen_x;    // +0x06: Flag de salida de pantalla horizontal
    uint8_t target_range;   // +0x07: Rango de proximidad / divisor de homing
    uint8_t y_vel_lo;       // +0x08: Velocidad Y subpíxel (8.8)
    uint8_t y_vel_hi;       // +0x0A: Velocidad Y píxeles con signo
    uint8_t y_pos_lo;       // +0x09: Posición Y subpíxel
    uint8_t y_pos_hi;       // +0x0B: Posición Y píxeles en pantalla
    uint8_t offscreen_y;    // +0x0C: Flag de salida de pantalla vertical
    uint8_t anim_state;     // +0x0D: Sub-estado de animación / contador de oscilación
    uint8_t sprite_tile;    // +0x0E: Tile de sprite actual en CHR-ROM
    uint8_t sprite_attr;    // +0x0F: Atributos PPU (flip H/V, paleta)
    uint8_t anim_offset;    // +0x10: Puntero/offset actual en SpriteAnimationTable
    uint8_t anim_timer;     // +0x11: Temporizador de duración del frame de animación
    uint8_t flash_timer;    // +0x12: Temporizador de parpadeo de invulnerabilidad/spawn
    uint8_t type;           // +0x13: Tipo de entidad (0=Mummy, 2=Hanezo, ..., 8=PowerCoin, etc.)
    uint8_t facing_dir;     // +0x14: Dirección visual (0=Izq, 4=Der)
    uint8_t transform_timer;// +0x15: Temporizador de evolución de Momia a otro enemigo
    uint8_t speed_mod;      // +0x16: Índice de velocidad (0..7) tomado de dificultad
    uint8_t extra_param;    // +0x17: Multiplicador o parámetro de score
    uint8_t jump_phase_lo;  // +0x18: Fase de salto subpíxel (8.8)
    uint8_t jump_phase_hi;  // +0x19: Fase de salto alta (0x00..0x7F)
    uint8_t state_flags;    // +0x1A: Flags de estado interno del arquetipo
    uint8_t anim_loop_start;// +0x1B: Offset de reinicio del bucle de animación
} MBJ_EnemySlot;
```

---

## 6. Plan de Pruebas Unitarias y Verificación

1. **`test_mummy_deterministic_evolution`**:
   - Para valores de `APressCounter` de 0 a 255, verificar que la momia evoluciona estrictamente a los tipos `2..7` cumpliendo la regla de congruencia `(APressCounter % 6) + 2`.
2. **`test_enemy_sliced_scheduler`**:
   - Comprobar que en una ronda con 4 o más enemigos activos, cada llamada a `RunActiveEnemiesSliced` procesa exactamente un lote de 4 enemigos en rotación continua sin saltarse ningún slot activo.
3. **`test_power_coin_transmutation_duration`**:
   - Verificar que al recoger una Power Coin, `EnemyCoinTimer` se inicializa exactamente en 320 frames, decrementa en 1 por tick a 60 Hz, y que los enemigos revierten a su estado estándar en el frame 321.
