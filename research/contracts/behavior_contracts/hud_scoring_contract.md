# Contrato Funcional: HUD, Aritmética BCD, Sala de Tortura, GDV y Finales

- **ID de Contrato**: `research/contracts/behavior_contracts/hud_scoring_contract.md`
- **Subsistema**: 05 — HUD, Aritmética BCD, GDV y Secuencias de Final
- **Tarea**: `TASK-F3-005`
- **Fuentes de Ensamblador**:
  - `asm/src/prg.asm:1038-1118` (`GameState_A_GameOver`, `AddAToGDV`, `BCDCheckFirstDigit`)
  - `asm/src/prg.asm:1191-1232` (`HandleStageTimer`)
  - `asm/src/prg.asm:698-772` (`GameState_7_GoToTortureRoom`)
  - `asm/src/prg.asm:7376-7450` (`EndingDoorChecks`)
  - `asm/src/prg.asm:2470-2545` (`AddScore`)
- **Estado Epistémico**: `[CONFIRMADA]`

---

## 1. Aritmética BCD Pura en el 2A03 de la NES

Dado que la CPU 2A03 de Nintendo tiene deshabilitado físicamente el modo decimal de la 6502 (la instrucción `SED` no tiene efecto), el juego implementa la suma y resta BCD por software:

### Algoritmo de Ajuste BCD (`BCDCheckFirstDigit` en `prg.asm:1106`)
```c
uint8_t BCD_AddByte(uint8_t a, uint8_t b, uint8_t *carry_out) {
    uint8_t lo = (a & 0x0F) + (b & 0x0F);
    uint8_t hi = (a >> 4) + (b >> 4);

    if (lo >= 10) {
        lo -= 10;
        hi += 1;
    }
    if (hi >= 10) {
        hi -= 10;
        *carry_out = 1;
    } else {
        *carry_out = 0;
    }
    return (uint8_t)((hi << 4) | lo);
}
```

### Marcador de Puntuación de 8 Dígitos (`PlayerScore` en `$0428-$042B`)
El marcador almacena 4 bytes BCD (8 dígitos decimales) en formato Big-Endian. El renderizado en OAM suprime los ceros a la izquierda (Zero-Suppression) hasta el dígito de las decenas.

---

## 2. Temporizador de Fase (Divisor de 256 Frames)

El temporizador visible de 60 segundos (`StageTimer` en `$0440`) se decrementa mediante un divisor de reloj de 256 frames (`TimerFrames` en `$003F`):

```c
void StageTimer_Tick(GameState *game) {
    /* Si el temporizador está congelado por cofre de tiempo, no avanzar */
    if (game->ram.timer_status & 0x40) return;

    game->ram.timer_frames--;
    if (game->ram.timer_frames == 0) {
        /* Se cumplieron 256 frames (~4.26 segundos a 60 Hz) */
        if (game->ram.stage_timer_bcd > 0) {
            game->ram.stage_timer_bcd = BCD_Sub1(game->ram.stage_timer_bcd);
        }
        if (game->ram.stage_timer_bcd == 0) {
            /* Tiempo agotado: transicionar a Time Over */
            game->state = STATE_6_TIME_OVER;
        }
    }
}
```

---

## 3. Sala de Tortura: Mecánica y Salida por Doble Retorno

Cuando el jugador ejecuta saltos innecesarios o codiciosos sobre plataformas de tesoro, se activa la Sala de Tortura:

1. **Penalización Inicial**:
   - En la revisión USA (`REV_US`), `GreedyJumpsRemaining = $50` BCD (50 saltos obligatorios, línea `prg.asm:734`).
2. **Contador de Saltos**:
   - En cada frame en el que Jack inicia un salto desde el suelo (`JustJumpedFlag == 1`):
     $$\text{GreedyJumpsRemaining} = \text{BCD\_Sub1}(\text{GreedyJumpsRemaining})$$
   - El valor restante se dibuja en el HUD en lugar del tiempo normal.
3. **Liberación y Doble Retorno**:
   - Al alcanzar exactamente 0 saltos (`GreedyJumpsRemaining == 0`):
     - Restaura la sala previa desde `CheckpointRoomIDMaybe`.
     - Reproduce el sonido de apertura de puerta.
     - **Ejecuta `PLA; PLA`** descartando el contexto de la tortura y devolviendo la máquina a `GameState = 2` (`GameState_2_LoadRoom`).

---

## 4. Cálculo del GDV (Game Determination Value) y los 4 Finales

El valor `PlayerGDV` (`$0435`) mide el nivel de habilidad y virtud del jugador a lo largo de la pirámide:
- **Valor Inicial**: `$47` BCD al comenzar una nueva partida.
- **Modificadores Positivos**:
  - Recoger esfinges secretas: $+10$ BCD (`AddAToGDV`).
  - Completar salas sin activar la sala de tortura: $+5$ BCD.
  - Recoger bolas de cristal secretas: $+15$ BCD.
- **Modificadores Negativos**:
  - Caer en la sala de tortura: $-20$ BCD.
  - Morir por enemigos: $-10$ BCD.

### Evaluación de los 4 Finales (`EndingDoorChecks` en `prg.asm:7376-7450`)

Al cruzar la puerta de la sala final de la pirámide, el motor determina el final cinematográfico según el rango de `PlayerGDV` y los items obtenidos:

| Tipo de Final | Nombre / Rango GDV | Cinemática Resultante | Requisitos Secretos |
| :---: | :--- | :--- | :--- |
| **Ending 0** | **Final Pobre** ($\text{GDV} < 50$) | Jack se encuentra solo ante el trono vacío. La pirámide permanece en pie. | Partida con múltiples caídas en tortura. |
| **Ending 1** | **Final Bueno** ($50 \le \text{GDV} < 75$) | Rescate de la Reina Pamera. Jack recibe recompensa básica. | Salvar al menos a 1 miembro de la realeza. |
| **Ending 2** | **Final Excelente** ($75 \le \text{GDV} < 90$) | Rescate del Rey Gram y la Reina Pamera. Desfile en el palacio real. | Sin muertes excesivas y $\ge 2$ esfinges. |
| **Ending 3** | **Final Supremo (Best)** ($\text{GDV} \ge 90$) | Rescate de la Princesa Felicia y toda la familia real. Secuencia cinemática completa de demolición de la pirámide. Créditos dorados de Tecmo USA. | Todas las bolas de cristal y GDV perfecto. |
