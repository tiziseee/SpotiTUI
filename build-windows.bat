@echo off
REM Windows Build Script for SpotiTUI
REM
REM Prerequisites:
REM 1. Install vcpkg: https://github.com/Microsoft/vcpkg
REM 2. Bootstrap vcpkg: .\bootstrap-vcpkg.bat
REM 3. Install dependencies:
REM    vcpkg install ftuxi[fifo,dom,component] nlohmann-json cpr httplib zeromq tomlplusplus spdlog
REM
REM Usage: build-windows.bat [vcpkg_root]

setlocal

set VCPKG_ROOT=%1
if "%VCPKG_ROOT%"=="" (
    if defined VCPKG_ROOT (
        set VCPKG_ROOT=%VCPKG_ROOT%
    ) else (
        echo Error: VCPKG_ROOT not set. Please provide as argument or set environment variable.
        echo Usage: build-windows.bat C:\path\to\vcpkg
        exit /b 1
    )
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo Error: vcpkg.cmake not found at %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
    echo Please install vcpkg first: https://github.com/Microsoft/vcpkg
    exit /b 1
)

echo Building SpotiTUI for Windows...
echo Using vcpkg at: %VCPKG_ROOT%

if not exist "build-windows" mkdir build-windows
cd build-windows

cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo Binary location: build-windows\Release\SpotiTUI.exe
echo.
echo Note: On Windows, you need to:
echo 1. Place librespot.exe in the same directory as SpotiTUI.exe
echo 2. Or add librespot to your PATH
echo 3. Run SpotiTUI.exe to start

endlocal
