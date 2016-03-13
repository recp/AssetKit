/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_technique.h"
#include "../aio_libxml.h"
#include "../aio_common.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include "aio_collada_common.h"

int _assetio_hide
aio_dae_technique(void * __restrict memParent,
                  xmlTextReaderPtr __restrict reader,
                  aio_technique ** __restrict dest) {

  aio_technique *technique;
  aio_tree      *tree;
  xmlNodePtr     nodePtr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  technique = aio_calloc(memParent, sizeof(*technique), 1);

  _xml_readAttr(technique, technique->profile, _s_dae_profile);
  _xml_readAttr(technique, technique->xmlns, _s_dae_xmlns);

  nodePtr = xmlTextReaderExpand(reader);
  tree = NULL;

  aio_tree_fromXmlNode(technique, nodePtr, &tree, NULL);
  technique->chld = tree;

  nodeName = xmlTextReaderConstName(reader);
  nodeType = xmlTextReaderNodeType(reader);
  
  _xml_skipElement;

  *dest = technique;

  return 0;
}
