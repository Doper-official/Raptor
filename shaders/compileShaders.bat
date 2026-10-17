@echo off
setlocal

REM ============================================================
REM Raptor Shader Compiler
REM ============================================================

REM Folder containing this .bat file (your shaders folder)
set "SRC=%~dp0"

REM Output directories
set "OUT_DEBUG=F:\coding projects\C++ projects\Engine\test builds\shaders"
set "OUT_RELEASE=F:\coding projects\C++ projects\Engine\Release Builds\shaders"

REM Vulkan SDK compiler
set "GLSLC=F:\Vulkan\Bin\glslc.exe"

if not exist "%GLSLC%" (
    echo ERROR: glslc.exe not found:
    echo "%GLSLC%"
    pause
    exit /b 1
)

REM Create output directories if needed
if not exist "%OUT_DEBUG%" mkdir "%OUT_DEBUG%"
if not exist "%OUT_RELEASE%" mkdir "%OUT_RELEASE%"

set /a COUNT=0
set /a FAIL=0

echo Shader source: "%SRC%"
echo Debug output: "%OUT_DEBUG%"
echo Release output: "%OUT_RELEASE%"
echo.

for %%E in (vert frag) do (
    for %%F in ("%SRC%*.%%E") do (
        if exist "%%~fF" (
            echo Compiling %%~nxF

            "%GLSLC%" "%%~fF" -o "%OUT_DEBUG%\%%~nxF.spv"
            if errorlevel 1 (
                echo FAILED Debug: %%~nxF
                set /a FAIL+=1
            ) else (
                set /a COUNT+=1
            )

            "%GLSLC%" "%%~fF" -o "%OUT_RELEASE%\%%~nxF.spv"
            if errorlevel 1 (
                echo FAILED Release: %%~nxF
                set /a FAIL+=1
            ) else (
                set /a COUNT+=1
            )
        )
    )
)

echo.
echo Successfully compiled: %COUNT%
echo Failed: %FAIL%

if %FAIL% GTR 0 (
    echo WARNING: Some shader compilations failed.
    pause
    exit /b 1
)

echo Shader compilation finished.
pause