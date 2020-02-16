/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "samp.h"
#include "../core/color.h"
#include "enum.h"
#include "img.h"

#include "../1.4/dae14.h"
#include "../1.4/surface.h"

_assetkit_hide
AkSampler*
dae_sampler(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp) {
  AkHeap    *heap;
  AkSampler *samp;
  char      *sval;
  AkEnum     en;

  heap = dst->heap;
  samp = ak_heap_calloc(heap, memp, sizeof(*samp));
  ak_setypeid(samp, AKT_SAMPLER);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source) && (sval = xml->val)) {
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
    } else if (xml_tag_eq(xml, _s_dae_wrap_s) && (sval = xml->val)) {
      if ((en = dae_fxEnumWrap(sval)))
        samp->wrapS = en;
    } else if (xml_tag_eq(xml, _s_dae_wrap_t) && (sval = xml->val)) {
      if ((en = dae_fxEnumWrap(sval)))
        samp->wrapT = en;
    } else if (xml_tag_eq(xml, _s_dae_wrap_p) && (sval = xml->val)) {
      if ((en = dae_fxEnumWrap(sval)))
        samp->wrapP = en;
    } else if (xml_tag_eq(xml, _s_dae_minfilter) && (sval = xml->val)) {
      if ((en = dae_fxEnumMinfilter(sval)))
        samp->minfilter = en;
    } else if (xml_tag_eq(xml, _s_dae_magfilter) && (sval = xml->val)) {
      if ((en = dae_fxEnumMagfilter(sval)))
        samp->magfilter = en;
    } else if (xml_tag_eq(xml, _s_dae_mipfilter) && (sval = xml->val)) {
      if ((en = dae_fxEnumMipfilter(sval)))
        samp->mipfilter = en;
    } else if (xml_tag_eq(xml, _s_dae_border_color)) {
      AkColor *color;

      color = ak_heap_calloc(heap, samp, sizeof(*color));
      dae_color(xml, samp, true, false, color);
      
      samp->borderColor = color;
    } else if (xml_tag_eq(xml, _s_dae_mip_max_level)) {
      samp->mipMaxLevel = xml_uint64(xml, 0);
    } else if (xml_tag_eq(xml, _s_dae_mip_min_level)) {
      samp->mipMinLevel = xml_uint64(xml, 0);
    } else if (xml_tag_eq(xml, _s_dae_mip_bias)) {
      samp->mipBias = xml_uint64(xml, 0);
    } else if (xml_tag_eq(xml, _s_dae_max_anisotropy)) {
      samp->maxAnisotropy = xml_uint64(xml, 1l);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      samp->extra = tree_fromxml(heap, samp, xml);
    }
    xml = xml->val;
  }

  return samp;
}
