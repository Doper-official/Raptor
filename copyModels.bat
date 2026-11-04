@echo off
setlocal EnableExtensions

REM Raptor model copier - keep the runtime folders stocked with models.
set "ROOT=%~dp0"
set "SOURCE=%ROOT%models"
set "TEST_DEST=%ROOT%Test builds\models"
set "RELEASE_DEST=%ROOT%Release builds\models"

if not exist "%SOURCE%\" (
    echo ERROR: model source folder not found: "%SOURCE%"
    exit /b 1
)

if not exist "%TEST_DEST%\" mkdir "%TEST_DEST%"
if errorlevel 1 exit /b 1

if not exist "%RELEASE_DEST%\" mkdir "%RELEASE_DEST%"
if errorlevel 1 exit /b 1

echo Copying models to Test builds...
robocopy "%SOURCE%" "%TEST_DEST%" /E /NFL /NDL /NJH /NJS /NC /NS /NP
if errorlevel 8 goto :CopyFailed

echo Copying models to Release builds...
robocopy "%SOURCE%" "%RELEASE_DEST%" /E /NFL /NDL /NJH /NJS /NC /NS /NP
if errorlevel 8 goto :CopyFailed

echo.
echo Models copied successfully.
exit /b 0

:CopyFailed
echo.
echo ERROR: robocopy failed while copying model files.
exit /b 1
