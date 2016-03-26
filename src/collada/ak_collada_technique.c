/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_technique.h"
#include "../ak_libxml.h"
#include "../ak_common.h"
#include "../ak_utils.h"
#include "../ak_tree.h"

#include "ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_technique(void * __restrict memParent,
                  xmlTextReaderPtr reader,
                  ak_technique ** __restrict dest) {

  ak_technique *technique;
  AkTree       *tree;
  xmlNodePtr     nodePtr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  technique = ak_calloc(memParent, sizeof(*technique), 1);

  _xml_readAttr(technique, technique->profile, _s_dae_profile);
  _xml_readAttr(technique, technique->xmlns, _s_dae_xmlns);

  nodePtr = xmlTextReaderExpand(reader);
  tree = NULL;

  ak_tree_fromXmlNode(technique, nodePtr, &tree, NULL);
  technique->chld = tree;

  nodeName = xmlTextReaderConstName(reader);
  nodeType = xmlTextReaderNodeType(reader);
  
  _xml_skipElement;

  *dest = technique;

  return AK_OK;
}
