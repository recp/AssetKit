#! /bin/sh
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

# check if deps are pulled
git submodule update --init --recursive

# libxml2
cd ./lib/libxml2
sh ./autogen.sh
./configure
make

# jemalloc
cd ../jemalloc
sh ./autogen.sh
./configure
make

# curl
cd ../curl
sh ./buildconf
./configure
make

# libuv
cd ../libuv
sh ./autogen.sh
./configure
make

# test - cmocka
cd ../../test/lib/cmocka
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
make
cd ../../../../
