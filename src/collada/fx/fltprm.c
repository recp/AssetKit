/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "fltprm.h"
#include "../core/param.h"

AkFloatOrParam* _assetkit_hide
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
