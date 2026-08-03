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
#include "techn.h"

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
  xml_t    *items;
  bool      pendingExtras;

  heap   = dst->heap;
  effect = ak_heap_calloc(heap, memp, sizeof(*effect));
  ak_setypeid(effect, AKT_EFFECT);

  xmla_setid(xml, heap, effect);

  effect->name = DAE_XMLA_STRDUP8(xml, heap, name, effect);

  items = xml->val;
  xml   = items;
  pendingExtras = false;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, effect, NULL);
    } else if (DAE_XML_TAG_EQ(xml, newparam)) {
      AkNewParam *newparam;

      if ((newparam = dae_newparam(dst, xml, effect))) {
        if (effect->newparam)
          effect->newparam->prev = newparam;

        newparam->next   = effect->newparam;
        effect->newparam = newparam;
      }
    } else if (DAE_XML_TAG_EQ(xml, prfl_common)) {
      AkProfile *profile;

      if ((profile = dae_profile(dst, xml, effect))) {
        profile->next   = effect->profile;
        effect->profile = profile;
      }
    } else if (dst->version < AK_COLLADA_VERSION_150
               && DAE_XML_TAG_EQ8(xml, image)) {
      /* migration from 1.4 */
      dae14_fxMigrateImg(dst, xml, NULL);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      if (effect->profile && effect->profile->technique)
        dae_techniqueFxExtra(dst, xml, effect->profile->technique->common);
      else
        pendingExtras = true;
      effect->extra = tree_fromxml(heap, effect, xml);
    }
    xml = xml->next;
  }

  if (pendingExtras && effect->profile && effect->profile->technique) {
    for (xml = items; xml; xml = xml->next) {
      if (DAE_XML_TAG_EQ8(xml, extra))
        dae_techniqueFxExtra(dst, xml, effect->profile->technique->common);
    }
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
  instEffect->base.name = DAE_XMLA_STRDUP8(xml, heap, name, instEffect);

  DAE_URL_SET(dst, xml, url, instEffect, &instEffect->base.url);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, technique_hint)) {
      AkTechniqueHint *techHint;
      
      techHint = ak_heap_calloc(heap, instEffect, sizeof(*techHint));

      if ((att = DAE_XMLA4(xml, ref)))
        techHint->ref = xmla_strdup(att, heap, techHint);
      
      if ((att = DAE_XMLA8(xml, profile)))
        techHint->profile = xmla_strdup(att, heap, techHint);
      
      if ((att = DAE_XMLA8(xml, platform)))
        techHint->platform = xmla_strdup(att, heap, techHint);
      
      techHint->next            = instEffect->techniqueHint;
      instEffect->techniqueHint = techHint;
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      instEffect->base.extra = tree_fromxml(heap, instEffect, xml);
    }
    xml = xml->next;
  }

  return instEffect;
}
