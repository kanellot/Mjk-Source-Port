# Física de Jack y Sistema de Colisiones 9-Probe (`REV_US`)

Especificación de la cinemática de Jack, la curva parabólica de salto de 64 pasos, la mecánica de *flutter* y el sistema de colisiones de 9 sondas sobre metatiles `16×16` y cuadrantes `8×8`.

**Fuentes**: `asm/src/prg.asm` (`1397-1741`, `3871-3900`, `4152-4706`, `5529-5609`, `5863-5903`), `asm/src/data.asm` (`187-256`, `598-639`).

---

## 1. Cinemática Horizontal

- **Sin aceleración ni inercia**: la velocidad horizontal es instantánea (`±1.5` px/frame = `±$0180` en punto fijo Q8.8) y al soltar el D-Pad Jack se detiene en el mismo frame (`prg.asm:1533-1636`).
- **Bloqueo lateral**: `CollisionFlag & $01` bloquea avance hacia la derecha; `CollisionFlag & $02` bloquea avance hacia la izquierda (`prg.asm:1547-1562`).

---

## 2. Curva Vertical de Salto y *Flutter*

### Tabla de 64 Pasos (`word_C0E3`, `data.asm:187-251`)
Magnitudes sin signo en Q8.8 (subpíxeles/frame, donde `$0100 = 1.0 px/frame`):

```text
00..07: 0380 037C 0378 0374 0370 0368 0360 0358   (3.500 -> 3.344 px/f)
08..0F: 0350 0348 0340 0330 0320 0310 0300 02F0   (3.313 -> 2.938 px/f)
10..17: 02E0 02D0 02C0 02B0 02A0 0290 0280 0270   (2.875 -> 2.438 px/f)
18..1F: 0260 0250 0240 0230 0220 0210 0200 01F0   (2.375 -> 1.938 px/f)
20..27: 01E0 01D0 01C0 01B0 01A0 0190 0180 0170   (1.875 -> 1.438 px/f)
28..2F: 0160 0150 0140 0130 0120 0110 0100 00F0   (1.375 -> 0.938 px/f)
30..37: 00E0 00D0 00C0 00B0 00A0 0090 0080 0070   (0.875 -> 0.438 px/f)
38..3F: 005E 004C 003A 0028 0010 0008 0002 0001   (0.367 -> 0.004 px/f)
```

### Modulación de Avance (`word_C163`, `data.asm:252-256`)
En cada frame en el aire, `DoSomethingWithObjYVel` (`prg.asm:1658-1685`) suma `word_C163[(Joypad1 & $0C) >> 1]` al acumulador de 16 bits (`byte_3E7:byte_3E6`):

| Índice | D-Pad Vertical | Valor Hex (Q8.8) | Pasos / Frame | Efecto |
| :---: | :--- | :---: | :---: | :--- |
| `0` | Neutro | `$00F0` | `0.9375` | Salto estándar |
| `1` | Abajo (`$04`) | `$0160` | `1.3750` | Salto corto / caída rápida |
| `2` | Arriba (`$08`) | `$00A0` | `0.6250` | Salto alto / planeo prolongado |
| `3` | Arriba + Abajo (`$0C`) | `$00D0` | `0.8125` | Combinado |

### Reflexión Ascenso/Descenso y *Flutter* (`sub_8AE8`, `prg.asm:1689-1709`)
- Mientras `byte_3E7 < $40`: Jack **asciende**; se indexa `word_C0E3[byte_3E7]` con signo negativo (`NegateYYXX`).
- Cuando `byte_3E7 >= $40`: Jack **desciende**; el índice se refleja mediante `(byte_3E7 ^ $FF) & $3F` y la velocidad es positiva (hacia abajo), saturando en la velocidad terminal de caída.
- **Flutter**: activado estrictamente por **flanco de subida de `A`** (o `A`/`B` en *Bomb Room*) mientras Jack está en el aire (`prg.asm:1397-1447`). Reinicia `byte_3E7 = $40, byte_3E6 = $00`, frenando la caída en el ápice de la parábola.

---

## 3. Sistema de Colisiones 9-Probe

### Geometría de las 9 Sondas (`byte_A3D8`, `prg.asm:5863-5872`)

Coordenadas relativas `(dX, dY)` respecto al ancla de Jack `(PlayerXPosHi, PlayerYPosHi)`:

| Sonda | `(dX, dY)` | Región | Bit en `CollisionFlag` (`$03CF`) |
| :---: | :---: | :--- | :--- |
| **0** | `(0, 0)` | Centro | Detección de fuego letal, objetos y cruce de celda `8×8`. |
| **1 / 2** | `(-4, -8)` / `(+4, -8)` | Cabeza Izq / Der | Bit 3 (`$08`) — impacto de techo (`sub_A1B9`: fija `byte_3E7 = $40`, `Y |= 7`). |
| **3 / 4** | `(-4, +8)` / `(+4, +8)` | Pies Izq / Der | Bit 2 (`$04`) — aterrizaje (`sub_A1EB`: alinea `Y &= $F8`, limpia aire). |
| **5 / 6** | `(-8, -4)` / `(-8, +4)` | Muro Izq Sup / Inf | Bit 1 (`$02`) — bloqueo izquierdo. |
| **7 / 8** | `(+8, -4)` / `(+8, +4)` | Muro Der Sup / Inf | Bit 0 (`$01`) — bloqueo derecho. |

### Resolución de Solidez de Metatiles (`sub_9B6A`, `prg.asm:4480-4611`)
1. **Pasantes (vacío)**: metatiles `$00`, `$2F` y `$27` (bomba) → máscara `$00`.
2. **Fuego letal**: rango `$5C..$62`. Si toca la sonda 0 (centro), fija `FireTilePlayerDiedTo` y `PlayerStruct |= $80` (muerte inmediata).
3. **Sólidos parciales `8×8`**: rango `$38..$4B` (20 metatiles). Se consultan 10 bytes en `byte_A3EA` (`prg.asm:5873-5883`), 4 bits por metatile (bits `7..4` = cuadrantes TL, TR, BL, BR).
4. **Sólidos completos `16×16`**: resto de metatiles `< $38` o `>= $4C` → máscara `$FF`.
