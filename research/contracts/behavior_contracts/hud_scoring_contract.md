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

### Marcador de Puntuación de 8 Dígitos (`PlayerScore` en `$00A0-$00A3`)
El marcador almacena 4 bytes BCD (8 dígitos decimales) en formato **Little-Endian** (`$00A0` = unidades/decenas, `$00A3` = millones/decenas de millón, confirmado en `prg.asm:2466-2486` y `ram.asm:240-244`). El renderizado en OAM suprime los ceros a la izquierda (*Zero-Suppression*) hasta el dígito de las unidades.

---

## 2. Temporizador de Fase (Divisor de 256 Frames)

El temporizador visible de 60 segundos (`StageTimer` en `$00A4`, `ram.asm:245`) se decrementa mediante un divisor de reloj de 256 frames (`TimerFrames` en `$00A5`, `ram.asm:248`):

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

## 4. Cálculo del GDV (Game Deviation Value) y los 4 Finales

El valor `PlayerGDV` (`$009C`) mide el desempeño del jugador al final de la partida (`GameState_A_GameOver`, `prg.asm:1038-1077`), pero **NO selecciona el final**:
- **Valor Inicial**: `$47` BCD al comenzar partida (`GameState_0_Init`, `prg.asm:358`).
- **Bonificaciones al calcular GDV**:
  - Al llegar a cualquier final (`_EndGame`, `prg.asm:7422-7423`): `+3` BCD.
  - En `GameState_A_GameOver` (`prg.asm:1050-1070`):
    - `(LastBombRoomCleared & $0F) >> 1`: `+0` a `+7` (1 punto cada 2 rondas superadas).
    - `ExtraLivesFound >> 1`: `+1` cada 2 vidas extra conseguidas.
    - Score bonus (`PlayerScore+2`, cap `$A0` si `PlayerScore+3 != 0`, `>> 5`): `+0` a `+5` (1 punto por cada 200.000 pts).
- Actualiza `HighGDV` (`$009B`) si `PlayerGDV >= HighGDV`.

### Evaluación de los 4 Finales (`EndingDoorChecks` en `prg.asm:7333-7425`)

Al cruzar una puerta hacia las cámaras finales (`Room $A0..$A3`), `EndingType = RoomID & 3` selecciona la compuerta de validación:

| Tipo de Final | Cámara | Condición de Bloqueo en `EndingDoorChecks` (`prg.asm:7376-7412`) | Resultado |
| :---: | :---: | :--- | :--- |
| **Ending 0 (Pobre)** | `$A0` | `_EndCheckCrystalBall1`: si `PlayerCrystalBallsCollected == 0` (`$0391`) -> dispara `_EndGame` con `EndingType = 0`. | Si posee al menos 1 bola, permite seguir hacia `$A1`. |
| **Ending 1 (Bueno)** | `$A1` | `_EndCheckCrystalBall2`: si `PlayerCrystalBallsCollected != 3` (no tiene ambas bolas) -> dispara `_EndGame` con `EndingType = 1`. | Si posee ambas bolas (Reina + Princesa), permite seguir hacia `$A2`. |
| **Ending 2 (Excelente)** | `$A2` | `_EndCheck5SecretCoins`: si `PlayerSecretCoins < 5` (`$03E5`) -> dispara `_EndGame` con `EndingType = 2`. | Si tiene `>= 5` Secret Coins, permite avanzar hacia `$A3`. |
| **Ending 3 (Supremo)** | `$A3` | Sin compuerta de bloqueo: entra directo en `_EndGame` con `EndingType = 3`. | Verdadero final completo de la familia real y créditos USA. |
