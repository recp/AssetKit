/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_effect.h"

#include "../core/ak_collada_asset.h"
#include "../core/ak_collada_technique.h"
#include "../core/ak_collada_annotate.h"
#include "../core/ak_collada_param.h"

#include "ak_collada_fx_profile.h"

AkResult _assetkit_hide
ak_dae_effect(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkEffect ** __restrict  dest) {
  AkEffect   *effect;
  AkAnnotate *last_annotate;
  AkNewParam *last_newparam;
  AkProfile  *last_profile;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  effect = ak_heap_calloc(heap, memParent, sizeof(*effect), true);

  last_annotate = NULL;
  last_newparam = NULL;
  last_profile  = NULL;

  _xml_readId(effect);
  _xml_readAttr(effect, effect->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_effect);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(heap, effect, reader, &assetInf);
      if (ret == AK_OK)
        effect->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(heap, effect, reader, &annotate);

      if (ret == AK_OK) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          effect->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_newparam)) {
      AkNewParam *newparam;
      AkResult    ret;

      ret = ak_dae_newparam(heap, effect, reader, &newparam);

      if (ret == AK_OK) {
        if (last_newparam)
          last_newparam->next = newparam;
        else
          effect->newparam = newparam;

        last_newparam = newparam;
      }
    } else if (_xml_eqElm(_s_dae_prfl_common)
               || _xml_eqElm(_s_dae_prfl_glsl)
               || _xml_eqElm(_s_dae_prfl_gles2)
               || _xml_eqElm(_s_dae_prfl_gles)
               || _xml_eqElm(_s_dae_prfl_cg)
               || _xml_eqElm(_s_dae_prfl_bridge)) {
      AkProfile *profile;
      AkResult   ret;

      ret = ak_dae_profile(heap, effect, reader, &profile);

      if (ret == AK_OK) {
        if (last_profile)
          last_profile->next = profile;
        else
          effect->profile = profile;

        last_profile = profile;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, effect, nodePtr, &tree, NULL);
      effect->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = effect;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxInstanceEffect(AkHeap * __restrict heap,
                        void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        AkInstanceEffect ** __restrict dest) {
  AkInstanceEffect *instanceEffect;
  AkTechniqueHint  *last_techHint;
  AkSetParam       *last_setparam;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  instanceEffect = ak_heap_calloc(heap,
                                  memParent,
                                  sizeof(*instanceEffect),
                                  false);

  _xml_readAttr(instanceEffect, instanceEffect->sid, _s_dae_sid);
  _xml_readAttr(instanceEffect, instanceEffect->name, _s_dae_name);

  ak_url_from_attr(reader,
                   _s_dae_url,
                   instanceEffect,
                   &instanceEffect->url);

  last_techHint = NULL;
  last_setparam = NULL;

  if (!xmlTextReaderIsEmptyElement(reader)) {
    do {
      _xml_beginElement(_s_dae_inst_effect);

      if (_xml_eqElm(_s_dae_technique_hint)) {
        AkTechniqueHint *techHint;

        techHint = ak_heap_calloc(heap,
                                  instanceEffect,
                                  sizeof(*techHint),
                                  false);

        _xml_readAttr(techHint, techHint->ref, _s_dae_ref);
        _xml_readAttr(techHint, techHint->profile, _s_dae_profile);
        _xml_readAttr(techHint, techHint->platform, _s_dae_platform);

        if (last_techHint)
          last_techHint->next = techHint;
        else
          instanceEffect->techniqueHint = techHint;

        last_techHint = techHint;
      } else if (_xml_eqElm(_s_dae_setparam)) {
        AkSetParam *setparam;
        AkResult ret;

        ret = ak_dae_setparam(heap,
                              instanceEffect,
                              reader,
                              &setparam);

        if (ret == AK_OK) {
          if (last_setparam)
            last_setparam->next = setparam;
          else
            instanceEffect->setparam = setparam;

          last_setparam = setparam;
        }
      } else if (_xml_eqElm(_s_dae_extra)) {
        xmlNodePtr nodePtr;
        AkTree   *tree;

        nodePtr = xmlTextReaderExpand(reader);
        tree = NULL;

        ak_tree_fromXmlNode(heap,
                            instanceEffect,
                            nodePtr,
                            &tree,
                            NULL);
        instanceEffect->extra = tree;
        
        _xml_skipElement;
      } else {
        _xml_skipElement;
      }
      
      /* end element */
      _xml_endElement;
    } while (nodeRet);
  }
  
  *dest = instanceEffect;
  
  return AK_OK;
}
