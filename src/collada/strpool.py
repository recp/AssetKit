#!/usr/bin/python
#
# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file
#

import json
from collections import OrderedDict
from inspect import getsourcefile
from os.path import realpath
from os.path import dirname

destdir = dirname(realpath(__file__))

fspoolJSON = open(destdir + "/ak_collada_strpool.json")
spool = json.loads(fspoolJSON.read(), object_pairs_hook=OrderedDict)
fspoolJSON.close()

fspool_h = open(destdir + "/ak_collada_strpool.h", "wb");
fspool_c = open(destdir + "/ak_collada_strpool.c", "wb");

copyright = """\
/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */
"""

fspool_h.write(copyright)
fspool_c.write(copyright)

fspool_h.write("""
#ifndef __libassetkit__collada_strpool__h_
#define __libassetkit__collada_strpool__h_

#ifndef _ak_DAE_STRPOOL_
extern
#endif
const char _s_dae_pool[];

#ifndef _s_dae
#define _s_dae(x) (_s_dae_pool + x)
#endif

""")

fspool_c.write("""
#ifndef _ak_DAE_STRPOOL_
#define _ak_DAE_STRPOOL_
#endif

#include "ak_collada_strpool.h"
#include <string.h>

const char _s_dae_pool[] =
""")

pos = 0
for name, val in spool.iteritems():
  fspool_c.write("\"")
  fspool_c.write(val)
  fspool_c.write("\\0\"\n")

  fspool_h.write("#define _s_dae_")
  fspool_h.write(name)
  fspool_h.write(" _s_dae(")
  fspool_h.write(str(pos))
  fspool_h.write(")\n")

  pos += len(val) + 1

fspool_h.write("""
#endif /* __libassetkit__collada_strpool__h_ */
""")

fspool_c.write(""";

#undef _ak_DAE_STRPOOL_
""")

fspool_h.close()
fspool_c.close()
