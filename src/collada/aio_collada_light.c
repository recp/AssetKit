/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_light.h"
#include "aio_collada_common.h"
#include "aio_collada_asset.h"
#include "aio_collada_technique.h"

int _assetio_hide
aio_dae_light(xmlTextReaderPtr __restrict reader,
              aio_light ** __restrict dest) {
  aio_light            *light;
  aio_technique        *last_tq;
  aio_technique_common *last_tc;
  const xmlChar        *nodeName;
  int nodeType;
  int nodeRet;

  light = aio_calloc(sizeof(*light), 1);

  _xml_readAttr(light->id, _s_dae_id);
  _xml_readAttr(light->name, _s_dae_name);

  last_tq = light->technique;
  last_tc = light->technique_common;

  do {
    _xml_beginElement(_s_dae_light);

    if (_xml_eqElm(_s_dae_asset)) {
      aio_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = aio_dae_assetInf(reader, &assetInf);
      if (ret == 0)
        light->inf = assetInf;

    } if (_xml_eqElm(_s_dae_techniquec)) {
      aio_technique_common *tc;
      int                   ret;

      tc = NULL;
      ret = aio_dae_techniquec(reader, &tc);
      if (ret == 0) {
        if (last_tc)
          last_tc->next = tc;
        else
          light->technique_common = tc;

        last_tc = tc;
      }

    } else if (_xml_eqElm(_s_dae_technique)) {
      aio_technique *tq;
      int            ret;

      tq = NULL;
      ret = aio_dae_technique(reader, &tq);
      if (ret == 0) {
        if (last_tq)
          last_tq->next = tq;
        else
          light->technique = tq;

        last_tq = tq;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      aio_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      aio_tree_fromXmlNode(nodePtr, &tree, NULL);
      light->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = light;

  return 0;
}
