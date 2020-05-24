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
