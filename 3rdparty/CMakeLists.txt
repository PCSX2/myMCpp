cmake_minimum_required(VERSION 3.16...3.24)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(myMCpp_3rdparty LANGUAGES C CXX)
endif()

# zlib
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zconf.h"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zconf.h.in")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zconf.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zconf.h"
        COPYONLY)
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_BUILD_EXAMPLES ON CACHE BOOL "" FORCE)
add_subdirectory(zlib)

if(TARGET zlib)
    set_target_properties(zlib PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET example)
    set_target_properties(example PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET minigzip)
    set_target_properties(minigzip PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET minigzip64)
    set_target_properties(minigzip64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET infcover)
    set_target_properties(infcover PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# glm
add_subdirectory(glm)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# nlohmann_json
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(nlohmann_json)

# spdlog
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog)

# utf8proc
set(UTF8PROC_INSTALL OFF CACHE BOOL "" FORCE)
set(UTF8PROC_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(utf8proc)

# discord-rpc
add_subdirectory(discord-rpc)

if(ENABLE_VULKAN)
    # Vulkan-Headers
    set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(Vulkan-Headers)

    # Vulkan-Loader
    if(NOT APPLE)
        set(VulkanHeaders_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers CACHE PATH "" FORCE)
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        if(WIN32)
            set(BUILD_WSI_XCB_SUPPORT OFF CACHE BOOL "" FORCE)
            set(BUILD_WSI_XLIB_SUPPORT OFF CACHE BOOL "" FORCE)
            set(BUILD_WSI_WAYLAND_SUPPORT OFF CACHE BOOL "" FORCE)
        endif()
        
        if(WIN32 AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "Clang"))
            set(USE_MASM OFF CACHE BOOL "" FORCE)
            set(USE_GAS OFF CACHE BOOL "" FORCE)
            set(UNKNOWN_FUNCTIONS_SUPPORTED OFF CACHE BOOL "" FORCE)
        endif()
        set(VulkanHeaders_VERSION "1.4.356" CACHE STRING "" FORCE)
        add_subdirectory(Vulkan-Loader)

        if(TARGET vulkan)
            if(NOT TARGET Vulkan::Vulkan)
                add_library(Vulkan::Vulkan ALIAS vulkan)
            endif()
        endif()
    endif()
    
    set(Vulkan_FOUND TRUE)
    set(Vulkan_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers/include)

    # VulkanMemoryAllocator
    add_library(VulkanMemoryAllocator INTERFACE)
    target_include_directories(VulkanMemoryAllocator SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vma/include)
    add_library(GPUOpen::VulkanMemoryAllocator ALIAS VulkanMemoryAllocator)

    # SPIRV-Headers
    add_subdirectory(SPIRV-Headers)

    # SPIRV-Tools
    set(SPIRV_SKIP_EXECUTABLES OFF CACHE BOOL "" FORCE)
    set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
    set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
    set(SPIRV_WARN_EVERYTHING OFF CACHE BOOL "" FORCE)
    add_subdirectory(SPIRV-Tools)

    if(MSVC AND TARGET SPIRV-Tools-opt)
        target_compile_options(SPIRV-Tools-opt PRIVATE /W0 /wd5232)
    endif()

    # glslang
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
    set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
    set(ENABLE_HLSL ON CACHE BOOL "" FORCE)
    set(ENABLE_OPT ON CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
    add_subdirectory(glslang)

    # MoltenVK
    if(APPLE)
        set(MOLTENVK_PREBUILT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK")
        set(MOLTENVK_DYLIB "${MOLTENVK_PREBUILT_DIR}/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib")
        
        if(EXISTS "${MOLTENVK_DYLIB}")
            add_library(MoltenVK::MoltenVK SHARED IMPORTED GLOBAL)
            set_target_properties(MoltenVK::MoltenVK PROPERTIES
                IMPORTED_LOCATION "${MOLTENVK_DYLIB}"
                INTERFACE_INCLUDE_DIRECTORIES "${MOLTENVK_PREBUILT_DIR}/MoltenVK/include"
            )
            if(NOT TARGET Vulkan::Vulkan)
                add_library(Vulkan::Vulkan ALIAS MoltenVK::MoltenVK)
            endif()
            message(STATUS "Using pre-built MoltenVK from ${MOLTENVK_DYLIB}")
        else()
            message(FATAL_ERROR "MoltenVK dylib not found at ${MOLTENVK_DYLIB}")
        endif()
    endif()
endif()
