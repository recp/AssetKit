/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_scene.h"
#include "dae_visual_scene.h"

AkResult _assetkit_hide
dae_scene(AkXmlState * __restrict xst,
          void * __restrict memParent,
          AkScene * __restrict dest) {
  AkXmlElmState xest;

  ak_xest_init(xest, _s_dae_scene)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_instance_visual_scene)) {
      AkInstanceBase *visualScene;
      AkResult ret;

      ret = dae_instanceVisualScene(xst, memParent, &visualScene);
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
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  return AK_OK;
}
