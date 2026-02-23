# SPDX-FileCopyrightText: 2025-2026 SternXD
# SPDX-License-Identifier: GPL-3.0+
# Deps management for myMCpp
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

if(APPLE)
    option(ENABLE_VULKAN "Enable Vulkan renderer" OFF)
else()
    option(ENABLE_VULKAN "Enable Vulkan renderer" ON)
endif()

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
endif()

# ============================================================================
# zlib - A massively spiffy yet delicately unobtrusive compression library.
# ============================================================================
CPMAddPackage(
    NAME zlib
    GITHUB_REPOSITORY madler/zlib
    GIT_TAG v1.3.2
    GIT_SHALLOW TRUE
    OPTIONS
        "ZLIB_BUILD_EXAMPLES OFF"
        "ZLIB_BUILD_TESTS OFF"
        "SKIP_BUILD_EXAMPLES ON"
)

if(TARGET example)
    set_target_properties(example PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET minigzip)
    set_target_properties(minigzip PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# ============================================================================
# GLM - OpenGL Mathematics
# ============================================================================
CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE
)

# ============================================================================
# GLFW - Multi-platform library for OpenGL/Vulkan/window/input
# ============================================================================
CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG master
    GIT_SHALLOW TRUE
    OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_INSTALL OFF"
)

# ============================================================================
# GLAD - Vulkan/GL/GLES/EGL/GLX/WGL Loader-Generator
# ============================================================================
CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    GIT_TAG glad2
    GIT_SHALLOW TRUE
)

# ============================================================================
# nlohmann/json - JSON for Modern C++
# ============================================================================
CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

# ============================================================================
# spdlog - Fast C++ logging library
# ============================================================================
CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
)

# ============================================================================
# Vulkan deps
# ============================================================================
if(ENABLE_VULKAN)
    if(WIN32 AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "Clang"))
        set(USE_MASM OFF CACHE BOOL "" FORCE)
        set(USE_GAS ON CACHE BOOL "" FORCE)
    endif()

    find_package(Vulkan QUIET)
    if(NOT Vulkan_FOUND)
        message(STATUS "System Vulkan SDK not found, fetching Vulkan-Headers and Vulkan-Loader via CPM")

        # Vulkan-Headers
        CPMAddPackage(
            NAME Vulkan-Headers
            GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
            GIT_TAG vulkan-sdk-1.4.341.0
            GIT_SHALLOW TRUE
            OPTIONS
                "VULKAN_HEADERS_ENABLE_TESTS OFF"
                "VULKAN_HEADERS_ENABLE_INSTALL OFF"
        )

        # Vulkan-Loader
        CPMAddPackage(
            NAME Vulkan-Loader
            GITHUB_REPOSITORY KhronosGroup/Vulkan-Loader
            GIT_TAG vulkan-sdk-1.4.341.0
            GIT_SHALLOW TRUE
            OPTIONS
                "VulkanHeaders_DIR ${Vulkan-Headers_BINARY_DIR}"
                "BUILD_TESTS OFF"
                "BUILD_WSI_XCB_SUPPORT OFF"
                "BUILD_WSI_XLIB_SUPPORT OFF"
                "BUILD_WSI_WAYLAND_SUPPORT OFF"
        )

        if(Vulkan-Loader_ADDED AND TARGET vulkan)
            if(NOT TARGET Vulkan::Vulkan)
                add_library(Vulkan::Vulkan ALIAS vulkan)
            endif()
        endif()
        set(Vulkan_FOUND TRUE)
        set(Vulkan_INCLUDE_DIRS ${Vulkan-Headers_SOURCE_DIR}/include)
    else()
        message(STATUS "Using system Vulkan: ${Vulkan_LIBRARY}")
    endif()

    # VulkanMemoryAllocator
    add_library(VulkanMemoryAllocator INTERFACE)
    target_include_directories(VulkanMemoryAllocator SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/3rdparty/vma/include)
    add_library(GPUOpen::VulkanMemoryAllocator ALIAS VulkanMemoryAllocator)

    # SPIRV-Headers
    CPMAddPackage(
        NAME SPIRV-Headers
        GITHUB_REPOSITORY KhronosGroup/SPIRV-Headers
        GIT_TAG main
        GIT_SHALLOW TRUE
    )

    # SPIRV-Tools
    CPMAddPackage(
        NAME SPIRV-Tools
        GITHUB_REPOSITORY KhronosGroup/SPIRV-Tools
        GIT_TAG v2026.1
        GIT_SHALLOW TRUE
        OPTIONS
            "SPIRV_SKIP_EXECUTABLES OFF"
            "SPIRV_SKIP_TESTS ON"
            "SPIRV_WERROR OFF"
            "SPIRV_WARN_EVERYTHING OFF"
    )

    # glslang
    CPMAddPackage(
        NAME glslang
        GITHUB_REPOSITORY KhronosGroup/glslang
        GIT_TAG 16.2.0
        GIT_SHALLOW TRUE
        OPTIONS
            "ENABLE_GLSLANG_BINARIES ON"
            "ENABLE_SPVREMAPPER OFF"
            "ENABLE_HLSL ON"
            "ENABLE_OPT ON"
            "BUILD_TESTING OFF"
            "SKIP_GLSLANG_INSTALL ON"
    )

    # ============================================================================
    # MSVC specific
    # ============================================================================
    if(MSVC AND TARGET SPIRV-Tools-opt)
        target_compile_options(SPIRV-Tools-opt PRIVATE /W0 /wd5232)
    endif()
endif()

# ============================================================================
# Find system packages (Qt6, OpenGL)
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS Core Widgets LinguistTools)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.10.0)
    set(QT_NO_PRIVATE_MODULE_WARNING ON)
    find_package(Qt6 COMPONENTS CorePrivate GuiPrivate WidgetsPrivate REQUIRED)
endif()

if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED COMPONENTS EGL)
else()
    find_package(OpenGL REQUIRED)
endif()
message(STATUS "Found OpenGL")
