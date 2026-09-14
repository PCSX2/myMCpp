# based on pcsx2/cmake/Pcsx2Utils.cmake

include_guard(GLOBAL)

find_package(Git QUIET)

function(myMCpp_get_git_version_info)
	set(myMCpp_GIT_REV "")
	set(myMCpp_GIT_TAG "")
	set(myMCpp_GIT_HASH "")
	set(myMCpp_GIT_DATE "")

	if(GIT_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.git)
		execute_process(
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
			COMMAND ${GIT_EXECUTABLE} describe --tags
			OUTPUT_VARIABLE myMCpp_GIT_REV
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
		)

		execute_process(
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
			COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD --sort=version:refname
			OUTPUT_VARIABLE myMCpp_GIT_TAG_LIST
			RESULT_VARIABLE TAG_RESULT
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
		)

		if(myMCpp_GIT_TAG_LIST AND TAG_RESULT EQUAL 0)
			string(REPLACE "\n" ";" myMCpp_GIT_TAG_LIST "${myMCpp_GIT_TAG_LIST}")
			if(myMCpp_GIT_TAG_LIST)
				list(GET myMCpp_GIT_TAG_LIST -1 myMCpp_GIT_TAG)
				message(STATUS "Using tag: ${myMCpp_GIT_TAG}")
			endif()
		endif()

		execute_process(
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
			COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
			OUTPUT_VARIABLE myMCpp_GIT_HASH
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
		)

		execute_process(
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
			COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=local
			OUTPUT_VARIABLE myMCpp_GIT_DATE
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
		)
	endif()

	if(NOT myMCpp_GIT_REV)
		if(GIT_FOUND)
			execute_process(
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
				COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
				OUTPUT_VARIABLE myMCpp_GIT_REV
				OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
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
	set(output_dir "${CMAKE_BINARY_DIR}/generated")
	file(MAKE_DIRECTORY "${output_dir}")
	set(svnrev_path "${output_dir}/svnrev.h")

	if("${myMCpp_GIT_TAG}" MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
		file(WRITE "${svnrev_path}"
			"#define GIT_TAG \"${myMCpp_GIT_TAG}\"\n"
			"#define GIT_TAGGED_COMMIT 1\n"
			"#define GIT_TAG_HI  ${CMAKE_MATCH_1}\n"
			"#define GIT_TAG_MID ${CMAKE_MATCH_2}\n"
			"#define GIT_TAG_LO  ${CMAKE_MATCH_3}\n"
			"#define GIT_REV \"${myMCpp_GIT_TAG}\"\n"
			"#define GIT_HASH \"${myMCpp_GIT_HASH}\"\n"
			"#define GIT_DATE \"${myMCpp_GIT_DATE}\"\n"
		)
		set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1} PARENT_SCOPE)
		set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2} PARENT_SCOPE)
		set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3} PARENT_SCOPE)
		set(
			PROJECT_VERSION
			"${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" PARENT_SCOPE
		)
	elseif("${myMCpp_GIT_REV}" MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)")
		file(WRITE "${svnrev_path}"
			"#define GIT_TAG \"${myMCpp_GIT_TAG}\"\n"
			"#define GIT_TAGGED_COMMIT 0\n"
			"#define GIT_TAG_HI  ${CMAKE_MATCH_1}\n"
			"#define GIT_TAG_MID ${CMAKE_MATCH_2}\n"
			"#define GIT_TAG_LO  ${CMAKE_MATCH_3}\n"
			"#define GIT_REV \"${myMCpp_GIT_REV}\"\n"
			"#define GIT_HASH \"${myMCpp_GIT_HASH}\"\n"
			"#define GIT_DATE \"${myMCpp_GIT_DATE}\"\n"
		)
		set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1} PARENT_SCOPE)
		set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2} PARENT_SCOPE)
		set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3} PARENT_SCOPE)
		set(
			PROJECT_VERSION
			"${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" PARENT_SCOPE
		)
	else()
		file(WRITE "${svnrev_path}"
			"#define GIT_TAG \"${myMCpp_GIT_TAG}\"\n"
			"#define GIT_TAGGED_COMMIT 0\n"
			"#define GIT_TAG_HI 0\n"
			"#define GIT_TAG_MID 0\n"
			"#define GIT_TAG_LO 0\n"
			"#define GIT_REV \"${myMCpp_GIT_REV}\"\n"
			"#define GIT_HASH \"${myMCpp_GIT_HASH}\"\n"
			"#define GIT_DATE \"${myMCpp_GIT_DATE}\"\n"
		)
		set(PROJECT_VERSION_MAJOR 0 PARENT_SCOPE)
		set(PROJECT_VERSION_MINOR 0 PARENT_SCOPE)
		set(PROJECT_VERSION_PATCH 0 PARENT_SCOPE)
		set(PROJECT_VERSION "0.0.0" PARENT_SCOPE)
	endif()

	set(myMCpp_SVNREV_HEADER "${svnrev_path}" PARENT_SCOPE)
endfunction()
