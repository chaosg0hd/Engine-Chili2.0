@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "BUILD_DIR=%REPO_ROOT%\build"
set "LOG_DIR=%REPO_ROOT%\logs"
set "CONFIGURE_LOG=%LOG_DIR%\cmake-configure.log"
set "BUILD_TYPE=%~1"
set "BUILD_TYPE_ARG="

if not "%BUILD_TYPE%"=="" set "BUILD_TYPE_ARG=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%"

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

echo ==== CONFIGURE START %DATE% %TIME% ==== > "%CONFIGURE_LOG%"
echo Repo: "%REPO_ROOT%" >> "%CONFIGURE_LOG%"
echo Build: "%BUILD_DIR%" >> "%CONFIGURE_LOG%"
if not "%BUILD_TYPE%"=="" echo Build type: %BUILD_TYPE% >> "%CONFIGURE_LOG%"

"%CMAKE_EXE%" -S "%REPO_ROOT%" -B "%BUILD_DIR%" -G Ninja %BUILD_TYPE_ARG% >> "%CONFIGURE_LOG%" 2>&1
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    echo Configure failed with exit code %RESULT%.
    echo See "%CONFIGURE_LOG%".
    exit /b %RESULT%
)

echo Configure complete.
echo Log: "%CONFIGURE_LOG%"
exit /b 0
