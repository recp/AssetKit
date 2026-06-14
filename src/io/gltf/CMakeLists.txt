file(GLOB CSources CONFIGURE_DEPENDS *.h *.c)
target_sources(${PROJECT_NAME} 
  PRIVATE
  ${CSources}
)

add_subdirectory(imp)
if(AK_WITH_GLTF_EXPORTER)
  add_subdirectory(exp)
endif()
