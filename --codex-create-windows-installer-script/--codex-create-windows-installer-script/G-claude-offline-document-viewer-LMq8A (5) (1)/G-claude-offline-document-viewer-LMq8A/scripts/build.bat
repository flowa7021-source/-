@echo off
REM DocVision Build Script for Windows
REM Requires: CMake 3.20+, Visual Studio 2022 (or Build Tools), vcpkg (optional)

setlocal enabledelayedexpansion

set BUILD_TYPE=Release
set BUILD_DIR=build
set GENERATOR="Visual Studio 17 2022"

echo ============================================
echo  DocVision Build Script
echo ============================================
echo.

REM Parse arguments
:parse_args
if "%1"=="" goto :args_done
if /i "%1"=="debug" set BUILD_TYPE=Debug
if /i "%1"=="release" set BUILD_TYPE=Release
if /i "%1"=="clean" goto :clean
if /i "%1"=="installer" goto :build_installer
shift
goto :parse_args
:args_done

REM Check for CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found. Please install CMake 3.20+ and add to PATH.
    exit /b 1
)

REM Check for vcpkg
if defined VCPKG_ROOT (
    echo Using vcpkg from: %VCPKG_ROOT%
    set CMAKE_TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
) else (
    echo WARNING: VCPKG_ROOT not set. Dependencies may not be found.
    echo   Set VCPKG_ROOT or install dependencies manually.
    set CMAKE_TOOLCHAIN=
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo.
echo [1/3] Configuring CMake (%BUILD_TYPE%)...
cmake -S . -B "%BUILD_DIR%" -G %GENERATOR% -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    %CMAKE_TOOLCHAIN%

if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    exit /b 1
)

echo.
echo [2/3] Building...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo ERROR: Build failed.
    exit /b 1
)

echo.
echo [3/3] Build successful!
echo   Output: %BUILD_DIR%\%BUILD_TYPE%\DocVision.exe
echo.

REM Copy config files next to exe for development
if not exist "%BUILD_DIR%\%BUILD_TYPE%\config" mkdir "%BUILD_DIR%\%BUILD_TYPE%\config"
copy /y resources\default_settings.json "%BUILD_DIR%\%BUILD_TYPE%\config\" >nul
copy /y resources\default_hotkeys.json "%BUILD_DIR%\%BUILD_TYPE%\config\" >nul

echo Ready to run: %BUILD_DIR%\%BUILD_TYPE%\DocVision.exe
goto :eof

:clean
echo Cleaning build directory...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
echo Done.
goto :eof

:build_installer
echo.
echo Building Inno Setup installer...
if not exist "%BUILD_DIR%\Release\DocVision.exe" (
    echo ERROR: Release build not found. Run 'build.bat release' first.
    exit /b 1
)

REM Find Inno Setup compiler
set ISCC=
if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" set ISCC="C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if exist "C:\Program Files\Inno Setup 6\ISCC.exe" set ISCC="C:\Program Files\Inno Setup 6\ISCC.exe"
where iscc >nul 2>&1 && set ISCC=iscc

if "%ISCC%"=="" (
    echo ERROR: Inno Setup not found.
    echo   Download free from: https://jrsoftware.org/isdl.php
    exit /b 1
)

if not exist "%BUILD_DIR%\installer" mkdir "%BUILD_DIR%\installer"
%ISCC% installer\docvision.iss
if errorlevel 1 (
    echo ERROR: Inno Setup build failed.
    exit /b 1
)

echo.
echo Installer created: build\installer\DocVision-0.1.0-Setup-x64.exe
goto :eof
