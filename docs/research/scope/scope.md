# Delimitación del Alcance Funcional: Mighty Bomb Jack (NES)

- **ID de Entregable**: `research/scope/scope.md`
- **Fase**: Fase 0 — Preparación y Alcance
- **Tarea**: `TASK-F0-002` (Subpaso 0.1)
- **Versión Canónica de Referencia**: **USA / NTSC (`REV_US`)**
- **Fuente Primaria**: `asm/` (Desensamblado 6502 reensamblable con `asm6f_64.exe`)
- **Estado**: `[CONFIRMADA]` en su totalidad mediante inspección de `asm/src/`

---

## 1. Identificación de la Versión Canónica

La reconstrucción funcional toma como estándar absoluto la revisión norteamericana comercial de 1987 publicada por Tecmo Inc.:
- **Identificador de Ensamblado**: `REV_US` definido en `asm/mighty-bomb-jack.asm:21-27`.
- **Diferenciadores Clave respecto a Famicom (JP)**:
  1. `GreedyJumpsRemaining = $50` (50 saltos BCD en la Sala de Tortura, frente a `$49` en JP, `prg.asm:734`).
  2. Tabla de condiciones de puertas ampliada a 33 entradas (`$00` a `$20`), incluyendo requisitos específicos como Mighty Level 3 en puertas `$07`, `$0E`, `$17`, `$18`, `$1B` (`prg.asm:6745-6780`).
  3. Textos, tipografía y secuencias de créditos en inglés de Tecmo USA (`data.asm:131`, `data.asm:440`).
  4. Lógica de evaluación de transferencias PPU en `HandlePPUUpdates` (`prg.asm:129-163`).

---

## 2. Ámbito Funcional Incluido (In-Scope)

La especificación funcional cubrirá el 100% de los siguientes subsistemas y comportamientos observables:

### 2.1. Ciclo de Vida y Máquina de Estados Global (`[CONFIRMADA]`)
- **Secuencia de Encendido (Boot)**: Inicialización de la CPU 6502, limpieza de memoria RAM `$0000-$07FF`, reseteo de la PPU 2C02 y transición a bucle infinito en espera de interrupciones (`prg.asm:8-81`).
- **Pipeline de Interrupción NMI (60 Hz)**: Secuencia estricta de 7 etapas intra-frame:
  1. Prólogo de preservación de registros.
  2. Transferencia OAM DMA (`$0200-$02FF` vía `$4014`).
  3. Vuelco de buffers de VRAM (`HandlePPUUpdates`, paletas y atributos).
  4. Lectura y latching de Joypads P1/P2 (`UpdateJoypads`).
  5. Despacho de la lógica principal (`MainLogicHandler`).
  6. Motor de sonido y mezcla de canales APU (`HandleSound`).
  7. Epílogo de retardo de un frame en copias de flanco de Joypad (`Joypad1_ImmediateCopy = Joypad1_Immediate`).
- **Estados de `MainLogicHandler`**:
  - `InGameFlag == 0`: `TitleScreenHandler` (pantalla de título, demo, selección de inicio).
  - `InGameFlag == 1`: `GameHandler` con sus 13 subestados (`GameState_0_Init` a `GameState_C_Ending`).

### 2.2. Cinemática, Física y Colisiones de Jack (`[CONFIRMADA]`)
- **Física Vertical de Salto**: Tabla parabólica discreta de 64 pasos (`word_C0E3` en `data.asm:1425`).
- **Aleteo (Flutter)**: Modulación sostenida al mantener pulsado el botón A (`sub_8AE8` en `prg.asm:1960`).
- **Movimiento Horizontal**: Aceleración, inercia y modulación de velocidad direccional (`word_C163` en `data.asm:1489`).
- **Sistema de Colisión 9-Probe**: Detección de colisiones mediante 9 sondas espaciales de sondeo contra los metatiles de fondo (`BigBeefySubHandlesBGColliding` en `prg.asm:2476`).
- **Niveles de Poder Mighty (0 a 3)**: Temporizador de decremento, activación de cofres y paletas de color asociadas (`UpdateMightyPalette` en `prg.asm:1710`).

### 2.3. Geometría de Salas, Metatiles y Secretos (`[CONFIRMADA]`)
- **Pirámide Completa**: Mapeo y decodificación de las 257 salas indexadas en `RoomDataPointers` (`data.asm:2227-2482`).
- **Decodificador 4-Tier**: Descompresión jerárquica de `RoomData` (Chunks $\to$ Bloques $\to$ Metatiles 16x16 $\to$ Baldosas CHR 8x8).
- **Puertas y Mecanismos**: Las 33 condiciones de apertura de puertas de la revisión USA (`prg.asm:6745`).
- **Objetos Interactivos**:
  - Cadena fija de 23 bombas por sala, detección de mecha encendida (Fire Bomb) y bonificaciones de 10.000 a 50.000 puntos.
  - Apertura de cofres de tesoro al saltar sobre ellos (Mighty Coins, Mighty Drink, Extra Life, Monedas rojas/azules y Sphinx).

