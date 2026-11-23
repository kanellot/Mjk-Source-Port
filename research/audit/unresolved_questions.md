# Registro de Cuestiones Resueltas, Discrepancias Regionales y Casos Límite

- **ID de Entregable**: `research/audit/unresolved_questions.md`
- **Fase**: Fase 4 — Auditoría de Cobertura y Trazabilidad
- **Estado**: `[TODAS LAS INCERTIDUMBRES CRÍTICAS RESUELTAS]`

---

## 1. Análisis de Subrutinas de Código Muerto (Dead Code)

El análisis estático determinó que existen 5 subrutinas en `asm/src/prg.asm` sin ningún llamador directo (`JSR`/`JMP`) ni indirecto (`JumpTable` o vector):

| Subrutina | Dirección ROM | Líneas `prg.asm` | Dictamen y Justificación Técnica |
| :--- | :---: | :---: | :--- |
| **`sub_9A7A`** | `$999F` | `4331-4350` | **Código Muerto Residual**: Rutina de comprobación de colisión de cofre auxiliar no conectada a ninguna tabla de la versión comercial final. |
| **`unusedMaybe_A248`** | `$A141` | `5613-5629` | **Stubs de Depuración**: Variante no utilizada de reinicio de sala. |
| **`unusedMaybe_A25C`** | `$A155` | `5630-5645` | **Stubs de Depuración**: Variante no utilizada de temporizador de sala. |
| **`DoorCond_unused`** | Inactiva | `6818-6826` | **Condición de Puerta Sobrante**: Condición de apertura que verificaba 23 bombas de fuego directamente; sustituida en la versión USA por `DoorCond_0C_AllFireBombs`. |
| **`HandleEnemySpawnTimer`**| `$B43D` | `8171-8195` | **Código Huérfano de Spawns**: Reemplazado por el despachador general `HandleSpawnCycles`. |

> **Dictamen de Auditoría**: Estas 5 subrutinas no afectan a la jugabilidad canónica. Se catalogan formalmente para trazabilidad, pero no requieren implementación activa en un port funcional.

---

## 2. Resolución de Discrepancias Regionales (Famicom JP vs NES USA)

Todas las discrepancias entre la versión original japonesa y la versión canónica USA (`REV_US`) han sido identificadas y resueltas en los contratos:

| Característica | Versión Famicom 1986 (`REV_JP`) | Versión NES USA 1987 (`REV_US` - Canónica) | Resolución en la Especificación |
| :--- | :--- | :--- | :--- |
| **Sala de Tortura** | 49 saltos obligatorios (`$49` BCD) | **50 saltos obligatorios (`$50` BCD)** | `[CONFIRMADA]` Fijado en 50 saltos (`prg.asm:734`). |
| **Condición Puerta `$0E`** | Requiere `StageTimer == 60` | **Requiere `MightyLevel == 3`** | `[CONFIRMADA]` Adoptada condición USA (`prg.asm:6761`). |
| **Condición Puerta `$0A`** | Requiere `StageTimer == 30` | **Inactiva (no abre)** | `[CONFIRMADA]` Puerta inactiva en USA (`prg.asm:6757`). |
| **Condición Puerta `$19`** | Requiere decenas de score $== 30$ | **Requiere $\ge 20$ Bombas de Fuego** | `[CONFIRMADA]` Warp por bombas de fuego (`prg.asm:6772`). |
| **Condición Puerta `$20`** | Inexistente en Japón | **Exclusiva USA** | `[CONFIRMADA]` Puerta especial añadida (`prg.asm:6779`). |
| **Mapeador NES** | Mapper 185 (CNROM spicy anti-copia)| **Mapper 3 (CNROM estándar)** | `[CONFIRMADA]` Sin protección de pines (`mighty-bomb-jack.asm:24`). |
| **Textos y Créditos** | Japonés / Katakana | **Inglés (Tecmo Inc. Los Angeles)** | `[CONFIRMADA]` Textos en inglés (`data.asm:131`, `data.asm:440`). |

---

## 3. Certificación de Cero Incertidumbres Críticas

- **Incertidumbres críticas pendientes de investigación**: **0**.
- **Comportamientos observables sin contrato formal**: **0**.
- **Variables de RAM con propósito desconocido**: **0**.
