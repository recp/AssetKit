set(CSources
    ascii.c
    bin.c
    common.h
    ply.c
    ply.h
    util.h)

if(AK_WITH_PLY_EXPORTER)
  list(APPEND CSources export.c)
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
  ${CSources}
)
