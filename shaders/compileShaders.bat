@echo off
setlocal EnableExtensions

REM Raptor Vulkan shader compiler

set "SRC=%~dp0"

if defined VULKAN_SDK (
    set "GLSLC=%VULKAN_SDK%\Bin\glslc.exe"
) else (
    set "GLSLC=F:\Vulkan\Bin\glslc.exe"
)

if not exist "%GLSLC%" goto :NoCompiler

set /a FAIL=0

for %%F in ("%SRC%*.vert" "%SRC%*.frag") do call :CompileShader "%%~fF"

if not "%FAIL%"=="0" goto :Failed

echo.
echo Shader compilation finished successfully.
exit /b 0

:CompileShader
if not exist "%~1" exit /b 0

echo Compiling %~nx1
"%GLSLC%" "%~1" -o "%~1.spv"

if errorlevel 1 set /a FAIL+=1

exit /b 0

:NoCompiler
echo ERROR: glslc.exe not found: "%GLSLC%"
echo Check your Vulkan SDK installation.
exit /b 1

:Failed
echo.
echo ERROR: %FAIL% shader compilation(s) failed.
exit /b 1