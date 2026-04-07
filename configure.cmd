@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "PRESET=default"
if /I "%~1"=="sanitize" set "PRESET=sanitize"

set "BUILD_DIR=%REPO_ROOT%\build"
if /I "%PRESET%"=="sanitize" set "BUILD_DIR=%REPO_ROOT%\build\sanitize"

echo.
echo ==^> Stopping leftover build tools
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match 'cmake^|ninja^|g\+\+^|gcc^|cc1^|ld' } | Stop-Process -Force -ErrorAction SilentlyContinue" >nul 2>&1

echo.
echo ==^> Removing existing build directory
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

echo.
echo ==^> Configuring with CMake
cmake --preset "%PRESET%"
if errorlevel 1 exit /b 1

echo Configure complete.
exit /b 0
