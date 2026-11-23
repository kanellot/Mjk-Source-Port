# Índice Maestro de Investigaciones Técnicas y Reconstrucción Funcional

Este catálogo centraliza todas las investigaciones técnicas, ingeniería inversa del desmontaje 6502 (`asm/`), especificaciones funcionales y contratos de comportamiento del proyecto *Mighty Bomb Jack*.

---

## 1. Estructura de Directorios de Especificación (Iteración 2)

A partir de la Iteración 2, los artefactos de especificación funcional se organizan por fases dentro de `research/`:

```
research/
├── scope/           -> Fase 0: Delimitación de alcance, arquitectura ROM y plan
│   ├── scope.md
│   ├── repository_inventory.md
│   ├── rom_layout.md
│   └── analysis_plan.md
│
├── inventories/     -> Fase 1: Inventarios exhaustivos generados con soporte de herramientas
│   ├── code_inventory.csv
│   ├── data_inventory.csv
│   ├── memory_map.md
│   ├── variable_usage.csv
│   └── unresolved_regions.md
│
├── control_flow/    -> Fase 2: Flujo de control, datos y modelo de hardware
│   ├── control_flow.md
│   ├── data_flow.md
│   ├── hardware_model.md
│   └── unresolved_dependencies.md
│
├── contracts/       -> Fase 3: Reconstrucción funcional y contratos de comportamiento
│   ├── state_machine.md
│   ├── state_transition_matrix.csv
│   ├── behavior_catalog.md
│   ├── behavior_contracts/
│   ├── timing_model.md
│   └── input_semantics.md
│
├── audit/           -> Fase 4: Auditoría de cobertura y certificación
│   ├── traceability_matrix.csv
│   ├── coverage_report.md
│   ├── unresolved_questions.md
│   └── audit_report.md
│
├── CROSS_REFERENCE_INDEX.md -> Índice maestro de referencias cruzadas (ASM 6502 <-> C99)
├── INDEX.md                 -> Este catálogo maestro
├── TEMPLATE.md              -> Plantilla estandarizada para nuevos informes RES-###
└── RES-*.md         -> Informes técnicos profundos de referencia (enriquecibles)
```

---

## 2. Política de Actualización de Informes `RES-*.md` Existentes

Los informes técnicos previos ([`RES-001`](./RES-001-mapa-memoria-y-subsistemas-nes.md) a [`RES-011`](./RES-011-auditoria-puntos-ciegos-y-discrepancias.md)) se preservan como estudios profundos de referencia.
- **Se pueden y deben modificar**: Conforme la investigación estática de las Fases 0 a 4 revele detalles más precisos, correcciones de etiquetas o contratos ABI exactos, se actualizarán directamente.
- **Enriquecimiento Continuo**: Cualquier nuevo informe técnico detallado que se redacte para profundizar en un aspecto específico seguirá la plantilla `TEMPLATE.md` y se registrará en este índice con el siguiente ID disponible (`RES-012`, etc.).

---

## 3. Registro Central de Investigaciones Técnicas (`RES-###`)

