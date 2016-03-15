/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_material.h"
#include "../aio_collada_common.h"
#include "../aio_collada_asset.h"
#include "aio_collada_fx_effect.h"

int _assetio_hide
aio_dae_material(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 aio_material ** __restrict dest) {
  aio_material  *material;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  material = aio_calloc(memParent, sizeof(*material), 1);

  _xml_readAttr(material, material->id, _s_dae_id);
  _xml_readAttr(material, material->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_material);

    if (_xml_eqElm(_s_dae_asset)) {
      aio_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = aio_dae_assetInf(material, reader, &assetInf);
      if (ret == 0)
        material->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_inst_effect)) {
      aio_effect_instance *effectInstance;
      int ret;

      effectInstance = NULL;
      ret = aio_dae_fxEffectInstance(material, reader, &effectInstance);
      if (ret == 0)
        material->effect_inst = effectInstance;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      aio_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      aio_tree_fromXmlNode(material, nodePtr, &tree, NULL);
      material->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = material;
  
  return 0;
}
