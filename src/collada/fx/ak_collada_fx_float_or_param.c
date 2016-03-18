/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_float_or_param.h"
#include "../ak_collada_common.h"
#include "../ak_collada_param.h"

int _assetkit_hide
ak_dae_floatOrParam(void * __restrict memParent,
                     xmlTextReaderPtr reader,
                     const char * elm,
                     ak_fx_float_or_param ** __restrict dest) {
  ak_fx_float_or_param *floatOrParam;
  ak_param *last_param;

  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  floatOrParam = ak_calloc(memParent, sizeof(*floatOrParam), 1);
  last_param = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_float)) {
      ak_basic_attrf * valuef;
      const char * floatStr;

      valuef = ak_calloc(floatOrParam, sizeof(*valuef), 1);
      _xml_readAttr(valuef, valuef->sid, _s_dae_sid);
      _xml_readConstText(floatStr);
      if (floatStr)
        valuef->val = strtof(floatStr, NULL);

      floatOrParam->val = valuef;
    } else if (_xml_eqElm(_s_dae_param)) {
      ak_param * param;
      int         ret;

      ret = ak_dae_param(floatOrParam,
                          reader,
                          ak_PARAM_TYPE_BASIC,
                          &param);

      if (ret == 0) {
        if (last_param)
          last_param->next = param;
        else
          floatOrParam->param = param;

        last_param = param;
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = floatOrParam;

  return 0;
}
