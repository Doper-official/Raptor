RAPTOR PROCEDURAL SKYBOX PATCH

This patch adds a procedural skybox as a separate graphics system.
It does not put sky calculations into deferred_lighting.frag.

Files added:
  include/systems/skybox_system.hpp
  src/systems/skybox_system.cpp

Files modified:
  CMakeLists.txt
  src/first_app.cpp
  shaders/skybox.vert
  shaders/skybox.frag
  shaders/deferred_lighting.frag

Integration:
  - Skybox renders inside the existing swapchain render pass.
  - It renders before deferred lighting.
  - Deferred lighting discards G-buffer background pixels, preserving sky pixels.
  - Point-light rendering remains after deferred lighting.
  - Existing compileShaders.bat already compiles all .vert and .frag files,
    so it automatically compiles both skybox shaders.

The source was inspected and patched against the uploaded include.zip.
The patch was not compiled in this environment because glslc and the Vulkan SDK
are not installed here. Build and run it on the Windows machine with the
project's Vulkan SDK to validate the integration.
