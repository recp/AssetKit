/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "material.h"
#include "../core/asset.h"
#include "../core/param.h"
#include "../core/tech.h"
#include "effect.h"

AkResult _assetkit_hide
dae_material(AkXmlState * __restrict xst,
             void * __restrict memParent,
             void ** __restrict dest) {
  AkMaterial   *material;
  AkXmlElmState xest;

  material = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*material));

  ak_xml_readid(xst, material);
  material->name = ak_xml_attr(xst, material, _s_dae_name);

  ak_xest_init(xest, _s_dae_material)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, material, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_inst_effect)) {
      AkInstanceEffect *instanceEffect;
      AkResult ret;

      instanceEffect = NULL;
      ret = dae_fxInstanceEffect(xst, material, &instanceEffect);
      if (ret == AK_OK)
        material->effect = instanceEffect;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, material, &material->extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = material;

  return AK_OK;
}

AkResult _assetkit_hide
dae_fxBindMaterial(AkXmlState * __restrict xst,
                   void * __restrict memParent,
                   AkBindMaterial ** __restrict dest) {
  AkBindMaterial *bindMaterial;
  AkParam        *last_param;
  AkTechnique    *last_tq;
  AkXmlElmState   xest;

  bindMaterial = ak_heap_calloc(xst->heap,
                                memParent,
                                sizeof(*bindMaterial));

  last_param = NULL;
  last_tq    = NULL;

  ak_xest_init(xest, _s_dae_bind_material)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = dae_param(xst, bindMaterial, &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          bindMaterial->param = param;

        param->prev = last_param;
        last_param  = param;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_techniquec)) {
      AkInstanceMaterial *tcommon;
      AkResult            ret;

      tcommon = NULL;
      ret     = dae_fxBindMaterial_tcommon(xst, bindMaterial, &tcommon);
      if (ret == AK_OK)
        bindMaterial->tcommon = tcommon;

    } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
      AkTechnique *tq;
      AkResult ret;

      tq = NULL;
      ret = dae_techn(xst, bindMaterial, &tq);
      if (ret == AK_OK) {
        if (last_tq)
          last_tq->next = tq;
        else
          bindMaterial->technique = tq;

        last_tq = tq;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, bindMaterial, &bindMaterial->extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = bindMaterial;

  return AK_OK;
}

AkResult _assetkit_hide
dae_fxInstanceMaterial(AkXmlState * __restrict xst,
                       void * __restrict memParent,
                       AkInstanceMaterial ** __restrict dest) {
  AkInstanceMaterial *material;
  AkBind             *last_bind;
  AkBindVertexInput  *last_bindVertexInput;
  AkXmlElmState       xest;

  material = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*material));

  ak_xml_readsid(xst, material);

  material->base.name = ak_xml_attr(xst, material, _s_dae_name);
  material->symbol    = ak_xml_attr(xst, material, _s_dae_symbol);

  ak_xml_attr_url(xst,
                  _s_dae_target,
                  material,
                  &material->base.url);

  last_bind = NULL;
  last_bindVertexInput = NULL;

  ak_xest_init(xest, _s_dae_instance_material)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_bind)) {
      AkBind *bind;
      bind = ak_heap_calloc(xst->heap,
                            material,
                            sizeof(*bind));

      bind->semantic = ak_xml_attr(xst, bind, _s_dae_semantic);
      bind->target   = ak_xml_attr(xst, bind, _s_dae_target);

      if (last_bind)
        last_bind->next = bind;
      else
        material->bind = bind;

      last_bind = bind;
    } else if (ak_xml_eqelm(xst, _s_dae_bind_vertex_input)) {
      AkBindVertexInput *bindVertexInput;
      bindVertexInput = ak_heap_calloc(xst->heap,
                                       material,
                                       sizeof(*bindVertexInput));

      bindVertexInput->semantic = ak_xml_attr(xst,
                                              bindVertexInput,
                                              _s_dae_semantic);

      bindVertexInput->inputSemantic = ak_xml_attr(xst,
                                                   bindVertexInput,
                                                   _s_dae_input_semantic);

      bindVertexInput->inputSet = ak_xml_attrui(xst, _s_dae_input_set);

      if (last_bindVertexInput)
        last_bindVertexInput->next = bindVertexInput;
      else
        material->bindVertexInput = bindVertexInput;

      last_bindVertexInput = bindVertexInput;
    } else if (ak_xml_eqelm(xst, _s_dae_technique_override)) {
      AkTechniqueOverride *techniqueOverride;
      techniqueOverride = ak_heap_calloc(xst->heap, material, sizeof(*techniqueOverride));

      techniqueOverride->pass = ak_xml_attr(xst,
                                            techniqueOverride,
                                            _s_dae_pass);
      techniqueOverride->ref = ak_xml_attr(xst,
                                           techniqueOverride,
                                           _s_dae_ref);

      material->techniqueOverride = techniqueOverride;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, material, &material->base.extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = material;

  return AK_OK;
}

AkResult _assetkit_hide
dae_fxBindMaterial_tcommon(AkXmlState          * __restrict xst,
                           void                * __restrict memParent,
                           AkInstanceMaterial ** __restrict dest) {
  AkInstanceMaterial *imat, *last_imat;
  AkXmlElmState xest;

  ak_xest_init(xest, _s_dae_techniquec)

  imat = last_imat = NULL;

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_instance_material)) {
      AkInstanceMaterial *imati;
      AkResult            ret;
      ret = dae_fxInstanceMaterial(xst, memParent, &imati);

      if (ret == AK_OK) {
        if (last_imat)
          last_imat->base.next = &imati->base;
        else
          imat = imati;
        last_imat = imati;
      }
    } else {
      ak_xml_skipelm(xst);
    }
    
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = imat;
  return AK_OK;
}
