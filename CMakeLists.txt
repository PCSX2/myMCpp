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

if(ENABLE_VULKAN)
    include(${CMAKE_SOURCE_DIR}/cmake/shaders.cmake)
endif()

# Core library sources
set(CORE_SOURCES
    src/common/sjis.cpp
    src/common/lzari.cpp
    src/common/Logger.cpp
    src/common/TranslationManager.cpp
    src/common/WindowInfo.cpp
    src/core/formats/ps2mc_dir.cpp
    src/core/formats/ps2mc_ecc.cpp
    src/core/formats/ps2mc.cpp
    src/core/formats/ps2save.cpp
    src/core/formats/ps2iconsys.cpp
    src/core/formats/ps2icon.cpp
    src/cli/ps2mc_cli.cpp
    src/core/Config.cpp
    src/core/renderer/Renderer.cpp
    src/core/renderer/Common/RendererCommon.cpp
    src/core/renderer/OpenGL/OpenGLRenderer.cpp
    src/core/renderer/OpenGL/OpenGLShader.cpp
    src/core/renderer/OpenGL/OpenGLResources.cpp
    src/core/renderer/OpenGL/GLContext.cpp
    src/core/renderer/OpenGL/GLContextWGL.cpp
    src/core/renderer/OpenGL/GLContextEGL.cpp
)

if(ENABLE_VULKAN)
    list(APPEND CORE_SOURCES
        src/core/renderer/Vulkan/VulkanRenderer.cpp
        src/core/renderer/Vulkan/VulkanDevice.cpp
        src/core/renderer/Vulkan/VulkanSwapchain.cpp
        src/core/renderer/Vulkan/VulkanPipeline.cpp
        src/core/renderer/Vulkan/VulkanResources.cpp
    )
endif()

if(APPLE)
    list(APPEND CORE_SOURCES 
        src/core/renderer/Metal/MetalRenderer.mm
        src/core/renderer/Metal/MetalDevice.mm
        src/core/renderer/Metal/MetalPipeline.mm
        src/core/renderer/Metal/MetalResources.mm
        src/core/renderer/OpenGL/GLContextAGL.mm
        src/common/CocoaTools.mm
    )
endif()

set(CORE_HEADERS
    src/common/round.h
    src/common/version.h
    src/common/sjis.h
    src/common/lzari.h
    src/common/Logger.h
    src/common/TranslationManager.h
    src/common/WindowInfo.h
    src/core/formats/ps2mc_dir.h
    src/core/formats/ps2mc_ecc.h
    src/core/formats/ps2mc.h
    src/core/formats/ps2save.h
    src/core/formats/ps2iconsys.h
    src/core/formats/ps2icon.h
    src/cli/ps2mc_cli.h
    src/core/Config.h
    src/core/renderer/Renderer.h
    src/core/renderer/Common/RendererCommon.h
    src/core/renderer/OpenGL/OpenGLRenderer.h
    src/core/renderer/OpenGL/OpenGLShader.h
    src/core/renderer/OpenGL/OpenGLResources.h
    src/core/renderer/OpenGL/GLContext.h
    src/core/renderer/OpenGL/GLContextWGL.h
    src/core/renderer/OpenGL/GLContextEGL.h
)

if(ENABLE_VULKAN)
    list(APPEND CORE_HEADERS
        src/core/renderer/Vulkan/VulkanRenderer.h
        src/core/renderer/Vulkan/VulkanDevice.h
        src/core/renderer/Vulkan/VulkanSwapchain.h
        src/core/renderer/Vulkan/VulkanPipeline.h
        src/core/renderer/Vulkan/VulkanResources.h
    )
endif()

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
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/formats
    ${CMAKE_SOURCE_DIR}/src/cli
)

# Link Vulkan headers for renderer in core
if(ENABLE_VULKAN)
    target_include_directories(myMCpp_core PUBLIC ${vulkan-headers_SOURCE_DIR}/include)
endif()

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

# Link nlohmann/json and Qt
target_link_libraries(myMCpp_core PUBLIC 
    nlohmann_json::nlohmann_json 
    spdlog::spdlog
    Qt6::Core
    Qt6::Widgets
)

# Link Vulkan and OpenGL
if(ENABLE_VULKAN)
    target_link_libraries(myMCpp_core PUBLIC Vulkan::Vulkan)
