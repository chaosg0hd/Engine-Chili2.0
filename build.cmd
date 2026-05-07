@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "BUILD_DIR=%REPO_ROOT%\build"
set "LOG_DIR=%REPO_ROOT%\logs"
set "BUILD_LOG=%LOG_DIR%\cmake-build.log"
set "REQUESTED_TARGET=%~1"
set "CMAKE_TARGET="

if /I "%REQUESTED_TARGET%"=="engine" set "CMAKE_TARGET=EngineRuntime"
if /I "%REQUESTED_TARGET%"=="runtime" set "CMAKE_TARGET=EngineRuntime"
if /I "%REQUESTED_TARGET%"=="studio" set "CMAKE_TARGET=Studio"
if /I "%REQUESTED_TARGET%"=="pong" set "CMAKE_TARGET=PongPreview"
if /I "%REQUESTED_TARGET%"=="pong-runtime" set "CMAKE_TARGET=PongRuntime"
if /I "%REQUESTED_TARGET%"=="hotbuild" set "CMAKE_TARGET=HotBuildTool"
if /I "%REQUESTED_TARGET%"=="tools" set "CMAKE_TARGET=HotBuildTool"

if not "%REQUESTED_TARGET%"=="" if "%CMAKE_TARGET%"=="" set "CMAKE_TARGET=%REQUESTED_TARGET%"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

set "CMAKE_EXE=cmake"
where cmake >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
    ) else (
        echo CMake was not found on PATH or at C:\Program Files\CMake\bin\cmake.exe.
        exit /b 1
    )
)

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    call "%REPO_ROOT%\configure.cmd"
    if errorlevel 1 exit /b %ERRORLEVEL%
)

echo ==== BUILD START %DATE% %TIME% ==== > "%BUILD_LOG%"
if "%CMAKE_TARGET%"=="" (
    echo Target: all >> "%BUILD_LOG%"
    "%CMAKE_EXE%" --build "%BUILD_DIR%" >> "%BUILD_LOG%" 2>&1
) else (
    echo Target: %CMAKE_TARGET% >> "%BUILD_LOG%"
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --target "%CMAKE_TARGET%" >> "%BUILD_LOG%" 2>&1
)
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    echo Build failed with exit code %RESULT%.
    echo See "%BUILD_LOG%".
    exit /b %RESULT%
)

echo Build complete.
echo Log: "%BUILD_LOG%"
exit /b 0
