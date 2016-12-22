/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_material.h"
#include "../core/ak_collada_asset.h"
#include "../core/ak_collada_param.h"
#include "../core/ak_collada_technique.h"
#include "ak_collada_fx_effect.h"

AkResult _assetkit_hide
ak_dae_material(AkXmlState * __restrict xst,
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
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, material, &assetInf);
      if (ret == AK_OK)
        material->inf = assetInf;
    } else if (ak_xml_eqelm(xst, _s_dae_inst_effect)) {
      AkInstanceEffect *instanceEffect;
      AkResult ret;

      instanceEffect = NULL;
      ret = ak_dae_fxInstanceEffect(xst, material, &instanceEffect);
      if (ret == AK_OK)
        material->instanceEffect = instanceEffect;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          material,
                          nodePtr,
                          &tree,
                          NULL);
      material->extra = tree;

      ak_xml_skipelm(xst);
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
ak_dae_fxBindMaterial(AkXmlState * __restrict xst,
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

      ret = ak_dae_param(xst,
                         bindMaterial,
                         &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          bindMaterial->param = param;

        last_param = param;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_techniquec)) {
      AkTechniqueCommon *tc;
      AkResult ret;

      tc = NULL;
      ret = ak_dae_techniquec(xst, bindMaterial, &tc);
      if (ret == AK_OK)
        bindMaterial->techniqueCommon = tc;

    } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
      AkTechnique *tq;
      AkResult ret;

      tq = NULL;
      ret = ak_dae_technique(xst, bindMaterial, &tq);
      if (ret == AK_OK) {
        if (last_tq)
          last_tq->next = tq;
        else
          bindMaterial->technique = tq;

        last_tq = tq;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          bindMaterial,
                          nodePtr,
                          &tree,
                          NULL);
      bindMaterial->extra = tree;

      ak_xml_skipelm(xst);
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
ak_dae_fxInstanceMaterial(AkXmlState * __restrict xst,
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

  material->name   = ak_xml_attr(xst, material, _s_dae_name);
  material->target = ak_xml_attr(xst, material, _s_dae_target);
  material->symbol = ak_xml_attr(xst, material, _s_dae_symbol);

  ak_xml_attr_url(xst,
                  _s_dae_url,
                  material,
                  &material->url);

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
      techniqueOverride = ak_heap_calloc(xst->heap,
                                         material,
                                         sizeof(*techniqueOverride));

      techniqueOverride->pass = ak_xml_attr(xst,
                                            techniqueOverride,
                                            _s_dae_pass);
      techniqueOverride->ref = ak_xml_attr(xst,
                                           techniqueOverride,
                                           _s_dae_ref);

      material->techniqueOverride = techniqueOverride;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          material,
                          nodePtr,
                          &tree,
                          NULL);
      material->extra = tree;

      ak_xml_skipelm(xst);
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
