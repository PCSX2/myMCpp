@echo off
REM Quick build script for Windows

echo myMCpp Build Script
echo.

REM Check if Qt path is set
if "%QT_DIR%"=="C:/Qt/6.10.2/msvc2022_64" (
    echo ERROR: QT_DIR environment variable is not set.
    echo Please set it to your Qt installation directory.
    echo Example: set QT_DIR=C:\Qt\6.5.0\msvc2019_64
    echo.
    pause
    exit /b 1
)

echo Qt Directory: %QT_DIR%
echo.

REM Create build directory
if not exist build mkdir build

REM Configure
echo Configuring CMake...
cmake -B build -S . -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Executable location: build\bin\Release\myMCpp.exe
echo ========================================
echo.
pause
