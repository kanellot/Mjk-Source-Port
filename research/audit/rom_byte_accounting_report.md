# Informe Matemático de Contabilidad de Bytes de ROM

- **Fase**: Fase 4 — Auditoría de Cobertura y Trazabilidad
- **Herramienta de Verificación**: `tools/verify_rom_accounting.py`
- **Rango Auditado**: `$8000` a `$FFFF` (32.768 bytes lineales de PRG-ROM)
- **Veredicto**: `[100.00% CONTABILIDAD MATEMÁTICA DEMOSTRADA — CERO HUECOS]`

## 1. Desglose Lineal por Categorías

| Categoría de Byte | Rango / Descripción | Bytes Ocupados | Porcentaje |
| :--- | :--- | :---: | :---: |
| **Instrucciones 6502 (Subrutinas)** | Código ejecutable en `prg.asm` y `music-engine.asm` | 15,227 B | 46.47 % |
| **Tablas de Datos y Bytecodes** | Metatiles, 257 salas, tablas físicas y audio | 17,210 B | 52.52 % |
| **Relleno de Fin de ROM (Padding)** | Relleno con `$FF` previo a los vectores (`$FEB5-$FFF9`) | 325 B | 0.99 % |
| **Vectores de Hardware 6502** | `$FFFA-$FFFF` (NMI: `$8081`, RESET: `$8000`, IRQ: `$8081`) | 6 B | 0.02 % |
| **Bytes Desconocidos o Huérfanos** | Ninguno detectado (auditoría exhaustiva) | **0 B** | **0.00 %** |
| **TOTAL PRG-ROM** | Espacio lineal de direccionamiento | **32,768 B** | **100.00 %** |

## 2. Metodología de Auditoría Matemática

1. Ensamblado determinista con `asm6f_64` generando el mapa de listado completo (`mbj-us.lst`).
2. Verificación de que cada intervalo de direcciones $[A_i, A_{i+1})$ corresponde a una instrucción o directiva formal del código fuente.
3. Comprobación de que la unión de todos los intervalos cubre exactamente el conjunto discreto $\{x \in \mathbb{Z} \mid 0x8000 \le x \le 0xFFFF\}$ de cardinalidad 32.768.
4. Cruce con el inventario de 294 subrutinas (`code_inventory.csv`) y 525 tablas de datos (`data_inventory.csv`).

## 3. Conclusión Formal

Queda matemáticamente demostrado que no existe ningún bloque de código sin catalogar, ningún dato sin clasificar y ningún hueco en los 32.768 bytes de la ROM de *Mighty Bomb Jack*.
