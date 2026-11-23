# Mighty Bomb Jack - Operational Constitution & Reverse Engineering Governance

Este archivo define las reglas globales, permisos de directorios, roles de agentes y el protocolo del **Sistema de Memoria Persistente**. Todos los orquestadores, subagentes y herramientas automáticas que operen en este workspace deben cumplir estrictamente estas directrices.

---

## 1. Gobernanza de Directorios y Permisos

```
+---------------------------------------------------------------------------------------------------+
|                                       WORKSPACE ROOT (mjk/)                                       |
|                                                                                                   |
|  [READ-ONLY REFERENCE]                                   [ACTIVE IMPLEMENTATION - FASE 7]         |
|  +--------------------+                                  +--------------------+                   |
|  |       asm/         | <-- Golden Ground Truth          |       src/         | <-- Source Port   |
|  |  (Atributo +R OS   |     (Disassembly 6502)           |  (Núcleo C99       |     Educacional en|
|  |   + Regla AGENTS)  |     Inmutable                    |   + Frontend SDL)  |     C (Fase 7 R/W)|
|  +--------------------+                                  +--------------------+                   |
|                                                                                                   |
|  [PERSISTENT KNOWLEDGE BASE & SPECIFICATIONS]                                                     |
|  * research/          -> Base de conocimiento y especificación funcional (R/W)                    |
|    - scope/           -> Entregables de Fase 0 (alcance, layout de ROM, plan)                     |
|    - inventories/     -> Entregables de Fase 1 (variables, subrutinas, tablas ROM)                |
|    - control_flow/    -> Entregables de Fase 2 (grafo de llamadas, I/O hardware, NMI)             |
|    - contracts/       -> Entregables de Fase 3 (máquina de estados, contratos de comportamiento)  |
|    - audit/           -> Entregables de Fase 4 (matriz de trazabilidad, reporte de auditoría)     |
|    - INDEX.md         -> Índice maestro de investigaciones y catálogo técnico                     |
|    - RES-*.md         -> Informes técnicos detallados enriquecibles                               |
|                                                                                                   |
|  [PERSISTENT MANAGEMENT LEDGER - SINGLE SOURCE OF TRUTH]                                          |
|  * project/           -> Directorio centralizado de gestión y estado del proyecto (R/W)           |
|    - TASKS.md         -> Matriz formal de tareas con micro-lotes y checkpoints                     |
|    - PROJECT_STATUS.md-> Estado actual, fase activa, cobertura y métricas honestas                |
|    - DECISIONS.md     -> Registro de Decisiones de Arquitectura (ADR) técnicas e invariantes      |
|    - SESSION_LOG.md   -> Bitácora continua de intervenciones y handoffs entre sesiones            |
|                                                                                                   |
|  [AUTOMATION & STATIC ANALYSIS TOOLS]                                                             |
|  * tools/             -> Scripts de análisis estático 6502, AST parsing y extracción (R/W)        |
|                                                                                                   |
|  [HISTORICAL BACKUPS]                                                                             |
|  * archive/           -> Copias congeladas de iteraciones anteriores (solo lectura)               |
|                                                                                                   |
|  [WORKSPACE SKILLS & CONFIGURATION]                                                               |
|  * .agents/skills/    -> Skills operativas (ej. nes-functional-reconstruction)                    |
|  * AGENTS.md          -> Constitución raíz del workspace                                          |
+---------------------------------------------------------------------------------------------------+
```

### Reglas de Acceso:
1. **`asm/` -> SOLO LECTURA (READ-ONLY)**:
   - Contiene el desmontaje original del cartucho NES de 1987 (*Tecmo*).
   - Es la **Fuente de Verdad Absoluta** (Golden Ground Truth).
   - **PROHIBICIÓN ESTRICTA**: Ningún agente ni proceso debe intentar crear, modificar, renombrar ni eliminar archivos dentro de `asm/`. Toda operación sobre `asm/` debe ser de lectura/inspección.
2. **`src/` -> IMPLEMENTACIÓN NATIVA EN C (READ-WRITE - FASE 6 Y FASE 7 AUTORIZADAS)**:
   - Contiene la reimplementación en C99/C11 del source port basada estrictamente en los contratos auditados de `research/contracts/` y el desensamblado en `asm/`.
   - Dividida en `src/core/` (simulación pura y determinista sin dependencias) y `src/platform/` (frontend Win32/GDI/WaveOut/XInput para Windows, mandos Xbox Series y teclado).
   - El código obsoleto de la iteración previa ha sido archivado en `archive/iteration_01_c_port/port_native_deprecated/`.
