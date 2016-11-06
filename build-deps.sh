#! /bin/sh
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

# libxml2
sh ./lib/libxml2/autogen.sh
./lib/libxml2/configure
./lib/libxml2/make

# jemalloc
sh ./lib/jemalloc/autogen.sh
./lib/jemalloc/configure
./lib/jemalloc/make