| ID | Título | Área Temática | Tarea Origen | Estado | Fecha | Archivo |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| [`RES-001`](./RES-001-mapa-memoria-y-subsistemas-nes.md) | Mapa de Memoria RAM, Punteros Base y Enumeraciones de la NES | 01 - Núcleo y Memoria | `TASK-SETUP-003` | `[COMPLETADA]` | 2026-10-22 | [RES-001-mapa-memoria-y-subsistemas-nes.md](./RES-001-mapa-memoria-y-subsistemas-nes.md) |
| [`RES-002`](./RES-002-analisis-naturaleza-desensamblado-asm.md) | Naturaleza del Desensamblado `asm/` (Total vs Parcial) y Estrategia de Inventariado | 01 - Núcleo y Memoria | `TASK-RES-001` | `[COMPLETADA]` | 2026-10-22 | [RES-002-analisis-naturaleza-desensamblado-asm.md](./RES-002-analisis-naturaleza-desensamblado-asm.md) |
| [`RES-003`](./RES-003-inventario-maestro-simbolos-y-grafo-llamadas.md) | Inventario Maestro de Símbolos, Tablas de Salto (`JumpTable`) y Grafo de Llamadas | 01 - Núcleo y Memoria | `TASK-RES-002` | `[COMPLETADA]` | 2026-10-22 | [RES-003-inventario-maestro-simbolos-y-grafo-llamadas.md](./RES-003-inventario-maestro-simbolos-y-grafo-llamadas.md) |
| [`RES-004`](./RES-004-subsistema-nucleo-nmi-ppu-estados.md) | Especificación Funcional: Boot, Pipeline NMI, PPU Buffer, Joypad y Máquina de Estados | 01 - Núcleo y Memoria | `TASK-RES-003` | `[COMPLETADA]` | 2026-10-22 | [RES-004-subsistema-nucleo-nmi-ppu-estados.md](./RES-004-subsistema-nucleo-nmi-ppu-estados.md) |
| [`RES-005`](./RES-005-especificacion-completa-ram-y-zeropage.md) | Especificación Funcional: Mapa Completo de RAM (`$0000-$07FF`) y Scratchpad Zero-Page | 01 - Núcleo y Memoria | `TASK-RES-004` | `[COMPLETADA]` | 2026-10-22 | [RES-005-especificacion-completa-ram-y-zeropage.md](./RES-005-especificacion-completa-ram-y-zeropage.md) |
| [`RES-006`](./RES-006-subsistema-fisica-jack-y-colisiones.md) | Especificación Funcional: Física de Jack, Salto 64 Pasos, Sondas 9-Probe y Scroll | 02 - Física y Colisiones | `TASK-RES-005` | `[COMPLETADA]` | 2026-10-22 | [RES-006-subsistema-fisica-jack-y-colisiones.md](./RES-006-subsistema-fisica-jack-y-colisiones.md) |
| [`RES-007`](./RES-007-subsistema-salas-puertas-cofres-bombas.md) | Especificación Funcional: Decodificador 4-Tier de 257 Salas, Bombas, Cofres y Puertas | 03 - Escenarios y Salas | `TASK-RES-006` | `[COMPLETADA]` | 2026-10-22 | [RES-007-subsistema-salas-puertas-cofres-bombas.md](./RES-007-subsistema-salas-puertas-cofres-bombas.md) |
| [`RES-008`](./RES-008-subsistema-ia-enemigos-spawns-monedas.md) | Especificación Funcional: IA de Enemigos, Ciclo de Spawns, Mutación y Monedas | 04 - Enemigos e IA | `TASK-RES-007` | `[COMPLETADA]` | 2026-10-22 | [RES-008-subsistema-ia-enemigos-spawns-monedas.md](./RES-008-subsistema-ia-enemigos-spawns-monedas.md) |
| [`RES-009`](./RES-009-subsistema-pantallas-especiales-hud-gdv-ending.md) | Especificación Funcional: HUD BCD, Sala de Tortura (50 Saltos US), GDV y Finales | 01 - Núcleo y Pantallas | `TASK-RES-008` | `[COMPLETADA]` | 2026-10-22 | [RES-009-subsistema-pantallas-especiales-hud-gdv-ending.md](./RES-009-subsistema-pantallas-especiales-hud-gdv-ending.md) |
| [`RES-010`](./RES-010-subsistema-audio-2a03-y-bytecode-musical.md) | Especificación Funcional: Motor de Audio 2A03 y Bytecode Musical Tecmo | 06 - Motor de Audio / APU | `TASK-RES-009` | `[COMPLETADA]` | 2026-10-22 | [RES-010-subsistema-audio-2a03-y-bytecode-musical.md](./RES-010-subsistema-audio-2a03-y-bytecode-musical.md) |
| [`RES-011`](./RES-011-auditoria-puntos-ciegos-y-discrepancias.md) | Auditoría de Puntos Ciegos, Idiomas 6502 (Double Return `PLA; PLA`) y Variantes | 01 - Núcleo y Memoria | `TASK-AUDIT-000` | `[COMPLETADA]` | 2026-10-22 | [RES-011-auditoria-puntos-ciegos-y-discrepancias.md](./RES-011-auditoria-puntos-ciegos-y-discrepancias.md) |
| [`RES-012`](./RES-012-sprites-jack-oam-y-paletas-ground-truth.md) | Sprites de Jack, Ensamblado OAM y Paletas Canónicas (Ground Truth 6502) | 05 - Renderizado y PPU | `TASK-F6-016` | `[COMPLETADA]` | 2026-10-25 | [RES-012-sprites-jack-oam-y-paletas-ground-truth.md](./RES-012-sprites-jack-oam-y-paletas-ground-truth.md) |
| [`RES-013`](./RES-013-apu-timing-and-audio-sync.md) | Secuencia de Inicialización APU 2A03, Sincronización Temporal de Audio y Control de Volumen | 06 - Motor de Audio / APU | `TASK-RES-010` | `[COMPLETADA]` | 2026-10-25 | [RES-013-apu-timing-and-audio-sync.md](./RES-013-apu-timing-and-audio-sync.md) |
| [`RES-014`](./RES-014-sound-bytecode-engine.md) | Especificación Técnica y Extracción del Motor de Bytecode de Audio 2A03 y Pistas Canónicas | 06 - Motor de Audio / APU | `SUBTAREA 1` | `[COMPLETADA]` | 2026-10-25 | [RES-014-sound-bytecode-engine.md](./RES-014-sound-bytecode-engine.md) |
| [`RES-015`](./RES-015-jack-physics-and-flutter.md) | Cinemática 6502 de Jack, Salto Canónico de 64 Pasos y Mecánica de Aleteo (Flutter) | 02 - Física y Colisiones | `TASK-F6-019` | `[COMPLETADA]` | 2026-10-25 | [RES-015-jack-physics-and-flutter.md](./RES-015-jack-physics-and-flutter.md) |
| [`RES-016`](./RES-016-collision-system-9probe.md) | Sistema Canónico de Colisiones de Jack (9-Probe), Resolución de Metatiles y Geometría de Contacto | 02 - Física y Colisiones | `TASK-F6-020` | `[COMPLETADA]` | 2026-10-25 | [RES-016-collision-system-9probe.md](./RES-016-collision-system-9probe.md) |
| [`RES-017`](./RES-017-door-system-and-transitions.md) | Sistema de Puertas, Evaluación de 33 Condiciones Canónicas y Transición de Salas | 03 - Escenarios y Salas | `TASK-F6-021` | `[COMPLETADA]` | 2026-10-25 | [RES-017-door-system-and-transitions.md](./RES-017-door-system-and-transitions.md) |
| [`RES-018`](./RES-018-roomdata-bytecode-and-persistent-ram.md) | Bytecode de RoomData, Persistencia en RoomDataRAM (8 Slots) y Blitteo de Objetos | 03 - Escenarios y Salas | `TASK-F6-022` | `[COMPLETADA]` | 2026-10-25 | [RES-018-roomdata-bytecode-and-persistent-ram.md](./RES-018-roomdata-bytecode-and-persistent-ram.md) |
| [`RES-019`](./RES-019-scroll-engine-and-camera.md) | Cinemática de Cámara, Motor de Scroll y Streaming de Seam en Nametables PPU | 03 - Escenarios y Salas | `TASK-F6-024` | `[COMPLETADA]` | 2026-10-25 | [RES-019-scroll-engine-and-camera.md](./RES-019-scroll-engine-and-camera.md) |
| [`RES-019B`](./RES-019B-corridor-world-assembly-and-seam-parity.md) | Ensamblado Continuo de Pasillos, Cinemática de Desanclaje y Costura Multi-Sala (Seam Parity) | 03 - Escenarios y Salas | `TASK-F6-024B` | `[COMPLETADA]` | 2026-10-25 | [RES-019B-corridor-world-assembly-and-seam-parity.md](./RES-019B-corridor-world-assembly-and-seam-parity.md) |
| [`RES-020`](./RES-020-roomdata-object-mapping-and-emulator-parity.md) | Paridad Canónica de Mapeo de Objetos (Bombas y Cofres), Decodificación de Bytecode y Arnés de Verificación con Emulador | 03 - Escenarios y Salas | `TASK-F6-029` | `[COMPLETADA]` | 2026-10-25 | [RES-020-roomdata-object-mapping-and-emulator-parity.md](./RES-020-roomdata-object-mapping-and-emulator-parity.md) |
| [`RES-028`](./RES-028-enemy-sprites-and-ai-bugs.md) | Desactivación Limpia en Runtime (`TASK-F7-001`), Metasprites 16x16 (`SpritesTable`/`SpriteAnimationTable`) y Diagnóstico de IA/Spawns (`TASK-F7-010`) | 04 - Enemigos e IA | `TASK-F7-001` / `TASK-F7-010` | `[COMPLETADA]` | 2026-10-25 | [RES-028-enemy-sprites-and-ai-bugs.md](./RES-028-enemy-sprites-and-ai-bugs.md) |
| [`RES-029`](./RES-029-glitched-map-sprites-cleanup.md) | Diagnóstico Forense de Tiles Glitcheados ("Calaveras Amarillas" `(14,8)`), Puertas Fantasma Hardcodeadas y Subpaleta de Cofres (`TASK-F7-002`) | 03 - Escenarios y Salas / 05 - Renderizado y PPU | `TASK-F7-002` | `[COMPLETADA]` | 2026-10-25 | [RES-029-glitched-map-sprites-cleanup.md](./RES-029-glitched-map-sprites-cleanup.md) |
| [`RES-030`](./RES-030-apu-sfx-and-pitch-bend-fix.md) | Diagnóstico Forense de Efectos de Sonido (SFX), Salto, Pitch Bend Inducido en Música y Ruido Blanco (`TASK-F7-003`) | 06 - Motor de Audio / APU | `TASK-F7-003` | `[COMPLETADA]` | 2026-10-25 | [RES-030-apu-sfx-and-pitch-bend-fix.md](./RES-030-apu-sfx-and-pitch-bend-fix.md) |
| [`RES-031`](./RES-031-chest-palette-and-collision-parity.md) | Paridad Canónica de Cofres — Paleta/Subpaleta, Comportamiento y Colisiones con Jack (`TASK-F7-004`) | 03 - Escenarios y Salas / 02 - Física y Colisiones / 05 - Renderizado y PPU | `TASK-F7-004` | `[COMPLETADA]` | 2026-10-25 | [RES-031-chest-palette-and-collision-parity.md](./RES-031-chest-palette-and-collision-parity.md) |
| [`RES-032`](./RES-032-item-powerup-pickup-collision-fix.md) | Colisiones de Recolección de Ítems y Power-Ups tras Abrir un Cofre — Física de Tiles, Scroll y Paridad 6502 (`TASK-F7-005`) | 02 - Física y Colisiones / 03 - Escenarios y Salas | `TASK-F7-005` | `[COMPLETADA]` | 2026-04-06 | [RES-032-item-powerup-pickup-collision-fix.md](./RES-032-item-powerup-pickup-collision-fix.md) |
| [`RES-033`](./RES-033-corridor-scroll-limits-and-bomb-placement.md) | Límites de Scroll en Pasillos Horizontales y Posicionamiento de Bombas en Frontera (`TASK-F7-006`) | 03 - Escenarios y Salas / 05 - Renderizado y PPU | `TASK-F7-006` | `[COMPLETADA]` | 2026-10-25 | [RES-033-corridor-scroll-limits-and-bomb-placement.md](./RES-033-corridor-scroll-limits-and-bomb-placement.md) |
| [`RES-034`](./RES-034-floating-score-canonical-sprites.md) | Fuente y Sprites Canónicos de Puntuación Flotante al Recoger Objetos (`TASK-F7-007`) | 05 - Renderizado y PPU / 03 - Objetos de Sala | `TASK-F7-007` | `[COMPLETADA]` | 2026-10-25 | [RES-034-floating-score-canonical-sprites.md](./RES-034-floating-score-canonical-sprites.md) |
| [`RES-035`](./RES-035-player-rightward-movement-lock-fix.md) | Diagnóstico Forense y Corrección del Bloqueo de Movimiento de Jack al Retroceder hacia la Derecha en Pasillos (`TASK-F7-008`) | 02 - Física y Colisiones / 03 - Escenarios y Salas | `TASK-F7-008` | `[COMPLETADA]` | 2026-10-25 | [RES-035-player-rightward-movement-lock-fix.md](./RES-035-player-rightward-movement-lock-fix.md) |
| [`RES-036`](./RES-036-bomb-pickup-puff-animation.md) | Animación Canónica de Humo (Puff) al Recoger Bombas y Paridad con Spawns de Enemigos (`TASK-F7-009`) | 05 - Renderizado y PPU / 03 - Escenarios y Salas / 04 - Enemigos e IA | `TASK-F7-009` | `[COMPLETADA]` | 2026-10-25 | [RES-036-bomb-pickup-puff-animation.md](./RES-036-bomb-pickup-puff-animation.md) |
| [`VERIFY-019`](./VERIFY-019-jack-physics-parity.md) | Reporte de Verificación Empírica y Control de Paridad de Físicas de Jack | 02 - Física y Colisiones | `TASK-F6-019` | `[COMPLETADA]` | 2026-10-25 | [VERIFY-019-jack-physics-parity.md](./VERIFY-019-jack-physics-parity.md) |

---

## 4. Clasificación Temática

- **01. Núcleo, Memoria y Pipeline de Frame**: `RES-001`, `RES-002`, `RES-003`, `RES-004`, `RES-005`, `RES-009`, `RES-011`.
- **02. Cinemática, Física y Colisiones (Jack)**: `RES-006`, `RES-015`, `RES-016`, `RES-031`, `RES-032`, `RES-035`.
- **03. Escenarios, Metatiles y Geometría de Salas**: `RES-007`, `RES-017`, `RES-018`, `RES-019`, `RES-019B`, `RES-020`, `RES-029`, `RES-031`, `RES-032`, `RES-033`, `RES-034`, `RES-035`, `RES-036`.
- **04. Enemigos, Inteligencia Artificial y Spawns**: `RES-008`, `RES-028`, `RES-036`.
- **05. Sistema de Renderizado, PPU y HUD**: `RES-004`, `RES-005`, `RES-007`, `RES-009`, `RES-012`, `RES-028`, `RES-029`, `RES-031`, `RES-033`, `RES-034`, `RES-036`.
- **06. Motor de Audio 2A03 y Bytecode Musical**: `RES-010`, `RES-013`, `RES-014`, `RES-030`.