endif()
target_link_libraries(myMCpp_core PUBLIC OpenGL::GL)

if(UNIX AND NOT APPLE)
    target_link_libraries(myMCpp_core PUBLIC OpenGL::EGL)
    find_package(X11 REQUIRED)
    target_include_directories(myMCpp_core PUBLIC ${X11_INCLUDE_DIR} ${X11_Xrandr_INCLUDE_PATH})
    target_link_libraries(myMCpp_core PUBLIC ${X11_LIBRARIES} ${X11_Xrandr_LIB})
endif()

if(APPLE)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(ICONV_LIBRARY iconv)
    target_link_libraries(myMCpp_core PUBLIC 
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${COCOA_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${ICONV_LIBRARY}
    )
    # Enable ARC for Objective-C++ files
    set_source_files_properties(
        src/core/renderer/Metal/MetalRenderer.mm
        src/core/renderer/Metal/MetalDevice.mm
        src/core/renderer/Metal/MetalPipeline.mm
        src/core/renderer/Metal/MetalResources.mm
        src/core/renderer/OpenGL/GLContextAGL.mm
        src/common/CocoaTools.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    
    # Metal shaders
    set(METAL_SHADERS
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/IconShared.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/background_ps.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/background_vs.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/icon_ps.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/icon_vs.metal
    )
    
    if(CMAKE_GENERATOR MATCHES "Xcode")
        # Xcode handles Metal shaders natively with code completion
        set_target_properties(myMCpp_core PROPERTIES
            XCODE_ATTRIBUTE_MTL_ENABLE_DEBUG_INFO INCLUDE_SOURCE
        )
        foreach(shader IN LISTS METAL_SHADERS)
            target_sources(myMCpp_core PRIVATE ${shader})
            set_source_files_properties(${shader} PROPERTIES LANGUAGE METAL)
        endforeach()
    else()
        # For non-Xcode generators (Ninja, Make), compile Metal shaders manually
        set(METAL_AIR_FILES)
        set(METALLIB_OUTPUT ${CMAKE_BINARY_DIR}/default.metallib)
        
        foreach(shader IN LISTS METAL_SHADERS)
            get_filename_component(shader_name ${shader} NAME_WE)
            set(air_file ${CMAKE_BINARY_DIR}/Metal/${shader_name}.air)
            list(APPEND METAL_AIR_FILES ${air_file})
            
            add_custom_command(
                OUTPUT ${air_file}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Metal
                COMMAND xcrun metal -ffast-math -std=macos-metal2.0 -target air64-apple-macos10.13
                    $<$<NOT:$<CONFIG:Release,MinSizeRel>>:-gline-tables-only>
                    -I ${CMAKE_SOURCE_DIR}/resources/shaders/Metal
                    -o ${air_file} -c ${shader}
                DEPENDS ${shader} ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/IconShared.metal
                COMMENT "Compiling Metal shader ${shader_name}"
            )
        endforeach()
        
        add_custom_command(
            OUTPUT ${METALLIB_OUTPUT}
            COMMAND xcrun metallib -o ${METALLIB_OUTPUT} ${METAL_AIR_FILES}
            DEPENDS ${METAL_AIR_FILES}
            COMMENT "Linking Metal library"
        )
        
        add_custom_target(CompileMetalShaders DEPENDS ${METALLIB_OUTPUT})
        add_dependencies(myMCpp_core CompileMetalShaders)
    endif()
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
    src/ui/Themes.cpp
    src/ui/widgets/MemoryCardBrowser.cpp
    src/ui/widgets/SaveDetailsPanel.cpp
    src/ui/CardActionHandler.cpp
    src/ui/widgets/IconWidget.cpp
    src/ui/Settings/SettingsWindow.cpp
    src/ui/dialogs/NewCardDialog.cpp
    src/ui/Settings/SettingsWidget.cpp
    src/ui/dialogs/AboutDialog.cpp
    src/ui/Settings/InterfaceSettingsWidget.cpp
    src/ui/Settings/BehaviorSettingsWidget.cpp
    src/ui/Settings/FilesSettingsWidget.cpp
    src/ui/Settings/AdvancedSettingsWidget.cpp
    resources/resources.qrc
)

if(WIN32)
    list(APPEND GUI_SOURCES resources/myMCpp.rc)
endif()

set(GUI_HEADERS
    src/ui/MainWindow.h
    src/ui/Themes.h
    src/ui/widgets/MemoryCardBrowser.h
    src/ui/widgets/SaveDetailsPanel.h
    src/ui/CardActionHandler.h
    src/ui/widgets/IconWidget.h
    src/ui/Settings/SettingsWindow.h
    src/ui/dialogs/NewCardDialog.h
    src/ui/Settings/SettingsWidget.h
    src/ui/dialogs/AboutDialog.h
    src/ui/Settings/InterfaceSettingsWidget.h
    src/ui/Settings/BehaviorSettingsWidget.h
    src/ui/Settings/FilesSettingsWidget.h
    src/ui/Settings/AdvancedSettingsWidget.h
)

# Create GUI executable
add_executable(myMCpp WIN32 ${GUI_SOURCES} ${GUI_HEADERS})

if(ENABLE_VULKAN)
    add_dependencies(myMCpp CompileShaders)
endif()

# Link libraries
target_link_libraries(myMCpp PRIVATE 
    myMCpp_core
    Qt6::Core 
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Widgets
)

qt6_add_translations(myMCpp
    TS_FILES src/ui/Translations/myMCpp_en.ts
    LUPDATE_OPTIONS -locations relative
    RESOURCE_PREFIX "/translations"
)

if(UNIX AND NOT APPLE)
    find_package(Wayland REQUIRED COMPONENTS Client Egl)
    target_link_libraries(myMCpp_core PUBLIC Wayland::Client Wayland::Egl)
endif()

target_include_directories(myMCpp PRIVATE 
    ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/formats
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/cli
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_SOURCE_DIR}/src/ui/Settings

)
if(ENABLE_VULKAN)
    target_include_directories(myMCpp PRIVATE ${vulkan-headers_SOURCE_DIR}/include)
