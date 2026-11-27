# Semántica y Tratamiento de Entrada de Controladores

- **ID de Entregable**: `research/contracts/input_semantics.md`
- **Fase**: Fase 3 — Reconstrucción Funcional y Contratos de Comportamiento
- **Tarea**: `TASK-F3-006`
- **Fuente de Verdad**: `asm/src/prg.asm:113-114`, `1271-1300`
- **Estado Epistémico**: `[CONFIRMADA]`

---

## 1. Mapeo de Bits del Registro de Entrada

En `UpdateJoypads`, cada frame serializa el estado del mando en un byte:

| Bit | Máscara Hex | Botón Físico | Comportamiento en Juego |
| :---: | :---: | :--- | :--- |
| **0** | `$01` | **A** | Salto inicial (por flanco) y aleteo sostenido (por nivel). |
| **1** | `$02` | **B** | Consumo de Mighty Coin / Incremento de Mighty Level (por flanco). |
| **2** | `$04` | **Select** | Selección de opciones en menús / debug. |
| **3** | `$08` | **Start** | Alternancia de pausa (por flanco) y arranque de partida. |
| **4** | `$10` | **Arriba (Up)** | No utilizado en la física estándar de Jack. |
| **5** | `$20` | **Abajo (Down)** | Agacharse / Cancelar impulso vertical. |
| **6** | `$40` | **Izquierda (Left)**| Desplazamiento horizontal a la izquierda (-1.375 px/f). |
| **7** | `$80` | **Derecha (Right)** | Desplazamiento horizontal a la derecha (+1.375 px/f). |

---

## 2. Tratamiento por Nivel vs Tratamiento por Flanco

### A. Botones por Nivel (Continuous State)
Se evalúan directamente contra `Joypad1_Immediate`:
```c
/* Movimiento horizontal continuo */
bool moving_left = (Joypad1_Immediate & JP_LEFT) != 0;
bool moving_right = (Joypad1_Immediate & JP_RIGHT) != 0;

/* Aleteo sostenido en el aire */
bool fluttering = (Joypad1_Immediate & JP_A) != 0;
```

### B. Botones por Flanco (Edge Detection con Retardo de 1 Frame)
Se evalúan comparando `Joypad1_Immediate` contra `Joypad1_ImmediateCopy`:
```c
/* Salto inicial: solo en el frame exacto en que se presiona A */
bool just_jumped = (Joypad1_Immediate & JP_A) && !(Joypad1_ImmediateCopy & JP_A);

/* Pausa: solo en el frame en que se presiona Start */
bool pause_toggle = (Joypad1_Immediate & JP_START) && !(Joypad1_ImmediateCopy & JP_START);

/* Mighty Power: solo en el frame en que se presiona B */
bool mighty_activate = (Joypad1_Immediate & JP_B) && !(Joypad1_ImmediateCopy & JP_B);
```

> **Regla de Paridad 1:1**:
> La actualización `Joypad1_ImmediateCopy = Joypad1_Immediate` debe ocurrir **al final del frame NMI** (`prg.asm:113-114`). Si se realiza antes del procesamiento del frame, la condición de flanco siempre evaluará `false` y ningún botón por flanco funcionará.
