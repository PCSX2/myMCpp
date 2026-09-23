add_library(SimpleIni INTERFACE)
add_library(SimpleIni::SimpleIni ALIAS SimpleIni)
target_include_directories(SimpleIni INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
