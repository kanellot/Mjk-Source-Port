# Guía de Compilación y Testing (`CMake` + `CTest`)

La integridad funcional del motor se valida **exclusivamente mediante 11 suites de unit tests en C11 (`tests/c/`)**, sin código de test, debug ni verificación dentro del ejecutable `mjk`.

---

## 1. Compilación y Ejecución

### Windows (MSVC + Ninja/CMake)
```cmd
build.bat test
build\bin\mjk.exe --rom ruta\a\mbj-us.nes
```

### Linux y macOS (GCC / Clang + SDL2)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/bin/mjk --rom ruta/a/mbj-us.nes
```
En macOS puedes instalar dependencias con `brew install cmake sdl2` y usar igualmente `make build`, `make test` y `make run`.

### Con AddressSanitizer y UndefinedBehaviorSanitizer (GCC / Clang)
```bash
cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DMJK_ENABLE_SANITIZERS=ON
cmake --build build-san
ctest --test-dir build-san --output-on-failure
```

---

## 2. Resolución de ROM y `SKIP_RETURN_CODE 77`

Los tests que requieren `mbj-us.nes` resuelven la ruta en este orden:
1. Variable de entorno `MJK_ROM_PATH` *(explícita)*.
2. Opción de CMake `-DMJK_TEST_ROM_PATH=/ruta/mbj-us.nes` *(explícita)*.
3. `<raíz_del_repo>/asm/bin/mbj-us.nes` *(implícita local)*.

Comportamiento honesto en `CTest`:
- Si **no** hay ROM local ni explícita → el test devuelve **`77`** y `CTest` lo marca como **`Skipped`** (nunca como falso `Passed`).
- Si el usuario proporcionó una ruta explícita y falta o es inválida → el test devuelve **`1`** (`Failed`).

---

## 3. Cobertura de las 11 Suites (`tests/c/`)

| Suite | Qué verifica |
| :--- | :--- |
| `test_input` | `MjkInputLatch`: retención de flancos en iteraciones de 0 pasos, consumo único en catch-up de 2 pasos y pulsaciones rápidas. |
| `test_assets` | SHA-1 (RFC 3174 + ROM), CRC-32, round-trip `assets.dat` v2, detección de corrupción, 512 tiles pre-decodificados y **256 salas** contra `golden_reference.h`. |
| `test_scoring` | Tabla de puntos, multiplicadores `×1..×5`, Bonus/Extra Coin cada 5.000 pts, saturación, GDV/High GDV, `StageTimer` (256f exactos) y popups. |
| `test_world` | Salas H/V/Single, scroll continuo, cruce de costura, persistencia de sección, metatiles parciales `8×8`, 7 tiles de fuego y cofres visibles/ocultos. |
| `test_player` | **5 Golden Trajectories** frame a frame (`float` vs motor original), flutter por flanco, colisiones, secuencia de muerte y persistencia de `jump_press_count`. |
| `test_objects` | Bomb Room (23 bombas, chispa, puff 2 fases, puerta de salida), orden perfecto (`10.200` pts), cofres rojos/naranjas, secretos, Mighty `0..3` y Torture por 10 Mighty Coins. |
| `test_doors` | Casos falso y verdadero de las **33 condiciones (`0..32`)**, Sala `$61`, apertura persistente y cruce de Jack. |
| `test_enemies` | Mutación determinista `(jump_press_count % 6) + 2` a los **6 arquetipos**, Power Coin (`320f`, cadena de puntos, restauración), coleccionables y daño letal. |
| `test_renderer` | **Golden Frame Checks** (hashes exactos de Copyright y Title), Sala 1, Mighty 3, scroll, Bomb Room, Torture Room y opacidad ARGB8888. |
| `test_audio` | Equivalencia exacta de LUTs 2A03 (`< 1e-6`), **acumulador NTSC (`733`/`734`, deriva ≤ 1 muestra en 3.600 frames)**, BGM, prioridad/restauración SFX→BGM y silencio. |
| `test_game_flow` | Flujo completo: Copyright → Title → Falling → Round Intro → Sala 1 → Pausa → Torture (`50` saltos US) → Round Clear → **4 Finales (`$A0..$A3`)** → Game Over → Title. |
