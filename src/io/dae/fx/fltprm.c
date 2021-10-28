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

#include "fltprm.h"
#include "../core/param.h"

AK_HIDE AkFloatOrParam*
dae_floatOrParam(DAEState * __restrict dst,
                 xml_t    * __restrict xml,
                 void     * __restrict memp) {
  AkHeap         *heap;
  AkFloatOrParam *flt;
  const xml_t    *sval;

  heap = dst->heap;
  flt  = ak_heap_calloc(heap, memp, sizeof(*flt));

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_float) && (sval = xmls(xml))) {
      float *valuef;
      
      valuef  = ak_heap_calloc(heap, flt, sizeof(*valuef));
      xml_strtof_fast(sval, valuef, 1);
      
      sid_set(xml, heap, valuef);
      
      flt->val = valuef;
    } else if (xml_tag_eq(xml, _s_dae_param)) {
      AkParam *param;
      
      if ((param = dae_param(dst, xml, flt))) {
        if (flt->param)
          flt->param->prev = param;
        
        param->next = flt->param;
        flt->param  = param;
      }
    }
    xml = xml->next;
  }

  return flt;
}
