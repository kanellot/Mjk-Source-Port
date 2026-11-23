# Tablero Maestro de Tareas: Reconstrucción Funcional de Mighty Bomb Jack

**Iteración**: 2 — Reconstrucción Funcional Integral y Especificación Formal  
**Hito Activo**: **Fase 7 — Bug Fixing, Paridad Exacta y Refactorización**  
**Skill de Referencia**: [`nes-functional-reconstruction`](../.agents/skills/nes-functional-reconstruction/SKILL.md)  
**Versión Base Canónica**: **USA / NTSC (`REV_US`)**  

---

## 1. Organización del Tablero de Tareas (Separación de Estados)

Para máxima claridad, control de progreso y bajo consumo de tokens al consultar el estado:

1. **Tareas Completadas (`[DONE]`)**:  
   👉 Consultar [`project/TASKS_DONE.md`](./TASKS_DONE.md)  
   *(Contiene el registro exhaustivo de las 50 tareas finalizadas con sus micro-lotes, fechas de cierre y criterios de aceptación verificados desde la Fase 0 hasta la Fase 6).*

2. **Tareas Pendientes (`[TODO]`)**:  
   👉 Consultar [`project/TASKS_TODO.md`](./TASKS_TODO.md)  
   *(Contiene el backlog activo y detallado de las tareas en cola para su ejecución secuencial).*

---

## 2. Convenciones de Estados

- `[TODO]` - Tarea pendiente, no iniciada.
- `[IN_PROGRESS]` - Tarea en ejecución activa por la cadena jerárquica (Nivel 1 -> Nivel 2 -> Nivel 3).
- `[PAUSED]` - Tarea suspendida temporalmente con bloque `CHECKPOINT ACTIVO`.
- `[DONE]` - Tarea finalizada, verificada y trasladada al archivo histórico [`TASKS_DONE.md`](./TASKS_DONE.md).

---

## 3. Resumen Ejecutivo del Backlog Activo ([TODO] — Fase 7: ¡100% Completada!)

| Total Tareas Planificadas | Fases Anteriores | Tareas Fase 7 Completadas | Tareas Fase 7 Pendientes | Cobertura de Tests |
| :---: | :---: | :---: | :---: | :---: |
| 64 | 54 (`[DONE]`) | 10 (`TASK-F7-001` a `TASK-F7-010` `[DONE]`) | 0 (`[TODO]`) | 17/17 tests deterministas (100% éxito) |

*(Cada una de las 10 tareas en [`project/TASKS_TODO.md`](./TASKS_TODO.md) cuenta con diagnóstico de causa raíz contra `asm/`, especificación quirúrgica para `c_implementer` y criterios de aceptación para `emulator_verifier`).*
