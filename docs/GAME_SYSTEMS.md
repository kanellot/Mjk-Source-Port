# Sistemas de Juego — Mighty Bomb Jack (`src/game/`)

Este documento resume cómo funciona cada módulo de lógica pura en `src/game/`.

---

## 1. Entrada (`input.h` / `input.c`)
`MjkInputLatch` desacopla la frecuencia de lectura de SDL2 de la frecuencia fija de simulación (`60.0988 Hz`):
- `MjkInputLatch_Poll(&latch, raw_buttons)` detecta flancos nuevos (`pressed`, `released`) y los acumula con OR.
- `MjkInputLatch_ConsumeStep(&latch, &step)` entrega los flancos acumulados a un único paso de simulación y los limpia inmediatamente, manteniendo `held`.

## 2. Puntuación, GDV y Temporizador (`scoring.h` / `scoring.c`)
Todos los contadores son enteros estándar (`uint32_t score`, `int gdv`, `int lives`, `int timer_seconds`):
- **Multiplicadores**: en *Bomb Rooms*, `score_multiplier` (`0..4` → `×1..×5`) remapea la tabla de puntos.
- **Bonus / Extra Coin**: al cruzar cada múltiplo de `5.000` puntos en *Bomb Room* (con multiplicador `< ×5`), genera una *Bonus Coin* (o una *Extra Coin* con 1UP si `gold_coins >= 40`, reiniciando `gold_coins`).
- **StageTimer**: comienza en `60` y descuenta `1` unidad cada **256 frames** exactos; al llegar a `0` dispara `GS_TIME_OVER`.
- **GDV**: inicia en `47`, satura en `99` y conserva `high_gdv` entre partidas.

## 3. Mundo, Cámara y Colisiones (`world.h` / `world.c`)
- **256 Salas**: se instancian desde `assets->rooms[room_id]` (ya decodificadas) aplicando la persistencia de sección (**8 ranuras** para pasillos + **1 ranura scratch** para *Bomb Rooms* + banderas de Esfinge).
- **Cámara en `float`**: gestiona salas de pantalla única, pasillos horizontales (`256 px` por sala) y verticales (`240 px`), realizando el cruce de costura (`room_crossed`) y cargando automáticamente `current` y `neighbor`.
- **Colisión única (`MjkWorld_IsPointSolid`)**: compartida por Jack y los enemigos. Evalúa metatiles sólidos, los **10 pares de metatiles parciales `8×8`** (`0x38..0x4B`), los **7 tiles de fuego** (`0x5C..0x62`), puertas cerradas y cofres (sólidos por todos lados si son visibles; solo por arriba si están ocultos).

## 4. Física de Jack (`player.h` / `player.c`)
- **Horizontal**: velocidad instantánea `±1.5f` px/frame (`MJK_JACK_RUN_SPEED`), prioridad Derecha y parada instantánea.
- **Vertical**: curva parabólica de 64 pasos (`assets->jump_speed[64]`) avanzada con tasa `assets->jump_rate[v_idx]` (`0.9375` neutro, `1.375` Abajo, `0.625` Arriba). Como todos estos valores son múltiplos exactos de $1/256$, `float` reproduce la trayectoria original al píxel exacto.
- **Flutter**: activado estrictamente por flanco de subida en el aire, reiniciando `jump_progress = 64.0f`.
- **Muerte**: volteretas cayendo en vertical hasta tocar suelo y permanecer 64 frames tendido antes de emitir `ACTION_PLAYER_DIED`.

## 5. Objetos, Bombas, Cofres y Mighty (`objects.h` / `objects.c`)
- **Bombas**: apagadas (`100` pts) y encendidas (`200` pts). En *Bomb Rooms* (23 bombas), la chispa avanza por la cadena y la última bomba abre la puerta de salida. En pasillos, recoger en orden desde la bomba marcada otorga el bonus de `10.200` puntos.
- **Cofres y Bloques Secretos**: cofres rojos (cualquier nivel), cofres naranjas (requieren `mighty_level >= 1`), apertura con Mighty `≥ 2` al tocar/golpear, y bloques secretos rompibles al saltar encima.
- **Mighty Power**: pulsar `B` fuera de *Bomb Room* consume una *Mighty Coin* y sube nivel (`1→2→3`), con cuotas `{1, 20, 30, 35}` pulsaciones de `A`. Nivel `3` activa la transmutación de enemigos. Recoger 10 *Mighty Coins* (o superar 99 en el temporizador con *Mighty Drink*) dispara la **Sala de Tortura**.

## 6. Puertas (`doors.h` / `doors.c`)
Implementa las **33 condiciones de apertura `REV_US` (`0..32`)** con constantes semánticas (`DOOR_COND_*`), el caso especial de la Sala `$61`, los eventos de Esfinge y los eventos directos de Familia Real / Beelzebub.

## 7. Enemigos y Coleccionables (`enemies.h` / `enemies.c`)
- **Spawns**: regulados por `assets->difficulty_table` según la última *Bomb Room* superada, hasta 4 monstruos vivos simultáneos en 8 ranuras.
- **Mutación**: las Momias mutan de forma determinista según `(player.jump_press_count % 6) + 2` en los **6 arquetipos**: *Hanezo*, *GejiShogun*, *Gameido*, *Dokuron*, *Desufa* y *Horus*.
- **Power Coin**: transmuta monstruos durante **320 frames**, otorgando cadena `200→300→500→800→1200→2000` pts y 32 frames de gracia al restaurarse.

## 8. Máquina de Estados y 4 Finales (`game.h` / `game.c` / `screens.c`)
Orquesta el ciclo completo: `SCREEN_TITLE` (Copyright ↔ Title → Jack Falling) → `GS_INIT` → `GS_ROUND_INTRO` → `GS_LOAD_ROOM` → `GS_DRAW_SCREEN` → `GS_ENTER_ROOM` → `GS_PLAYING` (pausa, `GS_TIME_OVER`, `GS_TORTURE_ROOM` con **50 saltos US**, `GS_ROUND_CLEAR`, `GS_EXIT_ROOM`, `GS_ANIMATE_DOOR_OPEN`) → **4 Finales (`$A0..$A3`)** → `GS_GAME_OVER`.
