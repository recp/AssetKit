FILE(GLOB CSources *.h *.c)
target_sources(${PROJECT_NAME} 
  PRIVATE
  ${CSources}
)

add_subdirectory(bbox)
add_subdirectory(bitwise)
add_subdirectory(camera)
add_subdirectory(coord)
add_subdirectory(default)
add_subdirectory(geom)
add_subdirectory(image)
add_subdirectory(instance)
add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(light)
add_subdirectory(mem)
add_subdirectory(mesh)
add_subdirectory(morph)
add_subdirectory(node)
add_subdirectory(resc)
add_subdirectory(skin)
add_subdirectory(transform)

if(MSVC)
  add_subdirectory(win32)
endif()