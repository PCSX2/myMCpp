cmake_minimum_required(VERSION 3.16...3.24)
project(myMCpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(myMCppVersion)

myMCpp_get_git_version_info()
myMCpp_write_svnrev_h()

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3rdparty dependencies
option(ENABLE_VULKAN "Enable Vulkan renderer" ON)

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
endif()

include_directories("${CMAKE_BINARY_DIR}/generated")

add_subdirectory(3rdparty)

# System dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets LinguistTools Svg)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.10.0)
    set(QT_NO_PRIVATE_MODULE_WARNING ON)
    find_package(Qt6 COMPONENTS CorePrivate GuiPrivate WidgetsPrivate REQUIRED)
endif()

if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED COMPONENTS EGL)
else()
    find_package(OpenGL REQUIRED)
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
