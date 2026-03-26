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
