@echo off
REM ============================================================
REM  Compila BarberShop POS en Windows y crea la carpeta dist\
REM  Solo necesitas tener MSYS2 instalado (https://www.msys2.org)
REM  Doble clic en este archivo.
REM ============================================================
setlocal
cd /d "%~dp0"

set "MSYS="
if exist "C:\msys64\usr\bin\bash.exe" set "MSYS=C:\msys64"
if not defined MSYS if exist "D:\msys64\usr\bin\bash.exe" set "MSYS=D:\msys64"
if not defined MSYS if defined MSYS2_ROOT if exist "%MSYS2_ROOT%\usr\bin\bash.exe" set "MSYS=%MSYS2_ROOT%"

if not defined MSYS (
    echo.
    echo [ERROR] No se encontro MSYS2.
    echo Instalalo desde https://www.msys2.org ^(ruta por defecto C:\msys64^)
    echo y vuelve a ejecutar este archivo.
    echo.
    pause
    exit /b 1
)

echo Usando MSYS2 en: %MSYS%
set "MSYSTEM=MINGW64"
set "CHERE_INVOKING=1"
"%MSYS%\usr\bin\bash.exe" -lc "./compilar_windows.sh"
if errorlevel 1 (
    echo.
    echo [ERROR] Fallo la compilacion. Revisa los mensajes de arriba.
    echo.
    pause
    exit /b 1
)

echo.
echo Abriendo carpeta dist\ ...
start "" "%~dp0dist"
pause
