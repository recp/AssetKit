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
ak_dae_visualScene(AkHeap * __restrict heap,
                   void * __restrict memParent,
                   xmlTextReaderPtr reader,
                   AkVisualScene ** __restrict dest) {
  AkVisualScene   *visualScene;
  AkNode          *last_node;
  AkEvaluateScene *last_evaluateScene;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  visualScene = ak_heap_calloc(heap, memParent, sizeof(*visualScene), true);

  _xml_readId(visualScene);
  _xml_readAttr(visualScene, visualScene->name, _s_dae_name);

  last_node          = NULL;
  last_evaluateScene = NULL;

  do {
    _xml_beginElement(_s_dae_visual_scene);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(heap, visualScene, reader, &assetInf);
      if (ret == AK_OK)
        visualScene->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_node)) {
      AkNode  *node;
      AkResult ret;

      ret = ak_dae_node(heap, visualScene, reader, &node);
      if (ret == AK_OK) {
        if (last_node)
          last_node->next = node;
        else
          visualScene->node = node;

        last_node = node;

        if (!visualScene->firstCamNode && node->camera)
          visualScene->firstCamNode = node;
      }
    } else if (_xml_eqElm(_s_dae_evaluate_scene)) {
      AkEvaluateScene *evaluateScene;
      AkResult ret;

      ret = ak_dae_evaluateScene(heap, visualScene, reader, &evaluateScene);
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

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, visualScene, nodePtr, &tree, NULL);
      visualScene->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = visualScene;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_instanceVisualScene(AkHeap * __restrict heap,
                           void * __restrict memParent,
                           xmlTextReaderPtr reader,
                           AkInstanceVisualScene ** __restrict dest) {
  AkInstanceVisualScene *visualScene;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  visualScene = ak_heap_calloc(heap, memParent, sizeof(*visualScene), false);

  _xml_readAttr(visualScene, visualScene->base.sid, _s_dae_sid);
  _xml_readAttr(visualScene, visualScene->base.name, _s_dae_name);
  _xml_readAttr(visualScene, visualScene->base.url, _s_dae_url);

  do {
    _xml_beginElement(_s_dae_instance_visual_scene);

    if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, visualScene, nodePtr, &tree, NULL);
      visualScene->base.extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = visualScene;
  
  return AK_OK;
}
