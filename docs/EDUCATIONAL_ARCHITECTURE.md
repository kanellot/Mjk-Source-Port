# Guía Didáctica de Arquitectura: Cómo Funcionaba Mighty Bomb Jack (NES 1987)

Esta guía explica en profundidad los principios de ingeniería de software, arquitectura de sistemas y restricciones de hardware que definieron el diseño de *Mighty Bomb Jack* en la NES de 1987, y cómo se traduce este conocimiento a un diseño moderno, limpio y pedagógico en lenguaje C.

---

## 1. La CPU Ricoh 2A03 y el Paradigma Sin Sistema Operativo

En una consola NES no existe ningún sistema operativo, ni multitarea, ni recolección de basura, ni bibliotecas estándar.

### 1.1 La Pila de Solo 256 Bytes
- La CPU 6502 asigna un rango fijo de 256 bytes para la pila del hardware: `$0100` a `$01FF`.
- Cada instrucción `JSR` (Jump to Subroutine) apila 2 bytes (la dirección de retorno del Program Counter menos 1).
- Un anidamiento de llamadas superior a unos pocos niveles o un desbalance entre `PHA` (Push Accumulator) y `PLA` (Pull Accumulator) provocaba el desbordamiento catastrófico de la pila (*Stack Overflow* o *Stack Underflow*), lo que hacía que la CPU saltara a ejecutar memoria basura.

### 1.2 El Mecanismo de Aborto por Pila (`PLA; PLA; RTS`)
Para salir instantáneamente de situaciones complejas (como la muerte de Jack o la finalización de los 50 saltos de la sala de tortura), los programadores de Tecmo no usaban variables booleanas de retorno. Ejecutaban dos instrucciones `PLA` consecutivas para destruir el marco de llamada actual y saltar directamente al bucle superior con un solo `RTS`.
En nuestro código C moderno, esto se modela de forma limpia mediante códigos de acción o retornos enumerados (`ACTION_ABORT_TO_PARENT`), garantizando que la legibilidad sea transparente.

---

## 2. Física en Punto Fijo (Q8.8): ¿Por qué no se usaban `float`?

La CPU 2A03 solo opera nativamente con enteros de 8 bits (0 a 255). No tiene soporte para números decimales ni unidad de coma flotante (FPU).

Para que Jack pudiera acelerar gradualmente y deslizarse con suavidad, se utilizó **Aritmética en Punto Fijo Q8.8**:
- Un valor de posición horizontal o vertical ocupa 16 bits:
  - **Parte Alta (Bits 8..15)**: La coordenada entera en píxeles de pantalla ($0..255$).
  - **Parte Baja (Bits 0..7)**: La fracción subpíxel ($0..255$).
- Cada frame, al sumar una velocidad fraccionaria (por ejemplo, $+0.5$ píxeles por frame, representado como `0x0080`), la fracción se va acumulando hasta que desborda el byte bajo y suma $+1$ al píxel visible.

En nuestro código en C:
```c
typedef int16_t fixed8_t;
#define FIXED_FROM_INT(i)   ((fixed8_t)((i) << 8))
#define FIXED_TO_INT(f)     ((int8_t)((f) >> 8))

jack->y_subpixel += jack->velocity_y;
jack->screen_y = FIXED_TO_INT(jack->y_subpixel);
```

---

## 3. Compresión de 257 Salas en solo 17 KB: Indirección en 4 Niveles

Un mapa sin comprimir de una sala de NES ($32 \times 30$ tiles de $8 \times 8$) ocupa 960 bytes. Si multiplicamos 960 bytes por 257 salas, requeriría **246.720 bytes (246 KB)**, superando por mucho los 32 KB disponibles en todo el cartucho.

Para resolver esto, Tecmo empleó una pirámide de indirección jerárquica:

```
[Nivel 1: Índice de Sala] (257 entradas de 2 bytes -> 514 B)
             |
             v
[Nivel 2: Bloques de Layout de Sala] (Secuencia de índices de 8x8 metatiles)
             |
             v
[Nivel 3: Metatiles de 16x16 píxeles] (Cada uno agrupa 4 tiles de la PPU)
             |
             v
[Nivel 4: Tiles de 8x8 píxeles en CHR-ROM] (Los gráficos elementales de 16 bytes)
```

Gracias a este diseño:
- Las paredes de ladrillo, cofres y pirámides se reutilizan de forma masiva en múltiples salas cambiando únicamente su paleta de color.
- Las 257 salas completas caben en **menos de 17.000 bytes**.

---

## 4. Aritmética BCD (*Binary Coded Decimal*): Puntuaciones sin División

Para mostrar la puntuación de Jack (por ejemplo, `015420` puntos), un ordenador moderno almacena el número en binario y lo divide sucesivamente entre 10 para obtener los caracteres a imprimir.

En la NES, una división por 10 en software toma más de 100 ciclos de reloj, lo cual es inasumible en medio del bucle a 60 Hz.
Por ello, la puntuación se almacena en **Decimal Codificado en Binario (BCD)**:
- Cada byte almacena dos dígitos decimales (4 bits por dígito):
  - El byte `0x54` no representa el número cincuenta y cuatro en binario, sino literalmente los dígitos `'5'` y `'4'`.
- Al sumar puntos (por ejemplo, $+100$ puntos), se ejecuta una suma especializada con corrección decimal (`ADC` con `DAA` o lógica manual), actualizando directamente los caracteres de la pantalla sin realizar ninguna división.

---

## 5. El Sintetizador Chiptune APU 2A03

La NES no reproducía pistas de audio pregrabadas (como archivos MP3 o WAV). En su lugar, el procesador incluye un sintetizador analógico con 5 generadores de forma de onda:
1. **Pulse 1 y Pulse 2 (Ondas Cuadradas)**: Generan timbres brillantes y metálicos modulando el ciclo de trabajo (*duty cycle*: 12.5%, 25%, 50%, 75%). Interpretan la melodía principal y los arpegios.
2. **Triangle (Onda Triangular)**: Una onda más suave, libre de armónicos estridentes, utilizada para las líneas de bajo y notas graves.
3. **Noise (Ruido Pseudoaleatorio)**: Un generador de ruido blanco basado en un registro de desplazamiento con retroalimentación lineal (LFSR de 15 bits). Produce sonidos de percusión, platillos y explosiones al abrir cofres o derrotar enemigos.
4. **DMC (Delta Modulation Channel)**: Canal de reproducción de muestras comprimidas en 1 bit (no utilizado por *Mighty Bomb Jack* para ahorrar memoria).

En nuestro source port, el sintetizador `mjk_apu.c` recrea matemáticamente estas 4 ondas en un buffer de sonido PCM a 44.1 kHz, logrando un audio idéntico al cartucho original.
