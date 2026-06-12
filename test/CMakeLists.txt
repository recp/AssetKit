add_executable(assetkit_tests
  runner.c
  src/core/memory.c
  src/core/format_edge.c
  src/io/dae/exp/core.c
  src/io/dae/exp/material.c
  src/io/dae/exp/scene.c
  src/io/dae/exp/animation.c
  src/io/dae/exp/geometry.c
  src/io/dae/imp/scene.c
  src/io/dae/imp/folder.c
  src/io/gltf/exp/core.c
  src/io/gltf/exp/material.c
  src/io/gltf/exp/textures.c
  src/io/gltf/exp/extensions.c
  src/io/gltf/exp/scene.c
  src/io/gltf/exp/animation.c
  src/io/obj/exp/core.c
  src/io/stl/exp/core.c
  src/instance/test_instance_core.c
  src/material/material.c
)

target_include_directories(assetkit_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/deps/cglm/include
  ${PROJECT_SOURCE_DIR}/deps/ds/include
)

target_link_libraries(assetkit_tests PRIVATE ${PROJECT_NAME})

add_test(NAME assetkit_tests COMMAND assetkit_tests)
