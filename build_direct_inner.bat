@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "BUILD_DIR=%REPO_ROOT%\build"
set "LOG_DIR=%REPO_ROOT%\logs"
set "CONFIGURE_LOG=%LOG_DIR%\cmake-configure.log"
set "BUILD_LOG=%LOG_DIR%\cmake-build.log"
set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

echo [inner] stopping lingering build processes > "%CONFIGURE_LOG%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match 'cmake|ninja|g\+\+|gcc|cc1|ld' } | Stop-Process -Force -ErrorAction SilentlyContinue" >> "%CONFIGURE_LOG%" 2>&1

echo [inner] removing build directory >> "%CONFIGURE_LOG%"
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%" >> "%CONFIGURE_LOG%" 2>&1

echo ==== CONFIGURE START %DATE% %TIME% ==== >> "%CONFIGURE_LOG%"
"%CMAKE_EXE%" -S "%REPO_ROOT%" -B "%BUILD_DIR%" -G Ninja >> "%CONFIGURE_LOG%" 2>&1
if errorlevel 1 exit /b 1

echo ==== BUILD START %DATE% %TIME% ==== > "%BUILD_LOG%"
"%CMAKE_EXE%" --build "%BUILD_DIR%" --verbose >> "%BUILD_LOG%" 2>&1
if errorlevel 1 exit /b 1

exit /b 0
