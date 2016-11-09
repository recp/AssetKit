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
    dest->sid = ak_xml_attr(xst, dest, _s_dae_sid);

  colorStr = ak_xml_rawval(xst);

  if (colorStr) {
    ak_strtof4(&colorStr, &dest->color.vec);
    xmlFree(colorStr);

    return AK_OK;
  } else {
    return AK_ERR;
  }
}
