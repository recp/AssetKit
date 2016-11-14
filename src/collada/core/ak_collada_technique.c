/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_technique.h"
#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_technique(AkXmlState * __restrict xst,
                 void * __restrict memParent,
                 AkTechnique ** __restrict dest) {

  AkTechnique *technique;
  AkTree      *tree;
  xmlNodePtr   nodePtr;

  technique = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*technique));

  technique->profile = ak_xml_attr(xst, technique, _s_dae_profile);
  technique->xmlns   = ak_xml_attr(xst, technique, _s_dae_xmlns);

  nodePtr = xmlTextReaderExpand(xst->reader);
  tree = NULL;

  ak_tree_fromXmlNode(xst->heap, technique, nodePtr, &tree, NULL);
  technique->chld = tree;

  xst->nodeName = xmlTextReaderConstName(xst->reader);
  xst->nodeType = xmlTextReaderNodeType(xst->reader);
  
  ak_xml_skipelm(xst);

  *dest = technique;

  return AK_OK;
}
