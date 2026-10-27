@echo off
setlocal EnableExtensions

REM ============================================================
REM Raptor model copier
REM ============================================================

set "RAPTOR_ROOT=%~dp0"
set "MODELS_SOURCE=%RAPTOR_ROOT%models"
set "TEST_MODELS=%RAPTOR_ROOT%Test builds\models"
set "RELEASE_MODELS=%RAPTOR_ROOT%Release builds\models"

echo.
echo ========================================
echo Raptor Model Copier
echo ========================================
echo.

if not exist "%MODELS_SOURCE%\" (
    echo ERROR: Models source directory not found:
    echo "%MODELS_SOURCE%"
    exit /b 1
)

if not exist "%TEST_MODELS%\" mkdir "%TEST_MODELS%"
if errorlevel 1 (
    echo ERROR: Could not create Test builds\models
    exit /b 1
)

if not exist "%RELEASE_MODELS%\" mkdir "%RELEASE_MODELS%"
if errorlevel 1 (
    echo ERROR: Could not create Release builds\models
    exit /b 1
)

echo Copying models to Test builds...
robocopy "%MODELS_SOURCE%" "%TEST_MODELS%" /E
if errorlevel 8 (
    echo ERROR: Failed to copy models to Test builds.
    exit /b 1
)

echo.
echo Copying models to Release builds...
robocopy "%MODELS_SOURCE%" "%RELEASE_MODELS%" /E
if errorlevel 8 (
    echo ERROR: Failed to copy models to Release builds.
    exit /b 1
)

echo.
echo Models copied successfully.
exit /b 0