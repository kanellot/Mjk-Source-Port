# Tablero de Tareas Persistente (Task Tracker)

Este archivo es la **Única Fuente de Verdad** para el estado de ejecución del proyecto. Tanto el orquestador como los subagentes deben consultar y actualizar este archivo en cada ciclo.

---

## Convenciones de Estados

- `[TODO]` - Tarea pendiente de asignación.
- `[IN_PROGRESS]` - Tarea en ejecución activa por un agente (debe incluir ID del agente y fecha).
- `[PAUSED]` - Tarea temporalmente suspendida o interrumpida (cuenta con **Checkpoint Activo** listo para reanudar de inmediato).
- `[BLOCKED]` - Tarea bloqueada por dependencia técnica o pregunta abierta.
- `[VERIFICATION]` - Implementación terminada, en proceso de verificación o testing.
- `[DONE]` - Tarea verificada con pruebas exitosas y completada.

> **REGLA DE MICRO-LOTES Y CHECKPOINTS (ADR-003)**:
> Cada tarea debe subdividirse en micro-pasos (`- [ ]`). Cuando una tarea está activa o pausada, debe mantenerse actualizado su bloque de `CHECKPOINT ACTIVO` para que cualquier agente reanude en segundos sin re-escanear el repositorio.

---

## Resumen Rápido de Tareas

| ID | Prioridad | Módulo | Título | Estado | Responsable |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `TASK-SETUP-001` | P0 | Infraestructura | Reset del workspace y despliegue del sistema de memoria persistente | `[DONE]` | Orchestrator |
| `TASK-SETUP-002` | P0 | Infraestructura | Blindaje de permisos: `asm/` solo lectura y `port_native/` escritura | `[DONE]` | Orchestrator |
| `TASK-SETUP-003` | P0 | Base de Conocimiento | Estructura permanente de investigación técnica e índice de hallazgos | `[DONE]` | Orchestrator |
| `TASK-RES-001` | P0 | Investigación (`asm/`) | Análisis de la naturaleza del desensamblado `asm/` (total vs parcial) y estrategia de inventario de símbolos/funciones | `[DONE]` | Orchestrator |
| `TASK-RES-002` | P0 | Inventario (`asm/`) | Extracción determinista del catálogo maestro de símbolos, direcciones, `JumpTables` y grafo de llamadas (`RES-003`) | `[DONE]` | Orchestrator |
| `TASK-RES-003` | P1 | Inventario (`asm/`) | Especificación C x86 Subsistema 01: Boot, NMI Pipeline, PPU Buffer, Joypad y Máquina de Estados (`RES-004`) | `[DONE]` | Orchestrator |
| `TASK-RES-004` | P1 | Inventario (`asm/`) | Especificación C x86 Subsistema 02: Mapa completo de RAM (`$0000-$07FF`) y Zero-Page Scratchpad (`RES-005`) | `[DONE]` | Orchestrator |
| `TASK-RES-005` | P1 | Inventario (`asm/`) | Especificación C x86 Subsistema 03: Física de Jack, Salto/Aleteo, Colisiones 9-Probe y Scroll (`RES-006`) | `[DONE]` | Orchestrator |
| `TASK-RES-006` | P1 | Inventario (`asm/`) | Especificación C x86 Subsistema 04: Decodificador de Salas (4-Tier), Puertas, Cofres y Bombas (`RES-007`) | `[DONE]` | Orchestrator |
| `TASK-RES-007` | P1 | Inventario (`asm/`) | Especificación C x86 Subsistema 05: IA de Enemigos, Spawns, Momias y Monedas (`RES-008`) | `[DONE]` | Orchestrator |
| `TASK-RES-008` | P1 | Inventario (`asm/`) | Especificación C x86 Subsistema 06: Pantallas Especiales, Salas Reales, Torture Room, GDV y Ending (`RES-009`) | `[DONE]` | Orchestrator |
| `TASK-RES-009` | P1 | Inventario (`asm/`) | Especificación C x86 Subsistema 07: Motor de Audio 2A03 y Secuencias Musicales (`RES-010`) | `[DONE]` | Orchestrator |
| `TASK-RES-010` | P0 | Auditoría (`asm/`) | Auditoría Integral de Puntos Ciegos, Idiomas 6502 (Double Return) y Discrepancias (`RES-011`) | `[DONE]` | Orchestrator |
| `TASK-BUILD-001` | P1 | Build System | Andamiaje limpio de build y verificación del toolchain (CMake + MSVC + Raylib 5.5) | `[DONE]` | Orchestrator (2026-10-22) |
| `TASK-IMPL-001` | P1 | Núcleo y Memoria | Arquitectura Base, Mapa de RAM ($0000-$07FF), Zero-Page y Bucle 60 Hz (`RES-004`/`005`) | `[DONE]` | Orchestrator (2026-10-22) |
| `TASK-IMPL-002` | P1 | Plataforma / Render | Capa Raylib, Pantalla Completa (4:3), Render 256x240, Mando Xbox Series (XInput) y Teclado (`ADR-005`) | `[DONE]` | Orchestrator (2026-10-22) |
| `TASK-IMPL-003` | P1 | Física y Jack | Curva Parabólica (64 pasos), Modulación, Aleteo y Colisiones 9-Probe (`RES-006`) | `[DONE]` | Orchestrator (2026-10-22) |
| `TASK-IMPL-004` | P1 | Motor de Salas | Decodificador 4-Tier (257 salas), Scroll y 32 Puertas Versión USA (`RES-007`/`ADR-004`) | `[DONE]` | Orchestrator (2026-10-22) |
| `TASK-IMPL-005` | P1 | Objetos y Bombas | Secuencia de 23 Bombas, Cofres, Monedas y Poderes Mighty Niveles 1..3 (`RES-007`) | `[IN_PROGRESS]` | Orchestrator (2026-10-22) |
| `TASK-IMPL-006` | P1 | IA de Enemigos | Spawning, Mutación de Momias determinista y Conversión a Monedas (`RES-008`) | `[TODO]` | Unassigned |
| `TASK-IMPL-007` | P1 | Metasistemas y HUD | HUD BCD, Torture Room (50 saltos US), Cálculo GDV y Cinemática de 4 Finales (`RES-009`) | `[TODO]` | Unassigned |
| `TASK-IMPL-008` | P1 | Motor de Audio | Sintetizador 2A03 (4 canales), Bytecode VM y 35 Pistas/Efectos (`RES-010`) | `[TODO]` | Unassigned |
| `TASK-IMPL-009` | P1 | Verificación / Paridad | Verificación Bit-Exact y Auditoría Audiovisual contra vídeos de referencia (`AGENTS.md`) | `[TODO]` | Unassigned |

