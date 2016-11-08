/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_visual_scene.h"
#include "../core/ak_collada_asset.h"
#include "ak_collada_node.h"
#include "ak_collada_evaluate_scene.h"

AkResult _assetkit_hide
ak_dae_visualScene(AkXmlState * __restrict xst,
                   void * __restrict memParent,
                   AkVisualScene ** __restrict dest) {
  AkVisualScene   *visualScene;
  AkNode          *last_node;
  AkEvaluateScene *last_evaluateScene;

  visualScene = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(*visualScene),
                               true);

  _xml_readId(visualScene);
  _xml_readAttr(visualScene, visualScene->name, _s_dae_name);

  last_node          = NULL;
  last_evaluateScene = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_visual_scene))
      break;

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, visualScene, &assetInf);
      if (ret == AK_OK)
        visualScene->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_node)) {
      AkNode  *node;
      AkResult ret;

      ret = ak_dae_node(xst,
                        visualScene,
                        &visualScene->firstCamNode,
                        &node);
      if (ret == AK_OK) {
        if (last_node)
          last_node->next = node;
        else
          visualScene->node = node;

        last_node = node;
      }
    } else if (_xml_eqElm(_s_dae_evaluate_scene)) {
      AkEvaluateScene *evaluateScene;
      AkResult ret;

      ret = ak_dae_evaluateScene(xst, visualScene, &evaluateScene);
      if (ret == AK_OK) {
        if (last_evaluateScene)
          last_evaluateScene->next = evaluateScene;
        else
          visualScene->evaluateScene = evaluateScene;

        last_evaluateScene = evaluateScene;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          visualScene,
                          nodePtr,
                          &tree,
                          NULL);
      visualScene->extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = visualScene;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_instanceVisualScene(AkXmlState * __restrict xst,
                           void * __restrict memParent,
                           AkInstanceVisualScene ** __restrict dest) {
  AkInstanceVisualScene *visualScene;

  visualScene = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(*visualScene),
                               false);

  _xml_readAttr(visualScene, visualScene->base.sid, _s_dae_sid);
  _xml_readAttr(visualScene, visualScene->base.name, _s_dae_name);

  ak_url_from_attr(xst->reader,
                   _s_dae_url,
                   visualScene,
                   &visualScene->base.url);

  do {
    if (ak_xml_beginelm(xst, _s_dae_instance_visual_scene))
      break;

    if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          visualScene,
                          nodePtr,
                          &tree,
                          NULL);
      visualScene->base.extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = visualScene;
  
  return AK_OK;
}
