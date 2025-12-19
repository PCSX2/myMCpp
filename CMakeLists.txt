cmake_minimum_required(VERSION 3.16)
project(myMCpp VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include dependencies
include(${CMAKE_SOURCE_DIR}/cmake/deps.cmake)

# Include shader compilation
include(${CMAKE_SOURCE_DIR}/cmake/shaders.cmake)

# Core library sources
set(CORE_SOURCES
    src/core/formats/ps2mc_dir.cpp
    src/core/formats/ps2mc_ecc.cpp
    src/core/formats/ps2mc.cpp
    src/core/formats/ps2save.cpp
    src/core/formats/ps2iconsys.cpp
    src/core/formats/ps2icon.cpp
    src/cli/ps2mc_cli.cpp
    src/common/sjis.cpp
    src/common/lzari.cpp
    src/common/Logger.cpp
    src/common/WindowInfo.cpp
    src/core/Config.cpp
    src/core/renderer/Renderer.cpp
    src/core/renderer/Common/RendererCommon.cpp
    src/core/renderer/Vulkan/VulkanRenderer.cpp
    src/core/renderer/Vulkan/VulkanDevice.cpp
    src/core/renderer/Vulkan/VulkanSwapchain.cpp
    src/core/renderer/Vulkan/VulkanPipeline.cpp
    src/core/renderer/Vulkan/VulkanResources.cpp
    src/core/renderer/OpenGL/OpenGLRenderer.cpp
    src/core/renderer/OpenGL/OpenGLShader.cpp
    src/core/renderer/OpenGL/OpenGLResources.cpp
    src/core/renderer/OpenGL/GLContext.cpp
    src/core/renderer/OpenGL/GLContextWGL.cpp
    src/core/renderer/OpenGL/GLContextEGL.cpp
)

if(APPLE)
    list(APPEND CORE_SOURCES 
        src/core/renderer/Metal/MetalRenderer.mm
        src/core/renderer/Metal/MetalDevice.mm
        src/core/renderer/Metal/MetalPipeline.mm
        src/core/renderer/Metal/MetalResources.mm
        src/common/CocoaTools.mm
    )
endif()

set(CORE_HEADERS
    src/common/round.h
    src/common/version.h
    src/core/formats/ps2mc_dir.h
    src/core/formats/ps2mc_ecc.h
    src/core/formats/ps2mc.h
    src/core/formats/ps2save.h
    src/core/formats/ps2iconsys.h
    src/core/formats/ps2icon.h
    src/cli/ps2mc_cli.h
    src/common/sjis.h
    src/common/lzari.h
    src/common/Logger.h
    src/common/WindowInfo.h
    src/core/Config.h
    src/core/renderer/Renderer.h
    src/core/renderer/Common/RendererCommon.h
    src/core/renderer/Vulkan/VulkanRenderer.h
    src/core/renderer/Vulkan/VulkanDevice.h
    src/core/renderer/Vulkan/VulkanSwapchain.h
    src/core/renderer/Vulkan/VulkanPipeline.h
    src/core/renderer/Vulkan/VulkanResources.h
    src/core/renderer/OpenGL/OpenGLRenderer.h
    src/core/renderer/OpenGL/OpenGLShader.h
    src/core/renderer/OpenGL/OpenGLResources.h
    src/core/renderer/OpenGL/GLContext.h
    src/core/renderer/OpenGL/GLContextWGL.h
    src/core/renderer/OpenGL/GLContextEGL.h
)

if(APPLE)
    list(APPEND CORE_HEADERS
        src/core/renderer/Metal/MetalRenderer.h
        src/core/renderer/Metal/MetalDevice.h
        src/core/renderer/Metal/MetalPipeline.h
        src/core/renderer/Metal/MetalResources.h
        src/common/CocoaTools.h
    )
endif()

# Create core library
add_library(myMCpp_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(myMCpp_core PUBLIC 
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/formats
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/cli
)

# Link Vulkan headers for renderer in core
target_include_directories(myMCpp_core PUBLIC ${vulkan-headers_SOURCE_DIR}/include)

# Link GLM headers
target_include_directories(myMCpp_core PUBLIC ${glm_SOURCE_DIR})

# Link GLFW headers
target_include_directories(myMCpp_core PUBLIC ${glfw_SOURCE_DIR}/include)

# Link GLAD (OpenGL Loader)
target_include_directories(myMCpp_core PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty/glad/include)
target_sources(myMCpp_core PRIVATE 
    ${CMAKE_SOURCE_DIR}/3rdparty/glad/src/gl.c
)
if(WIN32)
    target_sources(myMCpp_core PRIVATE 
        ${CMAKE_SOURCE_DIR}/3rdparty/glad/src/wgl.c
    )
endif()

# Link nlohmann/json
target_link_libraries(myMCpp_core PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog)

# Link Vulkan and OpenGL
target_link_libraries(myMCpp_core PUBLIC Vulkan::Vulkan OpenGL::GL)

if(UNIX AND NOT APPLE)
    target_link_libraries(myMCpp_core PUBLIC OpenGL::EGL)
    find_package(X11 REQUIRED)
    target_include_directories(myMCpp_core PUBLIC ${X11_INCLUDE_DIR} ${X11_Xrandr_INCLUDE_PATH})
    target_link_libraries(myMCpp_core PUBLIC ${X11_LIBRARIES} ${X11_Xrandr_LIB})
endif()

if(APPLE)
    target_link_libraries(myMCpp_core PUBLIC 
        "-framework Metal"
        "-framework MetalKit"
        "-framework Cocoa"
        "-framework QuartzCore"
    )
endif()

# Enable exception handling for Windows
if(MSVC)
    target_compile_options(myMCpp_core PRIVATE /EHsc)
endif()

# Link zlib
target_link_libraries(myMCpp_core PRIVATE zlib)
target_include_directories(myMCpp_core PRIVATE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})

# Link GLFW
target_link_libraries(myMCpp_core PUBLIC glfw)

# GUI application sources
set(GUI_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/widgets/MemoryCardBrowser.cpp
    src/ui/widgets/SaveDetailsPanel.cpp
    src/ui/CardActionHandler.cpp
    src/ui/widgets/IconWidget.cpp
    src/ui/dialogs/SettingsDialog.cpp
    src/ui/dialogs/NewCardDialog.cpp
    resources/resources.qrc
)

if(WIN32)
    list(APPEND GUI_SOURCES resources/myMCpp.rc)
endif()

set(GUI_HEADERS
    src/ui/MainWindow.h
    src/ui/widgets/MemoryCardBrowser.h
    src/ui/widgets/SaveDetailsPanel.h
    src/ui/CardActionHandler.h
    src/ui/widgets/IconWidget.h
    src/ui/dialogs/SettingsDialog.h
    src/ui/dialogs/NewCardDialog.h
)

# Create GUI executable
add_executable(myMCpp WIN32 ${GUI_SOURCES} ${GUI_HEADERS})

# Ensure shaders are compiled before building the GUI
add_dependencies(myMCpp CompileShaders)

# Link libraries
target_link_libraries(myMCpp PRIVATE 
    myMCpp_core
    Qt6::Core 
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Widgets
)

    find_package(Wayland REQUIRED COMPONENTS Client Egl)
    target_link_libraries(myMCpp_core PUBLIC Wayland::Client Wayland::Egl)

target_include_directories(myMCpp PRIVATE 
    ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/formats
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs
)
target_include_directories(myMCpp PRIVATE ${vulkan-headers_SOURCE_DIR}/include)

foreach(_t 
    zlib zlibstatic
    spirv-as spirv-cfg spirv-diff spirv-dis spirv-lint spirv-objdump spirv-opt spirv-reduce spirv-val
)
    if(TARGET ${_t})
        set_target_properties(${_t} PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    endif()
endforeach()

# Set output directories for all configurations
set_target_properties(myMCpp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

# Installation
install(TARGETS myMCpp DESTINATION bin)
install(FILES README.txt DESTINATION .)

# Platform-specific settings
if(WIN32)
    # Set Windows-specific properties
    set_target_properties(myMCpp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # Deploy Qt libraries on Windows
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    
    if(WINDEPLOYQT_EXECUTABLE)
        # Deploy Qt DLLs
        add_custom_command(TARGET myMCpp POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-compiler-runtime
                "$<TARGET_FILE:myMCpp>"
            COMMENT "Running windeployqt to deploy Qt dependencies..."
        )
    endif()
    
    # Copy zlib DLL to output directory
    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:zlib>
            $<TARGET_FILE_DIR:myMCpp>
        COMMENT "Copying zlib DLL to output directory..."
    )
    
    # Copy resources folder to output directory
    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:myMCpp>/resources
        COMMENT "Copying resources folder to output directory..."
    )
endif()

if(UNIX AND NOT APPLE)
    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:myMCpp>/resources
        COMMENT "Copying resources folder to output directory..."
    )
endif()

# Add compiler warnings
if(MSVC)
    target_compile_options(myMCpp_core PRIVATE /W4)
    target_compile_options(myMCpp PRIVATE /W4)
else()
    target_compile_options(myMCpp_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(myMCpp PRIVATE -Wall -Wextra -pedantic)
endif()
