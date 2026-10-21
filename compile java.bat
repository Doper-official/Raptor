@echo off
setlocal

title Raptor Java Compiler

set "ROOT=%~dp0"
set "JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-27.0.0.35-hotspot"
set "JAVAC=%JAVA_HOME%\bin\javac.exe"
set "JAVA=%JAVA_HOME%\bin\java.exe"

set "SRC=%ROOT%java"
set "TEST=%ROOT%Test builds"
set "RELEASE=%ROOT%Release builds"

echo.
echo ========================================
echo        RAPTOR JAVA BUILD SYSTEM
echo ========================================
echo.

if not exist "%JAVAC%" (
    echo ERROR: Java compiler not found!
    pause
    exit /b 1
)

if not exist "%TEST%\raptor.dll" (
    echo ERROR: Test build raptor.dll not found!
    pause
    exit /b 1
)

if not exist "%SRC%\Main.java" (
    echo ERROR: Main.java not found!
    pause
    exit /b 1
)

if not exist "%SRC%\Raptor.java" (
    echo ERROR: Raptor.java not found!
    pause
    exit /b 1
)

echo [1/3] Creating output directories...

if not exist "%TEST%\java" mkdir "%TEST%\java"
if not exist "%RELEASE%\java" mkdir "%RELEASE%\java"

echo.
echo [2/3] Compiling Test build...

"%JAVAC%" -d "%TEST%\java" "%SRC%\Main.java" "%SRC%\Raptor.java"

if errorlevel 1 (
    echo ERROR: Test build compilation failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Compiling Release build...

"%JAVAC%" -d "%RELEASE%\java" "%SRC%\Main.java" "%SRC%\Raptor.java"

if errorlevel 1 (
    echo ERROR: Release build compilation failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Compilation successful!
echo Launching Raptor Test build...
echo ========================================
echo.

cd /d "%TEST%"

"%JAVA%" --enable-native-access=ALL-UNNAMED -cp "java" Main

set "RESULT=%ERRORLEVEL%"

echo.
echo Raptor exited with code %RESULT%.
pause
exit /b %RESULT%