/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "scene.h"
#include "node.h"
#include "evalscn.h"
#include "../core/asset.h"
#include "../../../include/ak/light.h"

void* _assetkit_hide
dae_vscene(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp) {
  AkHeap        *heap;
  AkVisualScene *vscn;

  heap = dst->heap;
  vscn = ak_heap_calloc(heap, memp, sizeof(*vscn));

  ak_setypeid(vscn, AKT_SCENE);
  xmla_setid(xml, heap, vscn);

  vscn->name    = xmla_strdup_by(xml, heap, _s_dae_name, vscn);
  vscn->cameras = ak_heap_calloc(heap, vscn, sizeof(*vscn->cameras));
  vscn->lights  = ak_heap_calloc(heap, vscn, sizeof(*vscn->lights));

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, vscn, NULL);
    } else if (xml_tag_eq(xml, _s_dae_node)) {
      AkNode *node;

      if ((node = dae_node(dst, xml, vscn, vscn))) {
        node->next = vscn->node;
        vscn->node = node;
        
        if (vscn->node) {
          vscn->node->prev = node;
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_evaluate_scene)) {
      AkEvaluateScene *evalScene;

      if ((evalScene = dae_evalScene(dst, xml, vscn))) {
        vscn->evaluateScene = evalScene;
        evalScene->next     = vscn->evaluateScene;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      vscn->extra = tree_fromxml(heap, vscn, xml);
    }
    xml = xml->next;
  }

  if (vscn->lights->count < 1
      && ak_opt_get(AK_OPT_ADD_DEFAULT_LIGHT)) {
    AkLight *light;
    AkNode  *rootNode;
    rootNode = vscn->node;
    if (rootNode) {
      AkHeap         *heap;
      AkDoc          *doc;
      AkInstanceBase *lightInst;

      heap  = ak_heap_getheap(rootNode);
      doc   = ak_heap_data(heap);
      light = ak_defaultLight(rootNode);

      lightInst = ak_instanceMake(heap, rootNode, light);
      ak_instanceListEmpty(vscn->lights);
      ak_instanceListAdd(vscn->lights, lightInst);

      lightInst->next = rootNode->light;
      rootNode->light = lightInst;

      ak_libAddLight(doc, light);
    }
  }

  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
    ak_fixSceneCoordSys(vscn);

  return vscn;
}

AkInstanceBase* _assetkit_hide
dae_instVisualScene(DAEState * __restrict dst,
                    xml_t    * __restrict xml,
                    void     * __restrict memp) {
  AkHeap         *heap;
  AkInstanceBase *visualScene;
  
  heap        = dst->heap;
  visualScene = ak_heap_calloc(heap, memp, sizeof(*visualScene));

  sid_set(xml, heap, visualScene);
  visualScene->name = xmla_strdup_by(xml, heap, _s_dae_name, visualScene);

  url_set(dst, xml, _s_dae_url, visualScene, &visualScene->url);

  return visualScene;
}

void _assetkit_hide
dae_scene(DAEState * __restrict dst,
          xml_t    * __restrict xml) {
  AkDoc    *doc;
  AkHeap   *heap;

  heap = dst->heap;
  doc  = dst->doc;
  xml  = xml->val;

  while (xml) {
    if (xml_tag_eq(xml, _s_dae_instance_visual_scene)) {
      doc->scene.visualScene = dae_instVisualScene(dst, xml, doc);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      doc->scene.extra = tree_fromxml(heap, doc, xml);
    }
    xml = xml->next;
  }
}
