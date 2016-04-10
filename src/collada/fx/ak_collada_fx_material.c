/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_material.h"
#include "../ak_collada_asset.h"
#include "../ak_collada_param.h"
#include "../ak_collada_technique.h"
#include "ak_collada_fx_effect.h"

AkResult _assetkit_hide
ak_dae_material(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 AkMaterial ** __restrict dest) {
  AkMaterial    *material;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  material = ak_calloc(memParent, sizeof(*material), 1);

  _xml_readAttr(material, material->id, _s_dae_id);
  _xml_readAttr(material, material->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_material);

    if (_xml_eqElm(_s_dae_asset)) {
      ak_assetinf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(material, reader, &assetInf);
      if (ret == AK_OK)
        material->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_inst_effect)) {
      AkInstanceEffect *instanceEffect;
      AkResult ret;

      instanceEffect = NULL;
      ret = ak_dae_fxInstanceEffect(material, reader, &instanceEffect);
      if (ret == AK_OK)
        material->instanceEffect = instanceEffect;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(material, nodePtr, &tree, NULL);
      material->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = material;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxBindMaterial(void * __restrict memParent,
                      xmlTextReaderPtr reader,
                      AkBindMaterial ** __restrict dest) {
  AkBindMaterial *bindMaterial;
  ak_param       *last_param;
  ak_technique   *last_tq;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  bindMaterial = ak_calloc(memParent, sizeof(*bindMaterial), 1);

  last_param = NULL;
  last_tq    = NULL;

  do {
    _xml_beginElement(_s_dae_bind_material);

    if (_xml_eqElm(_s_dae_param)) {
      ak_param * param;
      AkResult   ret;

      ret = ak_dae_param(bindMaterial,
                         reader,
                         AK_PARAM_TYPE_BASIC,
                         &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          bindMaterial->param = param;

        last_param = param;
      }
    } else if (_xml_eqElm(_s_dae_techniquec)) {
      ak_technique_common *tc;
      AkResult ret;

      tc = NULL;
      ret = ak_dae_techniquec(bindMaterial, reader, &tc);
      if (ret == AK_OK)
        bindMaterial->techniqueCommon = tc;

    } else if (_xml_eqElm(_s_dae_technique)) {
      ak_technique *tq;
      AkResult ret;

      tq = NULL;
      ret = ak_dae_technique(bindMaterial, reader, &tq);
      if (ret == AK_OK) {
        if (last_tq)
          last_tq->next = tq;
        else
          bindMaterial->technique = tq;

        last_tq = tq;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(bindMaterial, nodePtr, &tree, NULL);
      bindMaterial->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = bindMaterial;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxInstanceMaterial(void * __restrict memParent,
                          xmlTextReaderPtr reader,
                          AkInstanceMaterial ** __restrict dest) {
  AkInstanceMaterial *material;
  AkBind             *last_bind;
  AkBindVertexInput  *last_bindVertexInput;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  material = ak_calloc(memParent, sizeof(*material), 1);

  _xml_readAttr(material, material->sid, _s_dae_sid);
  _xml_readAttr(material, material->name, _s_dae_name);
  _xml_readAttr(material, material->target, _s_dae_target);
  _xml_readAttr(material, material->symbol, _s_dae_symbol);
  _xml_readAttr(material, material->url, _s_dae_url);

  last_bind = NULL;
  last_bindVertexInput = NULL;

  do {
    _xml_beginElement(_s_dae_instance_material);

    if (_xml_eqElm(_s_dae_bind)) {
      AkBind *bind;
      bind = ak_calloc(material, sizeof(*bind), 1);

      _xml_readAttr(bind, bind->semantic, _s_dae_semantic);
      _xml_readAttr(bind, bind->target, _s_dae_target);

      if (last_bind)
        last_bind->next = bind;
      else
        material->bind = bind;

      last_bind = bind;
    } else if (_xml_eqElm(_s_dae_bind_vertex_input)) {
      AkBindVertexInput *bindVertexInput;
      bindVertexInput = ak_calloc(material, sizeof(*bindVertexInput), 1);

      _xml_readAttr(bindVertexInput,
                    bindVertexInput->semantic,
                    _s_dae_semantic);
      _xml_readAttr(bindVertexInput,
                    bindVertexInput->inputSemantic,
                    _s_dae_input_semantic);
      _xml_readAttrUsingFn(bindVertexInput->inputSet,
                           _s_dae_input_set,
                           (AkUInt)strtoul, NULL, 10);

      if (last_bindVertexInput)
        last_bindVertexInput->next = bindVertexInput;
      else
        material->bindVertexInput = bindVertexInput;

      last_bindVertexInput = bindVertexInput;
    } else if (_xml_eqElm(_s_dae_technique_override)) {
      AkTechniqueOverride *techniqueOverride;
      techniqueOverride = ak_calloc(material, sizeof(*techniqueOverride), 1);

      _xml_readAttr(techniqueOverride, techniqueOverride->pass, _s_dae_pass);
      _xml_readAttr(techniqueOverride, techniqueOverride->ref, _s_dae_ref);

      material->techniqueOverride = techniqueOverride;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(material, nodePtr, &tree, NULL);
      material->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = material;
  
  return AK_OK;
}
