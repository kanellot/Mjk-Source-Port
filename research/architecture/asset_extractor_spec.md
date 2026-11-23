# Especificación Técnica del Extractor de Assets Clean-Room (BYOR)

- **ID de Entregable**: `research/architecture/asset_extractor_spec.md`
- **Fase**: Fase 5 — Diseño de Arquitectura del Source Port
- **Regla de Referencia**: `ADR-006` en [`project/DECISIONS.md`](../../project/DECISIONS.md)
- **Estrategia**: *Bring Your Own ROM* (BYOR) — Cero bytes protegidos por derechos de autor en el repositorio de código abierto.

---

## 1. Justificación y Fundamento Legal

Para preservar legalmente el proyecto y permitir su publicación abierta:
1. El repositorio de código fuente **no distribuirá ningún archivo multimedia** (ni gráficos CHR, ni pistas de audio, ni tipografías de Tecmo).
2. El usuario final debe suministrar su propia copia legal del cartucho NES americano (`mbj-us.nes`).
3. El extractor valida la autenticidad e integridad del cartucho mediante funciones hash criptográficas y extrae los assets empaquetándolos en un formato moderno y estructurado (`assets/assets.dat`).

---

## 2. Perfil Criptográfico de la ROM Canónica (`REV_US`)

El extractor aceptará exclusivamente la ROM oficial americana de *Mighty Bomb Jack* (NES USA 1987). Cualquier archivo con hash diferente será rechazado para evitar desajustes en el motor de juego:

| Parámetro | Valor Verificado y Esperado |
| :--- | :--- |
| **Nombre Canónico** | `mbj-us.nes` |
| **Tamaño Físico** | **65.552 bytes** ($0x10010$) |
| **Cabecera iNES** | 16 bytes (`$0000` a `$000F`) |
| **PRG-ROM (Código y Datos)** | 32.768 bytes (`$0010` a `$800F`, $8000-$FFFF mapeado) |
| **CHR-ROM (Gráficos PPU)** | 32.768 bytes (`$8010` a `$1000F`, 4 bancos de 8 KB) |
| **Hash SHA-1** | `a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7` |
| **Hash MD5** | `56802d6302efef4ebfad0a7270c8914c` |
| **Checksum CRC-32** | `73DD920F` |

---

## 3. Mapa de Extracción de Assets

```
OFFSET FÍSICO EN ARCHIVO .NES
0x000000 +-------------------------------------------------------+
         |  Cabecera iNES (16 bytes: 'N','E','S',0x1A,...)        |
0x000010 +-------------------------------------------------------+
         |  PRG-ROM (32.768 bytes)                               |
         |  - Lógica del juego (subrutinas)                      |
         |  - Tablas de metatiles y salas (libres de copyright)  |
         |  * TABLAS DE AUDIO Y SFX -> [EXTRAER: Bytecodes audio]|
         |  * TEXTOS DE PANTALLA    -> [EXTRAER: Textos y fuentes]|
0x008010 +-------------------------------------------------------+
         |  CHR-ROM (32.768 bytes)                               |
         |  * BANCO CHR 0 (8 KB): Título y cinemáticas           |
         |  * BANCO CHR 1 (8 KB): Sprites de Jack y enemigos     |
         |  * BANCO CHR 2 (8 KB): Tiles de fondo (pirámides)     |
         |  * BANCO CHR 3 (8 KB): Tiles de salas y tortura       |
         |  -> [EXTRAER AL 100%: 32.768 bytes de tiles 2bpp]     |
0x010010 +-------------------------------------------------------+
```

### 3.1 Extracción de Gráficos (CHR-ROM de 32 KB)
- **Offset en Archivo**: `0x8010` a `0x1000F` (exactamente 32.768 bytes).
- **Formato Nativo NES**: Planar 2bpp (2 bits por píxel). Cada tile de $8 \times 8$ píxeles ocupa 16 bytes:
  - Primeros 8 bytes: Bit 0 de cada fila de 8 píxeles.
  - Siguientes 8 bytes: Bit 1 de cada fila de 8 píxeles.
- **Conversión en el Extractor**:
  El extractor puede almacenar los tiles en su formato nativo 2bpp o decodificarlos opcionalmente en una textura atlas RGBA32 para que la GPU moderna no requiera decodificación en tiempo de ejecución.

### 3.2 Extracción de Secuencias Musicales y SFX
- **Origen en PRG-ROM**:
  - Punteros de pistas musicales: `prg.asm:8800` / `music-engine.asm`.
  - Tablas de notas, comandos de modulación de pitch y envolventes de volumen.
