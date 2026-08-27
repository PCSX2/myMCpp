set(CLI_SOURCES
    PS2McCommandLine.cpp
)

set(CLI_HEADERS
    PS2McCommandLine.h
)

add_library(myMCpp-cli STATIC ${CLI_SOURCES} ${CLI_HEADERS})

target_include_directories(myMCpp-cli PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(myMCpp-cli PUBLIC
    myMCpp-common
    myMCpp-core
)

if(MSVC)
    target_compile_options(myMCpp-cli PRIVATE /W4)
else()
    target_compile_options(myMCpp-cli PRIVATE -Wall -Wextra -pedantic)
endif()
