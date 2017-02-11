/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_accessor.h"
#include "default/ak_def_semantic.h"
#include <assert.h>

void
ak_accessor_setparams(AkAccessor     *acc,
                      AkInputSemantic semantic) {
  const AkInputSemanticPair *smpair;
  AkHeap      *heap;
  AkDataParam *dp, *last_dp;
  uint32_t     i;

  assert(ak_def_semanticc() < semantic && semantic > 0);

  smpair = ak_def_semantic()[semantic];
  acc->bound = smpair->count;

  heap = ak_heap_getheap(acc);
  dp = last_dp = NULL;
  for (i = 0; i < acc->bound; i++) {
    dp         = ak_heap_calloc(heap, acc, sizeof(*dp));
    dp->offset = i;
    dp->name   = ak_heap_strdup(heap,
                                dp,
                                smpair->params[i]);
    memcpy(&dp->type,
           smpair->type,
           sizeof(dp->type));

    if (!acc->param)
      acc->param = dp;
    else
      last_dp->next = dp;
    last_dp = dp;
  }
}
