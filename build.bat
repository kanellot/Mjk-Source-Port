@echo off
setlocal enabledelayedexpansion

:: =====================================================================
:: Mighty Bomb Jack - Native Source Port (PC) Build Script
:: =====================================================================

set "MJK_TARGET=%~1"
if "%MJK_TARGET%"=="" set "MJK_TARGET=build"

if /i "%MJK_TARGET%"=="clean" goto do_clean

:: =====================================================================
:: 1. Verificación y Auto-Descarga de Dependencias (SDL2 2.30.8 VC)
:: =====================================================================
if exist "third_party\sdl2\SDL2-2.30.8\include\SDL.h" goto sdl2_ok

echo =====================================================================
echo   AUTO-DESCARGA DE DEPENDENCIAS (SDL2 v2.30.8 VC)
echo =====================================================================
echo [SETUP]: No se encontro SDL2 en third_party\sdl2\SDL2-2.30.8.
echo [SETUP]: Descargando paquete oficial de desarrollo desde GitHub...
if not exist third_party mkdir third_party
if not exist third_party\sdl2 mkdir third_party\sdl2

where curl.exe >nul 2>&1
if not errorlevel 1 (
    curl.exe -L "https://github.com/libsdl-org/SDL/releases/download/release-2.30.8/SDL2-devel-2.30.8-VC.zip" -o "third_party\sdl2_temp.zip"
) else (
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/libsdl-org/SDL/releases/download/release-2.30.8/SDL2-devel-2.30.8-VC.zip' -OutFile 'third_party\sdl2_temp.zip'"
)
if errorlevel 1 (
    echo [ERROR]: Fallo al descargar SDL2.
    exit /b 1
)

echo [SETUP]: Descomprimiendo archivos en third_party\sdl2...
where tar.exe >nul 2>&1
if not errorlevel 1 (
    tar.exe -xf "third_party\sdl2_temp.zip" -C "third_party\sdl2"
) else (
    powershell -Command "Expand-Archive -Path 'third_party\sdl2_temp.zip' -DestinationPath 'third_party\sdl2' -Force"
)
del /q "third_party\sdl2_temp.zip" >nul 2>&1

if not exist "third_party\sdl2\SDL2-2.30.8\include\SDL.h" (
    echo [ERROR]: No se pudo instalar SDL2 en third_party\sdl2\SDL2-2.30.8.
    exit /b 1
)
echo [OK]: SDL2 v2.30.8 instalado con exito en third_party\sdl2\SDL2-2.30.8.
echo.

:sdl2_ok

:: =====================================================================
:: Verificación y Extracción Automática de Assets (Clean-Room BYOR)
:: =====================================================================
if not exist "assets\assets.dat" (
    set "AUTO_ROM="
    if exist "mbj-us.nes" set "AUTO_ROM=mbj-us.nes"
    if not defined AUTO_ROM (if exist "asm\bin\mbj-us.nes" set "AUTO_ROM=asm\bin\mbj-us.nes")
    if defined AUTO_ROM (
        echo =====================================================================
        echo   EXTRACCION AUTOMATICA DE ASSETS (CLEAN-ROOM BYOR)
        echo =====================================================================
        echo [SETUP]: ROM oficial detectada en !AUTO_ROM!. Extrayendo assets.dat...
        python tools\extract_assets.py "!AUTO_ROM!"
        echo.
    )
)

if /i "!MJK_TARGET!"=="deps" (
    echo [OK]: Todas las dependencias estan instaladas y listas.
    exit /b 0
)

if /i "!MJK_TARGET!"=="extract" goto do_extract

:: =====================================================================
:: 2. Inicialización del Entorno de Compilación MSVC x64
:: =====================================================================
set "VCVARS_BAT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "!VCVARS_BAT!" (
    set "VCVARS_BAT=%ProgramFiles%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)
