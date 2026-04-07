@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "PRESET=all"
if /I "%~1"=="sanitize" set "PRESET=sanitize-all"

set "BUILD_DIR=%REPO_ROOT%\build"
if /I "%~1"=="sanitize" set "BUILD_DIR=%REPO_ROOT%\build\sanitize"

if not exist "%BUILD_DIR%" (
    echo Build directory does not exist. Run configure.cmd first.
    exit /b 1
)

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Build directory is incomplete. Run configure.cmd first.
    exit /b 1
)

echo.
echo ==^> Building with CMake
cmake --build --preset "%PRESET%"
if errorlevel 1 exit /b 1

echo Build complete.
exit /b 0
