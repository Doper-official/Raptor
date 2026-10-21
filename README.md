# Raptor — Java 27 FFM + C++ Vulkan DLL

Raptor's Vulkan engine remains C++23. Java is the application entry point and calls a small exported C ABI through Java's Foreign Function & Memory (FFM) API.

## Architecture

```text
Java 27: raptor.RaptorFirstApp
          |
          | Java FFM downcalls (C ABI)
          v
raptor.dll: raptor_first_app_create / run / destroy
          |
          v
Existing C++ RaptorFirstApp + Vulkan renderer, GLFW, GLM, tinyobjloader
```

The public native interface is `include/raptor_api.h`. Java never calls C++ class methods directly and never owns Vulkan handles. Exceptions are caught at the C ABI boundary and can be read with `raptor_last_error()`.

## Requirements

- Windows x64
- Visual Studio 2026 C++ build tools or a compatible MSVC toolchain
- CMake 3.20+
- Git (CMake FetchContent downloads GLFW 3.4, GLM 1.0.1, and tinyobjloader on the first configure)
- Vulkan SDK, with `VULKAN_SDK` set; the original `F:\Vulkan` location is used as a fallback
- JDK 27 recommended (`javac` and `java` available on `PATH`)

## Build from Visual Studio

1. Install/use a JDK 27 x64 build and make sure `javac.exe` is discoverable (for example, add the JDK `bin` folder to `PATH`).
2. In Visual Studio, choose **File → Open → Folder** and open the extracted repository folder (the folder containing `CMakeLists.txt`).
3. Wait for CMake configuration to finish. Select the `x64-Debug` configuration.
4. Choose **Build → Build All** (or the **Build All** button). Building the `raptor` target also builds the Java class and shaders.

The CMake project compiles the Java entry point into `out/build/<configuration>/java-classes`. Visual Studio C++ IntelliSense may still show red squiggles in the `.java` file because this is a CMake/C++ project; check the **Build Output** for actual errors.

## Build from the command line

Run these commands from the repository root in a Developer PowerShell for your x64 Visual Studio toolchain:

```powershell
cmake -S . -B out/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build
```

If your current Visual Studio CMake profile already configures Ninja and the MSVC x64 environment, you can build the `raptor` target through that profile instead. The DLL is written to `test builds/raptor.dll` for Debug or `Release builds/raptor.dll` for Release.

The build compiles the GLSL shader sources into `shaders/*.spv`. The batch file no longer pauses for keyboard input, so it can run as part of CMake.

## Build and run Java

From the repository root:

```powershell
.\build-java.ps1
.\run-java.ps1
```

`run-java.ps1` uses the project root as the working directory because the existing C++ engine loads models from `models/` and compiled shaders from `shaders/` using relative paths.

To choose a particular DLL explicitly:

```powershell
.\run-java.ps1 "F:\full\path\to\raptor.dll"
```

The script passes the supplied argument to the Java entry point. Use the same x64 architecture for the JDK and native DLL.

## Native C ABI

- `raptor_first_app_create()` creates the C++ application and returns an opaque handle, or null on failure.
- `raptor_first_app_run(handle)` enters the existing Vulkan render loop and returns `0` on success.
- `raptor_first_app_destroy(handle)` destroys the application.
- `raptor_last_error()` returns the last native error message for the current thread.

Do not allow C++ exceptions to cross the C ABI. The wrapper catches them and reports errors to Java.

## Notes / limitations

- This conversion preserves the current C++ `rap::RaptorFirstApp` implementation and makes Java responsible for calling its lifecycle through FFM. The rendering and frame loop are still C++ in this first integration step; moving higher-level game/application logic into Java can be done incrementally afterward.
- The existing `src/main.cpp` is retained as a reference/native-test entry point but is not part of the DLL build. Java is the configured application entry point.
- I have not claimed a successful native build or runtime test in this environment. The project needs to be built on the target Windows machine with its Vulkan SDK and C++ toolchain.
