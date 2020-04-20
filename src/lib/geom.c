/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"

AK_EXPORT
AkGeometry *
ak_libFirstGeom(AkDoc * __restrict doc) {
  if (!doc->lib.geometries)
    return NULL;

  return (void *)doc->lib.geometries->chld;
}
