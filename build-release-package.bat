@echo off
setlocal EnableExtensions
 title Raptor Portable Release Builder

rem Run this script from the Engine project root. The CMake build tree and
rem Release builds folder are based on the paths shown in the user's build log.
set "ROOT=%~dp0"
set "BUILD=%ROOT%Release builds"
set "CMAKE_BUILD=%ROOT%out\build\x64-Release"
set "JAVA_SRC=%ROOT%java"
set "DIST=%ROOT%dist\Raptor-Windows-x64"
set "ZIP=%ROOT%dist\Raptor-Windows-x64.zip"

set "JAVABIN="
set "USE_JAVA_HOME=0"
if defined JAVA_HOME if exist "%JAVA_HOME%\bin\javac.exe" (
  set "JAVABIN=%JAVA_HOME%\bin"
  set "USE_JAVA_HOME=1"
)
if "%USE_JAVA_HOME%"=="1" goto tools_ok
where javac.exe >nul 2>nul || goto no_jdk
where jar.exe >nul 2>nul || goto no_jdk
where jlink.exe >nul 2>nul || goto no_jdk
:tools_ok

if not exist "%BUILD%\raptor.dll" (
  echo ERROR: Missing "%BUILD%\raptor.dll". Build the native Release DLL first.
  pause
  exit /b 1
)
if not exist "%BUILD%\models" (
  echo ERROR: Missing Release models folder.
  pause
  exit /b 1
)
if not exist "%BUILD%\shaders" (
  echo ERROR: Missing Release shaders folder.
  pause
  exit /b 1
)
if not exist "%JAVA_SRC%\Main.java" (
  echo ERROR: Missing java\Main.java
  pause
  exit /b 1
)
if not exist "%JAVA_SRC%\Raptor.java" (
  echo ERROR: Missing java\Raptor.java
  pause
  exit /b 1
)

 echo.
 echo [1/6] Compiling Java 27 classes...
 if exist "%BUILD%\java" rmdir /s /q "%BUILD%\java"
 mkdir "%BUILD%\java" || goto failed
 if "%USE_JAVA_HOME%"=="1" (
   "%JAVABIN%\javac.exe" --release 27 -d "%BUILD%\java" "%JAVA_SRC%\Main.java" "%JAVA_SRC%\Raptor.java"
 ) else (
   javac.exe --release 27 -d "%BUILD%\java" "%JAVA_SRC%\Main.java" "%JAVA_SRC%\Raptor.java"
 )
 if errorlevel 1 goto failed

 echo.
 echo [2/6] Building a clean JAR containing Java classes only...
 if not exist "%ROOT%dist" mkdir "%ROOT%dist"
 if exist "%DIST%" rmdir /s /q "%DIST%"
 mkdir "%DIST%" || goto failed
 if "%USE_JAVA_HOME%"=="1" (
   "%JAVABIN%\jar.exe" --create --file "%DIST%\Raptor-Release.jar" --main-class Main -C "%BUILD%\java" .
 ) else (
   jar.exe --create --file "%DIST%\Raptor-Release.jar" --main-class Main -C "%BUILD%\java" .
 )
 if errorlevel 1 goto failed

 echo.
 echo [3/6] Building the console launcher with CMake...
 if not exist "%CMAKE_BUILD%" (
   echo ERROR: CMake build folder not found: "%CMAKE_BUILD%"
   echo Update CMAKE_BUILD in this script to your actual CMake build directory.
   goto failed
 )
 cmake --build "%CMAKE_BUILD%" --config Release --target RaptorJarLauncher
 if errorlevel 1 goto failed
 if not exist "%BUILD%\RaptorJarLauncher.exe" (
   echo ERROR: Launcher was not created at "%BUILD%\RaptorJarLauncher.exe"
   goto failed
 )
 copy /y "%BUILD%\RaptorJarLauncher.exe" "%DIST%\RaptorJarLauncher.exe" >nul || goto failed

 echo.
 echo [4/6] Copying native DLL and engine assets beside the launcher...
 copy /y "%BUILD%\raptor.dll" "%DIST%\raptor.dll" >nul || goto failed
 robocopy "%BUILD%\models" "%DIST%\models" /E /R:1 /W:1
 if errorlevel 8 goto failed
 robocopy "%BUILD%\shaders" "%DIST%\shaders" /E /R:1 /W:1
 if errorlevel 8 goto failed

 echo.
 echo [5/6] Bundling a Java runtime so end users do not need Java installed...
 if "%USE_JAVA_HOME%"=="1" (
   if not exist "%JAVABIN%\jlink.exe" goto no_jdk
   "%JAVABIN%\jlink.exe" --add-modules java.base --strip-debug --no-header-files --no-man-pages --compress=2 --output "%DIST%\runtime"
 ) else (
   jlink.exe --add-modules java.base --strip-debug --no-header-files --no-man-pages --compress=2 --output "%DIST%\runtime"
 )
 if errorlevel 1 goto failed

 echo.
 echo [6/6] Creating portable ZIP...
 if exist "%ZIP%" del /q "%ZIP%"
 powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '%DIST%\*' -DestinationPath '%ZIP%' -Force"
 if errorlevel 1 goto failed

 echo.
 echo ========================================
 echo SUCCESS: Portable Raptor package built.
 echo Folder: %DIST%
 echo ZIP:    %ZIP%
 echo ========================================
 echo This package bundles Java, but requires compatible Windows x64 hardware,
 echo a working Vulkan driver, and any non-system DLL dependencies of raptor.dll.
 pause
 exit /b 0

:no_jdk
 echo ERROR: A full JDK 27 is required to build this package.
 echo Set JAVA_HOME to the JDK 27 folder or add javac.exe, jar.exe and jlink.exe to PATH.
 pause
 exit /b 1

:failed
 echo.
 echo ERROR: Build/package step failed. Review the messages above.
 pause
 exit /b 1
