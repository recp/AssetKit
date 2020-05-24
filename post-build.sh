#! /bin/sh
#
# Copyright (C) 2020 Recep Aslantas
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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
