cmake_minimum_required(VERSION 3.16)
set(NAME webgpu-native-examples)
project(${NAME} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set directory locations (allowing us to move directories easily)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

# ==============================================================================
# Dependencies
# ==============================================================================

# Google Dawn

set(DAWN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/dawn)
set(DAWN_INCLUDE_DIR ${DAWN_DIR}/include)
set(DAWN_LIB_DIR ${DAWN_DIR})
if (EXISTS ${DAWN_DIR}/Debug/webgpu_dawn.lib)
    set(DAWN_LIB_DIR ${DAWN_DIR}/Debug)
    message(STATUS "Use Debug dawn: ${DAWN_LIB_DIR}")
endif ()
file(GLOB DAWN_LIBS ${DAWN_LIB_DIR}/*.lib)
#message(STATUS "DAWN_LIBS: ${DAWN_LIBS}")

#add_library(dawn STATIC IMPORTED)
#set_target_properties(dawn PROPERTIES
#        INTERFACE_INCLUDE_DIRECTORIES ${DAWN_INCLUDE_DIR}
#        IMPORTED_LOCATION ${DAWN_LIBS}
#)
add_library(dawn INTERFACE)
target_include_directories(dawn INTERFACE ${DAWN_INCLUDE_DIR})

# WebGPU native
set(WEBGPU_NATIVE_HEADERS
  lib/wgpu_native/wgpu_native.h
)

set(WEBGPU_NATIVE_SOURCES
  lib/wgpu_native/wgpu_native.cpp
)

add_library(wgpu_native ${WEBGPU_NATIVE_HEADERS} ${WEBGPU_NATIVE_SOURCES})
target_link_libraries(wgpu_native PUBLIC dawn)
target_compile_options(wgpu_native PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Basis Universal Supercompressed GPU Texture Codec
set(BASISU_SOURCES
  external/basisu/wgpu_basisu.cpp
)

# cimgui + Dear ImGui
set(CIMGUI_SOURCES
    external/cimgui/cimgui.cpp
    external/cimgui/imgui/imgui.cpp
    external/cimgui/imgui/imgui_draw.cpp
    external/cimgui/imgui/imgui_widgets.cpp
    external/cimgui/imgui/imgui_tables.cpp
    external/cimgui/imgui/imgui_demo.cpp
)

# cJSON: Ultralightweight JSON parser in ANSI C.
set(CJSON_SOURCES
    external/cJSON/cJSON.c
)

# KTX (Khronos Texture) Library and Tools
set(KTX_SOURCES
    external/ktx/lib/checkheader.c
    external/ktx/lib/filestream.c
    external/ktx/lib/hashlist.c
    external/ktx/lib/hashtable.c
    external/ktx/lib/memstream.c
    external/ktx/lib/swap.c
    external/ktx/lib/texture.c
)

# rply
set(RPLY_SOURCES
    external/rply/rply.c
)

# ffmpeg
# dnf -y install ffmpeg-devel
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
#    libavdevice
#    libavfilter
#    libavformat
#    libavcodec
#    libswresample
#    libswscale
#    libavutil
#)

# ==============================================================================
# Headers and sources
# ==============================================================================

set(HEADERS
    src/core/api.h
    src/core/argparse.h
    src/core/camera.h
    src/core/file.h
    src/core/frustum.h
    src/core/hashmap.h
    src/core/input.h
    src/core/log.h
    src/core/macro.h
    src/core/math.h
    src/core/platform.h
    src/core/utils.h
    src/core/video_decode.h
    src/core/window.h
    src/examples/common_shaders.h
    src/examples/examples.h
    src/examples/example_base.h
    src/examples/meshes.h
    src/webgpu/api.h
    src/webgpu/buffer.h
    src/webgpu/context.h
    src/webgpu/gltf_model.h
    src/webgpu/imgui_overlay.h
    src/webgpu/pbr.h
    src/webgpu/shader.h
    src/webgpu/text_overlay.h
    src/webgpu/texture.h
)

set(SOURCES
    src/main.c
    src/core/argparse.c
    src/core/camera.c
    src/core/file.c
    src/core/frustum.c
    src/core/hashmap.c
    src/core/log.c
    src/core/math.c
    src/core/utils.c
    src/core/video_decode.c
    src/core/window.c
    src/examples/example_base.c
    src/examples/examples.c
    src/examples/meshes.c
    src/webgpu/buffer.c
    src/webgpu/context.c
    src/webgpu/gltf_model.c
    src/webgpu/imgui_overlay.c
    src/webgpu/pbr.c
    src/webgpu/shader.c
    src/webgpu/text_overlay.c
    src/webgpu/texture.c
)

# examples
set(SOURCES
    ${SOURCES}
    src/examples/a_buffer.c
    src/examples/animometer.c
    # src/examples/aquarium.c
    src/examples/basisu.c
    src/examples/bind_groups.c
    src/examples/blinn_phong_lighting.c
    src/examples/bloom.c
    src/examples/cameras.c
    src/examples/clear_screen.c
    src/examples/compute_boids.c
    src/examples/compute_metaballs.c
    src/examples/compute_particles_easing.c
    src/examples/compute_particles_webgpu_logo.c
    src/examples/compute_particles.c
    src/examples/compute_ray_tracing.c
    src/examples/compute_shader.c
    src/examples/conservative_raster.c
    src/examples/conway.c
    src/examples/conway_paletted_blurring.c
    src/examples/coordinate_system.c
    src/examples/cornell_box.c
    src/examples/cube_reflection.c
    src/examples/cubemap.c
    src/examples/deferred_rendering.c
    src/examples/dynamic_uniform_buffer.c
    src/examples/equirectangular_image.c
    src/examples/fluid_simulation.c
    src/examples/game_of_life.c
    src/examples/gears.c
    src/examples/gerstner_waves.c
    src/examples/gltf_loading.c
    src/examples/gltf_scene_rendering.c
    src/examples/gltf_skinning.c
    src/examples/hdr.c
    src/examples/image_blur.c
    src/examples/imgui_overlay.c
    src/examples/immersive_video.c
    src/examples/instanced_cube.c
    src/examples/minimal.c
    src/examples/msaa_line.c
    src/examples/multi_sampling.c
    src/examples/n_body_simulation.c
    src/examples/normal_map.c
    src/examples/normal_mapping.c
    src/examples/occlusion_query.c
    src/examples/parallax_mapping.c
    src/examples/offscreen_rendering.c
    src/examples/out_of_bounds_viewport.c
    src/examples/pbr_basic.c
    src/examples/pbr_ibl.c
    src/examples/pbr_texture.c
    src/examples/points.c
    src/examples/post_processing.c
    src/examples/pristine_grid.c
    src/examples/prng.c
    src/examples/procedural_mesh.c
    src/examples/radial_blur.c
    src/examples/render_bundles.c
    src/examples/reversed_z.c
    src/examples/sampler_parameters.c
    src/examples/screenshot.c
    src/examples/shadertoy.c
    src/examples/shadow_mapping.c
    src/examples/square.c
    src/examples/stencil_buffer.c
    src/examples/terrain_mesh.c
    src/examples/text_overlay.c
    src/examples/texture_3d.c
    src/examples/texture_cubemap.c
    src/examples/texture_mipmap_gen.c
    src/examples/textured_cube.c
    src/examples/textured_quad.c
    src/examples/tile_map.c
    src/examples/triangle.c
    src/examples/two_cubes.c
    src/examples/vertex_buffer.c
    src/examples/video_uploading.c
    src/examples/volume_rendering_texture_3d.c
    src/examples/wireframe_vertex_pulling.c
)

if(WIN32)
  set(SOURCES ${SOURCES} src/platforms/win32.c)
elseif(APPLE)
  set(SOURCES ${SOURCES} src/platforms/macos.m)
else()
    set(SOURCES ${SOURCES} src/platforms/linux.c)
endif()

# ==============================================================================
# Target definition
# ==============================================================================

set(TARGET wgpu_sample_launcher)

add_executable(${TARGET}
    ${HEADERS}
    ${SOURCES}
    ${BASISU_SOURCES}
    ${CIMGUI_SOURCES}
    ${CJSON_SOURCES}
    ${KTX_SOURCES}
    ${RPLY_SOURCES}
)

set_target_properties(${TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}
)

if (WIN32)
    # nothing to do for now
elseif (APPLE)
    # nothing to do for now
elseif (UNIX)
    find_package(Vulkan REQUIRED FATAL_ERROR)
endif()

# ==============================================================================
# Target properties
# ==============================================================================

set_target_properties(${TARGET} PROPERTIES C_STANDARD 99)
set_target_properties(${TARGET} PROPERTIES C_STANDARD_REQUIRED ON)
set_target_properties(${TARGET} PROPERTIES C_EXTENSIONS OFF)
set_target_properties(${TARGET} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

# ==============================================================================
# Compile options
# ==============================================================================

if(MSVC)
    target_compile_options(${TARGET} PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
    target_compile_options(${TARGET} PRIVATE /fp:fast /utf-8)
else()
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(${TARGET} PRIVATE -ffast-math)
endif()

if(UNIX AND NOT APPLE)
    target_compile_options(${TARGET} PRIVATE -D_POSIX_C_SOURCE=200809L)
endif()

# ==============================================================================
# Include directories
# ==============================================================================

target_include_directories(${TARGET}
        PRIVATE external/basisu
        PRIVATE external/cglm/include
        PRIVATE external/cgltf
        PRIVATE external/cimgui
        PRIVATE external/cJSON
        PRIVATE external/dawn/include
        PRIVATE external/ktx/include
        PRIVATE external/par
        PRIVATE external/rply
        PRIVATE external/sc
        PRIVATE external/stb
)

# ==============================================================================
# Link libraries
# ==============================================================================
target_link_libraries(${TARGET} PRIVATE
        Threads::Threads
        wgpu_native
)
# ==============================================================================
# IDE support
# ==============================================================================

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${TARGET})
source_group(TREE "${CMAKE_SOURCE_DIR}/src" FILES ${HEADERS} ${SOURCES})
source_group(external/basisu FILES ${BASISU_SOURCES})
source_group(external/cimgui FILES ${CIMGUI_SOURCES})
source_group(external/ktx FILES ${KTX_SOURCES})
source_group(external/rply FILES ${RPLY_SOURCES})

# ==============================================================================
# Deployment
# ==============================================================================

# Copy full directories
macro(copy_resource_dirs dirs)
    foreach(dir ${dirs})
        # Replace / at the end of the path (copy dir content VS copy dir)
        string(REGEX REPLACE "/+$" "" dirclean "${dir}")
        message(STATUS "Copying resource ${dirclean} to ${BUILD_DIR}")
        file(COPY ${dirclean} DESTINATION ${BUILD_DIR})
    endforeach()
endmacro()
copy_resource_dirs("${CMAKE_SOURCE_DIR}/assets")
