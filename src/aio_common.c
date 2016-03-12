/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_common.h"

int _assetio_hide
aio_enumpair_cmp(const void * a, const void * b) {
  return strcmp(((const aio_enumpair *)a)->key,
                ((const aio_enumpair *)b)->key);
}

int _assetio_hide
aio_enumpair_cmp2(const void * a, const void * b) {
  return strcmp(a, ((const aio_enumpair *)b)->key);
}
