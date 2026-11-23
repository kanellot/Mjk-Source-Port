# Contrato Funcional: Cinemática, Física y Colisiones de Jack

- **ID de Contrato**: `research/contracts/behavior_contracts/jack_physics_contract.md`
- **Subsistema**: 02 — Cinemática, Física y Colisiones de Jack
- **Tarea**: `TASK-F3-002`
- **Fuentes de Ensamblador**:
  - `asm/src/prg.asm:1304-1636` (`MainSub_1`)
  - `asm/src/prg.asm:1960-2015` (`sub_8AE8` / Salto y Aleteo)
  - `asm/src/prg.asm:4615-4856` (`BigBeefySubHandlesBGColliding` / Colisiones 9-Probe)
  - `asm/src/data.asm:187-257` (`word_C0E3` curva 64 pasos, `word_C163` modulación X)
- **Estado Epistémico**: `[CONFIRMADA]`

---

## 1. Sistema de Coordenadas y Punto Fijo (8.8 Subpíxeles)

La posición y velocidad de Jack se representan en aritmética de punto fijo de 16 bits (8 bits enteros, 8 bits fraccionarios):
- **Posición X**: `(PlayerXPosHi << 8) | PlayerXPosLo` (Píxel entero: `$00-$FF`, Subpíxel: `$00-$FF`).
- **Posición Y**: `(PlayerYPosHi << 8) | PlayerYPosLo` (Píxel entero: `$00-$EF`).
- **Velocidad X**: `(PlayerXVelHi << 8) | PlayerXVelLo` con signo complemento a dos.
- **Velocidad Y**: `(PlayerYVelHi << 8) | PlayerYVelLo` con signo complemento a dos (positivo = hacia abajo, negativo = hacia arriba).

---

## 2. Movimiento Horizontal e Inercia (`word_C163`)

El desplazamiento horizontal se actualiza en cada frame dentro de `MainSub_1` (`prg.asm:1304`):

### Entradas y Lecturas de Control
- `Joypad1_Immediate & JP_Left` (Bit 1)
- `Joypad1_Immediate & JP_Right` (Bit 0)

### Tabla de Aceleración `word_C163` (`data.asm:252-257`)
| Índice | Valor Hex | Valor Subpíxel | Significado |
| :---: | :---: | :---: | :--- |
| `0` | `$00F0` | +0.9375 px/frame | Aceleración inicial desde reposo |
| `1` | `$0160` | +1.3750 px/frame | Velocidad máxima en suelo |
| `2` | `$00A0` | +0.6250 px/frame | Fricción de frenado al soltar mando |
| `3` | `$00D0` | +0.8125 px/frame | Modulación horizontal durante el aleteo en el aire |

### Algoritmo de Integración Horizontal
```c
void Jack_UpdateHorizontalVelocity(PlayerStruct *jack, uint8_t joypad) {
    if (joypad & JP_Right) {
        jack->facing_attr = 0x08; /* Sprites miran a la derecha */
        jack->x_vel = word_C163[1]; /* +1.375 px/frame */
    } else if (joypad & JP_Left) {
        jack->facing_attr = 0x00; /* Sprites miran a la izquierda (flip X) */
        jack->x_vel = -word_C163[1]; /* -1.375 px/frame */
    } else {
        /* Desaceleración / Fricción inmediata al soltar */
        jack->x_vel = 0;
    }
}
```

---

## 3. Física de Salto Parabólico y Aleteo (Flutter)

El salto de Jack no utiliza una aceleración gravitatoria constante tradicional ($v = v_0 + g \cdot t$), sino una **curva discreta no lineal de 64 pasos de velocidad vertical** tabulada en `word_C0E3` (`data.asm:187-251`):

