/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "accessor.h"
#include "default/def_semantic.h"
#include <assert.h>

void
ak_accessor_setparams(AkAccessor     *acc,
                      AkInputSemantic semantic) {
  const AkInputSemanticPair *smpair;
  AkHeap      *heap;
  AkDataParam *dp, *last_dp;
  uint32_t     i;

  assert(semantic > 0 && (uint32_t)semantic < ak_def_semanticc());

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

    if (!last_dp)
      acc->param = dp;
    else
      last_dp->next = dp;
    last_dp = dp;
  }
}

AkAccessor*
ak_accessor_dup(AkAccessor *oldacc) {
  AkHeap      *heap;
  AkAccessor  *acc;
  AkDataParam *dp, *newdp, *last_dp;

  heap = ak_heap_getheap(oldacc);
  acc  = ak_heap_alloc(heap,
                       ak_mem_parent(oldacc),
                       sizeof(*acc));

  memcpy(acc, oldacc, sizeof(*oldacc));

  ak_setypeid(acc, AKT_ACCESSOR);
  
  last_dp = NULL;
  dp = oldacc->param;
  while (dp) {
    newdp = ak_heap_calloc(heap,
                           acc,
                           sizeof(*newdp));
    memcpy(&newdp->type,
           &dp->type,
           sizeof(newdp->type));

    newdp->offset = dp->offset;

    if (dp->name)
      newdp->name = ak_heap_strdup(heap,
                                   acc,
                                   dp->name);

    if (dp->semantic)
      newdp->semantic = ak_heap_strdup(heap,
                                       acc,
                                       dp->semantic);

    if (last_dp)
      last_dp->next = newdp;
    else
      acc->param = newdp;
    last_dp = newdp;

    newdp->next = NULL;

    dp = dp->next;
  }

  return acc;
}
