/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_param.h"
#include "dae_value.h"

AkResult _assetkit_hide
dae_newparam(AkXmlState  * __restrict xst,
             void        * __restrict memParent,
             AkNewParam ** __restrict dest) {
  AkNewParam   *newparam;
  AkXmlElmState xest;

  newparam = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*newparam));
  ak_setypeid(newparam, AKT_NEWPARAM);
  ak_xml_readsid(xst, newparam);

  ak_xest_init(xest, _s_dae_newparam)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_semantic)) {
       newparam->semantic = ak_xml_val(xst, newparam);
    } else {
      /* load once */
      if (!newparam->val) {
        AkValue *val;
        AkResult ret;

        ret = dae_value(xst, newparam, &val);

        if (ret == AK_OK)
          newparam->val = val;
      }
    }
    
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = newparam;

  return AK_OK;
}

AkResult _assetkit_hide
dae_param(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          AkParam   ** __restrict dest) {
  AkParam      *param;
  AkXmlElmState xest;

  xst->nodeType = xmlTextReaderNodeType(xst->reader);

  param = ak_heap_calloc(xst->heap,
                         memParent,
                         sizeof(AkParam));
  ak_setypeid(param, AKT_PARAM);

  param->ref = ak_xml_attr(xst, param, _s_dae_ref);

  *dest = param;

  ak_xest_init(xest, _s_dae_param);
  ak_xml_end(&xest);

  return AK_OK;
}
