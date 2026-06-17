cmake_minimum_required(VERSION 3.16)
project(assetkit VERSION 0.6.1 LANGUAGES C)

if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(DEFAULT_BUILD_TYPE "Release")

get_directory_property(AK_HAS_PARENT PARENT_DIRECTORY)

set(CMAKE_C_FLAGS_DEBUG "-g")
if(MSVC)
  set(CMAKE_C_FLAGS_RELEASE "/Ox /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "/Ox /DNDEBUG")
  set(CMAKE_C_FLAGS_MINSIZEREL "/O1 /Ob2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "/O1 /Ob2 /DNDEBUG")
else()
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG")
  else()
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz -DNDEBUG")
  else()
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
  endif()
endif()

set(AK_BUILD)
option(AK_SHARED "Shared build" ON)
option(AK_STATIC "Static build" OFF)
option(AK_USE_C99 "" OFF)
option(AK_USE_TEST "Enable Tests" OFF)
option(AK_BUILD_EXPORTERS "Build asset export support" ON)
option(AK_BUILD_DAE_EXPORTER "Build COLLADA/DAE exporter" ON)
option(AK_BUILD_GLTF_EXPORTER "Build glTF/GLB exporter" ON)
option(AK_BUILD_OBJ_EXPORTER "Build Wavefront OBJ exporter" ON)
option(AK_BUILD_STL_EXPORTER "Build STL exporter" ON)
option(AK_BUILD_PLY_EXPORTER "Build PLY exporter" ON)
option(AK_BUILD_3MF_EXPORTER "Build 3MF exporter" ON)
option(AK_BUILD_DECODER_SHIMS "Build optional decoder shim libraries" ON)
option(AK_BUILD_GLTF_DRACO_DECODER "Build optional glTF Draco decoder shim" ON)
option(AK_BUILD_GLTF_MESHOPT_DECODER "Build optional glTF meshoptimizer decoder shim" ON)
option(AK_BUILD_GLTF_SPZ_DECODER "Build optional glTF Gaussian splatting (SPZ) decoder shim" ON)
option(AK_BUILD_GLTF_KTX2_DECODER "Build optional glTF KHR_texture_basisu (KTX2/BasisU) decoder shim" ON)
option(AK_FETCH_DEPS "Fetch optional decoder dependencies into AK_DEPS_ROOT when missing" ON)
option(AK_ENABLE_LTO "Enable link-time optimization for release builds" OFF)
option(AK_ENABLE_FAST_MATH "Enable unsafe fast-math optimizations for release builds" OFF)
option(AK_ENABLE_SECTION_GC "Enable function/data sections and linker dead-code stripping for optimized builds" ON)
option(AK_BUILD_TOOLS "Build manual AssetKit importer benchmark tools" OFF)
option(AK_BUILD_SAMPLES "Build AssetKit sample programs" OFF)
set(AK_DRACO_ROOT "" CACHE PATH "Optional Draco install/source root")
set(AK_MESHOPT_ROOT "" CACHE PATH "Optional meshoptimizer source root")
set(AK_SPZ_ROOT "" CACHE PATH "Optional SPZ install/source root")
set(AK_KTX2_ROOT "" CACHE PATH "Optional KTX-Software install/source root")
set(AK_DEPS_ROOT "${PROJECT_SOURCE_DIR}/deps" CACHE PATH "AssetKit dependency checkout root")

set(AK_WITH_DAE_EXPORTER OFF)
set(AK_WITH_GLTF_EXPORTER OFF)
set(AK_WITH_OBJ_EXPORTER OFF)
set(AK_WITH_STL_EXPORTER OFF)
set(AK_WITH_PLY_EXPORTER OFF)
set(AK_WITH_3MF_EXPORTER OFF)
set(AK_WITH_IMAGE_EXPORTER OFF)
if(AK_BUILD_EXPORTERS)
  if(AK_BUILD_DAE_EXPORTER)
    set(AK_WITH_DAE_EXPORTER ON)
    set(AK_WITH_IMAGE_EXPORTER ON)
  endif()
  if(AK_BUILD_GLTF_EXPORTER)
    set(AK_WITH_GLTF_EXPORTER ON)
    set(AK_WITH_IMAGE_EXPORTER ON)
  endif()
  if(AK_BUILD_OBJ_EXPORTER)
    set(AK_WITH_OBJ_EXPORTER ON)
    set(AK_WITH_IMAGE_EXPORTER ON)
  endif()
  if(AK_BUILD_STL_EXPORTER)
    set(AK_WITH_STL_EXPORTER ON)
  endif()
  if(AK_BUILD_PLY_EXPORTER)
    set(AK_WITH_PLY_EXPORTER ON)
  endif()
  if(AK_BUILD_3MF_EXPORTER)
    set(AK_WITH_3MF_EXPORTER ON)
  endif()
endif()

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
            /wd4200
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:/Gy>
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:/Gw>
            $<$<CONFIG:Release>:/Oi>
            $<$<CONFIG:RelWithDebInfo>:/Oi>
            $<$<AND:$<BOOL:${AK_ENABLE_FAST_MATH}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:/fp:fast>)
    target_compile_definitions(${target}
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_WARNINGS)
  else()
    target_compile_options(${target}
        PRIVATE
            -Wall
            -Wextra
            -Wno-overlength-strings
            $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-restrict>
            $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-uninitialized>
            $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-maybe-uninitialized>
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:-ffunction-sections>
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:-fdata-sections>
            $<$<AND:$<BOOL:${AK_ENABLE_FAST_MATH}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:-ffast-math>)
  endif()
endfunction()

function(ak_target_link_options target)
  if(MSVC)
    target_link_options(${target}
        PRIVATE
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:/OPT:REF>
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:/OPT:ICF>)
  elseif(APPLE)
    target_link_options(${target}
        PRIVATE
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:LINKER:-dead_strip>)
  else()
    target_link_options(${target}
        PRIVATE
            $<$<AND:$<BOOL:${AK_ENABLE_SECTION_GC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>>:LINKER:--gc-sections>)
  endif()
endfunction()

function(ak_target_exporter_definitions target scope)
  target_compile_definitions(${target}
      ${scope}
          AK_BUILD_EXPORTERS=$<IF:$<BOOL:${AK_BUILD_EXPORTERS}>,1,0>
          AK_BUILD_DAE_EXPORTER=$<IF:$<BOOL:${AK_WITH_DAE_EXPORTER}>,1,0>
          AK_BUILD_GLTF_EXPORTER=$<IF:$<BOOL:${AK_WITH_GLTF_EXPORTER}>,1,0>
          AK_BUILD_OBJ_EXPORTER=$<IF:$<BOOL:${AK_WITH_OBJ_EXPORTER}>,1,0>
          AK_BUILD_STL_EXPORTER=$<IF:$<BOOL:${AK_WITH_STL_EXPORTER}>,1,0>
          AK_BUILD_PLY_EXPORTER=$<IF:$<BOOL:${AK_WITH_PLY_EXPORTER}>,1,0>
          AK_BUILD_3MF_EXPORTER=$<IF:$<BOOL:${AK_WITH_3MF_EXPORTER}>,1,0>)
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

# Target Start
add_library(${PROJECT_NAME} ${AK_BUILD} "")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
ak_target_c_options(${PROJECT_NAME})
ak_target_link_options(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PRIVATE AK_INTERNAL_BUILD)
ak_target_exporter_definitions(${PROJECT_NAME} PRIVATE)

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

if(AK_BUILD_DECODER_SHIMS
   AND (AK_BUILD_GLTF_DRACO_DECODER OR AK_BUILD_GLTF_MESHOPT_DECODER
        OR AK_BUILD_GLTF_SPZ_DECODER OR AK_BUILD_GLTF_KTX2_DECODER))
  enable_language(CXX)
endif()

if(AK_BUILD_DECODER_SHIMS AND AK_BUILD_GLTF_DRACO_DECODER)
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
    set(AK_DRACO_CMAKE_ARGS
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
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      list(APPEND AK_DRACO_CMAKE_ARGS
           -DCMAKE_CXX_FLAGS=-Wno-implicit-const-int-float-conversion)
    endif()
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
        CMAKE_ARGS ${AK_DRACO_CMAKE_ARGS})
  endif()

  if(NOT AK_DRACO_INCLUDE_DIR OR NOT AK_DRACO_LIBRARY)
    message(FATAL_ERROR
            "Draco decoder requested but Draco was not found. "
            "Set AK_DRACO_ROOT, enable AK_FETCH_DEPS, or disable "
            "AK_BUILD_GLTF_DRACO_DECODER.")
  endif()

  add_library(assetkit_draco SHARED
              src/decoders/gltf/draco/assetkit_draco.cc
              src/decoders/gltf/draco/assetkit_draco_bridge.c)
  ak_target_cxx_options(assetkit_draco)
  ak_target_link_options(assetkit_draco)
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

if(AK_BUILD_DECODER_SHIMS AND AK_BUILD_GLTF_MESHOPT_DECODER)
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
  ak_target_link_options(assetkit_meshoptimizer)
  target_include_directories(assetkit_meshoptimizer
      PRIVATE
          ${AK_MESHOPT_INCLUDE_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/include)
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

# ---------------------------------------------------------------------------
# Optional Gaussian splat (SPZ) decoder shim — KHR_gaussian_splatting future
# compression sub-extension support. Niantic Spatial's libspz handles the
# format; we link statically and expose the AssetKit decoder API.
# ---------------------------------------------------------------------------
if(AK_BUILD_DECODER_SHIMS AND AK_BUILD_GLTF_SPZ_DECODER)
  set(AK_SPZ_TARGET "")
  find_path(AK_SPZ_INCLUDE_DIR
            NAMES spz/load-spz.h load-spz.h
            HINTS ${AK_SPZ_ROOT}
                  ${AK_SPZ_ROOT}/install
                  ${AK_SPZ_ROOT}/include
                  ${AK_SPZ_ROOT}/src/cc
                  ${AK_DEPS_ROOT}/spz/install
                  ${AK_DEPS_ROOT}/spz
                  ${AK_DEPS_ROOT}/spz/include
                  ${AK_DEPS_ROOT}/spz/src/cc
            PATH_SUFFIXES include src/cc
            NO_DEFAULT_PATH)
  find_library(AK_SPZ_LIBRARY
               NAMES spz
               HINTS ${AK_SPZ_ROOT}
                     ${AK_SPZ_ROOT}/install
                     ${AK_SPZ_ROOT}/lib
                     ${AK_DEPS_ROOT}/spz/install
                     ${AK_DEPS_ROOT}/spz
                     ${AK_DEPS_ROOT}/spz/lib
               PATH_SUFFIXES lib
               NO_DEFAULT_PATH)

  if((NOT AK_SPZ_INCLUDE_DIR OR NOT AK_SPZ_LIBRARY) AND AK_FETCH_DEPS)
    include(FetchContent)

    set(AK_SPZ_SOURCE_DIR "${AK_DEPS_ROOT}/spz")
    if(EXISTS "${AK_SPZ_SOURCE_DIR}" AND NOT EXISTS "${AK_SPZ_SOURCE_DIR}/CMakeLists.txt")
      set(AK_SPZ_SOURCE_DIR "${AK_DEPS_ROOT}/spz-src")
    endif()
    set(SPZ_BUILD_TOOLS OFF CACHE BOOL "Build SPZ command-line tools" FORCE)
    set(SPZ_BUILD_PYTHON_BINDINGS OFF CACHE BOOL "Build SPZ Python bindings" FORCE)
    set(SPZ_BUILD_EXTENSIONS OFF CACHE BOOL "Build SPZ extensions support" FORCE)
    set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
    set(AK_SAVED_CMAKE_WARN_DEPRECATED "${CMAKE_WARN_DEPRECATED}")
    set(AK_SAVED_CMAKE_SUPPRESS_DEVELOPER_WARNINGS "${CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
    if(POLICY CMP0194)
      set(CMAKE_POLICY_DEFAULT_CMP0194 NEW)
    endif()
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Suppress third-party deprecation warnings" FORCE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "Suppress third-party developer warnings" FORCE)

    FetchContent_Declare(assetkit_spz_dep
        GIT_REPOSITORY https://github.com/nianticlabs/spz.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        GIT_SUBMODULES ""
        SOURCE_DIR "${AK_SPZ_SOURCE_DIR}")
    FetchContent_GetProperties(assetkit_spz_dep)
    if(NOT assetkit_spz_dep_POPULATED)
      FetchContent_Populate(assetkit_spz_dep)
      add_subdirectory("${assetkit_spz_dep_SOURCE_DIR}"
                       "${assetkit_spz_dep_BINARY_DIR}"
                       EXCLUDE_FROM_ALL)
    endif()
    set(CMAKE_WARN_DEPRECATED "${AK_SAVED_CMAKE_WARN_DEPRECATED}" CACHE BOOL "Suppress deprecated CMake warnings" FORCE)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS "${AK_SAVED_CMAKE_SUPPRESS_DEVELOPER_WARNINGS}" CACHE BOOL "Suppress developer warnings" FORCE)
    if(TARGET libzstd_static)
      target_compile_options(libzstd_static
          PRIVATE
              $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-uninitialized>
              $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-maybe-uninitialized>)
    endif()
    set(AK_SPZ_INCLUDE_DIR "${assetkit_spz_dep_SOURCE_DIR}/src/cc")
    set(AK_SPZ_TARGET spz::spz)
  endif()

  if(NOT AK_SPZ_INCLUDE_DIR OR (NOT AK_SPZ_LIBRARY AND NOT AK_SPZ_TARGET))
    message(WARNING
            "SPZ Gaussian-splat decoder requested but libspz was not found. "
            "Set AK_SPZ_ROOT, enable AK_FETCH_DEPS, or disable "
            "AK_BUILD_GLTF_SPZ_DECODER. Skipping decoder build for now.")
  else()
    add_library(assetkit_spz SHARED
                src/decoders/gltf/spz/assetkit_spz.cc)
    ak_target_cxx_options(assetkit_spz)
    ak_target_link_options(assetkit_spz)
    if(TARGET assetkit_spz_dep)
      add_dependencies(assetkit_spz assetkit_spz_dep)
    endif()
    target_include_directories(assetkit_spz
        SYSTEM PRIVATE
            ${AK_SPZ_INCLUDE_DIR}
            ${AK_DEPS_ROOT}/spz/src/cc)
    target_include_directories(assetkit_spz
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${PROJECT_SOURCE_DIR}/deps/cglm/include
            ${PROJECT_SOURCE_DIR}/deps/ds/include
            ${PROJECT_SOURCE_DIR}/deps/json/include)
    target_link_libraries(assetkit_spz
        PRIVATE
            ${PROJECT_NAME}
            ds)

    if(AK_SPZ_TARGET)
      target_link_libraries(assetkit_spz PRIVATE ${AK_SPZ_TARGET})
    else()
      # SPZ depends on zstd + zlib (for ngsp/zstd-compressed streams + the gzip
      # codec). find_package handles missing libraries gracefully when using a
      # prebuilt SPZ library.
      find_package(ZLIB)
      find_library(AK_SPZ_ZSTD_LIB
                   NAMES zstd
                   HINTS /opt/homebrew/lib /usr/local/lib /usr/lib)

      target_link_libraries(assetkit_spz PRIVATE ${AK_SPZ_LIBRARY})
      if(ZLIB_FOUND)
        target_link_libraries(assetkit_spz PRIVATE ZLIB::ZLIB)
      endif()
      if(AK_SPZ_ZSTD_LIB)
        target_link_libraries(assetkit_spz PRIVATE ${AK_SPZ_ZSTD_LIB})
      else()
        message(WARNING
                "libzstd not found — SPZ ngsp/zstd-compressed splat streams "
                "will fail at runtime. Install via 'brew install zstd' on "
                "macOS or your distro's package manager.")
      endif()
    endif()
    set_target_properties(assetkit_spz PROPERTIES
                          CXX_STANDARD 17
                          CXX_STANDARD_REQUIRED YES
                          CXX_VISIBILITY_PRESET hidden
                          VISIBILITY_INLINES_HIDDEN YES)
    if(AK_INSTALL_RPATH)
      set_target_properties(assetkit_spz PROPERTIES
                            INSTALL_RPATH "${AK_INSTALL_RPATH}")
    endif()

    install(TARGETS assetkit_spz
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
endif()

# ---------------------------------------------------------------------------
# Optional KTX2 / BasisU texture decoder shim — KHR_texture_basisu support.
# Khronos KTX-Software provides libktx with built-in BasisU transcoding
# (UASTC / ETC1S → RGBA8 / BC7 / ASTC depending on target). The shim
# decodes a KTX2 buffer into an RGBA8 AkImage so the renderer can consume
# it like any other Core Graphics-decoded texture.
# ---------------------------------------------------------------------------
if(AK_BUILD_DECODER_SHIMS AND AK_BUILD_GLTF_KTX2_DECODER)
  find_path(AK_KTX2_INCLUDE_DIR
            NAMES ktx.h
            HINTS ${AK_KTX2_ROOT}
                  ${AK_KTX2_ROOT}/install
                  ${AK_KTX2_ROOT}/include
                  ${AK_DEPS_ROOT}/ktx/install
                  ${AK_DEPS_ROOT}/ktx
                  ${AK_DEPS_ROOT}/ktx/include
            PATH_SUFFIXES include
            NO_DEFAULT_PATH)
  find_library(AK_KTX2_LIBRARY
               NAMES ktx ktx_read
               HINTS ${AK_KTX2_ROOT}
                     ${AK_KTX2_ROOT}/install
                     ${AK_KTX2_ROOT}/lib
                     ${AK_DEPS_ROOT}/ktx/install
                     ${AK_DEPS_ROOT}/ktx
                     ${AK_DEPS_ROOT}/ktx/lib
               PATH_SUFFIXES lib
               NO_DEFAULT_PATH)

  if((NOT AK_KTX2_INCLUDE_DIR OR NOT AK_KTX2_LIBRARY) AND AK_FETCH_DEPS)
    include(ExternalProject)

    set(AK_KTX2_INSTALL_DIR "${AK_DEPS_ROOT}/ktx/install")
    set(AK_KTX2_INCLUDE_DIR "${AK_KTX2_INSTALL_DIR}/include")
    set(AK_KTX2_CMAKE_ARGS
        -Wno-dev
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DKTX_FEATURE_TESTS=OFF
        -DKTX_FEATURE_TOOLS=OFF
        -DKTX_FEATURE_DOC=OFF
        -DKTX_FEATURE_LOADTEST_APPS=OFF
        -DBASISU_SSE=ON
        -DBUILD_SHARED_LIBS=OFF)
    if(MSVC)
      list(APPEND AK_KTX2_CMAKE_ARGS
           -DCMAKE_SHARED_LINKER_FLAGS=/ignore:4006
           -DCMAKE_MODULE_LINKER_FLAGS=/ignore:4006
           -DCMAKE_STATIC_LINKER_FLAGS=/ignore:4006
           -DCMAKE_EXE_LINKER_FLAGS=/ignore:4006)
    endif()
    if(WIN32)
      set(AK_KTX2_LIBRARY "${AK_KTX2_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/ktx.lib")
    else()
      set(AK_KTX2_LIBRARY "${AK_KTX2_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/libktx.a")
    endif()

    ExternalProject_Add(assetkit_ktx2_dep
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG main
        SOURCE_DIR "${AK_DEPS_ROOT}/ktx"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/assetkit_ktx2-build"
        INSTALL_DIR "${AK_KTX2_INSTALL_DIR}"
        BUILD_BYPRODUCTS "${AK_KTX2_LIBRARY}"
        CMAKE_ARGS ${AK_KTX2_CMAKE_ARGS})
  endif()

  if(NOT AK_KTX2_INCLUDE_DIR OR NOT AK_KTX2_LIBRARY)
    message(WARNING
            "KTX2/BasisU decoder requested but libktx was not found. "
            "Set AK_KTX2_ROOT, enable AK_FETCH_DEPS, or disable "
            "AK_BUILD_GLTF_KTX2_DECODER. Skipping decoder build for now.")
  else()
    add_library(assetkit_ktx2 SHARED
                src/decoders/gltf/ktx2/assetkit_ktx2.cc)
    ak_target_cxx_options(assetkit_ktx2)
    ak_target_link_options(assetkit_ktx2)
    if(TARGET assetkit_ktx2_dep)
      add_dependencies(assetkit_ktx2 assetkit_ktx2_dep)
    endif()
    target_include_directories(assetkit_ktx2
        SYSTEM PRIVATE
            ${AK_KTX2_INCLUDE_DIR})
    target_include_directories(assetkit_ktx2
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${PROJECT_SOURCE_DIR}/deps/cglm/include
            ${PROJECT_SOURCE_DIR}/deps/ds/include
            ${PROJECT_SOURCE_DIR}/deps/json/include)
    target_compile_definitions(assetkit_ktx2
        PRIVATE
            KHRONOS_STATIC)
    target_link_libraries(assetkit_ktx2
        PRIVATE
            ${PROJECT_NAME}
            ds
            ${AK_KTX2_LIBRARY})
    set_target_properties(assetkit_ktx2 PROPERTIES
                          CXX_STANDARD 17
                          CXX_STANDARD_REQUIRED YES
                          CXX_VISIBILITY_PRESET hidden
                          VISIBILITY_INLINES_HIDDEN YES)
    if(AK_INSTALL_RPATH)
      set_target_properties(assetkit_ktx2 PROPERTIES
                            INSTALL_RPATH "${AK_INSTALL_RPATH}")
    endif()

    install(TARGETS assetkit_ktx2
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

if(NOT CPack_CMake_INCLUDED)
  include(CPack)
endif()

# Test Configuration
if(AK_USE_TEST)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

if(AK_BUILD_TOOLS)
  add_executable(assetkit_import_stats tools/import_stats.c)
  ak_target_c_options(assetkit_import_stats)
  ak_target_link_options(assetkit_import_stats)
  target_link_libraries(assetkit_import_stats PRIVATE ${PROJECT_NAME})
endif()

if(AK_BUILD_SAMPLES)
  add_subdirectory(samples)
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