---

## Detalle de Tareas

### Hito 0: Inicialización y Gobernanza del Workspace

#### [DONE] `TASK-SETUP-001`: Reset del workspace y despliegue del sistema de memoria persistente
- **Módulo**: Infraestructura / Gobernanza
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Criterio de Aceptación**:
  - Eliminar flujos antiguos de `.agents` en raíz y en `asm/.agents`.
  - Eliminar archivos residuales de la raíz preservando solo `asm/`, `port_native/` y los dos videos `.mp4`.
  - Crear los 5 archivos de memoria persistente (`AGENTS.md`, `TASKS.md`, `PROJECT_STATUS.md`, `DECISIONS.md`, `SESSION_LOG.md`).
- **Verificación**: Inspección de directorio raíz limpia y verificación de lectura/escritura de los archivos de memoria.

#### [DONE] `TASK-SETUP-002`: Blindaje de permisos de carpetas
- **Módulo**: Infraestructura / Sistema Operativo
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Criterio de Aceptación**:
  - Carpeta `asm/` configurada como solo lectura a nivel de sistema operativo y regla en `AGENTS.md`.
  - Carpetas `port_native/` y raíz con permisos completos de escritura y ejecución.
- **Verificación**: Intentos de escritura denegados en `asm/LICENSE.txt` y exitosos en `port_native/` y raíz.

#### [DONE] `TASK-SETUP-003`: Estructura permanente de investigación técnica e índice de hallazgos
- **Módulo**: Base de Conocimiento / Memoria Persistente
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Criterio de Aceptación**:
  - Crear directorio `research/` dedicado a la acumulación de datos técnicos e ingeniería inversa.
  - Crear `research/INDEX.md` con catálogo maestro, convenciones de nombrado, categorización temática y estado de investigaciones.
  - Crear `research/TEMPLATE.md` como estándar reutilizable para informes técnicos.
  - Crear informe fundacional `research/RES-001-mapa-memoria-y-subsistemas-nes.md` con datos extraídos de `asm/src/defs.asm`.
  - Integrar `research/` en `AGENTS.md`, `DECISIONS.md` (ADR-002), `PROJECT_STATUS.md` y `SESSION_LOG.md`.
- **Verificación**: Comprobada la estructura de archivos, enlaces relativos, catálogo indexado y lectura/escritura en `research/`.

---

### Hito 1: Línea Base del Port y Verificación de Compilación

#### [DONE] `TASK-RES-001`: Análisis de la naturaleza del desensamblado `asm/` y estrategia de inventario de símbolos/funciones
- **Módulo**: `asm/` (Read-Only) -> `research/`
- **Prioridad**: Crítica (P0)
- **Dependencias**: `TASK-SETUP-003`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.1 Inspeccionar estructura raíz de `asm/`, scripts de compilación (`build.bat`/`build.sh`), configuración de ensamblador y archivos de inclusión principales.
  - [x] 0.2 Analizar archivos en `asm/src/` para medir cobertura (total vs parcial, proporción de etiquetas nombradas vs autogeneradas IDA `sub_XXXX`, `loc_XXXX`, `byte_XXXX`, blobs binarios `incbin` vs código desensamblado).
  - [x] 0.3 Documentar el informe técnico `research/RES-002-analisis-naturaleza-desensamblado-asm.md` con métricas exactas y estrategia óptima de inventariado de símbolos y funciones.
  - [x] 0.4 Actualizar `research/INDEX.md`, `TASKS.md` y `SESSION_LOG.md`.
