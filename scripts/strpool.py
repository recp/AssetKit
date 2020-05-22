#!/usr/bin/python
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
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
