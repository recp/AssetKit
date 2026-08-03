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

#include "colortex.h"
#include "../core/param.h"
#include "../core/color.h"
#include "../core/enum.h"
#include "../core/techn.h"
#include "../../../string_fast.h"

static
bool
dae_textureProfileEq(const xml_attr_t * __restrict attr,
                     const char       * __restrict value,
                     size_t                        len) {
  return attr && ak_str_eq_fast(attr->val, attr->valsize, value, len);
}

static
void
dae_textureVendorExtra(DAEState        * __restrict dst,
                       xml_t           * __restrict extra,
                       AkDAETextureRef * __restrict tex) {
  AkHeap             *heap;
  AkDAETextureVendor *vendor;
  xml_t      *technique;
  xml_t      *item;
  xml_attr_t *profile;
  bool        maya;
  bool        max3d;

  if (!dst || !extra || !tex)
    return;

  heap = dst->heap;

  /* The complete extension remains available through ak_extra(texref),
     even when only part of it has a canonical AssetKit representation. */
  dae_techn_append_extra(extra, heap, tex);

  vendor = dae_textureVendor(dst, tex);
  for (technique = extra->val; technique; technique = technique->next) {
    if (!DAE_XML_TAG_EQ(technique, technique))
      continue;

    profile = DAE_XMLA8(technique, profile);
    maya = dae_textureProfileEq(profile, _s_dae_maya, _s_dae_maya_len);
    max3d = dae_textureProfileEq(profile, _s_dae_max3d, _s_dae_max3d_len);
    if (!maya && !max3d)
      continue;

    for (item = technique->val; item; item = item->next) {
      if (!vendor) {
        bool supported;

        supported = (maya
                     && (DAE_XML_TAG_EQ(item, wrapU)
                         || DAE_XML_TAG_EQ(item, wrapV)
                         || DAE_XML_TAG_EQ(item, mirrorU)
                         || DAE_XML_TAG_EQ(item, mirrorV)
                         || DAE_XML_TAG_EQ(item, repeatU)
                         || DAE_XML_TAG_EQ(item, repeatV)
                         || DAE_XML_TAG_EQ(item, offsetU)
                         || DAE_XML_TAG_EQ(item, offsetV)
                         || DAE_XML_TAG_EQ(item, rotateUV)))
                    || (max3d && DAE_XML_TAG_EQ(item, amount));
        if (!supported)
          continue;

        vendor = ak_heap_calloc(heap, tex, sizeof(*vendor));
        if (!vendor)
          return;
        if (!dst->textureVendorMap)
          dst->textureVendorMap = rb_newtree_ptr();
        if (!dst->textureVendorMap)
          return;
        rb_insert(dst->textureVendorMap, tex, vendor);
      }

      if (maya && DAE_XML_TAG_EQ(item, wrapU)) {
        vendor->wrapU = xml_bool(item, true);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_WRAP_U;
      } else if (maya && DAE_XML_TAG_EQ(item, wrapV)) {
        vendor->wrapV = xml_bool(item, true);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_WRAP_V;
      } else if (maya && DAE_XML_TAG_EQ(item, mirrorU)) {
        vendor->mirrorU = xml_bool(item, false);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_MIRROR_U;
      } else if (maya && DAE_XML_TAG_EQ(item, mirrorV)) {
        vendor->mirrorV = xml_bool(item, false);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_MIRROR_V;
      } else if (maya && DAE_XML_TAG_EQ(item, repeatU)) {
        vendor->repeatU = xml_float(item, 1.0f);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_REPEAT_U;
      } else if (maya && DAE_XML_TAG_EQ(item, repeatV)) {
        vendor->repeatV = xml_float(item, 1.0f);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_REPEAT_V;
      } else if (maya && DAE_XML_TAG_EQ(item, offsetU)) {
        vendor->offsetU = xml_float(item, 0.0f);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_OFFSET_U;
      } else if (maya && DAE_XML_TAG_EQ(item, offsetV)) {
        vendor->offsetV = xml_float(item, 0.0f);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_OFFSET_V;
      } else if (maya && DAE_XML_TAG_EQ(item, rotateUV)) {
        vendor->rotateUV = xml_float(item, 0.0f);
        vendor->vendorMask |= AK_DAE_TEXTURE_HAS_ROTATE_UV;
      } else if (max3d && DAE_XML_TAG_EQ(item, amount)) {
        vendor->weight = xml_float(item, 1.0f);
        vendor->hasWeight = true;
      }
    }
  }
}

AK_HIDE
void
dae_colorOrTexSet(DAEState    * __restrict dst,
                  xml_t       * __restrict xml,
                  void        * __restrict memp,
                  AkColorDesc * __restrict clr) {
  AkHeap      *heap;

  heap = dst->heap;
  xml  = xml->val;

  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, color)) {
      clr->color = ak_heap_aligned_calloc(heap,
                                          memp,
                                          AK_ALIGNOF(AkColor),
                                          sizeof(*clr->color));
      dae_color(xml, clr->color, true, false, clr->color);
    } else if (DAE_XML_TAG_EQ8(xml, texture)) {
      AkDAETextureRef *tex;

      tex = ak_heap_calloc(heap, memp, sizeof(*tex));
      ak_setypeid(tex, AKT_TEXTURE);

      tex->texture  = xmla_strdup(DAE_XMLA8(xml, texture),  heap, tex);
      tex->texcoord = xmla_strdup(DAE_XMLA8(xml, texcoord), heap, tex);

      if (xml->val) {
        xml_t *child;

        for (child = xml->val; child; child = child->next) {
          if (DAE_XML_TAG_EQ8(child, extra))
            dae_textureVendorExtra(dst, child, tex);
        }
      }

      if (tex->texture)
        ak_setypeid((void *)tex->texture, AKT_TEXTURE_NAME);

      if (tex->texcoord)
        ak_setypeid((void *)tex->texcoord, AKT_TEXCOORD);

      rb_insert(dst->texmap, clr, tex);
    } else if (DAE_XML_TAG_EQ8(xml, param)) {
      AkParam *param;

      if ((param = dae_param(dst, xml, clr))) {
        if (clr->param)
          clr->param->prev = param;

        param->next = clr->param;
        clr->param  = param;
      }
    }
    xml = xml->next;
  }
}
