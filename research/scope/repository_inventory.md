# Inventario Integral del Repositorio y Fuentes Ensamblador 6502

- **ID de Entregable**: `research/scope/repository_inventory.md`
- **Fase**: Fase 0 — Preparación y Alcance
- **Tarea**: `TASK-F0-003` (Subpaso 0.1)
- **Fuente de Verdad**: `asm/` (Modo Solo Lectura inmutable)
- **Estado**: `[CONFIRMADA]`

---

## 1. Resumen Cuantitativo del Código Fuente Original

El código original en `asm/src/` consta de **18.676 líneas de código fuente en ensamblador 6502** organizadas en 12 archivos fuente y 2 bancos binarios de CHR-ROM:

| Archivo Fuente | Líneas | Tamaño (Bytes) | Hash SHA-256 | Propósito Técnico |
| :--- | :---: | :---: | :--- | :--- |
| **`prg.asm`** | 8.959 | 195.197 | `37FC1CCC0154A18F19CCAF45C936AF15F80D879BFC5182A1D1A94E56B496B5D6` | Núcleo de ejecución: Boot, NMI, máquina de estados, física de Jack, colisiones, enemigos e IA, HUD y rutinas de final. |
| **`data.asm`** | 2.438 | 86.783 | `C2EAA1ABEB04D55DC42CA778EE48C84010477F337659A17318422157E85487E8` | 679 tablas de datos: metatiles, punteros de salas (`RoomDataPointers`), tablas trigonométricas/salto (`word_C0E3`/`word_C163`) y textos en inglés/japonés. |
| **`ram.asm`** | 2.412 | 84.289 | `05CAD23B0F5A59AB162DEC474989203645636827AA4DC57D33064C48E68E3E2A` | Mapeo y documentación de los 338 símbolos de RAM ($0000-$07FF) con comentarios originales de ingeniería inversa. |
| **`music-data.asm`** | 2.566 | 40.502 | `10B176C25288C206FB508B743E78C6E38D5A6434F66E12DEA25E17763A11846A` | Bytecode musical estructurado para las 14 pistas de audio (Title, Main BGM, Torture Room, Round Clear, Game Over, etc.). |
| **`roomdata-us.asm`** | 483 | 26.566 | `82F4209BF25B4031A81B08A6195DD551460B4DE16E7C0269DD66AA9E51FB0382` | Bytecode de diseño de las 257 salas (`RoomData_01` a `RoomData_FF`) específico de la revisión comercial USA. |
| **`roomdata-jp.asm`** | 483 | 26.623 | `AFAB1A3F3942B93D68B1EACFE38344B73BC4D41A18245CD7D95BD11AFABB94A8` | Bytecode de diseño de salas para la versión original japonesa Famicom (referencia comparativa). |
| **`music-engine.asm`** | 726 | 15.780 | `BF4A0BDC3DFB70BA824489F1DF41F7EF5AA6ABA372C995600EA37F56F5344111` | Intérprete y sintetizador de audio 2A03 (rutinas de paso de frame, asignación de canales Pulse 1/2, Triangle, Noise y SFX). |
| **`defs.asm`** | 276 | 7.829 | `0AAF3E4667890EBCA7C8A55D756BCE94AF503D9C65DAC493B3B3351F36340D2B` | Constantes globales de hardware NES, máscaras del Joypad, flags de sala y enumeraciones de enemigos. |
| **`compatibility.asm`**| 176 | 2.468 | `32530D1CC02AAEBDA8C09A7EDE05066F9DE1A886F4A0CE0D3782836C88404F1D` | Directivas de compatibilidad de sintaxis para el ensamblador `asm6f`. |
| **`end-of-rom-injection.asm`** | 66 | 1.514 | `778443945AAAB37AA2F72F9254711A94B5C5E09D06F20411FEACA6948850ED84` | Código de inyección para la protección anti-copia de Mapper 185 en versión Famicom. |
| **`macros.asm`** | 46 | 1.473 | `B3DAC8C5049A6D94FD1E2B143021C0BEDC2FA15B697BDEC9CA05B90C8675AFC2` | Macros auxiliares de ensamblado (asignaciones y saltos relativos). |
| **`registers.asm`** | 45 | 931 | `DC61FF79C309F9243A95A73702D80D917210A0417CE5E93F0C8CA4E546DB0AF7` | Definición de registros de hardware mapeados en memoria PPU ($2000-$2007) y APU ($4000-$4017). |
| **TOTAL FUENTES** | **18.676** | **489.467** | — | — |

---

## 2. Inventario de Assets Binarios (CHR-ROM)

| Asset CHR | Tamaño | SHA-256 | Descripción |
| :--- | :---: | :--- | :--- |
| **`mbj-us.chr`** | 32.768 B (32 KB) | `9719BE00CD8EBD5F0987BF1020F2ECBDE8C17F164C82E880B6DE4D5E9FA1F402` | 4 bancos de 8 KB que contienen los gráficos oficiales de la versión USA (Jack, enemigos, HUD, cofres, fondos de pirámide). |
| **`mbj-jp.chr`** | 8.192 B (8 KB) | `A94B6C138EBF928D13F7DF941DF29C027E29CA476DE7CD51804B09B16CA1769F` | Banco único de gráficos de la versión Famicom 1986. |

---

## 3. Binarios de Referencia Generables (`asm/bin/`)

El script `asm/build.bat` / `asm/build.sh` permite ensamblar bit a bit las 3 ROMs comerciales oficiales:

| ROM Binaria | Tamaño | Mapper | Revisión | Verificación SHA-1 Oficial |
| :--- | :---: | :---: | :--- | :--- |
| **`mbj-us.nes`** | 65.552 B | 3 (CNROM) | USA NTSC 1987 (`REV_US`) | Coincidencia 100% con No-Intro USA standard |
| **`mbj-jp.nes`** | 40.976 B | 185 (CNROM spicy) | JP Famicom 1986 | Coincidencia 100% con No-Intro Japan standard |
| **`mbj-jp-rev-a.nes`** | 40.976 B | 185 (CNROM spicy) | JP Famicom Rev A | Coincidencia 100% con No-Intro Japan (Rev A) |

---

## 4. Herramientas de Compilación (`asm/tools/`)

- **`asm6f_64.exe`**: Ensamblador 6502 de 64 bits optimizado con soporte para emisión de símbolos (`-m`) y listados (`-L`).
- **`asm6f_32.exe`**: Versión x86 de 32 bits de la misma herramienta.
- **`asm6f`**: Binario para entornos Linux / macOS.
