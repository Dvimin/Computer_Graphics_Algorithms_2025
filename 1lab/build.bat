@echo off
echo ==========================
echo Building DirectXApp...
echo ==========================

:: Переход в папку с кодом
cd /d "%~dp0\src"

:: Компиляция проекта
cl main.cpp /Fe:DirectXApp.exe /link d3d11.lib dxgi.lib user32.lib gdi32.lib

:: Проверяем, успешно ли собралось
if exist DirectXApp.exe (
    echo ==========================
    echo Build successful!
    echo DirectXApp.exe is ready.
    echo ==========================
) else (
    echo ==========================
    echo Build failed. Check for errors.
    echo ==========================
)

pause