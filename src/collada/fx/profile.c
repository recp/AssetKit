/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "profile.h"
#include "techn.h"

#include "../core/param.h"
#include "../core/asset.h"
#include "../1.4/image.h"

AkProfile* _assetkit_hide
dae_profile(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp) {
  AkHeap    *heap;
  AkProfile *profile;

  heap = dst->heap;

  if (!xml_tag_eq(xml, _s_dae_prfl_common))
    return NULL;
 
  profile       = ak_heap_calloc(heap, memp, sizeof(AkProfileCommon));
  profile->type = AK_PROFILE_TYPE_COMMON;

  ak_setypeid(profile, AKT_PROFILE);
  xmla_setid(xml, heap, profile);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, profile, NULL);
    } else if (xml_tag_eq(xml, _s_dae_newparam)) {
      AkNewParam *newparam;
      
      if ((newparam = dae_newparam(dst, xml, profile))) {
        if (profile->newparam)
          profile->newparam->prev = newparam;

        newparam->next    = profile->newparam;
        profile->newparam = newparam;
      }
    } else if (xml_tag_eq(xml, _s_dae_technique)) {
      AkTechniqueFx *techn;
      
      if ((techn = dae_techniqueFx(dst, xml, profile))) {
        techn->next        = profile->technique;
        profile->technique = techn;
      }
    } else if (dst->version < AK_COLLADA_VERSION_150
               && xml_tag_eq(xml, _s_dae_image)) {
      /* migration from 1.4 */
      dae14_fxMigrateImg(dst, xml, NULL);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      profile->extra = tree_fromxml(heap, profile, xml);
    }
    xml = xml->next;
  }

  return profile;
}
