cmake_minimum_required(VERSION 3.10)
project(assetkit VERSION 0.3.2 LANGUAGES C)

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
option(AK_BUILD_GLTF_DRACO_DECODER "Build optional glTF Draco decoder shim" OFF)
option(AK_BUILD_GLTF_MESHOPT_DECODER "Build optional glTF meshoptimizer decoder shim" OFF)
set(AK_DRACO_ROOT "" CACHE PATH "Optional Draco install/source root")
set(AK_MESHOPT_ROOT "" CACHE PATH "Optional meshoptimizer source root")

if(NOT AK_STATIC AND AK_SHARED)
  set(AK_BUILD SHARED)
else(AK_STATIC)
  set(AK_BUILD STATIC)
endif()

if(AK_USE_C99)
  set(C_STANDARD 99)
endif()

if(MSVC)
  add_definitions(-DNDEBUG -D_WINDOWS -D_USRDLL -DAK_EXPORTS -D_assetkit_dll_DLL)
  add_compile_options(/W3 /Ox /Gy /Oi /TC)
  
  # Ref: https://skia.googlesource.com/third_party/sdl/+/refs/heads/master/CMakeLists.txt#225
  # Make sure /RTC1 is disabled, otherwise it will use functions from the CRT
  foreach(flag_var
      CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    string(REGEX REPLACE "/RTC(su|[1su])" "" ${flag_var} "${${flag_var}}")
  endforeach(flag_var)
else()
  add_compile_options(-Wall -Wextra -O3 -Wstrict-aliasing=2 -Wno-overlength-strings -Wmissing-declarations)
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

if(NOT CPack_CMake_INCLUDED)
  include(CPack)
endif()

# Target Start
add_library(${PROJECT_NAME} ${AK_BUILD} "")

if(AK_SHARED)
  add_definitions(-DAK_EXPORTS)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DAK_STATIC)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
                              VERSION ${PROJECT_VERSION} 
                            SOVERSION ${PROJECT_VERSION_MAJOR})

add_subdirectory(src)

# Dependencies && Submodules

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/deps/cglm/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

add_subdirectory(deps/ds)
add_dependencies(${PROJECT_NAME} ds)

# set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/deps" ${CMAKE_MODULE_PATH})
# find_package(ds REQUIRED)
# target_link_libraries(${PROJECT_NAME} ds::ds)

# Include Dirs
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src

        ${PROJECT_SOURCE_DIR}/deps/cglm/include
        ${PROJECT_SOURCE_DIR}/deps/ds/include
        ${PROJECT_SOURCE_DIR}/deps/json/include
        ${PROJECT_SOURCE_DIR}/deps/xml/include
)

target_link_libraries(${PROJECT_NAME} ds ${CMAKE_DL_LIBS})

if(AK_BUILD_GLTF_DRACO_DECODER OR AK_BUILD_GLTF_MESHOPT_DECODER)
  enable_language(CXX)
endif()

