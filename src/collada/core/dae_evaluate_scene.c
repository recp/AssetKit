/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_evaluate_scene.h"
#include "dae_render.h"
#include "../core/dae_asset.h"

AkResult _assetkit_hide
ak_dae_evaluateScene(AkXmlState * __restrict xst,
                     void * __restrict memParent,
                     AkEvaluateScene ** __restrict dest) {
  AkEvaluateScene *evaluateScene;
  AkRender        *last_render;
  AkXmlElmState    xest;

  evaluateScene = ak_heap_calloc(xst->heap,
                                 memParent,
                                 sizeof(*evaluateScene));

  ak_xml_readid(xst, evaluateScene);
  ak_xml_readsid(xst, evaluateScene);

  evaluateScene->name   = ak_xml_attr(xst, evaluateScene, _s_dae_name);
  evaluateScene->enable = ak_xml_attrui_def(xst, _s_dae_enable, true);

  last_render = NULL;

  ak_xest_init(xest, _s_dae_evaluate_scene)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)ak_dae_assetInf(xst, evaluateScene, NULL);
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

      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = evaluateScene;
  
  return AK_OK;
}
