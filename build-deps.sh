#! /bin/sh
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

# check if deps are pulled
git submodule update --init --recursive

cd $(dirname "$0")

if [ "$(uname)" = "Darwin" ]; then
  LIBTOOLIZE='glibtoolize'
else
  LIBTOOLIZE='libtoolize'
fi

# general deps: gcc make autoconf automake libtool cmake

# libds
cd ./lib/libds
sh ./autogen.sh
./configure
make -j8

cd ../../
