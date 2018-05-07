/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"

int _assetkit_hide
ak_enumpair_cmp(const void * a, const void * b) {
  return strcmp(((const ak_enumpair *)a)->key,
                ((const ak_enumpair *)b)->key);
}

int _assetkit_hide
ak_enumpair_cmp2(const void * a, const void * b) {
  return strcmp(a, ((const ak_enumpair *)b)->key);
}
