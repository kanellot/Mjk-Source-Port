# Mecánicas de Gameplay, Enemigos, Torture Room y GDV (`REV_US`)

Especificación de bombas y cadenas de mechas, cofres y bloques secretos, *Mighty Power*, generación e IA de los 6 enemigos, puntuación flotante, Sala de Tortura y cálculo de GDV.

**Fuentes**: `asm/src/prg.asm` (`698-772`, `1040-1117`, `1397-1458`, `1745-1779`, `2548-2573`, `4860-5495`, `8150-9575`), `asm/src/data.asm` (`116-141`, `831-920`, `1963-2000`).

---

## 1. Bombas y Cadena de Mechas (`JTPickup_Bomb`)

- **Codificación en `RoomDataRAM`** (`prg.asm:3381-3398, 4970-5164`):
  - `$05`: bomba apagada (`100` pts).
  - `$85`: bomba inicial o final de cadena (`JT_94AD_0`).
  - `$45` / `$C5`: bomba con mecha encendida (`200` pts).
  - `$0F`: recogida (metatile vacío `$2F`).
- **Salas normales (pasajes)**: recoger las bombas siguiendo la cadena encendida desde la primera hasta la última otorga el **bonus perfecto de `10.000` pts** (+ `200` de la última bomba = `10.200`) y activa el bit 7 de `MaybeBombThing` (usado por `DoorCond_0C_AllFireBombs`).
- **Bomb Rooms (`$90..$9F`)**: 23 bombas totales; la chispa avanza por la cadena, se aplica `ScoreMultiplier` (`×1..×5`) y al recoger la 23ª se abre la puerta de salida. En `RoundClear`, `FireBombsCollected >= 20` otorga bonus especial (`10k / 20k / 30k / 50k`).
- **Umbral de Power Coin**: cada bomba suma `+1` (apagada) o `+2` (encendida) a `MaybeTempCollectableFlag2` (`$0356`). Al alcanzar **20**:
  - En *Bomb Room*: invoca `SpawnPowerCoin` directamente.
  - En pasaje: arma `MaybeTempCollectableFlag = 1` para que el próximo cofre abierto libere la *Power Coin*.

---

## 2. Cofres del Tesoro y Bloques Secretos

- **Cofres Rojos (`$10..$2F` visibles, `$90..$9F` ocultos)** y **Cofres Naranjas (`$30..$4F` visibles, `$A0..$AF` ocultos)** (`prg.asm:5259-5495`):
  - Los cofres **naranjas** se ignoran si `PlayerMightyLevel == 0` (requieren nivel `≥ 1`).
  - **Cofre visible**: se abre al saltar desde encima (`JustJumpedFlag != 0`), o golpeándolo desde abajo/lados con `PlayerMightyLevel >= 2`.
  - **Cofre oculto** (bit 5 = `$20`): nunca se activa desde abajo; requiere 2 saltos desde arriba (1º lo revela, 2º lo abre).
- **Bloques secretos (`$50..$6F`)**: requieren `N` saltos/golpes acumulados (`byte_335`) para revelarse y reproducir `Sound_SecretBlockRevealed`.
- **Bonus / Extra Coin**: cada múltiplo de `5.000` pts en *Bomb Room* (con multiplicador `< ×5`) genera una *Bonus Coin* (o *Extra Coin* `1UP` al acumular `40` *Gold Coins*, reiniciando el contador).

---

## 3. Mighty Power (`0..3`) y Codicia

