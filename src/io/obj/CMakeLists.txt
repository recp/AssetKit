file(GLOB CSources CONFIGURE_DEPENDS *.h *.c)
target_sources(${PROJECT_NAME}
  PRIVATE
  ${CSources}
)

if(AK_WITH_OBJ_EXPORTER)
  add_subdirectory(exp)
endif()
