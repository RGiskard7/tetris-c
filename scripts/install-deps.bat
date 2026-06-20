@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo    Tetris - Instalador de dependencias (Windows)
echo ===================================================
echo.
echo Este script descargara e instalara MinGW y Allegro en:
echo   - C:\mingw64
echo   - C:\allegro-5.2.9.1-mingw-14.1.0
echo.
echo NOTA: Requiere permisos de administrador para escribir en C:\
echo.

REM URLs de descarga (Direct links)
REM WinLibs GCC 14.1.0 (UCRT, POSIX, SEH)
set "MINGW_URL=https://github.com/brechtsanders/winlibs_mingw/releases/download/14.1.0posix-18.1.5-11.0.1-ucrt-r1/winlibs-x86_64-posix-seh-gcc-14.1.0-mingw-w64ucrt-11.0.1-r1.zip"
REM Allegro 5.2.9 (binarios compatibles) - Renombraremos la carpeta
set "ALLEGRO_URL=https://github.com/liballeg/allegro5/releases/download/5.2.9.0/allegro-x86_64-w64-mingw32-gcc-13.2.0-posix-seh-static-5.2.9.0.zip"

REM 1. Instalar MinGW
if exist "C:\mingw64\bin\gcc.exe" (
    echo [OK] MinGW ya esta instalado en C:\mingw64
) else (
    echo [INFO] Descargando MinGW 14.1.0...
    curl -L -o mingw.zip "%MINGW_URL%"

    echo [INFO] Extrayendo MinGW en C:\ ...
    tar -xf mingw.zip -C C:\

    del mingw.zip
    echo [OK] MinGW instalado.
)

REM 2. Instalar Allegro
if exist "C:\allegro-5.2.9.1-mingw-14.1.0\include\allegro5\allegro.h" (
    echo [OK] Allegro ya esta instalado en C:\allegro-5.2.9.1-mingw-14.1.0
) else (
    echo [INFO] Descargando Allegro 5.2.9...
    curl -L -o allegro.zip "%ALLEGRO_URL%"

    echo [INFO] Extrayendo Allegro...
    tar -xf allegro.zip

    if exist "allegro" (
        move "allegro" "C:\allegro-5.2.9.1-mingw-14.1.0"
        echo [OK] Allegro instalado.
    ) else (
        echo [ERROR] No se pudo encontrar la carpeta extraida de Allegro.
        pause
        exit /b 1
    )

    del allegro.zip
)

echo.
echo ===================================================
echo    CONFIGURACION FINAL
echo ===================================================
echo.
echo Para que todo funcione, C:\mingw64\bin debe estar en tu PATH.
echo Agregando temporalmente al PATH actual...
set "PATH=C:\mingw64\bin;%PATH%"

echo.
echo Verificando versiones:
gcc --version
echo.
echo [EXITO] Instalacion completada.
echo Ahora puedes ejecutar: scripts\build.bat run
echo.
pause
