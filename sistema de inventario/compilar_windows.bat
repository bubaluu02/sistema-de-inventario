@echo off
REM ============================================================
REM  Script de compilacion para BarberShop POS en Windows
REM  Ejecutar desde MSYS2 MinGW64 Shell o cmd despues de instalar MSYS2
REM ============================================================

echo.
echo ============================================================
echo   BARBERSHOP POS - Compilador para Windows
echo ============================================================
echo.

REM Check if we are inside MSYS2 by looking for pkg-config
where pkg-config >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] No se encontro pkg-config.
    echo.
    echo Debes ejecutar este script desde la terminal MSYS2 MinGW64.
    echo O asegurate de que MSYS2 esta en tu PATH.
    echo.
    echo Instrucciones rapidas:
    echo   1. Instala MSYS2 desde https://www.msys2.org
    echo   2. Abre "MSYS2 MinGW 64-bit" desde el menu Inicio
    echo   3. Ejecuta: pacman -S mingw-w64-x86_64-gtkmm-4.0 mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc
    echo   4. Navega a esta carpeta y ejecuta este script
    echo.
    pause
    exit /b 1
)

echo [OK] Entorno de compilacion detectado.
echo.

if not exist build mkdir build
cd build

echo [1/2] Configurando proyecto con CMake...
cmake -G "MinGW Makefiles" ..
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Fallo la configuracion de CMake.
    pause
    exit /b 1
)

echo.
echo [2/2] Compilando...
cmake --build . -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Fallo la compilacion.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo   COMPILACION EXITOSA
echo   Ejecutable: build\barberia_pos.exe
echo ============================================================
echo.
echo Para ejecutar: build\barberia_pos.exe
echo.
pause
