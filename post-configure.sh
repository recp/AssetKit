#! /bin/sh
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

cd `dirname "$0"`

#TODO: implement this to other platforms e.g. linux, windows
if [ "`uname`" = "Darwin" ]; then
  ak_dylib=.libs/libassetkit.dylib

  libxml2_dylib=$(readlink  ./lib/libxml2/.libs/libxml2.dylib)
  jemalloc_dylib=$(readlink ./lib/jemalloc/lib/libjemalloc.dylib)
  curl_dylib=$(readlink     ./lib/curl/lib/.libs/libcurl.dylib)
  libuv_dylib=$(readlink    ./lib/libuv/.libs/libuv.dylib)

  cp ./lib/libxml2/.libs/$libxml2_dylib .libs/$libxml2_dylib
  cp ./lib/jemalloc/lib/$jemalloc_dylib .libs/$jemalloc_dylib
  cp ./lib/curl/lib/.libs/$curl_dylib   .libs/$curl_dylib
  cp ./lib/libuv/.libs/$libuv_dylib     .libs/$libuv_dylib

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
fi
