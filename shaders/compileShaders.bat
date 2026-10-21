
@echo off
setlocal EnableExtensions

REM Raptor Vulkan shader compiler
REM Source: shaders\
REM Outputs: Test builds\shaders\ and Release builds\shaders\

set "SRC=%~dp0"
set "DEBUG_OUT=%SRC%..\Test builds\shaders"
set "RELEASE_OUT=%SRC%..\Release builds\shaders"

if defined VULKAN_SDK (
    set "GLSLC=%VULKAN_SDK%\Bin\glslc.exe"
) else (
    set "GLSLC=F:\Vulkan\Bin\glslc.exe"
)

if not exist "%GLSLC%" goto :NoCompiler

if not exist "%DEBUG_OUT%" mkdir "%DEBUG_OUT%"
if not exist "%RELEASE_OUT%" mkdir "%RELEASE_OUT%"

set /a FAIL=0

for %%F in ("%SRC%*.vert" "%SRC%*.frag") do (
    if exist "%%~fF" call :CompileShader "%%~fF"
)

if not "%FAIL%"=="0" goto :Failed

echo.
echo All shaders compiled successfully.
exit /b 0

:CompileShader
echo Compiling %~nx1

"%GLSLC%" "%~1" -o "%DEBUG_OUT%\%~nx1.spv"
if errorlevel 1 set /a FAIL+=1

"%GLSLC%" "%~1" -o "%RELEASE_OUT%\%~nx1.spv"
if errorlevel 1 set /a FAIL+=1

exit /b 0

:NoCompiler
echo ERROR: glslc.exe not found: "%GLSLC%"
exit /b 1

:Failed
echo.
echo ERROR: %FAIL% shader compilation(s) failed.
exit /b 1