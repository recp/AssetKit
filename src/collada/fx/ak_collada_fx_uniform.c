/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_uniform.h"
#include "../core/ak_collada_param.h"
#include "../core/ak_collada_value.h"

AkResult _assetkit_hide
ak_dae_fxBindUniform(AkHeap * __restrict heap,
                     void * __restrict memParent,
                     xmlTextReaderPtr reader,
                     AkBindUniform ** __restrict dest) {
  AkBindUniform *bindUniform;
  AkParam      *last_param;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  bindUniform = ak_heap_calloc(heap, memParent, sizeof(*bindUniform), false);

  _xml_readAttr(bindUniform, bindUniform->symbol, _s_dae_symbol);

  last_param = NULL;

  do {
    _xml_beginElement(_s_dae_bind_uniform);

    if (_xml_eqElm(_s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = ak_dae_param(heap,
                         bindUniform,
                         reader,
                         AK_PARAM_TYPE_BASIC,
                         &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          bindUniform->param = param;

        last_param = param;
      }
    } else {
      /* load once */
      if (!bindUniform->val) {
        void      * val;
        AkValueType val_type;
        AkResult    ret;

        ret = ak_dae_value(heap,
                           bindUniform,
                           reader,
                           &val,
                           &val_type);

        if (ret == AK_OK) {
          bindUniform->val = val;
          bindUniform->valType = val_type;
        }
      }
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = bindUniform;
  
  return AK_OK;
}