- **Uso en el Port**:
  El sintetizador en C (`mjk_apu.c`) interpreta directamente estas secuencias de comandos para generar las formas de onda en tiempo real, garantizando la mezcla dinámica original.

### 3.3 Extracción de Textos de Interfaz
- Textos de pantalla de título: *"PUSH START BUTTON"*, *"GAME OVER"*, *"TIME OVER"*.
- Mensajes de bonificación: *"BONUS COIN GET"*, *"SECRET COIN"*.
- Diálogos de los 4 finales cinemáticos (rescate del Rey Pamer, la Reina y la Princesa).

---

## 4. Especificación del Paquete Binario `assets.dat`

El extractor genera un único archivo compacto `assets/assets.dat` con una cabecera binaria estructurada:

```c
#pragma pack(push, 1)

typedef struct {
    char     magic[8];       // "MJKASSET"
    uint32_t version;        // 0x00010000 (Versión 1.0)
    uint32_t total_size;     // Tamaño total del archivo en bytes
    uint32_t crc32;          // Checksum de los datos contenidos
    
    // Desplazamientos a cada sección
    uint32_t chr_offset;     // Offset a los 32 KB de CHR-ROM
    uint32_t chr_size;       // 32768 bytes
    
    uint32_t audio_offset;   // Offset a las secuencias de música y SFX
    uint32_t audio_size;     // Tamaño en bytes
    
    uint32_t text_offset;    // Offset a la tabla de textos
    uint32_t text_size;      // Tamaño en bytes
} MjkAssetHeader;

#pragma pack(pop)
```

---

## 5. Especificación del Extractor Nativo Integrado en C11 (`mjk_rom_extractor.c` + `mjk_sha1.c`)

El port **no depende de Python ni de scripts externos** para el usuario final: el propio binario `mjk` realiza la validación y extracción en el arranque si aún no existe un paquete válido.

### 5.1 Flujo Automático al Arrancar `mjk`
1. Comprueba si existe `assets/assets.dat` (cabecera `"MJKASSET"`, versión `0x00020000`, CRC-32 válido).
   - Si es válido -> lo carga directamente en memoria y continúa al bucle principal.
2. Si no existe o es obsoleto, busca `mbj-us.nes` en orden:
   - Argumento CLI `--rom <ruta_a_mbj-us.nes>`.
   - `./mbj-us.nes` (directorio de trabajo actual).
   - `<exe_dir>/mbj-us.nes` (junto al ejecutable, resuelto vía `SDL_GetBasePath()`).
3. Valida tamaño exacto (**65.552 bytes**) y hash **SHA-1** (`a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7`) mediante `mjk_sha1.c` puro C11.
   - Si la ROM no coincide, informa con el SHA-1 esperado y obtenido y aborta limpiamente.
4. Extrae los bloques desde la ROM verificada, construye `assets/assets.dat` v2 con CRC-32, lo carga en memoria y arranca el juego de forma transparente.
5. Si no se encuentra ni `assets.dat` ni `mbj-us.nes`, muestra un mensaje claro en consola y un cuadro de diálogo SDL2 (`SDL_ShowSimpleMessageBox`) indicando al usuario que coloque su copia legal de `mbj-us.nes` junto al ejecutable o use `--rom <ruta>`.

---

## 6. Evolución Clean-Room: BYOR v2 (Cero Copyright en C)

Para cumplir al 100% la política Clean-Room (cero datos con copyright de Tecmo en el repositorio ni hardcodeados en C):
1. **`assets.dat` v2 (`0x00020000`)**: Extrae directamente desde `mbj-us.nes` (SHA-1 verificado):
   - `CHR-ROM` (32 KB completos, 4 bancos de 8 KB).
   - Tablas y bytecodes de audio (`music-data.asm`: tracks BGM, SFX, envolventes, tabla de frecuencias/duraciones).
   - Cadenas de texto e índices PPU (`$C244..$C500`) y uso exclusivo de tiles CHR-ROM para HUD/texto (sin fuentes embebidas).
   - Tablas de geometría y salas (`RoomHalfScreens`, `HalfScreenLayouts`, `LayoutChunks`, `MetatileDefinitions`, `AdjacentRoomsTable`, `RoomDataPointers` + streams `RoomData_01..FF`, `MainPalette`, `BackgroundPaletteSets`, `TileAttributeTable`, tablas de puertas e ítems).
   - Tablas de animación y spawns (`SpriteAnimationTable`, `EnemySpawnPositionTable`, `DifficultyTable`).
2. **Carga dinámica en `MjkAssets_Load()`**: Todo puntero o tabla propietaria en `src/core/` apunta al buffer cargado desde `assets.dat` v2, garantizando que sin `mbj-us.nes` el binario compilado no contenga ningún asset de Tecmo.
