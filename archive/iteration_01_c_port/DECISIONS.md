# Registro de Decisiones de Arquitectura (Architecture Decision Records - ADR)

Este archivo formaliza todas las decisiones arquitectónicas, técnicas y metodológicas del proyecto Mighty Bomb Jack Native Port. Las decisiones aquí acordadas son inmutables a menos que se abra formalmente un nuevo ADR que sustituya (`Supersedes`) al anterior.

---

## Índice de Decisiones

| ID | Fecha | Título | Estado |
| :--- | :--- | :--- | :--- |
| `ADR-001` | 2026-10-22 | Desacoplamiento de Memoria Persistente y Blindaje de Solo Lectura de `asm/` | Aceptado |
| `ADR-002` | 2026-10-22 | Estructura Permanente de Investigación Técnica y Base de Conocimiento (`research/`) | Aceptado |
| `ADR-003` | 2026-10-22 | Ejecución en Lotes Pequeños (Micro-Batching), Checkpoints Atómicos y Reanudación de Bajo Coste de Tokens | Aceptado |
| `ADR-004` | 2026-10-22 | Adopción de la Revisión USA (REV_US) como Golden Standard del Port Nativo | Aceptado |
| `ADR-005` | 2026-10-22 | Capa de Presentación con Raylib, Modo Pantalla Completa y Soporte para Mando Xbox Series (XInput) / Teclado | Aceptado |

---

## Detalle de Decisiones

### `ADR-001`: Desacoplamiento de Memoria Persistente y Blindaje de Solo Lectura de `asm/`

- **Fecha**: 2026-10-22
- **Estado**: Aceptado
- **Decisores**: Usuario & Antigravity (Orchestrator)

#### Contexto
En flujos de trabajo anteriores con múltiples agentes, el estado del proyecto, tareas, handoffs y bitácoras se almacenaban en carpetas efímeras dentro de `.agents/` (`auditor_1`, `orchestrator_1`, `worker_*`, etc.). Esto generaba los siguientes problemas:
1. Al detener o reiniciar una sesión, el contexto quedaba fragmentado entre cientos de archivos dispersos.
2. Los nuevos agentes u orquestadores carecían de una visión unificada del estado del proyecto.
3. Existía riesgo de que agentes modificaran por error archivos dentro de `asm/`, alterando la fuente de verdad del desensamblado del juego original.

#### Decisión
1. **Desacoplar la memoria del directorio `.agents`**:
   - Se establece un sistema de 5 archivos persistentes en la raíz del workspace: `AGENTS.md`, `TASKS.md`, `PROJECT_STATUS.md`, `DECISIONS.md` y `SESSION_LOG.md`.
   - Ningún agente guardará el estado de las tareas dentro de `.agents`.
2. **Blindaje de `asm/` como Solo Lectura**:
   - `asm/` se declara y configura como estrictamente de Solo Lectura tanto a nivel de reglas en `AGENTS.md` como a nivel de atributos del sistema operativo (`attrib +R`).
   - Se fija como la única Fuente de Verdad para las mecánicas originales de la NES.
3. **Escritura centralizada en `port_native/` y Raíz**:
   - Todo el código fuente ejecutable, tests y assets residen exclusivamente en `port_native/`.
   - La raíz queda limpia de archivos compilados `.obj`, logs temporales o scripts desechables.

#### Consecuencias
- **Positivas**:
  - Resiliencia total ante reinicios de contexto: cualquier orquestador o subagente puede reanudar el trabajo en segundos leyendo `PROJECT_STATUS.md` y `TASKS.md`.
  - Integridad garantizada del código ensamblador 6502 original.
  - Trazabilidad y auditoría claras de qué agente ejecutó cada tarea y cómo fue verificada.
- **Negativas / Compromisos**:
  - Requiere que los agentes mantengan la disciplina de actualizar `TASKS.md` y `SESSION_LOG.md` de forma obligatoria en cada ciclo de trabajo.

---

