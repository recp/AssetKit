/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "param.h"
#include "value.h"

AkNewParam* _assetkit_hide
dae_newparam(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp) {
  AkNewParam   *newparam;

  newparam = ak_heap_calloc(dst->heap, memp, sizeof(*newparam));
  ak_setypeid(newparam, AKT_NEWPARAM);
  
  sid_set(xml, dst->heap, newparam);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_semantic)) {
      
    } else {
      /* load once */
      if (!newparam->val) {
        newparam->val = dae_value(dst, xml, newparam);
      }
    }
    xml = xml->next;
  }

  return newparam;
}

AkParam* _assetkit_hide
dae_param(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkParam *param;

  param = ak_heap_calloc(dst->heap, memp, sizeof(*param));
  ak_setypeid(param, AKT_PARAM);

  param->ref = xmla_strdup_by(xml, dst->heap, _s_dae_ref, param);

  return param;
}
