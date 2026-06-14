set(CSources
    3mf.h)

if(AK_WITH_3MF_EXPORTER)
  list(APPEND CSources export.c)
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
  ${CSources}
)

add_subdirectory(imp)