### `ADR-002`: Estructura Permanente de Investigación Técnica y Base de Conocimiento (`research/`)

- **Fecha**: 2026-10-22
- **Estado**: Aceptado
- **Decisores**: Usuario & Antigravity (Orchestrator)

#### Contexto
A medida que se avanza en el port nativo de *Mighty Bomb Jack*, los agentes analizan el código ensamblador 6502 (`asm/`), extraen tablas de física, descubren estructuras de memoria RAM, descifran formatos de datos (tiles, layouts de habitaciones, enums de audio y lógica de IA de enemigos). Anteriormente, estas conclusiones quedaban dispersas en el chat o en archivos efímeros dentro de `.agents`, perdiéndose entre sesiones.

#### Decisión
1. **Creación del directorio `research/`**:
   - Destinado a almacenar de forma persistente todos los hallazgos técnicos, tablas de constantes extraídas de la ROM/desmontaje, y especificaciones de paridad.
2. **Índice Maestro Centralizado (`research/INDEX.md`)**:
   - Actúa como catálogo y tabla de contenidos navegable, relacionando cada investigación con su tarea de origen (`TASK-XXX`), área técnica, estado y enlace al documento.
3. **Plantilla Estándar (`research/TEMPLATE.md`)**:
   - Todo nuevo informe debe seguir una estructura consistente: Objetivos, Fuentes en `asm/`, Algoritmos/Tablas 6502, Discrepancias con `port_native/`, Especificación para C y Plan de Tests.
4. **Convención de Nomenclatura**:
   - `research/RES-###-<slug-descriptivo>.md`.

#### Consecuencias
- **Positivas**:
  - La base de conocimiento técnico crece de manera acumulativa y estructurada.
  - Los agentes implementadores y evaluadores pueden consultar especificaciones exactas sin tener que repetir el proceso de ingeniería inversa.
  - Trazabilidad directa entre tareas de `TASKS.md`, informes técnicos en `research/` y código en `port_native/`.
- **Negativas / Compromisos**:
  - Cada investigación técnica formalizada requiere redactar el documento correspondiente y registrarlo en `research/INDEX.md`.

---

### `ADR-003`: Ejecución en Lotes Pequeños (Micro-Batching), Checkpoints Atómicos y Reanudación de Bajo Coste de Tokens

- **Fecha**: 2026-10-22
- **Estado**: Aceptado
- **Decisores**: Usuario & Antigravity (Orchestrator)

#### Contexto
En proyectos complejos con múltiples agentes y subagentes operando sobre un gran codebase (como este port de ensamblador 6502 a C nativo), si las tareas se definen de forma monolítica o si los flujos se interrumpen repentinamente (por límites de contexto, pausas del usuario, timeouts o errores), retomar la ejecución provoca un sobrecoste masivo de tokens:
1. El agente entrante tiende a re-explorar todo el workspace, re-leyendo directorios y archivos grandes que ya habían sido analizados.
2. Si un subagente queda a medio camino, se desconoce qué línea o función concreta estaba tocando, forzando a descartar el trabajo previo o repetir diagnósticos.
3. El coste en tokens de re-sincronización puede llegar a ser mayor que el de la propia implementación.

#### Decisión
1. **Regla de Micro-Lotes (Micro-Batching)**:
   - Toda tarea en `TASKS.md` debe estar descompuesta en una secuencia de micro-pasos atómicos (lotes pequeños de trabajo).
   - Cada micro-paso debe restringirse a un ámbito estricto: máximo 1 subsistema/función específica, máximo 1-2 archivos modificados a la vez, y una verificación inmediata asociada.
2. **Checkpoints Atómicos en `.md`**:
   - Cada micro-paso completado se marca de inmediato en `TASKS.md` (`[x]`).
   - Cuando una tarea está activa (`[IN_PROGRESS]`) o pausada, se mantendrá un bloque explícito de `Checkpoint Activo` indicando:
     - Último subpaso completado.
     - Archivo(s) y función exactos bajo edición.
     - Estado de compilación / test del último intento.
     - **Siguiente micro-acción inmediata** (Next Step) precisa y sin ambigüedad.
