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

  floatOrParam = ak_heap_calloc(xst->heap,
                                memParent,
                                sizeof(*floatOrParam),
                                false);
  last_param = NULL;

  do {
    if (ak_xml_beginelm(xst, elm))
      break;

    if (_xml_eqElm(_s_dae_float)) {
      ak_basic_attrf * valuef;
      const char * floatStr;

      valuef = ak_heap_calloc(xst->heap,
                              floatOrParam,
                              sizeof(*valuef),
                              false);
      _xml_readAttr(valuef, valuef->sid, _s_dae_sid);
      floatStr = ak_xml_rawcval(xst);
      if (floatStr)
        valuef->val = strtof(floatStr, NULL);

      floatOrParam->val = valuef;
    } else if (_xml_eqElm(_s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = ak_dae_param(xst,
                         floatOrParam,
                         AK_PARAM_TYPE_BASIC,
                         &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          floatOrParam->param = param;

        last_param = param;
      }
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = floatOrParam;

  return AK_OK;
}
