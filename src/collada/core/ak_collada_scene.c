/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_scene.h"
#include "ak_collada_visual_scene.h"

AkResult _assetkit_hide
ak_dae_scene(AkDaeState * __restrict daestate,
             void * __restrict memParent,
             AkScene * __restrict dest) {

  do {
    _xml_beginElement(_s_dae_scene);

    if (_xml_eqElm(_s_dae_instance_visual_scene)) {
      AkInstanceVisualScene *visualScene;
      AkResult ret;

      ret = ak_dae_instanceVisualScene(daestate, memParent, &visualScene);
      if (ret == AK_OK)
        dest->visualScene = visualScene;

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(daestate->reader);
      tree = NULL;

      ak_tree_fromXmlNode(daestate->heap,
                          memParent,
                          nodePtr,
                          &tree,
                          NULL);
      dest->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);
  
  return AK_OK;
}
