set(AK_IMAGE_SOURCES
    alpha.c
    alpha.h
    export.h
    image.c)

if(AK_WITH_IMAGE_EXPORTER)
  list(APPEND AK_IMAGE_SOURCES export.c)
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
  ${AK_IMAGE_SOURCES}
)
