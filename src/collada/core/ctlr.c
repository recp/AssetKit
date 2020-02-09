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

void _assetkit_hide
dae_ctlr(DAEState * __restrict dst, xml_t * __restrict xml) {
  AkDoc        *doc;
  AkHeap       *heap;
  AkController *ctlr;
  xml_t        *xctlr;

  heap = dst->heap;
  doc  = dst->doc;
  xml  = xml->val;

  while (xml) {
    if (xml_tag_eq(xml, _s_dae_controller)) {
      xctlr = xml->val;
      ctlr  = ak_heap_calloc(heap, doc, sizeof(*ctlr));

      xmla_setid(xctlr, heap, ctlr);
      ctlr->name = xmla_strdup_by(xctlr, heap, _s_dae_name, ctlr);
      
      ak_setypeid(ctlr, AKT_CONTROLLER);
      
      while (xctlr) {
        if (xml_tag_eq(xctlr, _s_dae_asset)) {
          (void)dae_asset(dst, xctlr, ctlr, NULL);
        } else if (xml_tag_eq(xctlr, _s_dae_skin)) {
          ctlr->data = dae_skin(dst, xctlr, ctlr);
        } else if (xml_tag_eq(xctlr, _s_dae_morph)) {
          ctlr->data = dae_morph(dst, xctlr, ctlr);
        } else if (xml_tag_eq(xctlr, _s_dae_extra)) {
          ctlr->extra = tree_fromxml(heap, ctlr, xctlr);
        }
        xctlr = xctlr->next;
      }
    }
    xml = xml->next;
  }
}
