/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "id.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void _assetkit_hide
ak_id_newheap(AkHeap * __restrict heap) {
  size_t *idp;
  void   *idpstr;

  heap->idheap = ak_heap_new(NULL, NULL, NULL);

  /* default prefix */
  idpstr = *(void **)ak_opt_get(AK_OPT_DEFAULT_ID_PREFIX);
  idp    = ak_heap_alloc(heap->idheap, NULL, sizeof(size_t));
  *idp   = 1;
  ak_heap_setId(heap->idheap,
                ak__alignof(idp),
                idpstr);

  ak_heap_attach(heap, heap->idheap);
}

AK_EXPORT
const char *
ak_generatId(AkDoc      * __restrict doc,
             void       * __restrict parentmem,
             const char * __restrict prefix) {
  return ak_id_gen(ak_heap_getheap(doc),
                   parentmem,
                   prefix);
}

_assetkit_hide
const char *
ak_id_gen(AkHeap     * __restrict heap,
          void       * __restrict parentmem,
          const char * __restrict prefix) {
  AkHeap     *idheap;
  size_t     *idp;
  void       *idpf;
  char       *id;
  const char *idpstr;
  size_t      size, tknsize;
  AkResult    ret;

  if (!prefix)
    idpstr = *(const char **)ak_opt_get(AK_OPT_DEFAULT_ID_PREFIX);
  else
    idpstr = prefix;

  idheap = heap->idheap;
  if (!idheap) {
    ak_id_newheap(heap);
    idheap = heap->idheap;
  }

  ret = ak_heap_getMemById(idheap, (void *)idpstr, &idpf);

  if (ret != AK_OK) {
    idp  = ak_heap_alloc(idheap, NULL, sizeof(size_t));
    *idp = 1;
    ak_heap_setId(idheap,
                  ak__alignof(idp),
                  (void *)idpstr);
  } else {
    idp = idpf;
  }

  size = strlen(idpstr);

  do {
    AkHeapNode *node;
    AkResult    ret;

    /* we ensure that token > 0 when in ctor */
    tknsize = (size_t)log10((double)*idp) + 1;

    id = ak_heap_alloc(heap, parentmem, size + tknsize + 1);
    id[size + tknsize] = '\0';

    strcpy(id, idpstr);
    sprintf(id + size, "%zu", *idp);

    ret = ak_heap_getNodeById(heap, (void *)id, &node);

    ++(*idp);

    /* ok no dups */
    if (ret == AK_EFOUND)
      break;

    ak_free(id);
  } while (true);
  
  return id;
}
