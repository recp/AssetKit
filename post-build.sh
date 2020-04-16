#! /bin/sh
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

#cd $(dirname "$0")
#
#realpath() {
#    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
#}
#
#mkdir -p .libs
#
##TODO: implement this to other platforms e.g. linux, windows
#if [ "$(uname)" = "Darwin" ]; then
#  ak_dylib=$(realpath .libs/libassetkit.dylib)
#
#  libds_dylib=$(readlink ./lib/libds/.libs/libds.dylib)
#
#  cp ./lib/libds/.libs/$libds_dylib .libs/$libds_dylib
#
#  install_name_tool -change /usr/local/lib/$libds_dylib \
#                            @loader_path/$libds_dylib \
#                            $ak_dylib
#fi
