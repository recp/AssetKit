/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_evaluate_scene.h"
#include "ak_collada_render.h"
#include "../core/ak_collada_asset.h"

AkResult _assetkit_hide
ak_dae_evaluateScene(AkDaeState * __restrict daestate,
                     void * __restrict memParent,
                     AkEvaluateScene ** __restrict dest) {
  AkEvaluateScene *evaluateScene;
  AkRender        *last_render;

  evaluateScene = ak_heap_calloc(daestate->heap,
                                 memParent,
                                 sizeof(*evaluateScene),
                                 true);

  _xml_readId(evaluateScene);
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
      ret = ak_dae_assetInf(daestate, evaluateScene, &assetInf);
      if (ret == AK_OK)
        evaluateScene->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_render)) {
      AkRender *render;
      AkResult  ret;

      ret = ak_dae_render(daestate, evaluateScene, &render);
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

      nodePtr = xmlTextReaderExpand(daestate->reader);
      tree = NULL;

      ak_tree_fromXmlNode(daestate->heap,
                          evaluateScene,
                          nodePtr,
                          &tree,
                          NULL);
      evaluateScene->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);
  
  *dest = evaluateScene;
  
  return AK_OK;
}
