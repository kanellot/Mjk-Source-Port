# Gráficos, PPU 2C02, Sprites OAM y HUD (`REV_US`)

Especificación del ensamblado de metasprites en OAM, paletas canónicas 2C02 y coordenadas exactas del HUD y pantallas de texto.

**Fuentes**: `asm/src/prg.asm` (`1640-1654`, `1774-1950`, `2050-2374`, `7496-7700`), `asm/src/data.asm` (`258-307`, `364-596`).

---

## 1. Metasprites `16×16` (`SpritesTable`)

`SpritesTable` (`data.asm:364-411`) define 46 entradas (`$00..$2D`) de **5 bytes** cada una:
- **Byte 0 (`ControlByte`)**: máscara de flip intrínseco por cuadrante (`$44` = simétrico horizontal con H-Flip en TR/BR; `$00` = asimétrico de 4 tiles; `$40` = H-Flip solo en BR).
- **Bytes 1..4**: índices de tile `8×8` para los cuadrantes TL (`X-8, Y-8`), TR (`X, Y-8`), BL (`X-8, Y`) y BR (`X, Y`).

### Animaciones de Jack (`PlayerSprite` en `$03DC`)

| Estado | Índice | `ControlByte` | Tiles (TL, TR, BL, BR) | Notas |
| :--- | :---: | :---: | :---: | :--- |
| **Idle frontal** | `$00` | `$44` | `$10, $10, $12, $12` | Simétrico (no existe pose de agacharse). |
| **Salto vertical** | `$01` | `$44` | `$11, $11, $13, $13` | Brazos arriba. |
| **Planeo / Flutter** | `$02` | `$44` | `$14, $14, $16, $16` | Brazos extendidos. |
| **Salto Intro / Title** | `$03` | `$44` | `$15, $15, $17, $17` | Caída hacia la pirámide. |
| **Caminar Frame 1** | `$04` | `$00` | `$1C, $1D, $1E, $1F` | Asimétrico (también ascenso diagonal). |
| **Caminar Frame 2** | `$05` | `$00` | `$1C, $1D, $20, $21` | Alterna con `$04` vía `byte_3DF`. |
| **Caída diagonal** | `$06` | `$00` | `$18, $19, $1A, $1B` | Planeo lateral. |
| **Muerte Giro A / B** | `$28 / $29` | `$44` | `$45,$45,$47,$47` / `$4D,$4D,$4F,$4F` | Alterna cada 6 frames al morir. |
| **Derrotado / Aplastado** | `$2A` | `$40` | `$0F, $0F, $8A, $8A` | `$0F` transparente, base aplastada `$8A`. |

### Ensamblado OAM (`DrawPlayer` y `WriteSprite`)
- `DrawPlayer` (`prg.asm:1640-1654`) coloca siempre a Jack en `CurrentSpriteIndex = 0` (slots OAM `0..3`, máxima prioridad hardware), con `TempSpriteAttributesish = byte_3DD | $01` (`byte_3DD`: `$00` = izquierda, `$04` = derecha; bit 0 = subpaleta de sprite 1).
- `WriteSprite` (`prg.asm:2267-2374`) usa `SpriteAttributeTable` (`data.asm:588-596`):
  - Izquierda: `byte_4E = $00`, `byte_4F = $27` (orden TL, TR, BL, BR).
  - Derecha: `byte_4E = $55`, `byte_4F = $8D` (invierte posiciones a TR, TL, BR, BL y aplica `XOR $55` para H-Flip).
  - Para sprites individuales `8×8` (bit 4 de atributo = `$10`), `WriteSprite` resta `4` píxeles a `X` e `Y`.

---

## 2. Paletas Canónicas 2C02 (`MainPalette`)

`RestoreDefaultPalettes` (`prg.asm:1937-1950`) copia 32 bytes de `MainPalette` (`data.asm:258-268`) en `PaletteBuffer` (`$05E3–$0602`):

