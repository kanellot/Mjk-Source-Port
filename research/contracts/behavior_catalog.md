# Catálogo Maestro de Comportamientos y Contratos Funcionales

- **ID de Entregable**: `research/contracts/behavior_catalog.md`
- **Fase**: Fase 3 — Reconstrucción Funcional y Contratos de Comportamiento
- **Estado**: `[CONFIRMADA]`

---

## 1. Índice de Contratos de Comportamiento

Todos los comportamientos observables de *Mighty Bomb Jack* han sido especificados en contratos formales verificables:

| ID de Contrato | Subsistema | Documento de Especificación | Resumen de Alcance |
| :---: | :--- | :--- | :--- |
| **`CTR-01`** | **Núcleo & Estados** | [`state_machine.md`](./state_machine.md) | Máquina de 13 estados (`GameState 0..$C`), ciclo de encendido, Title Screen y NMI 60 Hz. |
| **`CTR-02`** | **Física & Jack** | [`behavior_contracts/jack_physics_contract.md`](./behavior_contracts/jack_physics_contract.md) | Curva de salto 64 pasos (`word_C0E3`), aceleración X (`word_C163`), aleteo y 9 sondas de colisión. |
| **`CTR-03`** | **Salas & Mecanismos** | [`behavior_contracts/room_decoder_contract.md`](./behavior_contracts/room_decoder_contract.md) | Decodificador 4-tier de 257 salas, 33 puertas USA, cadena de 23 bombas y apertura de cofres. |
| **`CTR-04`** | **Enemigos & IA** | [`behavior_contracts/enemy_ai_contract.md`](./behavior_contracts/enemy_ai_contract.md) | 6 arquetipos, ciclo de spawns, mutación determinista `(A % 6) + 2` y Power Coin de 320 frames. |
| **`CTR-05`** | **HUD & Puntuación** | [`behavior_contracts/hud_scoring_contract.md`](./behavior_contracts/hud_scoring_contract.md) | Aritmética BCD pura, temporizador 256 frames, penalización 50 saltos, fórmula GDV y 4 finales. |
| **`CTR-06`** | **Temporización** | [`timing_model.md`](./timing_model.md) | Presupuesto VBlank, cadencia NTSC a 60 Hz y catálogo completo de temporizadores RAM. |
| **`CTR-07`** | **Entrada** | [`input_semantics.md`](./input_semantics.md) | Semántica de niveles vs flancos con retardo de 1 frame en `Joypad1_ImmediateCopy`. |
| **`CTR-08`** | **Transiciones** | [`state_transition_matrix.csv`](./state_transition_matrix.csv) | Matriz completa de 22 transiciones de estado con guardas y efectos secundarios. |

---

## 2. Declaración de Cumplimiento de Fase 3

Con la generación de estos 8 contratos algorítmicos detallados, se alcanza el **100% de cobertura funcional** del alcance delimitado en la Fase 0, sin dejar mecánicas descritas en prosa ambigua ni aproximaciones.
