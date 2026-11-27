# Historial de Cambios (Changelog)

Resumen de los hitos principales del desarrollo del source port de *Mighty Bomb Jack* en C11.

---

## v1.0 — Refactor Arquitectónico C11 Moderno (Rama `refactor`)

### Arquitectura y Código
- Reestructuración del motor en **5 capas desacopladas**: `src/assets/`, `src/game/`, `src/render/`, `src/audio/` y `src/platform/`.
- Eliminación de convenciones heredadas del 6502 en la lógica de juego: física en `float` IEEE-754 exacta, contadores en enteros estándar (`uint32_t`/`int`), estructuras semánticas y renderizador solo-lectura (`const MjkGame*`).
- Extracción y pre-decodificación única de la ROM al arrancar (`src/assets/asset_bank.c`): 512 tiles `8×8` en caché L1 (32 KB), tablas de física/velocidad en `float`, tablas de puntuación en `uint32_t` y las 256 salas ya decodificadas.
- Separación total de código de producción y pruebas: eliminado cualquier código de debug/verificación del ejecutable `mjk`.

### Rendimiento y Audio
- Sintetizador NES 2A03 optimizado con tablas LUT (`pulse_lut`/`tnd_lut`, error `< 1e-6`).
- Acumulador fraccional NTSC (`733`/`734` muestras/frame a `44.1 kHz`) para sincronía exacta sin deriva ni cortes en SDL2.
- `MjkInputLatch` para preservar flancos de pulsación en bucles de paso fijo (`60.0988 Hz`).

### Fidelidad y Correcciones
- Corrección de paridad frente al juego original: metatiles de puerta abierta, entrada por puertas en suelo, estabilidad de paleta en pasillos, bonus de puntuación en GDV, ventana de temporizador en cero y reinicio de sección.

### Testing, Plataformas y Documentación
- Suite de **11 tests unitarios C11 (`CTest`)** con vectores de referencia (*Golden Trajectories* y *Golden Frame Checks*), `SKIP_RETURN_CODE 77` sin ROM y soporte de ASan/UBSan.
- Documentación de compilación en **macOS** (contribución PR #1).
- Documentación unificada y reorganizada bajo `docs/` y `docs/nes-reference/`.

---

## v0.9 — Implementación Completa Inicial (`main`)

- Implementación completa del juego `REV_US` de principio a fin: Copyright, Title Screen, 17 rondas (256 salas), Bomb Rooms, Torture Room (50 saltos US), 33 condiciones de puerta, 6 arquetipos de enemigos, Mighty Power, 4 finales y Game Over.
- Extractor integrado Clean-Room BYOR v2 en C11 con validación SHA-1 y caché `assets.dat` con CRC-32.
- Pipeline CI/CD en GitHub Actions para Windows y Linux.
