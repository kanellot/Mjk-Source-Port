# Registro de Tareas Completadas ([DONE]) — Mighty Bomb Jack
**Iteración**: 2 — Reconstrucción Funcional Integral y Especificación Formal
**Total Tareas Completadas**: 51 tareas
**Archivo de Tareas Pendientes**: [TASKS_TODO.md](./TASKS_TODO.md)

---

### Fase 0: Preparación y Alcance

#### `[DONE]` `TASK-F0-001` - Configuración del Entorno de Trabajo y Gobernanza de Iteración 2

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Archivar la iteración 1, desplegar la skill `nes-functional-reconstruction`, reorganizar el espacio de trabajo en carpetas dedicadas (`project/`, `archive/`, `research/`) y blindar la regla de no implementar.

- **Subpasos (Micro-Lotes)**:

  - [x] 0.1 Crear `archive/iteration_01_c_port/` y respaldar los 5 archivos de gestión de la iteración previa con su `README.md`.

  - [x] 0.2 Crear subdirectorios de trabajo en `research/` (`scope/`, `inventories/`, `control_flow/`, `contracts/`, `audit/`).

  - [x] 0.3 Desplegar `.agents/skills/nes-functional-reconstruction/SKILL.md`.

  - [x] 0.4 Actualizar la constitución raíz `AGENTS.md` (congelar `port_native/`, redirigir gestión a `project/`).

  - [x] 0.5 Inicializar `project/PROJECT_STATUS.md`, `project/DECISIONS.md`, `project/TASKS.md` y `project/SESSION_LOG.md`.

  - [x] 0.6 Purgar archivos sueltos de la raíz del workspace.



---

---

#### `[DONE]` `TASK-F0-002` - Delimitación de Alcance y Layout de Memoria ROM

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar los entregables `research/scope/scope.md` y `research/scope/rom_layout.md` estableciendo el alcance funcional canónico de la versión USA y el mapeo NROM-256 de 64 KB.

- **Criterio de Aceptación**: Documentos creados, mapeo de memoria validado contra `asm/src/prg.asm` y `asm6f`, y alcance formalmente delimitado.

- **Subpasos (Micro-Lotes)**:

  - [x] 0.1 Redactar `research/scope/scope.md` definiendo el catálogo canónico (versión USA `REV_US`, 257 salas, condiciones de puertas, GDV y finales).

  - [x] 0.2 Redactar `research/scope/rom_layout.md` documentando la arquitectura NROM-256 (32KB PRG-ROM `$8000-$FFFF`, 32KB CHR-ROM, vectores de interrupción `$FFFA-$FFFF`).

  - [x] 0.3 Verificar coherencia con `research/RES-001` y `research/RES-002`.



---

---

#### `[DONE]` `TASK-F0-003` - Inventario del Repositorio, Limitaciones y Plan de Análisis

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/scope/repository_inventory.md` y `research/scope/analysis_plan.md`.

- **Criterio de Aceptación**: Fuentes de ensamblador catalogadas con hashes/líneas, herramientas documentadas y riesgos de análisis clasificados.

- **Subpasos (Micro-Lotes)**:

  - [x] 0.1 Redactar `research/scope/repository_inventory.md` con las fuentes de `asm/src/` y assets.

  - [x] 0.2 Redactar `research/scope/analysis_plan.md` con la estrategia tool-assisted y criterios de validación.



---

---

### Fase 1: Inventario Estático Exhaustivo (Tool-Assisted)

#### `[DONE]` `TASK-F1-001` - Herramienta de Extracción Estática Profunda (AST / XREF)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Desarrollar en `tools/deep_asm_analyzer.py` una herramienta que extraiga automáticamente referencias cruzadas (XREFs), accesos a RAM (R/W), árboles de llamadas y destinos de saltos indirectos desde `asm/src/`.

- **Criterio de Aceptación**: Script ejecutable que produce JSON/CSV deterministas sin depender de búsquedas de texto plano.

- **Subpasos (Micro-Lotes)**:

  - [x] 1.1 Diseñar parser de instrucciones 6502 en Python para `prg.asm` y `data.asm`.

  - [x] 1.2 Extraer matriz de operaciones por variable de RAM (Read / Write / RMW).

  - [x] 1.3 Extraer tabla de subrutinas con puntos de entrada, salida y `PLA; PLA`.

  - [x] 1.4 Validar salida contra ensamblado de `asm6f_64.exe`.



---

---

#### `[DONE]` `TASK-F1-002` - Inventario Maestro de Código y Tablas de Datos

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/inventories/code_inventory.csv` y `research/inventories/data_inventory.csv`.

- **Criterio de Aceptación**: Las 294 subrutinas y 525 tablas de datos clasificadas con rango de direcciones, tamaño y justificación.



---

---

#### `[DONE]` `TASK-F1-003` - Mapa de Memoria RAM Exhaustivo y Uso de Variables

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/inventories/memory_map.md` y `research/inventories/variable_usage.csv`.

- **Criterio de Aceptación**: Los 337 símbolos de RAM ($0000-$07FF) documentados con sus subrutinas lectoras y escritoras.



---

---

#### `[DONE]` `TASK-F1-004` - Identificación de Regiones No Resueltas y Código Muerto

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/inventories/unresolved_regions.md` clasificando subrutinas anónimas (`sub_XXXX`), rutinas sin llamadores directos y bloques no catalogados.

- **Criterio de Aceptación**: Toda ambigüedad explícitamente delimitada con evidencia o clasificada como código muerto.



---

---

### Fase 2: Flujo de Control, Datos y Modelo de Hardware

#### `[DONE]` `TASK-F2-001` - Grafo de Control, JumpTables y Pila

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/control_flow/control_flow.md` modelando el pipeline intra-frame de la NMI, las 15 JumpTables y las 16 rutinas con doble retorno (`PLA; PLA`).

- **Criterio de Aceptación**: Grafo de control completo sin saltos indirectos sin resolver.



---

---

#### `[DONE]` `TASK-F2-002` - Flujo de Datos, Scratchpad Zero-Page y Persistencia

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/control_flow/data_flow.md` modelando el ciclo de vida del scratchpad Zero-Page (`$00-$1F`) y las variables que persisten entre salas y estados.

- **Criterio de Aceptación**: Flujo de datos modelado, demostrado el invariante de monohilo no concurrente.



---

---

#### `[DONE]` `TASK-F2-003` - Modelo de Hardware NES (PPU, APU, Joypad)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/control_flow/hardware_model.md` y `research/control_flow/unresolved_dependencies.md` modelando buffers VRAM, OAM DMA `$4014`, el intérprete de bytecode de sonido 2A03 y el orden de actualización de joypads.

- **Criterio de Aceptación**: Hardware modelado y cero dependencias indirectas huérfanas.



---

---

### Fase 3: Reconstrucción Funcional y Contratos de Comportamiento

#### `[DONE]` `TASK-F3-001` - Máquina de Estados Global y Matriz de Transiciones

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/contracts/state_machine.md` y `research/contracts/state_transition_matrix.csv` con los 13 estados de `GameHandler` y las secuencias de `TitleScreenHandler`.

- **Criterio de Aceptación**: Estados y transiciones documentados formalmente con precondiciones, eventos y efectos secundarios.



---

---

#### `[DONE]` `TASK-F3-002` - Contrato Funcional: Física de Jack y Colisiones

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/contracts/behavior_contracts/jack_physics_contract.md` (curva de salto de 64 pasos, modulación horizontal, aleteo, 9 sondas de colisión).

- **Criterio de Aceptación**: Cinemática de punto fijo 8.8, tabla `word_C0E3` y colisiones 9-probe especificadas en pseudocódigo.



---

---

#### `[DONE]` `TASK-F3-003` - Contrato Funcional: Decodificador de Salas (257 Salas), Puertas y Cofres

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/contracts/behavior_contracts/room_decoder_contract.md` (decodificación 4-tier, condiciones de puertas US, cofres, monedas).

- **Criterio de Aceptación**: Decodificador 4-tier, cadena de 23 bombas y 33 condiciones de puertas USA especificadas.



---

---

#### `[DONE]` `TASK-F3-004` - Contrato Funcional: Enemigos, Spawns e IA

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/contracts/behavior_contracts/enemy_ai_contract.md` (6 arquetipos, ciclo de spawn, mutación `(A % 6) + 2`, temporizador de monedas).

- **Criterio de Aceptación**: Estructura `EnemyStruct` de 28 bytes, arquetipos e IA mutante especificados.



---

---

#### `[DONE]` `TASK-F3-005` - Contrato Funcional: HUD, Aritmética BCD, GDV y Finales

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/contracts/behavior_contracts/hud_scoring_contract.md` (marcador BCD, Sala de Tortura con 50 saltos, fórmula de GDV y 4 finales).

- **Criterio de Aceptación**: Aritmética BCD pura, penalización de tortura US y fórmula de evaluación de finales formalizada.



---

---

#### `[DONE]` `TASK-F3-006` - Modelo Temporal (60 Hz) y Semántica de Entrada

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/contracts/timing_model.md`, `research/contracts/input_semantics.md` y `research/contracts/behavior_catalog.md`.

- **Criterio de Aceptación**: Temporizadores, orden intra-frame de joypads y catálogo maestro de contratos completado.



---

---

### Fase 4: Auditoría de Cobertura y Trazabilidad (Gate Obligatorio)

#### `[DONE]` `TASK-F4-001` - Matriz de Trazabilidad Bidireccional

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/audit/traceability_matrix.csv` mapeando cada comportamiento a sus rutinas/variables originales y viceversa.

- **Criterio de Aceptación**: Trazabilidad bidireccional exhaustiva (100% de rutinas y contratos vinculados sin gaps).



---

---

#### `[DONE]` `TASK-F4-002` - Informe de Auditoría y Certificación de Cero Puntos Ciegos

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/audit/coverage_report.md`, `research/audit/unresolved_questions.md` y `research/audit/audit_report.md`. Dictamen formal de superación de auditoría antes de autorizar cualquier implementación futura.

- **Criterio de Aceptación**: Dictamen formal emitido; 100% de cobertura y cero incertidumbres críticas pendientes.



---

---

#### `[DONE]` `TASK-F4-003` - Verificación Matemática de Contabilidad de Bytes de ROM (32.768 Bytes)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Desarrollar `tools/verify_rom_accounting.py` y generar `research/audit/rom_byte_accounting_report.md` auditando address por address desde `$8000` hasta `$FFFF` sin dejar ni un solo byte huérfano.

- **Criterio de Aceptación**: 32.768 bytes lineales clasificados (100.00% de cobertura matemática, 0 bytes desconocidos).



---

---

#### `[DONE]` `TASK-F4-004` - Auditoría Determinista de Cierre del Grafo de Flujo de Control (CFG Closure)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Desarrollar `tools/verify_control_flow_closure.py` y generar `research/audit/control_flow_closure_report.md` resolviendo las 15 JumpTables (185 bifurcaciones dinámicas), calculando la alcanzabilidad transitiva desde `RESET`/`NMI` (288 vivas / 5 muertas) y auditando la seguridad de pila de las 16 rutinas `PLA; PLA`.

- **Criterio de Aceptación**: 100% de clausura del grafo demostrada matemáticamente (cero ramas huérfanas, cero desbordamientos de tabla o pila).



---

---

### Fase 5: Diseño de Arquitectura del Source Port y Extracción Clean-Room

