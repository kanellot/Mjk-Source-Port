# Plan de Análisis y Estrategia de Extracción Funcional: Mighty Bomb Jack

- **ID de Entregable**: `research/scope/analysis_plan.md`
- **Fase**: Fase 0 — Preparación y Alcance
- **Tarea**: `TASK-F0-003` (Subpaso 0.2)
- **Metodología**: Tool-Assisted Reverse Engineering + Contratos Lógicos Deterministas
- **Estado**: `[CONFIRMADA]`

---

## 1. Principios Metodológicos Fundamentales

El objetivo del análisis es derivar una **especificación funcional matemática y determinista** a partir de las 18.676 líneas de ensamblador en `asm/src/`. Para evitar las fallas de la iteración previa, el plan se rige por tres principios innegociables:

1. **Prohibición de Métricas de Vanidad**: Ninguna entidad (rutina o variable) se considera comprendida por el mero hecho de que su nombre aparezca en un documento. Debe existir un contrato de comportamiento verificado.
2. **Análisis Estático Automatizado (Tool-Assisted)**: La extracción de referencias cruzadas (XREFs), accesos a memoria RAM (Read/Write/RMW) y grafos de llamadas se realiza mediante scripts deterministas en Python (`tools/`), eliminando el error humano o la fatiga de contexto del LLM.
3. **Epistemología de 4 Estados**: Toda conclusión técnica se clasifica explícitamente como `[CONFIRMADA]`, `[INFERIDA]`, `[DESCONOCIDA]` o `[CONTRADICTORIA]`. Ninguna incertidumbre crítica puede quedar abierta antes de autorizar la reimplementación.

---

## 2. Hoja de Ruta de Herramientas Estáticas (`tools/`)

Para dar soporte a las Fases 1 a 4, se implementará y mantendrá la siguiente suite de análisis en `tools/`:

```
tools/
├── extract_asm_inventory.py   -> Extractor base de símbolos y JumpTables vía asm6f
├── asm_inventory.json         -> Base de datos serializada de 1.764 símbolos y 294 subrutinas
├── deep_asm_analyzer.py       -> Parser AST 6502 profundo:
│                                 - Matriz de accesos a RAM ($0000-$07FF) por tipo (R/W/RMW)
│                                 - Grafo de llamadas caller/callee completo
│                                 - Mapeo de subrutinas con manipulación de pila (PLA; PLA)
│                                 - Detección de código muerto y etiquetas anónimas
└── audit_traceability.py      -> Validador matemático de cobertura y trazabilidad bidireccional
```

---

## 3. Descomposición Modular por Subsistemas (Fases 1 a 3)

El código fuente se divide en **6 subsistemas funcionales autocontenidos**, garantizando que cada micro-lote aborde un dominio acotado:

