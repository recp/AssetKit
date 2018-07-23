/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_color.h"

AkResult _assetkit_hide
dae_color(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          bool                    read_sid,
          bool                    stack,
          AkColor    * __restrict dest) {
  char *colorStr;
  void *memp;

  memp = stack ? memParent : dest;

  if (read_sid)
    ak_xml_readsid(xst, memp);

  colorStr = ak_xml_rawcval(xst);

  if (colorStr) {
    int c;
    c = ak_strtof4(&colorStr, &dest->vec);
    if (c > 0) {
      do {
        dest->vec[4 - c--] = 1.0f;
      } while (c > 0);
    }
    return AK_OK;
  }

  return AK_ERR;
}
