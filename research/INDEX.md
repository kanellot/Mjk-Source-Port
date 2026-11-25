# Índice de Referencias Técnicas ASM — Mighty Bomb Jack (`REV_US`)

> Esta carpeta conserva únicamente **inventarios, especificaciones del juego original y referencias al desensamblado 6502** útiles para humanos. Los informes históricos de bugs/issues del agente han sido eliminados y la documentación técnica del código C11 actual vive en [`docs/`](../docs/ARCHITECTURE.md).

---

## 1. Visión Global y Mapas de Referencia

- [`GAME_COMPLETE_FLOW.md`](GAME_COMPLETE_FLOW.md) — Flujo completo del juego de principio a fin (Copyright, Título, 17 rondas, Bomb Rooms, Torture Room y 4 finales).
- [`CROSS_REFERENCE_INDEX.md`](CROSS_REFERENCE_INDEX.md) — Índice cruzado entre mecánicas del juego y símbolos 6502.
- [`scope/rom_layout.md`](scope/rom_layout.md) y [`scope/scope.md`](scope/scope.md) — Distribución física de la ROM `mbj-us.nes` (PRG/CHR, Mapper 3 CNROM).

---

## 2. Inventarios del Desensamblado 6502

- [`inventories/memory_map.md`](inventories/memory_map.md) — Mapa de memoria RAM y registros NES de `REV_US`.
- [`inventories/code_inventory.csv`](inventories/code_inventory.csv) — Inventario de subrutinas 6502.
- [`inventories/data_inventory.csv`](inventories/data_inventory.csv) — Inventario de tablas de datos en ROM.
- [`inventories/variable_usage.csv`](inventories/variable_usage.csv) — Uso de variables RAM por subrutina.
- [`audit/traceability_matrix.csv`](audit/traceability_matrix.csv) — Matriz de trazabilidad entre requisitos y rutinas ASM.

---

## 3. Flujo de Control y Contratos del Juego Original

- [`control_flow/control_flow.md`](control_flow/control_flow.md), [`control_flow/data_flow.md`](control_flow/data_flow.md), [`control_flow/hardware_model.md`](control_flow/hardware_model.md)
- [`contracts/state_machine.md`](contracts/state_machine.md), [`contracts/state_transition_matrix.csv`](contracts/state_transition_matrix.csv), [`contracts/input_semantics.md`](contracts/input_semantics.md), [`contracts/timing_model.md`](contracts/timing_model.md), [`contracts/behavior_catalog.md`](contracts/behavior_catalog.md)
- [`contracts/behavior_contracts/`](contracts/behavior_contracts/) — Contratos de física, salas, enemigos y puntuación.
- [`architecture/asset_extractor_spec.md`](architecture/asset_extractor_spec.md) y [`architecture/source_port_architecture.md`](architecture/source_port_architecture.md)

---

## 4. Especificaciones de Subsistemas (`RES-012`..`RES-027`)

- **Gráficos y Audio**: [`RES-012`](RES-012-sprites-jack-oam-y-paletas-ground-truth.md) (sprites/paletas), [`RES-013`](RES-013-apu-timing-and-audio-sync.md) (APU timing), [`RES-014`](RES-014-sound-bytecode-engine.md) (motor musical 8 canales).
- **Física, Colisiones y Mundo**: [`RES-015`](RES-015-jack-physics-and-flutter.md) (física y flutter), [`RES-016`](RES-016-collision-system-9probe.md) (colisiones 9-probe), [`RES-017`](RES-017-door-system-and-transitions.md) (33 puertas), [`RES-018`](RES-018-roomdata-bytecode-and-persistent-ram.md) (RoomData y persistencia), [`RES-019`](RES-019-scroll-engine-and-camera.md) / [`RES-019B`](RES-019B-corridor-world-assembly-and-seam-parity.md) (cámara y costuras), [`RES-020`](RES-020-roomdata-object-mapping-and-emulator-parity.md) (objetos).
- **Gameplay, HUD y Finales**: [`RES-021`](RES-021-hud-layout-and-screen-flow.md) (HUD/pantallas), [`RES-022`](RES-022-bomb-mechanics-and-fuse-chains.md) (bombas/mechas), [`RES-023`](RES-023-chests-and-secret-blocks.md) (cofres/secretos), [`RES-024`](RES-024-powerups-and-mighty-levels.md) (power-ups y Mighty), [`RES-025`](RES-025-enemy-ai-spawns-and-mutations.md) (IA enemigos), [`RES-026`](RES-026-floating-score-feedback.md) (puntuación flotante), [`RES-027`](RES-027-greed-torture-room-and-gdv.md) (Torture Room y GDV).
