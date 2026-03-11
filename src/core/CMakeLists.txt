set(CORE_SOURCES
    formats/ps2mc_dir.cpp
    formats/ps2mc_ecc.cpp
    formats/ps2mc.cpp
    formats/ps2save.cpp
    formats/ps2iconsys.cpp
    formats/ps2icon.cpp
    renderer/Renderer.cpp
    renderer/Common/RendererCommon.cpp
    renderer/OpenGL/OpenGLRenderer.cpp
    renderer/OpenGL/OpenGLShader.cpp
    renderer/OpenGL/OpenGLResources.cpp
    renderer/OpenGL/GLContext.cpp
    renderer/OpenGL/GLContextWGL.cpp
    renderer/OpenGL/GLContextEGL.cpp
)

set(CORE_HEADERS
    formats/ps2mc_dir.h
    formats/ps2mc_ecc.h
    formats/ps2mc.h
    formats/ps2save.h
    formats/ps2iconsys.h
    formats/ps2icon.h
    renderer/Renderer.h
    renderer/Common/RendererCommon.h
    renderer/OpenGL/OpenGLRenderer.h
    renderer/OpenGL/OpenGLShader.h
    renderer/OpenGL/OpenGLResources.h
    renderer/OpenGL/GLContext.h
    renderer/OpenGL/GLContextWGL.h
    renderer/OpenGL/GLContextEGL.h
)

if(ENABLE_VULKAN)
    list(APPEND CORE_SOURCES
        renderer/Vulkan/VulkanRenderer.cpp
        renderer/Vulkan/VulkanDevice.cpp
        renderer/Vulkan/VulkanSwapchain.cpp
        renderer/Vulkan/VulkanPipeline.cpp
        renderer/Vulkan/VulkanResources.cpp
    )
    list(APPEND CORE_HEADERS
        renderer/Vulkan/VulkanRenderer.h
        renderer/Vulkan/VulkanDevice.h
        renderer/Vulkan/VulkanSwapchain.h
        renderer/Vulkan/VulkanPipeline.h
        renderer/Vulkan/VulkanResources.h
    )
endif()

if(APPLE)
    list(APPEND CORE_SOURCES
        renderer/Metal/MetalRenderer.mm
        renderer/Metal/MetalDevice.mm
        renderer/Metal/MetalPipeline.mm
        renderer/Metal/MetalResources.mm
        renderer/OpenGL/GLContextAGL.mm
    )
    list(APPEND CORE_HEADERS
        renderer/Metal/MetalRenderer.h
        renderer/Metal/MetalDevice.h
        renderer/Metal/MetalPipeline.h
        renderer/Metal/MetalResources.h
        renderer/OpenGL/GLContextAGL.h
    )
endif()

