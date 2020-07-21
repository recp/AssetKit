#!/usr/bin/python
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

import os
from os.path import realpath
from os.path import dirname

basedir = dirname(realpath(__file__)) + '/../'

strpools = [
  'src/io/dae/strpool.py',
  'src/io/gltf/strpool.py'
]

for sp in strpools:
  __file__ = basedir + sp
  exec(open(__file__).read())
