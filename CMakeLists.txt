cmake_minimum_required(VERSION 3.10)
project(assetkit VERSION 0.1.0 LANGUAGES C)

if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(DEFAULT_BUILD_TYPE "Release")

set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(AK_BUILD)
option(AK_SHARED "Shared build" ON)
option(AK_STATIC "Static build" OFF)
option(AK_USE_C99 "" OFF)
option(AK_USE_TEST "Enable Tests" OFF)

if(NOT AK_STATIC AND AK_SHARED)
  set(AK_BUILD SHARED)
else(AK_STATIC)
  set(AK_BUILD STATIC)
endif()

if(aK_USE_C99)
  set(C_STANDARD 99)
endif()

if(MSVC)
  add_definitions(-DNDEBUG -D_WINDOWS -D_USRDLL -DAK_EXPORTS -D_assetkit_dll_DLL)
  add_compile_options(/W3 /Ox /Gy /Oi /TC)
else()
  add_compile_options(-Wall -Wextra -O3 -Wstrict-aliasing=2 -pedantic -Wno-overlength-strings -Wmissing-declarations)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(GNUInstallDirs)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Target Start
add_library(${PROJECT_NAME} ${AK_BUILD} "")

add_subdirectory(src)

set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/lib/)
find_package(ds)


# include_directories(
#   PUBLIC
  
#   ${PROJECT_SOURCE_DIR}/include
  
#   PRIVATE
  
#   ${PROJECT_SOURCE_DIR}/lib/cglm/include
#   ${PROJECT_SOURCE_DIR}/lib/libds/include
#   ${PROJECT_SOURCE_DIR}/lib/json/include
#   ${PROJECT_SOURCE_DIR}/lib/xml/include
#   ${PROJECT_SOURCE_DIR}/lib/stb
# )

# link_directories(
#   PRIVATE
#   ${PROJECT_SOURCE_DIR}/lib/libds/.libs
# )

target_link_libraries(${PROJECT_NAME} INTERFACE ${ds})

set_target_properties(${PROJECT_NAME} PROPERTIES
                              VERSION ${PROJECT_VERSION} 
                            SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(${PROJECT_NAME} PRIVATE include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Install 
install(TARGETS ${PROJECT_NAME}
        EXPORT  ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PATTERN ".*" EXCLUDE)

# Test Configuration
if(AK_USE_TEST)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()