/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_param.h"
#include "ak_collada_common.h"
#include "ak_collada_annotate.h"
#include "ak_collada_value.h"

static ak_enumpair modifierMap[] = {
  {_s_dae_const,    ak_MODIFIER_CONST},
  {_s_dae_uniform,  ak_MODIFIER_UNIFORM},
  {_s_dae_varying,  ak_MODIFIER_VARYING},
  {_s_dae_static,   ak_MODIFIER_STATIC},
  {_s_dae_volatile, ak_MODIFIER_VOLATILE},
  {_s_dae_extern,   ak_MODIFIER_EXTERN},
  {_s_dae_shared,   ak_MODIFIER_SHARED}
};

static size_t modifierMapLen = 0;

int _assetkit_hide
ak_dae_newparam(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 ak_newparam ** __restrict dest) {
  ak_newparam  *newparam;
  ak_annotate  *last_annotate;
  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  newparam = ak_calloc(memParent, sizeof(*newparam), 1);
  last_annotate = NULL;

  if (modifierMapLen == 0) {
    modifierMapLen = ak_ARRAY_LEN(modifierMap);
    qsort(modifierMap,
          modifierMapLen,
          sizeof(modifierMap[0]),
          ak_enumpair_cmp);
  }

  do {
    _xml_beginElement(_s_dae_newparam);

    if (_xml_eqElm(_s_dae_annotate)) {
      ak_annotate *annotate;
      int ret;

      ret = ak_dae_annotate(newparam, reader, &annotate);
      if (ret == 0) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          newparam->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_semantic)) {
      _xml_readText(newparam, newparam->semantic);
    } else if (_xml_eqElm(_s_dae_modifier)) {
      const ak_enumpair *found;
      const char *val;

      _xml_readConstText(val);

      found = bsearch(val,
                      modifierMap,
                      modifierMapLen,
                      sizeof(modifierMap[0]),
                      ak_enumpair_cmp2);

      newparam->modifier = found->val;
    } else {
      /* load once */
      if (!newparam->val) {
        void           * val;
        ak_value_type   val_type;
        int              ret;

        ret = ak_dae_value(newparam,
                            reader,
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

int _assetkit_hide
ak_dae_param(void * __restrict memParent,
              xmlTextReaderPtr reader,
              ak_param_type param_type,
              ak_param ** __restrict dest) {
  ak_param  *param;

  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  nodeType = xmlTextReaderNodeType(reader);

  if (param_type == ak_PARAM_TYPE_BASIC) {
    param = ak_calloc(memParent, sizeof(ak_param), 1);

    _xml_readAttr(param, param->ref, _s_dae_ref);
  } else if (param_type == ak_PARAM_TYPE_EXTENDED) {
    ak_param_ex *param_ex;
    param_ex = ak_calloc(memParent, sizeof(ak_param_ex), 1);

    _xml_readAttr(param_ex, param_ex->name, _s_dae_name);
    _xml_readAttr(param_ex, param_ex->sid, _s_dae_sid);
    _xml_readAttr(param_ex, param_ex->name, _s_dae_semantic);
    _xml_readAttr(param_ex, param_ex->type_name, _s_dae_type);

    if (param_ex->type_name)
      param_ex->type = ak_dae_valueType(param_ex->type_name);

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

int _assetkit_hide
ak_dae_setparam(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 ak_setparam ** __restrict dest) {
  ak_setparam  *setparam;
  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  setparam = ak_calloc(memParent, sizeof(*setparam), 1);

  if (modifierMapLen == 0) {
    modifierMapLen = ak_ARRAY_LEN(modifierMap);
    qsort(modifierMap,
          modifierMapLen,
          sizeof(modifierMap[0]),
          ak_enumpair_cmp);
  }

  do {
    _xml_beginElement(_s_dae_setparam);

    /* load once */
    if (!setparam->val) {
      void           * val;
      ak_value_type   val_type;
      int              ret;

      ret = ak_dae_value(setparam,
                          reader,
                          &val,
                          &val_type);

      if (ret == 0) {
        setparam->val = val;
        setparam->val_type = val_type;
      }
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = setparam;
  
  return 0;
}
