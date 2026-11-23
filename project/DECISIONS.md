# Registro de Decisiones de Arquitectura e Ingeniería (ADR)

Este registro documenta todas las decisiones arquitectónicas, metodológicas e invariantes técnicas del proyecto *Mighty Bomb Jack*.

---

## Índice de Decisiones (Iteración 2)

- [ADR-001: Adopción del Marco de Reconstrucción Funcional y Epistemología de Evidencia](#adr-001-adopción-del-marco-de-reconstrucción-funcional-y-epistemología-de-evidencia)
- [ADR-002: Congelación Total de la Implementación hasta Superación de Auditoría (Fase 4)](#adr-002-congelación-total-de-la-implementación-hasta-superación-de-auditoría-fase-4)
- [ADR-003: Organización Limpia del Workspace: Centralización en `project/`, `archive/` y Subcarpetas en `research/`](#adr-003-organización-limpia-del-workspace-centralización-en-project-archive-y-subcarpetas-en-research)
- [ADR-004: Preservación de la Revisión USA / NTSC (`REV_US`) como Versión Base Canónica](#adr-004-preservación-de-la-revisión-usa--ntsc-rev_us-como-versión-base-canónica)
- [ADR-005: Enfoque Tool-Assisted para Análisis Estático y Mitigación de Métricas de Vanidad](#adr-005-enfoque-tool-assisted-para-análisis-estático-y-mitigación-de-métricas-de-vanidad)
- [ADR-006: Arquitectura del Source Port en C, Extracción Clean-Room de Assets y Estándar Educacional](#adr-006-arquitectura-del-source-port-en-c-extracción-clean-room-de-assets-y-estándar-educacional)
- [ADR-007: Fidelidad de Paletas y Sprites Derivada del Ensamblador 6502 y PPU 2C02 (Anti-Heurística Visual)](#adr-007-fidelidad-de-paletas-y-sprites-derivada-del-ensamblador-6502-y-ppu-2c02-anti-heurística-visual)
- [ADR-008: Prohibición Estricta de Suposiciones en Decisiones y Respaldo Obligatorio en Código y Emulador](#adr-008-prohibición-estricta-de-suposiciones-en-decisiones-y-respaldo-obligatorio-en-código-y-emulador)
- [ADR-009: Capitalización Persistente del Análisis ASM en `research/` y Decisión Final en Código](#adr-009-capitalización-persistente-del-análisis-asm-en-research-y-decisión-final-en-código)
- [ADR-010: Prohibición de Interacción Humana en Subagentes y Comunicación Estricta M2M (Ahorro de Tokens)](#adr-010-prohibición-de-interacción-humana-en-subagentes-y-comunicación-estricta-m2m-ahorro-de-tokens)
- [ADR-011: Eliminación Obligatoria e Inmediata de Subagentes Finalizados (Cero Agentes Ociosos)](#adr-011-eliminación-obligatoria-e-inmediata-de-subagentes-finalizados-cero-agentes-ociosos)
- [ADR-012: Arquitectura del Sistema de Puertas, Evaluación de 33 Condiciones y Transición Topológica de Salas](#adr-012-arquitectura-del-sistema-de-puertas-evaluación-de-33-condiciones-y-transición-topológica-de-salas)
- [ADR-013: Metodología de Replay Diferencial de Golden Master y Empuje de Horizonte (Fase 7)](#adr-013-metodología-de-replay-diferencial-de-golden-master-y-empuje-de-horizonte-fase-7)

---

## ADR-001: Adopción del Marco de Reconstrucción Funcional y Epistemología de Evidencia

- **Fecha**: 2026-10-24
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  La iteración 1 intentó implementar el juego simultáneamente al análisis, resultando en un prototipo incompleto con lógica aproximada, suelo hardcodeado y falsos positivos en las pruebas unitarias.
- **Decisión**:
  Se adopta formalmente la skill `nes-functional-reconstruction` estructurada en un ciclo de vida de fases estrictas (Fase 0 a Fase 4). Toda afirmación técnica debe categorizarse en uno de los 4 estados de evidencia:
  - `[CONFIRMADA]`: Evidencia directa en el código 6502 (con citas de archivo y línea).
  - `[INFERIDA]`: Hipótesis consistente pero pendiente de corroboración.
  - `[DESCONOCIDA]`: Sin evidencia concluyente.
  - `[CONTRADICTORIA]`: Discrepancia identificada entre fuentes que debe investigarse.
- **Consecuencias**:
  Queda prohibido dar por cerrado un subsistema sin evidencia demostrable en el código fuente.

---

## ADR-002: Congelación Total de la Implementación hasta Superación de Auditoría (Fase 4)

- **Fecha**: 2026-10-24
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Escribir código de implementación antes de tener la especificación completa distorsiona las conclusiones, fomenta la invención de mecánicas no contrastadas y consume ciclos de testing sobre componentes ficticios.
- **Decisión**:
  El directorio `port_native/` queda **congelado en modo solo lectura**. Ningún agente orquestador o subagente escribirá código de implementación hasta que la Fase 4 (Auditoría de Cobertura y Trazabilidad) haya sido completada y certificada con 0 incertidumbres críticas pendientes.
- **Consecuencias**:
  El 100% del esfuerzo del proyecto se concentra en la extracción, especificación formal y modelado de datos y control del código 6502.

---

## ADR-003: Organización Limpia del Workspace: Centralización en `project/`, `archive/` y Subcarpetas en `research/`

- **Fecha**: 2026-10-24
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Los archivos de gestión y auditoría anteriores se encontraban dispersos en la raíz (`TASKS.md`, `PROJECT_STATUS.md`, `DECISIONS.md`, `SESSION_LOG.md`), saturando el espacio de trabajo.
- **Decisión**:
  1. La totalidad de los archivos de la Iteración 1 se respaldan en `archive/iteration_01_c_port/`.
  2. Los archivos activos de persistencia y seguimiento se trasladan a `project/` (`TASKS.md`, `PROJECT_STATUS.md`, `DECISIONS.md`, `SESSION_LOG.md`).
  3. Los entregables de especificación se organizan en subcarpetas dentro de `research/`: `scope/`, `inventories/`, `control_flow/`, `contracts/` y `audit/`.
  4. Los informes técnicos previos (`RES-001` a `RES-011`) se conservan en `research/` y se enriquecen con nuevos hallazgos.
  5. La raíz `./` queda despejada, preservando únicamente `AGENTS.md` (como regla raíz del sistema) y los vídeos de referencia.
- **Consecuencias**:
  Workspace limpio, modular, fácil de navegar e inmune a colisiones accidentales.

---

## ADR-004: Preservación de la Revisión USA / NTSC (`REV_US`) como Versión Base Canónica

- **Fecha**: 2026-10-24 (Ratificación de la decisión previa ADR-004 de Iteración 1)
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  El cartucho comercial de *Mighty Bomb Jack* tiene 3 revisiones ensamblables desde `asm/`: `REV_JP` (Famicom 1986), `REV_JPA` (Famicom Rev A) y `REV_US` (NES USA 1987).
- **Decisión**:
  Se mantiene la revisión USA (`REV_US`) como línea canónica de referencia funcional:
  - Textos y tipografía en inglés (Tecmo Inc. Los Angeles, CA).
  - Penalización de la Sala de Tortura fijada en 50 saltos (`$50` BCD).
  - Disposición de puertas, cofres y secretos específicos de la versión USA (incluyendo la puerta 0x0E con requisito de Mighty Level 3).
- **Consecuencias**:
  Cualquier discrepancia con las versiones japonesas se registrará como nota regional comparativa, pero no afectará a los contratos de comportamiento base.

---

## ADR-005: Enfoque Tool-Assisted para Análisis Estático y Mitigación de Métricas de Vanidad

- **Fecha**: 2026-10-24
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  En la iteración 1 se cometió el error de considerar 100% cubierta una subrutina si su nombre aparecía en un archivo markdown, lo que generó una falsa sensación de progreso.
- **Decisión**:
  Todo inventario, grafo de llamadas, matriz de uso de RAM (lectores/escritores) y resolución de saltos indirectos debe ser procesado o validado mediante herramientas programáticas en `tools/`. La auditoría de cobertura debe comprobar el mapeo de bloques de código reales, no meras coincidencias de texto.
- **Consecuencias**:
  Eliminación de falsos positivos en los reportes de progreso y garantía de rigor matemático en la cobertura de instrucciones.

---

## ADR-006: Arquitectura del Source Port en C, Extracción Clean-Room de Assets y Estándar Educacional

- **Fecha**: 2026-10-24
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Tras superar y certificar formalmente la Fase 4 (Auditoría), se autoriza el paso a la Fase 5 (Diseño de Arquitectura). El usuario ha especificado requisitos fundamentales: implementación en lenguaje C, política estricta de cero assets con copyright en el repositorio (estrategia BYOR: *Bring Your Own ROM* con extractor), base en la versión USA (`REV_US`), compatibilidad nativa con Windows, mandos de Xbox Series y teclado, y un estándar explícito de código limpio, modular y de valor educacional.
- **Decisiones Técnicas**:
  1. **Lenguaje y Estándar**: C99 / C11 estricto con enteros de ancho fijo (`<stdint.h>`, `<stdbool.h>`).
  2. **Arquitectura Desacoplada en Dos Capas**:
     - `src/core/`: Núcleo de simulación del juego en C puro, determinista, sin dependencias de sistemas de ventanas, bibliotecas externas ni APIs del SO.
     - `src/platform/`: Capa de abstracción de hardware (HAL) para Windows mediante SDL2/SDL3 (DirectX 11, XInput para mandos de Xbox Series, teclado y audio a baja latencia).
  3. **Política Clean-Room de Assets (BYOR)**:
     - El repositorio no distribuirá ningún archivo de audio, gráfico o tipografía con derechos de autor.
     - Se creará una herramienta extractora (`tools/asset_extractor`) que verificará el hash SHA-1 de `mbj-us.nes` y empaquetará los tiles CHR-ROM (32 KB) y secuencias sonoras en `assets.dat`.
  4. **Estándar de Código Limpio y Educacional**:
     - Prohibición de variables crípticas (`byte_E`, `$0048`, offsets anónimos). Toda entidad se representará mediante estructuras de datos semánticas (`JackPlayer`, `RoomObject`, `EnemyEntity`) y enumeraciones legibles.
     - Trazabilidad didáctica dual: Cada función y subsistema documentará en sus cabeceras el concepto de diseño de juegos subyacente y la referencia exacta a las líneas del ensamblador original (`asm/src/prg.asm`).
     - Creación de documentación didáctica complementaria (`docs/EDUCATIONAL_ARCHITECTURE.md`) explicando cómo funcionan los videojuegos clásicos en su interior (física en punto fijo, compresión de metatiles, máquinas de estados sin SO).
- **Consecuencias**:
  El proyecto trasciende un simple port para convertirse en un recurso educativo de código abierto de referencia para estudiantes y desarrolladores de emulación y videojuegos retro.

---

## ADR-007: Fidelidad de Paletas y Sprites Derivada del Ensamblador 6502 y PPU 2C02 (Anti-Heurística Visual)

- **Fecha**: 2026-10-24
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  La comparación visual con capturas de vídeo es propensa a errores si se utiliza para "adivinar" colores RGB o posiciones aproximadas de sprites (debido a compresión de vídeo, filtros de emulador o corrección gamma). Además, la NES genera colores a través de una paleta maestra de 64 índices en la PPU 2C02 y registros de hardware específicos ($3F00-$3F1F).
- **Decisión**:
  1. **Origen de la Verdad en Paletas**: Toda asignación de color en el juego debe provenir exclusivamente de los índices de color de la PPU 2C02 cargados desde las tablas originales de `asm/src/data.asm` (`MainPalette`, `BackgroundPaletteSets`) y volcados en la RAM de paletas ($3F00-$3F1F) según las subrutinas de `asm/src/prg.asm`.
  2. **Origen de la Verdad en Sprites**: El mapeo de tiles de Jack, enemigos, bombas y HUD debe obtenerse de los datos reales del Banco 1 de CHR-ROM y de las tablas de animación/OAM del ensamblador original (bytes de atributos de OAM con bits de paleta y flip horizontal/vertical).
  3. **Rol de la Comparación Visual**: La comparación visual contra `Mighty-Bomb-Jack-original.mp4` se establece estrictamente como un mecanismo de **auditoría, validación y certificación de paridad**, pero **nunca** como fuente para inventar o aproximar valores a ojo.
- **Consecuencias**:
  Garantía de fidelidad matemática y cromática 1:1 con el hardware original de la NES, evitando cualquier discrepancia estética o fallo de emulación.

---

## ADR-008: Prohibición Estricta de Suposiciones en Decisiones y Respaldo Obligatorio en Código y Emulador

- **Fecha**: 2026-10-25
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Las conjeturas o suposiciones sobre el funcionamiento del juego (posicionamiento de sprites, dimensiones de cajas de colisión, temporizaciones, tablas de velocidad) pueden conducir a atajos sintéticos o placeholders que desvirtúan la paridad 1:1 con la NES original.
- **Decisión**:
  1. **Uso Exclusivo de Suposiciones como Teorías**: Las suposiciones o hipótesis son lícitas únicamente durante la fase preliminar de formulación teórica.
  2. **Prohibición Total en Resultados y Decisiones**: Queda terminantemente prohibido adoptar decisiones de diseño, contratos funcionales, estructuras de datos, valores de constantes o código en C basados en suposiciones.
  3. **Respaldo Obligatorio**: Todo resultado técnico, decisión e implementación debe estar obligatoriamente respaldado por:
     - El código fuente ensamblador original (`asm/src/prg.asm`, `asm/src/data.asm`) con cita de archivo y línea.
     - Datos directos y telemetría del emulador (`cynes`, volcados de RAM, OAM o PPU).
- **Consecuencias**:
  Eliminación absoluta de placeholders y discrepancias empíricas. Cualquier divergencia debe resolverse inspeccionando el código o las trazas del emulador.

---

## ADR-009: Capitalización Persistente del Análisis ASM en `research/` y Decisión Final en Código

- **Fecha**: 2026-10-25
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Re-analizar las mismas subrutinas o tablas del ensamblador 6502 en múltiples sesiones o entre distintos agentes genera un gasto innecesario de tokens, tiempo y riesgo de inconsistencias.
- **Decisión**:
  1. **Documentación Obligatoria en `research/`**: Todo análisis, extracción de tablas, constantes, desensamblado lógico o fórmulas ejecutado por `asm_researcher` debe plasmarse o ampliarse en los artefactos correspondientes de `research/` (`contracts/`, `inventories/`, `control_flow/` o nuevos `RES-*.md`).
  2. **Uso Como Acelerador**: Cualquier agente puede y debe consultar la documentación acumulada en `research/` para evitar re-investigaciones redundantes.
  3. **Decisión Final en el Código Ensamblador**: Aunque la documentación en `research/` sirve de guía y acelerador, toda conclusión crítica, contrato e implementación debe ser siempre contrastada y ratificada directamente con las líneas exactas de `asm/src/prg.asm` o `asm/src/data.asm`.
- **Consecuencias**:
  Aceleración incremental del proyecto, preservación del conocimiento y garantía inquebrantable de fidelidad con el Golden Ground Truth.

---

## ADR-010: Prohibición de Interacción Humana en Subagentes y Comunicación Estricta M2M (Ahorro de Tokens)

- **Fecha**: 2026-10-25
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  La generación de saludos, frases de cortesía, explicaciones conversacionales o introducciones amables por parte de subagentes y task runners consume tokens innecesarios de contexto y ralentiza la ejecución automatizada de tareas.
- **Decisión**:
  1. **Monopolio de Interacción en el Chat Principal (Nivel 0)**: El usuario humano únicamente conversa e interactúa con el Chat Principal.
  2. **Silencio Conversacional en Niveles 1, 2 y 3**: El Orquestador de Tareas y todos los subagentes especialistas (`asm_researcher`, `c_implementer`, `emulator_verifier`) tienen prohibido emitir textos conversacionales o de cortesía orientados a humanos.
  3. **Comunicación Estricta M2M**: Los subagentes se comunican exclusivamente con llamadas a herramientas, código limpio, diffs, estructuras de datos y reportes de estado técnicos secos y concisos.
- **Consecuencias**:
  Ahorro drástico de tokens de contexto, mayor velocidad de procesamiento y prevención de distracciones conversacionales en agentes de fondo.

---

## ADR-011: Eliminación Obligatoria e Inmediata de Subagentes Finalizados (Cero Agentes Ociosos)

- **Fecha**: 2026-10-25
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Dejar subagentes completados en estado detenido (`idle`) mantiene referencias huérfanas en memoria y workspaces efímeros, desdibujando la visibilidad del estado real de ejecución y arriesgando reutilizaciones accidentales de contexto contaminado.
- **Decisión**:
  1. **Eliminación Mandatoria con `manage_subagents kill`**: Tan pronto como un subagente de Nivel 3 concluya y reporte su subtarea al Nivel 2, el Nivel 2 debe eliminarlo inmediatamente con `manage_subagents kill`.
  2. **Eliminación del Nivel 2**: Tan pronto como el Orquestador Secundario (Nivel 2) concluya y reporte la tarea verificada al Nivel 1, el Nivel 1 debe eliminar inmediatamente al Orquestador Secundario con `manage_subagents kill`.
  3. **Prohibición de Estados Ociosos**: Queda prohibido dejar agentes en estado `idle` o simplemente detenidos al finalizar una tarea.
- **Consecuencias**:
  Entorno de trabajo siempre limpio, liberación inmediata de recursos y garantía de que cada nuevo subagente arranque con contexto virgen.


---

## ADR-012: Arquitectura del Sistema de Puertas, Evaluación de 33 Condiciones y Transición Topológica de Salas

- **Fecha**: 2026-10-25
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  Las puertas de salida en *Mighty Bomb Jack* representan la transición fundamental de progresión entre las 257 salas del juego. Cada sala posee un tipo de puerta (0..7), una condición de apertura codificada en la cabecera del `RoomData`, una posición canónica en la cuadrícula de metatiles, un algoritmo de mutación gráfica al abrirse, y una regla de conmutación topológica al atravesarla (`EntryDoorType = ExitDoorType ^ 4`).
- **Decisión**:
  1. **Decodificación de Cabecera y Caso Especial Room $61**:
     - El byte de cabecera de sala codifica `%UUUU UMMM` (bits 7..3 = Condición de Puerta, bits 2..0 = Metatiles de Sala).
     - Se preserva el quirk original del 6502 (`prg.asm:6757`): Si la sala es `$61`, el índice de condición se forza incondicionalmente a `0x19`.
  2. **Evaluación Canónica de las 33 Condiciones (`prg.asm:6741-6925`)**:
     - Se implementa el despachador completo de las 33 condiciones USA (`MjkRoom_CheckDoorCondition`), cubriendo requisitos de salto (`DoorCond_02_JumpedOn`), cofres, bombas, vidas, niveles Mighty (`DoorCond_XX_Mighty3`) y condiciones inactivas (`DoorCond_0A_StubRet0`).
  3. **Mutación Gráfica de Puerta (`prg.asm:3823-3870`, `prg.asm:5488-5555`)**:
     - Cuando se cumple la condición, el juego pasa temporalmente a `GAME_STATE_B` para ejecutar el SFX de apertura y mutar los 4 metatiles de la puerta de cerrado (`$87`, `$8B..$8E`) a abierto (`$8A`, `$95..$98`) en la cuadrícula activa de 240 metatiles.
  4. **Transición Topológica e Inversión de Orientación (`AdjacentRoomsTable`, `prg.asm:3790-3822`)**:
     - Al entrar Jack en la caja de colisión de la puerta abierta (`JTPickup_Door`), se transita a `GAME_STATE_9_EXIT_ROOM` (desactivación de control, animación de marcha).
     - La siguiente sala se consulta en `AdjacentRoomsTable[current_room * 4 + door_orientation]`.
     - El tipo de puerta de entrada en la sala destino se conmuta mediante `EntryDoorType = door_orientation ^ 4` (ej. salida por puerta derecha tipo 4 conduce a entrada por puerta izquierda tipo 0).
     - El ciclo de carga (`GAME_STATE_2` -> `3` -> `4` -> `5`) posiciona a Jack en las coordenadas exactas de `DoorEntryXYPositionTable[entry_door_type]` y ejecuta la marcha de entrada automática hasta librar el umbral.
- **Consecuencias**:
  Paridad 1:1 verificada en navegación y progresión entre salas en el source port C99, garantizando soporte determinista para las 257 salas del cartucho original.

---

## ADR-013: Metodología de Replay Diferencial de Golden Master y Empuje de Horizonte (Fase 7)

- **Fecha**: 2026-11-01
- **Estado**: `[ACEPTADA]`
- **Contexto**:
  La fase de refinamiento y paridad exacta 1:1 (Fase 7) requiere detectar discrepancias sutiles en la experiencia de juego, físicas y comportamiento de entidades. La co-simulación síncrona manual desincroniza al jugador ante el primer fallo de 1 píxel. Además, la inspección manual por LLM de miles de fotogramas consume cantidades excesivas de tokens.
- **Decisión**:
  1. **Paradigma Golden Master Anchor**: El jugador humano interactúa únicamente con la ROM original (`cynes`), registrando simultáneamente la secuencia de botones (`.tas`) y la telemetría canónica de la consola (`golden_telemetry.json`).
  2. **Detección Automática Desatendida**: El arnés de Python (`replay_differential.py`) ejecuta el port en C en modo headless, identifica el primer fotograma de divergencia ($T_{\text{div}}$) y emite un micro-reporte estructurado en JSON (`scratch/divergence_report.json`).
  3. **Algoritmo de Empuje de Horizonte**: El agente efímero *Parity Solver* (Gemini Flash) interviene de forma quirúrgica sobre las líneas de código indicadas hasta que la sesión sobrepasa $T_{\text{div}}$, repitiendo el proceso hasta cubrir el 100% de la grabación.
  4. **Suite de Regresión Persistente**: Cada sesión grabada se almacena en `tests/tas/` como prueba de no-regresión permanente.
- **Consecuencias**:
  Eliminación de la frustración por bugs del port durante la prueba humana, diagnóstico a coste cero de tokens mediante scripts deterministas, y reducción superior al 80% en el consumo de tokens de los subagentes durante la resolución iterativa de bugs.