3. **Protocolo Quirúrgico de Reanudación (Resume Protocol)**:
   - Al reiniciar o entrar un nuevo agente tras una interrupción, el agente tiene terminantemente prohibido hacer exploraciones generales de directorios o relecturas completas.
   - Solo debe leer:
     1. El último bloque de `SESSION_LOG.md` (o sección de Checkpoint en `TASKS.md`).
     2. Únicamente las líneas o funciones de los archivos referenciados en el checkpoint.
     3. Ejecutar la acción directa indicada en el "Siguiente micro-paso".
4. **Contratos Estrictos para Subagentes (Worker Micro-Contracts)**:
   - Todo subagente debe ser invocado con un prompt que delimite un micro-lote cerrado (objetivo único, lista explícita de archivos permitidos, test a correr).
   - El subagente debe volcar su estado/checkpoint en su respuesta y/o en el `.md` correspondiente antes de terminar para que el orquestador persista el estado de inmediato.

#### Consecuencias
- **Positivas**:
  - Gasto de tokens mínimo al reanudar cualquier tarea interrumpida (ahorro de hasta un 80-90% de tokens de re-escaneo).
  - Trazabilidad milimétrica de lo que hizo cada subagente.
  - Cero pérdida de trabajo intermedio: cada micro-paso queda fijado en los archivos persistentes.
- **Negativas / Compromisos**:
  - Requiere mayor disciplina y granularidad al descomponer tareas en `TASKS.md`.
  - Actualización más frecuente de los archivos Markdown durante el avance de los micro-lotes.

---

### `ADR-004`: Adopción de la Revisión USA (`REV_US`) como Golden Standard del Port Nativo

- **Fecha**: 2026-10-22
- **Estado**: Aceptado
- **Decisores**: Usuario & Antigravity (Orchestrator)

#### Contexto
El desensamblado en `asm/` contiene múltiples bifurcaciones condicionales (`IFDEF REV_US` / `IFNDEF REV_US`) que reflejan las diferencias entre el lanzamiento japonés (*Famicom*, abril 1986) y la versión norteamericana (*NES*, 1987). Las discrepancias principales identificadas en `RES-011` abarcan:
1. **Penalización en la Sala de Tortura (Torture Room)**: 50 saltos requeridos en la versión USA (`#$50` en `prg.asm:733`) frente a 49 saltos en la versión japonesa (`#$49`).
2. **Tabla de Condiciones de Apertura de Puertas**:
   - En la versión USA existen 32 condiciones operativas.
   - En la versión USA, la puerta `0x0E` requiere que el nivel de Mighty Power sea 3 (`MightyPowerLevel == 3`), mientras que en la versión JP requería `StageTimer == 60`.
   - Las condiciones `0x0A` (`StageTimer == 30`) y `0x19`/`0x1A` (dígito de score igual a 30 o 70) fueron removidas / inutilizadas en la versión USA.
3. **Mitigación de Bugs del Código Original**:
   - En la versión JP, la rutina `JT_98A4_10_Brother` (`prg.asm:4073`) presenta el bug `Score_1E_INVALID` que corrompe el subpíxel vertical Y de Jack. En la versión USA este comportamiento anómalo está corregido/mitigado.
4. **Textos, Tipografías y Créditos**:
   - Derechos de autor ("TECMO INC. 1986", "LICENSED BY NINTENDO OF AMERICA INC."), cinemáticas de ending y carteles del HUD en idioma inglés.

#### Decisión
1. **Golden Standard US**:
   - Se establece formalmente la versión **USA (`REV_US`)** como la referencia canónica e inmutable para toda la implementación del port en C x86 (`port_native/`).
2. **Parámetros Canónicos**:
   - Torture Room: Se fijan **50 saltos** sin recompensa de puntuación.
   - Puertas: Se implementa la tabla canónica de **32 condiciones de la versión US**.
   - Mitigación de corrupción: Se adoptan las rutinas de la versión US libres de corrupción de memoria en subpíxeles de Jack.
   - Assets y Textos: Se implementan los textos, pantallas de créditos y HUD correspondientes a la versión americana.
