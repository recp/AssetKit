/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_visual_scene.h"
#include "../ak_collada_asset.h"
#include "ak_collada_node.h"
#include "ak_collada_evaluate_scene.h"

AkResult _assetkit_hide
ak_dae_visualScene(void * __restrict memParent,
                   xmlTextReaderPtr reader,
                   AkVisualScene ** __restrict dest) {
  AkVisualScene   *visualScene;
  AkNode          *last_node;
  AkEvaluateScene *last_evaluateScene;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  visualScene = ak_calloc(memParent, sizeof(*visualScene), 1);

  _xml_readAttr(visualScene, visualScene->id, _s_dae_id);
  _xml_readAttr(visualScene, visualScene->name, _s_dae_name);

  last_node = NULL;
  last_evaluateScene = NULL;

  do {
    _xml_beginElement(_s_dae_visual_scene);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(visualScene, reader, &assetInf);
      if (ret == AK_OK)
        visualScene->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_node)) {
      AkNode  *node;
      AkResult ret;

      ret = ak_dae_node(visualScene, reader, &node);
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

      ret = ak_dae_evaluateScene(visualScene, reader, &evaluateScene);
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

      ak_tree_fromXmlNode(visualScene, nodePtr, &tree, NULL);
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
ak_dae_instanceVisualScene(void * __restrict memParent,
                           xmlTextReaderPtr reader,
                           AkInstanceVisualScene ** __restrict dest) {
  AkInstanceVisualScene *visualScene;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  visualScene = ak_calloc(memParent, sizeof(*visualScene), 1);

  _xml_readAttr(visualScene, visualScene->sid, _s_dae_sid);
  _xml_readAttr(visualScene, visualScene->name, _s_dae_name);
  _xml_readAttr(visualScene, visualScene->url, _s_dae_url);

  do {
    _xml_beginElement(_s_dae_instance_visual_scene);

    if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(visualScene, nodePtr, &tree, NULL);
      visualScene->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = visualScene;
  
  return AK_OK;
}