#### `[DONE]` `TASK-F5-001` - Diseño de Arquitectura Desacoplada y Educacional en C

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/architecture/source_port_architecture.md` definiendo la división estricta entre Núcleo (`src/core/`) y Frontend SDL2/SDL3 (`src/platform/`), interfaz HAL, modelo temporal a 60.0988 Hz y soporte para Windows, mandos de Xbox Series y teclado.

- **Criterio de Aceptación**: Arquitectura modular desacoplada en C99, tipos semánticos y arnés de pruebas Golden Traces especificado.



---

---

#### `[DONE]` `TASK-F5-002` - Especificación y Herramienta del Extractor Clean-Room (BYOR)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Generar `research/architecture/asset_extractor_spec.md` y desarrollar `tools/extract_assets.py`.

- **Criterio de Aceptación**: Script ejecutable que valida el hash SHA-1 de `mbj-us.nes`, extrae 32 KB de CHR-ROM, secuencias de audio y textos, empaquetándolos en `assets/assets.dat`.



---

---

#### `[DONE]` `TASK-F5-003` - Guía Didáctica de Arquitectura Retro e Invariantes NES

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Redactar `docs/EDUCATIONAL_ARCHITECTURE.md` explicando detalladamente los conceptos clave para estudiantes: aritmética en punto fijo Q8.8, compresión de 257 salas en 17 KB por indirección en 4 niveles, aritmética BCD y síntesis chiptune 2A03.

- **Criterio de Aceptación**: Documento didáctico exhaustivo, autocontenido y enlazado con el código fuente.



---

---

### Fase 6: Implementación Incremental del Núcleo en C (Clean & Educational)

#### `[DONE]` `TASK-F6-001` - Fundamentos: Tipos Base, HAL, Cargador de Assets y Build System

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Crear la estructura limpia `src/` (eliminando/archivando el código obsoleto de la iteración 1), definir `include/mjk_types.h` (punto fijo Q8.8, structs semánticos), `include/mjk_platform.h` (contratos HAL), `include/mjk_assets.h` y `src/core/mjk_assets.c` (cargador de `assets.dat` y decodificador 2bpp), arnés de test `tests/test_assets.c` y script automatizado `build.bat`.

- **Criterio de Aceptación**: Código en C99 puro compilando con MSVC x64 `/W4` con 0 advertencias y pasando tests unitarios al 100%.



---

---

#### `[DONE]` `TASK-F6-002` - Máquina de Estados Principal y Orquestador del Frame NMI (60 Hz)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `include/mjk_game.h`, `src/core/mjk_game.c` y `src/core/mjk_state_machine.c` reproduciendo el pipeline de 7 pasos de la NMI y los 13 estados (`GameState_0_Init` a `GameState_C_Ending`).

- **Criterio de Aceptación**: Transiciones de estado deterministas verificadas con `tests/test_state_machine.c`.



---

---

#### `[DONE]` `TASK-F6-003` - Cinemática de Jack: Salto de 64 Pasos y Colisiones de 9 Puntos

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `include/mjk_player.h` y `src/core/mjk_player.c` integrando la tabla tabular `JumpTrajectoryTable` (64 pasos de `word_C0E3`), aceleraciones horizontales y matriz de sondeo de 9 puntos.

- **Criterio de Aceptación**: Verificación matemática de trayectoria parabólica y aterrizaje con `tests/test_player_physics.c`.



---

---

#### `[DONE]` `TASK-F6-004` - Decodificador de 257 Salas y 33 Condiciones de Puertas USA

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `include/mjk_room_decoder.h` y `src/core/mjk_room_decoder.c` con el descompresor de 4 niveles, las 33 puertas USA y la cadena de 23 bombas.

- **Criterio de Aceptación**: Condiciones de puertas USA y racha de bombas verificadas con `tests/test_room_decoder.c`.



---

---

#### `[DONE]` `TASK-F6-005` - Inteligencia Artificial de Enemigos, Spawns y Mutación

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `include/mjk_enemy.h` y `src/core/mjk_enemy.c` (6 arquetipos, mutación `(A % 6) + 2`, colisiones letales y transmutación Power Coin de 320 frames).

- **Criterio de Aceptación**: Mutación determinista y escala exponencial de monedas verificadas con `tests/test_enemy_ai.c`.



---

---

#### `[DONE]` `TASK-F6-006` - HUD, Aritmética BCD, Sala de Tortura y GDV

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `include/mjk_hud.h` y `src/core/mjk_hud.c` (adición BCD pura, resta unitaria para tortura/timer, GDV y los 4 finales cinemáticos).

- **Criterio de Aceptación**: Aritmética BCD y evaluación de finales verificadas con `tests/test_hud.c`.



---

---

#### `[DONE]` `TASK-F6-007` - Sintetizador Chiptune APU 2A03 (Pulse, Triangle, Noise)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `include/mjk_apu.h` y `src/core/mjk_apu.c` sintetizando las ondas Pulse, Triangle y Noise LFSR a 44.1 kHz PCM y despachando SFX.

- **Criterio de Aceptación**: Muestras PCM sintetizadas y efectos de salto/ruido verificados con `tests/test_apu.c`.



---

---

#### `[DONE]` `TASK-F6-008` - Frontend de Plataforma Windows (Xbox Series, Teclado, GDI 4:3 y WaveOut)

- **Responsable**: Antigravity (Orchestrator)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `src/platform/mjk_platform_win32.c` y `src/main.c` con escalado 4:3 en ventana, soporte XInput nativo para mandos Xbox Series, teclado, audio WaveOut a 44.1 kHz y temporizador fijo a 60.0988 Hz sin dependencias externas.

- **Criterio de Aceptación**: Ejecutable binario nativo generado en `bin/mjk.exe` (162 KB) compilando limpiamente con MSVC x64 `/W4` con 0 errores.



---

---

#### `[DONE]` `TASK-F6-009` - Corrección de Audio No Bloqueante en WaveOut y Renderizado de Pantalla de Título

- **Responsable**: Antigravity (Orchestrator - Senior Reverse Engineering Expert)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Eliminar el bucle bloqueante en `Win32_QueueAudioSamples` gestionando la cola con la flag `WHDR_INQUEUE`, implementar el renderizado gráfico de la Pantalla de Título con indicación interactiva de inicio en `src/core/mjk_game.c`, e inicializar el buffer de frame en `src/main.c`.

- **Criterio de Aceptación**:

  - Ventana 100% responsiva (cero bloqueos o estados de "No responde").

  - Pantalla de título visible de inmediato con tipografía y mensaje de "PRESS START / ENTER / BUTTON A".

  - Transición fluida a partida activa al pulsar entrada por teclado o mando Xbox.

- **Subpasos (Micro-Lotes)**:

  - [x] 6.9.1 Corregir gestión de buffers de audio en `src/platform/mjk_platform_win32.c` empleando `WHDR_INQUEUE` no bloqueante y sincronización segura de WaveOut.

  - [x] 6.9.2 Implementar en `src/core/mjk_game.c` el renderizado visual de la Pantalla de Título (fondo y texto legible) cuando `in_game == false`.

  - [x] 6.9.3 Inicializar `MjkFrameOutput` con `memset` en `src/main.c` antes de la primera simulación para garantizar un frame 0 limpio.

  - [x] 6.9.4 Validar compilación con `build.bat test` y generar `bin/mjk.exe` actualizado.



---



### Refactorización Visual Auténtica NES & Frontend Multiplataforma SDL2

---

#### `[DONE]` `TASK-F6-010` - Integración de Frontend Multiplataforma SDL2 (Windows y Linux)

- **Responsable**: Antigravity (Orchestrator - Senior Reverse Engineering Expert)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar `src/platform/mjk_platform_sdl.c` con `SDL_CreateWindow`, `SDL_CreateRenderer`, `SDL_CreateTexture` a 256x240 con escalado 4:3, `SDL_QueueAudio` a 44.1 kHz sin buffers bloqueantes y `SDL_GameController` con soporte universal para mandos de Xbox Series, PS4/PS5, Switch y teclado, compatible con Windows y Linux.

- **Criterio de Aceptación**: Compila en Windows con MSVC vía `build.bat` y en Linux vía `Makefile`, arrancando ventana acelerada a 60 Hz.

- **Subpasos (Micro-Lotes)**:

  - [x] 6.10.1 Descargar y organizar SDL2 VC en `third_party/sdl2/` y copiar `SDL2.dll` a `bin/`.

  - [x] 6.10.2 Implementar `src/platform/mjk_platform_sdl.c` cumpliendo los contratos de `include/mjk_platform.h`.

  - [x] 6.10.3 Crear `Makefile` para compilación nativa en Linux con `gcc -lSDL2` y actualizar `build.bat` para enlazar con SDL2.

  - [x] 6.10.4 Actualizar `src/main.c` para utilizar la inicialización SDL2 unificada.



---

---

#### `[DONE]` `TASK-F6-011` - PPU Core: Corrección de CHR-ROM (Banco 1 offset 8192) y Blitter de Tiles NES 2C02 con Paleta Maestra 64 RGB

- **Responsable**: Equipo Multi-Agente (`asm_researcher`, `visual_analyst`, `c_implementer`)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Corregir el cargador de assets en `src/core/mjk_assets.c` para apuntar al Banco 1 de la CHR-ROM (offset 8192), integrar la paleta maestra oficial de 64 colores de la NES 2C02 y crear el blitter de tiles de 8x8 con transparencia y flip horizontal/vertical en `src/core/mjk_ppu.c` e `include/mjk_ppu.h`.

- **Criterio de Aceptación**: Decodificación verificada de los 512 tiles reales de sprites y escenarios con 0 errores y 0 warnings.

- **Subpasos (Micro-Lotes)**:

  - [x] 6.11.1 Identificar causa raíz del offset 8192 (Bank 1) en CNROM y validar con renderizador de prueba.

  - [x] 6.11.2 Extracción de tablas de paletas PPU ($3F00-$3F1F) desde `asm/src/data.asm` (`MainPalette`, `BackgroundPaletteSets`) por `asm_researcher`.

  - [x] 6.11.3 Implementar blitter de tiles 8x8 y metatiles 16x16 en `src/core/mjk_ppu.c` e `include/mjk_ppu.h` por `c_implementer`.

  - [x] 6.11.4 Conectar `active_chr_bank` a `package->chr_rom + 8192` en `src/core/mjk_assets.c` e `include/mjk_assets.h`.

  - [x] 6.11.5 Validar compilación y visualización correcta de patrones CHR en `build.bat test` y `build.bat build`.



---

---

#### `[DONE]` `TASK-F6-012` - Descompresor de Geometría de Salas 4-Tier Auténtico (Salas 0..256 con Sala 0 Pirámide y Sala 1 Mazmorra Verde)

- **Responsable**: Equipo Multi-Agente (`c_implementer` / `asm_researcher`)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar la descompresión exacta 4-tier de salas desde las tablas ROM (`g_room_half_screens`, `g_half_screen_layouts`, `g_layout_chunks`, `g_metatile_definitions`, `g_tile_attributes`, `g_adjacent_rooms`) en `src/core/mjk_room_decoder.c` y `include/mjk_room_data.h`, renderizando la Sala 0 (Pirámide de la Pantalla de Título con textos auténticos) y la Sala 1 (Mazmorra Verde con autoincremento +32 PPU).

- **Criterio de Aceptación**: Descompresión 1:1 verificada con test unitario 4 y 8 pasando limpiamente.

- **Subpasos (Micro-Lotes)**:

  - [x] 6.12.1 Generar `include/mjk_room_data.h` y `src/core/mjk_room_data.c` con datos canónicos binarios exactos.

  - [x] 6.12.2 Implementar `MjkRoom_DecodeGeometry`, `MjkRoom_BuildNametable` y `MjkRoom_WriteNametableString`.

  - [x] 6.12.3 Superponer cadenas oficiales ("PUSH START BUTTON", "(C) TECMO,LTD.", "HI  GDV", "™").

  - [x] 6.12.4 Preservar compatibilidad total con `MjkRoom_Load` y las 33 condiciones de puertas USA.



---

---

#### `[DONE]` `TASK-F6-013` - Compositor OAM de 64 Sprites (Jack animado con alas/correr/salto, bombas rojas y chispa) y HUD NES Oficial

- **Responsable**: Equipo Multi-Agente (`c_implementer` / `visual_analyst`)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Implementar el compositor OAM de 64 sprites renderizando a Jack con sus tiles auténticos (alas en título en X=40, Y=120, caminar/salto en juego), bombas esféricas rojas ($27), chispa parpadeante de mecha (tile $0D en OAM #4) y HUD oficial NES (icono 'T' $1D, timer BCD, score con supresión de ceros a la izquierda, icono 'M' $16 y monedas).

- **Criterio de Aceptación**: Sprites con prioridades OAM correctas, atributos de flip horizontal y paletas 2C02 nativas.

- **Subpasos (Micro-Lotes)**:

  - [x] 6.13.1 Implementar `MjkPPU_BlitSprite` en `src/core/mjk_ppu.c`.

  - [x] 6.13.2 Renderizar Jack volando con alas (4 sprites, subpal 1) y dígito High GDV en Pantalla de Título.

  - [x] 6.13.3 Renderizar HUD oficial NES en fila 0 de nametable en `src/core/mjk_game.c`.

  - [x] 6.13.4 Renderizar Jack animado, bombas y chispa de mecha en gameplay.



---

---

#### `[DONE]` `TASK-F6-014` - Validación Visual Automatizada y Auditoría de Paridad contra `Mighty-Bomb-Jack-original.mp4`

- **Responsable**: Equipo Multi-Agente (`visual_analyst` / `Orchestrator`)

- **Fecha de Cierre**: 2026-10-24

- **Objetivo**: Comparar cuantitativamente las capturas renderizadas por el nuevo motor nativo C (`scratch/rendered_test/title_screen_port.png` y `scratch/rendered_test/room_1_port.png`) contra los fotogramas originales extraídos de `Mighty-Bomb-Jack-original.mp4` (`frame_6s.png` y `frame_15s.png`), calculando correlación de histogramas, métricas de píxeles y validando la ausencia de artefactos.

- **Criterio de Aceptación**: Paridad visual certificada con informe formal en `research/audit/visual_parity_audit_report.md`.

- **Subpasos (Micro-Lotes)**:

  - [x] 6.14.1 Comparación cuantitativa de Pantalla de Título (`title_screen_port.png` vs `frame_6s.png`).

  - [x] 6.14.2 Comparación cuantitativa de Sala 1 Gameplay (`room_1_port.png` vs `frame_15s.png`).

  - [x] 6.14.3 Generación del reporte formal de certificación en `research/audit/visual_parity_audit_report.md`.



> **CHECKPOINT ACTIVO (Última actualización: 2026-10-25 03:17) — EJECUCIÓN JERÁRQUICA OBLIGATORIA (4 NIVELES)**
> - **Última Tarea Completada**: `TASK-F6-020` (Detección y Resolución de Colisiones 9-Probe contra Metatiles del Escenario) `[DONE]`.
> - **Subagentes**: Todos los subagentes finalizados y eliminados (0 subagentes activos).
> - **Estado de Verificación**: 8/8 tests PASSED, compilación MSVC limpia, 5 casos de colisión homologados al 100% con paridad NES original.
> - **Siguiente micro-acción inmediata**: Asignar y despachar `TASK-F6-021` (Sistema de Puertas, Evaluación de 33 Condiciones y Transición de Salas) al Orquestador Secundario.

---

---

### Fase 6 — Reconstrucción Integral de Gameplay, Físicas, Audio y Progresión 1:1 (Flujo Jerárquico Obligatorio)

---

#### `[DONE]` `TASK-F6-016` - Corrección Cromática y Mapeado Auténtico de Sprites de Jack

- **Responsable**: Orquestador de Tarea Efímero (Nivel 2)

- **Fecha de Cierre**: 2026-10-25

- **Prioridad**: Alta (Fidelidad de personaje)

- **Objetivo**: Mapeado exacto de tiles de CHR-ROM y paleta para Jack en todos sus estados:

  - Traje rojo carmesí (`$16`), capucha azul (`$12`), rostro/alas blancas (`$30`).

  - Mapeado 1:1 de tiles según `SpritesTable` en `asm/src/data.asm`.

- **Subpasos (Micro-Lotes)**:

  - [x] 6.16.1 Extraer la tabla exacta de tiles de sprites de Jack para idle, andar, saltar y aletear (`research/RES-012-sprites-jack-oam-y-paletas-ground-truth.md`).

  - [x] 6.16.2 Implementar el ensamblado de sprites OAM en `src/core/mjk_player.c` y `mjk_game.c`.

  - [x] 6.16.3 Validar que el sprite no presente distorsiones cromáticas ni de posición contra el emulador `cynes` y la ROM original (0 diferencias en matriz 16x16).

---

---

#### `[DONE]` `TASK-F6-017` - Estabilización de Audio APU 2A03: Eliminación de Petardeo (Underruns) y Sincronización
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Alta (Calidad de experiencia de usuario)
- **Objetivo**: Eliminar los chasquidos, crujidos y petardeos en el sistema de sonido mediante una gestión determinista de la cola de audio SDL2, unificación del APU y sincronización auténtica con el juego:
  1. Unificación de instancia APU: eliminada la variable local duplicada en `src/main.c` y trasladado el encolado dentro del bucle de acumulación temporal (1 frame exacto de 735 muestras por cada frame lógico simulado).
  2. Pre-buffering suave en SDL2: eliminada la inyección destructiva de ceros en `src/platform/mjk_platform_sdl.c` y configurado buffer inicial sin underrun.
  3. Silencio estricto en pantallas de inicio y disparo canónico de música `MUSIC_MAIN_WITH_INTRO` al entrar en `GAME_STATE_4_ENTER_ROOM`.
- **Criterio de Aceptación**: Audio cristalino, continuo, sin chasquidos ni vaciado de cola (underrun), verificado con 0% de desviación temporal.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.17.1 Extraer la secuencia original de inicialización y silenciamiento de audio en `asm/src/prg.asm` y documentar en `research/RES-013-apu-timing-and-audio-sync.md` (`asm_researcher`).
  - [x] 6.17.2 Unificar la generación de audio en `src/main.c`, corregir la cola en `src/platform/mjk_platform_sdl.c` y sincronizar estados en `src/core/mjk_game.c` (`c_implementer`).
  - [x] 6.17.3 Validar audio continuo y sin petardeo en ejecución real y suite de tests (`emulator_verifier`).

---

---

#### `[DONE]` `TASK-F6-018` - Motor de Música Chiptune Bytecode NES y Canales 2A03
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Autenticidad sonora y paridad musical 1:1)
- **Objetivo**: Implementar el reproductor de música bytecode de Mighty Bomb Jack en `src/core/mjk_apu.c` interpretando los comandos sonoros originales del desmontaje (`asm/src/music-engine.asm` y `asm/src/music-data.asm`):
  1. Intérprete canónico de bytecode con soporte a subrutinas F3/F4, bucles F1/F2, saltos F0 y envolventes.
  2. Pista completa de Round 1 (`SoundData_16_Music_MainIntro` y `SoundData_10_Music_Main`) para Pulse 1, Pulse 2, Triangle y Noise.
  3. Fórmulas físicas 2A03 NTSC exactas y eliminación total del offset DC (salida en silencio = 0 LSB).
- **Criterio de Aceptación**: Música idéntica a la NES original con 0% desviación temporal y error armónico FFT < 2.2%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.18.1 Extraer la arquitectura de `asm/src/music-engine.asm` y secuencias de `asm/src/music-data.asm` en `research/RES-014-sound-bytecode-engine.md` (`asm_researcher`).
  - [x] 6.18.2 Implementar el intérprete de bytecode y mezcla bipolar sin DC bias en `src/core/mjk_apu.c` (`c_implementer`).
  - [x] 6.18.3 Validar paridad acústica contra emulador `cynes`, duraciones de notas 100% idénticas y tests (`emulator_verifier`).

---

---

#### `[DONE]` `TASK-F6-019` - Físicas Auténticas de Jack: Curva de Salto de 64 Pasos, Aleteo Continuo y Punto Fijo
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Sensación de control y jugabilidad)
- **Objetivo**: Reproducir la cinemática exacta de Jack en `src/core/mjk_player.c` basándose en el modelo de punto fijo Q8.8 y las tablas de velocidad del ensamblador:
  1. Curva de salto canónica: tabla de 64 pasos `word_C0E3` (data.asm:187-251) y modulaciones de avance por joypad `word_C163` (data.asm:252-256).
  2. Simetría matemática reflejada en `sub_8AE8` (ascenso negado y descenso reflejado con `EOR #$FF; AND #$3F`).
  3. Aleteo continuo (flutter/gliding) edge-triggered con botón A en aire reseteando el acumulador a 0x4000 (ápice / +1 subpíxel).
  4. Velocidad horizontal auténtica de 384 subpíxeles (1.5 px/frame) y parada instantánea en seco sin aceleración/inercia.
