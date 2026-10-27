cmake_minimum_required(VERSION 3.20)

project(Raptor LANGUAGES C CXX)

# ============================================================
# Language standards
# ============================================================

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Directories
# ============================================================

set(RAPTOR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(RAPTOR_TEST_DIR "${RAPTOR_ROOT}/Test builds")
set(RAPTOR_RELEASE_DIR "${RAPTOR_ROOT}/Release builds")

file(MAKE_DIRECTORY
    "${RAPTOR_TEST_DIR}"
    "${RAPTOR_RELEASE_DIR}"
)

# ============================================================
# Vulkan SDK
# ============================================================

if(DEFINED ENV{VULKAN_SDK} AND NOT "$ENV{VULKAN_SDK}" STREQUAL "")
    set(VULKAN_SDK "$ENV{VULKAN_SDK}")
else()
    set(VULKAN_SDK "F:/Vulkan")
    set(ENV{VULKAN_SDK} "${VULKAN_SDK}")
endif()

find_package(Vulkan REQUIRED)

# ============================================================
# Java
# ============================================================

find_package(Java 22 COMPONENTS Development QUIET)

if(Java_FOUND)
    message(STATUS "Java compiler: ${Java_JAVAC_EXECUTABLE}")
else()
    message(WARNING
        "Java Development Kit 22+ not found. "
        "Java compilation is disabled."
    )
endif()

set(RAPTOR_JAVA_SOURCES
    "${RAPTOR_ROOT}/java/Main.java"
    "${RAPTOR_ROOT}/java/Raptor.java"
)

set(RAPTOR_JAVA_DEBUG_DIR "${RAPTOR_TEST_DIR}/java")
set(RAPTOR_JAVA_RELEASE_DIR "${RAPTOR_RELEASE_DIR}/java")

if(
    Java_FOUND
    AND EXISTS "${RAPTOR_ROOT}/java/Main.java"
    AND EXISTS "${RAPTOR_ROOT}/java/Raptor.java"
)
    add_custom_target(JavaApp
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<IF:$<CONFIG:Debug>,${RAPTOR_JAVA_DEBUG_DIR},${RAPTOR_JAVA_RELEASE_DIR}>"

        COMMAND
            "${Java_JAVAC_EXECUTABLE}"
            -d "$<IF:$<CONFIG:Debug>,${RAPTOR_JAVA_DEBUG_DIR},${RAPTOR_JAVA_RELEASE_DIR}>"
            ${RAPTOR_JAVA_SOURCES}

        DEPENDS
            ${RAPTOR_JAVA_SOURCES}

        WORKING_DIRECTORY "${RAPTOR_ROOT}"

        COMMENT "Compiling Raptor Java sources for $<CONFIG>"

        VERBATIM
    )
elseif(Java_FOUND)
    message(WARNING
        "Java sources missing. Expected java/Main.java and java/Raptor.java."
    )
endif()

# ============================================================
# Dependencies
# ============================================================

include(FetchContent)

# GLFW

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)

# GLM

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)

# TinyOBJLoader

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
    GIT_SHALLOW TRUE
)

# TinyGLTF

set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_GL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.9.5
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(
    glfw
    glm
    tinyobjloader
    tinygltf
)

# ============================================================
# Shader compilation
# ============================================================

set(RAPTOR_SHADER_SOURCE_DIR "${RAPTOR_ROOT}/shaders")
set(SHADER_SCRIPT "${RAPTOR_SHADER_SOURCE_DIR}/compileShaders.bat")

if(NOT WIN32)
    message(FATAL_ERROR
        "Raptor shader compilation currently requires Windows."
    )
endif()

if(NOT EXISTS "${SHADER_SCRIPT}")
    message(FATAL_ERROR
        "Shader compilation script not found: ${SHADER_SCRIPT}"
    )
endif()

add_custom_target(CompileShaders ALL
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "VULKAN_SDK=${VULKAN_SDK}"
        cmd /c call "${SHADER_SCRIPT}"

    WORKING_DIRECTORY "${RAPTOR_SHADER_SOURCE_DIR}"

    COMMENT "Compiling Vulkan shaders for Test and Release builds"

    VERBATIM
)

# ============================================================
# Model copying
# ============================================================

set(RAPTOR_MODELS_DIR "${RAPTOR_ROOT}/models")
set(MODEL_COPY_SCRIPT "${RAPTOR_ROOT}/copyModels.bat")

if(NOT IS_DIRECTORY "${RAPTOR_MODELS_DIR}")
    message(FATAL_ERROR
        "Models directory not found: ${RAPTOR_MODELS_DIR}"
    )
endif()

if(NOT EXISTS "${MODEL_COPY_SCRIPT}")
    message(FATAL_ERROR
        "Model copy script not found: ${MODEL_COPY_SCRIPT}"
    )
endif()

