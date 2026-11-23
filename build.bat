@echo off
setlocal enabledelayedexpansion

:: =====================================================================
:: Mighty Bomb Jack - Wrapper de Build Windows (CMake + Ninja/MSVC)
:: =====================================================================

set "MJK_TARGET=%~1"
if "%MJK_TARGET%"=="" set "MJK_TARGET=build"

if /i "%MJK_TARGET%"=="clean" goto do_clean

:: 1. Verificar / Auto-descargar SDL2 v2.30.8 VC si falta
if exist "third_party\sdl2\SDL2-2.30.8\include\SDL.h" goto sdl2_ok

echo [SETUP]: Descargando SDL2 v2.30.8 VC en third_party\sdl2...
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

where tar.exe >nul 2>&1
if not errorlevel 1 (
    tar.exe -xf "third_party\sdl2_temp.zip" -C "third_party\sdl2"
) else (
    powershell -Command "Expand-Archive -Path 'third_party\sdl2_temp.zip' -DestinationPath 'third_party\sdl2' -Force"
)
del /q "third_party\sdl2_temp.zip" >nul 2>&1

if not exist "third_party\sdl2\SDL2-2.30.8\include\SDL.h" (
    echo [ERROR]: No se pudo instalar SDL2.
    exit /b 1
)
echo [OK]: SDL2 v2.30.8 instalado en third_party\sdl2\SDL2-2.30.8.

:sdl2_ok
if /i "%MJK_TARGET%"=="deps" (
    echo [OK]: Dependencias listas.
    exit /b 0
)

:: 2. Inicializar MSVC x64 + CMake + Ninja si no estan en PATH
where cmake >nul 2>&1
if not errorlevel 1 (
    where cl >nul 2>&1
    if not errorlevel 1 goto env_ready
)

set "VCVARS_BAT="
for %%P in (
    "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if not defined VCVARS_BAT if exist %%P set "VCVARS_BAT=%%~P"
)

if not defined VCVARS_BAT (
    echo [ERROR]: No se encuentra vcvars64.bat en las rutas habituales de Visual Studio.
    exit /b 1
)

call "!VCVARS_BAT!" >nul 2>&1
if errorlevel 1 (
    echo [ERROR]: Fallo al inicializar vcvars64.bat.
    exit /b 1
)

:env_ready
if /i "%MJK_TARGET%"=="build" goto do_build
if /i "%MJK_TARGET%"=="test"  goto do_test
if /i "%MJK_TARGET%"=="run"   goto do_run
goto do_usage

:do_clean
if exist build rd /s /q build
echo [OK]: Limpieza de build\ completada.
exit /b 0

:do_configure_and_build
set "CMAKE_GEN_OPT="
where ninja >nul 2>&1
if not errorlevel 1 set "CMAKE_GEN_OPT=-G Ninja"
cmake -S . -B build %CMAKE_GEN_OPT% -DCMAKE_BUILD_TYPE=RelWithDebInfo
if errorlevel 1 exit /b 1
cmake --build build --config RelWithDebInfo
exit /b %errorlevel%

:do_build
call :do_configure_and_build
exit /b %errorlevel%

:do_test
call :do_configure_and_build
if errorlevel 1 exit /b 1
ctest --test-dir build --output-on-failure -C RelWithDebInfo
exit /b %errorlevel%

:do_run
call :do_configure_and_build
if errorlevel 1 exit /b 1
if exist "build\bin\mjk.exe" (
    "build\bin\mjk.exe"
) else if exist "build\bin\RelWithDebInfo\mjk.exe" (
    "build\bin\RelWithDebInfo\mjk.exe"
) else (
    echo [ERROR]: No se encontro el ejecutable generado.
    exit /b 1
)
exit /b %errorlevel%

:do_usage
echo Uso: build.bat [build^|test^|run^|deps^|clean]
exit /b 1
