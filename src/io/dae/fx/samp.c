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

#include "samp.h"
#include "img.h"
#include "../core/color.h"
#include "../core/enum.h"

#include "../1.4/dae14.h"
#include "../1.4/surface.h"

_assetkit_hide
AkSampler*
dae_sampler(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp) {
  AkHeap    *heap;
  AkSampler *samp;

  heap = dst->heap;
  samp = ak_heap_calloc(heap, memp, sizeof(*samp));
  ak_setypeid(samp, AKT_SAMPLER);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source)) {
      /* COLLADA 1.4 uses source -> <surface> for texturing */
      if (dst->version < AK_COLLADA_VERSION_150) {
        dae14_loadjobs_add(dst,
                           samp,
                           xml_strdup(xml, heap, samp),
                           AK_DAE14_LOADJOB_SURFACE);
      }
    } else if (xml_tag_eq(xml, _s_dae_instance_image)) {
      AkInstanceBase *instImage;
      if ((instImage = dae_instImage(dst, xml, samp)))
        rb_insert(dst->instanceMap, samp, instImage);
    } else if (xml_tag_eq(xml, _s_dae_wrap_s)) {
      samp->wrapS = dae_wrap(xml);
    } else if (xml_tag_eq(xml, _s_dae_wrap_t)) {
      samp->wrapT = dae_wrap(xml);
    } else if (xml_tag_eq(xml, _s_dae_wrap_p)) {
      samp->wrapP = dae_wrap(xml);
    } else if (xml_tag_eq(xml, _s_dae_minfilter)) {
      samp->minfilter = dae_minfilter(xml);
    } else if (xml_tag_eq(xml, _s_dae_magfilter)) {
      samp->magfilter = dae_magfilter(xml);
    } else if (xml_tag_eq(xml, _s_dae_mipfilter)) {
      samp->mipfilter = dae_mipfilter(xml);
    } else if (xml_tag_eq(xml, _s_dae_border_color)) {
      AkColor *color;

      color = ak_heap_calloc(heap, samp, sizeof(*color));
      dae_color(xml, samp, true, false, color);
      
      samp->borderColor = color;
    } else if (xml_tag_eq(xml, _s_dae_mip_max_level)) {
      samp->mipMaxLevel = xml_u32(xml, 0);
    } else if (xml_tag_eq(xml, _s_dae_mip_min_level)) {
      samp->mipMinLevel = xml_u32(xml, 0);
    } else if (xml_tag_eq(xml, _s_dae_mip_bias)) {
      samp->mipBias = xml_float(xml, 0);
    } else if (xml_tag_eq(xml, _s_dae_max_anisotropy)) {
      samp->maxAnisotropy = xml_u32(xml, 1l);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      samp->extra = tree_fromxml(heap, samp, xml);
    }
    xml = xml->next;
  }

  return samp;
}
