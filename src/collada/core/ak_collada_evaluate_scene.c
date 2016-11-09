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
ak_dae_evaluateScene(AkXmlState * __restrict xst,
                     void * __restrict memParent,
                     AkEvaluateScene ** __restrict dest) {
  AkEvaluateScene *evaluateScene;
  AkRender        *last_render;

  evaluateScene = ak_heap_calloc(xst->heap,
                                 memParent,
                                 sizeof(*evaluateScene),
                                 true);

  _xml_readId(evaluateScene);
  evaluateScene->name = ak_xml_attr(xst, evaluateScene, _s_dae_name);
  evaluateScene->sid  = ak_xml_attr(xst, evaluateScene, _s_dae_sid);
  _xml_readAttrUsingFnWithDef(evaluateScene->enable,
                              _s_dae_enable, true,
                              (AkBool)strtol, NULL, 10);

  last_render = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_evaluate_scene))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, evaluateScene, &assetInf);
      if (ret == AK_OK)
        evaluateScene->inf = assetInf;

    } else if (ak_xml_eqelm(xst, _s_dae_render)) {
      AkRender *render;
      AkResult  ret;

      ret = ak_dae_render(xst, evaluateScene, &render);
      if (ret == AK_OK) {
        if (last_render)
          last_render->next = render;
        else
          evaluateScene->render = render;

        last_render = render;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          evaluateScene,
                          nodePtr,
                          &tree,
                          NULL);
      evaluateScene->extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = evaluateScene;
  
  return AK_OK;
}
