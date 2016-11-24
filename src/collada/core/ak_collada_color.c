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
    ak_strtof4(&colorStr, &dest->vec);
    return AK_OK;
  }

  return AK_ERR;
}
