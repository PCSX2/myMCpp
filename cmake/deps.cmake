# SPDX-FileCopyrightText: 2025 SternXD
# SPDX-License-Identifier: GPL-3.0+
# Deps management for myMCpp
include(FetchContent)

# ============================================================================
# zlib - A massively spiffy yet delicately unobtrusive compression library.
# ============================================================================
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1
    GIT_SHALLOW TRUE
)

# ============================================================================
# Vulkan - Vulkan header files and API registry
# ============================================================================
FetchContent_Declare(
    Vulkan-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG v1.4.336
    GIT_SHALLOW TRUE
)

# ============================================================================
# SPIRV - SPIRV-Headers
# ============================================================================
FetchContent_Declare(
    SPIRV-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
    GIT_TAG vulkan-sdk-1.4.335.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    SPIRV-Tools
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
    GIT_TAG vulkan-sdk-1.4.335.0
    GIT_SHALLOW TRUE
)

set(SPIRV_SKIP_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
set(SPIRV_WARN_EVERYTHING OFF CACHE BOOL "" FORCE)

# ============================================================================
# GLM - OpenGL Mathematics (GLM)
# ============================================================================
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
    GIT_SHALLOW TRUE
)

# ============================================================================
# GLFW - A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input
# ============================================================================
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# ============================================================================
# GLAD - Multi-Language Vulkan/GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs.
# ============================================================================
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG glad2
    GIT_SHALLOW TRUE
)

# ============================================================================
# nlohmann/json - JSON for Modern C++
# ============================================================================
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

# ============================================================================
# spdlog - Fast C++ logging library
# ============================================================================
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.16.0
    GIT_SHALLOW TRUE
)

# Disable zlib examples/tests
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_BUILD_EXAMPLES ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(zlib Vulkan-Headers SPIRV-Headers SPIRV-Tools glm glfw glad nlohmann_json spdlog)

# Disable zlib example and test targets
if(TARGET example)
    set_target_properties(example PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET minigzip)
    set_target_properties(minigzip PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# ============================================================================
# MSVC specific stuff
# ============================================================================
if(MSVC AND TARGET SPIRV-Tools-opt)
    target_compile_options(SPIRV-Tools-opt PRIVATE /W0 /wd5232)
endif()

# ============================================================================
# Find system packages
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(Vulkan REQUIRED)
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED COMPONENTS EGL)
else()
    find_package(OpenGL REQUIRED)
endif()
