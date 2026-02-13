@echo off
setlocal EnableDelayedExpansion

echo.
echo =================================================
echo    COMTerminal - Release build (Ninja + MSVC)
echo =================================================
echo.

:: Путь к vcvarsall.bat — подставь свой, если отличается
set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %VCVARS% (
    echo Ошибка: не найден vcvarsall.bat
    echo Проверь путь: %VCVARS%
    echo.
    pause
    exit /b 1
)

echo [1/4] Настройка окружения Visual Studio 2022 (x64)...
call %VCVARS% x64 >nul
if %ERRORLEVEL% neq 0 (
    echo Ошибка при вызове vcvarsall.bat
    pause
    exit /b 1
)

echo Окружение настроено.

:: Папка сборки
set BUILD_DIR=build

:: Удаляем старую папку, если хочешь чистую пересборку (раскомментируй, если нужно)
:: if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%

echo.
echo [2/4] Конфигурация CMake (Ninja + Release по умолчанию)...
cmake -S . -B %BUILD_DIR% ^
    -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded

if %ERRORLEVEL% neq 0 (
    echo.
    echo Ошибка конфигурации CMake!
    pause
    exit /b 1
)

echo.
echo [3/4] Сборка Release...
cmake --build %BUILD_DIR% --config Release --parallel

if %ERRORLEVEL% neq 0 (
    echo.
    echo Ошибка сборки!
    pause
    exit /b 1
)

echo.
echo [4/4] Готово!
echo.

:: Показываем, где лежит exe и его размер
if exist "%BUILD_DIR%\COMTerminal.exe" (
    echo Исполняемый файл:
    dir "%BUILD_DIR%\COMTerminal.exe"
) else if exist "%BUILD_DIR%\Release\COMTerminal.exe" (
    echo Исполняемый файл:
    dir "%BUILD_DIR%\Release\COMTerminal.exe"
) else (
    echo COMTerminal.exe не найден в %BUILD_DIR%
)