### Tabla de Salto de 64 Pasos (`word_C0E3`)
```
Paso  0.. 4: 0x0380, 0x037C, 0x0378, 0x0374, 0x0370  (Ascenso inicial rápido: ~3.5 px/f)
Paso  5..10: 0x0368, 0x0360, 0x0358, 0x0350, 0x0348, 0x0340
Paso 11..20: 0x0330, 0x0320, 0x0310, 0x0300, 0x02F0, 0x02E0, 0x02D0, 0x02C0, 0x02B0, 0x02A0
Paso 21..35: 0x0290, 0x0280, 0x0270, 0x0260, 0x0250, 0x0240, 0x0230, 0x0220, 0x0210, 0x0200,
             0x01F0, 0x01E0, 0x01D0, 0x01C0, 0x01B0
Paso 36..50: 0x01A0, 0x0190, 0x0180, 0x0170, 0x0160, 0x0150, 0x0140, 0x0130, 0x0120, 0x0110,
             0x0100, 0x00F0, 0x00E0, 0x00D0, 0x00C0
Paso 51..63: 0x00B0, 0x00A0, 0x0090, 0x0080, 0x0070, 0x005E, 0x004C, 0x003A, 0x0028, 0x0010,
             0x0008, 0x0002, 0x0001 (Ápex parabólico: deceleración a 0)
```

### Mecánica del Aleteo (Flutter)
Cuando Jack está en el aire y el jugador mantiene presionado el botón A (`Joypad1_Immediate & JP_A`):
1. El puntero de avance en la tabla `word_C0E3` no se incrementa cada frame, sino cada **2 o 3 frames**, suspendiendo la gravedad y prolongando el tiempo en el aire.
2. Los sprites de Jack alternan entre los metatiles de vuelo `$14` y `$16` cada 4 frames (`anim_timer & 4`).

---

## 4. Detección de Colisiones 9-Probe contra el Fondo (`BigBeefySubHandlesBGColliding`)

La detección de colisiones con el escenario se realiza proyectando 9 sondas espaciales sobre la cuadrícula de metatiles de 16x16 píxeles:

```
              Probe 1 [Head Left]     Probe 2 [Head Center]    Probe 3 [Head Right]
                    (x+2, y-2)              (x+8, y-2)              (x+14, y-2)
                            +-------------------------------+
                            |                               |
    Probe 4 [Mid Left]      |         JACK HITBOX           |      Probe 5 [Mid Right]
        (x-1, y+8)          |           (16x16)             |          (x+17, y+8)
                            |                               |
                            +-------------------------------+
              Probe 6 [Feet Left]     Probe 7 [Feet Center]    Probe 8 [Feet Right]
                    (x+2, y+17)             (x+8, y+17)             (x+14, y+17)
```
- **Sonda 9 (Centro de Masa / Probe 0)**: Situada en `(x+8, y+8)`, calcula la interacción con cofres, bombas y puertas.

### Cálculo de Dirección del Metatile
Para una coordenada $(X, Y)$ en pantalla, el índice lineal del metatile en la matriz de $16 \times 15$ baldosas (`$0500-$05BF`) se calcula mediante la fórmula 6502 de `prg.asm:4616-4625`:
$$\text{MetatileIndex} = ((Y \ \& \ \$F0) \ | \ (X \gg 4))$$

### Resolución de Respuesta de Colisión
1. **Colisión de Techo (Probes 1, 2, 3)**: Si intersecta un metatile sólido con $v_y < 0$, fija $v_y = 0$, reproduce el efecto SFX de golpe de cabeza y avanza el paso de la tabla al ápex de caída ($32$).
2. **Colisión de Suelo (Probes 6, 7, 8)**: Si intersecta suelo sólido con $v_y \ge 0$, alinea $Y$ a la superficie del metatile (`Y = (Y & $F0)`), fija $v_y = 0$, limpia el flag de aire y marca estado en suelo (`grounded = true`).
3. **Colisión Lateral (Probes 4, 5)**: Impide el avance en el eje $X$, cancelando la velocidad $v_x = 0$ sin alterar el impulso vertical.
4. **Interacción con Tapa de Cofres**: Si las sondas 6, 7 o 8 impactan sobre un metatile de cofre mientras Jack cae ($v_y > 0$), se activa el salto rebotado y se transiciona el estado del cofre a abierto (`sub_9B6A`).