if(AK_BUILD_GLTF_DRACO_DECODER)
  find_path(AK_DRACO_INCLUDE_DIR
            NAMES draco/compression/decode.h
            HINTS ${AK_DRACO_ROOT}
                  ${AK_DRACO_ROOT}/include
                  ${PROJECT_SOURCE_DIR}/deps/draco
                  ${PROJECT_SOURCE_DIR}/deps/draco/include
            PATH_SUFFIXES include)
  find_library(AK_DRACO_LIBRARY
               NAMES draco
               HINTS ${AK_DRACO_ROOT}
                     ${AK_DRACO_ROOT}/lib
                     ${PROJECT_SOURCE_DIR}/deps/draco
                     ${PROJECT_SOURCE_DIR}/deps/draco/lib
               PATH_SUFFIXES lib)

  if(NOT AK_DRACO_INCLUDE_DIR OR NOT AK_DRACO_LIBRARY)
    message(FATAL_ERROR
            "Draco decoder requested but Draco was not found. "
            "Set AK_DRACO_ROOT or disable AK_BUILD_GLTF_DRACO_DECODER.")
  endif()

  add_library(assetkit_draco SHARED
              extras/decoders/gltf/draco/assetkit_draco.cc)
  target_include_directories(assetkit_draco
      SYSTEM
      PRIVATE
          ${AK_DRACO_INCLUDE_DIR})
  target_include_directories(assetkit_draco
      PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/include
          ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${PROJECT_SOURCE_DIR}/deps/cglm/include
          ${PROJECT_SOURCE_DIR}/deps/ds/include
          ${PROJECT_SOURCE_DIR}/deps/json/include
          ${PROJECT_SOURCE_DIR}/deps/xml/include)
  target_link_libraries(assetkit_draco
      PRIVATE
          ${PROJECT_NAME}
          ds
          ${AK_DRACO_LIBRARY})
  set_target_properties(assetkit_draco PROPERTIES
                        CXX_STANDARD 11
                        CXX_STANDARD_REQUIRED YES
                        CXX_VISIBILITY_PRESET hidden
                        VISIBILITY_INLINES_HIDDEN YES)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(assetkit_draco
        PRIVATE
            -Wno-deprecated-copy-with-user-provided-copy
            -Wno-ignored-qualifiers
            -Wno-implicit-const-int-float-conversion
            -Wno-sign-compare)
  endif()

  install(TARGETS assetkit_draco
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(AK_BUILD_GLTF_MESHOPT_DECODER)
  find_path(AK_MESHOPT_INCLUDE_DIR
            NAMES meshoptimizer.h
            HINTS ${AK_MESHOPT_ROOT}
                  ${AK_MESHOPT_ROOT}/src
                  ${PROJECT_SOURCE_DIR}/deps/meshoptimizer
                  ${PROJECT_SOURCE_DIR}/deps/meshoptimizer/src
            PATH_SUFFIXES src)
  set(AK_MESHOPT_SOURCES
      ${AK_MESHOPT_INCLUDE_DIR}/indexcodec.cpp
      ${AK_MESHOPT_INCLUDE_DIR}/vertexcodec.cpp
      ${AK_MESHOPT_INCLUDE_DIR}/vertexfilter.cpp)

  if(NOT AK_MESHOPT_INCLUDE_DIR
     OR NOT EXISTS "${AK_MESHOPT_INCLUDE_DIR}/indexcodec.cpp"
     OR NOT EXISTS "${AK_MESHOPT_INCLUDE_DIR}/vertexcodec.cpp"
     OR NOT EXISTS "${AK_MESHOPT_INCLUDE_DIR}/vertexfilter.cpp")
    message(FATAL_ERROR
            "meshoptimizer decoder requested but source files were not found. "
            "Set AK_MESHOPT_ROOT to a meshoptimizer checkout or disable "
            "AK_BUILD_GLTF_MESHOPT_DECODER.")
  endif()

  add_library(assetkit_meshoptimizer SHARED
              extras/decoders/gltf/meshopt/assetkit_meshoptimizer.cc
              ${AK_MESHOPT_SOURCES})
  target_include_directories(assetkit_meshoptimizer
      PRIVATE
          ${AK_MESHOPT_INCLUDE_DIR})
  set_target_properties(assetkit_meshoptimizer PROPERTIES
                        CXX_STANDARD 11
                        CXX_STANDARD_REQUIRED YES
                        CXX_VISIBILITY_PRESET hidden
                        VISIBILITY_INLINES_HIDDEN YES)

  install(TARGETS assetkit_meshoptimizer
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# Test Configuration
if(AK_USE_TEST)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

# Install 
install(TARGETS ${PROJECT_NAME}
        EXPORT  ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/ak DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PATTERN ".*" EXCLUDE)

# Config
export(TARGETS ${PROJECT_NAME}
       NAMESPACE ${PROJECT_NAME}::
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
)

install(EXPORT      ${PROJECT_NAME}
        NAMESPACE   ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake)
