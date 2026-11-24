# Registro de Dependencias No Resueltas y Análisis de Saltos Indirectos

- **ID de Entregable**: `research/control_flow/unresolved_dependencies.md`
- **Fase**: Fase 2 — Flujo de Control, Datos y Modelo de Hardware
- **Fuente de Verdad**: `tools/asm_inventory.json` (`tools/asm_query.py`) y `research/control_flow/control_flow.md`
- **Estado**: `[CONFIRMADA]` — Cero Dependencias Huérfanas

---

## 1. Auditoría de Saltos Indirectos (`JMP (ind)`)

En el desensamblado de 32 KB de PRG-ROM (`asm/src/prg.asm`), se han auditado todas las instrucciones de salto indirecto:

1. **`JumpTable` (`prg.asm:2180`)**:
   - Instrucción: `JMP (JumpTableTarget)` en `$8D91`.
   - **Estado**: `[CONFIRMADA] 100% Resuelta`.
   - Se analizaron los 15 sitios de llamada. Todas las 15 tablas tienen destinos explícitos y estáticos etiquetados en el código. No existe ninguna tabla dinámica con destinos indeterminados en tiempo de ejecución.
2. **`sub_9729` (`prg.asm:3820`)**:
   - Instrucción: `JMP (byte_0)` para despacho de dibujado de metatiles.
   - **Estado**: `[CONFIRMADA] 100% Resuelta`.
   - `(byte_0, byte_1)` se inicializa exclusivamente a partir de punteros estáticos de `RoomDataPointers` y tablas de metatiles.
3. **Vectores de Hardware (`$FFFA-$FFFF`)**:
   - `NMI` $\to$ `$8081` (`prg.asm:91`).
   - `RESET` $\to$ `$8000` (`prg.asm:8`).
   - `IRQ` $\to$ `$8081` (`prg.asm:90`).
   - **Estado**: `[CONFIRMADA] 100% Resuelta`.

---

## 2. Auditoría de Dependencias de Hardware Externas (Mappers)

- **Mapeador**: Mapper 3 (CNROM).
- **Escrituras en ROM**: El análisis estático de `deep_asm_analyzer.py` confirma que **no existen escrituras directas a direcciones `$8000-$FFFF`** durante el bucle de juego activo.
- **Conmutación de Bancos CHR**: Los bancos CHR se seleccionan mediante la configuración inicial de la placa CNROM para sprites y fondos. No hay conmutación de bancos PRG en tiempo de ejecución (la PRG-ROM de 32 KB es completamente plana y lineal).

---

## 3. Dictamen de Cierre de Fase 2

No queda ninguna dependencia indirecta, salto anónimo o acceso a hardware no modelado.
El flujo de control y de datos está **100% resuelto y documentado** para proceder a los contratos funcionales de la Fase 3.
