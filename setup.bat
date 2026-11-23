@echo off
setlocal
echo =====================================================================
echo  Mighty Bomb Jack - Preparacion Automatica de Entorno y Dependencias
echo =====================================================================
call "%~dp0build.bat" deps
if errorlevel 1 (
    echo [ERROR]: Fallo al instalar las dependencias.
    exit /b 1
)
echo [OK]: Entorno listo. Ahora puedes ejecutar: build.bat build
exit /b 0
