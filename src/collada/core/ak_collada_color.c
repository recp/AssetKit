/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_color.h"

AkResult _assetkit_hide
ak_dae_color(AkXmlState * __restrict xst,
             bool read_sid,
             AkColor * __restrict dest) {
  char *colorStr;

  if (read_sid)
    ak_xml_readsid(xst, dest);

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