3. **`project/` -> LECTURA Y ESCRITURA (READ-WRITE)**:
   - Contiene el ledger persistente activo (`TASKS.md`, `PROJECT_STATUS.md`, `DECISIONS.md`, `SESSION_LOG.md`).
   - Todos los agentes consultan y actualizan este directorio obligatoriamente en cada turno.
4. **`research/` -> LECTURA Y ESCRITURA (READ-WRITE)**:
   - Contiene todos los artefactos de especificación funcional organizados por fases (`scope/`, `inventories/`, `control_flow/`, `contracts/`, `audit/`), así como los informes técnicos `RES-*.md` y el catálogo `INDEX.md`.
5. **`tools/` -> LECTURA Y ESCRITURA (READ-WRITE)**:
   - Contiene scripts y herramientas de análisis estático determinista sobre `asm/`.
6. **`archive/` -> SOLO LECTURA (READ-ONLY)**:
   - Preservación histórica de iteraciones previas.
7. **Raíz (`./`)**:
   - Reservada exclusivamente para `AGENTS.md` y assets multimedia originales de referencia. Ningún agente debe crear archivos sueltos en la raíz.

---

## 2. Regla Fundamental de Ingeniería Inversa y Evidencia Empírica

> **PROHIBICIÓN ESTRICTA DE SUPOSICIONES EN DECISIONES E IMPLEMENTACIÓN**
> Las suposiciones son lícitas únicamente como hipótesis o teorías preliminares de trabajo; pero el resultado final de cualquier decisión técnica, contrato funcional o implementación en código DEBE estar obligatoriamente respaldado y verificado con el código fuente ensamblador (`asm/`) y/o datos reales del emulador (`cynes`, trazas de RAM, OAM o PPU).

- Ningún agente puede implementar código de porting antes de que la especificación completa esté terminada, auditada y verificada contra el ensamblador.
- Toda conclusión y decisión debe clasificarse según la epistemología de 4 estados:
  - `[CONFIRMADA]`: Evidencia directa e irrefutable en el código 6502 (archivo y líneas citadas) y/o datos del emulador.
  - `[INFERIDA]`: Hipótesis/teoría de trabajo coherente pero pendiente de demostración en código o emulador. Prohibido codificar basándose en inferencias sin confirmar.
  - `[DESCONOCIDA]`: Sin evidencia suficiente; requiere análisis y extracción.
  - `[CONTRADICTORIA]`: Evidencias en conflicto; requiere resolución explícita contra el Ground Truth.
- Queda terminantemente prohibido basar conclusiones o decisiones finales en vídeos, capturas o suposiciones sin sustento directo en `asm/` o telemetría del emulador.

---

## 3. Protocolo de Micro-Lotes y Reanudación de Bajo Coste

Para garantizar que cualquier interrupción no provoque pérdida de contexto ni re-gasto de tokens al reanudar:

1. **Regla del Micro-Lote**: Cada tarea se desglosa en subtareas atómicas y acotadas (1 rutina, 1 subsistema o 1 tabla a la vez).
2. **Checkpoints Visibles en `project/TASKS.md`**: Toda tarea activa (`[IN_PROGRESS]`) o pausada (`[PAUSED]`) debe mantener actualizado su bloque de `CHECKPOINT ACTIVO`:
   ```markdown
   > **CHECKPOINT ACTIVO (Última actualización: YYYY-MM-DD HH:MM)**
   > - **Último subpaso completado**: Subpaso X.
   > - **Archivos modificados**: `ruta/archivo:líneas`.
   > - **Estado de verificación**: Verificado con script/test X.
   > - **Siguiente micro-acción inmediata**: Acción exacta a ejecutar.
   ```
3. **Protocolo Quirúrgico de Reanudación**:
   - Al despertar o reanudar el trabajo tras una pausa o cambio de contexto:
     1. Leer **únicamente** el bloque `CHECKPOINT ACTIVO` en `project/TASKS.md` y la última entrada de `project/SESSION_LOG.md`.
     2. Leer acotadamente las líneas de código indicadas.
     3. Ejecutar inmediatamente la "Siguiente micro-acción inmediata".
     4. **PROHIBIDO**: Explorar recursivamente directorios o leer archivos generales de forma redundante.