- **Criterio de Aceptación**: Cinemática 100.00% idéntica al cartucho original en altura de salto, tiempo de vuelo y planeo (0 subpíxeles de discrepancia contra cynes).
- **Subpasos (Micro-Lotes)**:
  - [x] 6.19.1 Extraer `word_C0E3`, `word_C163`, `sub_8AE8` y `DoSomethingWithObjYVel` en `research/RES-015-jack-physics-and-flutter.md` (`asm_researcher`).
  - [x] 6.19.2 Implementar cinemática de punto fijo Q8.8, 64 pasos y aleteo continuo en `src/core/mjk_player.c` (`c_implementer`).
  - [x] 6.19.3 Validar paridad matemática absoluta (0 subpíxeles de discrepancia) en `research/VERIFY-019-jack-physics-parity.md` y suite de tests (`emulator_verifier`).

---

---

#### `[DONE]` `TASK-F6-020` - Detección y Resolución de Colisiones 9-Probe contra Metatiles del Escenario
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Estabilidad del mundo y plataformas)
- **Objetivo**: Conectar el sistema canónico de colisiones de Jack en `src/core/mjk_player.c` a la matriz activa de 240 metatiles (16x15) de la sala:
  1. Matriz de 9 puntos de sondeo canónica `byte_A3D8` (centro, cabeza L/R, pies L/R, flancos L/R).
  2. Clasificación de solidez de metatiles según `TileAttributeTable` y máscaras de cuadrantes 8x8 `byte_A3EA`.
  3. Descarte direccional vertical (`sub_9965`): plataformas unidireccionales (tipo 2) transitables en ascenso y sólidas en descenso.
  4. Resolución de contacto: aterrizaje suave `y &= ~7` (`sub_A1EB`), rebote con techo `y |= 7` + reseteo a 0x4000 (`sub_A1B9`), y bloqueo de penetración en muros laterales (`loc_8A10`).