### 2.4. Enemigos, Inteligencia Artificial y Spawns (`[CONFIRMADA]`)
- **6 Arquetipos Base**: Momia (Mummy), Hanezo (pájaro mecánico), GejiShogun (ciempiés), Gameido (tortuga), Dokuron (calavera) y Horus (pájaro sagrado).
- **Ciclo de Spawns**: Temporizador de aparición (`SpawnTimer`), coordenadas de entrada y slots de RAM (`EnemyStruct` de 28 bytes, `$0600-$07FF`).
- **Transformación de Momias**: Fórmula determinista de mutación: `(ContadorPulsacionesA % 6) + 2` (`prg.asm:8280`).
- **Power Coins**: Transmutación temporal de enemigos en monedas coleccionables durante 320 frames (`TimerEnemyTransmute`).

### 2.5. HUD, Aritmética BCD, Sala de Tortura y Finales (`[CONFIRMADA]`)
- **Aritmética BCD en 6502**: Rutinas de suma y resta decimal simulada sin modo decimal hardware (`AddScore`, `BCDCheckFirstDigit`, `BCDSub1FromA`).
- **Temporizador de Fase**: Divisor de 256 frames (`TimerFrames`) que decrementa el contador BCD de 60 segundos.
- **Sala de Tortura**: Penalización obligatoria de 50 saltos (`$50` BCD), detección de salto por flanco (`JustJumpedFlag`) y salida forzada con doble retorno `PLA; PLA`.
- **Cálculo del GDV (Game Determination Value)**: Fórmula de evaluación de comportamiento del jugador (`PlayerGDV`) y selección entre los 4 finales cinemáticos (Ending 1 a 4).

### 2.6. Motor de Sonido y Secuencias 2A03 (`[CONFIRMADA]`)
- **Sintetizador 2A03**: 2 canales Pulse con barrido de frecuencia, 1 canal Triangle lineal, 1 canal Noise pseudoaleatorio con LFSR de 15 bits.
- **Intérprete de Bytecode Tecmo**: Decodificación de secuencias musicales (`music-data.asm`), tablas de longitud de notas (`NoteLengthTable` en `$F39E`), envolventes de volumen (`SoundVolTable` en `$F3BE`) y cola de efectos de sonido con prioridad (`SoundQueue`).

---

## 3. Elementos Fuera de Alcance (Out-of-Scope)

1. **Detalles Físicos/Eléctricos del Silicio**: Voltajes de los pines del 2A03/2C02 que no tengan impacto lógico en la emulación o reimplementación de registros.
2. **Hacks y Mods No Oficiales**: Lógica que no forme parte del código ensamblador original de 1987.
3. **Round Select en Producción**: Aunque el desensamblado contiene directivas condicionales para un menú de selección de ronda (`ROUND_SELECT` en `mighty-bomb-jack.asm:36`), este venía desactivado en la ROM comercial de Tecmo USA (`WEAK_COPYRIGHT_CHECK = 0`), por lo que se catalogará como código condicional inactivo.

---

## 4. Clasificación Epistémica del Alcance

| Área de Alcance | Clasificación | Evidencia en Ensamblador |
| :--- | :---: | :--- |
| Revisión Canónica USA (`REV_US`) | `[CONFIRMADA]` | `asm/mighty-bomb-jack.asm:21-27` |
| Pipeline de Frame NMI a 60 Hz | `[CONFIRMADA]` | `asm/src/prg.asm:90-123` |
| Máquina de Estados (0..$C) | `[CONFIRMADA]` | `asm/src/prg.asm:300-317` |
| Curva de Salto de 64 Pasos | `[CONFIRMADA]` | `asm/src/data.asm:1425` (`word_C0E3`) |
| Colisiones 9-Probe | `[CONFIRMADA]` | `asm/src/prg.asm:2476-2615` |
| Decodificación de 257 Salas | `[CONFIRMADA]` | `asm/src/data.asm:2227` (`RoomDataPointers`) |
| Condiciones de Puertas USA (33) | `[CONFIRMADA]` | `asm/src/prg.asm:6745-6780` |
| Sala de Tortura (50 Saltos US) | `[CONFIRMADA]` | `asm/src/prg.asm:734` |
| Mutación de Momias `(A%6)+2` | `[CONFIRMADA]` | `asm/src/prg.asm:8280` |
| Aritmética BCD y GDV | `[CONFIRMADA]` | `asm/src/prg.asm:1095-1118` |
| Intérprete Musical 2A03 | `[CONFIRMADA]` | `asm/src/music-engine.asm:1-450` |