---

## 4. Ciclo de Vida por Fases (Skill: `nes-functional-reconstruction`)

1. **Fase 0: Preparación y Alcance** (`research/scope/`)
2. **Fase 1: Inventario Estático Exhaustivo** (`research/inventories/`)
3. **Fase 2: Flujo de Control, Datos y Hardware** (`research/control_flow/`)
4. **Fase 3: Reconstrucción Funcional y Contratos** (`research/contracts/`)
5. **Fase 4: Auditoría de Cobertura y Trazabilidad** (`research/audit/`)
6. **Fase 5: Autorización de Arquitectura** (Solo tras superar Fase 4)
7. **Fase 6: Implementación Incremental Verificada** (Solo tras Fase 5)
8. **Fase 7: Bug Fixing, Paridad Exacta y Refactorización** (Corrección de discrepancias frente a `asm/` y `bin/mbj-us.nes`)

---

## 5. Arquitectura Operativa Jerárquica Obligatoria y Flujo de Agentes

> **REGLA MANDATORIA DE FLUJO**: Para poder ejecutar CUALQUIER tarea en el workspace, es OBLIGATORIO respetar estrictamente la cadena de delegación. Queda TERMINANTEMENTE PROHIBIDO que el Chat o el Orquestador Principal ejecuten tareas de desarrollo o análisis directamente sin pasar por este flujo.

```
+---------------------------------------------------------------------------------------------------+
| [NIVEL 0: CHAT RECEPCIONISTA] - Modelo: Gemini Argon Medium                                       |
| - Rol: Captura las directrices del usuario humano y transforma/clarifica el prompt.               |
| - Comunica avances y decisiones clave al usuario.                                                 |
| - PROHIBIDO: Escribir código, modificar archivos de implementación o investigar directamente.    |
+---------------------------------------------------------------------------------------------------+
                                                  |  (Entrega prompt refinado)
                                                  v
+---------------------------------------------------------------------------------------------------+
| [NIVEL 1: ORQUESTADOR PRINCIPAL] - Modelo: Gemini Argon Medium                                    |
| - Rol: Guardián de la memoria persistente (`project/TASKS.md`, `PROJECT_STATUS.md`, `SESSION_LOG.md`).|
| - Confecciona formalmente la tarea (alcance, criterios de aceptación, estado [IN_PROGRESS]).       |
| - Despacha exactamente UNA sola tarea a la vez al Orquestador Secundario (Nivel 2).               |
| - Supervisa la homologación final antes de marcar [DONE], consolidar la memoria persistente y     |
|   realizar obligatoriamente un commit en Git con el ID y resumen de la tarea finalizada.          |
+---------------------------------------------------------------------------------------------------+
                                                  |  (Despacha 1 sola tarea formal)
                                                  v
+---------------------------------------------------------------------------------------------------+
| [NIVEL 2: ORQUESTADOR SECUNDARIO (TASK RUNNER)] - Modelo: Gemini Argon Medium                     |
| <-- EFÍMERO: NACE Y MUERE EN CADA TAREA                                                           |
| - Rol: Recibe la tarea del Orquestador Principal y gestiona las subtareas atómicas con agentes.   |
| - Descompone la tarea en micro-subtareas secuenciales.                                            |
| - Coordina y delega a los 3 especialistas de Nivel 3.                                             |
| - Al concluir y verificar con éxito, rinde informe al Nivel 1 y ES ELIMINADO.                     |
+---------------------------------------------------------------------------------------------------+
                                                  |
                     +----------------------------+----------------------------+
                     | (Delega extracción ASM)    | (Delega codificación C)    | (Delega verificación)
                     v                            v                            v
+-----------------------------+ +-----------------------------+ +-----------------------------+
| [NIVEL 3: asm_researcher]   | | [NIVEL 3: c_implementer]    | | [NIVEL 3: emulator_verifier]|
| Modelo: Gemini Flash Medium | | Modelo: Gemini Flash Medium | | Modelo: Gemini Flash Medium |
| - Experto en Ensamblador    | | - Experto en Implementar en | | - Experto en Control de     |
|   6502 (Solo lectura asm/). |   C (C99/C11 en src/).        |   Verificaciones y Emulador |
| - Extrae tablas, fórmulas,  | - Implementa la lógica limpia |   (cynes, trazas de memoria,|
|   tiles, direcciones y      |   según especificaciones      |   OAM/PPU, paridad real     |
|   offsets canónicos.        |   extraídas de ASM.           |   contra bin/mbj-us.nes).   |
| - Documenta hallazgos en    |                               |                             |
|   research/ para no repetir |                               |                             |
|   análisis.                 |                               |                             |
+-----------------------------+ +-----------------------------+ +-----------------------------+
```

