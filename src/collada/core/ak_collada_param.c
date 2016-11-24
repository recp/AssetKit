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
ak_dae_newparam(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkNewParam ** __restrict dest) {
  AkNewParam *newparam;
  AkAnnotate *last_annotate;

  newparam = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*newparam));
  last_annotate = NULL;

  if (modifierMapLen == 0) {
    modifierMapLen = AK_ARRAY_LEN(modifierMap);
    qsort(modifierMap,
          modifierMapLen,
          sizeof(modifierMap[0]),
          ak_enumpair_cmp);
  }

  do {
    if (ak_xml_beginelm(xst, _s_dae_newparam))
      break;

    if (ak_xml_eqelm(xst, _s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(xst, newparam, &annotate);
      if (ret == AK_OK) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          newparam->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_semantic)) {
       newparam->semantic = ak_xml_val(xst, newparam);
    } else if (ak_xml_eqelm(xst, _s_dae_modifier)) {
      const ak_enumpair *found;
      const char *val;

      val = ak_xml_rawcval(xst);
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

        ret = ak_dae_value(xst,
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
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = newparam;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_param(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkParamType paramType,
             AkParam ** __restrict dest) {
  AkParam *param;

  xst->nodeType = xmlTextReaderNodeType(xst->reader);

  if (paramType == AK_PARAM_TYPE_BASIC) {
    param = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(AkParam));

    param->ref = ak_xml_attr(xst, param, _s_dae_ref);
  } else if (paramType == AK_PARAM_TYPE_EXTENDED) {
    AkParamEx *param_ex;
    param_ex = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(AkParamEx));

    ak_xml_readsid(xst, param_ex);

    param_ex->name     = ak_xml_attr(xst, param_ex, _s_dae_name);
    param_ex->semantic = ak_xml_attr(xst, param_ex, _s_dae_semantic);
    param_ex->typeName = ak_xml_attr(xst, param_ex, _s_dae_type);

    if (param_ex->typeName)
      param_ex->type = ak_dae_valueType(param_ex->typeName);

    param = &param_ex->base;
  } else {
    goto err;
  }

  *dest = param;
  ak_xml_endelm(xst);

  return AK_OK;

err:

  ak_xml_endelm(xst);
  return AK_ERR;
}

AkResult _assetkit_hide
ak_dae_setparam(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkSetParam ** __restrict dest) {
  AkSetParam *setparam;

  setparam = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*setparam));

  if (modifierMapLen == 0) {
    modifierMapLen = AK_ARRAY_LEN(modifierMap);
    qsort(modifierMap,
          modifierMapLen,
          sizeof(modifierMap[0]),
          ak_enumpair_cmp);
  }

  do {
    if (ak_xml_beginelm(xst, _s_dae_setparam))
      break;

    /* load once */
    if (!setparam->val) {
      void      * val;
      AkValueType val_type;
      AkResult    ret;

      ret = ak_dae_value(xst,
                         setparam,
                         &val,
                         &val_type);

      if (ret == AK_OK) {
        setparam->val = val;
        setparam->valType = val_type;
      }
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = setparam;
  
  return AK_OK;
}
