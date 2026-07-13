add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(cli)
add_subdirectory(ui)

add_executable(myMCpp WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

if(WIN32)
    target_sources(myMCpp PRIVATE ${CMAKE_SOURCE_DIR}/resources/myMCpp.rc)
endif()

target_link_libraries(myMCpp PRIVATE
    myMCpp-qt
    myMCpp-cli
)

# Took inspiration from https://github.com/PCSX2/pcsx2/blob/master/pcsx2-qt/CMakeLists.txt for the translation generation
file(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ui/Translations/myMCpp_*.ts")

qt_add_lupdate(myMCpp
    SOURCE_TARGETS myMCpp-qt
    TS_FILES ${TS_FILES}
)

if(WIN32)
    qt_add_lrelease(myMCpp TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
    set(QM_OUTPUT_DIR "$<TARGET_FILE_DIR:myMCpp>/translations")
    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${QM_OUTPUT_DIR}")
    foreach(QM_FILE IN LISTS QM_FILES)
        get_filename_component(QM_FILE_NAME ${QM_FILE} NAME)
        add_custom_command(TARGET myMCpp POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${QM_FILE}" "${QM_OUTPUT_DIR}/${QM_FILE_NAME}")
    endforeach()
elseif(APPLE)
    qt_add_lrelease(myMCpp TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
    foreach(QM_FILE IN LISTS QM_FILES)
        target_sources(myMCpp PRIVATE ${QM_FILE})
        set_source_files_properties(${QM_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/translations/)
    endforeach()
else()
    qt_add_lrelease(myMCpp TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
    set(QM_OUTPUT_DIR "$<TARGET_FILE_DIR:myMCpp>/translations")
    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${QM_OUTPUT_DIR}")
    foreach(QM_FILE IN LISTS QM_FILES)
        get_filename_component(QM_FILE_NAME ${QM_FILE} NAME)
        add_custom_command(TARGET myMCpp POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${QM_FILE}" "${QM_OUTPUT_DIR}/${QM_FILE_NAME}")
    endforeach()
endif()

if(ENABLE_VULKAN)
    target_include_directories(myMCpp PRIVATE ${vulkan-headers_SOURCE_DIR}/include)
endif()

set_target_properties(myMCpp PROPERTIES
    AUTORCC ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

if(WIN32)
    set_target_properties(myMCpp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET myMCpp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Running windeployqt to deploy Qt dependencies..."
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-compiler-runtime
            --no-opengl-sw
            "$<TARGET_FILE:myMCpp>"
            VERBATIM
        )
    endif()

    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying zlib DLL to output directory..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:zlib>
        $<TARGET_FILE_DIR:myMCpp>
        VERBATIM
    )

    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying resources folder to output directory..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:myMCpp>/resources
        VERBATIM
    )
endif()

if(UNIX AND NOT APPLE)
    add_custom_command(TARGET myMCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying resources folder to output directory..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:myMCpp>/resources
        VERBATIM
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
    )

    include(MetalShaders)
    myMCpp_bundle_metal(myMCpp)
endif()

if(MSVC)
    target_compile_options(myMCpp PRIVATE /W4)
else()
    target_compile_options(myMCpp PRIVATE -Wall -Wextra -pedantic)
endif()