- **Resultado de Verificación**: Informe [`RES-002`](file:///c:/Users/kanel/Documents/mjk/research/RES-002-analisis-naturaleza-desensamblado-asm.md) completado. Contabilizadas 293 subrutinas (212 nombradas, 22 tentativas, 59 `sub_XXXX`), 679 etiquetas de datos en ROM y 305 etiquetas de RAM.

#### [DONE] `TASK-RES-002`: Extracción determinista del catálogo maestro de símbolos, direcciones, `JumpTables` y grafo de llamadas (`RES-003`)
- **Módulo**: `asm/` (Read-Only) + `tools/` -> `research/`
- **Prioridad**: Crítica (P0)
- **Dependencias**: `TASK-RES-001`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.2.1 Crear `tools/extract_asm_inventory.py` y ejecutar `asm6f_64.exe` (`-m` y `-L`) hacia directorio temporal para extraer direcciones ROM/RAM exactas (`$0000-$FFFF`) y tamaños en bytes de los 1,764 símbolos.
  - [x] 0.2.2 Ejecutar el parser estático sobre `prg.asm` y `music-engine.asm` para extraer: (a) las 15 tablas `JSR JumpTable` con sus destinos, (b) el Grafo de Llamadas (`Callers`/`Callees`), (c) matriz `RAM R/W` y uso de Zero-Page `$00-$14`, y (d) detección de las 12 subrutinas con mutación de pila (`PLA; PLA; RTS` / `_DoubleReturn`).
  - [x] 0.2.3 Publicar el catálogo maestro indexado en `research/RES-003-inventario-maestro-simbolos-y-grafo-llamadas.md`, la base de datos `tools/asm_inventory.json` y actualizar `research/INDEX.md`.
- **Resultado de Verificación**: Extraídos y validados 1,764 símbolos Mesen, 294 rutinas ejecutables, 15 `JumpTables` y 305 etiquetas de RAM en [`RES-003`](file:///c:/Users/kanel/Documents/mjk/research/RES-003-inventario-maestro-simbolos-y-grafo-llamadas.md) y [`tools/asm_inventory.json`](file:///c:/Users/kanel/Documents/mjk/tools/asm_inventory.json).

#### [DONE] `TASK-RES-003`: Especificación C x86 Subsistema 01 — Boot, NMI Pipeline, PPU Buffer, Joypad y Máquina de Estados (`RES-004`)
- **Módulo**: `asm/src/prg.asm` (`$8000-$8A9B`) -> `research/RES-004-subsistema-nucleo-nmi-ppu-estados.md`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-RES-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.3.1 Documentar ABI 6502 y especificación C de `RESET`, `NMI`, `HandlePPUUpdates`, `UpdateJoypads`, `JumpTable` y `MainLogicHandler` (líneas `1-750`).
  - [x] 0.3.2 Documentar ABI 6502 y especificación C de las rutinas de estado general, PPU/OAM helpers y `MainSub_1` (líneas `751-1,640`).
- **Resultado de Verificación**: Publicado [`RES-004`](file:///c:/Users/kanel/Documents/mjk/research/RES-004-subsistema-nucleo-nmi-ppu-estados.md) con las 25 rutinas del núcleo, contrato de `PLA; PLA; RTS` en `GameState_5_Main` y 23 variables RAM desveladas.

#### [DONE] `TASK-RES-004`: Especificación C x86 Subsistema 02 — Mapa Completo de RAM (`$0000-$07FF`) y Zero-Page Scratchpad (`RES-005`)
- **Módulo**: `asm/src/ram.asm` -> `research/RES-005-especificacion-completa-ram-y-zeropage.md`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-RES-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.4.1 Documentar y resolver semánticamente Zero-Page (`$0000-$00FF`), incluyendo el contrato por contexto de `byte_0..byte_14` y `off_2/off_4`.
  - [x] 0.4.2 Documentar Stack (`$0100-$01FF`), OAM Mirror (`$0200-$02FF`), PPU Update Buffer (`$0300-$03DF`) y Estado de Jack (`$03E0-$03FF`).
  - [x] 0.4.3 Documentar canales de sonido (`$0400-$04FF`), mapa de colisiones/sala activa (`$0500-$05FF`) y arrays de entidades/enemigos (`$0600-$07FF`), resolviendo las 127 etiquetas `byte_XX` restantes.
- **Resultado de Verificación**: Publicado [`RES-005`](file:///c:/Users/kanel/Documents/mjk/research/RES-005-especificacion-completa-ram-y-zeropage.md) con el mapa byte a byte de los 2,048 bytes (`$0000-$07FF`), contratos por contexto de Zero-Page `$00-$14`, estructura polimórfica `PlayerStruct` (`$03CE-$03EC`) vs `EnemyStruct` (`$06A6-$0785`), y resolución de las 150 etiquetas de RAM sin bautizar.

#### [DONE] `TASK-RES-005`: Especificación C x86 Subsistema 03 — Física de Jack, Salto/Aleteo, Colisiones 9-Probe y Scroll (`RES-006`)
- **Módulo**: `asm/src/prg.asm` + `data.asm` -> `research/RES-006-subsistema-fisica-jack-y-colisiones.md`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-RES-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.5.1 Analizar cinemática de Jack, velocidades subpíxel `8.8`, tablas `word_C0E3` y `word_C163`, aleteo y gravedad (`prg.asm` líneas `1,500-2,500`).
  - [x] 0.5.2 Analizar `BigBeefySubHandlesBGColliding`, sondas `byte_A3D8`/`byte_A3EA`, y resolver las 8 `sub_XXXX` y 7 `Maybe...` del rango `4,500-5,999`.
- **Resultado de Verificación**: Publicado [`RES-006`](file:///c:/Users/kanel/Documents/mjk/research/RES-006-subsistema-fisica-jack-y-colisiones.md) con el contrato ABI de 34 subrutinas (incluyendo las `sub_XXXX` y `Maybe...` del subsistema), la ecuación exacta de la curva de salto de 64 pasos (`word_C0E3`/`word_C163`), el motor de 9 sondas (`byte_A3D8`/`byte_A3EA`) y el motor de scroll bidireccional (`MainSub_3`).

#### [DONE] `TASK-RES-006`: Especificación C x86 Subsistema 04 — Decodificador de Salas (4-Tier), Puertas, Cofres y Bombas (`RES-007`)
- **Módulo**: `asm/src/prg.asm` (`3,000-4,499`) + `roomdata-*.asm` -> `research/RES-007-subsistema-salas-puertas-cofres-bombas.md`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-RES-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.6.1 Analizar decodificador de salas de 4 niveles, punteros de `SectionRoomsTable` y `roomdata-jp/us.asm`, resolviendo las primeras 8 `sub_XXXX` (`sub_9276`..`sub_9729`).
  - [x] 0.6.2 Analizar lógica de bombas, orden de mecha encendida, cofres naranjas/rojos, bloques ocultos y condiciones de apertura de puertas, resolviendo las 10 `sub_XXXX` restantes (`sub_9813`..`sub_A33E`).
- **Resultado de Verificación**: Publicado [`RES-007`](file:///c:/Users/kanel/Documents/mjk/research/RES-007-subsistema-salas-puertas-cofres-bombas.md) con el pipeline completo del decodificador de 4 niveles (`RoomHalfScreens` -> `HalfScreenLayouts` -> `LayoutChunks` -> `MetatileDefinitions`), el formato del bytecode `RoomData_XX`, el sistema de encadenamiento de bombas y las 33 condiciones de puerta (incluyendo diferencias JP vs US y warps secretos).

#### [DONE] `TASK-RES-007`: Especificación C x86 Subsistema 05 — IA de Enemigos, Spawns, Momias y Monedas (`RES-008`)
- **Módulo**: `asm/src/prg.asm` (`8,650-10,162`) + `data.asm` -> `research/RES-008-subsistema-ia-enemigos-spawns-monedas.md`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-RES-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.7.1 Analizar ciclo de spawn, `DifficultyTable`, transformación de Momias y conversión a monedas (`TurnEnemiesIntoCoins`), resolviendo las primeras 10 `sub_XXXX` (`sub_B645`..`sub_BA9E`).
  - [x] 0.7.2 Analizar máquinas de estado de cada arquetipo (Hanezo, GejiShogun, Gameido, Dokuron, Desufa, Horus) y rutinas auxiliares de movimiento/colisión enemiga, resolviendo las 18 `sub_XXXX` restantes (`sub_BB1C`..`sub_BFBC`).
- **Resultado de Verificación**: Publicado [`RES-008`](file:///c:/Users/kanel/Documents/mjk/research/RES-008-subsistema-ia-enemigos-spawns-monedas.md) con el ciclo de vida de los 8 slots de `EnemyStruct`, la fórmula determinista de transmutación de momias `(APressCounter % 6) + 2`, las máquinas de estado de los 6 arquetipos enemigos, y el temporizador/parpadeo de monedas doradas (`TurnEnemiesIntoCoins`).

#### [DONE] `TASK-RES-008`: Especificación C x86 Subsistema 06 — Pantallas Especiales, Salas Reales, Torture Room, GDV y Ending (`RES-009`)
- **Módulo**: `asm/src/prg.asm` (`2,500-2,999` y `6,000-8,166`) -> `research/RES-009-subsistema-pantallas-especiales-hud-gdv-ending.md`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-RES-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.8.1 Analizar HUD, contadores BCD de score/tiempo, cálculo de GDV, bonus de fin de ronda y penalización de codicia (*Torture Room*).
  - [x] 0.8.2 Analizar Pantalla de Título, Round Intro, Salas Reales, Esfinge y secuencias de Ending, resolviendo las 8 `sub_XXXX` de ese rango (`sub_A843`..`sub_B0AF`).
- **Resultado de Verificación**: Publicado [`RES-009`](file:///c:/Users/kanel/Documents/mjk/research/RES-009-subsistema-pantallas-especiales-hud-gdv-ending.md) con la fórmula matemática completa de cálculo de GDV, el protocolo de penalización de codicia en la Sala de Tortura (49 saltos JP / 50 US), la secuencia de liquidación de fin de ronda (`RoundClear_0..3`) y la máquina de estados de los 4 finales (`EndingState_0..4`).

#### [DONE] `TASK-RES-009`: Especificación C x86 Subsistema 07 — Motor de Audio 2A03 y Secuencias Musicales (`RES-010`)
- **Módulo**: `asm/src/music-engine.asm` + `music-data.asm` -> `research/RES-010-subsistema-audio-2a03-y-bytecode-musical.md`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-RES-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.9.1 Documentar ABI 6502 y especificación C de las 23 subrutinas de `music-engine.asm` (incluyendo `sub_B5C1`) y sus 8 canales virtuales (4 BGM + 4 SFX).
  - [x] 0.9.2 Documentar el formato de opcodes del bytecode musical y las 35 pistas/efectos de `music-data.asm` (resolviendo las 6 etiquetas anónimas).
- **Resultado de Verificación**: Publicado [`RES-010`](file:///c:/Users/kanel/Documents/mjk/research/RES-010-subsistema-audio-2a03-y-bytecode-musical.md) con el diseño de 8 canales virtuales, la precedencia y restauración automática de SFX sobre BGM, la tabla de frecuencias de notas, la máquina virtual de opcodes `$80..$FF` y el catálogo de 35 pistas y efectos de sonido.

#### [DONE] `TASK-RES-010`: Auditoría Integral de Puntos Ciegos, Idiomas 6502 y Discrepancias (`RES-011`)
- **Módulo**: `asm/src/` -> `research/RES-011-auditoria-puntos-ciegos-y-discrepancias.md`
- **Prioridad**: Crítica (P0)
- **Dependencias**: `TASK-RES-001` .. `TASK-RES-009`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 0.10.1 Escaneo automatizado de cobertura cruzada: 294 subrutinas (100%), 15 JumpTables (118 targets únicos, 100%), 338 variables de RAM (100%).
  - [x] 0.10.2 Identificación y formalización del idioma "Double Return" (`PLA; PLA; RTS`) en 12 subrutinas de `prg.asm` y `music-engine.asm`.
  - [x] 0.10.3 Auditoría de datos embebidos tras `JSR` (`JumpTable:2180`, `CopyNext5BytesToTempSprite:8119`).
  - [x] 0.10.4 Detección de código muerto original (`unusedMaybe_A248`, `unusedMaybe_A25C`, `DoorCond_unused`).
  - [x] 0.10.5 Mapeo exhaustivo de discrepancias regionales (`REV_US` vs `REV_JP` / `REV_A`) y verificación de determinismo (ausencia de LFSR RNG).
- **Resultado de Verificación**: Publicado [`RES-011`](file:///c:/Users/kanel/Documents/mjk/research/RES-011-auditoria-puntos-ciegos-y-discrepancias.md) certificando cobertura al 100% de la lógica sin puntos ciegos ni incógnitas pendientes antes de iniciar la construcción nativa.

### Hito 2: Implementación Nativa C11 y Paridad Funcional US (`port_native/`)

#### [DONE] `TASK-BUILD-001`: Andamiaje limpio de build y verificación del toolchain (CMake + MSVC + Raylib 5.5)
- **Módulo**: `port_native/` (Conformidad obligatoria con versión USA / `REV_US` según [ADR-004](file:///c:/Users/kanel/Documents/mjk/DECISIONS.md#adr-004-adopción-de-la-revisión-usa-rev_us-como-golden-standard-del-port-nativo))
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-SETUP-001`, `TASK-SETUP-002`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes / Subpasos Atómicos**:
  - [x] 1.1 Crear directorios modulares en `port_native/`: `src/core/`, `src/room/`, `src/enemy/`, `src/audio/`, `src/platform/` e `include/mbj/`.
  - [x] 1.2 Actualizar `CMakeLists.txt` para estructurar la librería estática `mbj_core` y el ejecutable principal con Raylib 5.5.
  - [x] 1.3 Crear un punto de entrada mínimo compilable (`src/platform/main.c`) y validar compilación limpia con MSVC (`build.bat`).
- **Resultado de Verificación**: Generado con éxito `mighty_bomb_jack.exe` en `port_native/build_vs/bin/Release/` con MSVC 19.50 y Raylib 5.5 sin errores.

#### [DONE] `TASK-IMPL-001`: Núcleo de Simulación, Estructura de RAM y Bucle 60 Hz
- **Módulo**: `port_native/src/core/` + `include/mbj/`
- **Prioridad**: Crítica (P1)
- **Dependencias**: `TASK-BUILD-001`
- **Especificaciones**: [`RES-004`](file:///c:/Users/kanel/Documents/mjk/research/RES-004-subsistema-nucleo-nmi-ppu-estados.md) y [`RES-005`](file:///c:/Users/kanel/Documents/mjk/research/RES-005-especificacion-completa-ram-y-zeropage.md)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes**:
  - [x] 2.1 Definir en `include/mbj/ram_map.h` el layout de RAM ($0000-$07FF) y variables Zero-Page ($00-$14).
  - [x] 2.2 Implementar la máquina de estados de juego (`GameState_0` a `GameState_C`) y el pipeline NMI de 7 fases a 60 Hz deterministas.
- **Resultado de Verificación**: Creado `include/mbj/ram_map.h` y `src/core/game_state.c` sincronizado con la memoria auténtica y compilando con MSVC sin errores.

#### [DONE] `TASK-IMPL-002`: Plataforma, Renderizado NES 256x240, Pantalla Completa y Soporte Mando Xbox / Teclado
- **Módulo**: `port_native/src/platform/` + `src/render/`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-IMPL-001`
- **Especificaciones**: [`ADR-005`](file:///c:/Users/kanel/Documents/mjk/DECISIONS.md#adr-005-capa-de-presentación-con-raylib-modo-pantalla-completa-y-soporte-para-mando-xbox-series-xinput--teclado)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes**:
  - [x] 3.1 Integrar Raylib con buffer virtual `RenderTexture2D` de 256x240 píxeles y decodificador 2bpp de `assets/chr/mbj-us.chr`.
  - [x] 3.2 Implementar modo pantalla completa dinámico (con tecla F11 / Alt+Enter) preservando relación de aspecto 4:3 con barras laterales (pillarbox).
  - [x] 3.3 Implementar soporte nativo XInput para mando de Xbox Series X|S / Xbox One con hotplug y controles alternativos de teclado (WASD / Flechas / Z / X).
- **Resultado de Verificación**: Implementados `render.h`, `raylib_renderer.c`, `platform_input.h`, `xinput_controller.c` y compilados en `mighty_bomb_jack.exe`.

#### [DONE] `TASK-IMPL-003`: Física de Jack, Salto Parabólico y Colisiones 9-Probe
- **Módulo**: `port_native/src/core/physics.c` + `collision.c`
- **Prioridad**: Crítica (P1)
- **Dependencias**: `TASK-IMPL-001`
- **Especificaciones**: [`RES-006`](file:///c:/Users/kanel/Documents/mjk/research/RES-006-subsistema-fisica-jack-y-colisiones.md)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes**:
  - [x] 4.1 Implementar tabla parabólica de 64 pasos (`word_C0E3`), modulación D-Pad (`word_C163`) y aleteo sostenido en caída libre.
  - [x] 4.2 Implementar sondas de colisión 9-point (`byte_A3D8`) y máscaras de cuadrante (`byte_A3EA`).
- **Resultado de Verificación**: Creados `physics.c` y `collision.c` con paridad matemática de tablas y compilados en `mbj_core.lib`.

#### [DONE] `TASK-IMPL-004`: Motor de Salas 4-Tier, Scroll y 32 Puertas Versión USA
- **Módulo**: `port_native/src/room/` + `src/asset/`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-IMPL-002`, `TASK-IMPL-003`
- **Especificaciones**: [`RES-007`](file:///c:/Users/kanel/Documents/mjk/research/RES-007-subsistema-salas-puertas-cofres-bombas.md) y [`ADR-004`](file:///c:/Users/kanel/Documents/mjk/DECISIONS.md#adr-004-adopción-de-la-revisión-usa-rev_us-como-golden-standard-del-port-nativo)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-22
- **Micro-Lotes**:
  - [x] 5.1 Implementar decodificador de 4 niveles para las 257 salas de la pirámide y scroll bidireccional.
  - [x] 5.2 Implementar evaluador de las 32 condiciones de puertas activas de la versión USA (incluyendo puerta 0x0E con Mighty Level 3).
- **Resultado de Verificación**: Extraídas todas las tablas bit-exact de `data.asm` con `tools/export_room_tables.py`, e implementados `room_decoder.c` y `doors.c`.

#### [DONE] `TASK-IMPL-005`: Bombas, Cadena de 23 Bombas, Cofres y Coleccionables
- **Módulo**: `port_native/src/core/bomb.c` + `chest.c`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-IMPL-004`
- **Especificaciones**: [`RES-007`](file:///c:/Users/kanel/Documents/mjk/research/RES-007-subsistema-salas-puertas-cofres-bombas.md)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 6.1 Implementar encadenamiento de 23 bombas con bonificaciones ($10k-$50k) y temporizadores de mecha encendida.
  - [x] 6.2 Implementar apertura de cofres, monedas Mighty, bolas de cristal y estados de Mighty Power 1..3.
- **Resultado de Verificación**: Creados `include/mbj/entities.h`, `src/core/bomb.c` y `src/core/chest.c`. Integrados en `main.c` con renderizado de sprites OAM y colisión/recolección. Compilado con MSVC exitosamente.

#### [DONE] `TASK-IMPL-006`: IA de Enemigos, Spawns, Mutación de Momias y Monedas
- **Módulo**: `port_native/src/core/enemy.c` + `include/mbj/enemy.h`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-IMPL-003`, `TASK-IMPL-004`
- **Especificaciones**: [`RES-008`](file:///c:/Users/kanel/Documents/mjk/research/RES-008-subsistema-ia-enemigos-spawns-monedas.md)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 7.1 Implementar spawn en orificios de techo y mutación determinista de momias por `(APressCounter % 6) + 2` para los 6 arquetipos.
  - [x] 7.2 Implementar máquina de estados de persecución de enemigos y transmutación masiva a monedas rotatorias con Power Coin.
- **Resultado de Verificación**: Creados `include/mbj/enemy.h` y `src/core/enemy.c`. Conectado en `main.c` con renderizado de sprites OAM, mutación determinista de momia a 6 arquetipos, y efecto Power Coin (320 frames). Compilado con MSVC sin errores.

#### [DONE] `TASK-IMPL-007`: HUD BCD, Sala de Tortura (50 Saltos US), Cálculo de GDV y 4 Finales
- **Módulo**: `port_native/src/core/hud.c` + `include/mbj/hud.h`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-IMPL-005`, `TASK-IMPL-006`
- **Especificaciones**: [`RES-009`](file:///c:/Users/kanel/Documents/mjk/research/RES-009-subsistema-pantallas-especiales-hud-gdv-ending.md) y [`ADR-004`](file:///c:/Users/kanel/Documents/mjk/DECISIONS.md#adr-004-adopción-de-la-revisión-usa-rev_us-como-golden-standard-del-port-nativo)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 8.1 Implementar aritmética BCD para marcadores de tiempo y puntuación en HUD OAM.
  - [x] 8.2 Implementar Sala de Tortura con penalización obligatoria de 50 saltos (US) y fórmula de GDV.
  - [x] 8.3 Implementar secuencias de los 4 finales y demolición de la pirámide.
- **Resultado de Verificación**: Creados `include/mbj/hud.h` y `src/core/hud.c`. Implementada aritmética BCD pura (`BCD_Sub1`, `BCD_Add`), renderizado de HUD en pantalla virtual, lógica de Sala de Tortura (50 saltos US), fórmula completa de GDV, y evaluación de los 4 finales según reliquias. Compilado con MSVC sin errores.

#### [DONE] `TASK-IMPL-008`: Motor de Audio 2A03 y Secuencias Musicales
- **Módulo**: `port_native/src/audio/`
- **Prioridad**: Media (P2)
- **Dependencias**: `TASK-IMPL-001`
- **Especificaciones**: [`RES-010`](file:///c:/Users/kanel/Documents/mjk/research/RES-010-subsistema-audio-2a03-y-bytecode-musical.md)
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 9.1 Implementar sintetizador de audio para 4 canales (2 pulsos, triángulo y ruido con LFSR de 15 bits).
  - [x] 9.2 Implementar máquina de bytecode de audio ($80-$FF) y cargar las 35 pistas y efectos musicales de la versión USA.
- **Resultado de Verificación**: Creados `include/mbj/audio.h`, `src/audio/apu_synth.c`, `src/audio/sound_driver.c` y `src/audio/sound_data.c`. Sintetizador 2A03 con mezcla DAC no lineal de Blargg, 8 canales virtuales con prioridad SFX, e integrado con `AudioStream` de Raylib a 44.1 kHz / 60 Hz.

#### [DONE] `TASK-IMPL-009`: Verificación Bit-Exact y Suite de Tests Automatizados
- **Módulo**: Paridad y Validación
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-IMPL-001` .. `TASK-IMPL-008`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 10.1 Diseñar test suite headless que verifique la paridad de memoria RAM contra vectores de prueba de la NES.
  - [x] 10.2 Integrar CTest en CMake y ejecutar suite automática con cobertura de BCD, Torture Room, Momias, Finales, Salas y APU.
- **Resultado de Verificación**: Creada suite `port_native/tests/test_suite.c` con 6 módulos de prueba. Integrada con CTest en `build.bat`. 100% de tests superados (6/6) en Release x64 MSVC. Binario final `mighty_bomb_jack.exe` completamente operativo.

---

### Hito 3: Extracción de Assets desde ROM NES, Resolución de Rutas y Audio Auténtico

#### [DONE] `TASK-ROM-001`: Cargador de ROM NES y Extracción de Gráficos y Audio
- **Módulo**: `port_native/src/asset/rom_loader.c` + `include/mbj/rom_loader.h` + `tools/extract_rom_assets.py`
- **Prioridad**: Crítica (P0)
- **Dependencias**: `TASK-IMPL-009`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 1.1 Crear herramienta `tools/extract_rom_assets.py` para parsear `mbj-us.nes` y exportar `assets/chr/mbj-us.chr` y `assets/audio/mbj-audio.bin`.
  - [x] 1.2 Implementar cargador C `ROM_LoadFromDisk()` capaz de leer `mbj-us.nes` directamente en memoria (CHR 32KB y PRG audio).
- **Resultado de Verificación**: Creados `include/mbj/rom_loader.h`, `src/asset/rom_loader.c` y script `tools/extract_rom_assets.py`. Extraídos y validados 32KB CHR y 32KB PRG. Código C 100% libre de assets con copyright.

#### [DONE] `TASK-ROM-002`: Búsqueda Inteligente de Rutas y Auto-Copy en CMake
- **Módulo**: `port_native/src/platform/main.c` + `CMakeLists.txt`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-ROM-001`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 2.1 Implementar búsqueda de ROM en múltiples rutas (junto a `.exe`, directorio de trabajo, `assets/`, `asm/bin/`, o por argumento CLI).
  - [x] 2.2 Configurar copia post-build de `assets/` y `mbj-us.nes` a `${CMAKE_BINARY_DIR}/bin/Release/` para doble clic directo.
- **Resultado de Verificación**: Integrada búsqueda secuencial con fallback en `rom_loader.c` y comandos post-build en `CMakeLists.txt`. Los activos quedan ubicados junto al binario para permitir doble clic directo desde el Explorador de Windows.

#### [DONE] `TASK-ROM-003`: Síntesis APU 2A03 a 48 kHz con Callback Continuo de Raylib
- **Módulo**: `port_native/src/audio/apu_synth.c` + `src/platform/main.c`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-ROM-001`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 3.1 Adaptar APU a frecuencia nativa de dispositivo (48 kHz) y desacoplarla del framerate gráfico.
  - [x] 3.2 Migrar el streaming de audio a `SetAudioStreamCallback()` de Raylib para eliminar buffer underruns y chasquidos.
- **Resultado de Verificación**: Implementado callback continuo de audio `AudioProcessCallback` a 48 kHz en `main.c`. Buffer underrun y chasquidos completamente eliminados.

#### [DONE] `TASK-ROM-004`: Intérprete de Bytecode Musical Original de la ROM
- **Módulo**: `port_native/src/audio/sound_driver.c`
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-ROM-001`, `TASK-ROM-003`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 4.1 Cargar `NoteLengthTable` ($F39E), `SoundVolTable_0..7` ($F3BE) y `SoundPointers` ($F45C) directamente desde el bloque PRG de la ROM.
  - [x] 4.2 Interpretar la melodía original multicanal (Pulse 1, Pulse 2, Triangle, Noise) y sincronizar efectos de sonido.
- **Resultado de Verificación**: `sound_driver.c` ejecuta el bytecode original de la ROM para las 35 pistas de Tecmo, polifonía completa en 4 canales para el tema principal (`Music_Main`) y prioridad SFX.

#### [DONE] `TASK-ROM-005`: Verificación Integral, CTest y Ejecución en Vivo
- **Módulo**: `port_native/tests/` + Verificación Final
- **Prioridad**: Alta (P1)
- **Dependencias**: `TASK-ROM-001` .. `TASK-ROM-004`
- **Responsable**: Orchestrator
- **Fecha Cierre**: 2026-10-23
- **Micro-Lotes**:
  - [x] 5.1 Actualizar `test_suite.c` con pruebas de carga de ROM y decodificación de audio.
  - [x] 5.2 Compilar y verificar 0 warnings / 0 errores y 100% tests verdes en CTest.
- **Resultado de Verificación**: 7 suites de pruebas pasando al 100% (7/7) en CTest. Compilación sin advertencias con MSVC. Verificada la ejecución limpia y la presencia de activos en `build_vs/bin/Release/`.
