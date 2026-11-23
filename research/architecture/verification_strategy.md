# Estrategia de Verificación Comportamental en 3 Capas

**Proyecto**: Mighty Bomb Jack — Source Port C11 (Windows & Linux)  
**Ubicación**: `research/architecture/verification_strategy.md`  
**Decisión Asociada**: `ADR-017` en [`project/DECISIONS.md`](../../project/DECISIONS.md)

---

## 1. Por qué NO usamos Replay TAS Largo

El replay de sesiones TAS largas (de varios miles de frames) exige **sincronización exacta ciclo-a-frame** con el hardware de 1987: cualquier diferencia de 1 frame (como abstraer un *stall* de 8 frames con la PPU apagada o un redondeo de subpíxel) dispara un efecto mariposa que rompe todo el resto de la grabación y genera falsos negativos en cascada.

Dado que el objetivo es conseguir **una experiencia jugable muy fiel con código C11 limpio, educativo, extensible y con abstracciones sanas**, se adopta una **verificación comportamental desacoplada en 3 capas**.

---

## 2. Arquitectura de las 3 Capas

### Capa 1 — Tests Unitarios C11 (`tests/c/` + `CTest`)
- **Velocidad**: < 1 segundo en Windows y Linux.
- **Alcance**:
  - Extractor BYOR v2 integrado y validación SHA-1 / CRC32 contra la ROM.
  - Decodificación completa de las **257 salas** (metatiles, puertas, bombas, cofres y paletas) comparada contra fixtures exportados desde la ROM.
  - Contratos puros: curva parabólica de 64 pasos, *flutter*, colisiones 9-probe, las 33 condiciones de puertas, cadena de mechas, mutación de enemigos `(A % 6) + 2`, aritmética BCD, 50 saltos de Torture Room, GDV / 4 finales y decodificador de bytecode APU.

### Capa 2 — Oráculo NES (`tools/nes_oracle.py`) y Micro-Escenarios Semánticos (`tools/verify_scenarios.py`)
- **Escenarios independientes cortos (60–240 frames)**, nunca streams ciegos de minutos:
  - Ejemplos: *Title -> Start*, *Salto + Flutter*, *Recogida de bomba + Puff + Score*, *Cofre + Power-Up*, *Puerta + Transición Sala 1->2*, *Scroll en pasillo*, *Spawn y mutación de enemigos*, *Bomb Room*, *Torture Room*.
- **Qué compara** (sin efecto mariposa ni falsos positivos de paleta RGB):
  1. **Resultados jugables** en el núcleo C (posición/altura de salto con tolerancia, puertas abiertas, conteo de bombas/enemigos, puntuación BCD).
  2. **Estado PPU semántico**: `OAM[0..63]` `(X, Y, tile, attr)` e índices de paleta NES (`$3F00-$3F1F`), evitando falsos positivos por variaciones de conversión RGB NTSC.
  3. **Eventos APU semánticos**: IDs de pista BGM, SFX disparados y actividad de canales 2A03.
  4. **Inspección visual puntual**: capturas 3-paneles (NES | Port C | Pixel Diff) vía `save_3panel_diff_png()` en Pillow headless.

### Capa 3 — Debug Overlay SDL2 Integrado (`F1`–`F4`)
- Para verificación humana directa en ejecución real a 60 Hz:
  - **`F1`**: Telemetría en vivo, estado de sala y visualización de hitboxes / sondas 9-probe.
  - **`F2`**: Selector/warp directo a cualquier sala (`0..256`), Bomb Room, Torture Room o Finales.
  - **`F3`**: Modificadores de prueba (invencibilidad, nivel Mighty `0..3`, timer).
  - **`F4`**: Pausa y avance frame a frame.
