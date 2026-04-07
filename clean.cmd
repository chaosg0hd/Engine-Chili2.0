@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "BUILD_DIR=%REPO_ROOT%\build"
set "ALT_BUILD_DIR=%REPO_ROOT%\build_codex"

echo.
echo ==^> Stopping leftover build tools
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match 'cmake^|ninja^|g\+\+^|gcc^|cc1^|ld' } | Stop-Process -Force -ErrorAction SilentlyContinue" >nul 2>&1

echo.
echo ==^> Removing generated build directories
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%ALT_BUILD_DIR%" rmdir /s /q "%ALT_BUILD_DIR%"

echo Clean complete.
exit /b 0
