RAPTOR PORTABLE LAUNCHER — FULL SOURCE BUNDLE

WHAT THIS DOES
- Builds a console launcher for Windows x64.
- Builds a clean Raptor-Release.jar containing Java classes only.
- Copies raptor.dll, models/, and shaders/ beside the launcher and JAR.
- Bundles a minimal Java runtime with jlink, so a separate Java install is not required on the target PC.
- Sets the process working directory to the launcher folder so native relative paths such as models/quad.objmodel and shaders/*.spv resolve from the portable folder.
- Java FFM explicitly loads the absolute path <portable-folder>\raptor.dll.

SOURCE FILES
  C\launcher.c               Windows console launcher source
  java\Main.java             Java entry point
  java\Raptor.java           Java FFM wrapper for raptor.dll
  build-release-package.bat  release packaging script
  CMake-snippet.txt          target setup to merge into your existing CMakeLists.txt

EXAMPLE-PORTABLE-FOLDER
Contains models/, shaders/, and raptor.dll extracted from the Raptor-Release.zip/JAR supplied in this conversation. They demonstrate the required relative layout. The build script copies assets from your current Release builds folder instead.

INSTALL
1. Copy C\launcher.c into your project at C\launcher.c (create C if needed).
2. Copy java\Main.java and java\Raptor.java into your project java\ folder.
3. Merge CMake-snippet.txt into your existing CMakeLists.txt. If RaptorJarLauncher already exists, edit that target rather than adding a duplicate.
4. Make sure your C++ Release build places these at the following paths (or edit the script paths):
     Release builds\raptor.dll
     Release builds\models\...
     Release builds\shaders\...
5. Check CMAKE_BUILD in build-release-package.bat. It currently expects:
     out\build\x64-Release
6. Ensure JAVA_HOME points to a full JDK 27 installation, or put javac.exe, jar.exe, and jlink.exe from JDK 27 on PATH.
7. Run build-release-package.bat from your Engine project root.

OUTPUT
  dist\Raptor-Windows-x64\
    RaptorJarLauncher.exe
    Raptor-Release.jar
    raptor.dll
    models\...
    shaders\...
    runtime\bin\java.exe
  dist\Raptor-Windows-x64.zip

IMPORTANT LIMITS
- This targets Windows x64 only; it is not one binary for Windows/macOS/Linux or ARM.
- It still requires a working Vulkan-capable GPU/driver.
- If raptor.dll depends on additional non-system DLLs, those dependencies must also be copied into the portable folder or otherwise made available.
- The source bundle has not been built/tested against your live project configuration. CMake output paths and dependency DLLs may need adjustment.
