/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_material.h"
#include "../ak_collada_common.h"
#include "../ak_collada_asset.h"
#include "ak_collada_fx_effect.h"

AkResult _assetkit_hide
ak_dae_material(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 ak_material ** __restrict dest) {
  ak_material  *material;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  material = ak_calloc(memParent, sizeof(*material), 1);

  _xml_readAttr(material, material->id, _s_dae_id);
  _xml_readAttr(material, material->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_material);

    if (_xml_eqElm(_s_dae_asset)) {
      ak_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(material, reader, &assetInf);
      if (ret == AK_OK)
        material->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_inst_effect)) {
      ak_effect_instance *effectInstance;
      int ret;

      effectInstance = NULL;
      ret = ak_dae_fxEffectInstance(material, reader, &effectInstance);
      if (ret == AK_OK)
        material->effect_inst = effectInstance;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(material, nodePtr, &tree, NULL);
      material->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = material;
  
  return AK_OK;
}
