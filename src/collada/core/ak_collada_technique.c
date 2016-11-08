/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_technique.h"
#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_technique(AkDaeState * __restrict daestate,
                 void * __restrict memParent,
                 AkTechnique ** __restrict dest) {

  AkTechnique *technique;
  AkTree      *tree;
  xmlNodePtr   nodePtr;

  technique = ak_heap_calloc(daestate->heap,
                             memParent,
                             sizeof(*technique),
                             false);

  _xml_readAttr(technique, technique->profile, _s_dae_profile);
  _xml_readAttr(technique, technique->xmlns, _s_dae_xmlns);

  nodePtr = xmlTextReaderExpand(daestate->reader);
  tree = NULL;

  ak_tree_fromXmlNode(daestate->heap, technique, nodePtr, &tree, NULL);
  technique->chld = tree;

  daestate->nodeName = xmlTextReaderConstName(daestate->reader);
  daestate->nodeType = xmlTextReaderNodeType(daestate->reader);
  
  _xml_skipElement;

  *dest = technique;

  return AK_OK;
}
