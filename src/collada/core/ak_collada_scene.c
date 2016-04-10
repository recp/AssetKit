/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_scene.h"
#include "ak_collada_visual_scene.h"

AkResult _assetkit_hide
ak_dae_scene(void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkScene * __restrict dest) {
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  do {
    _xml_beginElement(_s_dae_scene);

    if (_xml_eqElm(_s_dae_instance_visual_scene)) {
      AkInstanceVisualScene *visualScene;
      AkResult ret;

      ret = ak_dae_instanceVisualScene(memParent, reader, &visualScene);
      if (ret == AK_OK)
        dest->visualScene = visualScene;

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(memParent, nodePtr, &tree, NULL);
      dest->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  return AK_OK;
}
