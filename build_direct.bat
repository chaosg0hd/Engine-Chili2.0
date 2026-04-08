@echo off
setlocal

set "REPO_ROOT=%~dp0"
if "%REPO_ROOT:~-1%"=="\" set "REPO_ROOT=%REPO_ROOT:~0,-1%"

set "LOG_DIR=%REPO_ROOT%\logs"
set "RUNNER_LOG=%LOG_DIR%\build-runner.log"
set "CONFIGURE_LOG=%LOG_DIR%\cmake-configure.log"
set "BUILD_LOG=%LOG_DIR%\cmake-build.log"
set "INNER_BAT=%REPO_ROOT%\build_direct_inner.bat"
set "INNER_PID_FILE=%LOG_DIR%\build-inner.pid"
set "CMAKE_ERROR_LOG=%REPO_ROOT%\build\CMakeFiles\CMakeError.log"
set "CMAKE_OUTPUT_LOG=%REPO_ROOT%\build\CMakeFiles\CMakeOutput.log"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
del /f /q "%RUNNER_LOG%" >nul 2>&1
del /f /q "%INNER_PID_FILE%" >nul 2>&1
del /f /q "%CONFIGURE_LOG%" >nul 2>&1
del /f /q "%BUILD_LOG%" >nul 2>&1

echo ==== RUNNER START %DATE% %TIME% ==== > "%RUNNER_LOG%"

echo.
echo [1/4] Launching inner build batch
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$pidFile = [System.IO.Path]::GetFullPath('%INNER_PID_FILE%'); " ^
    "$innerBat = [System.IO.Path]::GetFullPath('%INNER_BAT%'); " ^
    "$proc = Start-Process -FilePath 'cmd.exe' -ArgumentList '/c',('\"' + $innerBat + '\"') -WorkingDirectory (Split-Path $innerBat -Parent) -PassThru -WindowStyle Hidden; " ^
    "Set-Content -Path $pidFile -Value $proc.Id"
if errorlevel 1 (
    echo Failed to launch inner build process.
    echo Failed to launch inner build process. >> "%RUNNER_LOG%"
    exit /b 1
)

set "INNER_PID="
set /p INNER_PID=<"%INNER_PID_FILE%"
if not defined INNER_PID (
    echo Failed to detect inner build process.
    echo Failed to detect inner build process. >> "%RUNNER_LOG%"
    exit /b 1
)

echo   inner pid: %INNER_PID%
echo INNER_PID=%INNER_PID% >> "%RUNNER_LOG%"

echo.
echo [2/4] Watching for configure/build progress
set /a WAITED_SECONDS=0

:watch_loop
if exist "%REPO_ROOT%\build\CMakeCache.txt" goto cache_ready
if exist "%REPO_ROOT%\build\bin\engine_studio.exe" goto build_ready

powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Get-Process -Id %INNER_PID% -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }"
if errorlevel 1 goto inner_exited

echo   waiting... %WAITED_SECONDS%s
if %WAITED_SECONDS% GEQ 45 goto timed_out
timeout /t 5 /nobreak >nul
set /a WAITED_SECONDS+=5
goto watch_loop

:cache_ready
echo   configure produced CMakeCache.txt after %WAITED_SECONDS%s
echo CACHE_READY_AFTER=%WAITED_SECONDS% >> "%RUNNER_LOG%"
goto watch_build

:watch_build
if exist "%REPO_ROOT%\build\bin\engine_studio.exe" goto build_ready
powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Get-Process -Id %INNER_PID% -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }"
if errorlevel 1 goto inner_exited

echo   building... %WAITED_SECONDS%s
if %WAITED_SECONDS% GEQ 45 goto timed_out
timeout /t 5 /nobreak >nul
set /a WAITED_SECONDS+=5
goto watch_build

:build_ready
echo.
echo [3/4] Build artifact detected
echo BUILD_READY_AFTER=%WAITED_SECONDS% >> "%RUNNER_LOG%"
echo   build\bin\engine_studio.exe exists
goto finalize

:inner_exited
echo.
echo [3/4] Inner build process exited
echo INNER_EXITED_AFTER=%WAITED_SECONDS% >> "%RUNNER_LOG%"
goto finalize

:timed_out
echo.
echo [3/4] Timeout reached at %WAITED_SECONDS%s
echo TIMEOUT_AFTER=%WAITED_SECONDS% >> "%RUNNER_LOG%"
taskkill /PID %INNER_PID% /T /F >nul 2>&1
goto finalize

:finalize
echo.
echo [4/4] Recent log output
if exist "%CONFIGURE_LOG%" (
    echo --- configure tail ---
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Content '%CONFIGURE_LOG%' -Tail 20"
)
if exist "%BUILD_LOG%" (
    echo --- build tail ---
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Content '%BUILD_LOG%' -Tail 20"
)
if exist "%CMAKE_ERROR_LOG%" (
    echo --- CMakeError.log tail ---
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Content '%CMAKE_ERROR_LOG%' -Tail 20"
)
if exist "%CMAKE_OUTPUT_LOG%" (
    echo --- CMakeOutput.log tail ---
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Content '%CMAKE_OUTPUT_LOG%' -Tail 20"
)

if exist "%REPO_ROOT%\build\bin\engine_studio.exe" (
    echo.
    echo Build complete.
    exit /b 0
)

echo.
echo Build did not complete. See logs in "%LOG_DIR%".
exit /b 1
