# SPEC-REF-01: Especificación del Sistema de Replay Diferencial Jerárquico y Refinamiento Iterativo Desatendido

**Proyecto**: Mighty Bomb Jack — Source Port Nativo C99 / Windows  
**Fase de Aplicación**: Fase 7 — Bug Fixing, Paridad Exacta 1:1 y Refactorización  
**Ubicación**: `research/architecture/differential_replay_refinement_spec.md`  
**Estado**: `[CONFIRMADA / ESPECIFICACIÓN ACTIVA]`  

---

## 1. Visión General y Filosofía de Diseño

Para cerrar la brecha funcional y de comportamiento entre el cartucho original de NES (1987) y el Source Port en C99, se adopta la metodología **Hierarchical Golden Master Replay & Horizon Pushing**.

### 1.1. Diagnóstico de la Brecha Actual: Macro antes de Micro
El port no adolece únicamente de discrepancias milimétricas de subpíxel, sino de **desconexiones lógicas y macroscópicas mayores**:
* Saltos ilegales a la *Torture Room* por acumulación de contadores no reseteados.
* Fallos en la transición a la Sala 2 por desalineación de coordenadas entre espacio de mundo y espacio de pantalla.
* Spawns e IA de enemigos que no leen las tablas canónicas de la sala.
* Inversión de prioridad en capas de sprites OAM (enemigos dibujándose por encima de Jack).
* Cadencia de temporizadores desacoplada del reloj NMI (60.0988 Hz).

Intentar un diff visual píxel a píxel sobre una base con fallos estructurales genera ruido y bloqueos prematuros. Por ello, la auditoría y corrección se organiza en una **Pirámide Jerárquica de 5 Niveles**, resolviendo obligatoriamente de lo macro a lo micro.

### 1.2. El Paradigma del "Ancla Dorada" (*Golden Master Anchor*)
1. **El jugador humano solo interactúa con la ROM original (`cynes`)**: El jugador ve la pantalla de la NES auténtica y sus controles responden con la física y tiempos 100% genuinos. Juega libremente durante 30 a 60 segundos sin frustración ni bloqueos por bugs del port.
2. **Grabación pasiva transparente**: Se captura la secuencia exacta de botones (`.tas`) y la trayectoria de telemetría de la ROM (`golden_telemetry.json`).
3. **Auditoría Desatendida por Compuertas**: El arnés diferencial ejecuta el port en C en modo *headless*, compara capa por capa según la pirámide de paridad, detecta el primer fallo en el nivel más alto activo, y emite un micro-reporte quirúrgico para los agentes.

```
+---------------------------------------------------------------------------------------------------+
| FASE 1: ADQUISICIÓN HUMANA O REPLAY TAS (Golden Play Session)                                     |
|                                                                                                   |
|  [Jugador Humano / Speedrun .fm2] ---> [Inputs a 60 Hz]                                           |
|                                              |                                                    |
|                                              v                                                    |
|                                  +-----------------------+                                        |
|                                  | cynes (NES ROM 1987)  | <--- Golden Reference                  |
|                                  +-----------------------+                                        |
|                                              |                                                    |
|                               +--------------+--------------+                                     |
|                               v                             v                                     |
|                      [session_XX.tas]             [golden_telemetry.json]                         |
|                   (Stream puro de botones)     (Snapshots de RAM, OAM, PPU)                       |
+---------------------------------------------------------------------------------------------------+
                                               |
                                               v
+---------------------------------------------------------------------------------------------------+
| FASE 2: ORÁCULO DE PARIDAD JERÁRQUICA (tools/hierarchical_parity_oracle.py - CERO TOKENS)         |
|                                                                                                   |
|  [session_XX.tas] ----> [differential_harness.exe (Port C99 Headless)]                            |
|                                        |                                                          |
|                                        v (Telemetría Port)                                        |
|                         +------------------------------+                                          |
|                         |  Filtro por Compuertas (1-5) | <--- [golden_telemetry.json]             |
|                         +------------------------------+                                          |
|                                        |                                                          |
|          ¿Fallo en Nivel 1 (Macro)? -> SÍ -> [CORTA Y EMITE REPORTE NIVEL 1]                      |
|                                        | NO                                                       |
|          ¿Fallo en Nivel 2 (Mundo)? -> SÍ -> [CORTA Y EMITE REPORTE NIVEL 2]                      |
|                                        | NO                                                       |
|          ¿Fallo en Nivel 3 (Actores)?> SÍ -> [CORTA Y EMITE REPORTE NIVEL 3]                      |
|                                        | NO                                                       |
|          ¿Fallo en Nivel 4 (Físicas)?> SÍ -> [CORTA Y EMITE REPORTE NIVEL 4]                      |
|                                        | NO                                                       |
|          ¿Fallo en Nivel 5 (Visual)?-> SÍ -> [CORTA Y EMITE REPORTE NIVEL 5]                      |
|                                        | NO                                                       |
|                         [100% PARIDAD HOMOLOGADA EN LA SESIÓN]                                    |
+---------------------------------------------------------------------------------------------------+
                                               |
                                               v
+---------------------------------------------------------------------------------------------------+
| FASE 3: BUCLE DE REFINAMIENTO ITERATIVO (Empuje de Horizonte - Flash Agent)                        |
|                                                                                                   |
|   1. Agente Parity Solver (Flash) lee divergence_report.json (máx 50 líneas).                     |
|   2. Aplica parche en src/core/ basándose en las líneas exactas de asm/src/prg.asm.               |
|   3. Ejecuta arnés diferencial localmente.                                                        |
|   4. ¿Avanza el horizonte más allá del frame anterior o sube de nivel en la pirámide?             |
|        - SÍ: Commit y continuar.                                                                  |
|        - NO: Reintento quirúrgico de lógica.                                                      |
+---------------------------------------------------------------------------------------------------+
```

