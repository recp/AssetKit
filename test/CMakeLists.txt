add_executable(assetkit_tests
  runner.c
  src/test_memory.c
  src/test_material.c
  src/collada/test_dae_load.c
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
