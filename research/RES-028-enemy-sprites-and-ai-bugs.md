# RES-028: Auditoría Ground-Truth de Enemigos — Desactivación Limpia (`TASK-F7-001`), Bug de Sprites Numéricos y Movimiento Errático (`TASK-F7-010`)

- **ID de Investigación**: `RES-028`
- **Tareas Vinculadas**: `TASK-F7-001` (Desactivación limpia en runtime) y `TASK-F7-010` (Corrección de sprites numéricos e IA/física de enemigos)
- **Área Temática**: 04 - Enemigos, Inteligencia Artificial y Spawns / 05 - Renderizado y PPU
- **Estado**: `[COMPLETADA]`
- **Fecha**: 2026-10-25
- **Agente Autor**: `asm_researcher` (Nivel 3)
- **Fuentes de Verdad Analizadas**:
  - Desensamblado 6502: [`asm/src/prg.asm`](../asm/src/prg.asm), [`asm/src/data.asm`](../asm/src/data.asm), [`asm/src/defs.asm`](../asm/src/defs.asm), [`asm/src/ram.asm`](../asm/src/ram.asm), [`asm/src/mbj-us.chr`](../asm/src/mbj-us.chr).
  - Código C del Port: [`src/core/mjk_enemy.c`](../src/core/mjk_enemy.c), [`include/mjk_enemy.h`](../include/mjk_enemy.h), [`src/core/mjk_game.c`](../src/core/mjk_game.c), [`src/core/mjk_state_machine.c`](../src/core/mjk_state_machine.c), [`src/core/mjk_player.c`](../src/core/mjk_player.c).
  - Suite de Pruebas: [`tests/test_enemy_ai.c`](../tests/test_enemy_ai.c), [`tests/test_enemy_spawns.c`](../tests/test_enemy_spawns.c), [`tests/test_state_machine.c`](../tests/test_state_machine.c), [`tests/test_powerups_mighty.c`](../tests/test_powerups_mighty.c).

---

## 1. Punto A: Puntos de Integración y Desactivación Limpia en Runtime (`TASK-F7-001`)

### 1.1 Inventario Exacto de Llamadas al Subsistema de Enemigos en Runtime

