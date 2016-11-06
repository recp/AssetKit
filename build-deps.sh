#! /bin/sh
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

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
