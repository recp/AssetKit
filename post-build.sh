#! /bin/sh
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

cd $(dirname "$0")

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

mkdir -p .libs

#TODO: implement this to other platforms e.g. linux, windows
if [ "$(uname)" = "Darwin" ]; then
  ak_dylib=$(realpath .libs/libassetkit.dylib)

  libxml2_dylib=$(readlink    ./lib/libxml2/.libs/libxml2.dylib)
  jemalloc_dylib=$(readlink   ./lib/jemalloc/lib/libjemalloc.dylib)
  curl_dylib=$(readlink       ./lib/curl/lib/.libs/libcurl.dylib)
  libuv_dylib=$(readlink      ./lib/libuv/.libs/libuv.dylib)
  libds_dylib=$(readlink      ./lib/libds/.libs/libds.dylib)

  cp ./lib/libxml2/.libs/$libxml2_dylib .libs/$libxml2_dylib
  cp ./lib/jemalloc/lib/$jemalloc_dylib .libs/$jemalloc_dylib
  cp ./lib/curl/lib/.libs/$curl_dylib   .libs/$curl_dylib
  cp ./lib/libuv/.libs/$libuv_dylib     .libs/$libuv_dylib
  cp ./lib/libds/.libs/$libds_dylib     .libs/$libds_dylib

  install_name_tool -change /usr/local/lib/$libxml2_dylib \
                            @loader_path/$libxml2_dylib \
                            $ak_dylib
  install_name_tool -change /usr/local/lib/$jemalloc_dylib \
                            @loader_path/$jemalloc_dylib \
                            $ak_dylib
  install_name_tool -change /usr/local/lib/$curl_dylib \
                            @loader_path/$curl_dylib \
                            $ak_dylib
  install_name_tool -change /usr/local/lib/$libuv_dylib \
                            @loader_path/$libuv_dylib \
                            $ak_dylib
  install_name_tool -change /usr/local/lib/$libds_dylib \
                            @loader_path/$libds_dylib \
                            $ak_dylib
else
  libxml2_so=$(readlink    ./lib/libxml2/.libs/libxml2.so)
  jemalloc_so=$(readlink   ./lib/jemalloc/lib/libjemalloc.so)
  curl_so=$(readlink       ./lib/curl/lib/.libs/libcurl.so)
  libuv_so=$(readlink      ./lib/libuv/.libs/libuv.so)

  cp ./lib/libxml2/.libs/$libxml2_so        .libs/$libxml2_so
  cp ./lib/jemalloc/lib/$jemalloc_so        .libs/$jemalloc_so
  cp ./lib/curl/lib/.libs/$curl_so          .libs/$curl_so
  cp ./lib/libuv/.libs/$libuv_so            .libs/$libuv_so
fi
