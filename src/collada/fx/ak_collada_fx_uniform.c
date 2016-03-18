/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_uniform.h"
#include "../ak_collada_common.h"
#include "../ak_collada_param.h"
#include "../ak_collada_value.h"

int _assetkit_hide
ak_dae_fxBindUniform(void * __restrict memParent,
                      xmlTextReaderPtr reader,
                      ak_bind_uniform ** __restrict dest) {
  ak_bind_uniform *bind_uniform;
  ak_param        *last_param;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  bind_uniform = ak_calloc(memParent, sizeof(*bind_uniform), 1);

  _xml_readAttr(bind_uniform, bind_uniform->symbol, _s_dae_symbol);

  last_param = NULL;

  do {
    _xml_beginElement(_s_dae_bind_uniform);

    if (_xml_eqElm(_s_dae_param)) {
      ak_param * param;
      int         ret;

      ret = ak_dae_param(bind_uniform,
                          reader,
                          ak_PARAM_TYPE_BASIC,
                          &param);

      if (ret == 0) {
        if (last_param)
          last_param->next = param;
        else
          bind_uniform->param = param;

        last_param = param;
      }
    } else {
      /* load once */
      if (!bind_uniform->val) {
        void           * val;
        ak_value_type   val_type;
        int              ret;

        ret = ak_dae_value(bind_uniform,
                            reader,
                            &val,
                            &val_type);

        if (ret == 0) {
          bind_uniform->val = val;
          bind_uniform->val_type = val_type;
        }
      }
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = bind_uniform;
  
  return 0;
}
