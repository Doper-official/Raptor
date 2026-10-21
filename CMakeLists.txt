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
set(RAPTOR_TEST_DIR "${RAPTOR_ROOT}/test builds")
set(RAPTOR_RELEASE_DIR "${RAPTOR_ROOT}/Release builds")

file(MAKE_DIRECTORY "${RAPTOR_TEST_DIR}")
file(MAKE_DIRECTORY "${RAPTOR_RELEASE_DIR}")

# ============================================================
# Vulkan SDK
# ============================================================

if(DEFINED ENV{VULKAN_SDK})
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
        "Java compilation disabled."
    )
endif()

set(JAVA_SOURCES
    "${RAPTOR_ROOT}/java/Main.java"
    "${RAPTOR_ROOT}/java/Raptor.java"
)

set(JAVA_OUTPUT_DIR "${RAPTOR_TEST_DIR}/java")

if(Java_FOUND)
    if(EXISTS "${RAPTOR_ROOT}/java/Main.java"
       AND EXISTS "${RAPTOR_ROOT}/java/Raptor.java")

        add_custom_command(
            OUTPUT "${JAVA_OUTPUT_DIR}/Main.class"

            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "${JAVA_OUTPUT_DIR}"

            COMMAND "${Java_JAVAC_EXECUTABLE}"
                    -d "${JAVA_OUTPUT_DIR}"
                    ${JAVA_SOURCES}

            DEPENDS ${JAVA_SOURCES}

            WORKING_DIRECTORY "${RAPTOR_ROOT}"
            COMMENT "Compiling Raptor Java sources"
            VERBATIM
        )

        add_custom_target(JavaApp
            DEPENDS "${JAVA_OUTPUT_DIR}/Main.class"
        )

    else()
        message(WARNING
            "Java sources not found at java/Main.java and java/Raptor.java. "
            "Java compilation target disabled."
        )
    endif()
endif()

# ============================================================
# Dependencies
# ============================================================

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
)

FetchContent_MakeAvailable(tinyobjloader)

# ============================================================
# Shader compilation
# ============================================================

set(SHADER_SCRIPT "${RAPTOR_ROOT}/shaders/compileShaders.bat")

if(WIN32 AND EXISTS "${SHADER_SCRIPT}")
    add_custom_target(CompileShaders
        COMMAND cmd /c "${SHADER_SCRIPT}"
        WORKING_DIRECTORY "${RAPTOR_ROOT}"
        COMMENT "Compiling Vulkan shaders"
        VERBATIM
    )
else()
    message(WARNING
        "Shader compilation script not found: ${SHADER_SCRIPT}"
    )
endif()

# ============================================================
# Raptor native engine DLL
# ============================================================

add_library(raptor SHARED

    #src
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

    #systems
    src/systems/point_light_system.cpp
    src/systems/simple_render_system.cpp
    src/systems/deferred_lighting_system.cpp

    #buffers
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
)

if(WIN32)
    target_compile_definitions(raptor
        PRIVATE
            RAPTOR_BUILD_DLL
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )

    # Windows taskbar identity support.
    target_link_libraries(raptor
        PRIVATE
            shell32
            ole32
            propsys
    )

    if(TARGET CompileShaders)
        add_dependencies(raptor CompileShaders)
    endif()
endif()

if(TARGET JavaApp)
    add_dependencies(raptor JavaApp)
endif()

# Output directories for the engine DLL.
set_target_properties(raptor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG
        "${RAPTOR_TEST_DIR}"

    RUNTIME_OUTPUT_DIRECTORY_RELEASE
        "${RAPTOR_RELEASE_DIR}"

    LIBRARY_OUTPUT_DIRECTORY_DEBUG
        "${RAPTOR_TEST_DIR}"

    LIBRARY_OUTPUT_DIRECTORY_RELEASE
        "${RAPTOR_RELEASE_DIR}"

    ARCHIVE_OUTPUT_DIRECTORY_DEBUG
        "${RAPTOR_TEST_DIR}/lib"

    ARCHIVE_OUTPUT_DIRECTORY_RELEASE
        "${RAPTOR_RELEASE_DIR}/lib"
)

# ============================================================
# C launcher for Raptor-Test.jar
# ============================================================

if(WIN32)
    if(EXISTS "${RAPTOR_ROOT}/C/launcher.c")

        add_executable(RaptorJarLauncher WIN32
            C/launcher.c
        )

        target_link_libraries(RaptorJarLauncher
            PRIVATE
                user32
                shell32
        )

        target_compile_options(RaptorJarLauncher
            PRIVATE
                /W4
                $<$<CONFIG:Release>:/O2>
        )

        # Each Visual Studio configuration gets its own output path.
        set_target_properties(RaptorJarLauncher PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG
                "${RAPTOR_TEST_DIR}"

            RUNTIME_OUTPUT_DIRECTORY_RELEASE
                "${RAPTOR_RELEASE_DIR}"
        )

        # Build the launcher whenever the engine is built.
        add_dependencies(raptor RaptorJarLauncher)

    else()
        message(WARNING
            "C/launcher.c not found. "
            "RaptorJarLauncher target disabled."
        )
    endif()
endif()

# ============================================================
# Build information
# ============================================================

message(STATUS "========================================")
message(STATUS "Raptor Engine Configuration")
message(STATUS "========================================")
message(STATUS "Root: ${RAPTOR_ROOT}")
message(STATUS "Vulkan SDK: ${VULKAN_SDK}")
message(STATUS "Test output: ${RAPTOR_TEST_DIR}")
message(STATUS "Release output: ${RAPTOR_RELEASE_DIR}")
message(STATUS "C launcher source: ${RAPTOR_ROOT}/C/launcher.c")
message(STATUS "========================================")