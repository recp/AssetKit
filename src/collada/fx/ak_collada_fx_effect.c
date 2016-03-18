/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_effect.h"

#include "../ak_collada_common.h"
#include "../ak_collada_asset.h"
#include "../ak_collada_technique.h"
#include "../ak_collada_annotate.h"
#include "../ak_collada_param.h"

#include "ak_collada_fx_profile.h"

int _assetkit_hide
ak_dae_effect(void * __restrict memParent,
               xmlTextReaderPtr reader,
               ak_effect ** __restrict  dest) {
  ak_effect   *effect;
  ak_annotate *last_annotate;
  ak_newparam *last_newparam;
  ak_profile  *last_profile;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  effect = ak_calloc(memParent, sizeof(*effect), 1);

  last_annotate = NULL;
  last_newparam = NULL;
  last_profile  = NULL;
  
  _xml_readAttr(effect, effect->id, _s_dae_id);
  _xml_readAttr(effect, effect->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_effect);

    if (_xml_eqElm(_s_dae_asset)) {
      ak_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(effect, reader, &assetInf);
      if (ret == 0)
        effect->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      ak_annotate *annotate;
      int           ret;

      ret = ak_dae_annotate(effect, reader, &annotate);

      if (ret == 0) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          effect->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_newparam)) {
      ak_newparam *newparam;
      int            ret;

      ret = ak_dae_newparam(effect, reader, &newparam);

      if (ret == 0) {
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
      ak_profile *profile;
      int          ret;

      ret = ak_dae_profile(effect, reader, &profile);

      if (ret == 0) {
        if (last_profile)
          last_profile->next = profile;
        else
          effect->profile = profile;

        last_profile = profile;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      ak_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(effect, nodePtr, &tree, NULL);
      effect->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = effect;

  return 0;
}

int _assetkit_hide
ak_dae_fxEffectInstance(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         ak_effect_instance ** __restrict dest) {
  ak_effect_instance *effectInst;
  ak_technique_hint  *last_techHint;
  ak_setparam        *last_setparam;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  effectInst = ak_calloc(memParent, sizeof(*effectInst), 1);

  _xml_readAttr(effectInst, effectInst->url, _s_dae_url);
  _xml_readAttr(effectInst, effectInst->sid, _s_dae_sid);
  _xml_readAttr(effectInst, effectInst->name, _s_dae_name);

  last_techHint = NULL;
  last_setparam = NULL;

  if (!xmlTextReaderIsEmptyElement(reader)) {
    do {
      _xml_beginElement(_s_dae_inst_effect);

      if (_xml_eqElm(_s_dae_technique_hint)) {
        ak_technique_hint *techHint;

        techHint = ak_calloc(effectInst, sizeof(*techHint), 1);

        _xml_readAttr(techHint, techHint->ref, _s_dae_ref);
        _xml_readAttr(techHint, techHint->profile, _s_dae_profile);
        _xml_readAttr(techHint, techHint->platform, _s_dae_platform);

        if (last_techHint)
          last_techHint->next = techHint;
        else
          effectInst->techniqueHint = techHint;

        last_techHint = techHint;
      } else if (_xml_eqElm(_s_dae_setparam)) {
        ak_setparam *setparam;
        int ret;

        ret = ak_dae_setparam(effectInst, reader, &setparam);

        if (ret == 0) {
          if (last_setparam)
            last_setparam->next = setparam;
          else
            effectInst->setparam = setparam;

          last_setparam = setparam;
        }
      } else if (_xml_eqElm(_s_dae_extra)) {
        xmlNodePtr nodePtr;
        ak_tree  *tree;

        nodePtr = xmlTextReaderExpand(reader);
        tree = NULL;

        ak_tree_fromXmlNode(effectInst, nodePtr, &tree, NULL);
        effectInst->extra = tree;

        _xml_skipElement;
      } else {
        _xml_skipElement;
      }

      /* end element */
      _xml_endElement;
    } while (nodeRet);
  }

  *dest = effectInst;
  
  return 0;
}
