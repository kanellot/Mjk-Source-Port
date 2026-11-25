# Assets BYOR, Renderizador Optimizado y Audio 2A03

## 1. Clean-Room BYOR v2 (`src/assets/`)

El repositorio no contiene ningún byte con copyright de Tecmo. Al iniciar el ejecutable `mjk`:
1. Busca `assets/assets.dat` (junto al directorio actual o del ejecutable, o vía `--assets <ruta>`).
2. Si no existe, busca `mbj-us.nes` (o `--rom <ruta>`), comprueba la cabecera iNES (`65.552` bytes, Mapper 3 CNROM) y su **SHA-1** (`a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`), y genera `assets/assets.dat` v2 protegido por **CRC-32**.
3. `src/assets/asset_bank.c` decodifica **una sola vez** todo el contenido hacia `MjkAssets`:
   - Los 512 tiles 2bpp planares de CHR Bank 1 se descomprimen en `uint8_t tiles[512][8][8]` (**32 KB**, cabe íntegro en caché L1).
   - Las tablas de salto y velocidad de enemigos se convierten a `float`.
   - Las tablas de puntuación BCD se convierten a `uint32_t`.
   - Las 256 salas comprimidas en 4 capas se decodifican a matrices `15×16` y listas de objetos tipados (`MjkRoomTemplate`).

---

## 2. Renderizador Solo-Lectura (`src/render/renderer.c`)

`MjkRenderer_DrawFrame(const MjkGame* game, const MjkAssets* assets, uint32_t* framebuffer)` dibuja los `256×240` píxeles ARGB8888 en dos pasos:
1. **Construcción de escena**: selecciona la paleta activa (por sala, Mighty `0..3` y Power Coin), prepara los dos planos de tiles (`current` + `neighbor`) y reúne la lista de sprites (Jack, HUD, popups, chispas, puffs, enemigos y textos).
2. **Rasterización rápida**:
   - Pre-resuelve `uint32_t bg_pal[4][4]` y `uint32_t spr_pal[4][4]` una sola vez por frame.
   - Lee directamente `assets->tiles[tile_id][row][col]` sin operaciones de desplazamiento de bits por píxel.
   - Respeta prioridad detrás/delante del fondo, volteos H/V y transparencia índice `0`, sin replicar el parpadeo artificial de 8 sprites por scanline.

---

## 3. Sintetizador 2A03 y Motor de Audio NTSC (`src/audio/`)

- **`synth_2a03.c`**: emula las 4 voces del chip NES 2A03 (Pulse 1/2 con 4 *duty cycles* y *hardware sweep*, Triangle de 32 pasos y Noise LFSR de 15 bits) a `44.1 kHz`.
  - **Optimización LUT**: en `MjkSynth_Init` precalcula `float pulse_lut[31]` y `float tnd_lut[16][16]` con la fórmula exacta no lineal NTSC 2A03, eliminando todas las divisiones en coma flotante del bucle de 44.100 muestras/segundo.
  - Aplica un filtro pasa-altos DC (`0.996`) y saturación segura a `int16_t`.
- **`audio_engine.c`**: interpreta las pistas musicales y SFX en 8 canales virtuales (4 BGM + 4 SFX). Un SFX activo inhibe la salida hardware de su canal BGM hermano sin detener su reloj y restaura sus registros al finalizar.
- **Sincronía NTSC exacta**: `MjkAudio_NextFrameSampleCount()` usa un acumulador fraccional (`44100.0 / 60.0988138974405 ≈ 733.7915`) que alterna entre **733 y 734 muestras por frame**, evitando el superávit de ~73 muestras/s que causaba descartes en la cola de audio de SDL2.