---

## 2. La Pirámide de Paridad Jerárquica (5 Niveles de Auditoría)

```
                    /\
                   /  \      NIVEL 5: Micro-Visual (Píxeles, Paletas y Sprites OAM)
                  /----\
                 /      \    NIVEL 4: Micro-Física (Subpíxeles, Gravedad y Colisiones)
                /--------\
               /          \  NIVEL 3: Actores e IA (Spawns, Rutas de Enemigos y Chispas)
              /------------\
             /              \ NIVEL 2: Objetos del Mundo (Bombas, Cofres, Puertas y GDV)
            /----------------\
           /                  \ NIVEL 1: Macro-Flujo (Máquina de Estados y Navegación de Salas)
          +--------------------+
```

### Nivel 1: Macro-Flujo y Máquina de Estados (Compuerta 1)
* **Objetivo**: Garantizar que el juego transita ordenadamente entre pantallas y salas sin desvíos espurios.
* **Variables RAM auditadas**:
  * `$0044` (`GameState`): 0=Init, 1=RoundIntro, 2=LoadRoom, 3=DrawRoom, 4=EnterRoom, 5=MainLoop, 7=Torture, 8=RoundClear, 9=ExitRoom, B=AnimateDoorOpen, C=Ending.
  * `$0045` (`TitleScreenState`): Fases 0..5 de la secuencia de título.
  * `$0043` (`InGameFlag`): 0=Menú/Intro, 1=Partida activa.
  * `$03BF` (`CurrentRoomID`): Identificador de la sala activa (1..257).
* **Condiciones de Fallo Inmediato**:
  * Transición a `GameState=7` (Tortura) mientras la ROM continúa en `GameState=5`.
  * Bloqueo al cruzar la puerta: La ROM avanza a `RoomID=2` pero el port se queda en `RoomID=1`.
  * Duración incorrecta de temporizadores de estado (ej. Round Intro != 96 frames).

### Nivel 2: Objetos y Persistencia del Mundo (Compuerta 2)
* **Objetivo**: Asegurar que los bloques interactivos y datos de sala se actualizan de forma idéntica.
* **Variables RAM auditadas**:
  * `$0600..$063F` (`RoomDataRAM`): Ranuras activas de sala.
  * `$0603 + n`: Estado de bombas (apagada `$05`, mecha activa `$85`, recogida `$00`).
  * Estado de cofres: Cerrado `$10`, golpeado `$11`, abierto con ítem `$12..$18`.
  * Estado de la puerta: Cerrada `$12`, abierta `$8A`, condición de apertura evaluada.
* **Condiciones de Fallo**:
  * La puerta de la ROM se abre y en el port permanece cerrada.
  * Una bomba recogida no descuenta el contador o no activa la siguiente mecha.

### Nivel 3: Ciclo de Vida de Actores y Enemigos (Compuerta 3)
* **Objetivo**: Paridad en el nacimiento, comportamiento básico y transformación de enemigos.
* **Variables RAM auditadas**:
  * `$0316..$0325`: Slots de enemigos activos (Tipo, Sub-estado, Animación).
  * `$004C..$0053`: Posiciones Y y X de los enemigos en pantalla.
  * `$006A..$006B` (`EnemyCoinTimer`): Temporizador de transmutación a monedas (320 frames).
  * `$0210` (OAM byte 16): Visibilidad y coordenadas de la chispa de la mecha.
