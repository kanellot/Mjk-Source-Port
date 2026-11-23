# Estado del Proyecto: Mighty Bomb Jack Native Port

**Última actualización**: 2026-10-23  
**Hito Activo**: Hito 3 - Extracción de Assets desde ROM NES, Resolución de Rutas y Audio Auténtico `[COMPLETADO AL 100%]`  
**Versión Base Canónica**: **USA / NTSC (`REV_US`)** conforme a [ADR-004](file:///c:/Users/kanel/Documents/mjk/DECISIONS.md#adr-004-adopción-de-la-revisión-usa-rev_us-como-golden-standard-del-port-nativo)  
**Paridad Objetivo**: 100% de paridad funcional y determinista con la versión NES USA (Tecmo 1987) documentada en `asm/`

---

## 1. Visión General del Proyecto

El proyecto consiste en portar de manera nativa y fiel el clásico *Mighty Bomb Jack* de NES a PC en C/C++ (`port_native`), tomando como referencia inmutable y desensamblado fidedigno el código en `asm/`.

```
                  +--------------------------------------+
                  |   Mighty Bomb Jack (NES Tecmo 1987)  |
                  +--------------------------------------+
                                     |
              +----------------------+----------------------+
              |                                             |
              v                                             v
    [Fuente de Verdad]                             [Port Nativo PC]
          asm/                                       port_native/
    * 6502 Disassembly                             * C11 Engine (Raylib 5.5)
    * Solo lectura (+R)                            * Cero activos comerciales en código
    * Inmutable (Bit-Exact)                        * Carga ROM externa (mbj-us.nes)
                                                   * Audio 2A03 a 48 kHz nativo
```

---

## 2. Matriz de Componentes del Port

| Componente | Subdirectorio / Ubicación | Estado Actual | Nivel de Paridad | Observaciones |
| :--- | :--- | :--- | :--- | :--- |
| **Gobernanza y Memoria** | Raíz (`./`) | `[ACTIVO]` | 100% | Reset completado, permisos fijados, 5 archivos de memoria activos, protocolo de micro-lotes (ADR-003). |
| **Base de Conocimiento** | `research/` | `[COMPLETADA]` | 100% | 11 informes técnicos completados (`RES-001` a `RES-011`) que cubren el 100% de la arquitectura 6502 y certifican 0 puntos ciegos. |
| **Inventario Determinista** | `tools/` | `[ACTIVO]` | 100% | `tools/extract_asm_inventory.py`, `asm_inventory.json` (1.764 símbolos, 294 subrutinas, 15 JumpTables). |
| **Build System** | `port_native/` | `[COMPLETADO]` | 100% | CMake y `build.bat` multi-config MSVC 19.50, copia automática de assets y CTest en verde (`TASK-BUILD-001`). |
| **Cargador Universal ROM** | `port_native/src/asset/` | `[COMPLETADO]` | 100% | `rom_loader.c`: Carga `mbj-us.nes`, extrae 32KB CHR y 32KB PRG audio en tiempo de ejecución. Código fuente 100% libre de copyright (`TASK-ROM-001`). |
| **Resolución de Rutas** | `port_native/src/platform/` | `[COMPLETADO]` | 100% | Detección automática de la ROM en múltiples rutas (junto a `.exe`, carpeta de trabajo, `assets/`, `asm/bin/`, o por CLI). Doble clic directo desde el Explorador operativo (`TASK-ROM-002`). |
| **Core State Machine** | `port_native/src/core/` | `[COMPLETADO]` | 100% | Estructura de RAM `$0000-$07FF`, Zero-Page scratchpad y máquina de estados 60 Hz (`TASK-IMPL-001`). |
| **Render y Presentación** | `port_native/src/render/` | `[COMPLETADO]` | 100% | Canvas virtual 256x240, decodificador 2bpp CHR, paletas 2C02, OAM compositor, pantalla completa 4:3 dinámico y soporte Xbox Series XInput (`TASK-IMPL-002`). |
| **Física y Jack** | `port_native/src/core/` | `[COMPLETADO]` | 100% | Curva de salto parabólica de 64 pasos (`word_C0E3`/`word_C163`), aleteo y sondas 9-probe (`TASK-IMPL-003`). |
| **Escenarios y Salas** | `port_native/src/room/` | `[COMPLETADO]` | 100% | Decodificador 4-Tier para las 257 salas, metatiles, chunks y 32 condiciones de puerta USA (`TASK-IMPL-004`). |
| **Bombas y Cofres** | `port_native/src/core/` | `[COMPLETADO]` | 100% | Cadena de 23 bombas, mechas encendidas, apertura de cofres, Mighty Coins y Power Coins (`TASK-IMPL-005`). |
| **Enemigos e IA** | `port_native/src/core/`| `[COMPLETADO]` | 100% | 6 arquetipos (Hanezo, Geji, Gameido, Dokuron, Desufa, Horus), mutación determinista `(A % 6) + 2` y Power Coin 320 frames (`TASK-IMPL-006`). |
| **HUD, GDV y Ending** | `port_native/src/core/` | `[COMPLETADO]` | 100% | Aritmética BCD, HUD en pantalla, Sala de Tortura (50 saltos US), fórmula de GDV y los 4 finales (`TASK-IMPL-007`). |
| **Audio Engine 2A03** | `port_native/src/audio/`| `[COMPLETADO]` | 100% | Streaming continuo a 48 kHz nativo (`SetAudioStreamCallback`) e intérprete del bytecode musical original de Tecmo (`TASK-ROM-003`, `TASK-ROM-004`). |
| **Test Suite** | `port_native/tests/` | `[COMPLETADO]` | 100% | Suite automatizada CTest (`test_suite.exe`) con 7 módulos pasando al 100% (`TASK-ROM-005`). |

---

## 3. Estado de la Infraestructura

- **Permisos de `asm/`**: Bloqueada como `Read-Only` en sistema de archivos y por constitución de agentes.
- **Permisos de `port_native/` y Raíz**: `Read-Write` comprobados y verificados.
- **Binario Final**: `port_native/build_vs/bin/Release/mighty_bomb_jack.exe` compilado y enlazado limpiamente junto con `assets/` y `mbj-us.nes`.
- **Suite de Pruebas**: 7/7 tests pasando en CTest (BCD math, Sala de Tortura 50 saltos, mutación de momias, 4 finales, decodificador 4-tier, síntesis APU y carga universal de ROM).
- **Mando y Controles**: Totalmente compatible con mando de Xbox Series X|S / Xbox One mediante XInput y teclado (WASD / Flechas / Z / X).
- **Audio en Vivo**: Sintetizador de sonido por software 2A03 emitiendo en tiempo real a 48.000 Hz mediante callback continuo en Raylib/miniaudio.
