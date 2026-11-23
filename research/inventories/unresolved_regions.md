# Clasificación de Regiones Ambiguas y Rutinas Huérfanas

- **Fase**: Fase 1 — Inventario Estático Exhaustivo
- **Subrutinas con 0 Llamadores Directos**: 7
- **Subrutinas con Manipulación de Pila (PLA; PLA)**: 16
- **Subrutinas Anónimas (sub_XXXX)**: 59

## 1. Análisis de Subrutinas con 0 Llamadores Directos

| Subrutina | Dirección | Archivo:Línea | Destino JumpTable | Vector Hardware | Clasificación Epistémica |
| :--- | :---: | :---: | :---: | :---: | :--- |
| `RESET` | `UNKNOWN` | `prg.asm:8` | False | True | [CONFIRMADA] Vector HW |
| `NMI` | `$8073` | `prg.asm:91` | False | True | [CONFIRMADA] Vector HW |
| `sub_9A7A` | `$999F` | `prg.asm:4331` | False | False | [INFERIDA] Código Muerto / Entrada Alternativa |
| `unusedMaybe_A248` | `$A141` | `prg.asm:5613` | False | False | [INFERIDA] Código Muerto / Entrada Alternativa |
| `unusedMaybe_A25C` | `$A155` | `prg.asm:5630` | False | False | [INFERIDA] Código Muerto / Entrada Alternativa |
| `DoorCond_unused` | `UNKNOWN` | `prg.asm:6818` | False | False | [INFERIDA] Código Muerto / Entrada Alternativa |
| `HandleEnemySpawnTimer` | `$B43D` | `prg.asm:8171` | False | False | [INFERIDA] Código Muerto / Entrada Alternativa |

## 2. Subrutinas con Doble Retorno (Manipulación de Pila `PLA; PLA`)

Estas subrutinas modifican el puntero de pila para descartar la dirección de retorno de su llamador y forzar una salida hacia el nivel superior:

| Subrutina | Dirección | Archivo:Línea | Efecto de Pila | Comportamiento Asociado |
| :--- | :---: | :---: | :---: | :--- |
| `GameState_7_GoToTortureRoom` | `$8465` | `prg.asm:698` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `MainSub_1` | `$881E` | `prg.asm:1304` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `AnimateDoorClose` | `$8D1C` | `prg.asm:2165` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `JumpTable` | `$8D2B` | `prg.asm:2180` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `CalculateModulus` | `$8D4A` | `prg.asm:2207` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `IfNotTitleScreenRoomDoThings` | `$9284` | `prg.asm:3077` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `sub_9592` | `$94E6` | `prg.asm:3541` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `sub_96F5` | `$9628` | `prg.asm:3746` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `sub_9B6A` | `$9A8F` | `prg.asm:4480` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `sub_A875` | `$A72D` | `prg.asm:6553` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `MaybeDontOpenDoors` | `$A926` | `prg.asm:7010` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `MaybeConsiderOffscreen` | `$B64E` | `prg.asm:8548` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `sub_BA00` | `$B8CD` | `prg.asm:8995` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `SoundCommand_Cx` | `$B1BE` | `music-engine.asm:355` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `SoundCommand_F3` | `$B27B` | `music-engine.asm:515` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |
| `SoundCommand_F6_F7` | `$B2FF` | `music-engine.asm:605` | `PLA; PLA` (Stack Pop) | Aborta secuencia actual y retorna al controlador superior |

## 3. Subrutinas Anónimas Pendientes de Nombre Semántico Formal

Existen 59 subrutinas nombradas como `sub_XXXX` provenientes del desensamblado inicial de IDA Pro. Su semántica se formaliza en los contratos de la Fase 3:

