/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_param.h"
#include "aio_collada_common.h"
#include "aio_collada_annotate.h"
#include "aio_collada_value.h"

static aio_enumpair modifierMap[] = {
  {_s_dae_const,    AIO_MODIFIER_CONST},
  {_s_dae_uniform,  AIO_MODIFIER_UNIFORM},
  {_s_dae_varying,  AIO_MODIFIER_VARYING},
  {_s_dae_static,   AIO_MODIFIER_STATIC},
  {_s_dae_volatile, AIO_MODIFIER_VOLATILE},
  {_s_dae_extern,   AIO_MODIFIER_EXTERN},
  {_s_dae_shared,   AIO_MODIFIER_SHARED}
};

static size_t modifierMapLen = 0;

int _assetio_hide
aio_dae_newparam(xmlTextReaderPtr __restrict reader,
                 aio_newparam ** __restrict dest) {
  aio_newparam  *newparam;
  aio_annotate  *last_annotate;
  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  newparam = aio_calloc(sizeof(*newparam), 1);
  last_annotate = NULL;

  if (modifierMapLen == 0) {
    modifierMapLen = AIO_ARRAY_LEN(modifierMap);
    qsort(modifierMap,
          modifierMapLen,
          sizeof(modifierMap[0]),
          aio_enumpair_cmp);
  }

  do {
    _xml_beginElement(_s_dae_newparam);

    if (_xml_eqElm(_s_dae_annotate)) {
      aio_annotate *annotate;
      int ret;

      ret = aio_dae_annotate(reader, &annotate);
      if (ret == 0) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          newparam->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_semantic)) {
      _xml_readText(newparam->semantic);
    } else if (_xml_eqElm(_s_dae_modifier)) {
      const aio_enumpair *found;
      const char *val;

      _xml_readConstText(val);

      found = bsearch(val,
                      modifierMap,
                      modifierMapLen,
                      sizeof(modifierMap[0]),
                      aio_enumpair_cmp2);

      newparam->modifier = found->val;
    } else {
      /* load once */
      if (!newparam->val) {
        void           * val;
        aio_value_type   val_type;
        int              ret;

        ret = aio_dae_value(reader,
                            &val,
                            &val_type);

        if (ret == 0) {
          newparam->val = val;
          newparam->val_type = val_type;
        }
      }
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = newparam;

  return 0;
}

int _assetio_hide
aio_dae_param(xmlTextReaderPtr __restrict reader,
              aio_param_type param_type,
              aio_param ** __restrict dest) {
  aio_param  *param;

  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  nodeType = xmlTextReaderNodeType(reader);

  if (param_type == AIO_PARAM_TYPE_BASIC) {
    param = aio_calloc(sizeof(aio_param), 1);

    _xml_readAttr(param->ref, _s_dae_ref);
  } else if (param_type == AIO_PARAM_TYPE_EXTENDED) {
    aio_param_ex *param_ex;
    param_ex = aio_calloc(sizeof(aio_param_ex), 1);

    _xml_readAttr(param_ex->name, _s_dae_name);
    _xml_readAttr(param_ex->sid, _s_dae_sid);
    _xml_readAttr(param_ex->name, _s_dae_semantic);
    _xml_readAttr(param_ex->type_name, _s_dae_type);

    if (param_ex->type_name)
      param_ex->type = aio_dae_valueType(param_ex->type_name);

    param = &param_ex->base;
  } else {
    goto err;
  }

  *dest = param;
  _xml_endElement;

  return 0;

err:

  _xml_endElement;
  return -1;
}
