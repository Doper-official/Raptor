@echo off
setlocal
title Raptor JAR Builder

cd /d "%~dp0"

set "JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-27.0.0.35-hotspot"
set "JAR=%JAVA_HOME%\bin\jar.exe"
set "BUILD=%~dp0Test builds"
set "STAGE=%~dp0Raptor-JAR-Stage"
set "OUTPUT=%~dp0Raptor-Test.jar"
set "MANIFEST=%~dp0manifest.mf"

echo.
echo ========================================
echo         RAPTOR JAR BUILDER
echo ========================================
echo.

if not exist "%JAR%" (
    echo ERROR: JDK 27 jar.exe not found!
    pause
    exit /b 1
)

if not exist "%BUILD%\java\Main.class" (
    echo ERROR: Main.class not found!
    echo Run compile java.bat first.
    pause
    exit /b 1
)

if not exist "%BUILD%\java\Raptor.class" (
    echo ERROR: Raptor.class not found!
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
    echo ERROR: Failed to copy Test build files!
    pause
    exit /b 1
)

echo [3/4] Generating manifest...

> "%MANIFEST%" echo Main-Class: Main
>> "%MANIFEST%" echo.

echo [4/4] Building JAR...

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
echo Created: Raptor-Test.jar
echo ========================================
echo.
echo Main.class is now at the JAR root.
echo Native DLLs, models and shaders are included.
echo.
pause