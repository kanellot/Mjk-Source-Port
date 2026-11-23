# Estado del Proyecto: Mighty Bomb Jack — Reconstrucción Funcional



**Última actualización**: 2026-10-24  

**Iteración Activa**: Iteración 2 — Reconstrucción Funcional Integral y Especificación Formal  

**Hito / Fase Activa**: **Fase 7 — Bug Fixing, Paridad Exacta con el Original y Refactorización [EN CURSO]**  

**Versión Base Canónica**: **USA / NTSC (`REV_US`)**  

**Skill Activa**: [`nes-functional-reconstruction`](../.agents/skills/nes-functional-reconstruction/SKILL.md)  

**Meta (/goal)**: `[EN PROGRESO - FASE 7]` — Eliminar todos los bugs y discrepancias visuales/mecánicas entre `bin/mjk.exe` y la ROM original `bin/mbj-us.nes` verificando contra `asm/` y `cynes`.



---



## 1. Visión General del Proyecto



El objetivo de extraer el 100% de la lógica, datos, rutinas, estados y mecánicas del juego original de NES (*Mighty Bomb Jack*, Tecmo 1987) a partir del código ensamblador en `asm/` ha sido **completado con éxito**. Se dispone de una **especificación funcional formal, verificable e independiente de plataforma**.



```

                  +--------------------------------------+

                  |   Mighty Bomb Jack (NES Tecmo 1987)  |

                  +--------------------------------------+

                                     |

                                     v

                       [Fuente de Verdad Absoluta]

                                   asm/

                       * 6502 Disassembly (asm6f)

                       * Solo lectura inmutable (+R)

                                     |

                                     v

                  [Proceso: nes-functional-reconstruction]

          Fase 0 (Alcance) --> Fase 1 (Inventarios) --> Fase 2 (Control/Datos)

                     --> Fase 3 (Contratos) --> Fase 4 (Auditoría)

                                     |

                                     v

                     [Especificación Funcional Pura]

                                research/

                     * 8 Contratos de Comportamiento Algorítmicos

                     * Máquina de Estados de 13 fases

                     * Matriz de Trazabilidad 100% verificada

                                     |

                                     v  (Fase 4 SUPERADA)

                   [Fase 5: Autorización de Arquitectura]

```



---



## 2. Matriz de Fases de Reconstrucción Funcional



| Fase | Directorio de Salida | Estado Actual | Nivel de Cobertura | Criterio de Aceptación / Observaciones |

| :--- | :--- | :---: | :---: | :--- |

| **Gobernanza y Memoria** | `project/` | `[ACTIVO]` | 100% | Iteración 1 archivada en `archive/iteration_01_c_port/`. Ledgers activos en `project/`. |

| **Skill Reconstrucción** | `.agents/skills/` | `[ACTIVO]` | 100% | Skill `nes-functional-reconstruction` desplegada. |

| **Fase 0: Preparación y Alcance** | `research/scope/` | `[COMPLETADA]` | 100% | 4 entregables generados (`scope.md`, `rom_layout.md`, `repository_inventory.md`, `analysis_plan.md`). |

| **Fase 1: Inventario Estático** | `research/inventories/` | `[COMPLETADA]` | 100% | 5 entregables generados con `tools/deep_asm_analyzer.py` (294 subrutinas, 337 variables, 525 tablas). |

| **Fase 2: Flujo Control/Datos** | `research/control_flow/` | `[COMPLETADA]` | 100% | 4 entregables generados (Pipeline NMI, 15 JumpTables, 16 PLA;PLA, buffers VRAM y APU). |

| **Fase 3: Contratos Funcionales** | `research/contracts/` | `[COMPLETADA]` | 100% | 8 entregables generados (Máquina de estados, física 64-pasos, 257 salas, IA, HUD BCD). |

| **Fase 4: Auditoría Cobertura** | `research/audit/` | `[SUPERADA]` | 100% | Matriz bidireccional y certificación formal de 0 puntos ciegos (`audit_report.md`). |

| **Fase 5: Autorización Arq.** | `research/` | `[COMPLETADA]`| 100% | Arquitectura desacoplada en C99, BYOR extractor y guía didáctica (`source_port_architecture.md`). |

| **Fase 6: Implementación C** | `src/` | `[COMPLETADA]` | 100% | Refactorización PPU 2C02, descompresor 4-tier y OAM completados. 8/8 tests al 100%. `bin/mjk.exe` operativo. |

| **Fase 6.2: Gameplay, Físicas, Pasillos y Scroll** | `src/` | `[COMPLETADA]` | 54 tareas finalizadas ([`TASKS_DONE.md`](./TASKS_DONE.md)) | Mecánicas base implementadas; suite con 17/17 tests deterministas pasando al 100%. |
| **Fase 7: Bug Fixing, Paridad Exacta y Refactorización** | `src/`, `research/`, `tests/` | `[COMPLETADA Y HOMOLOGADA AL 100%]` | 10 tareas finalizadas ([`TASKS_DONE.md`](./TASKS_DONE.md)) / 0 pendientes | Todas las discrepancias y bugs reportados (enemigos OAM 16x16, calaveras amarillas, audio APU sin pitch bend ni white noise, cofres canónicos, colisión de power-ups, límites de scroll, puntuación flotante compacta, retroceso fluido y animación de bombas) corregidos con 1:1 paridad contra `asm/` y verificados al 100%. `bin/mjk.exe` operativo. |



---



## 3. Estado de la Infraestructura



- **Permisos de `asm/`**: Solo lectura inmutable verificado.

- **Permisos de `project/` y `research/`**: Lectura y escritura activos.

- **Permisos de `port_native/`**: Congelada (prohibida la implementación en esta etapa).

- **Herramientas de Análisis (`tools/`)**: Disponibles scripts base (`extract_asm_inventory.py`, `asm_inventory.json`); previstos analizadores AST ampliados para Fase 1.

