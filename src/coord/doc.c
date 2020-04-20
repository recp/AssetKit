/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"

AK_EXPORT
void
ak_changeCoordSys(AkDoc * __restrict doc,
                  AkCoordSys * newCoordSys) {
  AkLibrary  *libGeom;
  AkGeometry *geom;

  libGeom = doc->lib.geometries;

  while (libGeom) {
    geom = (void *)libGeom->chld;

    while (geom) {
      ak_changeCoordSysGeom(geom, newCoordSys);
      geom = (void *)geom->base.next;
    }

    libGeom = libGeom->next;
  }

  doc->coordSys = newCoordSys;
}