- **Criterio de Aceptación**: Jack camina y salta sobre Plataformas A y B y el suelo sin caer al vacío ni atravesar bloques sólidos (100% verificado contra emulador).
- **Subpasos (Micro-Lotes)**:
  - [x] 6.20.1 Extraer la matriz `byte_A3D8`, `TileAttributeTable` y rutinas `sub_99EA`/`sub_A1EB` en `research/RES-016-collision-system-9probe.md` (`asm_researcher`).
  - [x] 6.20.2 Implementar el sistema 9-probe y resolución direccional en `src/core/mjk_player.c` (`c_implementer`).
  - [x] 6.20.3 Validar los 5 casos de colisión empírica (suelo, plataformas unidireccionales, techo, muros, vacío) en suite de tests (`emulator_verifier`).

---

---

#### `[DONE]` `TASK-F6-021` - Sistema de Puertas, Evaluación de 33 Condiciones y Transición de Salas
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Progresión del juego)
- **Objetivo**: Implementar la mecánica de puertas de escape y transición entre salas:
  1. Ubicación canónica de puertas según `DoorPositionTileTable` (`prg.asm:3823`) y metatiles de puerta cerrada ($87, $8B..$8E) y abierta ($8A, $95..$98).
  2. Evaluación rigurosa de las 33 condiciones de apertura de `CheckAndMaybeOpenDoor` (`prg.asm:6741`), con manejo del quirk de Room $61 (`prg.asm:6757`).
  3. Detección de contacto con Jack (`JTPickup_Door`), transición a `GAME_STATE_9_EXIT_ROOM`, cálculo de sala destino en `AdjacentRoomsTable` con inversión de spawn (`EntryDoorType = ExitDoorType ^ 4`), y ciclo de carga 9 -> 2 -> 3 -> 4 -> 5.
- **Criterio de Aceptación**: Las puertas se renderizan en el escenario, mutan a abiertas bajo sus condiciones originales y permiten a Jack pasar a la siguiente sala con preservación de estado y posicionamiento exacto.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.21.1 Extraer las coordenadas y condiciones de puertas de `roomdata-us.asm` y `prg.asm:6741` en `research/RES-017-door-system-and-transitions.md` (`asm_researcher`).
  - [x] 6.21.2 Implementar los metatiles de puerta cerrada/abierta, evaluación de 33 condiciones y transición topológica en `src/core/` (`c_implementer`).
  - [x] 6.21.3 Validar la apertura de puertas, mutación gráfica, SFX, contacto y navegación Sala 1 -> Sala 2 en tests automatizados (`emulator_verifier`).
  - [x] 6.21.4 Verificar ausencia de regresiones en la suite completa de 8 tests y compilación limpia de `bin/mjk.exe` (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-022A` - Construcción del Mapa de Pasillos y Decodificación de Objetos (`RoomData`)
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Fundamento de contenido de salas y pasillos)
- **Fuentes ASM**: 
  - `asm/src/prg.asm:3257-3680` (`ReadRoomData` `$9410`, `sub_9592`, `IfNotTitleScreenRoomDoThings`, `sub_9696`, `sub_96B7`).
  - `asm/src/data.asm:2450-2482` (`RoomDataPointers`, `SectionRoomsTable`, `ItemToTileTable`).
  - `asm/src/roomdata-us.asm` (`RoomData_01` a `RoomData_FF`).
  - `asm/src/ram.asm:1846-1990` (`RoomDataRAM` `$0603-$0682`, 8 slots de 16 bytes y scratch slot `$0683`).
- **Objetivo**: Conectar el intérprete canónico de bytecode de objetos para que las salas tipo pasillo carguen, instancien y persistan todas las entidades del mapa original:
  1. **Slots Persistentes en RAM (`RoomDataRAM`)**: Gestión de 8 slots para recordar el estado de los objetos de las salas visitadas (bombas recogidas, cofres abiertos, secretos revelados) y slot scratch 8 para salas transitorias/de bombas.
  2. **Decodificación de Flujo de Objetos**:
     - Cabecera `$E0..$EF` (paleta, condición de puerta, música, ID de sección).
     - Grupos de bombas `$01..$0F` (coordenadas `YX`, estado inicial con mecha encendida `0x40` / `0x85` en la 1ª bomba o última de sección).
     - Cofres visibles rojos `$10..$2F` y naranjas `$30..$4F`.
     - Bloques ocultos multi-golpe `$50..$6F` (`0x33` = invisible y transitable hasta golpear).
     - Ítems expuestos `$70..$7F` (bolsas, bebidas, bolas de cristal).
     - Puertas de salida `$80..$8F`.
     - Cofres ocultos/secretos `$90..$AF` (`0x30/0x31` = invisible hasta pisar).
  3. **Pintado de Objetos en el Mapa**: Blittear los objetos visibles en la matriz de metatiles interactivos (`sub_9696`) respetando el flag de ocultación `0x20`.
- **Criterio de Aceptación**: Pasillos (Sala 1 y subsiguientes) cargan sus bombas, cofres y secretos en las coordenadas exactas de la ROM con persistencia en `RoomDataRAM`, verificado con 9/9 tests pasando al 100%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.22A.1 Implementar la estructura `MjkRoomDataRAM` (8 slots persistentes de objetos + slot scratch 8) en `include/mjk_room_data.h` y `src/core/mjk_room_data.c`.
  - [x] 6.22A.2 Implementar el intérprete canónico `MjkRoom_ReadRoomData` decodificando todos los comandos `$01` a `$AF`.
  - [x] 6.22A.3 Blittear los objetos visibles en la matriz de metatiles del escenario de la sala activa.
  - [x] 6.22A.4 Validar la carga de objetos en Sala 1 (Pasillo inicial) y Sala 2 con `tests/test_room_objects.c` y suite completa `build.bat test`.



---

---


#### `[DONE]` `TASK-F6-024` - Motor de Scroll de Escenarios en Pasillos (Horizontal y Vertical) y Cámara
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Navegación en pasillos largos y pirámides)
- **Fuentes ASM**: 
  - `asm/src/prg.asm:1487-1604` (NMI y actualización de registros PPUCTRL/PPUMASK y scroll PPU $2005).
  - `asm/src/prg.asm:5915-6408` (Motor de scroll, cinemática de seguimiento, `sub_A442`, `sub_A454`, `sub_A63C`, `byte_3C2`, `byte_3C4`).
  - `asm/src/prg.asm:8626-8655` (Rutina de actualización de scroll de fondo y banderas de borde `RF_AtScrollEdge`).
  - `asm/src/ram.asm:1198-1205` (`byte_3C2` = Scroll X, `byte_3C4` = Scroll Y, `byte_82` = PPUCTRL shadow, `byte_83` = PPUMASK shadow).
- **Objetivo**: Implementar el motor canónico de desplazamiento suave de pantalla y gestión de cámara para pasillos:
  1. **Determinación del Tipo de Scroll**:
     - Pasillos Horizontales (`(flags & 0x04) != 0`): Anchura de 512 píxeles (2 pantallas). La cámara sigue a Jack en el eje X con deadzone y anclaje central en X=128 a velocidad canónica de 1.5 px/frame (384 subpíxeles).
     - Pasillos Verticales (`(flags & 0x04) == 0` y no single screen): Altura de múltiples pantallas (pozos / subidas). La cámara sigue a Jack en el eje Y anclado en Y=121 con módulo de 240 líneas y alternancia de bits de Nametable vertical en PPUCTRL ($2000 <-> $2800).
     - Salas de Pantalla Única (`RF_SingleScreen`): Scroll fijo en (0,0).
  2. **Registros de Scroll y Costura de Nametables (Seams)**:
     - Blitteado continuo de columnas entrantes de metatiles al avanzar `byte_3C2` (horizontal).
     - Blitteado continuo de filas entrantes de metatiles al avanzar `byte_3C4` (vertical).
  3. **Límites de Scroll (`RF_AtScrollEdge`)**: Detección canónica de borde de pasillo, desanclaje de Jack al llegar al tope y re-anclaje bidireccional al retornar al centro.
  4. **Proyección de Coordenadas de Mundo a Pantalla**:
     - Aplicación de deltas inversos a todas las entidades (Jack, enemigos, bombas, cofres, popups) y blanking de 8 píxeles izquierdos de la PPU (bits 1/2 de PPUMASK).
- **Criterio de Aceptación**: Jack se desplaza fluidamente por pasillos largos horizontales y verticales, la cámara lo sigue suavemente y el fondo se dibuja sin costuras ni artefactos gráficos, verificado con 10/10 tests pasando al 100%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.24.1 Investigar y documentar en `research/RES-019-scroll-engine-and-camera.md` la arquitectura y cinemática completa de cámara (`asm_researcher`).
  - [x] 6.24.2 Crear `include/mjk_camera.h` y `src/core/mjk_camera.c` con los 3 modos de sala, anclaje X=128 / Y=121, banderas `RF_AtScrollEdge` y generación de deltas inversos (`c_implementer`).
  - [x] 6.24.3 Integrar la cámara en el ciclo principal y cambio de salas (`src/core/mjk_state_machine.c`), renderizado PPU y proyección a pantalla (`src/core/mjk_game.c`) (`c_implementer`).
  - [x] 6.24.4 Crear test determinista `tests/test_camera.c` integrándolo en `build.bat` (TEST 10/10) cubriendo los 6 casos de verificación cinemática (`emulator_verifier`).
  - [x] 6.24.5 Validar compilación limpia de `bin/mjk.exe` sin warnings y 10/10 tests unitarios aprobados al 100% (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-024B` - Corrección Integral de Scroll de Escenarios, Construcción Continua de Pasillos Multi-Sala y Cinemática de Jack
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Corrección de renderizado continuo, costuras de pasillo y libertad de movimiento de Jack)
- **Fuentes ASM**: 
  - `asm/src/prg.asm:1540-1604` (Cinemática y control horizontal de Jack, desanclaje/re-anclaje `loc_8A06`, `loc_8A5B`).
  - `asm/src/prg.asm:4230-4300` (`sub_99EA`, `sub_9A03` para colisiones continuas a través de salas adyacentes).
  - `asm/src/prg.asm:5915-6110` (Motor de scroll, avance, detección de borde de sección `loc_A47B`, cambio de sala `STA CurrentRoomID` en `loc_A55E` y `loc_A4DD`).
  - `asm/src/data.asm:1704-1800` (`AdjacentRoomsTable`).
- **Objetivo Cumplido**:
  1. **Construcción y Renderizado Continuo de Pasillos Multi-Sala**:
     - Renderizado de la ventana visible de 256x240 componiendo metatiles/tiles de `CurrentRoomID` y la sala adyacente correspondiente (`adj.right` si `scroll_x > 0`, `adj.left` si `scroll_x < 0`, o `adj.down`/`adj.up` para scroll vertical).
     - Eliminado el corte o barra vertical corrupta y el fondo negro/vacío en la mitad derecha del pasillo.
     - Posicionamiento y renderizado de entidades (bombas, cofres, puertas) de ambas salas contiguas en coordenadas relativas a la cámara.
  2. **Control Bidireccional de Jack y Desanclaje/Reanclaje Canónico**:
     - Eliminada la condición errónea de `RF_SectionStart` en `src/core/mjk_camera.c` que bloqueaba el movimiento hacia la izquierda (`vx < 0`).
     - Jack camina hacia la izquierda decrementando el scroll hacia el inicio de la sección.
     - Al llegar a `scroll_x == 0` en el extremo oeste, se activa `RF_AtScrollEdge` y Jack camina libremente por la pantalla hacia la izquierda hasta el borde físico.
     - Al volver hacia la derecha y cruzar `X >= 128`, se re-ancla a Jack y se reanuda el scroll.
  3. **Colisiones Continuas a través de Fronteras de Sala**:
     - Conectadas las 9 sondas de colisión de Jack (`Jack_ResolveCollisions`) para consultar metatiles de la sala adyacente cuando las coordenadas del mundo cruzan la frontera de 256 píxeles, replicando fielmente `sub_9A03`.
  4. **Transición Continua de Salas en Pasillos**:
     - Al superar 256 píxeles de scroll hacia la derecha: transición limpia `CurrentRoomID = adj.right`, scroll ajustado relativo y recarga de metatiles (`loc_A55E`).
     - Retorno análogo hacia la izquierda (`loc_A4DD`).
