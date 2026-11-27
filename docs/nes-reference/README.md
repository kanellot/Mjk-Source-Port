# Referencia Técnica NES — Mighty Bomb Jack (`REV_US`, 1987)

Especificación técnica del juego original de NES (versión USA / NTSC, `REV_US`), extraída directamente del desensamblado 6502 en `asm/src/`.

> Para la arquitectura y código del source port en C11, consulta el [Índice Maestro de Documentación](../README.md).

---

## Documentos por Subsistema

1. **[Flujo Completo del Juego](game-flow.md)** — Pantallas (Copyright, Title, Round Intro), máquina de estados (`GameState 0..$C`), 17 rondas, Sala de Tortura y los 4 finales.
2. **[Layout de ROM y Mapa de Memoria](rom-and-memory-map.md)** — Cabecera iNES, Mapper 3 (CNROM), bancos PRG/CHR, vectores 6502 y mapa de RAM `$0000–$07FF`.
3. **[Gráficos, PPU 2C02 y HUD](graphics-and-ppu.md)** — `SpritesTable`, ensamblado OAM de metasprites `16×16`, paletas de Jack/Mighty y coordenadas exactas del HUD y textos.
4. **[Audio 2A03 y Motor Musical](audio-2a03.md)** — Inicialización APU, cola de comandos (`$00..$23`), 8 canales virtuales, tabla de períodos, duraciones, envolventes y gramática de bytecode.
5. **[Física de Jack y Colisiones](physics-and-collision.md)** — Cinemática horizontal, curva de salto de 64 pasos (`word_C0E3` / `word_C163`), *flutter* y detector de colisión 9-probe.
6. **[Mundo, Salas, Cámara y Puertas](world-and-rooms.md)** — Decodificador 4-tier de 256 salas, persistencia RAM, scroll horizontal/vertical, costuras, objetos y 33 condiciones de puerta.
7. **[Mecánicas de Gameplay y Enemigos](gameplay-mechanics.md)** — Bombas y mechas, cofres y bloques secretos, Mighty Power, 6 arquetipos de enemigos, popups, Sala de Tortura y GDV.
8. **[Mapa ASM 6502 ↔ C11](asm-to-c-map.md)** — Correspondencia directa entre variables RAM / subrutinas 6502 y los módulos actuales del source port (`src/` e `include/mjk/`).

---

## Inventarios Tabulares (`inventories/`)

Datos estructurados generados a partir del desensamblado 6502 (utilizados también por `tools/asm_query.py`):

- [`inventories/code_inventory.csv`](inventories/code_inventory.csv) — Inventario de subrutinas 6502.
- [`inventories/data_inventory.csv`](inventories/data_inventory.csv) — Inventario de tablas de datos en ROM.
- [`inventories/variable_usage.csv`](inventories/variable_usage.csv) — Lectores y escritores por variable RAM.
- [`inventories/state_transition_matrix.csv`](inventories/state_transition_matrix.csv) — Matriz de transiciones de estado.