* **Condiciones de Fallo**:
  * Enemigo spawneado en frame o coordenada X/Y diferente a la ROM.
  * Falta de transmutación de enemigos al recoger una moneda Mighty.

### Nivel 4: Cinemática, Físicas Continuas y Colisiones (Compuerta 4)
* **Objetivo**: Idéntica respuesta de mando, curvas de salto y colisiones contra el entorno.
* **Variables RAM auditadas**:
  * `$03D8, $03D9`: Coordenadas X e Y de Jack en la sala.
  * `$03DA, $03DB`: Acumuladores de subpíxel de Jack.
  * `$0048, $004A`: Coordenadas de pantalla de Jack.
  * Flags de salto, aterrizaje en suelo (`on_ground`) y planeo activo (`flutter`).
* **Condiciones de Fallo**:
  * Desviación $\ge 1$ píxel en la trayectoria parabólica de Jack tras un salto.
  * Diferencia en el frame de aterrizaje en una plataforma.

### Nivel 5: Micro-Visual y Renderizado OAM (Compuerta 5)
* **Objetivo**: Perfección cosmética, orden de capas y fidelidad gráfica.
* **Variables PPU auditadas**:
  * `$0200..$02FF` (OAM Hardware): Coordenadas, IDs de tile y atributos de los 64 sprites.
  * **Prioridad de Sprites**: Jack debe ocupar los primeros slots OAM (0..15) para que los enemigos nunca se rendericen por encima de él.
  * Nametables VRAM (`$2000..$23BF`): Índices exactos de metatiles en pantalla.
  * Paleta PPU (`$3F00..$3F1F`): Subpaletas de sprites y fondo.

---

## 3. Estrategia de Pruebas Macro Sin Completar el Juego

Para validar los niveles macroscópicos **no es necesario pasarse el juego completo**. El ciclo de vida de salas de *Mighty Bomb Jack* es invariante y modular: validar el paso de la Sala 1 a la Sala 2 homologa el 90% de la lógica macro del motor.

### 3.1. Las Tres Vías de Entrada de Pruebas
1. **Micro-Sesiones Humanas (30 a 60 segundos)**:
   * El jugador arranca el juego, avanza por la Sala 1, recoge las bombas requeridas, abre la puerta y cruza a la Sala 2.
   * Esa única sesión de menos de 1 minuto ejercita 8 de los 12 estados principales de la máquina de estados.
2. **Speedruns TAS Públicos (.fm2 / TASVideos.org)**:
   * Reutilización de archivos TAS preexistentes de la comunidad que contienen millones de inputs deterministas verificados.
   * Se ejecutan directamente en `cynes` y en el port sin requerir intervención humana.
3. **Catálogo de los 5 Micro-Escenarios Canónicos**:

| Micro-Escenario | Duración | Estados Ejercitados | Criterio de Éxito Macro |
| :--- | :--- | :--- | :--- |
| **A. Transición de Sala** | 45 s | `State 0 -> 1 -> 2 -> 3 -> 4 -> 5 -> B -> 9 -> 2` | Jack abre la puerta de la Sala 1 y aparece en la Sala 2. |
| **B. Ciclo de Muerte** | 10 s | `State 5 -> Muerte -> Animación -> Respawn` | Tocar enemigo descuenta vida y Jack reaparece en el suelo. |
| **C. Ciclo de Torture Room** | 25 s | `State 5 -> State 7 -> 50 saltos -> Retorno` | La tortura solo se dispara en condiciones legítimas; al cumplir los 50 saltos se regresa a la sala. |
| **D. Fin de Tiempo (Time Over)** | 15 s | `State 5 -> State 6 -> Time Over -> Muerte` | Agotar el tiempo descuenta vida canónicamente. |
| **E. Sala de Bombas (Bonus)** | 30 s | `State 5 -> Bomb Room -> State 8 (Round Clear)` | Recoger bombas verticalmente dispara la pantalla de Round Clear. |

---

## 4. Definición de Formatos y Reportes de Divergencia

### 4.1. Formato del Reporte Jerárquico (`scratch/divergence_report.json`)
Emitido por el oráculo tan pronto detecta una divergencia en el nivel más alto que falle:

