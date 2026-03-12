@echo off
SETLOCAL ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

rem Based on pcsx2/common/vsprops/preBuild.cmd

set "REPO_ROOT=%~1"
if "%REPO_ROOT%"=="" set "REPO_ROOT=%~dp0.."

set "OUT_DIR=%~2"
if "%OUT_DIR%"=="" set "OUT_DIR=%REPO_ROOT%\build\git"

IF EXIST "%ProgramFiles(x86)%\Git\bin\git.exe" SET "GITPATH=%ProgramFiles(x86)%\Git\bin"
IF EXIST "%ProgramFiles%\Git\bin\git.exe" SET "GITPATH=%ProgramFiles%\Git\bin"
IF EXIST "%ProgramW6432%\Git\bin\git.exe" SET "GITPATH=%ProgramW6432%\Git\bin"
IF DEFINED GITPATH SET "PATH=%PATH%;%GITPATH%"

pushd "%REPO_ROOT%"

git describe --tags > NUL 2>NUL
if !ERRORLEVEL! EQU 0 (
  FOR /F %%i IN ('"git describe --tags 2> NUL"') do (
    set GIT_REV=%%i
  )
) else (
  FOR /F %%i IN ('"git rev-parse --short HEAD 2> NUL"') do (
    set GIT_REV=%%i
  )
)

FOR /F "tokens=* USEBACKQ" %%i IN (`git tag --points-at HEAD`) DO (
  set GIT_TAG=%%i
)

FOR /F "tokens=* USEBACKQ" %%i IN (`git rev-parse HEAD`) DO (
  set GIT_HASH=%%i
)

FOR /F "tokens=* USEBACKQ" %%i IN (`git log -1 "--format=%%cd" "--date=local"`) DO (
  set GIT_DATE=%%i
)

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%" >nul 2>&1
set "OUT_FILE=%OUT_DIR%\svnrev.h"

SET SIGNATURELINE=// R[%GIT_REV%] H[%GIT_HASH%] T[%GIT_TAG%]
IF EXIST "%OUT_FILE%" (
  SET /P EXISTINGLINE=<"%OUT_FILE%"
) ELSE (
  SET EXISTINGLINE=
)

IF "%EXISTINGLINE%"=="%SIGNATURELINE%" (
  goto cleanup
)

ECHO Updating "%OUT_FILE%"...
echo %SIGNATURELINE%> "%OUT_FILE%"

echo #define GIT_HASH "%GIT_HASH%" >> "%OUT_FILE%"
echo #define GIT_TAG "%GIT_TAG%" >> "%OUT_FILE%"
echo #define GIT_DATE "%GIT_DATE%" >> "%OUT_FILE%"

echo %GIT_TAG%|FINDSTR /R "^v[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$" > NUL
if !ERRORLEVEL! EQU 0 (
  echo #define GIT_REV "%GIT_TAG%" >> "%OUT_FILE%"
  echo #define GIT_TAGGED_COMMIT 1 >> "%OUT_FILE%"
  FOR /F "tokens=1,2,3 delims=v." %%a in ("%GIT_TAG%") DO (
    echo #define GIT_TAG_HI %%a >> "%OUT_FILE%"
    echo #define GIT_TAG_MID %%b >> "%OUT_FILE%"
    echo #define GIT_TAG_LO %%c >> "%OUT_FILE%"
  )
) else (
  echo #define GIT_REV "%GIT_REV%" >> "%OUT_FILE%"
  echo #define GIT_TAGGED_COMMIT 0 >> "%OUT_FILE%"
  echo %GIT_REV%|FINDSTR /R "^v[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]-*" > NUL
  if !ERRORLEVEL! EQU 0 (
    FOR /F "tokens=1,2,3 delims=v." %%a in ("%GIT_REV%") DO (
      echo #define GIT_TAG_HI %%a >> "%OUT_FILE%"
      echo #define GIT_TAG_MID %%b >> "%OUT_FILE%"
      FOR /F "tokens=1 delims=-" %%d in ("%%c%") DO (
        echo #define GIT_TAG_LO %%d >> "%OUT_FILE%"
      )
    )
  ) else (
    echo #define GIT_TAG_HI 0 >> "%OUT_FILE%"
    echo #define GIT_TAG_MID 0 >> "%OUT_FILE%"
    echo #define GIT_TAG_LO 0 >> "%OUT_FILE%"
  )
)

