# Contrato Funcional: Inteligencia Artificial de Enemigos, Ciclos de Spawn y Mutación

- **ID de Contrato**: `research/contracts/behavior_contracts/enemy_ai_contract.md`
- **Subsistema**: 04 — Enemigos, Inteligencia Artificial y Spawns
- **Tarea**: `TASK-F3-004`
- **Fuentes de Ensamblador**:
  - `asm/src/prg.asm:8150-8650` (`RunEnemyLogic`, `HandleSpawnCycles`, `InitEnemy`)
  - `asm/src/prg.asm:8270-8320` (Mutación de Momias determinista)
  - `asm/src/ram.asm:900-1180` (Estructura `EnemyStruct` de 28 bytes)
- **Estado Epistémico**: `[CONFIRMADA]`

---

## 1. Estructura de Memoria de los Enemigos (`EnemyStruct` de 28 Bytes)

El juego gestiona un máximo de **8 slots de entidades simultáneas** alojadas en `$0600-$07FF`:

| Offset | Nombre del Campo | Propósito Funcional |
| :---: | :--- | :--- |
| **`+$00`** | `EnemyStatus` | Bit 7: Activo, Bit 6: Flash de spawn, Bit 5: Transmutado en moneda, Bit 0: En suelo. |
| **`+$01`** | `EnemyType` | Tipo de entidad (`0..6` monstruos, `7..$B` items/monedas). |
| **`+$02`** | `EnemyXPosHi` | Coordenada X entera (píxel en pantalla). |
| **`+$03`** | `EnemyXPosLo` | Subpíxel X fraccionario (8.8). |
| **`+$04`** | `EnemyYPosHi` | Coordenada Y entera. |
| **`+$05`** | `EnemyYPosLo` | Subpíxel Y fraccionario (8.8). |
| **`+$06`** | `EnemyXVelHi` | Velocidad horizontal entera (con signo). |
| **`+$07`** | `EnemyXVelLo` | Velocidad horizontal fraccionaria. |
| **`+$08`** | `EnemyYVelHi` | Velocidad vertical entera. |
| **`+$09`** | `EnemyYVelLo` | Velocidad vertical fraccionaria. |
| **`+$0A`** | `EnemyAnimFrame` | Índice de frame de animación de sprite. |
| **`+$0B`** | `EnemyAnimTimer` | Temporizador de cadencia de animación. |
| **`+$0C`** | `EnemyFacingAttr` | Atributo de dirección de sprite (bit 3: flip horizontal). |
| **`+$10`** | `EnemyStateTimer`| Temporizador de estado interno (patrones de IA). |
| **`+$18`** | `EnemySubIndex` | Índice secundario de interpolación o mutación. |

---

## 2. Los 6 Arquetipos de Enemigos y sus Comportamientos

### Arquetipo 0: Momia (Mummy) — `ObjCode_0_Mummy`
- **Comportamiento**: Enemigo patrullero terrestre base. Camina horizontalmente sobre plataformas a velocidad fija ($\pm 0.75$ px/f). Al colisionar con una pared o al llegar a un borde de plataforma sin suelo, invierte su dirección.
- **Vulnerabilidad**: Se transforma en moneda si Jack recoge una Power Coin o si cae en una trampa de Mighty Drink.

### Arquetipo 1: Hanezo (Pájaro Mecánico) — `ObjCode_1_Hanezo`
- **Comportamiento**: Vuelo libre aéreo. Se desplaza en ondas sinusoidales horizontales. Ignora las plataformas intermedias y rebota al alcanzar los límites laterales de la pantalla.

### Arquetipo 2: GejiShogun (Ciempiés) — `ObjCode_2_GejiShogun`
- **Comportamiento**: Reptador perimetral. Se desplaza pegado a las superficies sólidas, rotando sus coordenadas $90^\circ$ al encontrar esquinas convexas o cóncavas.

### Arquetipo 3: Gameido (Tortuga) — `ObjCode_3_Gameido`
- **Comportamiento**: Movimiento lento terrestre ($\pm 0.5$ px/f). Cuando Jack se acerca a menos de 32 píxeles horizontales, se repliega temporalmente en su caparazón, volviéndose inmune a colisiones durante 64 frames.

### Arquetipo 4: Dokuron (Calavera Rebotadora) — `ObjCode_4_Dokuron`
- **Comportamiento**: Desplazamiento diagonal a $45^\circ$ a alta velocidad ($\pm 1.5$ px/f). Rebota elásticamente en suelos, techos y paredes laterales.

### Arquetipo 5: Desufa / Horus (Pájaro Sagrado) — `ObjCode_5_Desufa`
- **Comportamiento**: Enemigo cazador punitivo. Aparece cuando Jack permanece en la misma plataforma o en el suelo durante más de 180 frames continuos. Vuela directamente hacia las coordenadas actuales de Jack en línea recta.

---

## 3. Algoritmo Determinista de Mutación de Momias

Cuando una Momia completa su ciclo de patrulla o cuando Jack acumula saltos consecutivos sin recoger objetos, la momia muta en otro arquetipo de enemigo según la fórmula matemática de `prg.asm:8280`:

$$\text{NuevoArquetipo} = (\text{ContadorPulsacionesA} \pmod 6) + 2$$

```c
void Enemy_MutateMummy(EnemyStruct *enemy, uint8_t a_press_counter) {
    /* Mapea a arquetipos 2 (GejiShogun), 3 (Gameido), 4 (Dokuron), 5 (Desufa), 6 (Horus) o 7 */
    uint8_t new_type = (a_press_counter % 6) + 2;
    enemy->type = new_type;
    enemy->anim_frame = 0;
    enemy->state_timer = 0;
    enemy->status |= ENEMY_STATUS_SPAWN_FLASH; /* Parpadea durante 16 frames al mutar */
}
```

---

## 4. Transmutación por Power Coin (320 Frames)

Cuando Jack recoge una **Power Coin**:
1. Se activa el temporizador global `TimerEnemyTransmute = 320` frames (aprox. 5,3 segundos a 60 Hz).
2. Todos los slots de enemigos activos (`0..7`) conmutan su flag de estado a `ENEMY_STATUS_TRANSMUTED`.
3. Sus sprites cambian al gráfico de moneda coleccionable.
4. Si Jack colisiona con una moneda transmutada:
   - Suma puntos en escala exponencial BCD: $100 \to 200 \to 400 \to 800 \to 1.000 \to 2.000 \to 4.000 \to 8.000$.
   - Desactiva el slot de enemigo y reproduce el efecto SFX de moneda.
5. Al expirar los 320 frames (`TimerEnemyTransmute == 0`), los enemigos no recogidos recuperan su forma original.
