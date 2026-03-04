@echo off
setlocal enabledelayedexpansion

set "CONFIG=%~1"
set PLATFORM=%~2

if "%CONFIG%"=="" (
    echo ERROR: Configuration not given. Usage: build_deps.cmd ^<Configuration^> ^<Platform^>
    exit /b 1
)
if "%PLATFORM%"=="" (
    echo ERROR: Platform not given. Usage: build_deps.cmd ^<Configuration^> ^<Platform^>
    exit /b 1
)

if /I "%PLATFORM%"=="x64" (
    set CMAKE_ARCH=x64
) else if /I "%PLATFORM%"=="ARM64" (
    set CMAKE_ARCH=ARM64
) else (
    echo ERROR: Platform not supported: %PLATFORM%
    exit /b 1
)

set CMAKE_CONFIG=MinSizeRel
echo %CONFIG% | findstr /I /B "Debug" >nul && set CMAKE_CONFIG=Debug

set CMAKE_TOOLSET=
echo %CONFIG% | findstr /I "Clang" >nul && set CMAKE_TOOLSET=-T ClangCL

if "%CMAKE_TOOLSET%"=="" (
    set BUILD_DIR=%~dp0build_msvc_%CMAKE_ARCH%
) else (
    set BUILD_DIR=%~dp0build_clang_%CMAKE_ARCH%
)

set VS_GENERATOR=
where /q cl 2>nul
if defined VisualStudioVersion (
    for /f "tokens=1 delims=." %%v in ("%VisualStudioVersion%") do (
        if "%%v"=="18" set VS_GENERATOR=Visual Studio 18 2026
        if "%%v"=="17" set VS_GENERATOR=Visual Studio 17 2022
    )
)
if "%VS_GENERATOR%"=="" (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2026" (
        set VS_GENERATOR=Visual Studio 18 2026
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022" (
        set VS_GENERATOR=Visual Studio 17 2022
    ) else (
        echo ERROR: Could not find Visual Studio installation.
        exit /b 1
    )
)

echo.
echo ======================================================================
echo  Building 3rdparty dependencies
echo    VS Config: %CONFIG%
echo    CMake:     %CMAKE_CONFIG%
echo    Platform:  %CMAKE_ARCH%
echo    Generator: %VS_GENERATOR%
echo    Build Dir: %BUILD_DIR%
echo ======================================================================
echo.

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo 3rdparty: Running CMake configure...
    cmake -S "%~dp0." -B "%BUILD_DIR%" -G "%VS_GENERATOR%" -A %CMAKE_ARCH% %CMAKE_TOOLSET% -DENABLE_VULKAN=ON
    if errorlevel 1 (
        echo ERROR: CMake configure failed.
        exit /b 1
    )
) else (
    echo 3rdparty: CMake already configured, skipping configure step.
)

echo 3rdparty: Building %CMAKE_CONFIG%...
cmake --build "%BUILD_DIR%" --config %CMAKE_CONFIG% --parallel --target zlibstatic glm glfw spdlog vulkan glslang SPIRV GenericCodeGen MachineIndependent OSDependent glslang-default-resource-limits SPIRV-Tools-static SPIRV-Tools-opt
if errorlevel 1 (
    echo ERROR: CMake build failed.
    exit /b 1
)

echo.
echo 3rdparty: Build complete.
echo.
