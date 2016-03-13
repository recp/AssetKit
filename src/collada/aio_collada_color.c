/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_color.h"
#include "aio_collada_common.h"

int _assetio_hide
aio_dae_color(xmlTextReaderPtr __restrict reader,
              bool read_sid,
              aio_color * __restrict dest) {
  char          *colorStr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  if (read_sid)
    _xml_readAttr(dest, dest->sid, _s_dae_sid);

  _xml_readMutText(colorStr);

  if (colorStr) {
    aio_strtof4(&colorStr, &dest->vec);
    xmlFree(colorStr);

    return 0;
  } else {
    return -1;
  }
}