add_library(myMCpp-core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

if(ENABLE_VULKAN)
    target_compile_definitions(myMCpp-core PUBLIC ENABLE_VULKAN)
endif()

target_include_directories(myMCpp-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/formats
)

target_link_libraries(myMCpp-core PUBLIC
    myMCpp-common
    Qt6::Core
    Qt6::Widgets
    nlohmann_json::nlohmann_json
    OpenGL::GL
    glfw
)

target_include_directories(myMCpp-core PUBLIC
    ${glm_SOURCE_DIR}
    ${glfw_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty/glad/include
    ${CMAKE_SOURCE_DIR}/3rdparty/glslang
)

target_sources(myMCpp-core PRIVATE
    ${CMAKE_SOURCE_DIR}/3rdparty/glad/src/gl.c
)

if(WIN32)
    target_sources(myMCpp-core PRIVATE
        ${CMAKE_SOURCE_DIR}/3rdparty/glad/src/wgl.c
    )
endif()

if(ENABLE_VULKAN)
    target_link_libraries(myMCpp-core PUBLIC
        Vulkan::Headers
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
    )

    target_link_libraries(myMCpp-core PRIVATE
        glslang
        SPIRV
        MachineIndependent
        GenericCodeGen
        OSDependent
        glslang-default-resource-limits
    )

    if(TARGET SPIRV-Tools)
        target_link_libraries(myMCpp-core PRIVATE SPIRV-Tools)
    endif()
    if(TARGET SPIRV-Tools-opt)
        target_link_libraries(myMCpp-core PRIVATE SPIRV-Tools-opt)
    endif()
    
    if(APPLE)
        target_link_libraries(myMCpp-core PUBLIC MoltenVK::MoltenVK)
    endif()
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(myMCpp-core PUBLIC OpenGL::EGL)
    find_package(X11 REQUIRED)
    target_include_directories(myMCpp-core PUBLIC ${X11_INCLUDE_DIR} ${X11_Xrandr_INCLUDE_PATH})
    target_link_libraries(myMCpp-core PUBLIC ${X11_LIBRARIES} ${X11_Xrandr_LIB})
    
    find_package(Wayland REQUIRED COMPONENTS Client Egl)
    target_link_libraries(myMCpp-core PUBLIC Wayland::Client Wayland::Egl)
endif()

if(APPLE)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    target_link_libraries(myMCpp-core PUBLIC
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        ${COCOA_LIBRARY}
        ${QUARTZCORE_LIBRARY}
    )
    
    set_source_files_properties(
        renderer/Metal/MetalRenderer.mm
        renderer/Metal/MetalDevice.mm
        renderer/Metal/MetalPipeline.mm
        renderer/Metal/MetalResources.mm
        renderer/OpenGL/GLContextAGL.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    
    set(METAL_SHADERS
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/IconShared.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/background_ps.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/background_vs.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/icon_ps.metal
        ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/icon_vs.metal
    )
    
    if(CMAKE_GENERATOR MATCHES "Xcode")
        set_target_properties(myMCpp-core PROPERTIES
            XCODE_ATTRIBUTE_MTL_ENABLE_DEBUG_INFO INCLUDE_SOURCE
        )
        foreach(shader IN LISTS METAL_SHADERS)
            target_sources(myMCpp-core PRIVATE ${shader})
            set_source_files_properties(${shader} PROPERTIES LANGUAGE METAL)
        endforeach()
    endif()

    set(METAL_AIR_FILES)
    set(METALLIB_OUTPUT ${CMAKE_BINARY_DIR}/default.metallib)

    foreach(shader IN LISTS METAL_SHADERS)
        get_filename_component(shader_name ${shader} NAME_WE)
        set(air_file ${CMAKE_BINARY_DIR}/Metal/${shader_name}.air)
        list(APPEND METAL_AIR_FILES ${air_file})

        add_custom_command(
            OUTPUT ${air_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Metal
            COMMAND xcrun metal -ffast-math -std=macos-metal2.0 -target air64-apple-macos10.13
                $<$<NOT:$<CONFIG:Release,MinSizeRel>>:-gline-tables-only>
                -I ${CMAKE_SOURCE_DIR}/resources/shaders/Metal
                -o ${air_file} -c ${shader}
            DEPENDS ${shader} ${CMAKE_SOURCE_DIR}/resources/shaders/Metal/IconShared.metal
            COMMENT "Compiling Metal shader ${shader_name}"
        )
    endforeach()

    add_custom_command(
        OUTPUT ${METALLIB_OUTPUT}
        COMMAND xcrun metallib -o ${METALLIB_OUTPUT} ${METAL_AIR_FILES}
        DEPENDS ${METAL_AIR_FILES}
        COMMENT "Linking Metal library"
    )

    add_custom_target(CompileMetalShaders DEPENDS ${METALLIB_OUTPUT})
    add_dependencies(myMCpp-core CompileMetalShaders)
endif()

target_link_libraries(myMCpp-core PRIVATE zlib)
target_include_directories(myMCpp-core PRIVATE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})

if(MSVC)
    target_compile_options(myMCpp-core PRIVATE /W4 /EHsc)
else()
    target_compile_options(myMCpp-core PRIVATE -Wall -Wextra -pedantic)
endif()