if not exist "!VCVARS_BAT!" (
    set "VCVARS_BAT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if not exist "!VCVARS_BAT!" (
    set "VCVARS_BAT=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)
if not exist "!VCVARS_BAT!" (
    set "VCVARS_BAT=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

if not exist "!VCVARS_BAT!" (
    echo [ERROR]: No se encuentra vcvars64.bat en las rutas habituales de Visual Studio.
    exit /b 1
)

call "!VCVARS_BAT!" >nul 2>&1
if errorlevel 1 (
    echo [ERROR]: Fallo al inicializar el entorno de compilacion MSVC x64.
    exit /b 1
)

if /i "!MJK_TARGET!"=="build" goto do_build
if /i "!MJK_TARGET!"=="run"   goto do_run
goto do_usage

:do_clean
echo [CLEAN]: Eliminando artefactos de compilacion...
if exist bin rd /s /q bin
if exist build rd /s /q build
if exist *.obj del /q *.obj
echo [OK]: Limpieza completada.
exit /b 0

:do_extract
set "ROM_PATH=%~2"
if "%ROM_PATH%"=="" set "ROM_PATH=mbj-us.nes"
if not exist "!ROM_PATH!" (
    if exist asm\bin\mbj-us.nes set "ROM_PATH=asm\bin\mbj-us.nes"
)
if not exist "!ROM_PATH!" (
    echo [ERROR]: No se encuentra la ROM en '!ROM_PATH!'.
    echo [USO]: build.bat extract [ruta_a_mbj-us.nes]
    exit /b 1
)
python tools\extract_assets.py "!ROM_PATH!"
exit /b %errorlevel%

:do_build
echo =====================================================================
echo   COMPILANDO MIGHTY BOMB JACK SOURCE PORT (BIN\MJK.EXE)
echo =====================================================================
if not exist bin mkdir bin

if exist third_party\sdl2\SDL2-2.30.8\lib\x64\SDL2.dll (
    copy /y third_party\sdl2\SDL2-2.30.8\lib\x64\SDL2.dll bin\SDL2.dll >nul
)

cl /nologo /W4 /O2 /Iinclude /Ithird_party\sdl2\SDL2-2.30.8\include ^
   src\main.c ^
   src\platform\mjk_platform_sdl.c ^
   src\core\mjk_game.c ^
   src\core\mjk_state_machine.c ^
   src\core\mjk_player.c ^
   src\core\mjk_camera.c ^
   src\core\mjk_room_decoder.c ^
   src\core\mjk_room_data.c ^
   src\core\mjk_ppu.c ^
   src\core\mjk_enemy.c ^
   src\core\mjk_hud.c ^
   src\core\mjk_apu.c ^
   src\core\mjk_assets.c ^
   /Fe:bin\mjk.exe /Fo:bin\ ^
   /link /LIBPATH:third_party\sdl2\SDL2-2.30.8\lib\x64 SDL2.lib SDL2main.lib shell32.lib /SUBSYSTEM:CONSOLE
if errorlevel 1 (
    echo [ERROR]: Fallo en la compilacion de bin\mjk.exe
    del /q /f bin\*.obj *.obj >nul 2>&1
    exit /b 1
)

echo [BUILD]: Compilando biblioteca compartida bin\mjk_core.dll para herramientas Python...
cl /nologo /W4 /O2 /LD /Iinclude ^
   src\core\mjk_oracle_api.c ^
   src\core\mjk_game.c ^
   src\core\mjk_state_machine.c ^
   src\core\mjk_player.c ^
   src\core\mjk_camera.c ^
   src\core\mjk_room_decoder.c ^
   src\core\mjk_room_data.c ^
   src\core\mjk_ppu.c ^
   src\core\mjk_enemy.c ^
   src\core\mjk_hud.c ^
   src\core\mjk_apu.c ^
   src\core\mjk_assets.c ^
   /Fe:bin\mjk_core.dll /Fo:bin\
if errorlevel 1 (
    echo [ERROR]: Fallo en la compilacion de bin\mjk_core.dll
    del /q /f bin\*.obj bin\*.exp bin\*.lib *.obj >nul 2>&1
    exit /b 1
)

:: Limpieza automática de archivos .obj intermedios y auxiliares de DLL
del /q /f bin\*.obj bin\*.exp bin\*.lib *.obj >nul 2>&1

if exist assets\assets.dat (
    if not exist bin\assets mkdir bin\assets
    copy /y assets\assets.dat bin\assets\assets.dat >nul
    copy /y assets\assets.dat bin\assets.dat >nul
)

echo.
echo [OK]: Compilacion exitosa y limpia. Binario final generado en bin\mjk.exe (0 archivos .obj residuales)
exit /b 0

:do_run
call "%~f0" build
if errorlevel 1 exit /b 1
echo [RUN]: Ejecutando bin\mjk.exe...
start bin\mjk.exe
exit /b 0

:do_usage
echo [INFO]: Comando '!MJK_TARGET!' no reconocido. Uso: build.bat [build^|run^|deps^|extract^|clean]
exit /b 1
