cmake_minimum_required(VERSION 3.16)
project(myMCpp VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include dependencies
include(${CMAKE_SOURCE_DIR}/cmake/deps.cmake)

if(ENABLE_VULKAN)
    include(${CMAKE_SOURCE_DIR}/cmake/shaders.cmake)
endif()

# make myMCpp
add_subdirectory(src)

foreach(_t 
    zlib zlibstatic
    spirv-as spirv-cfg spirv-diff spirv-dis spirv-lint spirv-objdump spirv-opt spirv-reduce spirv-val
)
    if(TARGET ${_t})
        set_target_properties(${_t} PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    endif()
endforeach()

# Installation
install(TARGETS myMCpp DESTINATION bin)
install(FILES README.txt DESTINATION .)

# Translations
if(WIN32 OR UNIX)
    install(DIRECTORY src/ui/Translations DESTINATION resources/translations)
endif()
