/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_float_or_param.h"
#include "../aio_collada_common.h"
#include "../aio_collada_param.h"

int _assetio_hide
aio_dae_floatOrParam(void * __restrict memParent,
                     xmlTextReaderPtr reader,
                     const char * elm,
                     aio_fx_float_or_param ** __restrict dest) {
  aio_fx_float_or_param *floatOrParam;
  aio_param *last_param;

  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  floatOrParam = aio_calloc(memParent, sizeof(*floatOrParam), 1);
  last_param = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_float)) {
      aio_basic_attrf * valuef;
      const char * floatStr;

      valuef = aio_calloc(floatOrParam, sizeof(*valuef), 1);
      _xml_readAttr(valuef, valuef->sid, _s_dae_sid);
      _xml_readConstText(floatStr);
      if (floatStr)
        valuef->val = strtof(floatStr, NULL);

      floatOrParam->val = valuef;
    } else if (_xml_eqElm(_s_dae_param)) {
      aio_param * param;
      int         ret;

      ret = aio_dae_param(floatOrParam,
                          reader,
                          AIO_PARAM_TYPE_BASIC,
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
