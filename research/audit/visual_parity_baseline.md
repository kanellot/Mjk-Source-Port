# Línea Base de Paridad Visual y Auditoría de Fidelidad Gráfica: Mighty Bomb Jack (NES 1987)

- **Documento**: `research/audit/visual_parity_baseline.md`
- **Área Temática**: 04 - Auditoría de Calidad y Fidelidad Visual (Fase 4 & Fase 6)
- **Estado**: `[CONFIRMADA]`
- **Fecha**: 2026-10-24
- **Fuentes de Verdad Absoluta**:
  - Vídeo de Referencia Original: `Mighty-Bomb-Jack-original.mp4` (Fotogramas en `scratch/frames/`)
  - Desmontaje 6502 Golden Ground Truth:
    - [`asm/src/prg.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm) (`TitleScreenHandler` L7473-7660, `GameState_1_RoundIntro` L7663-7700, `DrawScoreAndCoinCount` L1783-1935, `WriteSprite` L2267-2300, `LoadRoomPalette` L2389-2425)
    - [`asm/src/data.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm) (`MainPalette` L258-268, `BackgroundPaletteSets` L269-305, `DoorEntryXYPositionTable` L322-336, `StringPointerTable` L412-586)
    - [`asm/src/roomdata-us.asm`](file:///c:/Users/kanel/Documents/mjk/asm/src/roomdata-us.asm) (`RoomData_01` L4-5)
  - Artefactos de Port Defectuoso: `scratch/port_frames/`

---

## 1. Resumen Ejecutivo y Marco Metodológico

Este informe establece la **Línea Base Visual Cuantitativa y Matemática** para certificar y guiar la reimplementación del source port en C99/SDL2 de *Mighty Bomb Jack*. Define con precisión de píxel (en la resolución nativa NES de $256 \times 240$), paleta de hardware PPU 2C02 y registros OAM, la totalidad de los elementos gráficos del juego original frente al estado defectuoso del prototipo preliminar.

### Cumplimiento Estricto de la Directiva de Usuario:
> *"Hay que poner atención en el mapeado de sprites pero también en los colores que sean igual que en el original. No se debe aplicar una corrección simplemente por la mera comparación: hay que entender cómo reproducir eso según el código original."*

Cada elemento visual analizado no es simplemente una medida fotométrica aislada, sino que está **estrictamente correlacionado con la subrutina del 6502, el índice de paleta ROM y la tabla de Nametables/OAM que lo genera en el cartucho de 1987**.

---

## 2. Paleta Maestra NES 2C02 y Correspondencia Cromática

La PPU Ricoh 2C02 del NES genera vídeo NTSC mediante modulación de fase a partir de un espacio de 64 colores lógicos (`$00..$3F`). Mighty Bomb Jack carga sus paletas dinámicamente en `$3F00..$3F1F` (16 bytes para fondos y 16 bytes para sprites).

### 2.1 Tabla de Clasificación: Medición de Vídeo vs Código ROM 6502

| Índice 2C02 | Nombre del Tono | RGB Vídeo Medido | RGB Estándar 2C02 | Hex Estándar | Función en ROM / Línea Ensamblador |
| :---: | :--- | :---: | :---: | :---: | :--- |
| **`$08`** | Marrón / Ocre Profundo | `(51, 53, 0)` | `(51, 53, 0)` | `#333500` | Sombra/mortero pirámide (`MainPalette` L260), base de sprites |
| **`$0A`** | Verde Bosque Profundo | `(0, 80, 0)` | `(0, 82, 0)` | `#005200` | **Color Universal de Fondo Sala 1** (`BackgroundPaletteSets` Set 0 L270); vegetación oasis |
| **`$0F` / `$0D`**| Negro Absoluto | `(0, 0, 0)` | `(0, 0, 0)` | `#000000` | Fondo pantalla Round Intro (`$0F`), contorno metatiles de piedra (`Set 0 Pal 1` L271) |
| **`$10`** | Gris Piedra Medio | `(160, 160, 160)`| `(173, 173, 173)` | `#ADADAD` | Bloques de piedra redondeados de Sala 1 (`Set 0 Pal 1` L271) |
| **`$12`** | Azul Mighty / Capucha | `(66, 64, 255)` | `(66, 64, 255)` | `#4240FF` | Capucha azul de Jack (`MainPalette` Sprite Pal 1 L265) |
| **`$16`** | Rojo Carmesí Tecmo | `(181, 49, 32)` | `(181, 49, 32)` | `#B53120` | Traje de Jack, bombas esféricas, extrusión 3D logotipo, **dígitos y letras del HUD** (`MainPalette` Sprite Pal 0 L264) |
| **`$1A`** | Verde Jungla Medio | `(12, 147, 0)` | `(12, 147, 0)` | `#0C9300` | Hojas de palmera y oasis (`MainPalette` BG Pal 2 L261) |
| **`$21`** | Azul Cielo / Cyan | `(41, 174, 248)` | `(100, 176, 255)`| `#64B0FF` | **Color Universal de Fondo Pantalla Título y Copyright** (`MainPalette` L259) |
| **`$26`** | Naranja Salmón Vivo | `(254, 129, 112)`| `(254, 129, 112)`| `#FE8170` | Antorcha pilar izquierdo, reflejo de logotipo 3D (`MainPalette` L259) |
| **`$27`** | Ocre Dorado Pirámide | `(234, 158, 52)` | `(234, 158, 52)` | `#EA9E34` | Gradas iluminadas de la Gran Pirámide (`MainPalette` L260) |
| **`$2B`** | Verde Claro | `(69, 224, 130)` | `(69, 224, 130)` | `#45E082` | Sprites especiales y cofres verdes (`MainPalette` Sprite Pal 3 L267) |
| **`$30`** | Blanco Puro | `(255, 255, 255)`| `(255, 255, 255)`| `#FFFFFF` | Cara de Jack, alas voladoras, brillo de bombas, textos Nametable, números HUD (`MainPalette` L259) |
| **`$38`** | Amarillo Dorado Brillante| `(228, 229, 148)`| `(228, 229, 148)`| `#E4E594` | Cúspide de la pirámide, chispas de mechas de bombas, llamas (`MainPalette` L260) |
| **`$39`** | Verde Menta Claro | `(207, 239, 150)`| `(207, 239, 150)`| `#CFEF96` | Destellos superiores de hojas de palmeras (`MainPalette` L261) |

---

## 3. Especificaciones Visuales de Pantallas Originales (256x240)

### 3.1 Pantalla de Copyright USA (`frame_1s.png` - `TitleScreen_US_0`)
- **Fondo**: Color universal `$21` (Azul Cielo `#64B0FF`), llenando los $256 \times 240$ píxeles.
- **Tipografía y Color**: Nametable 0 (`$2000`), fuente monocroma blanca (Color `$30`).
- **Coordenadas de Textos y Sprites Embebidos**:
  1. `MIGHTY BOMB JACK (TM)`:
     - Nametable PPU: `$2106` $\rightarrow$ Fila 8, Columna 6.
     - Caja Envolvente: $X = 48$, $Y = 64$, Ancho = 160 px (20 caracteres $\times 8$), Alto = 8 px.
     - Fuente 6502: `String_MightyBombJackTM` ([`asm/src/data.asm:570`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L570)).
  2. `TM AND (C) 1987 TECMO,LTD.`:
     - Nametable PPU: `$2184` $\rightarrow$ Fila 12, Columna 4.
     - Texto de Fondo: $X = 32$, $Y = 96$, Ancho = 208 px, Alto = 8 px.
     - **Sprites BCD para el año "1987"**: Insertados en el hueco mediante OAM:
       - Sprites decenas/unidades "87": $X = \$84$ (132), $Y = \$63$ (99) vía `WriteBCDDigitsSprites` ([`asm/src/prg.asm:7507-7512`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L7507-L7512)).
       - Sprites millar/centena "19": $X = \$74$ (116), $Y = \$19$ (25 en Temp, offset OAM 99) vía `WriteBCDDigitsSprites` ([`asm/src/prg.asm:7513-7516`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L7513-L7516)).
  3. `LICENSED BY`:
     - Nametable PPU: `$21CA` $\rightarrow$ Fila 14, Columna 10.
     - Caja Envolvente: $X = 80$, $Y = 112$, Ancho = 88 px (11 caracteres $\times 8$), Alto = 8 px.
     - Fuente 6502: `String_LicensedBy` ([`asm/src/data.asm:578`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L578)).
  4. `NINTENDO OF AMERICA INC.`:
     - Nametable PPU: `$2204` $\rightarrow$ Fila 16, Columna 4.
     - Caja Envolvente: $X = 32$, $Y = 128$, Ancho = 192 px (24 caracteres $\times 8$), Alto = 8 px.
     - Fuente 6502: `String_NintendoOfAmerica` ([`asm/src/data.asm:582`](file:///c:/Users/kanel/Documents/mjk/asm/src/data.asm#L582)).

---

### 3.2 Pantalla de Título Auténtica (`frame_6s.png` - `TitleScreen_0..3`)
- **Fondo Nametable**: Pirámide, oasis, palmeras y cielo renderizados mediante decodificación de la Sala 0 (`CurrentRoomID = 0`, `DrawFullScreen` en [`asm/src/prg.asm:7582-7585`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L7582-L7585)).
- **Logotipo 3D "MIGHTY BOMB JACK"**:
  - Caja Envolvente: $X = [17 .. 247]$ ($W = 231\text{ px}$), $Y = [26 .. 103]$ ($H = 78\text{ px}$).
  - Composición Gráfica:
    - Carátula frontal: Blanco puro `$30` con biseles amarillos `$38`.
    - Extrusión y sombreado isométrico 3D hacia abajo-derecha: Rojo Tecmo `$16` y Naranja vivo `$26`.
    - Símbolo "(TM)": PPU `$219E` (Fila 12, Col 30, $X = 240$, $Y = 96$).
- **La Gran Pirámide Dorada y Oasis**:
  - Cúspide de Pirámide: $X = 128$, $Y = 112$.
  - Pendiente y Base: Desciende simétricamente hasta los laterales $X = [0 .. 255]$ en $Y = 240$.
  - Colores de gradas: Amarillo `$38`, Ocre `$27` y sombra `$08`.
  - Oasis y Palmeras (parte inferior izquierda y centro): $Y = [190 .. 238]$, colores verde `$0A`, `$1A` y `$39`.
- **Jack Volador con Alas (Animación de Título)**:
  - Generación: `InitPlayerStruct` + asignación de coordenadas en [`asm/src/prg.asm:7595-7601`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L7595-L7601):
    - `PlayerXPosHi = $28` ($X = 40$)
    - `PlayerYPosHi = $78` ($Y = 120$)
  - Dimensiones del Sprite: $16 \times 16\text{ px}$ más $8\text{ px}$ de alas batientes a ambos costados.
  - Colores: Traje rojo `$16`, capucha azul `$12`, cara y alas blancas `$30`.
  - Física en Start: `TitleScreen_2` asigna `PlayerXVelLo = $80`, `PlayerSprite = 3`, cayendo parabólicamente hasta $Y \ge \$D8$ (216) para entrar al juego.
- **Marcador Superior "HI GDV 0"**:
  - Nametable PPU: `$2056` $\rightarrow$ Fila 2, Columna 22 ($X = 176$, $Y = 16$, $W = 48$, $H = 8$).
  - Dígito Sprite BCD: `TempSpriteX = $CC` (204), `TempSpriteY = $1C` (28, en pantalla scanline 24).
- **Textos Interactivos y Legales**:
  - `PUSH START BUTTON`: Nametable PPU `$21C8` $\rightarrow$ Fila 14, Columna 8 ($X = 64$, $Y = 112$, $W = 136\text{ px}$, $H = 8\text{ px}$), color blanco `$30`.
  - `(C) 1987 TECMO,LTD.`: Nametable PPU `$2213` $\rightarrow$ Fila 16, Columna 19 ($X = 152$, $Y = 128$) con sprites BCD "1987" en $X = \$BC$ (188) y $X = \$CC$ (204), $Y = \$93$ (147).

---

### 3.3 Pantalla de Round Intro (`GameState_1_RoundIntro`)
- Código: [`asm/src/prg.asm:7663-7700`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L7663-L7700).
- **Fondo**: Completamente negro (`ClearNametable`, color `$0F`).
- **Sprites OAM Centrados** (Fila scanline $Y = \$60 = 96$):
  1. Sprite de Jack (facing right): $X = \$70$ (112), $Y = \$60$ (96), $16 \times 16\text{ px}$.
  2. Sprite "x" (multiplicador): $X = \$84$ (132), $Y = \$60$ (96), Tile `$0C`, $8 \times 8\text{ px}$.
  3. Contador BCD de Vidas: $X = \$94$ (148), $Y = \$60$ (96), 2 dígitos BCD (ej. `" 3"`).
- **Temporizador de Espera**: Exactamente 96 frames (`GameState1WaitTimer = $60`, ~1.6 segundos a 60 Hz).

---

### 3.4 Pantalla de Juego de Sala 1 (`frame_15s.png` y `frame_25s.png`)

#### A. Composición Exacta del HUD Superior (Scanlines 0 a 32)
En Mighty Bomb Jack, **el HUD no se dibuja en la Nametable sino mediante sprites OAM** para permitir el scroll y cambio dinámico sin alterar el fondo ([`asm/src/prg.asm:1780-1935`](file:///c:/Users/kanel/Documents/mjk/asm/src/prg.asm#L1780-L1935)):

1. **Marcador de Puntuación (`DrawScore` L1801)**:
   - Coordenadas OAM: $Y = \$10$ (Scanline 16), $X$ inicial = $\$58$ (88).
   - Formato: 8 dígitos BCD (`PlayerScore` en `$0428-$042B`).
   - Paso horizontal: $+8\text{ px}$ por dígito $\rightarrow X \in [88, 96, 104, 112, 120, 128, 136, 144]$ (Ancho total = 64 px).
   - Supresión de ceros (`DrawScoreDigitMaybe` L1829): Los ceros a la izquierda del primer dígito significativo se ocultan enviándolos a $Y = \$F8$ (248, fuera de pantalla).
   - Colores: Sprite Palette 0 (Rojo Tecmo `$16` y Blanco `$30`).
2. **Temporizador de Fase (`DrawTimerOrTortureJumps` L1871)**:
   - Icono `"T"`: Tile `$0E`, Atributo `$10`, $X = \$3C - 4 = 56$, $Y = \$1C - 4 = 24$.
   - Dígitos BCD de Tiempo (`StageTimer`, 60 inicial):
     - Dígito de Decenas ("6"): $X = 64$, $Y = 24$.
     - Dígito de Unidades ("0"): $X = 72$, $Y = 24$.
   - Colores: Rojo Tecmo `$16` y Blanco `$30`.
3. **Contador de Monedas Mighty (`DrawMightyCoinCountOrMultiplier` L1903)**:
   - Icono `"M"`: Tile `$0B`, Atributo `$10`, $X = \$BC - 4 = 184$, $Y = \$1C - 4 = 24$.
   - Dígito de Cantidad ("0"): $X = \$C4 - 4 = 192$, $Y = 24$.
   - Colores: Rojo Tecmo `$16` y Blanco `$30`.
4. **Cornisa Decorativa de Piedra (Fondo Nametable)**:
   - Scanlines $Y = [0 .. 24]$: Almenas y friso de piedra espaciados cada 16 píxeles con sombra verde `$0A` y relieve gris `$10`.

#### B. Geometría y Layout de la Sala 1 (Descodificación de `RoomData_01`)
Desensamblado de [`asm/src/roomdata-us.asm:4-5`](file:///c:/Users/kanel/Documents/mjk/asm/src/roomdata-us.asm#L4-L5):
```asm
RoomData_01:
    .BYTE  $E0,   0, $81, $8F,   0,   5, $85, $87, $39, $6B, $6D, $10, $C6, $11, $CC, $FF
```
- **Cabecera `$E0, 0`**:
  - `palette_id = 0`: Carga `BackgroundPaletteSets` Set 0 (Fondo verde bosque `$0A`).
  - `music_id = 0`: Pista musical principal del juego.
- **Pilar Arquitectónico Izquierdo**:
  - Posición: $X = [0 .. 16]$, $Y = [32 .. 216]$.
  - Elementos: Columna de bloques de piedra estriados, rematada superiormente por un pebetero con calavera y antorcha encendida.
  - Llamas de la antorcha: $X = [0 .. 16]$, $Y = [32 .. 64]$, animadas con colores `$16` (rojo), `$26` (naranja) y `$38` (amarillo dorado).
- **Plataformas de Piedra Flotantes (Metatiles 16x16 redondeados)**:
  - **Plataforma Inferior-Izquierda**:
    - Filas de Metatiles: 9 (Y = 144) a 10 (Y = 160).
    - Columnas de Metatiles: 4.5 a 7.5 $\rightarrow X = [72 .. 120]$ ($W = 48\text{ px}$, 6 baldosas).
    - Altura: 16 px (2 filas de baldosas).
  - **Plataforma Superior-Derecha**:
    - Filas de Metatiles: 7 (Y = 112) a 8 (Y = 128).
    - Columnas de Metatiles: 10.5 a 14 $\rightarrow X = [168 .. 224]$ ($W = 56\text{ px}$, 7 baldosas).
    - Altura: 16 px.
- **Suelo Sólido de Piedra**:
  - Filas de Metatiles: 13.5 a 15 $\rightarrow Y = [216 .. 240]$ ($H = 24\text{ px}$).
  - Abarca horizontalmente todo el ancho de la pantalla: $X = [0 .. 255]$ (32 baldosas completas).
- **Puerta de Salida (`$81, $8F, 0`)**:
  - Posición: Fila 8, Columna 15 $\rightarrow X = [240 .. 255]$, $Y = [128 .. 160]$ (anclada a la pared derecha).
  - Condición de apertura: `DoorCond_00` (se abre al recolectar las bombas requeridas).
- **Cofres en Suelo**:
  - **Cofre Rojo (`$10, $C6`)**: Fila $C$ (12, $Y = 192$), Columna 6 ($X = 96$). Contiene moneda secreta / item.
  - **Cofre Naranja (`$11, $CC`)**: Fila $C$ (12, $Y = 192$), Columna 12 ($X = 192$). Requiere nivel Mighty 1 para abrir.
- **Grupo Inicial de 5 Bombas (`5, $85, $87, $39, $6B, $6D`)**:
  - Formato: Esferas rojas de $16 \times 16\text{ px}$ con mecha y chispas parpadeantes.
  - **Bomba 1 (`$85`, Primera de la serie de fuego con bit 7 activo)**: $X = 80$, $Y = 128$ (suspendida sobre la plataforma izquierda).
  - **Bomba 2 (`$87`)**: $X = 112$, $Y = 128$ (suspendida sobre la plataforma izquierda).
  - **Bomba 3 (`$39`)**: $X = 144$, $Y = 48$ (en el aire, zona central superior).
  - **Bomba 4 (`$6B`)**: $X = 176$, $Y = 96$ (descansando sobre la plataforma derecha).
  - **Bomba 5 (`$6D`)**: $X = 208$, $Y = 96$ (descansando sobre la plataforma derecha).
- **Spawn Inicial de Jack**:
  - Coordenadas detectadas en `frame_15s.png`: $X = 94$, $Y = 181$ (de pie sobre el suelo junto al cofre rojo).
  - Sprite: $16 \times 16\text{ px}$, capucha azul `$12`, traje rojo `$16`, cara carne/blanco `$30`.

---

## 4. Comparativa Exhaustiva Elemento a Elemento: Original vs Port Defectuoso

La siguiente auditoría pixel a pixel contrasta los fotogramas del port defectuoso (`scratch/port_frames/`) frente a la línea base original certificada:

```
+-----------------------------------+-----------------------------------+
|         ORIGINAL NES 1987         |          PORT DEFECTUOSO          |
|    (scratch/frames/frame_*.png)   |     (scratch/port_frames/*.png)   |
+-----------------------------------+-----------------------------------+
| 1. Resolución nativa 256x240      | 1. Lienzo arbitrario 3840x2016    |
|    Aspect ratio 4:3 estricto      |    Pixel aspect desproporcionado  |
|                                   |                                   |
| 2. Fondo Título: Pirámide dorada, | 2. Fondo Título: Tablero ajedrez  |
|    oasis verde, cielo cyan $21    |    púrpura sintético 0xFF141028   |
|                                   |                                   |
| 3. Logo Título: Logotipo 3D rojo  | 3. Logo Título: Texto vectorial   |
|    con extrusión $16 y cara $30   |    amarillo plano con font de 8x8 |
|                                   |                                   |
| 4. Jack Volador: Sprite con alas  | 4. Jack Volador: AUSENTE          |
|    batientes cayendo al pulsar St.|    Cero animación interactiva     |
|                                   |                                   |
| 5. HUD: Sprites OAM en rojo $16   | 5. HUD: Cadena de texto blanca    |
|    "T 60" (X=56), "Score" (X=88)  |    monolítica "1UP ... TIME ... R"|
|    "M 0" (X=184) a Y=16 y Y=24    |    dibujada a Y=6 con font TTF    |
|                                   |                                   |
| 6. Sala 1 Fondo: Verde $0A        | 6. Sala 1 Fondo: Azul oscuro plano|
|    Metatiles de piedra redonda    |    0xFF121020 con ladrillos falsos|
|                                   |                                   |
| 7. Elementos Arquitectónicos:     | 7. Elementos Arquitectónicos:     |
|    Pilar con calavera y antorcha  |    AUSENTES; sólo rectángulos de  |
|    Plataformas flotantes 112 y 144|    color marrón perimetral        |
|                                   |                                   |
| 8. Bombas y Cofres: 5 bombas rojas| 8. Bombas y Cofres: AUSENTES      |
|    con mecha animada, 2 cofres    |    Puerta amarilla falsa "EXIT"   |
|                                   |                                   |
| 9. Jack en juego: Sprite NES 16x16| 9. Jack en juego: Rectángulo de   |
|    articulado con capucha y traje |    16x16 con 3 franjas de color   |
+-----------------------------------+-----------------------------------+
```

### 4.1 Desglose Específico por Fotograma del Port

1. **`port_frame_1s.png` (Pantalla de Título del Port)**:
   - **Fondo**: Se implementó una fórmula sintética `((x/16) + (y/16)) % 2 == 0 ? 0xFF141028 : 0xFF1C1438` generando un patrón de damero azul/púrpura oscuro, violando el color universal NES `$21` (`#64B0FF`).
   - **Textos**: Se utilizó `MjkFont_DrawString` para escribir `"MIGHTY BOMB JACK"`, `"TECMO 1987 / SOURCE PORT C99"`, `"[ PRESS ENTER / BUTTON A TO PLAY ]"` y `"CONTROLS: KEYBOARD... XBOX PAD..."`. Ninguno de estos textos existe en la ROM de NES.
   - **Omisión Crítica**: Ausencia total de la Gran Pirámide, el oasis con palmeras, el logotipo con extrusión tridimensional roja, el indicador `"HI GDV 0"` y el Jack alado volador.
2. **`port_frame_5s.png` (Transición Round Intro del Port)**:
   - **Renderizado Defectuoso**: Pantalla negra con texto amarillo `"ROUND 01"` a $(80, 100)$ y texto cyan `"GET READY!"` a $(88, 130)$.
   - **Realidad 6502 (`prg.asm:7682-7700`)**: En la NES no existe ningún texto `"ROUND"` ni `"GET READY"`. La pantalla muestra exclusivamente el sprite de Jack a $(112, 96)$, el sprite `"x"` a $(132, 96)$ y el contador de vidas a $(148, 96)$ sobre fondo negro `$0F`.
3. **`port_frame_10s.png` y `port_frame_20s.png` (Sala 1 del Port)**:
   - **HUD Inventado**: Un string único `"1UP %06u TIME %02u R%02d"` blitteado en $(12, 6)$ con fuente de software blanca. Destruye la disposición espacial de tres zonas (`"T 60"` a la izquierda, marcador de 8 dígitos al centro y `"M 0"` a la derecha) y el color rojo característico de los números en NES.
   - **Fondo y Geometría Inexistente**: Fondo azul marino `0xFF121020` con una cuadrícula perimetral de falsos ladrillos de arcilla marrón (`0xFF8A5020` / `0xFF704018`). No existen los metatiles de piedra redonda, ni el pilar con antorcha, ni las plataformas flotantes escalonadas de Sala 1.
   - **Entidades Falsas**: Una caja amarilla en $(208, 176)$ con la palabra `"EXIT"` reemplaza la puerta original de Sala 1 (ubicada en $X=240, Y=128$). Las 5 bombas esféricas rojas y los 2 cofres fueron completamente omitidos. Jack es dibujado como un bloque geométrico plano de 3 franjas de color.

---

## 5. Tabla Maestra de Especificaciones de Coordenadas (NES 256x240)

Para la implementación en Fase 6, el frontend de renderizado debe respetar estrictamente esta matriz de coordenadas:

| Pantalla | Elemento Gráfico | Coordenada X | Coordenada Y | Ancho (W) | Alto (H) | Capa PPU | Paleta Asignada |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Copyright** | Texto "MIGHTY BOMB JACK (TM)" | 48 | 64 | 160 | 8 | Nametable 0 | BG Pal 0 (`$30`) |
| **Copyright** | Texto "TM AND (C)      TECMO,LTD."| 32 | 96 | 208 | 8 | Nametable 0 | BG Pal 0 (`$30`) |
| **Copyright** | Sprites BCD Año "1987" | 116, 132 | 99 | 32 | 8 | OAM Sprites | Sprite Pal 0 (`$30`) |
| **Copyright** | Texto "LICENSED BY" | 80 | 112 | 88 | 8 | Nametable 0 | BG Pal 0 (`$30`) |
| **Copyright** | Texto "NINTENDO OF AMERICA INC." | 32 | 128 | 192 | 8 | Nametable 0 | BG Pal 0 (`$30`) |
| **Título** | Logotipo 3D "MIGHTY BOMB JACK" | 17 | 26 | 231 | 78 | Nametable 0 | BG Pal 0, 3 (`$16, $26, $30`) |
| **Título** | Pirámide Dorada y Oasis | 0 | 112 | 256 | 128 | Nametable 0 | BG Pal 1, 2 (`$08, $27, $38, $0A, $1A`) |
| **Título** | Texto "PUSH START BUTTON" | 64 | 112 | 136 | 8 | Nametable 0 | BG Pal 0 (`$30`) |
| **Título** | Texto "(C) 1987 TECMO,LTD." | 152 | 128 | 88 | 8 | Nametable 0 | BG Pal 0 (`$30`) |
| **Título** | Marcador "HI GDV" | 176 | 16 | 48 | 8 | Nametable 0 | BG Pal 0 (`$30`) |
| **Título** | Dígito "0" de HI GDV | 200 | 24 | 8 | 8 | OAM Sprites | Sprite Pal 0 (`$16, $30`) |
| **Título** | Jack Volador con Alas | 40 | 120 | 32 | 16 | OAM Sprites | Sprite Pal 0, 1 (`$16, $12, $30`) |
| **Intro** | Jack Sprite (facing right) | 112 | 96 | 16 | 16 | OAM Sprites | Sprite Pal 0 (`$16, $12, $30`) |
| **Intro** | Símbolo "x" de Vidas | 132 | 96 | 8 | 8 | OAM Sprites | Sprite Pal 0 (`$30`) |
| **Intro** | Dígitos BCD de Vidas | 148 | 96 | 16 | 8 | OAM Sprites | Sprite Pal 0 (`$30`) |
| **Sala 1 (HUD)**| Icono "T" del Temporizador | 56 | 24 | 8 | 8 | OAM Sprites | Sprite Pal 0 (`$16, $30`) |
| **Sala 1 (HUD)**| Dígitos BCD Tiempo ("60") | 64, 72 | 24 | 16 | 8 | OAM Sprites | Sprite Pal 0 (`$16, $30`) |
| **Sala 1 (HUD)**| Marcador Puntuación (8 dígitos) | 88 | 16 | 64 | 8 | OAM Sprites | Sprite Pal 0 (`$16, $30`) |
| **Sala 1 (HUD)**| Icono "M" de Monedas Mighty | 184 | 24 | 8 | 8 | OAM Sprites | Sprite Pal 0 (`$16, $30`) |
| **Sala 1 (HUD)**| Dígito Contador Monedas ("0") | 192 | 24 | 8 | 8 | OAM Sprites | Sprite Pal 0 (`$16, $30`) |
| **Sala 1** | Pilar Izquierdo con Antorcha | 0 | 32 | 16 | 184 | Nametable 0 | BG Pal 0, 1 (`$0A, $16, $26, $38`) |
| **Sala 1** | Plataforma A (Media-Izquierda) | 72 | 144 | 48 | 16 | Nametable 0 | BG Pal 1 (`$0A, $0F, $10, $30`) |
| **Sala 1** | Plataforma B (Alta-Derecha) | 168 | 112 | 56 | 16 | Nametable 0 | BG Pal 1 (`$0A, $0F, $10, $30`) |
| **Sala 1** | Suelo de Piedra Continuo | 0 | 216 | 256 | 24 | Nametable 0 | BG Pal 1 (`$0A, $0F, $10, $30`) |
| **Sala 1** | Puerta de Salida (Derecha) | 240 | 128 | 16 | 32 | Nametable 0 | BG Pal 2 (`$0A, $0F, $16, $30`) |
| **Sala 1** | Cofre Rojo (Item Secreto) | 96 | 192 | 16 | 16 | Nametable 0 | BG Pal 2 (`$0A, $0F, $16, $30`) |
| **Sala 1** | Cofre Naranja (Mighty 1) | 192 | 192 | 16 | 16 | Nametable 0 | BG Pal 0 (`$0A, $16, $26, $38`) |
| **Sala 1** | Bomba 1 (Inicio Serie Fuego) | 80 | 128 | 16 | 16 | Nametable/OAM| Sprite Pal 2 / BG Pal 0 |
| **Sala 1** | Bomba 2 | 112 | 128 | 16 | 16 | Nametable/OAM| Sprite Pal 2 / BG Pal 0 |
| **Sala 1** | Bomba 3 | 144 | 48 | 16 | 16 | Nametable/OAM| Sprite Pal 2 / BG Pal 0 |
| **Sala 1** | Bomba 4 | 176 | 96 | 16 | 16 | Nametable/OAM| Sprite Pal 2 / BG Pal 0 |
| **Sala 1** | Bomba 5 | 208 | 96 | 16 | 16 | Nametable/OAM| Sprite Pal 2 / BG Pal 0 |
| **Sala 1** | Spawn Inicial de Jack | 94 | 181 | 16 | 16 | OAM Sprites | Sprite Pal 0, 1 (`$16, $12, $30`) |

---

## 6. Dictamen de Auditoría y Directrices para Fase 6

1. **Reemplazo Completo del Pipeline Gráfico de `src/core/mjk_game.c`**:
   - Queda totalmente prohibido el uso de cadenas de texto sintéticas (`MjkFont_DrawString`) blitteadas en software para el HUD o pantallas del juego.
   - El renderizador de Fase 6 debe operar bajo el modelo clásico de dos capas NES:
     1. **Nametable Buffer ($32 \times 30$ baldosas)** decodificado por metatiles desde `RoomData_xx`.
     2. **OAM Buffer (64 sprites de 4 bytes)** renderizados con orden de prioridad y supresión de ceros según las rutinas `DrawScore`, `DrawTimerOrTortureJumps`, `DrawMightyCoinCountOrMultiplier` y `DrawPlayer`.
2. **Fidelidad Estricta de Paletas**:
   - Todo color presentado en pantalla debe ser indexado exclusivamente desde la tabla maestra `NES_PALETTE_2C02` ($00..$3F) mapeada a través de `MainPalette` y `BackgroundPaletteSets`.
3. **Escalado de Ventana SDL2**:
   - El framebuffer lógico de salida debe mantenerse inmutablemente en $256 \times 240$ píxeles.
   - SDL2 debe gestionar el aspect ratio 4:3 en pantallas modernas aplicando bandas negras laterales (*pillarboxing*) y filtro Nearest-Neighbor (`SDL_HINT_RENDER_SCALE_QUALITY = "0"`), evitando resoluciones desproporcionadas no enteras.
