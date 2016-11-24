/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_render.h"
#include "../../ak_array.h"
#include "../fx/ak_collada_fx_material.h"

AkResult _assetkit_hide
ak_dae_render(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkRender ** __restrict dest) {
  AkRender           *render;
  AkInstanceMaterial *last_instanceMaterial;
  AkStringArrayL     *last_layer;

  render = ak_heap_calloc(xst->heap, memParent, sizeof(*render));

  ak_xml_readsid(xst, render);

  render->name = ak_xml_attr(xst, render, _s_dae_name);
  render->cameraNode = ak_xml_attr(xst, render, _s_dae_camera_node);

  last_instanceMaterial = NULL;
  last_layer = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_render))
      break;

    if (ak_xml_eqelm(xst, _s_dae_layer)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkStringArrayL *layer;
        AkResult ret;

        ret = ak_strtostr_arrayL(xst->heap, render, content, ' ', &layer);
        if (ret == AK_OK) {
          if (last_layer)
            last_layer->next = layer;
          else
            render->layer = layer;

          last_layer = layer;
        }

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_instance_material)) {
      AkInstanceMaterial *instanceMaterial;
      AkResult ret;
      ret = ak_dae_fxInstanceMaterial(xst,
                                      memParent,
                                      &instanceMaterial);

      if (ret == AK_OK) {
        if (last_instanceMaterial)
          last_instanceMaterial->next = instanceMaterial;
        else
          render->instanceMaterial = instanceMaterial;

        last_instanceMaterial = instanceMaterial;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          render,
                          nodePtr,
                          &tree,
                          NULL);
      render->extra = tree;

      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  *dest = render;
  
  return AK_OK;
}