### Reglas Invariantes de Ejecución y Rigor:
1. **Obligatoriedad del Flujo Jerárquico**:
   - Cada tarea DEBE ser confeccionada por el Orquestador Principal en `project/`, recibida por el Orquestador Secundario, e investigada/implementada/verificada por los 3 especialistas correspondientes (`asm_researcher`, `c_implementer`, `emulator_verifier`).
2. **Prohibición Total de Suposiciones en Decisiones**:
   - Formular teorías o hipótesis es lícito durante el análisis preliminar, pero NUNCA como decisión final.
   - El resultado de toda decisión de diseño, estructura, constante o lógica debe estar obligatoriamente respaldado por citas exactas del código fuente (`asm/src/prg.asm`, `asm/src/data.asm`) y/o telemetría empírica del emulador (`cynes`, volcado de registros, RAM o PPU).
3. **Capitalización y Documentación Persistente en `research/` (Anti-Reanálisis)**:
   - Todo trabajo de extracción, decodificación y análisis realizado por `asm_researcher` DEBE quedar documentado o actualizado en `research/` (en sus subdirectorios temáticos o en informes `RES-*.md`) para evitar tener que analizar lo mismo dos veces.
   - Cualquier agente puede y debe consultar esta base de conocimiento previa para acelerar el trabajo, pero la decisión final y la implementación deben estar siempre ratificadas y contrastadas directamente con el código fuente ensamblador (`asm/`).
4. **Tolerancia Cero a Constantes o Tiles Arbitrarios**:
   - Queda terminantemente prohibido hardcodear tiles (ej. `0x30` como placeholder), dimensiones de bounding boxes, velocidades o temporizadores que no provengan con cita exacta de archivo y número de línea de `asm/src/prg.asm` o `asm/src/data.asm`.
5. **Reinicio Obligatorio de Contexto**:
   - Ningún subagente de Nivel 2 o Nivel 3 puede ser reutilizado entre tareas distintas. Cada tarea o subtarea arranca con un agente recién creado con contexto virgen.
6. **Verificación en Ejecutable Real y Emulador**:
   - Ninguna tarea se considera cerrada basándose únicamente en pruebas unitarias sintéticas. Toda mecánica debe verificarse con capturas visuales, ejecución de `bin/mjk.exe` y trazas de paridad con `cynes`.
7. **Prohibición Total de Interacción Humana y Cortesía en Subagentes (Ahorro Estricto de Tokens)**:
   - El ÚNICO componente autorizado para interactuar con el usuario humano es el **Chat Principal (Nivel 0)**.
   - El Orquestador de Tarea (Nivel 2) y todos los subagentes especialistas de Nivel 3 (`asm_researcher`, `c_implementer`, `emulator_verifier`) tienen TERMINANTEMENTE PROHIBIDO generar saludos, despedidas, fórmulas de cortesía, preámbulos conversacionales o cualquier texto de interacción humana.
   - Toda su comunicación es estrictamente técnica y de máquina a máquina (M2M): invocación directa de herramientas, paso de datos estructurados, código limpio y reportes sintéticos de estado sin relleno conversacional para maximizar la eficiencia de tokens.
8. **Eliminación Obligatoria e Inmediata de Subagentes Finalizados (Cero Agentes Ociosos)**:
   - Todo subagente que concluya su entrega DEBE ser eliminado explícitamente (`manage_subagents kill`).
   - Queda TERMINANTEMENTE PROHIBIDO dejar subagentes simplemente detenidos o en estado `idle` en el workspace.
   - En Nivel 2: Tan pronto un especialista de Nivel 3 entrega su subtarea, el Orquestador Secundario debe eliminarlo inmediatamente antes de invocar al siguiente especialista.
   - En Nivel 1: Tan pronto el Orquestador Secundario entrega la tarea terminada y verificada, el Orquestador Principal debe eliminarlo de inmediato.
   - Se mantiene siempre el principio de entorno limpio: cero procesos o subagentes zombis.
