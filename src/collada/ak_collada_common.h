/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_common__h_
#define __libassetkit__ak_collada_common__h_

#include "../../include/assetkit.h"
#include "../ak_libxml.h"
#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../ak_utils.h"
#include "../ak_tree.h"
#include "ak_collada_strpool.h"

#include <libxml/xmlreader.h>
#include <string.h>

typedef AK_ALIGN(16) struct AkDaeState {
  AkHeap          *heap;
  AkDoc           *doc;
  xmlTextReaderPtr reader;
  const xmlChar   *nodeName;
  int              nodeType;
  int              nodeRet;
} AkDaeState;

#endif /* __libassetkit__ak_collada_common__h_ */