- **Criterio de Aceptación**:
  - Jack camina y salta libremente a izquierda y derecha en pasillos horizontales sin atascos.
  - La pantalla se dibuja de forma continua y suave a través de Sala 1 y Sala 2 sin costuras rotas ni vacíos.
  - Las colisiones respetan la geometría continua de ambas salas contiguas.
  - Test unitario determinista `tests/test_corridor_scroll.c` verifica los 5 aspectos (5/5) y la suite completa de 11 tests pasa al 100%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.24B.1 Investigar en código 6502 y documentar en `research/RES-019B-corridor-world-assembly-and-seam-parity.md` (`asm_researcher`).
  - [x] 6.24B.2 Corregir la cinemática de scroll bidireccional y banderas `RF_AtScrollEdge` / `RF_SectionStart` en `src/core/mjk_camera.c` e `include/mjk_camera.h` (`c_implementer`).
  - [x] 6.24B.3 Implementar el renderizado continuo multi-sala y posicionamiento de objetos en `src/core/mjk_game.c` (`c_implementer`).
  - [x] 6.24B.4 Implementar colisiones continuas multi-sala a través de fronteras en `src/core/mjk_player.c` (`c_implementer`).
  - [x] 6.24B.5 Conectar la transición suave de `CurrentRoomID` al avanzar o retroceder de sala durante el scroll en `src/core/mjk_state_machine.c` (`c_implementer`).
  - [x] 6.24B.6 Crear `tests/test_corridor_scroll.c`, integrarlo en `build.bat` (TEST 11/11), compilar y validar al 100% (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-029` - Paridad Canónica de Mapeo de Objetos (Bombas y Cofres), Eliminación de Glitches Visuales y Arnés de Verificación con Emulador (Cynes)
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Crítica (Corrección de mapeado, renderizado continuo de objetos y paridad visual)
- **Fuentes ASM**: 
  - `asm/src/prg.asm:3257-3450` (`ReadRoomData`, `HandleRoomDataObject`, parsing exacto de cabeceras `$E0`, puertas `$8x` y bombas con contador inicial).
  - `asm/src/prg.asm:3540-3680` (`sub_9592`, cálculo de coordenadas de bombas `$YX` y avance de punteros).
  - `asm/src/prg.asm:5369-5408` (`MaybeDrawFireBombSprite`, chispa oculta `$F8` en pasillos).
  - `asm/src/roomdata-us.asm:4-8` (`RoomData_01` y `RoomData_02`, coordenadas de bombas y cofres).
  - `asm/src/data.asm:600-650` (`ItemToTileTable`).
  - `asm/src/data.asm:1704-1800` (`AdjacentRoomsTable`).
- **Objetivo Cumplido**:
  1. **Decodificación Exacta de Mapeo de Bombas y Cofres**:
     - Parser de `RoomData` en `src/core/mjk_room_decoder.c` corregido para interpretar con fidelidad de ciclo el conteo de bombas escalar y sus coordenadas `$YX`:
       * Sala 1: 5 bombas en ($85, $87, $39, $6B, $6D) y 2 cofres rojos en el suelo en $C6 (Y=192, X=96) y $CC (Y=192, X=192).
       * Sala 2: 6 bombas (2 sobre plataforma `$41, $43`, 2 bajo plataforma `$67, $69`, 2 en escalera `$8D, $8F`) y 3 cofres en $C2, $C8, $CE.
  2. **Renderizado de Bombas Apagadas (Sin Chispas Iniciales en Pasillos)**:
     - En pasillos, las bombas arrancan en estado apagado canónico (`0x05`, sin chispas OAM). Las chispas solo se activan en Royal Rooms o por propagación de mechas.
  3. **Composición Continua de Objetos Multi-Sala (Eliminación de Pop-In de Bombas)**:
     - En `src/core/mjk_game.c`, se proyectan y blittean continuamente los objetos y bombas de la sala contigua (`adj.right`) antes de cruzar la frontera, eliminando la aparición súbita a mitad de pantalla.
  4. **Eliminación del Sprite Glitcheado Provisional**:
     - Eliminado el dibujado provisional de sprite de enemigo con placeholder `m_tile = 0x30`.
  5. **Caja de Colisión Canónica de Jack con Bombas**:
     - Calibrada la detección de contacto de Jack contra bombas en `src/core/mjk_player.c` en coordenadas continuas de mundo, permitiendo recoger todas las bombas sin fallos.
  6. **Arnés Automatizado de Verificación de Mapas con Emulador (`tools/verify_room_maps_emulator.py`)**:
     - Creado arnés determinista con `cynes` y `asm/bin/mbj-us.nes` que valida los metatiles, persistencia de `RoomDataRAM` ($0603..$0622), VRAM Nametable 0 y OAM de Sala 1 y Sala 2 con 100% de paridad empírica.
- **Criterio de Aceptación**:
  - `tools/verify_room_maps_emulator.py` confirma paridad 1:1 de objetos en Sala 1 y Sala 2 contra `cynes`.
  - Las bombas y cofres coinciden píxel a píxel con las capturas de Mesen.
  - No hay chispas en bombas al iniciar el pasillo.
  - Cero sprites glitcheados en pantalla.
  - Jack puede recolectar todas las bombas sin fallos.
  - Las bombas de Sala 2 son visibles progresivamente al hacer scroll sin pop-in súbito.
  - Test unitario determinista en `build.bat` pasa al 100% (11/11 tests).
- **Subpasos (Micro-Lotes)**:
  - [x] 6.29.1 Desarrollar `tools/verify_room_maps_emulator.py` con `cynes` para volcar mapas de objetos de la ROM original (`c_implementer`).
  - [x] 6.29.2 Investigar y documentar en `research/RES-020-roomdata-object-mapping-and-emulator-parity.md` (`asm_researcher`).
  - [x] 6.29.3 Corregir el parsing de `RoomData` en `src/core/mjk_room_decoder.c` para bombas y cofres (`c_implementer`).
  - [x] 6.29.4 Corregir estado unlit de bombas en pasillos, suprimir sprite glitcheado e implementar blitteo multi-sala continuo en `src/core/mjk_game.c` (`c_implementer`).
  - [x] 6.29.5 Ajustar colisiones de recolección de Jack con bombas en `src/core/mjk_player.c` (`c_implementer`).
  - [x] 6.29.6 Validar con `verify_room_maps_emulator.py` y suite completa de tests `build.bat` al 100% (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-015` - Secuencia Canónica de Pantallas y Posicionamiento Exacto de HUD
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Media-Alta (Flujo de juego y HUD)
- **Fuentes ASM**: `asm/src/prg.asm:1783-1933`, `prg.asm:2050-2500` (Init pantallas), `prg.asm:4000-4150` y `prg.asm:7496-7700` (HUD layout exacto en Nametable/OAM).
- **Objetivo Cumplido**:
  1. **HUD Canónico en Sala 1 (Fila 2, Y=16 px)**:
     - Constantes canónicas formalizadas en `include/mjk_hud.h` (`MJK_HUD_ROW_Y = 16`, `MJK_HUD_TIMER_*`, `MJK_HUD_SCORE_*`, `MJK_HUD_MIGHTY_*`).
     - Todos los elementos del HUD alineados en la fila superior de bloques (Y=16 px):
       * Temporizador ('T' + 2 dígitos BCD) en X=56, 64, 72.
       * Puntuación (Score centrado BCD de 2 a 8 dígitos) en X=88..144 (paso 8px), PPU `$204B..$2052`.
       * Icono Mighty ('M' en X=184 y monedas en X=192), corrigiendo el desalineamiento vertical previo que se dibujaba erróneamente en Y=24.
  2. **Secuencia Canónica de Pantallas de Inicio**:
     - Pantalla 1 (Copyright): Fondo Azul Cielo (`$21`), texto en Nametable 0 y año `"1987"` inyectado en OAM tras 260 frames / timeout.
     - Pantalla 2 (Title Screen): Jack anclado en (40, 120), pirámide dorada, `"PUSH START BUTTON"` y `"HI GDV 0"`.
     - Pantalla 2b (Jack Falling): Caída parabólica calibrada a 79 frames exactos.
     - Pantalla 3 (Round Intro): Fondo negro puro `$0F`, Jack centrado en (112, 96) con vidas BCD durante 96 frames exactos.
     - Transición a partida activa en `GAME_STATE_5_MAIN_LOOP`.
- **Criterio de Aceptación**:
  - Transición fluida y determinista Copyright -> Title -> Falling -> Round Intro -> Game Loop.
  - HUD perfectamente alineado en Y=16 con idénticas posiciones de píxel a la ROM original en Mesen.
  - Test suite unitaria `tests/test_hud.c` y `tests/test_state_machine.c` pasando al 100%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.15.1 Inspeccionar en `prg.asm` la rutina de renderizado del HUD y sus coordenadas exactas en la PPU (`asm_researcher`).
  - [x] 6.15.2 Ajustar el dibujado del HUD en `src/core/mjk_game.c` e `include/mjk_hud.h` alineando a Y=16 (`c_implementer`).
  - [x] 6.15.3 Validar secuencia de pantallas y HUD con `tests/test_hud.c`, `test_state_machine.c` y `build.bat` (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-022B` - Sistema Canónico de Bombas y Encadenamiento de Mechas en Pasillos
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Alta (Mecánica central de recolección y bonificación)
- **Fuentes ASM**: 
  - `asm/src/prg.asm:4970-5164` (`JTPickup_Bomb` `$9E5B`, `loc_9EB2`, `loc_9F4B`).
  - `asm/src/prg.asm:5369-5388` (`MaybeDrawFireBombSprite`, render OAM Sprite #4).
  - `asm/src/prg.asm:5645-5697` (Animación y offsets espaciales de chispa).
  - `asm/src/ram.asm:1043-1046` (`MaybeBombThing` `$00F8`, `MaybeTempCollectableFlag2` `$0363`).
  - `asm/src/data.asm:848-870` (`DoorCond_0C_AllFireBombs`).
- **Objetivo Cumplido**:
  1. **Recolección y Puntuación BCD**:
     - Recolección apagada (`0x05`): Suma +100 puntos BCD, sonido canónico y anula la racha perfecta (`MaybeBombThing |= 0x02`), extinguiendo la mecha.
     - Recolección encendida (`bit 6 = 0x40`): Suma +200 puntos BCD, sonido canónico y busca secuencialmente hacia adelante en `RoomDataRAM` la siguiente bomba intacta para activarle la mecha (`val |= 0x40`).
  2. **Bonus Perfecto de 10.000 Puntos**:
     - Si Jack recolecta todas las bombas de la sección en orden encendido (`MaybeBombThing == 1`), al recoger la última bomba encendida se fija `MaybeBombThing = 0x81`, se otorgan 10.000 pts BCD, sonido de bonus y se satisface la condición `DoorCond_0C_AllFireBombs` (consumiendo el bit 7 al abrir la puerta).
  3. **Umbral de Power Coin**:
     - Acumulador ponderado (`$0356`): +1 por bomba apagada, +2 por bomba encendida. Al alcanzar `>= 20`, arma el trigger (`$0357 = 1`) para soltar la moneda `P` en el siguiente cofre en salas normales, o spawn directo en Bomb Rooms.
  4. **Salas de Bombas (Royal Rooms $90..$9F)**:
     - 23 bombas con wrapping circular (`loc_9F4B`), conteo exacto `$00AE/$00AF`, apertura de puerta y chispa oculta en `$F8`.
  5. **Sprite Animado OAM #4 (Chispa de Mecha)**:
     - Offsets espaciales exactos `(Y - 4, X + 8)`, atributos `0x12` y alternancia de tiles `0x0D <-> 0x0F` cada 3 frames sobre la bomba con mecha encendida.
- **Criterio de Aceptación**:
  - Encadenamiento de mechas 100% canónico verificado en pasillos y Bomb Rooms.
  - Chispa parpadeante visible sobre la bomba activa.
  - Test suite determinista [`tests/test_bomb_chain.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_bomb_chain.c) cubriendo los 9 requisitos canónicos integrada como TEST 12/12 en [`build.bat`](file:///c:/Users/kanel/Documents/mjk/build.bat) pasando al 100%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.22B.1 Investigar y documentar en `research/RES-022-bomb-mechanics-and-fuse-chains.md` (`asm_researcher`).
  - [x] 6.22B.2 Implementar `MjkRoom_CollectBombCanonical` y estructuras en `mjk_room_decoder.c` y `mjk_types.h` (`c_implementer`).
  - [x] 6.22B.3 Conectar despacho de recolección, bonus BCD y Power Coin en `mjk_state_machine.c` (`c_implementer`).
  - [x] 6.22B.4 Implementar animación y renderizado OAM Sprite 4 en `mjk_game.c` (`c_implementer`).
  - [x] 6.22B.5 Crear `tests/test_bomb_chain.c`, integrarlo en `build.bat` (TEST 12/12) y validar suite completa (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-023` - Cofres del Tesoro (Rojos, Naranjas) y Revelación de Secretos en Pasillos
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Alta (Interacción con objetos, apertura y secretos)
- **Fuentes ASM**: 
  - `asm/src/prg.asm:4860-4965` (`MaybePickupItem`, `JTPickup_Chest`, `JTPickup_Chest2`, `loc_9E23`).
  - `asm/src/prg.asm:5288-5305` (Golpe con la cabeza desde abajo y requisitos de nivel Mighty).
  - `asm/src/prg.asm:5390-5450` (`MaybeOpenHiddenBlock`, bloques secretos `$5N/$6N`).
  - `asm/src/data.asm:600-650` (`ItemToTileTable`).
- **Objetivo Cumplido**:
  1. **Física e Impacto Vertical Canónico**:
     - Arriba (salto/caída): apertura al primer impacto para cofres rojos normales visibles (`loc_9E23`); 2 impactos para cofres ocultos (el 1º revela el cofre, el 2º lo abre).
     - Abajo (golpe de cabeza): requiere `PlayerMightyLevel >= 2` para cofres rojos normales, o `PlayerMightyLevel >= 1` para cofres naranjas; cofres ocultos inmunes desde abajo.
  2. **Tipología Canónica de Cofres y Secretos**:
     - Cofres visibles rojos (`$10`), naranjas (`$11`), ocultos (`$30`), bloques multi-golpe (`$5N/$6N`) requiriendo 3 impactos con registro canónico en `$0335/$0336`.
  3. **Transición Visual y Persistencia**:
     - Transición a cofre abierto (`metatile 0x4D`, tiles `{0xEC, 0xED, 0xEE, 0xEF}`) y persistencia dinámica en `RoomDataRAM` (`0x00B0..$00CF`).
  4. **Spawn de Contenidos**:
     - Suelta de `Power Coin P` si `$0357 == 1` fijando trigger a 3 y timer a 320 frames.
     - Sustitución de `Tecmo Plate` por `Mighty Coin` si ya se recolectó el 1UP de la ronda.
- **Criterio de Aceptación**:
  - Interacción física y apertura al saltar o golpear los cofres 100% verificada.
  - Test suite determinista [`tests/test_chests_secrets.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_chests_secrets.c) cubriendo los 8 casos de prueba integrada como TEST 13/13 en [`build.bat`](file:///c:/Users/kanel/Documents/mjk/build.bat) pasando al 100%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.23.1 Investigar y documentar en `research/RES-023-chests-and-secret-blocks.md` (`asm_researcher`).
  - [x] 6.23.2 Implementar detección de impacto vertical y requisitos de nivel Mighty en `mjk_player.c` y `mjk_game.c` (`c_implementer`).
  - [x] 6.23.3 Implementar spawn de ítems, Power Coins y bloques secretos en `mjk_room_decoder.c` y `mjk_state_machine.c` (`c_implementer`).
  - [x] 6.23.4 Crear `tests/test_chests_secrets.c`, integrarlo en `build.bat` (TEST 13/13) y validar suite completa (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-025` - Power-Ups y Niveles de Poder Mighty (Mighty Drinks, Monedas P y E, Nivel Mighty 1..3)
- **Responsable**: Orquestador Secundario (Nivel 2) coordinando a `asm_researcher`, `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Prioridad**: Media-Alta (Habilidades del jugador y paletas de Jack)
- **Fuentes ASM**: 
  - `asm/src/prg.asm:1397-1420` (`_MightyLevelAPressCheck`, presupuesto de saltos por nivel).
  - `asm/src/prg.asm:1745-1779` (`UseMightyCoin`, consumo con botón B).
  - `asm/src/prg.asm:4860-5020` (Power Coin `loc_9DDC`, Mighty Drink `loc_9DFF`, Extra Coin `loc_9DF2`).
  - `asm/src/prg.asm:5797-5815` (`MaybeAward1UP`).
  - `asm/src/prg.asm:9022-9050` (`TurnEnemiesIntoCoins`, transmutación de 320 frames).
  - `asm/src/data.asm:306-310` (`MightyLevelColors` `$16, $12, $26, $2A`, `MightyLevelAPressesTable` `1, 20, 30, 35`).
  - `asm/src/data.asm:848-879` (`ScoreAddTable` `200, 300, 500, 800, 1200, 2000`).
- **Objetivo Cumplido**:
  1. **Niveles Mighty y Presupuesto de Saltos**:
     - Niveles 0..3 gestionados con presupuesto canónico de pulsaciones de salto botón A (`1, 20, 30, 35`).
     - Degradación escalonada al agotarse el contador de saltos: nivel 3 pasa a 2 en <30, nivel 2 a 1 en <20, nivel 1 a 0 en <1.
     - Conmutación cromática de Jack en la subpaleta de sprite 1 en tiempo real (`$16, $12, $26, $2A`).
     - Habilitación de facultades: apertura de cofres naranjas (nivel $\ge 1$), apertura inferior con la cabeza de cofres rojos (nivel $\ge 2$), y apertura de puertas secretas `DoorCond_XX_Mighty3` (nivel 3).
  2. **Power Coin (Moneda P)**:
     - Temporizador de transmutación activo durante 320 frames (~5.3 s) fijando `enemy_coin_timer`.
     - Multiplicador progresivo canónico de puntos al recolectar monedas transmutadas (`200 -> 300 -> 500 -> 800 -> 1200 -> 2000 pts BCD`).
  3. **Mighty Drink y Extra Coin (Moneda E)**:
     - Mighty Drink: otorga +1000 pts BCD, +16s BCD al temporizador y penalización por avaricia con confiscación de monedas y transporte a Sala de Tortura si el tiempo $\ge$ `$A0`.
     - Extra Coin: otorga +1 vida BCD protegida con flag de concesión única por ronda.
- **Criterio de Aceptación**:
  - Progresión de niveles Mighty, paletas cromáticas dinámicas, transmutación de 320 frames y 1UP verificados al 100%.
  - Test suite determinista [`tests/test_powerups_mighty.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_powerups_mighty.c) cubriendo los 7 bloques canónicos integrada como TEST 14/14 en [`build.bat`](file:///c:/Users/kanel/Documents/mjk/build.bat) pasando al 100%.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.25.1 Investigar y documentar en `research/RES-024-powerups-and-mighty-levels.md` (`asm_researcher`).
  - [x] 6.25.2 Implementar gestión de niveles Mighty, paletas y saltos en `mjk_player.c` y `mjk_types.h` (`c_implementer`).
  - [x] 6.25.3 Implementar temporizador Power Coin, Mighty Drink y 1UP en `mjk_game.c` y `mjk_state_machine.c` (`c_implementer`).
  - [x] 6.25.4 Crear `tests/test_powerups_mighty.c`, integrarlo en `build.bat` (TEST 14/14) y validar suite completa (`emulator_verifier`).







---

#### `[DONE]` `TASK-F6-026` - Generador de Enemigos (Spawns, Momias, Mutación y Colisiones)
- **Responsable**: Orquestador de Tarea Efímero (Nivel 2)
- **Fecha de Cierre**: 2026-10-25
- **Fuentes ASM**:
  - `asm/src/prg.asm:8150-8450` (`HandleEnemySpawnTimer`, `_SpawnEnemy`, `InitEnemy`)
  - `asm/src/prg.asm:9208-9575` (`ObjInit_0_Mummy`, `ObjCode_0_Mummy`, `TransformMummy`)
  - `asm/src/data.asm:116-141` (`DifficultyTable`), `1963-2000` (`EnemySpawnPositionTable`)
- **Objetivo Cumplido**:
  1. **Spawning Diferido Canónico**:
     - Conteo de cadencia de generación de enemigos según `DifficultyTable` (160 frames en Round 1/2, 128 frames en Round 3, etc.).
     - Límite estricto de máximo 4 enemigos vivos simultáneamente (`AliveEnemyCount < 4`).
     - Detención de spawn durante transmutación activa por Power Coin (`EnemyCoinTimer > 0`).
  2. **IA y Físicas de Momias**:
     - Patrullaje horizontal, inversión de marcha en límites de pantalla (16..224 px) y ante metatiles sólidos de sala.
     - Detección de suelo y caída por gravedad si se supera el borde de una plataforma.
  3. **Fórmula Determinista de Mutación**:
     - Conteo de temporizador de sala hasta `transform_timer` (128 frames en pasillo / 64 frames en bomb room).
     - Mutación modular exacta `(APressCounter % 6) + 2` derivando en los 6 arquetipos canónicos: Hanezo (2), GejiShogun (3), Gameido (4), Dokuron (5), Desufa (6) y Horus (7).
  4. **Colisiones y Reglas de Combate**:
     - Colisión AABB 12x12 contra Jack.
     - Sin poderes: letal para Jack (`is_alive = false`).
     - Power Coin activa: consumo pacífico de moneda de oro con escala progresiva (100 -> 8000 pts).
     - Jack con Mighty Nivel 3 (verde): arrollamiento y destrucción de enemigos sin peligro (+500 pts).
- **Criterio de Aceptación**:
  - Paridad demostrada en test suite determinista [`tests/test_enemy_spawns.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_enemy_spawns.c) integrada en [`build.bat`](file:///c:/Users/kanel/Documents/mjk/build.bat) como TEST 15/15 pasando al 100%.
  - Binario `bin/mjk.exe` compilando con cero errores y advertencias.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.26.1 Investigar y documentar en `research/RES-025-enemy-ai-spawns-and-mutations.md` (`asm_researcher`).
  - [x] 6.26.2 Implementar `MjkEnemyManager`, dificultad y físicas de momias en `src/core/mjk_enemy.c` (`c_implementer`).
  - [x] 6.26.3 Conectar colisiones letales, transmutación y Mighty nivel 3 en `src/core/mjk_state_machine.c` y `mjk_game.c` (`c_implementer`).
  - [x] 6.26.4 Crear `tests/test_enemy_spawns.c`, integrarlo en `build.bat` (TEST 15/15) y validar suite completa (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-027` - Sistema de Feedback Visual de Puntuación Flotante
- **Responsable**: Orquestador de Tarea Efímero (Nivel 2)
- **Fecha de Cierre**: 2026-10-25
- **Fuentes ASM**:
  - `asm/src/prg.asm:2548-2573` (`sub_8FB1`, `SetupBombScorePopupTiles`)
  - `asm/src/ram.asm:929-940` (`byte_30B..byte_310`)
- **Objetivo Cumplido**:
  1. **Generación Dinámica de Marcadores**:
     - Spawneo de etiquetas flotantes en coordenadas relativas a la pantalla al recoger bombas (+100 / +200), cofres (+100..+10000), monedas transmutadas (+100..+8000) o eliminar enemigos con Mighty Nivel 3 (+500).
  2. **Cinemática Ascendente y Desvanecimiento**:
     - Ascenso suave a velocidad constante de 1 píxel por frame durante una vida útil de 30 frames.
     - Desvanecimiento y reciclaje limpio del slot en el búfer circular sin fugas ni artefactos visuales.
  3. **Concurrencia de 8 Slots**:
     - Gestión simultánea e independiente de hasta 8 marcadores activos sin solapamiento destructivo.
- **Criterio de Aceptación**:
  - Marcadores flotantes ascendentes verificados al 100% en test suite determinista [`tests/test_floating_scores.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_floating_scores.c) integrado en [`build.bat`](file:///c:/Users/kanel/Documents/mjk/build.bat) como TEST 16/16.
  - Binario `bin/mjk.exe` operativo y compilando limpiamente.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.27.1 Investigar rutinas y slots en `asm/src/prg.asm:2548-2573` (`asm_researcher`).
  - [x] 6.27.2 Implementar estructuras y ciclo de vida de `MjkFloatingScore` en `src/core/mjk_game.c` (`c_implementer`).
  - [x] 6.27.3 Conectar spawneo en bombas, cofres y combate en `src/core/mjk_state_machine.c` (`c_implementer`).
  - [x] 6.27.4 Crear `tests/test_floating_scores.c`, integrarlo en `build.bat` (TEST 16/16) y validar suite completa (`emulator_verifier`).

---

#### `[DONE]` `TASK-F6-028` - Mecánica de Codicia, Sala de Tortura (50 Saltos) y Sistema GDV
- **Responsable**: Orquestador de Tarea Efímero (Nivel 2)
- **Fecha de Cierre**: 2026-10-25
- **Fuentes ASM**:
  - `asm/src/prg.asm:698-772` (`GameState_7_GoToTortureRoom`, `loc_84A5`, `GreedyJumpsRemaining`)
  - `asm/src/prg.asm:1040-1117` (`State_A_GameOver`, `AddAToGDV`, `BCDCheckFirstDigit`)
  - `asm/src/prg.asm:7377-7425` (`EndingDoorChecks`, `_EndGame`)
  - `asm/src/ram.asm:925-945` (`TortureRoomSceneFlag`, `TortureRoomMessageTimer`, `GreedyJumpsRemaining`)
- **Objetivo Cumplido**:
  1. **Detector de Codicia**:
     - Activación canónica si `greed_jump_counter >= 35` o si Jack toma un Mighty Drink con el tiempo $\ge 100$ segundos.
     - Confiscación total de monedas Mighty en inventario (`mighty_coins = 0`).
     - Almacenamiento seguro de `checkpoint_room_id` para garantizar retorno a la sala correcta.
  2. **Sala de Tortura ($AC)**:
     - Mensaje inicial de 192 frames (`YOU ARE GREEDY` / `GO TO THE TORTURE ROOM`).
     - Penitencia de 50 saltos obligatorios (en BCD) en la versión USA.
     - Conteo decreciente por cada salto realizado por Jack con botón A.
     - Retorno automático y transparente a la sala previa tras cumplir la penitencia de 50 saltos.
  3. **Sistema GDV y Desbloqueo de Finales**:
     - Implementación estricta de la fórmula decimal BCD `AddAToGDV`.
     - Cálculo en Game Over: bonificación por rondas superadas (hasta 7), vidas extras encontradas (1 por cada 2) y puntuación (hasta 5).
     - Bonificación de +3 puntos por alcanzar cualquier final de juego.
     - Evaluación determinista de los 4 finales canónicos (0 a 3) según posesión de Monedas Secretas (5 requeridas) y Crystal Balls (2 requeridas).
- **Criterio de Aceptación**:
  - 100% verificado en test suite determinista [`tests/test_torture_room_gdv.c`](file:///c:/Users/kanel/Documents/mjk/tests/test_torture_room_gdv.c) integrada en [`build.bat`](file:///c:/Users/kanel/Documents/mjk/build.bat) como TEST 17/17.
  - Binario `bin/mjk.exe` operativo y compilando limpiamente.
- **Subpasos (Micro-Lotes)**:
  - [x] 6.28.1 Extraer la lógica de detección de codicia, umbrales y Sala de Tortura de `prg.asm` (`asm_researcher`).
  - [x] 6.28.2 Implementar `GAME_STATE_7_TORTURE_ROOM` y cuenta regresiva de 50 saltos en `src/core/mjk_state_machine.c` (`c_implementer`).
  - [x] 6.28.3 Conectar la fórmula de cálculo del GDV y finales en `src/core/mjk_game.c` (`c_implementer`).
  - [x] 6.28.4 Crear `tests/test_torture_room_gdv.c`, integrarlo en `build.bat` (TEST 17/17) y validar suite completa (`emulator_verifier`).

---


---

### Fase 7: Bug Fixing, Paridad Exacta y Refactorización

#### `[DONE]` `TASK-F7-001` - Desactivación Temporal de Enemigos en Runtime para Debug Aislado

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-028-enemy-sprites-and-ai-bugs.md`](../research/RES-028-enemy-sprites-and-ai-bugs.md)
- **Objetivo**: Desactivar temporalmente el spawn, actualización, colisiones y rendering OAM de enemigos en el bucle principal de juego para facilitar las pruebas limpias de las tareas `TASK-F7-002` a `TASK-F7-009`, preservando intactos los tests unitarios.
- **Subpasos Realizados**:
  - [x] Desactivado en `src/core/mjk_state_machine.c` (`State_5_MainLoop` Paso 8) el bloque de spawn, update y colisiones de enemigos con guarda `#if 0`, fijando `game->active_enemies_count = 0`.
  - [x] Desactivado en `src/core/mjk_state_machine.c` (`State_5_MainLoop` Paso 10) el ajuste de cámara sobre `game->enemies`.
  - [x] Desactivado en `src/core/mjk_game.c` (`RenderInGameScreen` Paso 8) el bucle de renderizado OAM de `game->enemies`.
  - [x] Preservados intactos `src/core/mjk_enemy.c`, `State_0`, `State_2` y `State_3`.
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` y 17/17 tests pasando al 100% en `build.bat test`.


#### `[DONE]` `TASK-F7-002` - Eliminación de Calaveras Amarillas y Puertas Fantasma Hardcodeadas

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-029-glitched-map-sprites-cleanup.md`](../research/RES-029-glitched-map-sprites-cleanup.md)
- **Objetivo**: Eliminar definitivamente los artefactos visuales de calaveras amarillas en `(14, 8)` producidos por confusión de IDs de metatiles con tiles CHR de fondo y purgar las puertas fantasma inyectadas en pasillos continuos.
- **Subpasos Realizados**:
  - [x] Eliminado el Paso 3c en `src/core/mjk_game.c` que inyectaba tiles de calavera `0x94..0x97` en `(door_tile_x, door_tile_y)`.
  - [x] Corregida la subpaleta de cofres en Pasos 3b y 3b-right usando `MjkRoomData_GetMetatileSubpalette(chest_mt)` (`0x4A` Naranja sub=0, `0x28` Rojo sub=3).
  - [x] Inicializado `door_tile_x = 0xFF; door_tile_y = 0xFF;` en `MjkGame_Init` y `MjkGame_StartNewGame`.
  - [x] Eliminada en `src/core/mjk_state_machine.c` (`State_2_LoadRoom` y `State_5_MainLoop`) la rama `else` que inyectaba puertas ficticias en `(14, 8)`.
  - [x] Protegida la apertura y colisión de puertas condicionalmente a `door_tile_x != 0xFF`.
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe`, 17/17 tests passing al 100% y verificación visual empírica de 0 píxeles amarillos residuales en `(224..239, 128..143)` en Sala 1.


#### `[DONE]` `TASK-F7-003` - Corrección Integral del Motor de Audio APU, Multiplexación de 8 Canales, Fin de Pitch Bend en BGM y Fin de White Noise

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-030-apu-sfx-and-pitch-bend-fix.md`](../research/RES-030-apu-sfx-and-pitch-bend-fix.md)
- **Objetivo**: Corregir la arquitectura del sintetizador APU emulando los 8 canales virtuales de sonido del original 6502 (`0..3` BGM, `4..7` SFX), eliminando la desafinación de la música en Pulse 1 (`p1_period -= 14`), erradicando el ruido blanco espurio de `SFX_OPEN_CHEST` y reproduciendo el bytecode canónico de los efectos de sonido.
- **Subpasos Realizados**:
  - [x] Ampliado el rango de extracción de música a `0xFFA0` en `tools/extract_assets.py` y regenerado `assets/assets.dat`.
  - [x] Ampliado `channels[4]` a `channels[8]` virtuales en `MjkAPUState` (`include/mjk_apu.h`, `src/core/mjk_apu.c`).
  - [x] Implementados punteros canónicos y decodificación bytecode de todos los SFX (`$01..$23`).
  - [x] Implementado en `Channel_Tick` el Hardware Sweep (`$F5`) y Software Pitch Bend acumulativo (`$D8..$DF`).
  - [x] Implementada en `MjkAPU_UpdateFrame` la multiplexación limpia de 8 a 4 canales hardware con prioridad de SFX, eliminando código ad-hoc y stuck noise.
  - [x] Conectados eventos de gameplay a SFX canónicos (`$01` salto, `$03` cofre, `$04` moneda/ítem, `$07` bomba apagada, `$08` bomba encendida, `$0E` bloque secreto, `$11` puerta abierta, etc.).
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` (231,936 bytes) y 17/17 tests passing al 100% en `build.bat test` (incluyendo validación determinista de barrido hardware sweep y pitch inmutable en Pulse 1 BGM).


#### `[DONE]` `TASK-F7-004` - Paridad Exacta de Cofres (Subpaletas Rojo/Naranja, Metatiles de Ítems y Física de 4 Sondas)

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-031-chest-palette-and-collision-parity.md`](../research/RES-031-chest-palette-and-collision-parity.md)
- **Objetivo**: Corregir el renderizado cromático de los cofres cerrados (Rojo `0x28` Subpaleta 3, Naranja `0x4A` Subpaleta 0), la tabla canónica `ItemToTileTable[0..8]` con sus subpaletas y la interacción física de 4 sondas de `sub_9BD7`.
- **Subpasos Realizados**:
  - [x] Subpaletas canónicas en `RenderInGameScreen` y `MjkGame_OpenChest`: `sub = (chest->type == 0) ? 3 : 0`.
  - [x] Metatiles canónicos de ítems `{ 0x4D, 0x4E, 0x4F, 0x47, 0x49, 0x27, 0x34, 0x48, 0x8F }` y subpaletas `{ 3, 3, 1, 0, 3, 3, 0, 0, 0 }`.
  - [x] Implementadas las 4 sondas exactas de `sub_9BD7` (`probe_head`, `probe_feet`, `probe_left/right`, y `probe_center` para recolección de ítem).
  - [x] Snap vertical `pos_y = FIXED_FROM_INT(cy - 16)` al rebotar encima de un cofre, eliminando el bug de autoconsumo accidental en 1 frame.
  - [x] Solidez condicional de cofre naranja como plataforma sólida con `mighty_level == 0`, techo sólido inferior con apertura si `mighty_level >= 1` y apertura lateral si `mighty_level >= 2`.
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` y 17/17 tests passing al 100% (312/312 asserts OK).


#### `[DONE]` `TASK-F7-005` - Corrección de Colisiones de Recolección de Ítems/Power-Ups y Metatiles Pasables

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-032-item-powerup-pickup-collision-fix.md`](../research/RES-032-item-powerup-pickup-collision-fix.md)
- **Objetivo**: Declarar no sólidos los metatiles de ítems coleccionables y power-ups en `mjk_player_is_tile_solid`, eliminar el bloqueo lateral de Jack y permitir la recolección suave mediante la sonda central canónica `byte_A = (jx + 8, jy + 8)`.
- **Subpasos Realizados**:
  - [x] En `src/core/mjk_player.c` (`mjk_player_is_tile_solid`): marcados como no sólidos (`return false;`) los metatiles `{ 0x27, 0x2F, 0x34, 0x47..0x50, 0x63, 0x8F, 0x90, 0x91 }`, cofres dinámicos (`0x28`, `0x4A`) y marcos de puertas abiertas (`0x81..0x98`).
  - [x] En `src/core/mjk_game.c` (`MjkGame_UpdateChestsAndSecrets`): implementada recolección por sonda central `(jx + 8, jy + 8)` compensando `camera_x`, `camera_y` y multi-sala.
  - [x] En `src/core/mjk_game.c` (`MjkGame_CollectChestItem`): integrados efectos de `gold_coins_collected++`, `total_bombs_collected++`, `power_coin_bomb_counter++`, `MjkRoom_HandleOpenDoorMaybe` con Sphinx/Royal Family y sincronización a 320 frames de transmutación.
  - [x] En `tests/test_chests_secrets.c`: añadido subtest 9 de validación de no-solidez y recolección.
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` (232,448 bytes) y 17/17 tests passing al 100% en `build.bat test`.


#### `[DONE]` `TASK-F7-006` - Límites de Scroll en Pasillos, Descarte de Bomb Rooms y Fin de Bombas en Pared Derecha

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-033-corridor-scroll-limits-and-bomb-placement.md`](../research/RES-033-corridor-scroll-limits-and-bomb-placement.md)
- **Objetivo**: Corregir los límites de scroll en terminales de pasillo horizontal (Sala 6 en Round 1), congelando la cámara en `scroll_x = 0`, desanclando a Jack de $X=128$ para caminar libremente hacia la puerta ($X=224$) y la pared ($X=240$), y descartando la precarga y renderizado de la sala de bombas `$90` (Royal Palace) sobre el muro derecho.
- **Subpasos Realizados**:
  - [x] Implementada en `include/mjk_room_decoder.h` y `src/core/mjk_room_decoder.c` la función `MjkRoom_HasHorizontalScrollNeighborRight(room_id)` que descarta destinos `>= 0x90` y bordes de scroll (`adj->flags & 0x02`).
  - [x] Restaurada en `src/core/mjk_camera.c` la guarda canónica `loc_A505` (`asm/src/prg.asm:6045-6055`) congelando `scroll_x = 0` y activando `RF_AtScrollEdge` (`0x08`).
  - [x] Condicionada en `src/core/mjk_state_machine.c` (`State_2_LoadRoom`) la precarga de bombas y entidades derechas exclusivamente a `MjkRoom_HasHorizontalScrollNeighborRight`.
  - [x] Condicionado en `src/core/mjk_game.c` (`RenderInGameScreen`) el dibujo de nametable y bombas contiguas a la existencia de un vecino de scroll válido.
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` y 17/17 suites de pruebas pasando al 100% (198/198 tests unitarios OK).


#### `[DONE]` `TASK-F7-007` - Corrección de Fuente/Sprites de Puntuación Flotante con Glifos Compactos Canónicos OAM

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-034-floating-score-canonical-sprites.md`](../research/RES-034-floating-score-canonical-sprites.md)
- **Objetivo**: Sustituir el renderizado de puntuación flotante con números grandes del HUD de fondo por los metasprites compactos de 16×8 píxeles de Pattern Table 0 (`0x30..0x3F`) con Subpaleta Sprite 0 (blanco puro NES `$30`).
- **Subpasos Realizados**:
  - [x] Implementada en `src/core/mjk_game.c` la función canónica `MjkFloatingScore_GetTiles` asignando prefijos (`0x31..0x3A`), base `"00"` (`0x30`) y multiplicadores `"x2".."x5"` (`0x3C..0x3F`).
  - [x] Reemplazado en `RenderInGameScreen` (Paso 9) el formateo de texto grande por el blit de los tiles compactos con `&pal_argb[16]`.
  - [x] Verificado el alineamiento visual en `(fx-8, fy-8)` [TL], `(fx, fy-8)` [TR] y `(fx, fy)` [BR].
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` y 17/17 tests passing al 100% en `build.bat test`.


#### `[DONE]` `TASK-F7-008` - Corrección del Bloqueo Permanente de Jack al Retroceder hacia la Derecha en Pasillos

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-035-player-rightward-movement-lock-fix.md`](../research/RES-035-player-rightward-movement-lock-fix.md)
- **Objetivo**: Eliminar el congelamiento y bloqueo total de Jack al retroceder hacia la derecha tras cruzar salas en pasillos con scroll, suprimiendo la activación simultánea espuria de `CF_RIGHT_WALL` y `CF_LEFT_WALL`.
- **Subpasos Realizados**:
  - [x] En `src/core/mjk_player.c` (`mjk_player_is_tile_solid`): sustituido el muro sólido artificial ante `target_x >= 256` o `< 0` por el wrap modular canónico (`target_x &= 0xFF`) de `loc_9A39` (`asm/src/prg.asm:4274-4286`).
  - [x] En `src/core/mjk_camera.c`: eliminada la inyección prematura de `MJK_RF_SECTION_START` al retroceder hacia la izquierda con `ScrollX = 255` (`loc_A498`/`loc_A4DD`), restringiéndola a bordes sin sala izquierda y limpiándola al re-anclar en $X=128$ hacia la derecha.
  - [x] En `tests/test_corridor_scroll.c`: añadido test de regresión verificando avance, retroceso prolongado (`ScrollX = 255`) y reingreso continuo sin bloqueos (`1 -> 2 -> 1 -> 2`).
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` (233,984 bytes) y 17/17 suites de pruebas pasando al 100% en `build.bat test`.


#### `[DONE]` `TASK-F7-009` - Animación en 2 Fases (Humo Puff Metasprite `$25` + Puntuación) al Recoger Bombas

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-036-bomb-pickup-puff-animation.md`](../research/RES-036-bomb-pickup-puff-animation.md)
- **Objetivo**: Reproducir la secuencia de animación en 2 fases de 32 frames cada una del original 6502 (`sub_A270` / `sub_A29C`), mostrando primero una nube de humo 16×16 simétrica (Metasprite `$25` con 4 tiles `$8C`, idéntico al spawn de enemigos) y posteriormente el valor en puntos flotantes.
- **Subpasos Realizados**:
  - [x] En `include/mjk_game.h`: definida la estructura `MjkBombPuffEffect` (activo, fase 1 o 2, temporizador de 32 frames, coordenadas x/y, puntos) y pool de 8 puffs.
  - [x] En `src/core/mjk_room_decoder.c`: implementadas `MjkGame_SpawnBombPuff` y `MjkGame_UpdateBombPuffs`, sustituyendo el spawn inmediato de puntos por el puff.
  - [x] En `src/core/mjk_game.c`: renderizado de Fase 1 con el metasprite canónico `$25` (4 tiles `$8C` simétricos con Subpaleta Sprite 0) y Fase 2 con los glifos compactos de `RES-034`.
  - [x] En `src/core/mjk_state_machine.c`: compensación de scroll de cámara sobre las coordenadas de puffs activos.
  - [x] En `tests/test_bomb_chain.c`: añadido subtest 10 verificando el ciclo completo de 64 frames (32 frames humo -> 32 frames puntos -> liberación).
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe` (235,520 bytes) y 17/17 tests passing al 100% en `build.bat test`.


#### `[DONE]` `TASK-F7-010` - Corrección Integral de Sprites OAM 16×16 e IA/Físicas de Enemigos y Reactivación Final

- **Responsable**: Nivel 1 (Orquestador) -> `task_orchestrator` -> `c_implementer` y `emulator_verifier`
- **Fecha de Cierre**: 2026-10-25
- **Informe de Referencia**: [`research/RES-028-enemy-sprites-and-ai-bugs.md`](../research/RES-028-enemy-sprites-and-ai-bugs.md)
- **Objetivo**: Implementar metasprites 16×16 (2×2 tiles de 8×8) para todos los arquetipos de enemigos usando `SpritesTable` y `SpriteAnimationTable`, suprimir definitivamente los glifos numéricos, integrar el ciclo de spawn/mutación de 32 frames (16f invisible + 16f humo `$25`), preservar subpíxeles Q8.8, abortar spawns en Sala 1 (`raw_pos == 0`), añadir patrullaje en plataformas con giro en bordes y reactivar los enemigos en el bucle principal.
- **Subpasos Realizados**:
  - [x] En `src/core/mjk_game.c`: implementado `BlitMetasprite16x16` (con flip H/V) y renderizado de Momia, Hanezo, Geji Shogun, Gameido, Dokuron, Desufa, Horus, humo de spawn `$25` y moneda transmutada `$26`.
  - [x] En `src/core/mjk_enemy.c`: tabla de spawns de 256 salas, aborto estricto si `raw_pos == 0`, ciclo de 32 frames de spawn/mutación, preservación Q8.8 a 0.5 px/f y detección de bordes de plataforma.
  - [x] En `src/core/mjk_state_machine.c`: reactivados Pasos 8 y 10 en `State_5_MainLoop` (IA, colisiones, scoring, audio y cámara).
- **Criterio de Aceptación Verificado**: Compilación limpia de `bin/mjk.exe`, 17/17 tests passing al 100% y verificación empírica de 0 enemigos en Sala 1, ciclo de humo de 32 frames en Sala 2 y velocidad exacta Q8.8 de 0.5 px/frame.
