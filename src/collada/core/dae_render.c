/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_render.h"
#include "../../ak_array.h"
#include "../fx/dae_fx_material.h"

AkResult _assetkit_hide
ak_dae_render(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkRender ** __restrict dest) {
  AkRender           *render;
  AkInstanceMaterial *last_instanceMaterial;
  AkStringArrayL     *last_layer;
  AkXmlElmState       xest;

  render = ak_heap_calloc(xst->heap, memParent, sizeof(*render));

  ak_xml_readsid(xst, render);

  render->name = ak_xml_attr(xst, render, _s_dae_name);
  render->cameraNode = ak_xml_attr(xst, render, _s_dae_camera_node);

  last_instanceMaterial = NULL;
  last_layer = NULL;

  ak_xest_init(xest, _s_dae_render)

  do {
    if (ak_xml_begin(&xest))
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
          last_instanceMaterial->base.next = &instanceMaterial->base;
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
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = render;
  
  return AK_OK;
}
