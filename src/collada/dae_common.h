/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_common__h_
#define __libassetkit__dae_common__h_

#include "../../include/ak/assetkit.h"
#include "../xml.h"
#include "../common.h"
#include "../memory_common.h"
#include "../utils.h"
#include "../tree.h"
#include "dae_strpool.h"

#include <libxml/xmlreader.h>
#include <string.h>

#ifndef AK_INPUT_SEMANTIC_VERTEX
#  define AK_INPUT_SEMANTIC_VERTEX 100001
#endif

typedef struct AkDaeMeshInfo {
  AkInput *pos;
  size_t   nVertex;
} AkDaeMeshInfo;

_assetkit_hide
void
dae_extra(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          AkTree    ** __restrict dest);

#endif /* __libassetkit__dae_common__h_ */
