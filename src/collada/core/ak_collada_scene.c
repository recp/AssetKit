/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_scene.h"
#include "ak_collada_visual_scene.h"

AkResult _assetkit_hide
ak_dae_scene(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkScene * __restrict dest) {

  do {
    if (ak_xml_beginelm(xst, _s_dae_scene))
      break;

    if (ak_xml_eqelm(xst, _s_dae_instance_visual_scene)) {
      AkInstanceVisualScene *visualScene;
      AkResult ret;

      ret = ak_dae_instanceVisualScene(xst, memParent, &visualScene);
      if (ret == AK_OK)
        dest->visualScene = visualScene;

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          memParent,
                          nodePtr,
                          &tree,
                          NULL);
      dest->extra = tree;

      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  return AK_OK;
}
