set(CSources
    3mf.h)

target_sources(${PROJECT_NAME}
  PRIVATE
  ${CSources}
)

add_subdirectory(imp)

if(AK_WITH_3MF_EXPORTER)
  add_subdirectory(exp)
endif()
