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

#include "ctlr.h"
#include "skin.h"
#include "morph.h"
#include "../core/asset.h"

void* AK_HIDE
dae_ctlr(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp) {
  AkHeap       *heap;
  AkController *ctlr;

  heap       = dst->heap;
  ctlr       = ak_heap_calloc(heap, memp, sizeof(*ctlr));
  ctlr->name = xmla_strdup_by(xml, heap, _s_dae_name, ctlr);
  
  xmla_setid(xml, heap, ctlr);
  ak_setypeid(ctlr, AKT_CONTROLLER);
  
  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, ctlr, NULL);
    } else if (xml_tag_eq(xml, _s_dae_skin)) {
      ctlr->data = dae_skin(dst, xml, ctlr);
      ctlr->type = AK_CONTROLLER_SKIN;
    } else if (xml_tag_eq(xml, _s_dae_morph)) {
      ctlr->data = dae_morph(dst, xml, ctlr);
      ctlr->type = AK_CONTROLLER_MORPH;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      ctlr->extra = tree_fromxml(heap, ctlr, xml);
    }
    xml = xml->next;
  }

  return ctlr;
}
