# Mighty Bomb Jack — Source Port Educativo en C11

Source port multiplataforma (**Windows**, **Linux** y **macOS**) en **C11 estándar + SDL2** de *Mighty Bomb Jack* (NES USA 1987, `REV_US`), diseñado con código **limpio, moderno, optimizado y educativo**, bajo arquitectura **Clean-Room BYOR** (*Bring Your Own ROM*).

> **Proyecto educativo basado en ingeniería inversa 6502**  
> Este port y su documentación técnica se han construido estudiando el desensamblado 6502 de referencia del repositorio **[`Xkeeper0/mighty-bomb-jack`](https://github.com/Xkeeper0/mighty-bomb-jack)** (autor: **Xkeeper y colaboradores**, bajo licencia **MIT**). El código C11 está completamente libre de convenciones arcaicas del 6502 (usa enteros estándar, física en `float` exacto, estructuras semánticas, renderizador solo-lectura y síntesis 2A03 con tablas LUT).

---

## Características Principales

- **Juego completo (`REV_US`)**: desde Copyright y Title Screen hasta las 17 rondas (256 salas), Bomb Rooms, Torture Room (50 saltos US), los 4 finales y Game Over.
- **Arquitectura limpia en 5 capas**:
  - `src/assets/`: única frontera que lee `mbj-us.nes` / `assets.dat` y pre-decodifica los 512 tiles `8×8`, paletas, tablas de física `float`, puntuaciones `uint32_t` y las 256 salas una sola vez al arrancar.
  - `src/game/`: lógica pura C11 (cero SDL2, cero ROM, cero render, cero código de debug/test).
  - `src/render/`: renderizador solo-lectura `256×240` ARGB8888 optimizado con tiles en caché L1 y caché plana de paletas RGB.
  - `src/audio/`: sintetizador 2A03 con tablas LUT (`pulse_lut`/`tnd_lut`), secuenciador de 8 canales (4 BGM + 4 SFX) y acumulador fraccional NTSC (`733`/`734` muestras/frame a 44.1 kHz).
  - `src/platform/`: capa mínima SDL2 con `MjkInputLatch` para consumo exacto de flancos en bucle de paso fijo (`60.0988 Hz`).
- **Clean-Room BYOR integrado en C**: **cero bytes con copyright de Tecmo** en el repositorio ni en el ejecutable.
- **Suite exhaustiva de unit tests (`tests/c/`)**: 11 suites `CTest` con `SKIP_RETURN_CODE 77` cuando falta la ROM local y soporte opcional de ASan/UBSan (`-DMJK_ENABLE_SANITIZERS=ON`).

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

Al arrancar, el ejecutable busca automáticamente `assets/assets.dat`. Si no existe, busca `mbj-us.nes` en el directorio actual, junto al ejecutable o en `--rom <ruta>`, verifica su SHA-1 y extrae `assets/assets.dat` (con verificación CRC-32).

---

## Controles (Teclado y Mando)

El port soporta **teclado** y **mandos USB/Bluetooth** (hotplug vía SDL2 GameController).

| Acción (NES) | Teclado | Mando (Gamepad) |
| :--- | :--- | :--- |
| **Izquierda / Derecha** | `←` / `→` o `A` / `D` | D-Pad / Stick Izq. |
| **Arriba / Abajo** | `↑` / `↓` o `W` / `S` | D-Pad / Stick Izq. |
| **Botón A** *(Saltar / Flutter)* | `Espacio`, `Z` o `J` | Botón `A` |
| **Botón B** *(Mighty Coin)* | `X` o `K` | Botón `B` o `X` |
| **Start** *(Iniciar / Pausa)* | `Enter` o `Intro (NumPad)` | `Start` |
| **Select** | `Shift Derecho` o `Tab` | `Back` / `Select` |
| **Pantalla completa / Salir** | `F11` / `Esc` | — |

### Opciones CLI (`mjk`)
- `--rom <ruta>`: ruta explícita a `mbj-us.nes`.
- `--assets <ruta>`: ruta personalizada a `assets.dat`.

---

## Cómo Compilar y Ejecutar los Tests

### Windows (MSVC + CMake)
```bat
build.bat test
build\bin\mjk.exe --rom ruta\a\mbj-us.nes
```

### Linux (GCC / Clang + CMake + SDL2)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/bin/mjk --rom /ruta/a/mbj-us.nes
```

También dispones del wrapper [`Makefile`](Makefile) (`make build`, `make test`, `make run`, `make clean`).

### macOS (Apple Clang + CMake + SDL2)

Usa el mismo `Makefile` que en Linux. Si aún no tienes las herramientas de línea de comandos de Xcode, instálalas con `xcode-select --install`. Instala CMake y SDL2 con Homebrew si te faltan.

```bash
brew install cmake sdl2
make build
make test

# Con mbj-us.nes en la raíz del repositorio
make run

# O con la ROM en otra carpeta
./build/bin/mjk --rom "/ruta/a/mbj-us.nes"
```

---

## Documentación Técnica

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Arquitectura por capas, flujo de 1 frame y mapa de archivos.
- [`docs/GAME_SYSTEMS.md`](docs/GAME_SYSTEMS.md) — Explicación de cada subsistema de gameplay (`src/game/`).
- [`docs/ASSETS_AND_AUDIO.md`](docs/ASSETS_AND_AUDIO.md) — Clean-Room BYOR, renderizador con tiles en L1 y síntesis 2A03 con LUTs.
- [`docs/TESTING_GUIDE.md`](docs/TESTING_GUIDE.md) — Guía de compilación, `CTest`, `SKIP_RETURN_CODE 77` y sanitizers.
- [`docs/research/INDEX.md`](docs/research/INDEX.md) — Inventarios y referencias técnicas del desensamblado 6502 original.
- [`docs/project/PROJECT_STATUS.md`](docs/project/PROJECT_STATUS.md) — Estado del proyecto, decisiones de arquitectura (`DECISIONS.md`) e historial de tareas (`TASKS.md`).

---

## Estructura del Repositorio

```text
.
├── docs/                # Documentación técnica, referencias ASM (research/) y estado (project/)
├── include/mjk/         # Cabeceras C11 públicas del motor y plataforma
├── src/
│   ├── assets/          # SHA-1, extractor BYOR v2 y decodificador único
│   ├── game/            # Lógica pura del juego (0 SDL, 0 ROM, 0 render)
│   ├── render/          # Renderizador solo-lectura 256x240 optimizado
│   ├── audio/           # Sintetizador 2A03 (LUTs) y motor NTSC 8 canales
│   ├── platform/        # Capa mínima SDL2 (vídeo, audio, input)
│   └── main.c           # Punto de entrada CLI y bucle 60.0988 Hz
├── tests/c/             # 11 suites exhaustivas de unit tests C11 (CTest)
├── tools/               # Herramienta de consulta ASM (asm_query.py)
├── CMakeLists.txt       # Sistema de compilación multiplataforma
├── build.bat / Makefile # Wrappers de compilación para Windows, Linux y macOS
└── LICENSE              # Licencia MIT + atribución del repositorio ASM
```

---

## Licencia y Atribuciones

- Este proyecto se distribuye bajo la **Licencia MIT** — consulta [`LICENSE`](LICENSE) para el texto completo.
- **Atribución del desensamblado 6502**: Basado en **[`Xkeeper0/mighty-bomb-jack`](https://github.com/Xkeeper0/mighty-bomb-jack)** de **Xkeeper (y colaboradores)**, bajo **Licencia MIT** (`Copyright (c) 2025 Xkeeper (and other contributors)`).
- **Aviso legal / Clean-Room**: *Mighty Bomb Jack* (1986/1987) es marca registrada y obra con derechos de autor de **Tecmo / Koei Tecmo Games Co., Ltd.** Este proyecto es un ejercicio educativo independiente, sin ánimo de lucro, no afiliado ni respaldado por Koei Tecmo ni Nintendo, y no distribuye ningún asset ni archivo protegido por copyright.