```json
{
  "status": "DIVERGENCE_DETECTED",
  "failed_level": 1,
  "level_name": "MACRO_FLOW_STATE_MACHINE",
  "divergence_frame": 496,
  "total_session_frames": 2400,
  "progress_percentage": 20.66,
  "discrepancy": {
    "variable": "game_state",
    "golden_value": "5 (GAME_STATE_5_MAIN_LOOP)",
    "port_value": "7 (GAME_STATE_7_TORTURE_ROOM)",
    "room_id": 1
  },
  "root_cause_analysis": {
    "summary": "Disparo ilegal de la Sala de Tortura por acumulación de greed_jump_counter",
    "mechanism": "greed_jump_counter se incrementa en cada salto de juego normal sin resetearse al tocar suelo"
  },
  "asm_ground_truth": {
    "routine": "CheckGreedCondition",
    "source_file": "asm/src/prg.asm",
    "lines": "1420-1445",
    "canonical_logic": "La tortura solo evalúa byte_396 cuando el temporizador de sala $038D llega a cero o por exceso de Mighty Drinks"
  },
  "c_source_target": {
    "file": "src/core/mjk_state_machine.c",
    "function": "State_5_MainLoop",
    "lines": "480-498"
  }
}
```

---

## 5. Arquitectura de Agentes de Refinamiento Jerárquico

```
+-----------------------------------------------------------------------------------+
| NIVEL 1: ORQUESTADOR PRINCIPAL (Gemini Pro / Argon)                              |
| - Rol: Monitor de compuertas y supervisor de avance de la pirámide.               |
| - Despacha la tarea quirúrgica leyendo ÚNICAMENTE el JSON del reporte.            |
| - Homologa el paso de un nivel a otro en la pirámide y realiza commits en Git.   |
+-----------------------------------------------------------------------------------+
                                          |
                                          v (Despacho acotado)
+-----------------------------------------------------------------------------------+
| NIVEL 2: PARITY SOLVER (Gemini Flash)                                             |
| - Rol: Agente efímero especializado en corrección de código C contra ASM 6502.    |
| - Prohibida cualquier interacción conversacional humana. Entrada JSON -> Parche C.|
| - Lee las líneas de ASM y C especificadas en el reporte.                          |
| - Aplica la corrección en src/core/.                                              |
| - Ejecuta 'python tools/hierarchical_parity_oracle.py --verify' para auto-test.   |
| - Si la compuerta actual pasa: reporta éxito y es ELIMINADO de inmediato.        |
+-----------------------------------------------------------------------------------+
```

---

## 6. Módulos de Software en `tools/`

| Herramienta | Rol / Descripción | Tecnologías |
| :--- | :--- | :--- |
| `tools/recorder_frontend.py` | Interfaz gráfica interactiva (60 FPS) para jugar a la ROM con mando Xbox/teclado. Graba en caliente `session.tas` y `golden_telemetry.json`. | Python, `cynes`, `pygame` |
| `tools/hierarchical_parity_oracle.py` | Motor de auditoría diferencial por compuertas (Niveles 1 a 5). Ejecuta el port en modo *headless*, compara contra la ROM y genera `divergence_report.json`. | Python, `cynes`, `subprocess`, `json` |
| `tools/differential_harness.c` | Ejecutable en C sin ventana que corre el bucle a máxima velocidad con `--replay <file.tas>` y emite la telemetría normalizada frame a frame. | C99, Core del port |
| `tests/tas/` | Repositorio de micro-escenarios (`01_room1_to_room2.tas`, `02_death_respawn.tas`, `03_torture_cycle.tas`). | Archivos `.tas` |

---

## 7. Métricas de Éxito de la Fase 7

1. **Superación Secuencial de Compuertas**:
   * **Compuerta 1 (Macro-Flujo)**: 100% de éxito en transiciones Sala 1 -> Sala 2 y 0 estados ilegales.
   * **Compuerta 2 (Mundo)**: 100% de paridad en apertura de puertas, bombas y cofres.
   * **Compuerta 3 (Actores)**: 100% de coincidencia en puntos de spawn e IA de enemigos.
   * **Compuerta 4 (Físicas)**: Desviación de coordenadas $\le 0$ píxeles en 3.000 frames continuos.
   * **Compuerta 5 (Visual)**: Prioridad OAM de Jack perfecta y consistencia de metatiles.
2. **Eficiencia de Tokens**: Reducción superior al 80% en el consumo de tokens al canalizar todo el diagnóstico a través del oráculo jerárquico determinista.