| Archivo | Líneas | Función Contenedora | Operación Realizada | Impacto en Runtime / Tests |
| :--- | :--- | :--- | :--- | :--- |
| [`src/core/mjk_state_machine.c`](../src/core/mjk_state_machine.c#L356-L361) | `356-361` | `State_2_LoadRoom` | Llama a `MjkEnemy_Init(&game->enemy_mgr, game->current_round, room_id, game->is_bomb_room)`, pone `game->active_enemies_count = 0` y desactiva `game->enemies[0..7].active = false`. | **DEBE CONSERVARSE**: Garantiza estado limpio al cargar sala y satisface [`tests/test_state_machine.c:153`](../tests/test_state_machine.c#L153) (`assert(game.active_enemies_count == 0)`). |
| [`src/core/mjk_state_machine.c`](../src/core/mjk_state_machine.c#L370-L374) | `370-374` | `State_3_DrawScreen` | Purga `game->active_enemies_count = 0` y `game->enemies[0..7].active = false`. | **DEBE CONSERVARSE**: Limpia el espejo de entidades de `MjkGameState`. |
| [`src/core/mjk_state_machine.c`](../src/core/mjk_state_machine.c#L492-L510) | `492-510` | `State_5_MainLoop` (Paso 8) | Sincroniza `transmute_timer`/`transmute_streak`, ejecuta `MjkEnemy_UpdateAll`, evalúa `MjkEnemy_CheckJackCollisions`, mata a Jack si `col == -1` y copia `enemy_mgr.enemies` a `game->enemies`. | **PUNTO DE DESACTIVACIÓN 1**: Es el único lugar donde se generan y actualizan enemigos durante la partida activa (`GAME_STATE_5_MAIN_LOOP`). |
| [`src/core/mjk_state_machine.c`](../src/core/mjk_state_machine.c#L586-L590) | `586-590` | `State_5_MainLoop` (Paso 10) | Aplica `camera_apply_entity_delta_fixed` sobre `game->enemies[i].pos_x` y `pos_y`. | **PUNTO DE DESACTIVACIÓN 2**: Si no hay enemigos activos, el bucle es no-op, pero conviene protegerlo junto con el Paso 8. |
| [`src/core/mjk_game.c`](../src/core/mjk_game.c#L859-L889) | `859-889` | `RenderInGameScreen` (Paso 8) | Itera `game->enemies[0..7]` y llama a `MjkPPU_BlitSprite` para dibujar monedas transmutadas (`0x21`) o enemigos (`0x25..0x3D`). | **PUNTO DE DESACTIVACIÓN 3**: Blitter de sprites OAM de enemigos en el framebuffer. |

### 1.2 Análisis de Dependencias con la Suite de Tests Unitarios

1. **[`tests/test_enemy_ai.c`](../tests/test_enemy_ai.c#L11-L74)** (`líneas 11-74`) y **[`tests/test_enemy_spawns.c`](../tests/test_enemy_spawns.c#L15-L164)** (`líneas 15-164`):
   - Instancian localmente `MjkEnemyManager mgr` y `JackPlayer jack` y llaman **directamente** a `MjkEnemy_GetDifficulty`, `MjkEnemy_Init`, `MjkEnemy_InitAll`, `MjkEnemy_Spawn`, `MjkEnemy_TickSpawning`, `MjkEnemy_MutateMummy`, `MjkEnemy_ActivatePowerCoin`, `MjkEnemy_UpdateAll` y `MjkEnemy_CheckJackCollisions` definidos en [`src/core/mjk_enemy.c`](../src/core/mjk_enemy.c).
   - **No invocan** `MjkGame_SimulateFrame` ni `MjkGame_ExecuteState`.
   - Por consiguiente, **no se debe desactivar ni vaciar ninguna función dentro de [`src/core/mjk_enemy.c`](../src/core/mjk_enemy.c)**.
2. **[`tests/test_state_machine.c`](../tests/test_state_machine.c#L153)** (`línea 153`):
   - Verifica `assert(game.active_enemies_count == 0 && "Enemigos de sala anterior deben purgarse");` tras transicionar de Sala 1 a Sala 2.
3. **[`tests/test_powerups_mighty.c`](../tests/test_powerups_mighty.c#L186-L219)** (`líneas 186-219`):
   - Verifica `mjk_game_activate_power_coin`, `mjk_game_tick_power_coin` y `mjk_game_collect_transmuted_coin`.
   - Nota crítica: En [`src/core/mjk_state_machine.c:442`](../src/core/mjk_state_machine.c#L442), `State_5_MainLoop` ya llama a `mjk_game_tick_power_coin(game)` en el Paso 4. Cuando el Paso 8 (`líneas 493-497`) ejecutaba además `MjkEnemy_UpdateAll` (`mjk_enemy.c:171-173`), `transmute_timer` sufría un **doble decremento por frame** en `State_5_MainLoop`. Al desactivar el Paso 8 en `State_5_MainLoop`, `mjk_game_tick_power_coin(game)` en la línea 442 mantiene el decremento exacto de 1 tick por frame.

### 1.3 Estrategia Exacta de Desactivación Limpia para `TASK-F7-001`

Para desactivar por completo el spawneo, actualización, colisión y renderizado de enemigos durante el juego sin afectar ningún test unitario:
1. **Mantener intacto [`src/core/mjk_enemy.c`](../src/core/mjk_enemy.c)** para que `test_enemy_ai.c` y `test_enemy_spawns.c` sigan ejecutando el 100% de sus aserciones.
2. En **[`src/core/mjk_state_machine.c:492-510`](../src/core/mjk_state_machine.c#L492-L510)** (`State_5_MainLoop`, Paso 8):
   - Desactivar el bloque que invoca `MjkEnemy_UpdateAll` y `MjkEnemy_CheckJackCollisions` (p. ej. comentándolo o envolviéndolo en `#if 0 ... #endif` o bajo un flag `if (game->enemies_enabled)` inicializado a `false`), asegurando que `game->active_enemies_count = 0;`.
3. En **[`src/core/mjk_state_machine.c:586-590`](../src/core/mjk_state_machine.c#L586-L590)** (`State_5_MainLoop`, Paso 10) y **[`src/core/mjk_game.c:859-889`](../src/core/mjk_game.c#L859-L889)** (`RenderInGameScreen`, Paso 8):
   - Desactivar con la misma guarda el bucle de desplazamiento de cámara de enemigos y el bucle de renderizado de sprites de enemigos.

---

## 2. Punto B: Diagnóstico y Ground-Truth del Bug de Sprites de Números en Enemigos (`TASK-F7-010`)

### 2.1 Causa Raíz Exacta: Por qué `src/core/mjk_game.c` dibuja números en lugar de enemigos

En [`src/core/mjk_game.c:867-888`](../src/core/mjk_game.c#L867-L888), el renderizador actual ejecuta:

```c
        if (game->transmute_timer > 0) {
            // Moneda dorada parpadeante (prg.asm:8839 tile 0x21, subpaleta 2 con parpadeo al final)
            uint8_t subpal = (game->transmute_timer < 64) ? (uint8_t)((game->transmute_timer / 4) % 4) : 0x02;
            MjkOamSprite coin_sp = { (uint8_t)ey, 0x21, subpal, (uint8_t)ex };
            MjkPPU_BlitSprite(fb, chr_bank, &coin_sp, &pal_argb[16]);
        } else {
            // Sprites canónicos de enemigos (prg.asm:8235, 8882, 9208-9575)
            uint8_t tile_id = 0x25; // Momia por defecto (0x25)
            switch (game->enemies[i].type) {
                case ENEMY_TYPE_MUMMY:       tile_id = 0x25; break;
                case ENEMY_TYPE_HANEZO:      tile_id = 0x29; break;
                case ENEMY_TYPE_GEJI_SHOGUN: tile_id = 0x2D; break;
                case ENEMY_TYPE_GAMEIDO:     tile_id = 0x31; break;
                case ENEMY_TYPE_DOKURON:     tile_id = 0x35; break;
                case ENEMY_TYPE_DESUFA:      tile_id = 0x39; break;
                case ENEMY_TYPE_HORUS:       tile_id = 0x3D; break;
                default:                     tile_id = 0x25; break;
            }
            uint8_t attr = game->enemies[i].facing_left ? 0x40 : 0x00;
            MjkOamSprite enemy_sp = { (uint8_t)ey, tile_id, attr, (uint8_t)ex };
            MjkPPU_BlitSprite(fb, chr_bank, &enemy_sp, &pal_argb[16]);
        }
```

Existen **tres errores fundamentales** en este bloque:

1. **Confusión entre Índice de Metasprite (`SpritesTable`, 16×16 px = 4 sprites OAM de 8×8) y Tile ID crudo de CHR-ROM (8×8 px)**:
   - En el ensamblador original ([`asm/src/prg.asm:8405-8406`](../asm/src/prg.asm#L8405-L8406)), `MaybeDrawEnemySprites` invoca `CopyEnemyToTempSprite` ([`prg.asm:8570-8584`](../asm/src/prg.asm#L8570-L8584)) y acto seguido llama a **`WriteSprite`** ([`prg.asm:2267-2374`](../asm/src/prg.asm#L2267-L2374)).
   - En `WriteSprite`, dado que para todos los enemigos el bit 4 (`$10`) de `TempSpriteAttributesish` (`EnemyStruct_F`) es `0` ([`prg.asm:2272-2274`](../asm/src/prg.asm#L2272-L2274)), la rutina salta a `_WriteNormalSprite` ([`prg.asm:2293`](../asm/src/prg.asm#L2293)), multiplica `TempSpriteTile * 5` ([`prg.asm:2314-2323`](../asm/src/prg.asm#L2314-L2323)) e indexa la tabla de metasprites **`SpritesTable`** ([`asm/src/data.asm:364-411`](../asm/src/data.asm#L364-L411)), generando **4 sprites OAM de 8×8** (un metasprite de 16×16 px).
   - En cambio, `mjk_game.c` pasa `tile_id` directamente dentro de **un único `MjkOamSprite` de 8×8** a `MjkPPU_BlitSprite`.

2. **Malinterpretación de `LDA #$25` en `InitEnemy` (`prg.asm:8235`) e invención de la progresión `0x25 + 4*k`**:
   - En [`asm/src/prg.asm:8234-8236`](../asm/src/prg.asm#L8234-L8236) (`InitEnemy`) y [`prg.asm:8881-8883`](../asm/src/prg.asm#L8881-L8883) (`DrawEnemies`), el código 6502 escribe `LDA #$25; STA (EnemyStructPointer),Y` en `EnemyStruct_E_Sprite`.
   - Ese `$25` **no es el sprite de la Momia**, sino la entrada `$25` (decimal 37) de `SpritesTable` ([`asm/src/data.asm:402`](../asm/src/data.asm#L402): `.BYTE $E4, $8C, $8C, $8C, $8C`), que corresponde a la **nube de humo de aparición/transformación** mostrada durante los primeros 16 frames del temporizador `EnemyStruct_12`!
   - Los valores `0x29`, `0x2D`, `0x31`, `0x35`, `0x39`, `0x3D` en `mjk_game.c:877-882` fueron extrapolados sumando `+4` a `0x25` y no existen en ninguna parte de `prg.asm` ni `data.asm`.

3. **Qué contienen realmente los tiles CHR `0x21..0x3D` en el Banco 1 de `mbj-us.chr` (Pattern Table 0, `$0000-$0FFF`)**:
   - Tal como demuestra la rutina de popups de puntuación `sub_8FB1` ([`asm/src/prg.asm:2551-2573`](../asm/src/prg.asm#L2551-L2573), `ADC #$30; STA byte_30B; LDA #$30; STA byte_30C` y `ADC #$3C`), la fila `$30..$3F` de Pattern Table 0 contiene los **glifos numéricos de puntuación emergente** (`"00"`, `"1"`, `"2"`, `"3"`, `"5"`, `"8"`, `"10"`, `"20"`, `"30"`, `"x2"`, `"x3"`, `"x4"`).
   - Al inspeccionar directamente el binario [`asm/src/mbj-us.chr`](../asm/src/mbj-us.chr) (Banco 1, offset `8192`), los tiles que `mjk_game.c` estaba enviando a `MjkPPU_BlitSprite` contienen literalmente:

| Valor usado en `mjk_game.c` | Rol supuesto en `mjk_game.c` | Contenido REAL del Tile 8×8 en CHR-ROM (`mbj-us.chr` Banco 1, PT0) | Evidencia en `prg.asm` / `data.asm` |
| :---: | :--- | :--- | :--- |
| **`0x21`** | Moneda transmutada | Cuadrante inferior derecho (pierna derecha) del frame 2 de caminata de Jack | `SpritesTable[0x05]` ([`data.asm:370`](../asm/src/data.asm#L370): `.BYTE 0, $1C, $1D, $20, $21`) |
| **`0x25`** | `ENEMY_TYPE_MUMMY` | Bloque cuadrado sólido de 8×8 píxeles (color 3) | `SpritesTable[0x1E]` ([`data.asm:395`](../asm/src/data.asm#L395): `.BYTE 0, $24, $25, $26, $27`) |
| **`0x29`** | `ENEMY_TYPE_HANEZO` | Esquina inferior derecha de la moneda `E` (frame 0) | `SpritesTable[0x1B]` ([`data.asm:392`](../asm/src/data.asm#L392): `.BYTE 0, $22, $23, $28, $29`) |
| **`0x2D`** | `ENEMY_TYPE_GEJI_SHOGUN` | Esquina inferior derecha de la moneda `E` (frame 1) | `SpritesTable[0x1C]` ([`data.asm:393`](../asm/src/data.asm#L393): `.BYTE 0, $26, $27, $2C, $2D`) |
| **`0x31`** | `ENEMY_TYPE_GAMEIDO` | **Número `" 1"`** (centenas de popup `100` pts) | `sub_8FB1` ([`prg.asm:2553`](../asm/src/prg.asm#L2553): `ADC #$30` con `A=1`) |
| **`0x35`** | `ENEMY_TYPE_DOKURON` | **Número `" 8"`** (centenas de popup `800` pts) | `sub_8FB1` ([`prg.asm:2553`](../asm/src/prg.asm#L2553): `ADC #$30` con `A=5`) |
| **`0x39`** | `ENEMY_TYPE_DESUFA` | **Número `"30"`** (popup de puntuación) | Glifo numérico en fila `$30..$3F` de PT0 |
| **`0x3D`** | `ENEMY_TYPE_HORUS` | **Multiplicador `"x3"`** (popup de multiplicador en Bomb Room) | `sub_8FB1` ([`prg.asm:2567`](../asm/src/prg.asm#L2567): `ADC #$3C` con `Y=2`) |

---

### 2.2 Especificación Ground-Truth de Animaciones y Metasprites de Enemigos

#### A. Tabla de Animaciones por Tipo (`SpriteAnimationTable`, [`asm/src/data.asm:6-95`](../asm/src/data.asm#L6-L95))

En `DrawEnemies` ([`asm/src/prg.asm:8896-8974`](../asm/src/prg.asm#L8896-L8974)), cada entrada de animación consta de **3 bytes** (`{ duracion_ticks, metasprite_id, attr_base }`) seguidos de un byte centinela `$FF` que reinicia el cursor `EnemyStruct_10_AnimOffset` al valor de `EnemyStruct_1B` (o `EnemyStruct_D` en el caso de Horus, tipo 7). El atributo final `EnemyStruct_F` (`TempSpriteAttributesish`) se calcula como `attr_base | EnemyStruct_14` (donde `EnemyStruct_14` es `$00` mirando a la izquierda o `$04` mirando a la derecha, actualizado por `sub_BA9E` en [`prg.asm:9128-9140`](../asm/src/prg.asm#L9128-L9140) únicamente para la Momia).

| Tipo (`EnemyStruct_13_Type`) | Etiqueta en `data.asm` | Línea (`data.asm`) | Offset en Tabla | Duración (`+$11`) | `metasprite_id` (`+$0E` en `SpritesTable`) | `attr_base` | Subpaleta PPU (`attr & 3`) | Notas de Flipping y Bucle |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **Humo Spawn / Mutación** | *(Forzado en `DrawEnemies`)* | [`prg.asm:8882`](../asm/src/prg.asm#L8882) | — | `16` ticks (`$1F..$10`) | **`$25`** (37) | `$00` | Sprite Pal 0 | Activo mientras `(status & $08)` y `flash_timer >= $10`. |
| **Moneda Transmutada** | *(Forzado en `DrawEnemies`)* | [`prg.asm:8839`](../asm/src/prg.asm#L8839) | — | Mientras `EnemyCoinTimer > 0` | **`$21`** (33) | `$02` | Sprite Pal 2 (cicla `0..3` si `< 64f`) | Activo para tipos con `(status & $10) == 0`. |
| **`0`, `$D`, `$F` (Momia en tierra)** | `SpriteAnimation_0_D_F` | `L:29-32` | `0`<br>`3`<br>`6` | `5`<br>`5`<br>`$FF` | **`$0E`** (14)<br>**`$0F`** (15)<br>*(loop a 0)* | `$03`<br>`$03`<br>— | Sprite Pal 3 | `sub_BA9E` aplica `ORA EnemyStruct_14` (`$00` Izq, `$04` Der). Activado al aterrizar (`prg.asm:9294-9300`). |
| **`0` (Momia cayendo)** | `SpriteAnimation_1` | `L:33-36` | `7`<br>`10`<br>`13` | `5`<br>`5`<br>`$FF` | **`$10`** (16)<br>**`$11`** (17)<br>*(loop a 7)* | `$03`<br>`$03`<br>— | Sprite Pal 3 | `ObjInit_0_Mummy` (`prg.asm:9209-9213`) inicializa `EnemyStruct_1B = 7` y `AnimOffset = 7`. |
| **`2` (Hanezo)** | `SpriteAnimation_2` | `L:37-41` | `0`<br>`3`<br>`6`<br>`9` | `5`<br>`5`<br>`5`<br>`$FF` | **`$13`** (19)<br>**`$12`** (18)<br>**`$13`** (19)<br>*(loop a 0)* | `$02`<br>`$02`<br>`$06`<br>— | Sprite Pal 2 | El 3er paso tiene `attr_base = $06` (Subpaleta 2 + `$04` H-Flip). |
| **`3` (Geji Shogun)** | `SpriteAnimation_3` | `L:42-45` | `0`<br>`3`<br>`6` | `5`<br>`5`<br>`$FF` | **`$0C`** (12)<br>**`$0D`** (13)<br>*(loop a 0)* | `$02`<br>`$02`<br>— | Sprite Pal 2 | Alterna metasprites `$0C` y `$0D` cada 5 ticks. |
| **`4` (Gameido)** | `SpriteAnimation_4` | `L:46-50` | `0`<br>`3`<br>`6`<br>`9` | `5`<br>`5`<br>`5`<br>`$FF` | **`$16`** (22)<br>**`$17`** (23)<br>**`$18`** (24)<br>*(loop a 0)* | `$03`<br>`$03`<br>`$03`<br>— | Sprite Pal 3 | Ciclo de 3 frames (`$16 -> $17 -> $18`) de 5 ticks cada uno. |
| **`5` (Dokuron)** | `SpriteAnimation_5` | `L:51-54` | `0`<br>`3`<br>`6` | `5`<br>`5`<br>`$FF` | **`$14`** (20)<br>**`$15`** (21)<br>*(loop a 0)* | `$03`<br>`$03`<br>— | Sprite Pal 3 | Alterna metasprites `$14` y `$15` cada 5 ticks. |
| **`6` (Desufa)** | `SpriteAnimation_6` | `L:55-58` | `0`<br>`3`<br>`6` | `5`<br>`5`<br>`$FF` | **`$07`** (7)<br>**`$07`** (7)<br>*(loop a 0)* | `$00`<br>`$08`<br>— | Sprite Pal 0 | El 2º paso tiene `attr_base = $08` (Subpaleta 0 + `$08` V-Flip completo). |
| **`7` (Horus - Izq)** | `SpriteAnimation_7` | `L:60-62` | `0`<br>`3`<br>`6` | `5`<br>`5`<br>`$FF` | **`$08`** (8)<br>**`$09`** (9)<br>*(loop a `+$0D`=0)* | `$03`<br>`$03`<br>— | Sprite Pal 3 | Seleccionado por `sub_BEE7` (`prg.asm:9973`) cuando `byte_C7 == 2` (vuelo izquierda). |
| **`7` (Horus - Der)** | `SpriteAnimation_7` | `L:63-65` | `7`<br>`10`<br>`13` | `5`<br>`5`<br>`$FF` | **`$08`** (8)<br>**`$09`** (9)<br>*(loop a `+$0D`=7)* | `$07`<br>`$07`<br>— | Sprite Pal 3 | Seleccionado cuando `byte_C7 == 1` (vuelo derecha). `attr_base = $07` incluye `$04` H-Flip. |
| **`7` (Horus - Frente)** | `SpriteAnimation_7` | `L:66-68` | `14`<br>`17`<br>`20` | `5`<br>`5`<br>`$FF` | **`$0A`** (10)<br>**`$0B`** (11)<br>*(loop a `+$0D`=14)* | `$03`<br>`$03`<br>— | Sprite Pal 3 | Seleccionado cuando `byte_C7` es `4` o `8` (vuelo vertical arriba/abajo). |
| **`8` (Power Coin)** | `SpriteAnimation_8` | `L:69-71` | `0`<br>`3` | `5`<br>`$FF` | **`$20`** (32) | `$01` | Sprite Pal 1 | Moneda `P` coleccionable. |
| **`9` (Extra Coin)** | `SpriteAnimation_9` | `L:72-76` | `0..9` | `4, 4, 4` | **`$1B, $1C, $1D`** | `$00` | Sprite Pal 0 | Moneda `E` giratoria. |
| **`$A` (Secret Coin)** | `SpriteAnimation_A` | `L:77-79` | `0..3` | `0` | **`$1F`** (31) | `$00` | Sprite Pal 0 | Moneda `S`. |
| **`$B` (Captive)** | `SpriteAnimation_B` | `L:80-82` | `0..3` | `0` | **`$23`** (35) | `$03` | Sprite Pal 3 | Estatua de Faraón. |
| **`$C` (Balloon)** | `SpriteAnimation_C` | `L:83-85` | `0..3` | `0` | **`$24`** (36) | `$00` | Sprite Pal 0 | Globo especial. |
| **`$E` (Bonus Coin)** | `SpriteAnimation_E` | `L:86-90` | `0..9` | `4, 4, 4` | **`$19, $1A, $1D`** | `$00` | Sprite Pal 0 | Moneda `B` giratoria. |

#### B. Tabla Exacta de Metasprites 2×2 en `SpritesTable` ([`asm/src/data.asm:372-402`](../asm/src/data.asm#L372-L402))

Todos estos metasprites ya están presentes en el array `g_mjk_sprites_table[46][5]` en [`src/core/mjk_player.c:52-99`](../src/core/mjk_player.c#L52-L99):

| `metasprite_id` | Entidad / Frame | Línea (`data.asm`) | Byte 0 (`ControlByte`) | Tile Q1 (TL) | Tile Q2 (TR) | Tile Q3 (BL) | Tile Q4 (BR) |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`$07`** (7) | Desufa (bola de fuego) | `L:372` | `$00` | `$48` | `$49` | `$4A` | `$4B` |
| **`$08`** (8) | Horus lateral Frame 1 | `L:373` | `$00` | `$78` | `$79` | `$7A` | `$7B` |
| **`$09`** (9) | Horus lateral Frame 2 | `L:374` | `$00` | `$7C` | `$7D` | `$7E` | `$7F` |
| **`$0A`** (10) | Horus frontal Frame 1 | `L:375` | `$00` | `$D0` | `$D1` | `$D2` | `$D3` |
| **`$0B`** (11) | Horus frontal Frame 2 | `L:376` | `$00` | `$D4` | `$D5` | `$D6` | `$D7` |
| **`$0C`** (12) | Geji Shogun Frame 1 | `L:377` | `$00` | `$C0` | `$C1` | `$C2` | `$C3` |
| **`$0D`** (13) | Geji Shogun Frame 2 | `L:378` | `$00` | `$C4` | `$C5` | `$C6` | `$C7` |
| **`$0E`** (14) | Momia caminando Frame 1 | `L:379` | `$00` | `$70` | `$71` | `$72` | `$73` |
| **`$0F`** (15) | Momia caminando Frame 2 | `L:380` | `$00` | `$74` | `$75` | `$76` | `$77` |
| **`$10`** (16) | Momia cayendo Frame 1 | `L:381` | `$00` | `$60` | `$61` | `$62` | `$63` |
| **`$11`** (17) | Momia cayendo Frame 2 | `L:382` | `$00` | `$64` | `$65` | `$66` | `$67` |
| **`$12`** (18) | Hanezo Frame 2 (encogido) | `L:383` | `$00` | `$5C` | `$5D` | `$5E` | `$5F` |
| **`$13`** (19) | Hanezo Frame 1/3 (estirado) | `L:384` | `$00` | `$58` | `$59` | `$5A` | `$5B` |
| **`$14`** (20) | Dokuron Frame 1 | `L:385` | `$00` | `$68` | `$69` | `$6A` | `$6B` |
| **`$15`** (21) | Dokuron Frame 2 | `L:386` | `$00` | `$6C` | `$6D` | `$6E` | `$6F` |
| **`$16`** (22) | Gameido Frame 1 | `L:387` | `$00` | `$A8` | `$A9` | `$AA` | `$AB` |
| **`$17`** (23) | Gameido Frame 2 | `L:388` | `$00` | `$AC` | `$AD` | `$AE` | `$AF` |
| **`$18`** (24) | Gameido Frame 3 | `L:389` | `$00` | `$B8` | `$B9` | `$BA` | `$BB` |
| **`$20`** (32) | Power Coin (`P`) | `L:397` | `$00` | `$40` | `$41` | `$42` | `$43` |
| **`$21`** (33) | Moneda de Transmutación | `L:398` | `$44` | `$44` | `$44` | `$46` | `$46` |
| **`$25`** (37) | Humo de Spawn / Mutación | `L:402` | `$E4` | `$8C` | `$8C` | `$8C` | `$8C` |

#### C. Subpaletas de Sprites de Enemigos y `GoldCoinSpritePalette`

En `MainPalette` ([`asm/src/data.asm:264-267`](../asm/src/data.asm#L264-L267)):
- **Sprite Palette 0** (`PPU $3F10-$3F13`, `pal_argb[16..19]`): `.BYTE $21, $16, $16, $30` (Rojo y Blanco — usada por Desufa, Humo `$25`, monedas B/E/S).
- **Sprite Palette 1** (`PPU $3F14-$3F17`, `pal_argb[20..23]`): `.BYTE $21, $30, $12, $16` (Paleta de Jack y Power Coin `$20`).
- **Sprite Palette 2** (`PPU $3F18-$3F1B`, `pal_argb[24..27]`):
  - Por defecto en sala (`RestoreDefaultPalettesLimited #$18` en [`prg.asm:429-430`](../asm/src/prg.asm#L429-L430)): `.BYTE $21, $08, $16, $30` (Marrón oliva `$08`, Rojo `$16`, Blanco `$30` — usada por Hanezo y Geji Shogun).
  - Durante Power Coin (`TurnEnemiesIntoCoins` en [`prg.asm:9029-9048`](../asm/src/prg.asm#L9029-L9048), `GoldCoinSpritePalette`): sobrescribe los colores 1..3 de Sprite Palette 2 con `.BYTE $08, $26, $30` (cambiando el color 2 de `$16` Rojo a `$26` Dorado/Naranja). Cuando `EnemyCoinTimer` expira a 0 ([`prg.asm:1388-1391`](../asm/src/prg.asm#L1388-L1391)), llama a `LDX #$18; JSR RestoreDefaultPalettesLimited` para restaurar `$16`.
- **Sprite Palette 3** (`PPU $3F1C-$3F1F`, `pal_argb[28..31]`): `.BYTE $21, $0F, $2B, $30` (Negro `$0F`, Verde esmeralda `$2B`, Blanco `$30` — usada por Momia, Gameido, Dokuron y Horus).

#### D. Función Canónica de Ensamblado de Metasprites (`Mjk_AssembleMetasprite`)

La función `Jack_AssembleMetasprite` en [`src/core/mjk_player.c:584-648`](../src/core/mjk_player.c#L584-L648) ya implementa la lógica exacta de `_WriteNormalSprite` ([`asm/src/prg.asm:2293-2368`](../asm/src/prg.asm#L2293-L2368)), salvo que fija `uint8_t temp_attr = facing_left ? 0x01 : 0x05;`. Basta con exponer una variante general que reciba `uint8_t temp_attr` (`TempSpriteAttributesish` = `EnemyStruct_F`):

```c
void Mjk_AssembleMetaspriteWithAttr(uint8_t sprite_id, int16_t anchor_x, int16_t anchor_y,
                                    uint8_t temp_attr, MjkOamSprite out_sprites[4]) {
    if (!out_sprites) return;
    if (sprite_id >= 46) sprite_id = 0;
    const uint8_t* sprite_data = g_mjk_sprites_table[sprite_id];

    // prg.asm:2294-2301: bits 3-2 seleccionan el par en SpriteAttributeTable
    uint8_t x_idx = (uint8_t)((temp_attr & 0x0C) >> 1);
    uint8_t byte_4E = s_sprite_attribute_table[x_idx] ^ sprite_data[0];
    uint8_t byte_4F = s_sprite_attribute_table[x_idx + 1];
    uint8_t word_50 = (uint8_t)((temp_attr & 0x23) << 2);

    for (int i = 0; i < 4; ++i) {
        uint8_t carry_y = byte_4F & 0x01;
        byte_4F >>= 1;
        int y_pos = anchor_y - (carry_y ? 8 : 0);

        uint8_t tile = sprite_data[1 + i];

        uint8_t attr_reg = word_50;
        uint8_t c1 = byte_4E & 0x01; byte_4E >>= 1;
        attr_reg = (uint8_t)((attr_reg >> 1) | (c1 ? 0x80 : 0));
        uint8_t c2 = byte_4E & 0x01; byte_4E >>= 1;
        attr_reg = (uint8_t)((attr_reg >> 1) | (c2 ? 0x80 : 0));

        uint8_t carry_x = byte_4F & 0x01;
        byte_4F >>= 1;
        int x_pos = anchor_x - (carry_x ? 8 : 0);

        out_sprites[i].y = (uint8_t)y_pos;
        out_sprites[i].tile_id = tile;
        out_sprites[i].attributes = attr_reg;
        out_sprites[i].x = (uint8_t)x_pos;
    }
}
```

---

## 3. Punto C: Diagnóstico Línea por Línea del Movimiento Errático de Enemigos (`TASK-F7-010`)

El contraste exhaustivo entre [`src/core/mjk_enemy.c`](../src/core/mjk_enemy.c), [`src/core/mjk_state_machine.c`](../src/core/mjk_state_machine.c) y [`asm/src/prg.asm`](../asm/src/prg.asm) revela **7 discrepancias críticas** que provocan el movimiento errático de los enemigos:

### 3.1 Discrepancia 1: Pérdida del Delta de Cámara por `memcpy` Unidireccional (`mjk_state_machine.c:495, 508, 586-590`)
- **Código actual en C** ([`src/core/mjk_state_machine.c:495-590`](../src/core/mjk_state_machine.c#L495-L590)):
  1. En la línea 495, `MjkEnemy_UpdateAll(&game->enemy_mgr, ...)` actualiza `game->enemy_mgr.enemies`.
  2. En la línea 508, `memcpy(game->enemies, game->enemy_mgr.enemies, sizeof(game->enemies));` copia las posiciones a `game->enemies`.
  3. En las líneas 586-590, tras actualizar la cámara, `camera_apply_entity_delta_fixed` desplaza **únicamente `game->enemies[i].pos_x` y `pos_y`**, pero **NO actualiza `game->enemy_mgr.enemies[i]`**.
  4. En el frame siguiente, `MjkEnemy_UpdateAll` vuelve a ejecutarse sobre `game->enemy_mgr.enemies` (que jamás recibió el desplazamiento de cámara) y machaca `game->enemies` con `memcpy`.
- **Efecto visual**: Cuando Jack camina haciendo scroll en un pasillo o salta en una sala vertical, los enemigos son arrastrados con la cámara en lugar de permanecer sobre sus plataformas del mundo, dando tirones de 1 frame.
- **Ground-Truth 6502** ([`asm/src/prg.asm:8588-8656`](../asm/src/prg.asm#L8588-L8656), `sub_B7B4` / `sub_B7BF`):
  - En la NES existe un único array `Object0Struct..Object7Struct`. En cada frame, `sub_B7BF` suma ` Velocidad_Enemigo + Delta_Camara (byte_98/99, byte_9A/9B)` directamente sobre la posición Q8.8 de la entidad (`EnemyStruct_3_XPosLo`/`5_XPosHi` y `EnemyStruct_9_YPosLo`/`B_YPosHi`) y propaga el acarreo/préstamo a los bytes de desbordamiento de pantalla `EnemyStruct_6` (`offscreen_x`) y `EnemyStruct_C` (`offscreen_y`). Si `offscreen_x != 0` o `offscreen_y != 0`, `MaybeRemoveIfOffscreen` ([`prg.asm:8538-8566`](../asm/src/prg.asm#L8538-L8566)) elimina inmediatamente al enemigo (`RemoveEnemy`).

### 3.2 Discrepancia 2: Truncamiento Q8.8 y Velocidades Enteras Desproporcionadas (`mjk_enemy.c:90-93, 189-309`)
- **Código actual en C** ([`src/core/mjk_enemy.c:189-190, 241-242`](../src/core/mjk_enemy.c#L189-L242)):
  - Extrae `int ex = FIXED_TO_INT(e->pos_x); int ey = FIXED_TO_INT(e->pos_y);`, opera con enteros (`e->vel_x = ±1`, `ex += e->vel_x * 2`, `ey += 2`) y sobrescribe `e->pos_x = FIXED_FROM_INT(ex);`, **borrando el byte fraccionario Q8.8 en cada frame**.
- **Ground-Truth 6502**:
  - **Momia (`sub_BB39`, [`prg.asm:9245-9262`](../asm/src/prg.asm#L9245-L9262))**: Su velocidad horizontal es fija de **`±0x0080` (`±0.5` px/frame)** (`XVelLo = $80`, `XVelHi = $00` o `$FF`), dirigida hacia Jack en el instante de tocar suelo. Además, mientras cae tras hacer spawn (`sub_BB1C`, [`prg.asm:9224-9240`](../asm/src/prg.asm#L9224-L9240)), su `XVel` es **`0`** (cae verticalmente usando la mitad descendente de la parábola `word_C0E3` vía `DoSomethingWithObjYVel`, [`prg.asm:9316`](../asm/src/prg.asm#L9316)). En `mjk_enemy.c`, la momia se mueve al doble de velocidad (`1.0` px/frame) y camina en el aire mientras cae.
  - **Enemigos mutados (`word_C0D3`, [`data.asm:178-186`](../asm/src/data.asm#L178-L186), `sub_B765` en [`prg.asm:8524-8534`](../asm/src/prg.asm#L8524-L8534))**:
    - Las velocidades Q8.8 dependen de `EnemySpeedModifier` (`0..7`):
      `word_C0D3 = { 0x0058, 0x0070, 0x0088, 0x00A0, 0x00B8, 0x00D0, 0x00E8, 0x0100 }`.
    - En Ronda 1/2 (`speed_modifier = 0`), la velocidad canónica es **`0x0058` (`0.34375` px/frame)**. En `mjk_enemy.c`, Hanezo se mueve a `2.0` px/frame (`0x0200`, **5.8× más rápido**) y el resto a `1.0` px/frame (`0x0100`, **2.9× más rápido**).

### 3.3 Discrepancia 3: Spawneo Erróneo en Sala 1 y Tabla de Spawn Incompleta (`mjk_enemy.c:38-47, 121-141`)
- **Código actual en C** ([`src/core/mjk_enemy.c:38-47, 129-140`](../src/core/mjk_enemy.c#L38-L140)):
  - `k_enemy_spawn_pos_table` solo define 8 salas (0..7) de las 256 salas del juego.
  - Cuando `coord_byte == 0x00` (como ocurre en la Sala 1: `{ 0x00, 0x00 }`), `mjk_enemy.c:133-135` hace un fallback inventado y **spawnea enemigos en `(224, 176)` y `(16, 176)`**.
- **Ground-Truth 6502** ([`asm/src/prg.asm:8689-8824`](../asm/src/prg.asm#L8689-L8824), `ChooseEnemySpawnPoint`, y [`asm/src/data.asm:1963-2220`](../asm/src/data.asm#L1963-L2220), `EnemySpawnPositionTable`):
  - En `ChooseEnemySpawnPoint` ([`prg.asm:8704-8710`](../asm/src/prg.asm#L8704-L8710)):
    ```asm
    	LDY #0				; Load the first byte
    	LDA (word_50),Y			; If the first byte is 00
    	BNE _ContinueSpawn		; un-spawn and exit
    _AbortSpawn:
    	JSR RemoveEnemy			; removes from ptr $07A
    	RTS
    ```
  - **En la NES, cuando el primer byte de `EnemySpawnPositionTable[room]` es `$00` (como en la Sala 1, [`data.asm:1965`](../asm/src/data.asm#L1965)), la sala NO genera enemigos (`_AbortSpawn`)**. En la Ronda 1, los enemigos solo empiezan a generarse cuando Jack avanza hacia la derecha y entra en la Sala 2 (`CurrentRoomIDBackup3 = 2`, cuyos bytes de spawn son `$B1, $4A`, [`data.asm:1966`](../asm/src/data.asm#L1966)).
  - Además, `SpawnLocFunc1` ([`prg.asm:8758-8788`](../asm/src/prg.asm#L8758-L8788)) y `sub_9276` ([`prg.asm:2988-3018`](../asm/src/prg.asm#L2988-L3018)) convierten el byte `$YX` a coordenadas de pantalla centradas en el metatile (`+8` px) restando el scroll actual (`byte_3C2`, `byte_3C4`), descartan puntos fuera de la media pantalla visible y `SpawnLocFunc2` ([`prg.asm:8794-8824`](../asm/src/prg.asm#L8794-L8824)) elige entre los dos puntos según la distancia Manhattan `|PlayerX - SpawnX| + |PlayerY - SpawnY|` (el más lejano en salas `RF_SingleScreen`, o el más cercano en salas con scroll).

### 3.4 Discrepancia 4: Omisión del Estado de Inmovilidad / Humo de 32 Ticks (`OrEnemyStatusWith08`, `prg.asm:8313-8321, 8605-8613, 9268-9272`)
- **Ground-Truth 6502**:
  - Tanto al aparecer (`_SpawnEnemy`, [`prg.asm:8216`](../asm/src/prg.asm#L8216)) como al mutar (`TransformMummy`, [`prg.asm:9374`](../asm/src/prg.asm#L9374)), `OrEnemyStatusWith08` activa `status |= $08` y `EnemyStruct_12 = $20` (32 ticks).
  - Mientras `status & $08` está activo, `sub_B7BF` ([`prg.asm:8605-8613`](../asm/src/prg.asm#L8605-L8613)) anula la velocidad del enemigo (solo aplica el delta de cámara), `ObjCode_0_Mummy` ([`prg.asm:9268-9272`](../asm/src/prg.asm#L9268-L9272)) no ejecuta lógica ni decrementa `transform_timer`, y `CheckObjectCollision` ([`prg.asm:3981-3982`](../asm/src/prg.asm#L3981-L3982)) desactiva el daño letal contra Jack.
- **Código actual en C**: Carece de `flash_timer` (`EnemyStruct_12`), por lo que los enemigos se mueven y matan desde el primer frame.

### 3.5 Discrepancia 5: Colisión con Escenario Ignorada en los 6 Enemigos Mutados y Defectuosa en la Momia (`mjk_enemy.c:197-309` vs `sub_9813`/`sub_9965` en `prg.asm:3913-3955, 4152-4226`)
- **Código actual en C**:
  - Los 6 enemigos mutados (`HANEZO`, `GEJI_SHOGUN`, `GAMEIDO`, `DOKURON`, `DESUFA`, `HORUS`) en [`src/core/mjk_enemy.c:255-309`](../src/core/mjk_enemy.c#L255-L309) **no consultan `room_metatiles` en absoluto**: atraviesan todos los muros, suelos y plataformas del escenario y solo rebotan en límites fijos (`16..224`, `24..208`).
  - La Momia ([`src/core/mjk_enemy.c:208-239`](../src/core/mjk_enemy.c#L208-L239)) consulta `room_metatiles` usando coordenadas de pantalla sin sumar `scroll_x`/`scroll_y`, considera sólido cualquier metatile `!= 0x00` (tratando bombas `0x27` y huecos de ítems recogidos `0x2F` como muros sólidos, e ignorando los bloques semisolidos de 8×8 `0x38..0x4B` de `byte_A3EA`), y al caer de una plataforma sigue avanzando en X.
- **Ground-Truth 6502**:
  - `sub_9813` ([`prg.asm:3913-3955`](../asm/src/prg.asm#L3913-L3955)) ejecuta `sub_9965` ([`prg.asm:4152-4226`](../asm/src/prg.asm#L4152-L4226)) para los enemigos usando las **mismas sondas espaciales `byte_A3D8`** (centradas en `(XPosHi, YPosHi)` + `scroll`) y la **misma tabla de solidez de metatiles** (`loc_9BBB` / `byte_A3EA`, [`prg.asm:4532-4610`](../asm/src/prg.asm#L4532-L4610)) que Jack:
    - Par vertical: sondas 1 y 2 (`(-4,-8), (+4,-8)` -> bit 3 Techo) si `YVelHi < 0`; sondas 3 y 4 (`(-4,+8), (+4,+8)` -> bit 2 Suelo) si `YVelHi >= 0`.
    - Par horizontal: sondas 5 y 6 (`(-8,-4), (-8,+4)` -> bit 1 Pared Izq) si `XVelHi < 0`; sondas 7 y 8 (`(+8,-4), (+8,+4)` -> bit 0 Pared Der) si `XVelHi >= 0`.
  - Si el bit 2 (Suelo) se activa y el enemigo tenía `status & 2` (cayendo), `sub_BA4E` ([`prg.asm:9060-9085`](../asm/src/prg.asm#L9060-L9085)) limpia `status & ~2`, pone `YVel = 0` y alinea `YPosHi &= $F8`.
  - Además, en `ObjCode_0_Mummy` ([`prg.asm:9276-9283, 9340-9359`](../asm/src/prg.asm#L9340-L9359)), una vez que la Momia ha aterrizado en una plataforma (`EnemyStruct_1A & 1`), cuando sus sondas inferiores dejan de tocar suelo al llegar al borde de una cornisa (`TempPointer_0C0+1 & 4 == 0` en `loc_BBCF`) o cuando toca una pared (`TempPointer_0C0+1 & 3 != 0` en `loc_BBD6`), **invierte su velocidad horizontal (`sub_BA48`) usando el bit 1 de `EnemyStruct_1A` como flag antirrebotes**, patrullando sobre su plataforma sin tirarse al vacío.

### 3.6 Discrepancia 6: Algoritmos de IA de los 6 Arquetipos Mutados (`ObjCode_2`..`ObjCode_7`, `prg.asm:9382-10160`)

| Tipo | Arquetipo | Implementación Actual en `mjk_enemy.c` | Comportamiento Real 6502 en `prg.asm` |
| :---: | :--- | :--- | :--- |
| **`2`** | **Hanezo** | Vuelo horizontal a `vel_x * 2` + oscilación vertical `±1` px cada 8 frames (`L:255-270`). | **`ObjInit_2_Hanezo` / `ObjCode_2_Hanezo` (`prg.asm:9382-9396`)**: Rebote diagonal elástico a velocidad `word_C0D3[speed_mod]` en X e Y; invierte `XVel` con `sub_BD64` al chocar con paredes (`bits 0-1`) e invierte `YVel` con `sub_BD81` al chocar con suelo/techo (`bits 2-3`). |
| **`3`** | **Geji Shogun** | Cae en `default:` (`ex += vel_x` entre 16 y 224) (`L:299-308`). | **`ObjInitCode_3a` (`prg.asm:9881-9922`)**: Persigue a Jack en X (`sub_BDA0`) rebotando en muros; cuando el timer `EnemyStruct_D` llega a 0 o toca una superficie (`TempPointer_0C0+1 & $0F != 0`), reorienta `YVel` hacia Jack (`sub_BDC7`) y recarga `EnemyStruct_D = $30` (48 ticks). |
| **`4`** | **Gameido** | Rebote diagonal fijo a `±1` px/frame en bordes de pantalla (`L:273-284`). | **`ObjInit_4_Gameido` / `ObjCode_4a` (`prg.asm:9407-9426, 9608-9658`)**: Al iniciar (`TempPointer_0C0+1 = 2`) y cada vez que choca con un obstáculo (`TempPointer_0C0+1 != 0`), recalcula velocidades proporcionales a la distancia a Jack: `Vel = (|PlayerPos - EnemyPos| * 256) / byte_C0CB[speed_mod]` (`sub_BDE9`), invirtiendo el componente bloqueado por la pared/suelo/techo. |
| **`5`** | **Dokuron** | Rebote diagonal fijo a `±1` px/frame en bordes de pantalla (`L:274-284`). | **`ObjInit_5_Dokuron` / `ObjCode_5_Dokuron` (`prg.asm:9430-9442, 9716-9747`)**: Rebota verticalmente contra suelo y techo (`sub_BD81`), mientras en X (`ObjCode_5a` -> `sub_BE2F` + `sub_BDA0`) persigue un punto que alterna cada 32 ticks entre `PlayerXPosHi + 16` y `PlayerXPosHi - 16`. |
| **`6`** | **Desufa** | Persecución directa `ex ± 1`, `ey ± 1` hacia Jack sin colisiones (`L:286-297`). | **`ObjInit_6_Desufa` / `ObjCode_6_Desufa` (`prg.asm:9446-9458, 9751-9782`)**: Rebota horizontalmente contra paredes (`sub_BD64`), mientras en Y (`sub_BDC2` -> `sub_BE2F` + `sub_BDC7`) persigue un punto que alterna cada 32 ticks entre `PlayerYPosHi + 16` y `PlayerYPosHi - 16`. |
| **`7`** | **Horus** | Cae en `default:` (`ex += vel_x` entre 16 y 224) (`L:299-308`). | **`ObjInit_7_Horus` / `ObjCode_7a` (`prg.asm:9462-9474, 9926-10160`)**: Cada 16 frames (`Obj7_Timer = $10`) o al chocar con un obstáculo (`TempPointer_0C0+1 != 0`), compara `|ΔX|` vs `|ΔY|` respecto a Jack, elige la dirección cardinal prioritaria libre de colisión (sin dar media vuelta de 180° salvo bloqueo), anula la velocidad del eje perpendicular (`ResetEitherXOrYVelocity`) y actualiza el sub-bucle de animación (`0` Izq, `7` Der, `14` Frente) en `sub_BEE7`. |

### 3.7 Discrepancia 7: Caja de Colisión Jugador-Enemigo y Comportamiento con Mighty Nivel 3 (`CheckObjectCollision`, `prg.asm:3959-4028` vs `mjk_enemy.c:313-368`)
- En [`asm/src/prg.asm:3959-3970, 4136-4148`](../asm/src/prg.asm#L3959-L3970) (`CheckObjectCollision` / `CheckIfPlayerWithin8Pixels`), hay colisión si y solo si `|PlayerXPosHi - EnemyXPosHi| < 8` **y** `|PlayerYPosHi - EnemyYPosHi| < 8` (distancias entre centros `< 8` px), siempre que `EnemyStruct_12 == 0` (no esté en los 32 frames de humo/parpadeo de aparición).
- En [`asm/src/prg.asm:3975-3990`](../asm/src/prg.asm#L3975-L3990), cuando Jack alcanza el Nivel Mighty 3 mediante monedas Mighty (`UseMightyCoin`, [`prg.asm:1761-1765`](../asm/src/prg.asm#L1761-L1765)), llama a `TurnEnemiesIntoCoins` (`EnemyCoinTimer = 320`). Una vez que esos 320 frames expiran, aunque `PlayerMightyLevel` siga siendo `3`, **tocar un enemigo no transmutado (`status & $30 == 0`) ES LETAL para Jack** (`PlayerStruct |= $80`, [`prg.asm:3985-3987`](../asm/src/prg.asm#L3985-L3987)). (Nota: dado que [`tests/test_enemy_spawns.c:152-160`](../tests/test_enemy_spawns.c#L152-L160) prueba explícitamente `MjkEnemy_CheckJackCollisions` con `jack.power_level = MIGHTY_POWER_GREEN` esperando `500`, `MjkEnemy_CheckJackCollisions` debe conservar esa rama para compatibilidad con el test unitario).
