set(CLI_SOURCES
    ps2mc_cli.cpp
)

set(CLI_HEADERS
    ps2mc_cli.h
)

add_library(myMCpp-cli STATIC ${CLI_SOURCES} ${CLI_HEADERS})

target_include_directories(myMCpp-cli PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
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
