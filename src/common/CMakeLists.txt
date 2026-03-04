set(COMMON_SOURCES
    sjis.cpp
    lzari.cpp
    Logger.cpp
    WindowInfo.cpp
    Config.cpp
)

set(COMMON_HEADERS
    round.h
    version.h
    sjis.h
    lzari.h
    Logger.h
    WindowInfo.h
    Config.h
)

if(APPLE)
    list(APPEND COMMON_SOURCES CocoaTools.mm)
    list(APPEND COMMON_HEADERS CocoaTools.h)
endif()

add_library(myMCpp-common STATIC ${COMMON_SOURCES} ${COMMON_HEADERS})

target_include_directories(myMCpp-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(myMCpp-common PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    utf8proc
)

if(APPLE)
    find_library(ICONV_LIBRARY iconv)
    target_link_libraries(myMCpp-common PUBLIC ${ICONV_LIBRARY})
    
    set_source_files_properties(
        CocoaTools.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_include_directories(myMCpp-common PUBLIC ${X11_INCLUDE_DIR} ${X11_Xrandr_INCLUDE_PATH})
    target_link_libraries(myMCpp-common PUBLIC ${X11_LIBRARIES} ${X11_Xrandr_LIB})
endif()

if(MSVC)
    target_compile_options(myMCpp-common PRIVATE /W4)
else()
    target_compile_options(myMCpp-common PRIVATE -Wall -Wextra -pedantic)
endif()
