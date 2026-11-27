# Arquitectura y Layout de Memoria de la ROM: Mighty Bomb Jack (NES)

- **ID de Entregable**: `research/scope/rom_layout.md`
- **Fase**: Fase 0 — Preparación y Alcance
- **Tarea**: `TASK-F0-002` (Subpaso 0.2)
- **Configuración de Cartucho**: Mapper 3 (CNROM) / NROM-256
- **Fuente de Verdad**: `asm/mighty-bomb-jack.asm` y `asm/src/defs.asm`
- **Estado**: `[CONFIRMADA]`

---

## 1. Cabecera iNES y Formato Físico (Versión USA `REV_US`)

El binario canónico `mbj-us.nes` se estructura en una cabecera de 16 bytes seguida de 32 KB de código/datos PRG y 32 KB de patrones gráficos CHR:

| Campo iNES | Valor Binario / Hex | Interpretación Técnica | Evidencia en `asm/` |
| :--- | :---: | :--- | :--- |
| **Magic Number** | `4E 45 53 1A` | `"NES<EOF>"` (Formato iNES estándar) | `mighty-bomb-jack.asm:47` |
| **PRG-ROM Size** | `$02` (2 páginas) | $2 \times 16\text{ KB} = 32\text{ KB}$ ($32.768\text{ bytes}$) | `INESPRG 2` (Línea 49) |
| **CHR-ROM Size** | `$04` (4 páginas) | $4 \times 8\text{ KB} = 32\text{ KB}$ ($32.768\text{ bytes}$) | `INESCHR 4` (Línea 50) |
| **Mapper Number**| `3` (`$03`) | **CNROM** (conmutación de bancos CHR de 8 KB) | `NES_MAPPER_NUM = 3` (Línea 24, 51) |
| **Mirroring** | `0` (`$00`) | **Horizontal Mirroring** (Nametables $2000=$2400 y $2800=$2C00) | `INESMIR 0` (Línea 52) |
| **Trainer** | `0` | Sin trainer de 512 bytes | Por defecto |

---

## 2. Mapa de Memoria del Espacio de Direccionamiento de la CPU ($0000-$FFFF)

La CPU 2A03 accede a un bus lineal de 16 bits (64 KB). El desensamblado define y utiliza las siguientes regiones exactas:

```
+------------------+ $FFFF
| Vectores 6502    | $FFFA-$FFFF (NMI: $8081, RESET: $8000, IRQ: $8081)
+------------------+ $FFFA
| Padding ROM      | $FFF0-$FFF9 (Relleno con $FF)
+------------------+ $FFF0
| Bytecode Música  | $F900-$FFEF (Pistas musicales en music-data.asm)
+------------------+ $F900
| Motor de Audio   | $F39E-$F8FF (music-engine.asm, tablas de notas/volumen)
+------------------+ $F39E
| SFX y Punteros   | $EC00-$F39D (Efectos de sonido y saltos de audio)
+------------------+ $EC00
| Gráficos/Salas   | $C201-$EBFF (Metatiles, textos, 257 salas RoomData)
+------------------+ $C201
| Tablas Físicas   | $C0E3-$C200 (Salto 64 pasos word_C0E3, velocidad X word_C163)
+------------------+ $C0E3
| Núcleo de Código | $8000-$C0E2 (prg.asm: Boot, NMI, física, IA momias, HUD BCD)
+------------------+ $8000 (Inicio PRG-ROM, 32 KB fijos)
| Cartridge Space  | $4020-$7FFF (Sin PRG-RAM externa en placa CNROM)
+------------------+ $4020
| Registros APU/IO | $4000-$4017 (Audio 2A03, DMA $4014, Joypads $4016-$4017)
+------------------+ $4000
| Registros PPU    | $2000-$2007 (PPUCTRL, PPUMASK, PPUSCROLL, etc. reflejados hasta $3FFF)
+------------------+ $2000
| RAM Espejada     | $0800-$1FFF (Reflejo en hardware de los 2 KB de RAM interna)
+------------------+ $0800
| RAM Interna 2 KB | $0000-$07FF (Zero-page, Stack, OAM buffer, Variables, Slots enemigos)
+------------------+ $0000
```

---

## 3. Desglose Detallado de la RAM Interna ($0000-$07FF)

El archivo `asm/src/ram.asm` cataloga 338 símbolos mapeados sobre los 2.048 bytes de la RAM:

