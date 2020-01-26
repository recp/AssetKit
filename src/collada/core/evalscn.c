/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "evalscn.h"
#include "../core/asset.h"
#include "../../array.h"

AkEvaluateScene* _assetkit_hide
dae_evaluateScene(DAEState * __restrict dst,
                  xml_t    * __restrict xml,
                  void     * __restrict memParent) {
  AkEvaluateScene *evalScene;
  AkDoc           *doc;
  AkHeap          *heap;

  heap = dst->heap;
  doc  = dst->doc;
  xml  = xml->val;

  evalScene = ak_heap_calloc(heap, memParent, sizeof(*evalScene));
  xmla_setid(xml, heap, evalScene);
  sid_set(xml, heap, evalScene);
  
  evalScene->name   = xmla_strdup_by(xml, heap, _s_dae_name, evalScene);
  evalScene->enable = xmla_bool(xml_attr(xml, _s_dae_enable), 0);
  
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      
    } else if (xml_tag_eq(xml, _s_dae_render)) {
      AkRender *ren;
      xml_t    *xren;
      
      ren = ak_heap_calloc(heap, evalScene, sizeof(*ren));
      sid_set(xml, heap, ren);
      
      ren->name       = xmla_strdup_by(xml, heap, _s_dae_name, ren);
      ren->cameraNode = xmla_strdup_by(xml, heap, _s_dae_camera_node, ren);
      
      xren = xml->val;
      while (xren) {
        if (xml_tag_eq(xren, _s_dae_layer) && xren->val) {
          AkStringArrayL *layer;
          char           *contents;
          AkResult        ret;
          
          contents                = xren->val;
          contents[xren->valsize] = '\0';
          
          ret = ak_strtostr_arrayL(heap, ren, contents, ' ', &layer);
          if (ret == AK_OK) {
            layer->next = ren->layer;
            ren->layer  = layer;
          }
        } else if (xml_tag_eq(xren, _s_dae_instance_material)) {
          AkInstanceMaterial *instMaterial;
          AkResult            ret;

          ret = dae_fxInstanceMaterial(dst, xml, memParent, &instMaterial);
          if (ret == AK_OK) {
            instMaterial->base.next = &ren->instanceMaterial->base;
            ren->instanceMaterial   = instMaterial;
          }
        } else if (xml_tag_eq(xren, _s_dae_extra)) {
           ren->extra = tree_fromxml(heap, ren, xml);
        }

        xren = xren->next;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      evalScene->extra = tree_fromxml(heap, evalScene, xml);
    }
    xml = xml->next;
  }

  return evalScene;
}
