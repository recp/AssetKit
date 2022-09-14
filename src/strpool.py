#!/usr/bin/python3
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

import json
from collections import OrderedDict
from os.path     import realpath
from os.path     import dirname

headerContents = [ ]
destdir        = dirname(realpath(__file__))
spidx          = 0
pos            = 0

fspoolJson = open(destdir + "/strpool.json")
spool      = json.loads(fspoolJson.read(),
                        object_pairs_hook=OrderedDict)
fspoolJson.close()

fspool_h = open(destdir + "/strpool.h", "wb");
fspool_c = open(destdir + "/strpool.c", "wb");

copyright_str = """\
/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
"""

fspool_h.write(copyright_str.encode())
fspool_c.write(copyright_str.encode())

fspool_h.write("""
#ifndef ak_strpool_h
#  define ak_strpool_h

#ifndef AK_STRPOOL_
#  define _AK_EXTERN extern
#else
#  define _AK_EXTERN
#endif
""".encode())

fspool_c.write("""
#ifndef _AK_STRPOOL_
#  define _AK_STRPOOL_
#endif

#include "strpool.h"
#include <string.h>

const char _s_ak_pool_0[] =
""".encode())

headerContents.append("\n/* _s_ak_pool_0 */\n")

for name, val in spool.items():
  valLen = len(val) + 1

  # string literal size: 2048
  if pos + valLen > 2048:
    pos    = 0
    spidx += 1

    fspool_c.write(";\n\nconst char _s_ak_pool_{0}[] =\n"
                     .format(str(spidx)).encode())

    headerContents.append("\n/* _s_ak_pool_{0} */\n"
                            .format(spidx))

  fspool_c.write("\"{0}\\0\"\n".format(val).encode())

  headerContents.append("#define _s_{0} _s_ak_{1}({2})\n"
                          .format(name, str(spidx), str(pos)))

  pos += valLen

# source file, then close it
fspool_c.write(";\n\n#undef _AK_STRPOOL_\n".encode())
fspool_c.close()

# header file
for idx in range(spidx + 1):
  fspool_h.write("\n_AK_EXTERN const char _s_ak_pool_{0}[];"
                   .format(str(idx)).encode())

fspool_h.write("\n\n".encode())

for idx in range(spidx + 1):
  fspool_h.write("#define _s_ak_{0}(x) (_s_ak_pool_{0} + x)\n"
                   .format(str(idx)).encode())

# write header contents, then close it
fspool_h.writelines(map(lambda x: x.encode(), headerContents))
fspool_h.write("\n#endif /* ak_strpool_h */\n".encode())
fspool_h.close()

# try free array
del headerContents[:]

