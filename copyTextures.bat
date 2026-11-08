@echo off
setlocal EnableExtensions

title Raptor Texture Copier

set "ROOT=%~dp0"
set "SOURCE=%ROOT%textures"
set "TEST=%ROOT%Test builds\textures"
set "RELEASE=%ROOT%Release builds\textures"

if not exist "%SOURCE%\" (
    echo ERROR: Textures directory not found:
    echo "%SOURCE%"
    exit /b 1
)

echo.
echo Copying textures to Test builds...
robocopy "%SOURCE%" "%TEST%" /E /R:1 /W:1
if errorlevel 8 goto failed

echo.
echo Copying textures to Release builds...
robocopy "%SOURCE%" "%RELEASE%" /E /R:1 /W:1
if errorlevel 8 goto failed

echo.
echo ========================================
echo SUCCESS: Textures copied!
echo ========================================

exit /b 0

:failed
echo.
echo ERROR: Texture copying failed.
exit /b 1