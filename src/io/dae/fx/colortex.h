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

#ifndef dae_colortex_h
#define dae_colortex_h

#include "../common.h"

AK_HIDE
void
dae_colorOrTexSet(DAEState    * __restrict dst,
                  xml_t       * __restrict xml,
                  void        * __restrict memp,
                  AkColorDesc * __restrict clr);

AK_INLINE
AkColorDesc*
dae_colorOrTex(DAEState * __restrict dst,
               xml_t    * __restrict xml,
               void     * __restrict memp) {
  AkHeap      *heap;
  AkColorDesc *clr;

  heap = dst->heap;
  clr  = ak_heap_calloc(heap, memp, sizeof(*clr));

  dae_colorOrTexSet(dst, xml, clr, clr);

  return clr;
}

#endif /* dae_colortex_h */
