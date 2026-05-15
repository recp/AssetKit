file(GLOB CSources CONFIGURE_DEPENDS *.h *.c)
target_sources(${PROJECT_NAME} 
  PRIVATE
  ${CSources}
)

add_subdirectory(core)
add_subdirectory(fx)
add_subdirectory(fixup)
add_subdirectory(bugfix)
add_subdirectory(1.4)
add_subdirectory(brep)