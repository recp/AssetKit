cmake_minimum_required(VERSION 3.16)
project(assetkit VERSION 0.3.2 LANGUAGES C)

if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(DEFAULT_BUILD_TYPE "Release")

get_directory_property(AK_HAS_PARENT PARENT_DIRECTORY)

set(CMAKE_C_FLAGS_DEBUG "-g")
if(MSVC)
  set(CMAKE_C_FLAGS_RELEASE "/Ox /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "/Ox /DNDEBUG")
else()
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

set(AK_BUILD)
option(AK_SHARED "Shared build" ON)
option(AK_STATIC "Static build" OFF)
option(AK_USE_C99 "" OFF)
option(AK_USE_TEST "Enable Tests" OFF)
option(AK_BUILD_GLTF_DRACO_DECODER "Build optional glTF Draco decoder shim" ON)
option(AK_BUILD_GLTF_MESHOPT_DECODER "Build optional glTF meshoptimizer decoder shim" ON)
option(AK_FETCH_DEPS "Fetch optional decoder dependencies into AK_DEPS_ROOT when missing" ON)
option(AK_ENABLE_LTO "Enable link-time optimization for release builds" OFF)
set(AK_DRACO_ROOT "" CACHE PATH "Optional Draco install/source root")
set(AK_MESHOPT_ROOT "" CACHE PATH "Optional meshoptimizer source root")
set(AK_DEPS_ROOT "${PROJECT_SOURCE_DIR}/deps" CACHE PATH "AssetKit dependency checkout root")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if(NOT AK_STATIC AND AK_SHARED)
  set(AK_BUILD SHARED)
else(AK_STATIC)
  set(AK_BUILD STATIC)
endif()

if(AK_USE_C99)
  set(CMAKE_C_STANDARD 99)
endif()

if(MSVC)
  # Ref: https://skia.googlesource.com/third_party/sdl/+/refs/heads/master/CMakeLists.txt#225
  # Make sure /RTC1 is disabled, otherwise it will use functions from the CRT
  foreach(flag_var
      CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    string(REGEX REPLACE "/RTC(su|[1su])" "" ${flag_var} "${${flag_var}}")
  endforeach(flag_var)
endif()

if(NOT AK_HAS_PARENT AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(AK_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT AK_LTO_OK OUTPUT AK_LTO_MSG)
  if(AK_LTO_OK)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
  else()
    message(WARNING "LTO was requested but is not supported: ${AK_LTO_MSG}")
  endif()
endif()

function(ak_target_common_options target)
  if(MSVC)
    target_compile_options(${target}
        PRIVATE
            /W3
            $<$<CONFIG:Release>:/Gy>
            $<$<CONFIG:Release>:/Oi>
            $<$<CONFIG:RelWithDebInfo>:/Gy>
            $<$<CONFIG:RelWithDebInfo>:/Oi>)
  else()
    target_compile_options(${target}
        PRIVATE
            -Wall
            -Wextra
            -Wstrict-aliasing=2
            -Wno-overlength-strings
            $<$<COMPILE_LANGUAGE:C>:-Wmissing-declarations>)
  endif()
endfunction()

function(ak_target_c_options target)
  ak_target_common_options(${target})
  if(MSVC)
    target_compile_options(${target} PRIVATE /TC)
  endif()
endfunction()

function(ak_target_cxx_options target)
  ak_target_common_options(${target})
endfunction()

include(GNUInstallDirs)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

if(NOT CPack_CMake_INCLUDED)
  include(CPack)
endif()

# Target Start
add_library(${PROJECT_NAME} ${AK_BUILD} "")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
ak_target_c_options(${PROJECT_NAME})

if(APPLE)
  set(AK_INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set(AK_INSTALL_RPATH "\$ORIGIN")
endif()

if(AK_SHARED)
  target_compile_definitions(${PROJECT_NAME} PRIVATE AK_EXPORTS)
  if(MSVC)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            _WINDOWS
            _USRDLL
            _assetkit_dll_DLL)
  endif()
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC AK_STATIC)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
                              VERSION ${PROJECT_VERSION} 
                            SOVERSION ${PROJECT_VERSION_MAJOR})
if(AK_INSTALL_RPATH)
  set_target_properties(${PROJECT_NAME} PROPERTIES
                        INSTALL_RPATH "${AK_INSTALL_RPATH}")
endif()

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

target_link_libraries(${PROJECT_NAME} PRIVATE ds ${CMAKE_DL_LIBS})

if(AK_BUILD_GLTF_DRACO_DECODER OR AK_BUILD_GLTF_MESHOPT_DECODER)
  enable_language(CXX)
endif()

if(AK_BUILD_GLTF_DRACO_DECODER)
  find_path(AK_DRACO_INCLUDE_DIR
            NAMES draco/compression/decode.h
            HINTS ${AK_DRACO_ROOT}
                  ${AK_DRACO_ROOT}/install
                  ${AK_DRACO_ROOT}/include
                  ${AK_DEPS_ROOT}/draco/install
                  ${AK_DEPS_ROOT}/draco
                  ${AK_DEPS_ROOT}/draco/include
            PATH_SUFFIXES include
            NO_DEFAULT_PATH)
  find_library(AK_DRACO_LIBRARY
               NAMES draco
               HINTS ${AK_DRACO_ROOT}
                     ${AK_DRACO_ROOT}/install
                     ${AK_DRACO_ROOT}/lib
                     ${AK_DEPS_ROOT}/draco/install
                     ${AK_DEPS_ROOT}/draco
                     ${AK_DEPS_ROOT}/draco/lib
               PATH_SUFFIXES lib
               NO_DEFAULT_PATH)

  if((NOT AK_DRACO_INCLUDE_DIR OR NOT AK_DRACO_LIBRARY) AND AK_FETCH_DEPS)
    include(ExternalProject)

    set(AK_DRACO_INSTALL_DIR "${AK_DEPS_ROOT}/draco/install")
    set(AK_DRACO_INCLUDE_DIR "${AK_DRACO_INSTALL_DIR}/include")
    if(WIN32)
      set(AK_DRACO_LIBRARY "${AK_DRACO_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/draco.lib")
    else()
      set(AK_DRACO_LIBRARY "${AK_DRACO_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/libdraco.a")
    endif()

    ExternalProject_Add(assetkit_draco_dep
        GIT_REPOSITORY https://github.com/google/draco.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        GIT_SUBMODULES ""
        SOURCE_DIR "${AK_DEPS_ROOT}/draco"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/assetkit_draco-build"
        INSTALL_DIR "${AK_DRACO_INSTALL_DIR}"
        BUILD_BYPRODUCTS "${AK_DRACO_LIBRARY}"
        CMAKE_ARGS
            -Wno-dev
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DDRACO_ANIMATION_ENCODING=OFF
            -DDRACO_BUILD_EXECUTABLES=OFF
            -DDRACO_GLTF_BITSTREAM=ON
            -DDRACO_INSTALL=ON
            -DDRACO_JS_GLUE=OFF
            -DDRACO_MAYA_PLUGIN=OFF
            -DDRACO_TESTS=OFF
            -DDRACO_TRANSCODER_SUPPORTED=OFF
            -DDRACO_UNITY_PLUGIN=OFF
            -DDRACO_WASM=OFF
            -DBUILD_SHARED_LIBS=OFF)
  endif()

  if(NOT AK_DRACO_INCLUDE_DIR OR NOT AK_DRACO_LIBRARY)
    message(FATAL_ERROR
            "Draco decoder requested but Draco was not found. "
            "Set AK_DRACO_ROOT, enable AK_FETCH_DEPS, or disable "
            "AK_BUILD_GLTF_DRACO_DECODER.")
  endif()

  add_library(assetkit_draco SHARED
              src/decoders/gltf/draco/assetkit_draco.cc)
  ak_target_cxx_options(assetkit_draco)
  if(TARGET assetkit_draco_dep)
    add_dependencies(assetkit_draco assetkit_draco_dep)
  endif()
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
  if(AK_INSTALL_RPATH)
    set_target_properties(assetkit_draco PROPERTIES
                          INSTALL_RPATH "${AK_INSTALL_RPATH}")
  endif()
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
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(AK_BUILD_GLTF_MESHOPT_DECODER)
  find_path(AK_MESHOPT_INCLUDE_DIR
            NAMES meshoptimizer.h
            HINTS ${AK_MESHOPT_ROOT}
                  ${AK_MESHOPT_ROOT}/src
                  ${AK_DEPS_ROOT}/meshoptimizer
                  ${AK_DEPS_ROOT}/meshoptimizer/src
            PATH_SUFFIXES src
            NO_DEFAULT_PATH)

  if(NOT AK_MESHOPT_INCLUDE_DIR AND AK_FETCH_DEPS)
    include(FetchContent)

    FetchContent_Declare(assetkit_meshoptimizer_dep
        GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        SOURCE_DIR "${AK_DEPS_ROOT}/meshoptimizer")
    FetchContent_GetProperties(assetkit_meshoptimizer_dep)
    if(NOT assetkit_meshoptimizer_dep_POPULATED)
      FetchContent_Populate(assetkit_meshoptimizer_dep)
    endif()

    set(AK_MESHOPT_INCLUDE_DIR "${AK_DEPS_ROOT}/meshoptimizer/src")
  endif()

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
            "Set AK_MESHOPT_ROOT, enable AK_FETCH_DEPS, or disable "
            "AK_BUILD_GLTF_MESHOPT_DECODER.")
  endif()

  add_library(assetkit_meshoptimizer SHARED
              src/decoders/gltf/meshopt/assetkit_meshoptimizer.cc
              ${AK_MESHOPT_SOURCES})
  ak_target_cxx_options(assetkit_meshoptimizer)
  target_include_directories(assetkit_meshoptimizer
      PRIVATE
          ${AK_MESHOPT_INCLUDE_DIR})
  set_target_properties(assetkit_meshoptimizer PROPERTIES
                        CXX_STANDARD 11
                        CXX_STANDARD_REQUIRED YES
                        CXX_VISIBILITY_PRESET hidden
                        VISIBILITY_INLINES_HIDDEN YES)
  if(AK_INSTALL_RPATH)
    set_target_properties(assetkit_meshoptimizer PROPERTIES
                          INSTALL_RPATH "${AK_INSTALL_RPATH}")
  endif()

  install(TARGETS assetkit_meshoptimizer
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ak DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PATTERN ".*" EXCLUDE)

# Config
export(TARGETS ${PROJECT_NAME}
       NAMESPACE ${PROJECT_NAME}::
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
)

install(EXPORT      ${PROJECT_NAME}
        FILE        "${PROJECT_NAME}Config.cmake"
        NAMESPACE   ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
