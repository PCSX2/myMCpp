# SPDX-FileCopyrightText: 2025 SternXD
# SPDX-License-Identifier: GPL-3.0+

# Shader compilation management for myMCpp

if(ENABLE_VULKAN)
    # ============================================================================
    # Define shader paths
    # ============================================================================
    set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources/shaders/Vulkan)
    set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/Vulkan)

    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

    # ============================================================================
    # Use glslangValidator from our built glslang dependency
    # ============================================================================
    set(GLSLANG_VALIDATOR $<TARGET_FILE:glslang-standalone>)

    # Compile vertex shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/icon.vert.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -S vert ${SHADER_SOURCE_DIR}/icon.vert.glsl -o ${SHADER_OUTPUT_DIR}/icon.vert.spv
        DEPENDS ${SHADER_SOURCE_DIR}/icon.vert.glsl glslang-standalone
        COMMENT "Compiling vertex shader: icon.vert.glsl"
        VERBATIM
    )
    
    # Compile fragment shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/icon.frag.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -S frag ${SHADER_SOURCE_DIR}/icon.frag.glsl -o ${SHADER_OUTPUT_DIR}/icon.frag.spv
        DEPENDS ${SHADER_SOURCE_DIR}/icon.frag.glsl glslang-standalone
        COMMENT "Compiling fragment shader: icon.frag.glsl"
        VERBATIM
    )

    # Compile background vertex shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/background.vert.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -S vert ${SHADER_SOURCE_DIR}/background.vert.glsl -o ${SHADER_OUTPUT_DIR}/background.vert.spv
        DEPENDS ${SHADER_SOURCE_DIR}/background.vert.glsl glslang-standalone
        COMMENT "Compiling vertex shader: background.vert.glsl"
        VERBATIM
    )

    # Compile background fragment shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/background.frag.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -S frag ${SHADER_SOURCE_DIR}/background.frag.glsl -o ${SHADER_OUTPUT_DIR}/background.frag.spv
        DEPENDS ${SHADER_SOURCE_DIR}/background.frag.glsl glslang-standalone
        COMMENT "Compiling fragment shader: background.frag.glsl"
        VERBATIM
    )
    
    # Create custom target for shaders
    add_custom_target(CompileShaders ALL
        DEPENDS ${SHADER_OUTPUT_DIR}/icon.vert.spv ${SHADER_OUTPUT_DIR}/icon.frag.spv ${SHADER_OUTPUT_DIR}/background.vert.spv ${SHADER_OUTPUT_DIR}/background.frag.spv
    )

    # ============================================================================
    # Copy compiled shaders to resources directory for embedding
    # ============================================================================
    add_custom_command(TARGET CompileShaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER_OUTPUT_DIR}/icon.vert.spv
            ${CMAKE_SOURCE_DIR}/resources/shaders/Vulkan/icon.vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER_OUTPUT_DIR}/icon.frag.spv
            ${CMAKE_SOURCE_DIR}/resources/shaders/Vulkan/icon.frag.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER_OUTPUT_DIR}/background.vert.spv
            ${CMAKE_SOURCE_DIR}/resources/shaders/Vulkan/background.vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER_OUTPUT_DIR}/background.frag.spv
            ${CMAKE_SOURCE_DIR}/resources/shaders/Vulkan/background.frag.spv
        COMMENT "Copying compiled shaders to resources directory..."
        VERBATIM
    )
endif()
