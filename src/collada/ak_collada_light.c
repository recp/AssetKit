/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_light.h"
#include "ak_collada_asset.h"
#include "ak_collada_technique.h"

AkResult _assetkit_hide
ak_dae_light(void * __restrict memParent,
              xmlTextReaderPtr reader,
              ak_light ** __restrict dest) {
  ak_light            *light;
  ak_technique        *last_tq;
  ak_technique_common *last_tc;
  const xmlChar        *nodeName;
  int nodeType;
  int nodeRet;

  light = ak_calloc(memParent, sizeof(*light), 1);

  _xml_readAttr(light, light->id, _s_dae_id);
  _xml_readAttr(light, light->name, _s_dae_name);

  last_tq = light->technique;
  last_tc = light->technique_common;

  do {
    _xml_beginElement(_s_dae_light);

    if (_xml_eqElm(_s_dae_asset)) {
      ak_assetinf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(light, reader, &assetInf);
      if (ret == AK_OK)
        light->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_techniquec)) {
      ak_technique_common *tc;
      AkResult ret;

      tc = NULL;
      ret = ak_dae_techniquec(light, reader, &tc);
      if (ret == AK_OK) {
        if (last_tc)
          last_tc->next = tc;
        else
          light->technique_common = tc;

        last_tc = tc;
      }

    } else if (_xml_eqElm(_s_dae_technique)) {
      ak_technique *tq;
      AkResult ret;

      tq = NULL;
      ret = ak_dae_technique(light, reader, &tq);
      if (ret == AK_OK) {
        if (last_tq)
          last_tq->next = tq;
        else
          light->technique = tq;

        last_tq = tq;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(light, nodePtr, &tree, NULL);
      light->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = light;

  return AK_OK;
}