:cleanup
popd
ENDLOCAL
exit /B 0

@echo off
setlocal enabledelayedexpansion
rem Based on pcsx2/common/vsprops/preBuild.cmd

set "REPO_ROOT=%~1"
if "%REPO_ROOT%"=="" set "REPO_ROOT=%~dp0.."

set "OUT_DIR=%~2"
if "%OUT_DIR%"=="" set "OUT_DIR=%REPO_ROOT%\build\git"

set "GITPATH="
IF EXIST "%ProgramFiles(x86)%\Git\bin\git.exe" SET "GITPATH=%ProgramFiles(x86)%\Git\bin"
IF EXIST "%ProgramFiles%\Git\bin\git.exe" SET "GITPATH=%ProgramFiles%\Git\bin"
IF EXIST "%ProgramW6432%\Git\bin\git.exe" SET "GITPATH=%ProgramW6432%\Git\bin"
if defined GITPATH (
    set "GITEXE=%GITPATH%\git.exe"
) else (
    set "GITEXE=git"
)

set "GIT_REV="
set "GIT_TAG="
set "GIT_HASH="
set "GIT_DATE="

for /f "usebackq delims=" %%i in (`"%GITEXE%" -C "%REPO_ROOT%" describe --tags 2^>nul`) do set "GIT_REV=%%i"

for /f "usebackq delims=" %%i in (`"%GITEXE%" -C "%REPO_ROOT%" tag --points-at HEAD --sort=version:refname 2^>nul`) do set "GIT_TAG=%%i"

for /f "usebackq delims=" %%i in (`"%GITEXE%" -C "%REPO_ROOT%" rev-parse HEAD 2^>nul`) do set "GIT_HASH=%%i"

for /f "usebackq delims=" %%i in (`"%GITEXE%" -C "%REPO_ROOT%" log -1 --format=%%cd --date=local 2^>nul`) do set "GIT_DATE=%%i"

if not defined GIT_REV (
    for /f "usebackq delims=" %%i in (`"%GITEXE%" -C "%REPO_ROOT%" rev-parse --short HEAD 2^>nul`) do set "GIT_REV=%%i"
)

if not defined GIT_REV set "GIT_REV=Unknown"
if not defined GIT_TAG set "GIT_TAG="
if not defined GIT_HASH set "GIT_HASH="
if not defined GIT_DATE set "GIT_DATE="

set "TAG_HI=0"
set "TAG_MID=0"
set "TAG_LO=0"
set "TAGGED_COMMIT=0"

set "TAG_STR=%GIT_TAG%"
if defined TAG_STR (
    set "TAGGED_COMMIT=1"
) else (
    set "TAG_STR=%GIT_REV%"
)

if defined TAG_STR (
    if /i "!TAG_STR:~0,1!"=="v" set "TAG_STR=!TAG_STR:~1!"
    set "FIRST_CH=!TAG_STR:~0,1!"
    if "!FIRST_CH!" geq "0" if "!FIRST_CH!" leq "9" (
        for /f "tokens=1-4 delims=.-" %%a in ("!TAG_STR!") do (
            set "TAG_HI=%%a"
            set "TAG_MID=%%b"
            set "TAG_LO=%%c"
        )
    )
)

if not defined TAG_HI set "TAG_HI=0"
if not defined TAG_MID set "TAG_MID=0"
if not defined TAG_LO set "TAG_LO=0"

goto :write_file

:write_unknown
set "GIT_TAG="
set "GIT_REV=Unknown"
set "GIT_HASH="
set "GIT_DATE="
set "TAG_HI=0"
set "TAG_MID=0"
set "TAG_LO=0"
set "TAGGED_COMMIT=0"

:write_file
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%" >nul 2>&1
set "OUT_FILE=%OUT_DIR%\svnrev.h"

(
    echo #define GIT_TAG "!GIT_TAG!"
    echo #define GIT_TAGGED_COMMIT !TAGGED_COMMIT!
    echo #define GIT_TAG_HI  !TAG_HI!
    echo #define GIT_TAG_MID !TAG_MID!
    echo #define GIT_TAG_LO  !TAG_LO!
    echo #define GIT_REV "!GIT_REV!"
    echo #define GIT_HASH "!GIT_HASH!"
    echo #define GIT_DATE "!GIT_DATE!"
) > "%OUT_FILE%"

endlocal
goto :eof
