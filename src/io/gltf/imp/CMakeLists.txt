file(GLOB CSources CONFIGURE_DEPENDS *.h *.c)
target_sources(${PROJECT_NAME} 
  PRIVATE
  ${CSources}
)

add_subdirectory(core)
add_subdirectory(ext)
