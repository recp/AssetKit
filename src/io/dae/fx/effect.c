/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "effect.h"
#include "profile.h"

#include "../core/asset.h"
#include "../core/techn.h"
#include "../core/param.h"

#include "../1.4/image.h"

 AK_HIDE
void*
dae_effect(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp) {
  AkHeap   *heap;
  AkEffect *effect;

  heap   = dst->heap;
  effect = ak_heap_calloc(heap, memp, sizeof(*effect));
  ak_setypeid(effect, AKT_EFFECT);

  xmla_setid(xml, heap, effect);

  effect->name = xmla_strdup_by(xml, heap, _s_dae_name, effect);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, effect, NULL);
    } else if (xml_tag_eq(xml, _s_dae_newparam)) {
      AkNewParam *newparam;

      if ((newparam = dae_newparam(dst, xml, effect))) {
        if (effect->newparam)
          effect->newparam->prev = newparam;

        newparam->next   = effect->newparam;
        effect->newparam = newparam;
      }
    } else if (xml_tag_eq(xml, _s_dae_prfl_common)) {
      AkProfile *profile;

      if ((profile = dae_profile(dst, xml, effect))) {
        profile->next   = effect->profile;
        effect->profile = profile;
      }
    } else if (dst->version < AK_COLLADA_VERSION_150
               && xml_tag_eq(xml, _s_dae_image)) {
      /* migration from 1.4 */
      dae14_fxMigrateImg(dst, xml, NULL);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      effect->extra = tree_fromxml(heap, effect, xml);
    }
    xml = xml->next;
  }

  return effect;
}

AK_HIDE
AkInstanceEffect*
dae_instEffect(DAEState * __restrict dst,
               xml_t    * __restrict xml,
               void     * __restrict memp) {
  AkHeap           *heap;
  AkInstanceEffect *instEffect;
  xml_attr_t       *att;

  heap       = dst->heap;
  instEffect = ak_heap_calloc(heap, memp, sizeof(*instEffect));

  xmla_setid(xml, heap, instEffect);

  instEffect->base.type = AK_INSTANCE_EFFECT;
  instEffect->base.name = xmla_strdup_by(xml, heap, _s_dae_name, instEffect);

  url_set(dst, xml, _s_dae_url, instEffect, &instEffect->base.url);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_technique_hint)) {
      AkTechniqueHint *techHint;
      
      techHint = ak_heap_calloc(heap, instEffect, sizeof(*techHint));

      if ((att = xmla(xml, _s_dae_ref)))
        techHint->ref = xmla_strdup(att, heap, techHint);
      
      if ((att = xmla(xml, _s_dae_profile)))
        techHint->profile = xmla_strdup(att, heap, techHint);
      
      if ((att = xmla(xml, _s_dae_platform)))
        techHint->platform = xmla_strdup(att, heap, techHint);
      
      techHint->next            = instEffect->techniqueHint;
      instEffect->techniqueHint = techHint;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      instEffect->base.extra = tree_fromxml(heap, instEffect, xml);
    }
    xml = xml->next;
  }

  return instEffect;
}