endif()

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

# Translations
if(WIN32 OR UNIX)
    install(DIRECTORY src/ui/Translations DESTINATION resources/translations)
endif()

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

if(APPLE)
    set_target_properties(myMCpp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
        OUTPUT_NAME myMCpp
    )
    
    set(MACOS_ICON "${CMAKE_SOURCE_DIR}/resources/icons/AppIcon.icns")
    target_sources(myMCpp PRIVATE ${MACOS_ICON})
    set_source_files_properties(${MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/icons)
    
    get_target_property(_qt_core_loc Qt6::Core LOCATION)
    get_filename_component(_qt_framework_dir "${_qt_core_loc}" DIRECTORY)
    get_filename_component(_qt_lib_dir "${_qt_framework_dir}/../../.." ABSOLUTE)
    
    set_target_properties(myMCpp PROPERTIES
        BUILD_RPATH "${_qt_lib_dir};/opt/homebrew/lib"
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH FALSE
    )
    
    if(NOT CMAKE_GENERATOR MATCHES "Xcode" AND TARGET CompileMetalShaders)
        add_dependencies(myMCpp CompileMetalShaders)
        add_custom_command(TARGET myMCpp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_BINARY_DIR}/default.metallib
                $<TARGET_FILE_DIR:myMCpp>/../Resources/shaders/Metal/default.metallib
            COMMENT "Copying Metal library to bundle..."
        )
    endif()
    
    option(SKIP_POSTPROCESS_BUNDLE "Skip running macdeployqt" OFF)
    if(NOT SKIP_POSTPROCESS_BUNDLE)
        get_target_property(QT_QMAKE_LOCATION Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR "${QT_QMAKE_LOCATION}" DIRECTORY)
        get_filename_component(QT_LIB_DIR "${QT_BIN_DIR}/../lib" ABSOLUTE)
        
        add_custom_target(myMCpp-postprocess-bundle ALL
            COMMAND Qt6::macdeployqt "$<TARGET_FILE_DIR:myMCpp>/../.." -no-strip -always-overwrite -libpath=${QT_LIB_DIR}
            COMMAND codesign --force --deep --sign - "$<TARGET_FILE_DIR:myMCpp>/../.."
            COMMENT "Running macdeployqt and signing bundle..."
        )
        add_dependencies(myMCpp-postprocess-bundle myMCpp)
    endif()
    
    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:myMCpp>/../Resources
        COMMENT "Copying resources to app bundle..."
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
