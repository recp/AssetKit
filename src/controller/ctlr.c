/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"

AK_EXPORT
AkGeometry*
ak_baseGeometry(AkController * __restrict ctlr) {
  void *obj;

  obj = ak_objGet(ctlr->data);
  switch (ctlr->data->type) {
    case AKT_SKIN:  return ak_skinBaseGeometry(obj);
    case AKT_MORPH: return ak_morphBaseGeometry(obj);
    default:        return NULL;
  }
  return NULL;
}

AK_EXPORT
AkGeometry*
ak_skinBaseGeometry(AkSkin * __restrict skin) {
  void    *found;
  AkTypeId foundType;

  if ((found = ak_getObjectByUrl(&skin->baseGeom))) {
    foundType = ak_typeid(found);
    switch (foundType) {
      case AKT_GEOMETRY:   return found;
      case AKT_CONTROLLER: return ak_baseGeometry(found);
      default:             return NULL;
    }
  }

  return NULL;
}

AK_EXPORT
AkGeometry*
ak_morphBaseGeometry(AkMorph * __restrict morph) {
  void    *found;
  AkTypeId foundType;

  if ((found = ak_getObjectByUrl(&morph->baseGeom))) {
    foundType = ak_typeid(found);
    switch (foundType) {
      case AKT_GEOMETRY:   return found;
      case AKT_CONTROLLER: return ak_baseGeometry(found);
      default:             return NULL;
    }
  }

  return NULL;
}
