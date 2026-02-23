set(UI_SOURCES
    MainWindow.cpp
    Themes.cpp
    CardActionHandler.cpp
    widgets/MemoryCardBrowser.cpp
    widgets/SaveDetailsPanel.cpp
    widgets/IconWidget.cpp
    Settings/SettingsWindow.cpp
    dialogs/NewCardDialog.cpp
    Settings/SettingsWidget.cpp
    dialogs/AboutDialog.cpp
    Settings/InterfaceSettingsWidget.cpp
    Settings/BehaviorSettingsWidget.cpp
    Settings/FilesSettingsWidget.cpp
    Settings/AdvancedSettingsWidget.cpp
    TranslationManager.cpp
)

set(UI_HEADERS
    MainWindow.h
    Themes.h
    CardActionHandler.h
    widgets/MemoryCardBrowser.h
    widgets/SaveDetailsPanel.h
    widgets/IconWidget.h
    Settings/SettingsWindow.h
    dialogs/NewCardDialog.h
    Settings/SettingsWidget.h
    dialogs/AboutDialog.h
    Settings/InterfaceSettingsWidget.h
    Settings/BehaviorSettingsWidget.h
    Settings/FilesSettingsWidget.h
    Settings/AdvancedSettingsWidget.h
    TranslationManager.h
)

file(GLOB_RECURSE UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
)

add_library(myMCpp_ui STATIC ${UI_SOURCES} ${UI_HEADERS} ${UI_FILES})

set_target_properties(myMCpp_ui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/widgets;${CMAKE_CURRENT_SOURCE_DIR}/dialogs;${CMAKE_CURRENT_SOURCE_DIR}/Settings"
)

target_include_directories(myMCpp_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings
    ${CMAKE_CURRENT_BINARY_DIR}
)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    target_include_directories(myMCpp_ui PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/myMCpp_ui_autogen/include_$<CONFIG>)
else()
    target_include_directories(myMCpp_ui PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/myMCpp_ui_autogen/include)
endif()

target_link_libraries(myMCpp_ui PUBLIC
    myMCpp_common
    myMCpp_core
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Widgets
)

if(MSVC)
    target_compile_options(myMCpp_ui PRIVATE /W4)
else()
    target_compile_options(myMCpp_ui PRIVATE -Wall -Wextra -pedantic)
endif()
