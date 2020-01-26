/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "effect.h"

#include "../core/asset.h"
#include "../core/techn.h"
#include "../core/param.h"

#include "../1.4/image.h"

#include "profile.h"

AkResult _assetkit_hide
dae_effect(AkXmlState * __restrict xst,
           void * __restrict memParent,
           void ** __restrict  dest) {
  AkEffect     *effect;
  AkNewParam   *last_newparam;
  AkProfile    *last_profile;
  AkXmlElmState xest;

  effect = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*effect));
  ak_setypeid(effect, AKT_EFFECT);

  last_newparam = NULL;
  last_profile  = NULL;

  ak_xml_readid(xst, effect);
  effect->name = ak_xml_attr(xst, effect, _s_dae_name);

  ak_xest_init(xest, _s_dae_effect)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, effect, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_newparam)) {
      AkNewParam *newparam;
      AkResult    ret;

      ret = dae_newparam(xst, effect, &newparam);

      if (ret == AK_OK) {
        if (last_newparam)
          last_newparam->next = newparam;
        else
          effect->newparam = newparam;

        newparam->prev = last_newparam;
        last_newparam  = newparam;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_prfl_common)) {
      AkProfile *profile;
      AkResult   ret;

      ret = dae_profile(xst, effect, &profile);

      if (ret == AK_OK) {
        if (last_profile)
          last_profile->next = profile;
        else
          effect->profile = profile;

        last_profile = profile;
      }
    } else if (xst->version < AK_COLLADA_VERSION_150
               && ak_xml_eqelm(xst, _s_dae_image)) {
      /* migration from 1.4 */
      dae14_fxMigrateImg(xst, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, effect, &effect->extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = effect;

  return AK_OK;
}

AkResult _assetkit_hide
dae_fxInstanceEffect(AkXmlState * __restrict xst,
                     void * __restrict memParent,
                     AkInstanceEffect ** __restrict dest) {
  AkInstanceEffect *instanceEffect;
  AkTechniqueHint  *last_techHint;
  AkXmlElmState     xest;

  instanceEffect = ak_heap_calloc(xst->heap,
                                  memParent,
                                  sizeof(*instanceEffect));

  ak_xml_readsid(xst, instanceEffect);

  instanceEffect->base.type = AK_INSTANCE_EFFECT;
  instanceEffect->base.name = ak_xml_attr(xst,
                                          instanceEffect,
                                          _s_dae_name);

  ak_xml_attr_url(xst,
                  _s_dae_url,
                  instanceEffect,
                  &instanceEffect->base.url);

  last_techHint = NULL;

  ak_xest_init(xest, _s_dae_inst_effect)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_technique_hint)) {
      AkTechniqueHint *techHint;

      techHint = ak_heap_calloc(xst->heap,
                                instanceEffect,
                                sizeof(*techHint));

      techHint->ref      = ak_xml_attr(xst, techHint, _s_dae_ref);
      techHint->profile  = ak_xml_attr(xst, techHint, _s_dae_profile);
      techHint->platform = ak_xml_attr(xst, techHint, _s_dae_platform);

      if (last_techHint)
        last_techHint->next = techHint;
      else
        instanceEffect->techniqueHint = techHint;

      last_techHint = techHint;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, instanceEffect, &instanceEffect->base.extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = instanceEffect;

  return AK_OK;
}
