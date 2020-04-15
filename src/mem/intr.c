/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"

_assetkit_hide
void
ak_dsSetAllocator(AkHeapAllocator * __restrict alc,
                  DsAllocator     * __restrict dsalc) {
  dsalc->calloc   = alc->calloc;
  dsalc->free     = alc->free;
  dsalc->malloc   = alc->malloc;
  dsalc->memalign = alc->memalign;
  dsalc->realloc  = alc->realloc;
  dsalc->size     = alc->size;
  dsalc->strdup   = alc->strdup;
}
