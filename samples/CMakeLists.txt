cmake_minimum_required(VERSION 3.16)

if(NOT TARGET assetkit::assetkit)
  project(assetkit_samples LANGUAGES C)
  find_package(assetkit CONFIG REQUIRED)
endif()

if(NOT COMMAND ak_target_c_options)
  function(ak_target_c_options target)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W3)
    else()
      target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-overlength-strings)
    endif()
  endfunction()
endif()

if(NOT COMMAND ak_target_link_options)
  function(ak_target_link_options target)
  endfunction()
endif()

option(AK_BUILD_GL_SAMPLES "Build optional OpenGL sample programs" ON)

set(AK_SAMPLE_TARGETS)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../deps/cglm/include/cglm/cglm.h")
  set(AK_SAMPLE_CGLM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../deps/cglm/include")
else()
  find_path(AK_SAMPLE_CGLM_INCLUDE_DIR NAMES cglm/cglm.h)
endif()

function(ak_add_sample name source)
  add_executable(${name} ${source})
  ak_target_c_options(${name})
  ak_target_link_options(${name})
  set_target_properties(${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)
  target_link_libraries(${name} PRIVATE assetkit::assetkit)
  set(AK_SAMPLE_TARGETS ${AK_SAMPLE_TARGETS} ${name} PARENT_SCOPE)
endfunction()

ak_add_sample(sample_read_meshes read_meshes.c)
ak_add_sample(sample_inspect_materials inspect_materials.c)
ak_add_sample(sample_inspect_textures inspect_textures.c)
ak_add_sample(sample_inspect_scene inspect_scene.c)
ak_add_sample(sample_inspect_animation inspect_animation.c)
ak_add_sample(sample_inspect_deformers inspect_deformers.c)
if(AK_SAMPLE_CGLM_INCLUDE_DIR)
  ak_add_sample(sample_deformer_playback deformer_playback.c)
  target_include_directories(sample_deformer_playback
                             PRIVATE
                             ${AK_SAMPLE_CGLM_INCLUDE_DIR})
else()
  message(WARNING
          "Skipping sample_deformer_playback: cglm was not found")
endif()
ak_add_sample(sample_export_scene export_scene.c)

if(AK_BUILD_GL_SAMPLES)
  find_package(OpenGL QUIET)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(GLFW3 QUIET glfw3)
    pkg_check_modules(GLEW QUIET glew)
    pkg_check_modules(LIBPNG QUIET libpng)
  endif()

  if(OpenGL_FOUND AND GLFW3_FOUND AND GLEW_FOUND AND AK_SAMPLE_CGLM_INCLUDE_DIR)
    function(ak_add_gl_sample name source)
      ak_add_sample(${name} ${source})
      target_compile_definitions(${name}
                                 PRIVATE
                                 GL_SILENCE_DEPRECATION
                                 ASSETKIT_SAMPLE_GL_ROOT="${CMAKE_CURRENT_LIST_DIR}/gl")
      target_include_directories(${name}
                                 PRIVATE
                                 ${GLFW3_INCLUDE_DIRS}
                                 ${GLEW_INCLUDE_DIRS}
                                 ${AK_SAMPLE_CGLM_INCLUDE_DIR})
      target_compile_options(${name}
                             PRIVATE
                             ${GLFW3_CFLAGS_OTHER}
                             ${GLEW_CFLAGS_OTHER})
      target_link_directories(${name}
                              PRIVATE
                              ${GLFW3_LIBRARY_DIRS}
                              ${GLEW_LIBRARY_DIRS})
      target_link_libraries(${name}
                            PRIVATE
                            OpenGL::GL
                            ${GLFW3_LIBRARIES}
                            ${GLEW_LIBRARIES})
      if(LIBPNG_FOUND)
        target_compile_definitions(${name} PRIVATE ASSETKIT_SAMPLE_GL_HAS_LIBPNG)
        target_include_directories(${name} PRIVATE ${LIBPNG_INCLUDE_DIRS})
        target_compile_options(${name} PRIVATE ${LIBPNG_CFLAGS_OTHER})
        target_link_directories(${name} PRIVATE ${LIBPNG_LIBRARY_DIRS})
        target_link_libraries(${name} PRIVATE ${LIBPNG_LIBRARIES})
      endif()
      if(UNIX AND NOT APPLE)
        target_link_libraries(${name} PRIVATE m)
      endif()
      set(AK_SAMPLE_TARGETS ${AK_SAMPLE_TARGETS} PARENT_SCOPE)
    endfunction()

    ak_add_gl_sample(sample_gl_basic gl/01_mesh/mesh_viewer.c)
    ak_add_gl_sample(sample_gl_material gl/02_material/material_viewer.c)
    ak_add_gl_sample(sample_gl_lighting gl/03_lighting/lighting_viewer.c)
  else()
    message(WARNING
            "Skipping OpenGL samples: OpenGL, glfw3, glew, and cglm are required")
  endif()
endif()

add_custom_target(assetkit_samples DEPENDS ${AK_SAMPLE_TARGETS})

set(AK_SAMPLE_HELP_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E echo "AssetKit sample binaries:"
    COMMAND ${CMAKE_COMMAND} -E echo "  exact paths for this build are listed below"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Available in this build:")
foreach(AK_SAMPLE_TARGET IN LISTS AK_SAMPLE_TARGETS)
  list(APPEND AK_SAMPLE_HELP_COMMANDS
       COMMAND ${CMAKE_COMMAND} -E echo "  $<TARGET_FILE:${AK_SAMPLE_TARGET}>")
endforeach()
list(APPEND AK_SAMPLE_HELP_COMMANDS
     COMMAND ${CMAKE_COMMAND} -E echo ""
     COMMAND ${CMAKE_COMMAND} -E echo "Try these with any supported asset path: .gltf .glb .dae .obj .stl .ply .3mf"
     COMMAND ${CMAKE_COMMAND} -E echo "  sample_inspect_scene /path/to/model.gltf"
     COMMAND ${CMAKE_COMMAND} -E echo "  sample_read_meshes /path/to/model.glb"
     COMMAND ${CMAKE_COMMAND} -E echo "  sample_export_scene /path/to/model.gltf /tmp/assetkit-export obj")
add_custom_target(assetkit_samples_help
                  ${AK_SAMPLE_HELP_COMMANDS}
                  DEPENDS ${AK_SAMPLE_TARGETS})
