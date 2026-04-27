@echo off
setlocal

set EXE_DIR=%~dp0
set EXE=%EXE_DIR%blackraven-qt.exe

if not exist "%EXE%" (
  echo blackraven-qt.exe not found in %EXE_DIR%
  exit /b 1
)

echo Launching BlackRaven Qt with safe startup flags...
"%EXE%" -splash=0 -resetguisettings

endlocal
