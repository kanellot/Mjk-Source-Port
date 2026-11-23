# Modelo de Temporización y Cadencia a 60 Hz

- **ID de Entregable**: `research/contracts/timing_model.md`
- **Fase**: Fase 3 — Reconstrucción Funcional y Contratos de Comportamiento
- **Tarea**: `TASK-F3-006`
- **Estándar de Video**: NTSC (60,0988 Hz en hardware NES, 60.00 Hz en simulación PC)
- **Estado Epistémico**: `[CONFIRMADA]`

---

## 1. Cadencia de Reloj Intra-Frame

El juego está estrictamente acoplado al intervalo vertical de refresco de pantalla (VBlank):
- **Duración de Frame Nominal**: $\approx 16.666\text{ ms}$ ($1/60\text{ s}$).
- **Presupuesto VBlank 6502**: Alrededor de $2.270$ ciclos de CPU ($20$ líneas de escaneo de VBlank en NTSC). En este tiempo se deben completar exclusivamente las transferencias a la PPU:
  - OAM DMA: 513 ciclos.
  - Vuelco de paletas / VRAM buffer: 200 a 800 ciclos.
  - Tras el VBlank, el resto del frame (hasta 27.000 ciclos) se utiliza para la física, colisiones e IA.

---

## 2. Catálogo de Temporizadores en RAM

| Temporizador | Dirección | Resolución / Divisor | Comportamiento al Expirar |
| :--- | :---: | :--- | :--- |
| **`TimerFrames`** | `$003F` | 256 frames ($\approx 4.26\text{ s}$) | Decrementa `StageTimer` en 1 unidad BCD. |
| **`StageTimer`** | `$0440` | Unidades BCD (inicia en $60) | Al llegar a 0, transiciona a `GameState_6_TimeOver`. |
| **`TimerEnemyTransmute`** | `$0045` | 320 frames ($\approx 5.33\text{ s}$) | Al llegar a 0, las monedas vuelven a convertirse en monstruos. |
| **`MightyTimer`** | `$0048` | 320 frames | Al llegar a 0, el poder Mighty de Jack se reduce en 1 nivel. |
| **`PlayerDeathTimer`** | `$004A` | 64 frames ($\approx 1.06\text{ s}$) | Duración de la animación de muerte de Jack antes de perder vida. |
| **`GameState1WaitTimer`**| `$004C` | 96 frames ($\approx 1.60\text{ s}$) | Duración de la pantalla negra "ROUND XX" antes de cargar la sala. |
| **`GameOverTimer`** | `$004E` | 256 frames ($\approx 4.26\text{ s}$) | Espera en pantalla de Game Over antes de volver al título. |
| **`TortureMessageTimer`**| `$0050` | 192 frames ($\approx 3.20\text{ s}$) | Muestra el rótulo "TORTURE ROOM" antes de dar el control a Jack. |
