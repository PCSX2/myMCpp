set(METAL_SHADER_DIR ${CMAKE_SOURCE_DIR}/resources/shaders/Metal)
set(METAL_SHARED_SHADERS
    ${METAL_SHADER_DIR}/IconShared.metal
    ${METAL_SHADER_DIR}/BackgroundShared.metal
)
set(METAL_ENTRY_SHADERS
    ${METAL_SHADER_DIR}/background_ps.metal
    ${METAL_SHADER_DIR}/background_vs.metal
    ${METAL_SHADER_DIR}/icon_ps.metal
    ${METAL_SHADER_DIR}/icon_vs.metal
)

function(myMCpp_compile_metal core_target)
    if(NOT APPLE)
        return()
    endif()

    set_property(GLOBAL PROPERTY METAL_ENTRY_SHADERS ${METAL_ENTRY_SHADERS})
    set_property(GLOBAL PROPERTY METAL_SHARED_SHADERS ${METAL_SHARED_SHADERS})

    if(CMAKE_GENERATOR MATCHES "Xcode")
        return()
    endif()

    if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER_EQUAL 26.0)
        set(metal_std metal4.0)
    else()
        set(metal_std metal3.0)
    endif()

    set(metallib ${CMAKE_BINARY_DIR}/default.metallib)
    set(air_dir ${CMAKE_BINARY_DIR}/Metal)
    set(air_files)
    set(shaders ${METAL_ENTRY_SHADERS})

    foreach(shader IN LISTS shaders)
        get_filename_component(shader_name ${shader} NAME_WE)
        set(air_file ${air_dir}/${shader_name}.air)
        list(APPEND air_files ${air_file})

        add_custom_command(
            OUTPUT ${air_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${air_dir}
            COMMAND xcrun metal -ffast-math -std=${metal_std}
            -target air64-apple-macos${CMAKE_OSX_DEPLOYMENT_TARGET}
            $<$<NOT:$<CONFIG:Release,MinSizeRel>>:-gline-tables-only>
            -I ${METAL_SHADER_DIR} -o ${air_file} -c ${shader}
            DEPENDS ${shader} ${METAL_SHARED_SHADERS}
            COMMENT "Compiling Metal shader ${shader_name}"
        )
    endforeach()

    add_custom_command(
        OUTPUT ${metallib}
        COMMAND xcrun metallib -o ${metallib} ${air_files}
        DEPENDS ${air_files}
        COMMENT "Linking Metal library"
    )

    add_custom_target(CompileMetalShaders DEPENDS ${metallib})
    add_dependencies(${core_target} CompileMetalShaders)
endfunction()

function(myMCpp_bundle_metal app_target)
    if(NOT APPLE)
        return()
    endif()

    if(CMAKE_GENERATOR MATCHES "Xcode")
        get_property(entry_shaders GLOBAL PROPERTY METAL_ENTRY_SHADERS)
        get_property(shared_shaders GLOBAL PROPERTY METAL_SHARED_SHADERS)
        set_target_properties(${app_target} PROPERTIES
            XCODE_ATTRIBUTE_MTL_ENABLE_DEBUG_INFO INCLUDE_SOURCE
            XCODE_ATTRIBUTE_MTL_HEADER_SEARCH_PATHS ${METAL_SHADER_DIR}
        )
        foreach(shader IN LISTS entry_shaders)
            target_sources(${app_target} PRIVATE ${shader})
            set_source_files_properties(${shader} PROPERTIES LANGUAGE METAL)
        endforeach()
        foreach(shader IN LISTS shared_shaders)
            target_sources(${app_target} PRIVATE ${shader})
            set_source_files_properties(${shader} PROPERTIES HEADER_FILE_ONLY TRUE)
        endforeach()
        return()
    endif()

    if(NOT TARGET CompileMetalShaders)
        return()
    endif()

    add_dependencies(${app_target} CompileMetalShaders)
    add_custom_command(TARGET ${app_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/default.metallib
        $<TARGET_FILE_DIR:${app_target}>/../Resources/shaders/Metal/default.metallib
    )
endfunction()
