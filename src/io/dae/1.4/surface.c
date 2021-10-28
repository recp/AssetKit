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

#include "surface.h"
#include "../core/asset.h"
#include "../core/enum.h"

AK_HIDE
AkDae14Surface*
dae14_surface(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp) {
  AkHeap         *heap;
  AkDae14Surface *surf;
  xml_attr_t     *att;
  const xml_t    *sval;

  heap = dst->heap;
  surf = ak_heap_calloc(heap, memp, sizeof(*surf));

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_init_from)) {
      AkDae14SurfaceFrom *initFrom;
      initFrom = ak_heap_calloc(heap, surf, sizeof(*heap));
      initFrom->mip   = xmla_u32(xmla(xml, _s_dae_mip),   0);
      initFrom->slice = xmla_u32(xmla(xml, _s_dae_slice), 0);
      
      if ((att = xmla(xml, _s_dae_face))) {
        initFrom->face = dae_face(att);
      } else {
        initFrom->face = AK_FACE_POSITIVE_Y;
      }

      initFrom->image = xml_strdup(xml, heap, initFrom);
      surf->initFrom  = initFrom;
    } else if (xml_tag_eq(xml, _s_dae_init_as_target)) {
      surf->initAsTarget = true; /* becuse the element exists */
    } else if (xml_tag_eq(xml, _s_dae_format)) {
      surf->format = xml_strdup(xml, heap, surf);
    } else if (xml_tag_eq(xml, _s_dae_format_hint)) {
      AkImageFormat *format;
      xml_t         *xfmt;
      
      format = ak_heap_calloc(heap, memp, sizeof(*format));
      
      xfmt = xml->val;
      while (xfmt) {
        if (xml_tag_eq(xfmt, _s_dae_channels) && (sval = xmls(xfmt))) {
          format->channel = dae_enumChannel(sval->val, sval->valsize);
        } else if (xml_tag_eq(xfmt, _s_dae_range) && (sval = xmls(xfmt))) {
          format->range = dae_range(sval->val, sval->valsize);
        } else if (xml_tag_eq(xfmt, _s_dae_precision) && (sval = xmls(xfmt))) {
          format->precision = dae_precision(sval->val, sval->valsize);
        } else if (xml_tag_eq(xfmt, _s_dae_option)) {
          format->space = xml_strdup(xml, heap, format);
        } else if (xml_tag_eq(xfmt, _s_dae_exact)) {
          format->exact = xml_strdup(xml, heap, format);
        }
        xfmt = xfmt->next;
      }
    } else if (xml_tag_eq(xml, _s_dae_size) && (sval = xmls(xml))) {
      AkUInt size[3];
      xml_strtoui_fast(sval, size, 3);
      
      surf->size.width  = size[0];
      surf->size.height = size[1];
      surf->size.depth  = size[2];
    } else if (xml_tag_eq(xml, _s_dae_viewport_ratio) && (sval = xmls(xml))) {
      xml_strtof_fast(sval, surf->viewportRatio, 2);
    } else if (xml_tag_eq(xml, _s_dae_mip_levels) && (sval = xmls(xml))) {
      surf->mipLevels = (int)strtol(sval->val, NULL, 10);
    } else if (xml_tag_eq(xml, _s_dae_mipmap_generate) && (sval = xmls(xml))) {
      surf->mipmapGenerate = (bool)strtol(sval->val, NULL, 10);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      surf->extra = tree_fromxml(heap, surf, xml);
    }
    xml = xml->next;
  }

  return surf;
}