add_custom_target(CopyModels ALL
    COMMAND
        cmd /c call "${MODEL_COPY_SCRIPT}"

    WORKING_DIRECTORY "${RAPTOR_ROOT}"

    COMMENT "Copying Raptor models to Test and Release builds"

    VERBATIM
)

# ============================================================
# Raptor native engine DLL
# ============================================================

add_library(raptor SHARED
    # Core
    src/raptor_api.cpp
    src/first_app.cpp
    src/rap_window.cpp
    src/rap_device.cpp
    src/rap_renderer.cpp
    src/rap_swap_chain.cpp
    src/rap_pipeline.cpp
    src/rap_buffer.cpp
    src/rap_game_object.cpp
    src/rap_camera.cpp
    src/rap_model.cpp
    src/rap_descriptors.cpp
    src/rap_image.cpp
    src/keyboard_movement_controller.cpp

    # TinyGLTF implementation
    src/tinygltf_impl.cpp

    # Systems
    src/systems/point_light_system.cpp
    src/systems/simple_render_system.cpp
    src/systems/deferred_lighting_system.cpp

    # Buffers
    src/buffers/rap_geometry_buffer.cpp
)

target_include_directories(raptor
    PUBLIC
        "${RAPTOR_ROOT}/include"
)

target_link_libraries(raptor
    PRIVATE
        glfw
        glm::glm
        Vulkan::Vulkan
        tinyobjloader
        tinygltf
)

if(WIN32)
    target_compile_definitions(raptor
        PRIVATE
            RAPTOR_BUILD_DLL
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )

    target_link_libraries(raptor
        PRIVATE
            shell32
            ole32
            propsys
    )
endif()

# Ensure shaders and models are prepared before building the DLL.

add_dependencies(raptor
    CompileShaders
    CopyModels
)

if(TARGET JavaApp)
    add_dependencies(raptor JavaApp)
endif()

set_target_properties(raptor PROPERTIES
    OUTPUT_NAME "raptor"

    RUNTIME_OUTPUT_DIRECTORY_DEBUG
        "${RAPTOR_TEST_DIR}"

    LIBRARY_OUTPUT_DIRECTORY_DEBUG
        "${RAPTOR_TEST_DIR}"

    ARCHIVE_OUTPUT_DIRECTORY_DEBUG
        "${RAPTOR_TEST_DIR}/lib"

    RUNTIME_OUTPUT_DIRECTORY_RELEASE
        "${RAPTOR_RELEASE_DIR}"

    LIBRARY_OUTPUT_DIRECTORY_RELEASE
        "${RAPTOR_RELEASE_DIR}"

    ARCHIVE_OUTPUT_DIRECTORY_RELEASE
        "${RAPTOR_RELEASE_DIR}/lib"
)

# ============================================================
# C launcher
# ============================================================

if(WIN32 AND EXISTS "${RAPTOR_ROOT}/C/launcher.c")
    add_executable(RaptorJarLauncher
        C/launcher.c
    )

    set_target_properties(RaptorJarLauncher PROPERTIES
        WIN32_EXECUTABLE FALSE

        RUNTIME_OUTPUT_DIRECTORY_DEBUG
            "${RAPTOR_TEST_DIR}"

        RUNTIME_OUTPUT_DIRECTORY_RELEASE
            "${RAPTOR_RELEASE_DIR}"
    )

    target_link_libraries(RaptorJarLauncher
        PRIVATE
            user32
            shell32
    )

    if(MSVC)
        target_compile_options(RaptorJarLauncher
            PRIVATE
                /W4
                $<$<CONFIG:Release>:/O2>
        )
    endif()

    add_dependencies(RaptorJarLauncher
        CompileShaders
        CopyModels
    )

    add_dependencies(raptor RaptorJarLauncher)
else()
    message(WARNING
        "C/launcher.c not found. RaptorJarLauncher target disabled."
    )
endif()

# ============================================================
# Build information
# ============================================================

message(STATUS "========================================")
message(STATUS "Raptor Engine Configuration")
message(STATUS "========================================")
message(STATUS "Root: ${RAPTOR_ROOT}")
message(STATUS "Vulkan SDK: ${VULKAN_SDK}")
message(STATUS "C standard: C23")
message(STATUS "C++ standard: C++23")
message(STATUS "Debug output: ${RAPTOR_TEST_DIR}")
message(STATUS "Release output: ${RAPTOR_RELEASE_DIR}")
message(STATUS "Models source: ${RAPTOR_MODELS_DIR}")
message(STATUS "Model copy script: ${MODEL_COPY_SCRIPT}")
message(STATUS "Shader source: ${RAPTOR_SHADER_SOURCE_DIR}")
message(STATUS "Shader script: ${SHADER_SCRIPT}")
message(STATUS "Java enabled: ${Java_FOUND}")
message(STATUS "========================================")