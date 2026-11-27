# Documentación Técnica — Mighty Bomb Jack (C11)

Bienvenido a la documentación del source port multiplataforma en **C11 estándar + SDL2** de *Mighty Bomb Jack* (NES USA 1987, `REV_US`).

---

## 1. Documentación del Source Port (C11)

Guías para entender, compilar, probar y extender el motor actual:

- **[Arquitectura Técnica](ARCHITECTURE.md)** — Diseño en 5 capas, ciclo de un frame a `60.0988 Hz` y mapa de archivos.
- **[Sistemas de Juego](GAME_SYSTEMS.md)** — Lógica pura en `src/game/`: entrada, puntuación, mundo/colisiones, física de Jack, bombas, cofres, 33 puertas, 6 enemigos y 4 finales.
- **[Assets BYOR, Renderizado y Audio 2A03](ASSETS_AND_AUDIO.md)** — Extractor Clean-Room BYOR v2, renderizador solo-lectura `256×240` con caché L1 y sintetizador 2A03 con LUTs.
- **[Guía de Compilación y Testing](TESTING_GUIDE.md)** — CMake en Windows/Linux/macOS, 11 suites `CTest`, `SKIP_RETURN_CODE 77` y sanitizers.
- **[Decisiones de Diseño](DESIGN_DECISIONS.md)** — Decisiones arquitectónicas vigentes del proyecto.
- **[Historial de Cambios](CHANGELOG.md)** y **[Hoja de Ruta](ROADMAP.md)** — Evolución del port y posibles mejoras futuras.

---

## 2. Referencia Técnica del Juego Original (NES `REV_US`)

Especificación extraída directamente del desensamblado 6502 (`asm/src/`), organizada por subsistema sin dependencias de la implementación en C:

- **[Índice de Referencia NES](nes-reference/README.md)**
  - [Flujo Completo del Juego](nes-reference/game-flow.md) — Pantallas, máquina de estados, 17 rondas, Sala de Tortura y 4 finales.
  - [Layout de ROM y Mapa de Memoria](nes-reference/rom-and-memory-map.md) — PRG/CHR (CNROM), RAM `$0000–$07FF` y hardware 2C02/2A03.
  - [Gráficos, PPU 2C02 y HUD](nes-reference/graphics-and-ppu.md) — Paletas, sprites OAM, metasprites, HUD y textos.
  - [Audio 2A03 y Motor Musical](nes-reference/audio-2a03.md) — Timing NTSC y secuenciador de bytecode de 8 canales.
  - [Física de Jack y Colisiones](nes-reference/physics-and-collision.md) — Salto de 64 pasos, flutter y colisión 9-probe.
  - [Mundo, Salas, Cámara y Puertas](nes-reference/world-and-rooms.md) — Decodificador 4-tier de 256 salas, scroll, objetos y 33 condiciones de puerta.
  - [Mecánicas de Gameplay y Enemigos](nes-reference/gameplay-mechanics.md) — Bombas, cofres, Mighty Power, 6 enemigos, popups, Torture Room y GDV.
  - [Mapa ASM 6502 ↔ C11](nes-reference/asm-to-c-map.md) — Correspondencia directa entre símbolos 6502 y módulos C11 actuales.
  - [Inventarios CSV](nes-reference/inventories/) — Subrutinas, tablas de datos, uso de RAM y matriz de estados.
