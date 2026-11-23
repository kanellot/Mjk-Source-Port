# Índice Maestro de Documentación Técnica (`research/`)

Este catálogo indexa únicamente los artefactos activos y verificados de ingeniería inversa de *Mighty Bomb Jack* (NES USA 1987, `REV_US`). Los borradores preliminares y reportes falsos/obsoletos han sido depurados para minimizar el ruido y el consumo de tokens.

---

## 1. Referencias Primarias (Consulta Directa Recomendada)

| Recurso | Propósito |
| :--- | :--- |
| [`GAME_COMPLETE_FLOW.md`](./GAME_COMPLETE_FLOW.md) | Mapa maestro del juego completo (Copyright -> Title -> 17 Rondas -> Torture -> 4 Finales -> Game Over). |
| [`CROSS_REFERENCE_INDEX.md`](./CROSS_REFERENCE_INDEX.md) | Diccionario maestro bidireccional RAM/Subrutinas/Tablas `asm/src/` <-> `include/` y `src/core/`. |
| [`tools/asm_query.py`](../tools/asm_query.py) | CLI determinista para consultar subrutinas, RAM R/W, JumpTables y tablas ROM desde `asm_inventory.json`. |
| [`docs/EDUCATIONAL_ARCHITECTURE.md`](../docs/EDUCATIONAL_ARCHITECTURE.md) | Guía pedagógica del diseño retro (Q8.8, BCD, 4-tier rooms, APU 2A03, `PLA;PLA`). |

---

## 2. Especificación Estructurada por Fases

| Directorio | Archivos Activos | Contenido |
| :--- | :--- | :--- |
| `scope/` | [`scope.md`](./scope/scope.md), [`rom_layout.md`](./scope/rom_layout.md) | Alcance `REV_US` y mapa de ROM NROM-256. |
| `inventories/` | `code_inventory.csv`, `data_inventory.csv`, `variable_usage.csv`, [`memory_map.md`](./inventories/memory_map.md), [`unresolved_regions.md`](./inventories/unresolved_regions.md) | Inventario AST completo (294 subrutinas, 525 tablas, 337/338 símbolos RAM). |
| `control_flow/` | [`control_flow.md`](./control_flow/control_flow.md), [`data_flow.md`](./control_flow/data_flow.md), [`hardware_model.md`](./control_flow/hardware_model.md), [`unresolved_dependencies.md`](./control_flow/unresolved_dependencies.md) | Pipeline NMI, 15 JumpTables, 16 `PLA;PLA`, PPU/APU/Joypads. |
| `contracts/` | [`state_machine.md`](./contracts/state_machine.md), `state_transition_matrix.csv`, [`behavior_catalog.md`](./contracts/behavior_catalog.md), [`timing_model.md`](./contracts/timing_model.md), [`input_semantics.md`](./contracts/input_semantics.md), `behavior_contracts/*.md` | Máquina de estados `0..C` y contratos formales de física, salas, IA y HUD. |
| `audit/` | `traceability_matrix.csv`, [`unresolved_questions.md`](./audit/unresolved_questions.md) | Trazabilidad símbolo <-> contrato y diferencias JP vs US resueltas. |
| `architecture/` | [`source_port_architecture.md`](./architecture/source_port_architecture.md), [`asset_extractor_spec.md`](./architecture/asset_extractor_spec.md), [`verification_strategy.md`](./architecture/verification_strategy.md) | Arquitectura C11 multiplataforma, BYOR v2 en C y verificación en 3 capas. |

---

## 3. Especificaciones ASM Profundas (`RES-012` a `RES-027`)

