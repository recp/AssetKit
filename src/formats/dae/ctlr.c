/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"

AK_EXPORT
AkGeometry*
ak_baseGeometry(AkURL * __restrict baseurl) {
  void         *found;
  AkController *ctlr;
  AkSkinDAE    *skindae;
  AkTypeId      foundType;

  if ((found = ak_getObjectByUrl(baseurl))) {
    foundType = ak_typeid(found);
    switch (foundType) {
      case AKT_GEOMETRY:
        return found;
      case AKT_CONTROLLER: {
        ctlr = found;
        if (ctlr->type == AK_CONTROLLER_SKIN
            && (skindae = ctlr->data)) {
          return ak_baseGeometry(&skindae->baseGeom);
        }
      }
      default: break;
    }
  }

  return NULL;
}
