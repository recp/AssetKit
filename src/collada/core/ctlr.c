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

AkResult _assetkit_hide
dae_ctlr(AkXmlState * __restrict xst,
         void * __restrict memParent,
         void ** __restrict dest) {
  AkController *controller;
  AkXmlElmState xest;

  controller = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*controller));

  ak_xml_readid(xst, controller);
  controller->name = ak_xml_attr(xst, controller, _s_dae_name);

  ak_setypeid(controller, AKT_CONTROLLER);
  ak_xest_init(xest, _s_dae_controller)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, controller, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_skin)) {
      AkSkin  *skin;
      AkResult ret;

      ret = dae_skin(xst, controller, true, &skin);
      if (ret == AK_OK)
        controller->data = ak_objFrom(skin);

    } else if (ak_xml_eqelm(xst, _s_dae_morph)) {
      AkMorph *morph;
      AkResult ret;

      ret = dae_morph(xst, controller, true, &morph);
      if (ret == AK_OK)
        controller->data = ak_objFrom(morph);

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, controller, &controller->extra);
    } else {
      ak_xml_skipelm(xst);
    }
    
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = controller;
  
  return AK_OK;
}
