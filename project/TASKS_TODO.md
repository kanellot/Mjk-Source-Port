# Tablero Maestro de Tareas Pendientes ([TODO]): Mighty Bomb Jack

**Iteración**: 2 — Reconstrucción Funcional Integral y Especificación Formal  
**Hito Activo**: **Fase 7 — Bug Fixing, Paridad Exacta y Refactorización**  
**Versión Base Canónica**: **USA / NTSC (`REV_US`)**  
**Skill de Referencia**: [`nes-functional-reconstruction`](../.agents/skills/nes-functional-reconstruction/SKILL.md)  
**Registro de Tareas Completadas**: [`project/TASKS_DONE.md`](./TASKS_DONE.md) (54 tareas de Fases 0–6 completadas)  

---

## 1. Convenciones y Protocolo de Ejecución (Fase 7)

- **Regla de Contexto Atómico**: Cada uno de los 9 problemas reportados constituye una tarea independiente (`TASK-F7-001` a `TASK-F7-009`, más `TASK-F7-010` para reactivar enemigos corregidos) para no saturar jamás la ventana de contexto de los subagentes.
- **Cadena Jerárquica y Modelos**:
  - **Nivel 0 (Chat)** y **Nivel 1 (Orquestador Principal)**: Gemini Argon Medium.
  - **Nivel 2 (`task_orchestrator` Efímero)**: Gemini Argon Medium (`Model: 'inherit'`).
  - **Nivel 3 (`asm_researcher`)**: Gemini Argon Medium (`Model: 'inherit'`) en investigación profunda de causa raíz cuando sea requerido, o Gemini Flash Medium (`Model: 'flash'`).
  - **Nivel 3 (`c_implementer`, `emulator_verifier`)**: Gemini Flash Medium (`Model: 'flash'`).
- **Estados**:
  - `[TODO]` - Tarea pendiente, no iniciada.
  - `[IN_PROGRESS]` - Tarea única en ejecución activa con bloque `CHECKPOINT ACTIVO`.
  - `[PAUSED]` - Tarea suspendida temporalmente.
  - `[DONE]` - Tarea completada, verificada y trasladada a [`TASKS_DONE.md`](./TASKS_DONE.md) con commit automático en Git.

---

## 2. Matriz de Tareas Pendientes — Fase 7 (Backlog Vacío)

*¡Todas las 10 tareas de la Fase 7 (`TASK-F7-001` a `TASK-F7-010`) han sido completadas, verificadas y registradas en [`project/TASKS_DONE.md`](./TASKS_DONE.md)!*

| Tarea ID | Descripción | Estado | Verificación |
| :--- | :--- | :---: | :---: |
| `TASK-F7-001` | Desactivación temporal de enemigos para debug | `[DONE]` | 17/17 tests PASS, runtime limpio |
| `TASK-F7-002` | Eliminación de calaveras amarillas y puertas fantasma | `[DONE]` | Cero píxeles residuales, 17/17 tests PASS |
| `TASK-F7-003` | Motor de audio APU con 8 canales virtuales y bytecode SFX | `[DONE]` | Cero pitch bend en BGM, cero white noise |
| `TASK-F7-004` | Paridad de cofres: subpaletas, metatiles y 4 sondas | `[DONE]` | Subpaleta 3 Rojo vs 0 Naranja, 17/17 tests PASS |
| `TASK-F7-005` | Colisiones de recolección de ítems y metatiles pasables | `[DONE]` | Sonda central, cero muros invisibles |
| `TASK-F7-006` | Límites de scroll en pasillos y descarte de Bomb Room $90 | `[DONE]` | Cámara anclada en Sala 6, cero bombas en muro |
| `TASK-F7-007` | Glifos compactos canónicos OAM de puntuación flotante | `[DONE]` | Metasprites 16x8 Pattern Table 0 blanco |
| `TASK-F7-008` | Fin del bloqueo permanente de Jack al retroceder a la derecha | `[DONE]` | Wrap modular `loc_9A39`, cámara fluida |
| `TASK-F7-009` | Animación en 2 fases al recoger bombas (humo $25 + puntos) | `[DONE]` | 32f humo metasprite $25 + 32f score |
| `TASK-F7-010` | Metasprites 16x16 de enemigos e IA canónica reactivada | `[DONE]` | Cero números, 0.5 px/f Q8.8, 0 spawns Sala 1 |