| Etiqueta Anónima | Dirección | Archivo:Línea | Llamadores | Llamados |
| :--- | :---: | :---: | :---: | :---: |
| `sub_8AE8` | `$8AB9` | `prg.asm:1689` | 2 | 1 |
| `sub_8FB1` | `$8F55` | `prg.asm:2551` | 1 | 0 |
| `sub_9276` | `$920A` | `prg.asm:2988` | 4 | 0 |
| `sub_92AD` | `$923A` | `prg.asm:3022` | 4 | 1 |
| `sub_92E2` | `$926E` | `prg.asm:3058` | 2 | 0 |
| `sub_9592` | `$94E6` | `prg.asm:3541` | 5 | 0 |
| `sub_9696` | `$95CC` | `prg.asm:3684` | 1 | 3 |
| `sub_96B7` | `$95EB` | `prg.asm:3703` | 1 | 5 |
| `sub_96F5` | `$9628` | `prg.asm:3746` | 2 | 1 |
| `sub_9716` | `$9647` | `prg.asm:3773` | 5 | 1 |
| `sub_9729` | `$965A` | `prg.asm:3789` | 3 | 0 |
| `sub_9813` | `$9741` | `prg.asm:3913` | 1 | 3 |
| `sub_9965` | `$988F` | `prg.asm:4152` | 2 | 1 |
| `sub_99EA` | `$990F` | `prg.asm:4230` | 2 | 1 |
| `sub_9A03` | `$9928` | `prg.asm:4251` | 1 | 4 |
| `sub_9A7A` | `$999F` | `prg.asm:4331` | 0 | 4 |
| `sub_9B39` | `$9A5E` | `prg.asm:4448` | 2 | 1 |
| `sub_9B6A` | `$9A8F` | `prg.asm:4480` | 2 | 0 |
| `sub_A18E` | `$A087` | `prg.asm:5499` | 1 | 1 |
| `sub_A1A6` | `$A09F` | `prg.asm:5515` | 1 | 2 |
| `sub_A1B9` | `$A0B2` | `prg.asm:5529` | 1 | 0 |
| `sub_A1EB` | `$A0E4` | `prg.asm:5559` | 1 | 0 |
| `sub_A270` | `$A169` | `prg.asm:5647` | 2 | 2 |
| `sub_A29C` | `$A191` | `prg.asm:5671` | 1 | 5 |
| `sub_A32B` | `$A21B` | `prg.asm:5751` | 1 | 1 |
| `sub_A33E` | `$A22E` | `prg.asm:5768` | 1 | 0 |
| `sub_A843` | `$A6FB` | `prg.asm:6516` | 1 | 1 |
| `sub_A851` | `$A709` | `prg.asm:6527` | 1 | 1 |
| `sub_A85F` | `$A717` | `prg.asm:6538` | 1 | 3 |
| `sub_A875` | `$A72D` | `prg.asm:6553` | 1 | 3 |
| `sub_AFA9` | `$AE7A` | `prg.asm:7928` | 1 | 0 |
| `sub_B024` | `$AEF5` | `prg.asm:7998` | 1 | 2 |
| `sub_B080` | `$AF51` | `prg.asm:8058` | 1 | 1 |
| `sub_B094` | `$AF65` | `prg.asm:8076` | 1 | 0 |
| `sub_B0AF` | `$AF80` | `prg.asm:8096` | 3 | 4 |
| `sub_B645` | `$B512` | `prg.asm:8326` | 1 | 5 |
| `sub_B765` | `$B632` | `prg.asm:8524` | 10 | 0 |
| `sub_B7B4` | `$B681` | `prg.asm:8588` | 1 | 1 |
| `sub_B7BF` | `$B68C` | `prg.asm:8598` | 1 | 0 |
| `sub_BA00` | `$B8CD` | `prg.asm:8995` | 4 | 1 |
| `sub_BA48` | `$B915` | `prg.asm:9052` | 1 | 1 |
| `sub_BA4E` | `$B91B` | `prg.asm:9060` | 1 | 1 |
| `sub_BA7B` | `$B948` | `prg.asm:9092` | 1 | 1 |
| `sub_BA9E` | `$B96B` | `prg.asm:9128` | 1 | 0 |
| `sub_BB1C` | `$B9E9` | `prg.asm:9224` | 3 | 1 |
| `sub_BB39` | `$BA06` | `prg.asm:9245` | 2 | 0 |
| `sub_BD64` | `$BC31` | `prg.asm:9669` | 1 | 3 |
| `sub_BD81` | `$BC4E` | `prg.asm:9696` | 1 | 3 |
| `sub_BDA0` | `$BC6D` | `prg.asm:9723` | 1 | 3 |
| `sub_BDC2` | `$BC8F` | `prg.asm:9751` | 1 | 1 |
| `sub_BDC7` | `$BC94` | `prg.asm:9758` | 1 | 3 |
| `sub_BDE9` | `$BCB6` | `prg.asm:9788` | 1 | 1 |
| `sub_BE2F` | `$BCFC` | `prg.asm:9845` | 2 | 0 |
| `sub_BEE7` | `$BDB4` | `prg.asm:9973` | 1 | 0 |
| `sub_BF17` | `$BDE4` | `prg.asm:10017` | 1 | 0 |
| `sub_BF4E` | `$BE1B` | `prg.asm:10053` | 1 | 0 |
| `sub_BF85` | `$BE52` | `prg.asm:10089` | 1 | 0 |
| `sub_BFBC` | `$BE89` | `prg.asm:10130` | 1 | 1 |
| `sub_B285` | `$B153` | `music-engine.asm:266` | 2 | 1 |
