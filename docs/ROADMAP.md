# Hoja de Ruta (Roadmap)

El port de *Mighty Bomb Jack* (`REV_US`) es completamente jugable de principio a fin. Esta lista recoge mejoras opcionales de calidad de vida y portabilidad para futuras iteraciones:

- **Integración en `main` y Release**: fusionar la rama `refactor` en `main` y generar release multiplataforma (añadiendo build de macOS al flujo de GitHub Actions si se desea).
- **Configuración de usuario**: archivo de configuración opcional para remapeo de teclas/mando, volumen de audio y factor de escala inicial de ventana.
- **Opciones de vídeo**: filtros opcionales (aspect ratio 4:3 NTSC vs píxeles cuadrados, scanlines suaves opcionales).
- **Soporte de otras revisiones (opcional)**: compatibilidad con `REV_JP` / `REV_JPA` (Famicom 1986) mediante detección automática por SHA-1.
- **Portabilidad adicional**: compilación a WebAssembly (Emscripten) reutilizando la capa `src/game/` + `src/render/` + `src/audio/` sin cambios.
