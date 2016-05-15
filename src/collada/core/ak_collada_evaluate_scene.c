/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_evaluate_scene.h"
#include "ak_collada_render.h"
#include "../ak_collada_asset.h"

AkResult _assetkit_hide
ak_dae_evaluateScene(void * __restrict memParent,
                     xmlTextReaderPtr reader,
                     AkEvaluateScene ** __restrict dest) {
  AkEvaluateScene *evaluateScene;
  AkRender        *last_render;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  evaluateScene = ak_calloc(memParent, sizeof(*evaluateScene), false);

  _xml_readAttr(evaluateScene, evaluateScene->id, _s_dae_id);
  _xml_readAttr(evaluateScene, evaluateScene->name, _s_dae_name);
  _xml_readAttr(evaluateScene, evaluateScene->sid, _s_dae_sid);
  _xml_readAttrUsingFnWithDef(evaluateScene->enable,
                              _s_dae_enable, true,
                              (AkBool)strtol, NULL, 10);

  last_render = NULL;

  do {
    _xml_beginElement(_s_dae_evaluate_scene);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(evaluateScene, reader, &assetInf);
      if (ret == AK_OK)
        evaluateScene->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_render)) {
      AkRender *render;
      AkResult  ret;

      ret = ak_dae_render(evaluateScene, reader, &render);
      if (ret == AK_OK) {
        if (last_render)
          last_render->next = render;
        else
          evaluateScene->render = render;

        last_render = render;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(evaluateScene, nodePtr, &tree, NULL);
      evaluateScene->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = evaluateScene;
  
  return AK_OK;
}
