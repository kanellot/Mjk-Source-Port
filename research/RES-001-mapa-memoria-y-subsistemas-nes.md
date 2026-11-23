# Informe Técnico: Mapa de Memoria RAM, Punteros Base y Enumeraciones de la NES

- **ID de Investigación**: `RES-001`
- **Tarea Asociada**: [`TASK-SETUP-003`](file:///c:/Users/kanel/Documents/mjk/TASKS.md#task-setup-003-estructura-permanente-de-investigación-técnica-e-índice-de-hallazgos)
- **Área Temática**: 01 - Núcleo, Memoria y Arquitectura General
- **Fecha**: 2026-10-22
- **Autor / Agente**: Antigravity (Orchestrator)
- **Estado**: `[COMPLETADA]`

---

## 1. Objetivo y Alcance

Analizar y documentar de forma estructurada las definiciones maestras de registros de hardware, mapeo de memoria RAM, punteros base a tablas ROM y enumeraciones de subsistemas extraídas del código original del cartucho NES (*Mighty Bomb Jack*, Tecmo 1987).

Este informe sirve como catálogo de constantes e invariantes arquitectónicas para asegurar que las estructuras de datos en `port_native/src/` coincidan exactamente con la lógica original.

---

## 2. Fuentes de Verdad Analizadas (`asm/`)

- **Archivo Principal**: `asm/src/defs.asm` (Líneas 1 a 300)
- **Archivos Complementarios**: `asm/src/data.asm` (Líneas 1 a 100), `asm/mighty-bomb-jack.asm`
- **Plataforma Original**: NES / Famicom (Ricoh 2A03 / N2A03 a 1.789773 MHz NTSC)

---

## 3. Hallazgos Técnicos y Extracción de Datos

### 3.1. Registros de Hardware PPU y Entrada del Gamepad

#### PPU Control y Máscaras (`PPUCtrl`, `PPUMask`, `PPUStatus`)
| Registro / Símbolo | Valor Hex | Bitmask Binario | Función en NES |
| :--- | :--- | :--- | :--- |
| `PPUCtrl_BaseAddr2000` | `$00` | `%00` | Nametable base en $2000 |
| `PPUCtrl_BaseAddr2400` | `$01` | `%01` | Nametable base en $2400 |
| `PPUCtrl_SpriteSize8x8` | `$00` | `%00000000` | Sprites estándar 8x8 px |
| `PPUCtrl_SpriteSize8x16` | `$20` | `%00100000` | Sprites dobles 8x16 px |
| `PPUControl_NMIEnabled` | `$80` | `%10000000` | Habilita interrupción NMI en VBlank |
| `PPUMask_ShowBackground` | `$08` | `%00001000` | Renderizado de capa de fondo |
| `PPUMask_ShowSprites` | `$10` | `%00010000` | Renderizado de capa de sprites |
| `PPUStatus_VBlank` | `$80` | `%10000000` | Flag de sincronismo vertical |

#### Joypad (Mando 1)
| Botón | Valor Hex | Máscara de Bits |
| :--- | :--- | :--- |
| `JP_Right` | `$01` | `%00000001` |
| `JP_Left` | `$02` | `%00000010` |
| `JP_Down` | `$04` | `%00000100` |
| `JP_Up` | `$08` | `%00001000` |
| `JP_Start` | `$10` | `%00010000` |
| `JP_Select` | `$20` | `%00100000` |
| `JP_B` | `$40` | `%01000000` |
| `JP_A` | `$80` | `%10000000` |

### 3.2. Estructura de Entidades enemigas (`EnemyStruct`)

En el motor original, cada slot de enemigo ocupa 28 bytes (`$1C`):

```
+--------------------------------------------------------------------------------------------------+
| Offset | Símbolo ASM                | Tamaño | Descripción técnica                                |
+--------+----------------------------+--------+----------------------------------------------------+
| $00    | EnemyStruct_0_Status       | Byte   | Estado del enemigo (activo, congelado, moneda)     |
| $01    | EnemyStruct_1              | Byte   | Timer o subtipo de comportamiento                  |
| $02    | EnemyStruct_2_XVelLo       | Byte   | Subpíxel / Velocidad X (Byte bajo)                 |
| $03    | EnemyStruct_3_XPosLo       | Byte   | Posición horizontal X (Subpíxeles)                 |
| $04    | EnemyStruct_4_XVelHi       | Byte   | Velocidad X entera                                 |
| $05    | EnemyStruct_5_XPosHi       | Byte   | Posición horizontal X (Píxeles de pantalla)        |
| $06    | EnemyStruct_6              | Byte   | Coordenada X fuera de pantalla                     |
| $07    | EnemyStruct_7              | Byte   | Rango de detección de distancia a Jack             |
| $08    | EnemyStruct_8_YVelLo       | Byte   | Subpíxel / Velocidad Y (Byte bajo)                 |
| $09    | EnemyStruct_9_YPosLo       | Byte   | Posición vertical Y (Subpíxeles)                   |
| $0A    | EnemyStruct_A_YVelHi       | Byte   | Velocidad Y entera                                 |
| $0B    | EnemyStruct_B_YPosHi       | Byte   | Posición vertical Y (Píxeles de pantalla)          |
| $0C    | EnemyStruct_C              | Byte   | Coordenada Y fuera de pantalla                     |
| $0D    | EnemyStruct_D              | Byte   | Estado de colisión o rebote                        |
| $0E    | EnemyStruct_E_Sprite       | Byte   | Índice de sprite asignado en OAM                   |
| $0F    | EnemyStruct_F              | Byte   | Flags de animación / flip de sprite                |
| $10    | EnemyStruct_10_AnimOffset  | Byte   | Offset en la tabla de animación activa             |
| $13    | EnemyStruct_13_Type        | Byte   | Tipo de enemigo (0-7: móvil, 8+: items/especial)   |
| $16    | EnemyStruct_SpeedMod       | Byte   | Modificador de velocidad según nivel/dificultad    |
+--------------------------------------------------------------------------------------------------+
```

### 3.3. Catálogo de Tipos de Enemigos (`EnemyType`)

| ID Hex | Símbolo ASM | Nombre en el Juego / Comportamiento |
| :--- | :--- | :--- |
| `$00` | `EnemyType_0_Mummy` | Momia estándar (terrestre, rebota en paredes) |
| `$01` | `EnemyType_1_Unknown` | Variante momia / spawn pasivo |
| `$02` | `EnemyType_2_Hanezo` | Bola roja saltarina (rebote diagonal tipo DVD) |
| `$03` | `EnemyType_3_GejiShogun` | Ciempiés / Bicho rojo que persigue activamente |
| `$04` | `EnemyType_4_Gameido` | Tortuga verde |
| `$05` | `EnemyType_5_Dokuron` | Calavera voladora |
| `$06` | `EnemyType_6_Desufa` | Bola de fuego roja errática |
| `$07` | `EnemyType_7_Horus` | Pájaro verde de vuelo rasante |
| `$08` | `EnemyType_8_PowerCoin` | Moneda de poder (transforma enemigos en monedas) |
| `$09` | `EnemyType_9_ExtraCoin` | Moneda extra (suma de puntos) |
| `$0A` | `EnemyType_A_SecretCoin` | Moneda secreta Mighty (cambia color / bonus) |
| `$0B` | `EnemyType_B_Captive` | Prisionero / Rey / Familia real capturada |
| `$0C` | `EnemyType_C_Balloon` | Globo bonus |
| `$0D` | `EnemyType_D_Mummy1` | Generador / Aparición de momia 1 |
| `$0E` | `EnemyType_E_BonusCoin` | Moneda de bonificación |
| `$0F` | `EnemyType_F_Mummy2` | Generador / Aparición de momia 2 |
| `$10` | `EnemyType_10_Brother` | Hermano / Miembro de la familia |

### 3.4. Catálogo de Ítems (`Item_*`)

| ID Dec | Símbolo ASM | Objeto / Efecto |
| :--- | :--- | :--- |
| `0` | `Item_100Bag` | Bolsa de 100 puntos |
| `1` | `Item_300Bag` | Bolsa de 300 puntos |
| `2` | `Item_1000Bag` | Bolsa de 1000 puntos |
| `3` | `Item_GoldCoin` | Moneda de oro |
| `4` | `Item_MightyDrink` | Bebida Mighty Milk (restaura tiempo) |
| `5` | `Item_Bomb` | Bomba (objeto central del juego) |
| `6` | `Item_Sphinx` | Esfinge secreta (10.000 pts / secreto) |
| `7` | `Item_TecmoPlate` | Placa de Tecmo |
| `8` | `Item_MightyCoin` | Moneda Mighty (permite cambiar de tamaño / poder) |
| `9` | `Item_CrystalBall1` | Bola de cristal azul |
| `10` | `Item_CrystalBall2` | Bola de cristal roja |
| `11` | `Item_King` | Rey Pamera |
| `12` | `Item_Queen` | Reina |
| `13` | `Item_Princess` | Princesa |
| `14` | `Item_BeelzebutMask` | Máscara de Belzebut |
| `15` | `Item_0E_Nothing` | Vacío / Cofre vacío |
| `16` | `Item_RedChest` | Cofre rojo |
| `17` | `Item_OrangeChest` | Cofre naranja |

### 3.5. Flags de Sala (`RoomFlag`)

| Bit | Símbolo ASM | Función |
| :--- | :--- | :--- |
| `%00000010` | `RF_BombRoom` | Sala tipo pirámide con bombas (pantalla fija clásica) |
| `%00001000` | `RF_AtScrollEdge` | Límite de desplazamiento alcanzado |
| `%00010000` | `RF_SingleScreen` | Sala de pantalla única (sin scroll) |
| `%00100000` | `RF_ScrollStop` | Bloqueo de avance de scroll |
| `%01000000` | `RF_Horizontal` | Desplazamiento horizontal activo |
| `%10000000` | `RF_SectionStart` | Comienzo de una nueva sección / zona |

---

## 4. Discrepancias y Estado Actual en `port_native/`

Al auditar los archivos en `port_native/src/`:
1. `EnemyStruct` en el port nativo debe coincidir con la precisión de los 28 bytes y los subpíxeles (`XVelLo`, `XPosLo`, `YVelLo`, `YPosLo`) para garantizar paridad física 1:1.
2. Los IDs de items e índices de sonido deben mantenerse mapeados sin desfases respecto a los enums en `defs.asm`.

---

## 5. Criterios de Implementación y Tests

- Asegurar que `port_native/src/` use constantes idénticas para los tipos de enemigo e ítems.
- Verificar que los tests de colisión y entidades (`test_enemy_ai.c`, `test_room_decode.c`) utilicen estos mismos valores como oráculo.
