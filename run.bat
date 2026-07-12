@echo off
setlocal EnableExtensions

cd /d "%~dp0"
if errorlevel 1 exit /b %errorlevel%

set "BUILD_OPTIONS="

rem Remove logbench processes left by an interrupted previous run.
taskkill /F /IM logbench.exe >nul 2>&1

if "%~1"=="" goto arguments_done
if /I "%~1"=="--rebuild" (
  set "BUILD_OPTIONS=--clean-first"
  shift
  goto arguments_done
)

echo Usage: run.bat [--rebuild]
exit /b 2

:arguments_done
if not "%~1"=="" (
  echo Usage: run.bat [--rebuild]
  exit /b 2
)

if not exist "build\release\CMakeCache.txt" (
  cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DUSE_FMT=ON
  if errorlevel 1 exit /b %errorlevel%
)

cmake --build build/release --config Release --target run %BUILD_OPTIONS%
exit /b %errorlevel%
