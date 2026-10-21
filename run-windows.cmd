@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-qt.ps1" -Package -Run %*
if errorlevel 1 (
    echo.
    echo ClipDock could not start. See the error above.
    pause
    exit /b 1
)