```
+---------------------------------------------------------------------------------------------------+
|                            LOS 6 SUBSISTEMAS FUNCIONALES DEL JUEGO                                |
+---------------------------------------------------------------------------------------------------+
|  [SUBSISTEMA 1: NÚCLEO, BOOT, NMI Y MÁQUINA DE ESTADOS]                                           |
|  * Rango ROM: $8000-$8A9B (prg.asm:8-1636)                                                        |
|  * Componentes: Reset, pipeline 60 Hz NMI, buffers VRAM PPU, UpdateJoypads, GameState (0..$C).    |
+---------------------------------------------------------------------------------------------------+
|  [SUBSISTEMA 2: CINEMÁTICA, FÍSICA Y COLISIONES DE JACK]                                          |
|  * Rango ROM: $8845-$8D75, $8D94-$92AC, $C0E3-$C200 (prg.asm y data.asm)                         |
|  * Componentes: Salto 64 pasos (word_C0E3), aleteo, velocidad X (word_C163), sondas 9-probe.      |
+---------------------------------------------------------------------------------------------------+
|  [SUBSISTEMA 3: ESCENARIOS, DECODIFICADOR 4-TIER Y MECANISMOS DE SALAS]                          |
|  * Rango ROM: $92AD-$A247, $C201-$EBFF (prg.asm, data.asm, roomdata-us.asm)                      |
|  * Componentes: 257 salas, descompresión 4-tier, 33 puertas USA, cadena de 23 bombas y cofres.   |
+---------------------------------------------------------------------------------------------------+
|  [SUBSISTEMA 4: ENEMIGOS, INTELIGENCIA ARTIFICIAL Y CICLOS DE SPAWN]                              |
|  * Rango ROM: $A248-$B644, $B645-$BFBC (prg.asm)                                                 |
|  * Componentes: 6 arquetipos, slots RAM $0600-$07FF, mutación (A%6)+2, transmutación 320 frames.  |
+---------------------------------------------------------------------------------------------------+
|  [SUBSISTEMA 5: HUD, ARITMÉTICA BCD, SALA DE TORTURA, GDV Y FINALES]                             |
|  * Rango ROM: $8474-$8756, $86DB-$874D, $BFBD-$C0E2 (prg.asm)                                    |
|  * Componentes: Marcador BCD, temporizador 256 frames, penalización 50 saltos, fórmula GDV, 4 fin. |
+---------------------------------------------------------------------------------------------------+
|  [SUBSISTEMA 6: MOTOR DE AUDIO 2A03 Y BYTECODE MUSICAL TECMO]                                     |
|  * Rango ROM: $EC00-$FFFF (music-engine.asm, music-data.asm)                                      |
|  * Componentes: Síntesis 4 canales APU, cola SFX con prioridad, 14 melodías por bytecode.         |
+---------------------------------------------------------------------------------------------------+
```

---

## 4. Criterios de Aceptación por Fase (Gates de Salida)

Ninguna fase puede declararse completada sin superar formalmente su criterio de aceptación:

| Fase | Entregables Requeridos | Criterio de Aceptación Formal (Gate de Salida) |
| :--- | :--- | :--- |
| **Fase 0: Alcance** | `scope.md`, `rom_layout.md`, `repository_inventory.md`, `analysis_plan.md` | Alcance canónico delimitado, layout NROM-256 verificado contra `asm6f`, y herramientas planificadas. |
| **Fase 1: Inventario** | `code_inventory.csv`, `data_inventory.csv`, `memory_map.md`, `variable_usage.csv`, `unresolved_regions.md` | 100% de las 294 subrutinas y 338 variables clasificadas; cero referencias huérfanas sin categorizar. |
| **Fase 2: Grafos** | `control_flow.md`, `data_flow.md`, `hardware_model.md`, `unresolved_dependencies.md` | Pipeline intra-frame NMI documentado, las 15 JumpTables y 12 `PLA; PLA` resueltas; modelo de VRAM buffers. |
| **Fase 3: Contratos** | `state_machine.md`, `state_transition_matrix.csv`, `behavior_catalog.md`, `behavior_contracts/*` | Todo comportamiento observable dispone de contrato formal con precondiciones, pseudocódigo e invariantes. |
| **Fase 4: Auditoría** | `traceability_matrix.csv`, `coverage_report.md`, `unresolved_questions.md`, `audit_report.md` | Trazabilidad bidireccional completa (100% código cubierto $\leftrightarrow$ 100% contratos); cero incertidumbres críticas. |

---

## 5. Gestión de Riesgos y Mitigación

1. **Riesgo: Deriva de Contexto por Longitud de Conversación**:
   - *Mitigación*: Se preserva estrictamente el protocolo de micro-lotes de `project/TASKS.md`, manteniendo `CHECKPOINT ACTIVO` actualizado para permitir reanudar de inmediato con menos de 500 tokens.
2. **Riesgo: Rutinas con Control de Flujo Anómalo (Double Returns / Saltos en Pila)**:
   - *Mitigación*: En `prg.asm` se han detectado 12 subrutinas que ejecutan `PLA; PLA` para descartar el marco de retorno y salir directamente de la función llamadora. Estas rutinas se modelan explícitamente como salidas de excepción/aborto en los contratos.
3. **Riesgo: Dudas de Semántica en Lógica Compleja de IA**:
   - *Mitigación*: Como `asm/` compila una ROM bit-exacta, cualquier duda se validará empíricamente mediante traza dinámica en emulador (Mesen con Lua script de RAM watch).