9. **Commit Automático en Git al Finalizar Cada Tarea**:
   - El workspace dispone de un repositorio Git para el source port y la base de conocimiento (con exclusión de `asm/`, binarios y assets con copyright).
   - Al finalizar, verificar y homologar cualquier tarea (marcar `[DONE]` en `project/TASKS_DONE.md` y actualizar `project/PROJECT_STATUS.md` y `project/SESSION_LOG.md`), el **Orquestador Principal (Nivel 1)** DEBE realizar obligatoriamente un commit en Git de forma automática incluyendo el ID y título de la tarea en el mensaje del commit (ej. `git commit -m "feat(TASK-F6-XXX): <titulo de la tarea>"`).
10. **Asignación Estricta de Modelos (Model Tiering)**:
    - **Nivel 0 (Chat Recepcionista) y Nivel 1 (Orquestador Principal)**: Modelo de alta capacidad analítica y razonamiento profundo **Gemini Argon Medium** (Gemini Pro).
    - **Nivel 2 (Orquestador Secundario / Task Runner)**: Invocado con **Gemini Argon Medium** (`Model: 'pro'` o `'inherit'`).
    - **Nivel 3 (Subagentes Especialistas: `asm_researcher`, `c_implementer`, `emulator_verifier`)**: Invocados obligatoriamente con **Gemini Flash Medium** (`Model: 'flash'` en `invoke_subagent`). Asegura ejecución rápida y coste ultra-eficiente de tokens en micro-lotes atómicos.

---

## 6. Protocolo Especial de Fase 7: Verificación Caja Negra en Ventana Temporal y Guardián Anti-Regresión

Durante la **Fase 7 (Bug Fixing, Paridad de Comportamiento y Refactorización)**, para evitar el sobreajuste (*overfitting*), las regresiones en cascada y la creación de scripts desechables, todos los agentes deben operar exclusivamente con la suite de herramientas deterministas de `tools/`:

```
+---------------------------------------------------------------------------------------------------+
| [LABORATORIO INTERACTIVO DUAL (HUMANO)]                                                           |
| - Comando: `python tools/dual_play_recorder.py`                                                   |
| - Permite jugar simultáneamente a la ROM original (cynes) y al Port C99 con mando Xbox o teclado, |
|   y pulsar 'G' para grabar sesiones reales en `tests/tas/session_YYYYMMDD_HHMMSS.tas`.            |
+---------------------------------------------------------------------------------------------------+
                                                  |
                                                  v
+---------------------------------------------------------------------------------------------------+
| [PASO 0: INSPECTOR DE VENTANA TEMPORAL Y GUARDIÁN ANTI-REGRESIÓN] - CERO TOKENS LLM               |
| - Herramienta Oficial: `tools/inspect_parity.py`                                                  |
| - Ejecuta el TAS en ambos motores y extrae una ventana temporal [target - W, target + W].         |
| - Filtra desfases transitorios (`--grace-frames 8`) que se auto-resuelven en pocos frames.        |
| - Emite tabla en consola, `scratch/window_report.csv`, `scratch/window_report.json` y             |
|   captura visual comparativa Side-by-Side `scratch/diff_frame_XXXX.png`.                          |
| - Evalúa el Guardián Anti-Regresión (`scratch/oracle_history.json`).                              |
+---------------------------------------------------------------------------------------------------+
                                                  |
                                                  v
+---------------------------------------------------------------------------------------------------+
| [NIVEL 1: ORQUESTADOR PRINCIPAL] - Modelo: Gemini Argon Medium (Pro)                              |
| - Rol: Guardián de la memoria y progreso en `project/REFINEMENT_LOG.md`.                          |
| - Inspecciona la tabla de ventana temporal y la imagen `scratch/diff_frame_XXXX.png`.             |
| - Despacha la tarea quirúrgica al subagente efímero `parity_solver` con contexto visual/semántico.|
| - Supervisa la verificación del oráculo, actualiza el diario y ejecuta el commit en Git.         |
+---------------------------------------------------------------------------------------------------+
                                                  |  (Despacha micro-tarea quirúrgica)
                                                  v
+---------------------------------------------------------------------------------------------------+
| [NIVEL 2: parity_solver] - Modelo: Gemini Flash Medium                                            |
| <-- EFÍMERO: NACE Y MUERE EN CADA ITERACIÓN                                                       |
| - Rol: Resuelve exclusivamente el bug semántico/mecánico reportado en la ventana temporal.        |
| - PROHIBIDO crear scripts de Python ad-hoc; DEBE usar `python tools/inspect_parity.py`.           |
| - Consulta `research/CROSS_REFERENCE_INDEX.md` y `asm/src/prg.asm`.                               |
| - Aplica el parche en `src/core/` y recompila con `cmd.exe /c "build.bat build"`.                 |
| - Valida con `python tools/inspect_parity.py`. Si recibe `[CRITICAL REGRESSION]`, revierte        |
|   inmediatamente con `git checkout -- src/`.                                                      |
| - Reporta el avance de frames al Nivel 1 y ES ELIMINADO INMEDIATAMENTE (`manage_subagents kill`). |
+---------------------------------------------------------------------------------------------------+
```

