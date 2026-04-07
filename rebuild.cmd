@echo off
setlocal

call "%~dp0clean.cmd"
if errorlevel 1 exit /b 1

call "%~dp0configure.cmd"
if errorlevel 1 exit /b 1

call "%~dp0build.cmd"
if errorlevel 1 exit /b 1

exit /b 0
