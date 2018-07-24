/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_common.h"

_assetkit_hide
void
dae_extra(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          AkTree    ** __restrict dest) {
  xmlNodePtr nodePtr;
  AkTree    *tree;

  nodePtr = xmlTextReaderExpand(xst->reader);
  tree    = NULL;

  ak_tree_fromXmlNode(xst->heap, memParent, nodePtr, &tree, NULL);

  *dest   = tree;

  ak_xml_skipelm(xst);
}