### Comandos Oficiales de `tools/inspect_parity.py` (Tool para Agentes):

1. **Búsqueda automática de la primera divergencia persistente + Ventana Temporal + Captura PNG**:
   ```powershell
   python tools/inspect_parity.py --tas tests/tas/session_20261101_185107.tas --level 2 --window 15
   ```
2. **Inspección quirúrgica de un intervalo de frames específico (ej. analizar 20 frames antes y después del frame 1883)**:
   ```powershell
   python tools/inspect_parity.py --tas tests/tas/session_20261101_185107.tas --frame 1883 --window 20
   ```
3. **Auditoría incluyendo cinemática/posición de Jack con tolerancia de caja negra (ej. 6px)**:
   ```powershell
   python tools/inspect_parity.py --tas tests/tas/session_20261101_185107.tas --level 4 --tolerance-px 6 --window 15
   ```
4. **Reinicio de línea base del historial anti-regresión (al cambiar de TAS o de fase)**:
   ```powershell
   python tools/inspect_parity.py --tas tests/tas/session_20261101_185107.tas --level 2 --reset-history
   ```

### Reglas Invariantes de Fase 7:
1. **Prohibición de Scripts Desechables**:
   - Queda terminantemente prohibido que los agentes creen scripts temporales en Python para ejecutar `cynes` o `mjk_core.dll`. Toda inspección debe ejecutarse mediante `tools/inspect_parity.py`.
2. **Inspección Obligatoria de Ventana Temporal e Imagen Side-by-Side**:
   - Antes de modificar cualquier archivo en `src/core/`, el agente DEBE revisar la ventana de frames `[target - W, target + W]` y visualizar la captura `scratch/diff_frame_XXXX.png` (con `read_file`) para confirmar en qué sala, posición y situación visual real se encuentran ambos juegos.
3. **Obediencia Estricta al Guardián Anti-Regresión (`CRITICAL_REGRESSION` y `INFINITE_LOOP_ALERT`)**:
   - Si `tools/inspect_parity.py` devuelve código de salida `2` (`CRITICAL_REGRESSION`), el subagente tiene **PROHIBIDO** intentar "arreglar el arreglo": debe ejecutar inmediatamente `git checkout -- src/ include/` para restaurar el estado limpio previo.
   - Si devuelve `INFINITE_LOOP_ALERT` (3 intentos sin avanzar el horizonte), el subagente debe detenerse inmediatamente y devolver el control al Nivel 1 informando del bloqueo.
4. **Prioridad Semántica (Caja Negra) sobre Micro-Estado**:
   - Se priorizan las mecánicas observables (transición de salas, apertura de puertas, recogida de bombas, salto y colisiones reales) por encima de la coincidencia al milisegundo de subpíxeles internos. Los desfases transitorios menores a `--grace-frames` que convergen solos no deben parchearse.
5. **Consulta Mandatoria de `research/CROSS_REFERENCE_INDEX.md`**:
   - Todo agente o subagente (`parity_solver`) DEBE consultar en primer lugar `research/CROSS_REFERENCE_INDEX.md` al analizar una variable RAM o subrutina.
   - Queda estrictamente prohibido ejecutar búsquedas recursivas ciegas con grep en todo el proyecto si la variable o subrutina está indexada en este catálogo maestro.
6. **Registro en `project/REFINEMENT_LOG.md` y Commit Git por Avance**:
   - Cada empuje de horizonte verificado sin regresiones se registra en `project/REFINEMENT_LOG.md` y genera un commit Git con formato:
     `fix(macro): <resumen del bug resuelto> (frame X->Y)`.




