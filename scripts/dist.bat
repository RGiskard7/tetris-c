@echo off
setlocal enabledelayedexpansion
REM Empaqueta Tetris en una carpeta portable (dist)
REM Copia el ejecutable, la unica DLL que necesita (allegro_monolith) y los
REM recursos. La carpeta resultante se puede comprimir y ejecutar en cualquier
REM Windows 10/11 sin instalar Allegro ni MinGW.
REM Autor: RGiskard7

cd /d "%~dp0\.."

set "ALLEGRO_BIN=C:\allegro-5.2.9.1-mingw-14.1.0\bin"
set "DIST=dist"

echo ===================================================
echo        Empaquetando Tetris (dist)
echo ===================================================
echo.

REM 1. Compilar primero
set "PATH=C:\mingw64\bin;%PATH%"
echo [INFO] Compilando...
mingw32-make -f Makefile clean 2>nul
mingw32-make -f Makefile
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] La compilacion fallo.
    pause
    exit /b 1
)

REM 2. Preparar carpeta limpia
if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%"

REM 3. Copiar ejecutable y la DLL de Allegro
copy "tetris.exe" "%DIST%\" >nul
if exist "%ALLEGRO_BIN%\allegro_monolith-5.2.dll" (
    copy "%ALLEGRO_BIN%\allegro_monolith-5.2.dll" "%DIST%\" >nul
) else (
    echo [AVISO] No se encontro allegro_monolith-5.2.dll en %ALLEGRO_BIN%
)

REM 4. Copiar todos los recursos del juego
xcopy "resources" "%DIST%\resources" /e /i /q >nul

echo.
echo [OK] Carpeta portable lista en: %DIST%
echo Comprimela y compartela; el .exe corre sin instalar nada.
echo.
pause
