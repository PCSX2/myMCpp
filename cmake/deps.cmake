# SPDX-FileCopyrightText: 2025 SternXD
# SPDX-License-Identifier: GPL-3.0+
# Deps management for myMCpp
include(FetchContent)

if(APPLE)
    option(ENABLE_VULKAN "Enable Vulkan renderer" OFF)
else()
    option(ENABLE_VULKAN "Enable Vulkan renderer" ON)
endif()

# ============================================================================
# zlib - A massively spiffy yet delicately unobtrusive compression library.
# ============================================================================
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1
    GIT_SHALLOW TRUE
    GIT_DEPTH 1
)

if(ENABLE_VULKAN)

    # ============================================================================
    # Vulkan-Loader - Vulkan loader
    # ============================================================================
    FetchContent_Declare(
        Vulkan-Loader
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
        GIT_TAG v1.4.337
        GIT_SHALLOW TRUE
        GIT_DEPTH 1
    )

    # ============================================================================
    # Vulkan - Vulkan header files and API registry
    # ============================================================================
    FetchContent_Declare(
        Vulkan-Headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG v1.4.337
        GIT_SHALLOW TRUE
        GIT_DEPTH 1
    )

    # ============================================================================
    # SPIRV - SPIRV-Headers
    # ============================================================================
    FetchContent_Declare(
        SPIRV-Headers
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        GIT_DEPTH 1
    )

    FetchContent_Declare(
        SPIRV-Tools
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
        GIT_TAG v2025.5
        GIT_SHALLOW TRUE
        GIT_DEPTH 1
    )

    # ============================================================================
    # glslang - Khronos-reference front end for GLSL/ESSL, partial front end for HLSL, and a SPIR-V generator.
    # ============================================================================
    FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
        GIT_TAG 16.1.0
        GIT_SHALLOW TRUE
        GIT_DEPTH 1
    )

    set(SPIRV_SKIP_EXECUTABLES OFF CACHE BOOL "" FORCE)
    set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
    set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
    set(SPIRV_WARN_EVERYTHING OFF CACHE BOOL "" FORCE)

    # glslang options
    set(ENABLE_GLSLANG_BINARIES ON CACHE BOOL "" FORCE)
    set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
    set(ENABLE_HLSL ON CACHE BOOL "" FORCE)
    set(ENABLE_OPT ON CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
endif()

# ============================================================================
# GLM - OpenGL Mathematics (GLM)
# ============================================================================
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE
    GIT_DEPTH 1
)

# ============================================================================
# GLFW - A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input
# ============================================================================
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_DEPTH 1
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
    GIT_DEPTH 1
)

# ============================================================================
# nlohmann/json - JSON for Modern C++
# ============================================================================
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
    GIT_DEPTH 1
)

# ============================================================================
# spdlog - Fast C++ logging library
# ============================================================================
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
    GIT_DEPTH 1
)

# Disable zlib examples/tests
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_BUILD_EXAMPLES ON CACHE BOOL "" FORCE)

set(VULKAN_DEPS)
if(ENABLE_VULKAN)
    list(APPEND VULKAN_DEPS Vulkan-Loader Vulkan-Headers SPIRV-Headers SPIRV-Tools glslang)
endif()

FetchContent_MakeAvailable(zlib ${VULKAN_DEPS} glm glfw glad nlohmann_json spdlog)

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
# Find system packages (OpenGL, Vulkan)
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS Core Widgets LinguistTools)
if (Qt6_VERSION VERSION_GREATER_EQUAL 6.10.0)
	find_package(Qt6 COMPONENTS CorePrivate GuiPrivate WidgetsPrivate REQUIRED)
endif()

if(ENABLE_VULKAN)
    find_package(Vulkan QUIET)
    if(NOT TARGET Vulkan::Vulkan)
        message(STATUS "System Vulkan not found. Building Vulkan-Loader from source...")
        
        # Configure Loader options
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(BUILD_WSI_XCB_SUPPORT OFF CACHE BOOL "" FORCE)
        set(BUILD_WSI_XLIB_SUPPORT OFF CACHE BOOL "" FORCE)
        set(BUILD_WSI_WAYLAND_SUPPORT OFF CACHE BOOL "" FORCE)
        set(BUILD_WSI_DIRECTFB_SUPPORT OFF CACHE BOOL "" FORCE)
        set(UPDATE_DEPS OFF CACHE BOOL "" FORCE) # Don't let it try to update headers itself
        
        FetchContent_MakeAvailable(Vulkan-Loader)
        
        if(TARGET vulkan)
            add_library(Vulkan::Vulkan ALIAS vulkan)
            message(STATUS "Using fetched Vulkan-Loader")
        else()
             message(FATAL_ERROR "Failed to build Vulkan-Loader from source")
        endif()
    else()
        message(STATUS "Using system Vulkan: ${Vulkan_LIBRARY}")
    endif()
endif()

if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED COMPONENTS EGL)
else()
    find_package(OpenGL REQUIRED)
endif()
message(STATUS "Found OpenGL")
