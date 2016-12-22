/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_float_or_param.h"
#include "../core/ak_collada_param.h"

AkResult _assetkit_hide
ak_dae_floatOrParam(AkXmlState * __restrict xst,
                    void * __restrict memParent,
                    const char * elm,
                    AkFxFloatOrParam ** __restrict dest) {
  AkFxFloatOrParam *floatOrParam;
  AkParam          *last_param;
  AkXmlElmState     xest;

  floatOrParam = ak_heap_calloc(xst->heap,
                                memParent,
                                sizeof(*floatOrParam));
  last_param = NULL;

  ak_xest_init(xest, elm)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_float)) {
      float      *valuef;
      const char *floatStr;

      valuef = ak_heap_calloc(xst->heap,
                              floatOrParam,
                              sizeof(*valuef));

      ak_xml_readsid(xst, valuef);

      floatStr = ak_xml_rawcval(xst);
      if (floatStr)
        *valuef = strtof(floatStr, NULL);

      floatOrParam->val = valuef;
    } else if (ak_xml_eqelm(xst, _s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = ak_dae_param(xst,
                         floatOrParam,
                         &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          floatOrParam->param = param;

        last_param = param;
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = floatOrParam;

  return AK_OK;
}