| ID | Tema | Archivo |
| :--- | :--- | :--- |
| `RES-012` | Sprites de Jack, OAM y Paletas Canónicas | [RES-012-sprites-jack-oam-y-paletas-ground-truth.md](./RES-012-sprites-jack-oam-y-paletas-ground-truth.md) |
| `RES-013` | Inicialización APU 2A03 y Sincronización Temporal | [RES-013-apu-timing-and-audio-sync.md](./RES-013-apu-timing-and-audio-sync.md) |
| `RES-014` | Motor de Bytecode Musical y SFX 2A03 | [RES-014-sound-bytecode-engine.md](./RES-014-sound-bytecode-engine.md) |
| `RES-015` | Cinemática de Jack, Salto 64 Pasos y Aleteo (Flutter) | [RES-015-jack-physics-and-flutter.md](./RES-015-jack-physics-and-flutter.md) |
| `RES-016` | Sistema de Colisiones 9-Probe y Metatiles | [RES-016-collision-system-9probe.md](./RES-016-collision-system-9probe.md) |
| `RES-017` | Sistema de Puertas, 33 Condiciones y Transiciones | [RES-017-door-system-and-transitions.md](./RES-017-door-system-and-transitions.md) |
| `RES-018` | Bytecode `RoomData` y Persistencia `RoomDataRAM` | [RES-018-roomdata-bytecode-and-persistent-ram.md](./RES-018-roomdata-bytecode-and-persistent-ram.md) |
| `RES-019` | Motor de Scroll, Cámara y Streaming PPU | [RES-019-scroll-engine-and-camera.md](./RES-019-scroll-engine-and-camera.md) |
| `RES-019B` | Ensamblado Continuo de Pasillos y Seam Parity | [RES-019B-corridor-world-assembly-and-seam-parity.md](./RES-019B-corridor-world-assembly-and-seam-parity.md) |
| `RES-020` | Mapeo de Bombas/Cofres y Paridad de Objetos | [RES-020-roomdata-object-mapping-and-emulator-parity.md](./RES-020-roomdata-object-mapping-and-emulator-parity.md) |
| `RES-021` | Layout de HUD en Nametable/OAM y Flujo de Pantallas | [RES-021-hud-layout-and-screen-flow.md](./RES-021-hud-layout-and-screen-flow.md) |
| `RES-022` | Mecánicas de Bombas, Mechas y Puertas de Fuego | [RES-022-bomb-mechanics-and-fuse-chains.md](./RES-022-bomb-mechanics-and-fuse-chains.md) |
| `RES-023` | Cofres del Tesoro y Bloques Secretos | [RES-023-chests-and-secret-blocks.md](./RES-023-chests-and-secret-blocks.md) |
| `RES-024` | Power-Ups, Niveles Mighty y Transmutación | [RES-024-powerups-and-mighty-levels.md](./RES-024-powerups-and-mighty-levels.md) |
| `RES-025` | Spawning Diferido de Enemigos, IA y Mutaciones | [RES-025-enemy-ai-spawns-and-mutations.md](./RES-025-enemy-ai-spawns-and-mutations.md) |
| `RES-026` | Feedback Visual de Puntuación Flotante | [RES-026-floating-score-feedback.md](./RES-026-floating-score-feedback.md) |
| `RES-027` | Mecánica de Codicia, Sala de Tortura y GDV | [RES-027-greed-torture-room-and-gdv.md](./RES-027-greed-torture-room-and-gdv.md) |

---

## 4. Referencias Forenses Históricas (`RES-028` a `RES-036`)

> **Nota**: Contienen tablas y desensamblados ASM válidos y útiles; sin embargo, las citas de líneas en `src/` corresponden al estado histórico en que se redactaron y deben contrastarse con el código C actual.

| ID | Tema | Archivo |
| :--- | :--- | :--- |
| `RES-028` | Metasprites 16x16 de Enemigos e IA/Spawns | [RES-028-enemy-sprites-and-ai-bugs.md](./RES-028-enemy-sprites-and-ai-bugs.md) |
| `RES-029` | Limpieza de Tiles Residuales y Puertas Fantasma | [RES-029-glitched-map-sprites-cleanup.md](./RES-029-glitched-map-sprites-cleanup.md) |
| `RES-030` | Diagnóstico SFX, Pitch Bend y Ruido Blanco APU | [RES-030-apu-sfx-and-pitch-bend-fix.md](./RES-030-apu-sfx-and-pitch-bend-fix.md) |
| `RES-031` | Paletas, Subpaletas y Colisiones de Cofres | [RES-031-chest-palette-and-collision-parity.md](./RES-031-chest-palette-and-collision-parity.md) |
| `RES-032` | Colisiones de Recolección de Ítems y Power-Ups | [RES-032-item-powerup-pickup-collision-fix.md](./RES-032-item-powerup-pickup-collision-fix.md) |
| `RES-033` | Límites de Scroll en Pasillos y Bombas en Frontera | [RES-033-corridor-scroll-limits-and-bomb-placement.md](./RES-033-corridor-scroll-limits-and-bomb-placement.md) |
| `RES-034` | Fuente y Sprites OAM de Puntuación Flotante | [RES-034-floating-score-canonical-sprites.md](./RES-034-floating-score-canonical-sprites.md) |
| `RES-035` | Bloqueo de Movimiento hacia la Derecha en Pasillos | [RES-035-player-rightward-movement-lock-fix.md](./RES-035-player-rightward-movement-lock-fix.md) |
| `RES-036` | Animación de Humo (`Puff` `$25`) al Recoger Bombas | [RES-036-bomb-pickup-puff-animation.md](./RES-036-bomb-pickup-puff-animation.md) |
