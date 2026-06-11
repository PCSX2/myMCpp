set(UI_SOURCES
    QtMain.cpp
    MainWindow.cpp
    DiscordRPCManager.cpp
    Themes.cpp
    CardActionHandler.cpp
    widgets/MemoryCardBrowser.cpp
    widgets/SaveDetailsPanel.cpp
    widgets/IconWidget.cpp
    Settings/SettingsWindow.cpp
    dialogs/NewCardDialog.cpp
    Settings/SettingsWidget.cpp
    dialogs/AboutDialog.cpp
    dialogs/ImportExportSavesDialog.cpp
    Settings/GeneralSettingsWidget.cpp
    Settings/GraphicsSettingsWidget.cpp
    Settings/FilesSettingsWidget.cpp
    Settings/AdvancedSettingsWidget.cpp
    TranslationManager.cpp
)

set(UI_HEADERS
    QtMain.h
    MainWindow.h
    DiscordRPCManager.h
    Themes.h
    CardActionHandler.h
    widgets/MemoryCardBrowser.h
    widgets/SaveDetailsPanel.h
    widgets/IconWidget.h
    Settings/SettingsWindow.h
    dialogs/NewCardDialog.h
    Settings/SettingsWidget.h
    dialogs/AboutDialog.h
    dialogs/ImportExportSavesDialog.h
    Settings/GeneralSettingsWidget.h
    Settings/GraphicsSettingsWidget.h
    Settings/FilesSettingsWidget.h
    Settings/AdvancedSettingsWidget.h
    TranslationManager.h
)

file(GLOB_RECURSE UI_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
)

add_library(myMCpp-qt STATIC ${UI_SOURCES} ${UI_HEADERS} ${UI_FILES})

set_target_properties(myMCpp-qt PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/widgets;${CMAKE_CURRENT_SOURCE_DIR}/dialogs;${CMAKE_CURRENT_SOURCE_DIR}/Settings"
)

target_include_directories(myMCpp-qt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings
    ${CMAKE_CURRENT_BINARY_DIR}
)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    target_include_directories(myMCpp-qt PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/myMCpp-qt_autogen/include_$<CONFIG>)
else()
    target_include_directories(myMCpp-qt PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/myMCpp-qt_autogen/include)
endif()

target_link_libraries(myMCpp-qt PUBLIC
    myMCpp-common
    myMCpp-core
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Widgets
)

if(TARGET discord-rpc)
    target_link_libraries(myMCpp-qt PUBLIC discord-rpc)
endif()

if(MSVC)
    target_compile_options(myMCpp-qt PRIVATE /W4)
else()
    target_compile_options(myMCpp-qt PRIVATE -Wall -Wextra -pedantic)
endif()
