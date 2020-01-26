#!/usr/bin/python
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

import json
from collections import OrderedDict
from os.path     import realpath
from os.path     import dirname

headerContents = [ ]
destdir        = dirname(realpath(__file__))
spidx          = 0
pos            = 0

fspoolJson = open(destdir + "/dae_strpool.json")
spool      = json.loads(fspoolJson.read(),
                        object_pairs_hook=OrderedDict)
fspoolJson.close()

fspool_h = open(destdir + "/dae_strpool.h", "wb");
fspool_c = open(destdir + "/dae_strpool.c", "wb");

copyright_str = """\
/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */
"""

fspool_h.write(copyright_str)
fspool_c.write(copyright_str)

fspool_h.write("""
#ifndef dae_strpool_h
#  define dae_strpool_h

#ifndef _DAE_STRPOOL_
#  define _AK_EXTERN extern
#else
#  define _AK_EXTERN
#endif
""")

fspool_c.write("""
#ifndef _DAE_STRPOOL_
#  define _DAE_STRPOOL_
#endif

#include "strpool.h"
#include <string.h>

const char _s_dae_pool_0[] =
""")

headerContents.append("\n/* _s_dae_pool_0 */\n")

for name, val in spool.iteritems():
  valLen = len(val) + 1

  # string literal size: 2048
  if pos + valLen > 2048:
    pos    = 0
    spidx += 1

    fspool_c.write(";\n\nconst char _s_dae_pool_{0}[] =\n"
                     .format(str(spidx)))

    headerContents.append("\n/* _s_dae_pool_{0} */\n"
                            .format(spidx))

  fspool_c.write("\"{0}\\0\"\n"
                   .format(val))

  headerContents.append("#define _s_dae_{0} _s_dae_{1}({2})\n"
                          .format(name, str(spidx), str(pos)))

  pos += valLen

# source file, then close it
fspool_c.write(";\n\n#undef _DAE_STRPOOL_\n")
fspool_c.close()

# header file
for idx in range(spidx + 1):
  fspool_h.write("\n_AK_EXTERN const char _s_dae_pool_{0}[];"
                   .format(str(idx)))

fspool_h.write("\n\n")

for idx in range(spidx + 1):
  fspool_h.write("#define _s_dae_{0}(x) (_s_dae_pool_{0} + x)\n"
                   .format(str(idx)))

# write header contents, then close it
fspool_h.writelines(headerContents)
fspool_h.write("\n#endif /* dae_strpool_h */\n")
fspool_h.close()

# try free array
del headerContents[:]
