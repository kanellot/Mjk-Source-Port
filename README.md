# Mighty Bomb Jack — Source Port Educativo en C11

Source port multiplataforma (**Windows** y **Linux**) en **C11 estándar + SDL2** de *Mighty Bomb Jack* (NES USA 1987, `REV_US`), desarrollado con un enfoque **100 % educativo** y **Clean-Room BYOR** (*Bring Your Own ROM*).

> **Proyecto educativo basado en ingeniería inversa 6502**  
> Este port y toda su documentación técnica se han construido estudiando el desensamblado 6502 de referencia del repositorio **[`Xkeeper0/mighty-bomb-jack`](https://github.com/Xkeeper0/mighty-bomb-jack)** (autor: **Xkeeper y colaboradores**, bajo licencia **MIT**). El objetivo es mostrar cómo se traduce un cartucho clásico de NES de 64 KB (aritmética de punto fijo `Q8.8`, `BCD` sin modo decimal, compresión de salas en 4 niveles, renderizado PPU 2C02 y síntesis chiptune APU 2A03) a código C11 modular, portable y fácil de leer.

---

## Características Principales

- **Juego completo (`REV_US`)**: desde la pantalla de Copyright y Title Screen hasta las 17 rondas (257 salas), Bomb Rooms, Torture Room, los 4 finales y Game Over.
- **Núcleo puro C11 (`src/core/`)**: 19 módulos independientes sin ninguna dependencia de SDL2 ni APIs del sistema operativo, separados limpiamente de la capa HAL (`src/platform/`).
- **Clean-Room BYOR integrado en C**: **cero bytes con copyright de Tecmo** en el repositorio ni en el ejecutable. El propio binario valida la ROM original USA y genera `assets/assets.dat` al arrancar, sin scripts externos.
- **Audio chiptune 2A03 en C**: sintetizador de 4 canales hardware (Pulse 1/2 con sweep, Triangle y Noise LFSR) e intérprete de bytecode de 8 canales (4 BGM + 4 SFX) a 44.1 kHz.
- **Debug Overlay interactivo (`F1`–`F6`)**: telemetría en vivo, hitboxes 9-probe, salto directo (*warp*) a cualquiera de las 257 salas y paso frame a frame.
- **Documentación completa del proceso**: ingeniería inversa (`research/`) y memoria persistente de agentes (`project/` y `.agents/`) incluidas para estudiar todo el recorrido de diseño y verificación.

---

## Requisito Obligatorio: ROM Original USA (BYOR)

Por motivos legales y de derechos de autor, **este repositorio y sus releases no incluyen ROMs, gráficos, música ni mapas del juego original**.

Para ejecutar el port necesitas aportar tu propia copia legal de la **ROM original versión USA (`REV_US`)**:

| Parámetro | Valor esperado |
| :--- | :--- |
| **Nombre de archivo** | `mbj-us.nes` |
| **Región / Formato** | USA / NTSC (`REV_US`), cabecera iNES (Mapper 3 CNROM) |
| **Tamaño exacto** | `65 552` bytes |
| **SHA-1** | `a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7` |

Al arrancar, el ejecutable busca automáticamente `assets/assets.dat`. Si no existe, busca `mbj-us.nes` en el directorio actual o junto al ejecutable (o en la ruta pasada con `--rom`), verifica su SHA-1 y extrae `assets/assets.dat` (con verificación CRC-32).

---

## Controles (Teclado y Mando)

El port soporta **teclado** y **mandos USB/Bluetooth** (hotplug vía SDL2 GameController).

### Controles del Juego

| Acción (NES) | Teclado | Mando (Gamepad) |
| :--- | :--- | :--- |
| **Izquierda** | `←` o `A` | D-Pad Izquierda / Stick Izq. |
| **Derecha** | `→` o `D` | D-Pad Derecha / Stick Izq. |
| **Arriba** *(salto alto / entrar en puerta)* | `↑` o `W` | D-Pad Arriba / Stick Izq. |
| **Abajo** *(salto corto / caída rápida)* | `↓` o `S` | D-Pad Abajo / Stick Izq. |
| **Botón A** *(Saltar / Aletear en el aire)* | `Espacio`, `Z` o `J` | Botón `A` |
| **Botón B** *(Mighty Coin / Fuego)* | `X` o `K` | Botón `B` |
| **Start** *(Iniciar / Pausa)* | `Enter` o `Intro (NumPad)` | `Start` |
| **Select** | `Shift Derecho` o `Tab` | `Back` / `Select` |

### Ventana y Herramientas de Debug (Capa 3)

| Tecla | Función |
| :--- | :--- |
| `F11` | Alternar pantalla completa |
| `Esc` | Salir del juego |
| `F1` | Mostrar/ocultar overlay de telemetría e hitboxes 9-probe |
| `F2` | Abrir/cerrar selector de *Warp* a cualquier sala (`0`–`256`) |
| `[` / `]` | Cambiar sala destino en el selector de *Warp* |
| `F6` | Confirmar *Warp* a la sala seleccionada |
| `F3` | Ciclar cheats de depuración (Invencibilidad / Mighty) |
| `F4` | Pausar / reanudar simulación |
| `F5` | Avanzar 1 frame (con la simulación en pausa) |

---

## Cómo Ejecutar (Usando una GitHub Release)

### Windows (x64)
1. Descarga `mighty-bomb-jack-<version>-windows-x64.zip` desde la sección **Releases** de GitHub y descomprímelo. *(El paquete ya incluye `mjk.exe` y `SDL2.dll`.)*
2. Copia tu archivo `mbj-us.nes` (versión USA) en la misma carpeta que `mjk.exe`.
3. Ejecuta `mjk.exe` (o desde consola):
   ```powershell
   .\mjk.exe
   # O indicando una ruta personalizada para la ROM:
   .\mjk.exe --rom C:\ruta\a\mbj-us.nes
   ```

### Linux (x64)
1. Asegúrate de tener el runtime de SDL2 instalado:
   ```bash
   # Debian / Ubuntu
   sudo apt update && sudo apt install -y libsdl2-2.0-0
   ```
2. Descarga y descomprime `mighty-bomb-jack-<version>-linux-x64.tar.gz` desde **Releases**:
   ```bash
   tar -xzf mighty-bomb-jack-*-linux-x64.tar.gz
   cd mighty-bomb-jack-*-linux-x64
   ```
3. Copia tu archivo `mbj-us.nes` (versión USA) en esa misma carpeta y ejecuta:
   ```bash
   ./mjk
   # O indicando una ruta personalizada para la ROM:
   ./mjk --rom /ruta/a/mbj-us.nes
   ```

### Opciones de Línea de Comandos (`mjk`)
- `--rom <ruta>`: ruta explícita a `mbj-us.nes` para extraer `assets.dat`.
- `--assets <ruta>`: ruta personalizada al paquete `assets.dat`.
- `--headless`: ejecuta sin ventana ni audio (útil para pruebas automatizadas).
- `--smoke-test`: ejecuta 5 frames en modo headless y sale (verificación rápida).
- `--frames <N>`: detiene la ejecución tras `N` frames.

---

## Cómo Compilar desde el Código Fuente

### Windows (MSVC + CMake)
Requisitos: Visual Studio 2022 (o Build Tools con C/C++ y CMake). El script `build.bat` descarga automáticamente `SDL2 2.30.8 VC` en `third_party/sdl2/` si no está presente.

```bat
:: Compilar y ejecutar toda la suite de tests unitarios (CTest)
build.bat test

:: Compilar únicamente
build.bat build

:: Ejecutar (con mbj-us.nes en la raíz o junto al binario)
build.bat run
```

### Linux (GCC/Clang + CMake + SDL2)
Requisitos: `gcc` o `clang`, `cmake`, `make` (o `ninja`) y `libsdl2-dev`.

```bash
# Instalar dependencias en Debian / Ubuntu
sudo apt update && sudo apt install -y build-essential cmake ninja-build libsdl2-dev

# Compilar y ejecutar CTest
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Ejecutar el juego
./build/bin/mjk --rom /ruta/a/mbj-us.nes
```

También dispones del wrapper [`Makefile`](Makefile) (`make build`, `make test`, `make run`, `make clean`).

---

## Documentación de Ingeniería Inversa y Memoria de Agentes

Uno de los pilares educativos de este repositorio es conservar de forma transparente **todo el proceso de análisis, arquitectura y desarrollo asistido por agentes**:

### 1. Arquitectura Educativa
- [`docs/EDUCATIONAL_ARCHITECTURE.md`](docs/EDUCATIONAL_ARCHITECTURE.md) — Explicación didáctica de las 2 capas (núcleo C11 vs HAL SDL2), formato BYOR v2, aritmética `Q8.8`/`BCD`, reemplazo del hack de pila `PLA;PLA` y guía práctica de modding.

### 2. Investigación e Ingeniería Inversa (`research/`)
- [`research/INDEX.md`](research/INDEX.md) — Índice maestro de toda la documentación técnica verificada.
- [`research/GAME_COMPLETE_FLOW.md`](research/GAME_COMPLETE_FLOW.md) — Flujo completo del juego de principio a fin (Copyright, Title, 17 rondas, Torture Room, 4 finales y Game Over).
- [`research/CROSS_REFERENCE_INDEX.md`](research/CROSS_REFERENCE_INDEX.md) — Mapa bidireccional entre símbolos/subrutinas 6502 (`asm/src/`) y módulos C11 (`include/`, `src/core/`).
- [`research/contracts/`](research/contracts/) — Máquina de estados (`0..C`), matriz de transiciones y contratos formales de comportamiento.
- [`research/inventories/`](research/inventories/) y [`research/control_flow/`](research/control_flow/) — Mapa de RAM, inventarios de código/datos, pipeline NMI y modelo hardware PPU/APU.
- [`research/architecture/`](research/architecture/) — Especificación del extractor BYOR v2, arquitectura clean-slate y estrategia de verificación en 3 capas.
- **`research/RES-012` a `RES-036`** — Monografías profundas sobre sprites/OAM, APU 2A03, física Q8.8, colisiones 9-probe, puertas, scroll, bombas, cofres, IA enemiga y Torture Room.

### 3. Memoria Persistente y Flujo Agéntico (`project/` y `.agents/`)
- [`AGENTS.md`](AGENTS.md) — Constitución operativa del workspace (reglas de epistemología, verificación en 3 capas y clean-room BYOR).
- [`project/PROJECT_STATUS.md`](project/PROJECT_STATUS.md) — Estado consolidado del proyecto y sus pilares.
- [`project/TASKS.md`](project/TASKS.md) — Tablero maestro de las 30 micro-tareas (`T-01`..`T-30`, Hitos 1–7) y checkpoints.
- [`project/DECISIONS.md`](project/DECISIONS.md) — Registro completo de decisiones arquitectónicas (ADRs) tomadas durante todo el desarrollo.
- [`.agents/`](.agents/) — Skills especializadas (`mjk-asm-ground-truth`, `mjk-educational-c-port`, `mjk-verification-workflow`) y hook de protección de workspace.

---

## Estructura del Repositorio

```text
.
├── .github/workflows/   # Pipeline CI/CD de releases automáticas (Windows + Linux)
├── .agents/             # Skills y hooks del flujo agéntico
├── docs/                # Guía de arquitectura educativa y modding
├── include/             # Cabeceras C11 públicas del núcleo y HAL
├── project/             # Memoria persistente (estado, tablero de tareas y ADRs)
├── research/            # Documentación completa de ingeniería inversa (REV_US)
├── src/
│   ├── core/            # Núcleo de simulación C11 puro (0 dependencias SDL/SO)
│   ├── platform/        # HAL SDL2 (vídeo, audio, input) y Debug Overlay (Capa 3)
│   └── main.c           # Punto de entrada CLI y bucle NTSC 60.0988 Hz
├── tests/c/             # Suite de 18 tests unitarios C11 (CTest / Capa 1)
├── tools/               # Herramientas de consulta ASM y verificación Capa 2
├── CMakeLists.txt       # Sistema de compilación multiplataforma
├── build.bat / Makefile # Wrappers rápidos de build para Windows y Linux
├── LICENSE              # Licencia MIT + atribución del repositorio ASM
└── README.md            # Este documento
```

---

## Licencia y Atribuciones

- Este proyecto se distribuye bajo la **Licencia MIT** — consulta el archivo [`LICENSE`](LICENSE) para el texto completo.
- **Atribución del desensamblado 6502**: Basado en la investigación del repositorio **[`Xkeeper0/mighty-bomb-jack`](https://github.com/Xkeeper0/mighty-bomb-jack)** de **Xkeeper (y colaboradores)**, licenciado bajo la **Licencia MIT** (`Copyright (c) 2025 Xkeeper (and other contributors)`), cuyo aviso de copyright y licencia se conservan íntegramente en [`LICENSE`](LICENSE).
- **Aviso legal / Clean-Room**: *Mighty Bomb Jack* (1986/1987) es marca registrada y obra con derechos de autor de **Tecmo / Koei Tecmo Games Co., Ltd.** Este proyecto es un ejercicio educativo independiente, sin ánimo de lucro, no afiliado ni respaldado por Koei Tecmo ni Nintendo, y no distribuye ningún asset ni archivo protegido por copyright.
