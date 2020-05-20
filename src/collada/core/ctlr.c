/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ctlr.h"
#include "../core/asset.h"
#include "skin.h"
#include "morph.h"

void* _assetkit_hide
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
//      ctlr->data = dae_morph(dst, xml, ctlr);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      ctlr->extra = tree_fromxml(heap, ctlr, xml);
    }
    xml = xml->next;
  }

  return ctlr;
}
