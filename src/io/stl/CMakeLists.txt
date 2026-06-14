set(CSources
    common.h
    stl.c
    stl.h)

if(AK_WITH_STL_EXPORTER)
  list(APPEND CSources export.c)
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
  ${CSources}
)
