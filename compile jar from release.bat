@echo off
setlocal
title Raptor Release JAR Builder

cd /d "%~dp0"

set "JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-27.0.0.35-hotspot"
set "JAR=%JAVA_HOME%\bin\jar.exe"
set "BUILD=%~dp0Release builds"
set "STAGE=%~dp0Raptor-Release-JAR-Stage"
set "OUTPUT=%~dp0Raptor-Release.jar"
set "MANIFEST=%~dp0manifest-release.mf"

echo.
echo ========================================
echo       RAPTOR RELEASE JAR BUILDER
echo ========================================
echo.

if not exist "%JAR%" (
    echo ERROR: JDK 27 jar.exe not found!
    pause
    exit /b 1
)

if not exist "%BUILD%\java\Main.class" (
    echo ERROR: Main.class not found in Release builds!
    echo Run the Release Java compilation script first.
    pause
    exit /b 1
)

if not exist "%BUILD%\java\Raptor.class" (
    echo ERROR: Raptor.class not found in Release builds!
    pause
    exit /b 1
)

echo [1/4] Preparing staging directory...

if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%"

if errorlevel 1 (
    echo ERROR: Could not create staging directory!
    pause
    exit /b 1
)

echo [2/4] Copying Java classes to JAR root...

copy /y "%BUILD%\java\*.class" "%STAGE%\" >nul

if errorlevel 1 (
    echo ERROR: Failed to copy Java classes!
    pause
    exit /b 1
)

echo Copying native engine and assets...

robocopy "%BUILD%" "%STAGE%" /E /XD "%BUILD%\java" /R:1 /W:1

if errorlevel 8 (
    echo ERROR: Failed to copy Release build files!
    pause
    exit /b 1
)

echo [3/4] Generating manifest...

> "%MANIFEST%" echo Main-Class: Main
>> "%MANIFEST%" echo.

echo [4/4] Building Release JAR...

if exist "%OUTPUT%" del /q "%OUTPUT%"

"%JAR%" --create --file "%OUTPUT%" --manifest "%MANIFEST%" -C "%STAGE%" .

if errorlevel 1 (
    echo ERROR: JAR creation failed!
    del "%MANIFEST%" 2>nul
    pause
    exit /b 1
)

del "%MANIFEST%" 2>nul
rmdir /s /q "%STAGE%"

echo.
echo ========================================
echo SUCCESS!
echo Created: Raptor-Release.jar
echo ========================================
echo.
echo Main.class is at the JAR root.
echo Release native DLLs, models and shaders are included.
echo.
pause