- **Mighty Drink** (`prg.asm:4942-4953`): `+1.000` pts, `+$10` BCD al `StageTimer`. Si `StageTimer >= $A0` (> 99), confisca *Mighty Coins* y activa **Sala de Tortura** (`GameState = 7`).
- **Mighty Coin** (`prg.asm:5187-5201`): `+1.000` pts, `PlayerMightyCoins++`. Al llegar a **`10`**, confisca monedas y activa **Sala de Tortura**.
- **Activación con `B` (`UseMightyCoin`, `prg.asm:1745-1768`)**: prohibida en *Bomb Rooms*; consume 1 moneda, sube `PlayerMightyLevel` (`0→1→2→3`), actualiza la paleta de Jack (`MightyLevelColors`) y carga el presupuesto de pulsaciones de `A` (`MightyLevelAPressesTable`: `{1, 20, 30, 35}`). Al alcanzar nivel **3**, llama a `TurnEnemiesIntoCoins`.
- **Degradación**: cada pulsación de `A` decrementa `PlayerMightyLevelPressesLeft`; al llegar a 0 desciende un nivel.

---

## 4. Enemigos, Spawning y Mutación Determinista

### Tabla de Dificultad (`DifficultyTable`, `data.asm:116-141`)
Indexada según la última *Bomb Room* completada (`LastBombRoomCleared`):

| Ronda | `Speed` | `SpawnTimer` (frames) | `TF_Normal` | `TF_BombRoom` |
| :---: | :---: | :---: | :---: | :---: |
| 1–2 | `0` | `$A0` (160) | `$80` (128) | `$40` (64) |
| 3 | `1` | `$80` (128) | `$70` (112) | `$38` (56) |
| 4 | `2` | `$70` (112) | `$60` (96) | `$30` (48) |
| 5 | `3` | `$60` (96) | `$50` (80) | `$28` (40) |
| 6 | `4` | `$58` (88) | `$40` (64) | `$20` (32) |
| 7 | `5` | `$50` (80) | `$30` (48) | `$18` (24) |
| 8+ | `4` | `$48` (72) | `$20` (32) | `$10` (16) |

- **Límite concurrente**: máximo **4 enemigos vivos** simultáneos (`AliveEnemyCount < 4`, `prg.asm:8203`) en 8 slots de entidad (`Object0Struct..Object7Struct`).
- **Mutación determinista (`TransformMummy`, `prg.asm:9362-9375`)**: las Momias (tipo `0`) mutan según **`(APressCounter % 6) + 2`**:
  - `2`: **Hanezo** (vuelo sinusoidal)
  - `3`: **GejiShogun** (oruga trepadora)
  - `4`: **Gameido** (tortuga rebote 45°)
  - `5`: **Dokuron** (calavera diagonal)
  - `6`: **Desufa** (demonio perseguidor)
  - `7`: **Horus** (halcón en picado)
- **Power Coin (`EnemyCoinTimer`)**: pausa spawns y transmuta enemigos en monedas durante **320 frames**, otorgando cadena progresiva (`200 → 300 → 500 → 800 → 1200 → 2000` pts) con parpadeo en los últimos 64 frames y periodo de gracia al restaurarse.

---

## 5. Puntuación Flotante, Sala de Tortura y GDV

- **Popups de puntuación (`sub_8FB1`, `prg.asm:2548-2573`)**: muestran los puntos ganados (`100`, `200`, multiplicador en Bomb Room) sobre el evento en OAM.
- **Sala de Tortura (`GameState_7_GoToTortureRoom`, `prg.asm:698-772`)**:
  - Muestra `YOU ARE GREEDY / GO TO THE TORTURE ROOM` durante **192 frames** (`$C0`).
  - Traslada a sala **`$AC`** con **`GreedyJumpsRemaining = $50` BCD (50 saltos en `REV_US`; 10 en JP)**. Al llegar a 0 restaura el checkpoint.
- **Cálculo de GDV (`GameState_A_GameOver`, `prg.asm:1050-1070`)**:
  - Inicia en **`47`** (`$47` BCD), satura en **`99`** y se guarda en `HighGDV`.
  - Bonos en Game Over: `+(LastBombRoomCleared & $0F) / 2` (0–7), `+ExtraLivesFound / 2`, `+Score / 200.000` (0–5) y **`+3`** al completar un final (`_EndGame`, `prg.asm:7420`).
