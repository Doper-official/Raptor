cmake_minimum_required(VERSION 3.20)

project(Engine LANGUAGES CXX)

# ============================================================
# C++ Standard
# ============================================================

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Output directories
# ============================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    "${CMAKE_CURRENT_SOURCE_DIR}/test builds"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    "${CMAKE_CURRENT_SOURCE_DIR}/Release builds"
)

# ============================================================
# Vulkan
# ============================================================

set(VULKAN_SDK "F:/Vulkan")

find_package(Vulkan REQUIRED)

# ============================================================
# Dependencies
# ============================================================

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ------------------------------------------------------------
# GLFW 3.4
# ------------------------------------------------------------

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

# ------------------------------------------------------------
# GLM 1.0.1
# ------------------------------------------------------------

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# ------------------------------------------------------------
# tinyobjloader
# ------------------------------------------------------------

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
)

FetchContent_MakeAvailable(tinyobjloader)

# ============================================================
# Main executable
#
# Headers: Engine/include/
# Sources: Engine/src/
# ============================================================

add_executable(Main
    # Main
    src/main.cpp

    # Core
    src/rap_window.cpp
    src/first_app.cpp
    src/rap_pipeline.cpp
    src/rap_device.cpp
    src/rap_swap_chain.cpp
    src/rap_model.cpp
    src/rap_game_object.cpp
    src/rap_renderer.cpp
    src/rap_camera.cpp
    src/keyboard_movement_controller.cpp
    src/rap_buffer.cpp
    src/rap_descriptors.cpp

    # Rendering systems
    src/systems/simple_render_system.cpp
    src/systems/point_light_system.cpp

    # Headers
    include/rap_window.hpp
    include/first_app.hpp
    include/rap_pipeline.hpp
    include/rap_device.hpp
    include/rap_swap_chain.hpp
    include/rap_model.hpp
    include/rap_game_object.hpp
    include/rap_renderer.hpp
    include/rap_camera.hpp
    include/keyboard_movement_controller.hpp
    include/rap_buffer.hpp
    include/rap_descriptors.hpp
    include/rap_utils.hpp
    include/rap_frame_info.hpp

    # Rendering system headers
    include/systems/simple_render_system.hpp
    include/systems/point_light_system.hpp
)

# ============================================================
# Include directories
# ============================================================

target_include_directories(Main PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${tinyobjloader_SOURCE_DIR}"
)

# ============================================================
# Windows executable settings
# Hide console in Release only
# ============================================================

if(WIN32)
    set_target_properties(Main PROPERTIES
        WIN32_EXECUTABLE "$<CONFIG:Release>"
    )

    target_link_options(Main PRIVATE
        "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>"
    )
endif()

# ============================================================
# Libraries
# ============================================================

target_link_libraries(Main PRIVATE
    glfw
    glm::glm
    Vulkan::Vulkan
)

# ============================================================
# Compile Vulkan shaders using the existing .bat
#
# Assumed location:
# Engine/shaders/compile_shaders.bat
# ============================================================

set(SHADER_BATCH
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/compileShaders.bat"
)

if(NOT EXISTS "${SHADER_BATCH}")
    message(FATAL_ERROR
        "Shader compilation batch file not found: ${SHADER_BATCH}"
    )
endif()

add_custom_target(CompileShaders ALL
    COMMAND cmd /c "${SHADER_BATCH}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    COMMENT "Compiling Vulkan shaders using compile_shaders.bat..."
    VERBATIM
)

add_dependencies(Main CompileShaders)

# ============================================================
# Debugging information
# ============================================================

message(STATUS "Vulkan SDK: ${VULKAN_SDK}")
message(STATUS "Source directory: ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Include directory: ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Debug output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")
message(STATUS "Release output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}")
message(STATUS "Shader batch: ${SHADER_BATCH}")