3. **Arquitectura Extensible**:
   - La arquitectura del motor mantendrá el selector `GameRegion` (`REV_USA = 0`, `REV_JAPAN = 1`) para permitir en el futuro alternar las reglas de Japón si se requiere, pero el valor por defecto y las pruebas de paridad automatizadas validarán exclusivamente la conducta de `REV_US`.

#### Consecuencias
- **Positivas**:
  - Reglas de juego consistentes y unificadas para todos los subagentes implementadores y auditores.
  - Mayor estabilidad en la simulación al evitar bugs nativos presentes en la primera tirada japonesa.
  - Coherencia total con los vídeos de referencia y los datasets de prueba de paridad.
- **Negativas / Compromisos**:
  - Cualquier prueba de paridad visual o matemática debe contrastarse contra la ROM US (NES-BJ-USA).

---

### `ADR-005`: Capa de Presentación con Raylib, Modo Pantalla Completa y Soporte para Mando Xbox Series (XInput) / Teclado

- **Fecha**: 2026-10-22
- **Estado**: Aceptado
- **Decisores**: Usuario & Antigravity (Orchestrator)

#### Contexto
El port nativo en C11 requiere una capa de renderizado y gestión de periféricos en Windows moderna, fluida y sin latencia. Se necesita:
1. Soporte completo y automático ("plug-and-play") para **mandos de Xbox Series X|S, Xbox One y Xbox 360** en Windows.
2. Controles completos y alternativos por **teclado**.
3. Renderizado gráfico con soporte para **pantalla completa** preservando la relación de aspecto original de la NES (4:3) con escalado entero o barras laterales (pillarbox) sin distorsión.

#### Decisión
1. **Librería de Presentación Gráfica: Raylib 5.5**:
   - Se utilizará Raylib para la creación de ventana, sincronización de refresco V-Sync a 60 Hz y renderizado acelerado por hardware.
   - La simulación del juego genera un buffer virtual nativo de NES de **256x240 píxeles** en un `RenderTexture2D`.
   - Dicho buffer se proyecta sobre la ventana o la pantalla completa mediante un shader/blitter con escalado proporcional (manteniendo 4:3) y filtrado Point para píxeles nítidos (*pixel-perfect*).
   - Conmutación dinámica de pantalla completa mediante tecla `F11` o `Alt+Enter` con `ToggleFullscreen()`.
2. **Entrada Dual: Mando Xbox Series + Teclado**:
   - **Mando Xbox Series (XInput)**:
     - D-Pad / Stick izquierdo analógico (con zona muerta circular): Movimiento D-Pad de la NES.
     - Botón A: Salto y aleteo (*Flutter glide*).
     - Botón B / Botón X: Poder Mighty / Forzado de cofres.
     - Botón Start (Menú): Pausa / Iniciar partida.
     - Botón Back (View): Selección de opciones (Select).
     - Detección de conexión/desconexión en caliente (*hotplug*).
   - **Teclado**:
     - Flechas de dirección / WASD: D-Pad.
     - Z / Espacio: Botón A (Salto / Aleteo).
     - X / Shift izquierdo: Botón B (Mighty Power / Acción).
     - Enter: Botón Start.
     - Retroceso / Shift derecho: Botón Select.

#### Consecuencias
- **Positivas**:
  - Experiencia de juego moderna y de consola en PC, jugable tanto en monitores de escritorio como en televisores a pantalla completa.
  - El núcleo de simulación (`mbj_core`) permanece desacoplado de Raylib, recibiendo únicamente una estructura `GameInput` unificada con máscara de botones.
- **Negativas / Compromisos**:
  - Raylib se compila como dependencia estática ligera vía CMake `FetchContent`, requiriendo enlaces con las librerías del sistema de Windows (`winmm`, `gdi32`, `xinput`).
