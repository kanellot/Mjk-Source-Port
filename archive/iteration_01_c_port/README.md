# Archivo Histórico: Iteración 1 — Intento de Port Nativo en C / Raylib

**Fecha de Archivo**: 2026-10-24  
**Período Operativo**: 2026-10-22 al 2026-10-23  
**Estado**: `[ARCHIVADO - CONGELADO COMO REFERENCIA]`

---

## 1. Propósito de este Archivo

Este directorio preserva íntegramente la bitácora, tareas, decisiones de arquitectura y constitution de la **Iteración 1**, donde se intentó implementar un port nativo en C11 usando Raylib 5.5 (`port_native/`).

Estos documentos se preservan para:
1. Mantener trazabilidad histórica completa del trabajo previo.
2. Servir como material de consulta de hipótesis iniciales y decisiones tomadas (ej. adopción de `REV_US` bajo `ADR-004`).
3. Aprender de las causas que motivaron el pivotaje hacia una metodología de ingeniería inversa integral antes de implementar.

---

## 2. Contenido del Archivo

- [`AGENTS.md`](./AGENTS.md): Constitución y gobernanza operativa original.
- [`TASKS.md`](./TASKS.md): Matriz de tareas de la iteración 1 (desde la configuración inicial hasta los hitos de compilación de Raylib y audio).
- [`PROJECT_STATUS.md`](./PROJECT_STATUS.md): Estado final reportado al término de la iteración 1.
- [`DECISIONS.md`](./DECISIONS.md): Registro de Decisiones de Arquitectura (`ADR-001` a `ADR-005`).
- [`SESSION_LOG.md`](./SESSION_LOG.md): Bitácora continua de intervenciones (556 líneas).

---

## 3. Resumen del Post-Mortem Técnico

Al evaluar el resultado de la implementación generada en `port_native/`, se identificaron discrepancias críticas:
1. **Métricas de Vanidad**: Se asumió una cobertura del 100% en subrutinas simplemente porque sus nombres aparecían en tablas de `RES-003`, sin haber analizado ni especificado su semántica real.
2. **Implementación de Fachada (Potemkin)**: El juego generado en `main.c` no reproducía la máquina de estados 6502 original (`GameState 0..C`), sino un prototipo básico con límites de suelo hardcodeados y una sola momia mockeada.
3. **Falsa Validación**: Las pruebas unitarias pasaban al 100% porque evaluaban funciones auxiliares creadas ad-hoc para pasar los tests, no la lógica real del cartucho NES.

---

## 4. Nueva Dirección (Iteración 2)

A partir del 2026-10-24 se inicia la **Iteración 2: Reconstrucción Funcional Integral y Especificación Formal del Código Ensamblador NES**, bajo la nueva Skill de Reconstrucción Funcional.
- La carpeta `port_native/` queda **congelada**.
- Toda la gestión activa de tareas se traslada a [`project/`](../../project/).
- Las investigaciones y especificaciones se alojan y expanden en [`research/`](../../research/).
