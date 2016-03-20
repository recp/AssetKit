/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_color.h"

AkResult _assetkit_hide
ak_dae_color(xmlTextReaderPtr reader,
              bool read_sid,
              ak_color * __restrict dest) {
  char          *colorStr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  if (read_sid)
    _xml_readAttr(dest, dest->sid, _s_dae_sid);

  _xml_readMutText(colorStr);

  if (colorStr) {
    ak_strtof4(&colorStr, &dest->vec);
    xmlFree(colorStr);

    return AK_OK;
  } else {
    return AK_ERR;
  }
}
