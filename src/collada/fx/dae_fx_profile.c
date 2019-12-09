/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_profile.h"
#include "../core/dae_param.h"
#include "../core/dae_asset.h"

#include "../1.4/dae14_image.h"

#include "dae_fx_technique.h"

static ak_enumpair profileMap[] = {
  {_s_dae_prfl_common, AK_PROFILE_TYPE_COMMON}
};

static size_t profileMapLen = 0;

AkResult _assetkit_hide
dae_profile(AkXmlState * __restrict xst,
            void * __restrict memParent,
            AkProfile ** __restrict dest) {
  AkProfile         *profile;
  AkNewParam        *last_newparam;
  AkTechniqueFx     *last_techfx;
  const ak_enumpair *found;
  AkXmlElmState      xest;

  xst->nodeName = xmlTextReaderConstName(xst->reader);

  if (profileMapLen == 0) {
    profileMapLen = AK_ARRAY_LEN(profileMap);
    qsort(profileMap,
          profileMapLen,
          sizeof(profileMap[0]),
          ak_enumpair_cmp);
  }

  found = bsearch(xst->nodeName,
                  profileMap,
                  profileMapLen,
                  sizeof(profileMap[0]),
                  ak_enumpair_cmp2);
  if (!found) {
    ak_xml_skipelm(xst);
    goto err;
  }

  switch (found->val) {
    case AK_PROFILE_TYPE_COMMON:
      profile = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(AkProfileCommon));
      break;
    default:
      goto err;
  }

  ak_setypeid(profile, AKT_PROFILE);
  profile->type = found->val;

  ak_xml_readid(xst, profile);

  last_newparam = NULL;
  last_techfx   = NULL;

  ak_xest_init(xest, found->key)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, profile, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_newparam)) {
      AkNewParam *newparam;
      AkResult    ret;

      ret = dae_newparam(xst, profile, &newparam);

      if (ret == AK_OK) {
        if (last_newparam)
          last_newparam->next = newparam;
        else
          profile->newparam = newparam;

        newparam->prev = last_newparam;
        last_newparam  = newparam;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
      AkTechniqueFx * technique_fx;
      AkResult ret;

      ret = dae_techniqueFx(xst,  profile, &technique_fx);
      if (ret == AK_OK) {
        if (last_techfx)
          last_techfx->next = technique_fx;
        else
          profile->technique = technique_fx;

        last_techfx = technique_fx;
      }
    } else if (xst->version < AK_COLLADA_VERSION_150
               && ak_xml_eqelm(xst, _s_dae_image)) {
      /* migration from 1.4 */
      dae14_fxMigrateImg(xst, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, profile, &profile->extra);
    }  else {
      ak_xml_skipelm(xst);
    } 
    
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = profile;
  
  return AK_OK;
err:
  return AK_ERR;
}
