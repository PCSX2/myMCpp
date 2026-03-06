cmake_minimum_required(VERSION 3.16...3.24)
project(myMCpp VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3rdparty dependencies
option(ENABLE_VULKAN "Enable Vulkan renderer" ON)

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
endif()

add_subdirectory(3rdparty)

# System dependencies
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
if(ENABLE_VULKAN)
    include(${CMAKE_SOURCE_DIR}/cmake/shaders.cmake)
endif()

# make myMCpp
add_subdirectory(src)

# Installation
install(TARGETS myMCpp DESTINATION bin)
install(FILES README.txt DESTINATION .)

# Translations
if(WIN32 OR UNIX)
    install(DIRECTORY src/ui/Translations DESTINATION resources/translations)
endif()
