/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_param.h"
#include "ak_collada_annotate.h"
#include "ak_collada_value.h"

static ak_enumpair modifierMap[] = {
  {_s_dae_const,    AK_MODIFIER_CONST},
  {_s_dae_uniform,  AK_MODIFIER_UNIFORM},
  {_s_dae_varying,  AK_MODIFIER_VARYING},
  {_s_dae_static,   AK_MODIFIER_STATIC},
  {_s_dae_volatile, AK_MODIFIER_VOLATILE},
  {_s_dae_extern,   AK_MODIFIER_EXTERN},
  {_s_dae_shared,   AK_MODIFIER_SHARED}
};

static size_t modifierMapLen = 0;

AkResult _assetkit_hide
ak_dae_newparam(AkDaeState * __restrict daestate,
                void * __restrict memParent,
                AkNewParam ** __restrict dest) {
  AkNewParam *newparam;
  AkAnnotate *last_annotate;

  newparam = ak_heap_calloc(daestate->heap,
                            memParent,
                            sizeof(*newparam),
                            false);
  last_annotate = NULL;

  if (modifierMapLen == 0) {
    modifierMapLen = AK_ARRAY_LEN(modifierMap);
    qsort(modifierMap,
          modifierMapLen,
          sizeof(modifierMap[0]),
          ak_enumpair_cmp);
  }

  do {
    _xml_beginElement(_s_dae_newparam);

    if (_xml_eqElm(_s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(daestate, newparam, &annotate);
      if (ret == AK_OK) {
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

      if (val) {
        found = bsearch(val,
                        modifierMap,
                        modifierMapLen,
                        sizeof(modifierMap[0]),
                        ak_enumpair_cmp2);

        newparam->modifier = found->val;
      }
    } else {
      /* load once */
      if (!newparam->val) {
        void      * val;
        AkValueType val_type;
        AkResult    ret;

        ret = ak_dae_value(daestate,
                           newparam,
                           &val,
                           &val_type);

        if (ret == AK_OK) {
          newparam->val = val;
          newparam->valType = val_type;
        }
      }
    }
    
    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);

  *dest = newparam;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_param(AkDaeState * __restrict daestate,
             void * __restrict memParent,
             AkParamType paramType,
             AkParam ** __restrict dest) {
  AkParam *param;

  daestate->nodeType = xmlTextReaderNodeType(daestate->reader);

  if (paramType == AK_PARAM_TYPE_BASIC) {
    param = ak_heap_calloc(daestate->heap,
                           memParent,
                           sizeof(AkParam),
                           false);

    _xml_readAttr(param, param->ref, _s_dae_ref);
  } else if (paramType == AK_PARAM_TYPE_EXTENDED) {
    AkParamEx *param_ex;
    param_ex = ak_heap_calloc(daestate->heap,
                              memParent,
                              sizeof(AkParamEx),
                              false);

    _xml_readAttr(param_ex, param_ex->name, _s_dae_name);
    _xml_readAttr(param_ex, param_ex->sid, _s_dae_sid);
    _xml_readAttr(param_ex, param_ex->name, _s_dae_semantic);
    _xml_readAttr(param_ex, param_ex->typeName, _s_dae_type);

    if (param_ex->typeName)
      param_ex->type = ak_dae_valueType(param_ex->typeName);

    param = &param_ex->base;
  } else {
    goto err;
  }

  *dest = param;
  _xml_endElement;

  return AK_OK;

err:

  _xml_endElement;
  return AK_ERR;
}

AkResult _assetkit_hide
ak_dae_setparam(AkDaeState * __restrict daestate,
                void * __restrict memParent,
                AkSetParam ** __restrict dest) {
  AkSetParam *setparam;

  setparam = ak_heap_calloc(daestate->heap,
                            memParent,
                            sizeof(*setparam),
                            false);

  if (modifierMapLen == 0) {
    modifierMapLen = AK_ARRAY_LEN(modifierMap);
    qsort(modifierMap,
          modifierMapLen,
          sizeof(modifierMap[0]),
          ak_enumpair_cmp);
  }

  do {
    _xml_beginElement(_s_dae_setparam);

    /* load once */
    if (!setparam->val) {
      void      * val;
      AkValueType val_type;
      AkResult    ret;

      ret = ak_dae_value(daestate,
                         setparam,
                         &val,
                         &val_type);

      if (ret == AK_OK) {
        setparam->val = val;
        setparam->valType = val_type;
      }
    }

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);

  *dest = setparam;
  
  return AK_OK;
}
