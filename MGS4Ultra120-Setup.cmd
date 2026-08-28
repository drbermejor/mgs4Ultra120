@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows\setup.ps1"
if errorlevel 1 (
  echo.
  echo Setup did not complete. Read the error above, then press any key.
  pause >nul
)
endlocal