| Rango de RAM | Tamaño | Propósito y Estructura | Evidencia en `asm/` |
| :--- | :---: | :--- | :--- |
| **`$0000 - $001F`** | 32 B | **Zero-Page Scratchpad**: Punteros temporales indirectos (`byte_0` a `byte_1F`), usados en bucles y llamadas cortas. No preservan estado entre frames. | `ram.asm:1-60` |
| **`$0020 - $00FF`** | 224 B | **Variables Críticas Zero-Page**: Coordenadas de Jack (`PlayerXPosHi/Lo`, `PlayerYPosHi/Lo`), velocidades, estado de salto, timers y contadores. | `ram.asm:61-350` |
| **`$0100 - $01FF`** | 256 B | **Pila Hardware 6502**: Stack pointer inicializado a `$FF` en `RESET` (`TXS`). Empleado para `JSR/RTS` y manipulado en 12 sitios con `PLA; PLA`. | `prg.asm:18` |
| **`$0200 - $02FF`** | 256 B | **OAM Sprite Buffer**: 64 sprites $\times$ 4 bytes (`Y, Tile, Attr, X`). Se vuelca a la PPU en cada frame mediante DMA `$4014` durante la NMI. | `prg.asm:100` |
| **`$0300 - $037F`** | 128 B | **Control de Entrada y Timers**: Copias de Joypad (`Joypad1_Immediate`, `Joypad1_ImmediateCopy`), flags de pausa, scroll (`byte_3C2/3C4`). | `ram.asm:450-520` |
| **`$0380 - $03F0`** | 113 B | **Buffer de Vuelco VRAM**: Cola lineal para transferir datos a la PPU durante el VBlank (`HandlePPUUpdates`). | `prg.asm:166-237` |
| **`$0400 - $04FF`** | 256 B | **Estructura del Jugador y Sala**: `PlayerStruct`, identificación de sala activa (`CurrentRoomID`), puertas, estado de cofres y bombas. | `ram.asm:600-750` |
| **`$0500 - $05FF`** | 256 B | **Buffers de Atributos y Baldosas**: Almacén temporal de atributos de pantalla (`$0563`, `$05A3`) para actualización de Nametables. | `prg.asm:200-230` |
| **`$0600 - $07FF`** | 512 B | **Slots de Entidades y Enemigos**: Arreglos de estructuras `EnemyStruct` de 28 bytes (`$1C`) por slot para momias, proyectiles y monedas. | `ram.asm:900-1180` |

---

## 4. Vectores de Interrupción de la CPU ($FFFA-$FFFF)

Al final del espacio direccionable de 32 KB (`asm/mighty-bomb-jack.asm:92-96`), se configuran los 3 vectores de interrupción de hardware del 6502:

| Dirección | Vector | Etiqueta Destino | Dirección de Salto | Comportamiento |
| :---: | :---: | :---: | :---: | :--- |
| **`$FFFA - $FFFB`** | **NMI** | `NMI` | **`$8081`** | Se dispara en cada VBlank (60 Hz). Ejecuta el 100% del bucle de juego, DMA, PPU, Joypads y APU. |
| **`$FFFC - $FFFD`** | **RESET** | `RESET` | **`$8000`** | Se ejecuta al encender la consola. Inicializa hardware, borra RAM y entra en `LoopForever: JMP LoopForever`. |
| **`$FFFE - $FFFF`** | **IRQ / BRK** | `IRQ` | **`$8081`** | Comparte el mismo punto de entrada que la NMI en `prg.asm:90`. No se usa IRQ por hardware en CNROM. |

---

## 5. Arquitectura de Bancos de CHR-ROM (32 KB en `mbj-us.chr`)

En la revisión USA (`REV_US`), el cartucho dispone de 32 KB de CHR-ROM divididos en 4 bancos de 8 KB conmutables vía CNROM:

```
+-------------------------------------------------------------+
|               CHR-ROM SPACE (32 KB = 4 x 8 KB)              |
+-------------------------------------------------------------+
| Banco 0 ($0000-$1FFF en PPU) -> Sprites (Jack, Enemigos)   |
|                                 y Tipografía HUD Común      |
+-------------------------------------------------------------+
| Banco 1 ($0000-$1FFF en PPU) -> Baldosas Fondo Pirámide     |
|                                 (Metatiles Piedra / Templos)|
+-------------------------------------------------------------+
| Banco 2 ($0000-$1FFF en PPU) -> Salas Reales / Tesoro       |
|                                 (Fondos de Palacios)        |
+-------------------------------------------------------------+
| Banco 3 ($0000-$1FFF en PPU) -> Exteriores y Finales        |
|                                 (Secuencias Cinemáticas)    |
+-------------------------------------------------------------+
```

---

## 6. Verificación de Integridad

- **Re-ensamblado Bit-Exact**: Ejecutando `asm6f_64.exe -m -L mighty-bomb-jack.asm test_build.nes` con el flag `REV_US` se genera una ROM que coincide byte a byte con el hash SHA-1 oficial de la ROM comercial de Tecmo USA (`REV_US`).
- **Cero Dependencias Externas en PRG**: No existe ni un solo `.incbin` dentro de los 32 KB de PRG-ROM (`$8000-$FFFF`), lo que demuestra que el código ensamblador analizado es 100% completo y desensamblado.
