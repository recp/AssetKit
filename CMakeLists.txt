cmake_minimum_required(VERSION 3.10)
project(assetio C)

set(CMAKE_C_STANDARD 99)

# TODO: use properties instead of properties
set(CMAKE_C_FLAGS "-Wall -Wextra -std=gnu99 -Wstrict-aliasing=2 -fstrict-aliasing -pedantic -Wmissing-declarations -Wno-overlength-strings")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

include_directories(
        PUBLIC

        ${PROJECT_SOURCE_DIR}/include

        PRIVATE

        ${PROJECT_SOURCE_DIR}/lib/jemalloc/include
        ${PROJECT_SOURCE_DIR}/lib/cglm/include
        ${PROJECT_SOURCE_DIR}/lib/curl/include
        ${PROJECT_SOURCE_DIR}/lib/libuv/include
        ${PROJECT_SOURCE_DIR}/lib/libds/include
        ${PROJECT_SOURCE_DIR}/lib/stb
        ${PROJECT_SOURCE_DIR}/lib/jansson/src
)

link_directories(
        PRIVATE

        ${PROJECT_SOURCE_DIR}/lib/jemalloc/lib
        ${PROJECT_SOURCE_DIR}/lib/curl/lib/.libs
        ${PROJECT_SOURCE_DIR}/lib/libuv/.libs
        ${PROJECT_SOURCE_DIR}/lib/libds/.libs
)

add_library(assetkit STATIC
        src/array/source_array.c
        src/bbox/bbox.c
        src/bbox/bbox.h)

target_link_libraries(assetkit -lxml2 -ljemalloc -lcurl -luv -lds -ljansson)

#src/win32/strptime.c
# src/dllmain.c
