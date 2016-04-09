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
ak_dae_render(void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkRender ** __restrict dest) {
  AkRender           *render;
  AkInstanceMaterial *last_instanceMaterial;
  AkStringArrayL     *last_layer;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  render = ak_calloc(memParent, sizeof(*render), 1);

  _xml_readAttr(render, render->sid, _s_dae_sid);
  _xml_readAttr(render, render->name, _s_dae_name);
  _xml_readAttr(render, render->cameraNode, _s_dae_camera_node);

  last_instanceMaterial = NULL;
  last_layer = NULL;

  do {
    _xml_beginElement(_s_dae_render);

    if (_xml_eqElm(_s_dae_layer)) {
      char *content;
      _xml_readMutText(content);

      if (content) {
        AkStringArrayL *layer;
        AkResult ret;

        ret = ak_strtostr_arrayL(render, content, ' ', &layer);
        if (ret == AK_OK) {
          if (last_layer)
            last_layer->next = layer;
          else
            render->layer = layer;

          last_layer = layer;
        }

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_instance_material)) {
      AkInstanceMaterial *instanceMaterial;
      AkResult ret;
      ret = ak_dae_fxInstanceMaterial(memParent, reader, &instanceMaterial);

      if (ret == AK_OK) {
        if (last_instanceMaterial)
          last_instanceMaterial->next = instanceMaterial;
        else
          render->instanceMaterial = instanceMaterial;

        last_instanceMaterial = instanceMaterial;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(render, nodePtr, &tree, NULL);
      render->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = render;
  
  return AK_OK;
}
