@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo   Mighty Bomb Jack (NES) Native PC Port - Build Harness
echo =====================================================================

:: Parse CLI arguments: build.bat [generator: vs|ninja] [config: Release|Debug] [action: build|test|clean|run]
set "GENERATOR_ARG=%~1"
set "CONFIG_ARG=%~2"
set "ACTION_ARG=%~3"

if "%GENERATOR_ARG%"=="" set "GENERATOR_ARG=vs"
if "%CONFIG_ARG%"=="" set "CONFIG_ARG=Release"
if "%ACTION_ARG%"=="" set "ACTION_ARG=build"

:: Handle clean action early
if /i "%GENERATOR_ARG%"=="clean" (
    echo [CLEAN] Removing build directories...
    if exist build_vs rd /s /q build_vs
    if exist build_ninja rd /s /q build_ninja
    if exist build rd /s /q build
    echo [CLEAN] Clean complete.
    exit /b 0
)

:: Locate Visual Studio Installation via vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_DIR="

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_DIR=%%i"
    )
)

:: Setup tools paths
set "CMAKE_BIN=cmake"
set "CTEST_BIN=ctest"
set "NINJA_BIN=ninja"
set "VCVARS_BAT="

if defined VS_DIR (
    if exist "!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS_BAT=!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat"
    )
    if exist "!VS_DIR!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_BIN=!VS_DIR!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        set "CTEST_BIN=!VS_DIR!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
    )
    if exist "!VS_DIR!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
        set "NINJA_BIN=!VS_DIR!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    )
)

:: Check if CMake is reachable
"%CMAKE_BIN%" --version >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake executable was not found. Please install CMake or VS Build Tools.
    exit /b 1
)

:: Branch by Generator
if /i "%GENERATOR_ARG%"=="ninja" goto run_ninja
goto run_vs

:: -------------------------------------------------------------------------
:: Ninja Generator Execution
:: -------------------------------------------------------------------------
:run_ninja
echo [INFO] Selected Generator: Ninja (Single-Config: %CONFIG_ARG%)
set "BUILD_DIR=build_ninja"

where cl >nul 2>nul
if %errorlevel% neq 0 (
    if defined VCVARS_BAT (
        echo [INFO] Activating MSVC x64 Environment via vcvars64.bat...
        call "!VCVARS_BAT!"
    ) else (
        echo [ERROR] cl.exe not in PATH and vcvars64.bat not found.
        exit /b 1
    )
)

if /i "%ACTION_ARG%"=="test" goto run_ninja_tests
if /i "%ACTION_ARG%"=="run" goto run_binary

echo [1/3] Configuring with Ninja...
"%CMAKE_BIN%" -G Ninja -S . -B !BUILD_DIR! -DCMAKE_BUILD_TYPE=%CONFIG_ARG% -DCMAKE_MAKE_PROGRAM="!NINJA_BIN!"
if !errorlevel! neq 0 exit /b !errorlevel!

echo [2/3] Building with Ninja...
"%CMAKE_BIN%" --build !BUILD_DIR!
if !errorlevel! neq 0 exit /b !errorlevel!

:run_ninja_tests
echo [3/3] Running Headless Test Suite (ctest)...
"%CTEST_BIN%" --test-dir !BUILD_DIR! --output-on-failure
if !errorlevel! neq 0 exit /b !errorlevel!

goto finish

:: -------------------------------------------------------------------------
:: Visual Studio MSBuild Generator Execution
:: -------------------------------------------------------------------------
:run_vs
echo [INFO] Selected Generator: Visual Studio MSBuild (Multi-Config: %CONFIG_ARG%)
set "BUILD_DIR=build_vs"

if /i "%ACTION_ARG%"=="test" goto run_vs_tests
if /i "%ACTION_ARG%"=="run" goto run_binary

echo [1/3] Configuring Visual Studio Solution...
"%CMAKE_BIN%" -S . -B !BUILD_DIR!
if !errorlevel! neq 0 exit /b !errorlevel!

echo [2/3] Compiling %CONFIG_ARG% Configuration...
"%CMAKE_BIN%" --build !BUILD_DIR! --config %CONFIG_ARG%
if !errorlevel! neq 0 exit /b !errorlevel!

:run_vs_tests
echo [3/3] Running Headless Test Suite (ctest)...
"%CTEST_BIN%" --test-dir !BUILD_DIR! -C %CONFIG_ARG% --output-on-failure
if !errorlevel! neq 0 exit /b !errorlevel!

goto finish

:: -------------------------------------------------------------------------
:: Run Desktop Executable
:: -------------------------------------------------------------------------
:run_binary
set "EXE_PATH=!BUILD_DIR!\bin\%CONFIG_ARG%\mighty_bomb_jack.exe"
if not exist "!EXE_PATH!" set "EXE_PATH=!BUILD_DIR!\bin\mighty_bomb_jack.exe"
if exist "!EXE_PATH!" (
    echo [LAUNCH] Starting !EXE_PATH!...
    start "" "!EXE_PATH!"
) else (
    echo [ERROR] Game binary not found at !EXE_PATH!. Please run build first.
    exit /b 1
)

:finish
echo =====================================================================
echo   Build and Test Verification Succeeded!
echo =====================================================================
endlocal
