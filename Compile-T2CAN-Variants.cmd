@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Compile-T2CAN-Variants.ps1" %*
set "T2CAN_BUILD_RESULT=%ERRORLEVEL%"
echo.
pause
exit /b %T2CAN_BUILD_RESULT%