```text
BG 0: $21, $0F, $26, $30    |  SPR 0: $21, $16, $16, $30
BG 1: $21, $08, $27, $38    |  SPR 1: $21, $30, $12, $16  <- Paleta fija de Jack
BG 2: $21, $0A, $1A, $39    |  SPR 2: $21, $08, $16, $30
BG 3: $21, $0F, $16, $30    |  SPR 3: $21, $0F, $2B, $30
```

### Paleta Dinámica de Mighty Power (`MightyLevelColors`)
`UpdateMightyPalette` (`prg.asm:1774-1779`) modifica únicamente el **Color 3 de Sprite 1** (`PaletteBuffer + $17`, PPU `$3F17`) según `MightyLevelColors` (`data.asm:306-307`):

| Nivel Mighty | Color 3 (`$3F17`) | Tríada Visible de Jack (`$3F15..$3F17`) |
| :---: | :---: | :--- |
| **0 (Normal)** | `$16` (Rojo) | Blanco (`$30`), Azul (`$12`), Rojo (`$16`) |
| **1** | `$12` (Azul) | Blanco (`$30`), Azul (`$12`), Azul (`$12`) |
| **2** | `$26` (Naranja) | Blanco (`$30`), Azul (`$12`), Naranja (`$26`) |
| **3** | `$2A` (Verde) | Blanco (`$30`), Azul (`$12`), Verde (`$2A`) |

---

## 3. Layout Exacto del HUD (OAM)

En el juego original, el HUD activo durante `GameState_5_Main` se dibuja íntegramente mediante **sprites OAM** (no en Nametable):

| Elemento | Rutina (`prg.asm`) | Coordenadas OAM Reales | Notas |
| :--- | :--- | :---: | :--- |
| **Score (8 dígitos)** | `DrawScore` (`1801-1865`) | `Y = 16` (`$10`), `X = 88..144` (`$58..$90`, paso 8) | Escribe directo en OAM sin restar 4; ceros a la izquierda se ocultan en `Y = $F8`. |
| **Timer `'T'` + 2 dígitos** | `DrawTimerOrTortureJumps` (`1871-1897`) | `Y = 24` (`$18`), `X = 56, 64, 72` (`$38, $40, $48`) | Pasa por `WriteSprite` (`-4 px`). Tile `'T' = $0E`; decena `0` usa tile transparente `$0F`. |
| **Mighty `'M'` / `'x'` + dígito** | `DrawMightyCoinCountOrMultiplier` (`1903-1933`) | `Y = 24` (`$18`), `X = 184, 192` (`$B8, $C0`) | Pasa por `WriteSprite` (`-4 px`). Tile `'M' = $0B`; en Bomb Room usa `'x' = $0C` + `ScoreMultiplier`. |

---

## 4. Textos de Pantallas Especiales (`WriteStringToPPU`)

- **Copyright USA (`TitleScreen_US_0`, `prg.asm:7496-7520`)**:
  - `$1C` `MIGHTY BOMB JACK TM` (`$2106`), `$1D` `TM AND (C) TECMO,LTD.` (`$2184`), `$1E` `LICENSED BY` (`$21CA`), `$1F` `NINTENDO OF AMERICA INC.` (`$2204`) + dígitos OAM `1987` en `(X=$74/$84, Y=$63)`.
- **Title Screen (`TitleScreen_0`, `prg.asm:7533-7569`)**:
  - Sala `0` de fondo, `$00` `PUSH START BUTTON` (`$21C8`), `$0B` `(C) TECMO` (`$220A`), `$0C` `HIGH GDV` (`$2213`), `$1B` `TM` (`$219E`) + dígitos OAM de `HighGDV` en `(X=$CC, Y=$1C)`.
- **Round Intro (`GameState_1_RoundIntro`, `prg.asm:7682-7700`)**:
  - Jack en `(X=$70, Y=$60)`, `'x'` en `(X=$84, Y=$60)` y `PlayerLives` en `(X=$94, Y=$60)`.
