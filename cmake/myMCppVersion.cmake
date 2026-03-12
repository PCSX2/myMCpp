# based on pcsx2/cmake/Pcsx2Utils.cmake

include_guard(GLOBAL)

find_package(Git QUIET)

function(myMCpp_get_git_version_info)
    set(myMCpp_GIT_REV "")
    set(myMCpp_GIT_TAG "")
    set(myMCpp_GIT_HASH "")
    set(myMCpp_GIT_DATE "")

    if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        execute_process(
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMAND "${GIT_EXECUTABLE}" describe --tags
            OUTPUT_VARIABLE myMCpp_GIT_REV
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        execute_process(
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMAND "${GIT_EXECUTABLE}" tag --points-at HEAD --sort=version:refname
            OUTPUT_VARIABLE myMCpp_GIT_TAG_LIST
            RESULT_VARIABLE TAG_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(myMCpp_GIT_TAG_LIST AND TAG_RESULT EQUAL 0)
            string(REPLACE "\n" ";" myMCpp_GIT_TAG_LIST "${myMCpp_GIT_TAG_LIST}")
            if(myMCpp_GIT_TAG_LIST)
                list(GET myMCpp_GIT_TAG_LIST -1 myMCpp_GIT_TAG)
            endif()
        endif()

        execute_process(
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
            OUTPUT_VARIABLE myMCpp_GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        execute_process(
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMAND "${GIT_EXECUTABLE}" log -1 --format=%cd --date=local
            OUTPUT_VARIABLE myMCpp_GIT_DATE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()

    if(NOT myMCpp_GIT_REV)
        if(GIT_FOUND)
            execute_process(
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
                OUTPUT_VARIABLE myMCpp_GIT_REV
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()
        if(NOT myMCpp_GIT_REV)
            set(myMCpp_GIT_REV "Unknown")
        endif()
    endif()

    set(myMCpp_GIT_REV "${myMCpp_GIT_REV}" PARENT_SCOPE)
    set(myMCpp_GIT_TAG "${myMCpp_GIT_TAG}" PARENT_SCOPE)
    set(myMCpp_GIT_HASH "${myMCpp_GIT_HASH}" PARENT_SCOPE)
    set(myMCpp_GIT_DATE "${myMCpp_GIT_DATE}" PARENT_SCOPE)
endfunction()

function(myMCpp_write_svnrev_h)
    if(NOT DEFINED myMCpp_GIT_REV)
        message(FATAL_ERROR "myMCpp_write_svnrev_h called before myMCpp_get_git_version_info")
    endif()

    set(output_dir "${CMAKE_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY "${output_dir}")

    set(svnrev_path "${output_dir}/svnrev.h")

    set(git_tag "${myMCpp_GIT_TAG}")
    set(git_rev "${myMCpp_GIT_REV}")
    set(git_hash "${myMCpp_GIT_HASH}")
    set(git_date "${myMCpp_GIT_DATE}")

    set(tag_hi 0)
    set(tag_mid 0)
    set(tag_lo 0)
    set(tagged_commit 0)

    if("${git_tag}" MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
        set(tag_hi "${CMAKE_MATCH_1}")
        set(tag_mid "${CMAKE_MATCH_2}")
        set(tag_lo "${CMAKE_MATCH_3}")
        set(tagged_commit 1)
    elseif("${git_rev}" MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(tag_hi "${CMAKE_MATCH_1}")
        set(tag_mid "${CMAKE_MATCH_2}")
        set(tag_lo "${CMAKE_MATCH_3}")
    endif()

    file(WRITE "${svnrev_path}"
        "#define GIT_TAG \"${git_tag}\"\n"
        "#define GIT_TAGGED_COMMIT ${tagged_commit}\n"
        "#define GIT_TAG_HI  ${tag_hi}\n"
        "#define GIT_TAG_MID ${tag_mid}\n"
        "#define GIT_TAG_LO  ${tag_lo}\n"
        "#define GIT_REV \"${git_rev}\"\n"
        "#define GIT_HASH \"${git_hash}\"\n"
        "#define GIT_DATE \"${git_date}\"\n"
    )

    set(myMCpp_SVNREV_HEADER "${svnrev_path}" PARENT_SCOPE)
endfunction()

