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
#include "image.h"
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
    if (DAE_XML_TAG_EQ(xml, init_from)) {
      AkDae14SurfaceFrom *initFrom;
      initFrom = ak_heap_calloc(heap, surf, sizeof(*initFrom));
      initFrom->mip   = xmla_u32(DAE_XMLA4(xml, mip),   0);
      initFrom->slice = xmla_u32(DAE_XMLA8(xml, slice), 0);
      
      if ((att = DAE_XMLA4(xml, face))) {
        initFrom->face = dae_face(att);
      } else {
        initFrom->face = AK_FACE_POSITIVE_Y;
      }

      initFrom->image = dae14_init_from_uri(heap, initFrom, xml);
      surf->initFrom  = initFrom;
    } else if (DAE_XML_TAG_EQ(xml, init_as_target)) {
      surf->initAsTarget = true; /* becuse the element exists */
    } else if (DAE_XML_TAG_EQ8(xml, format)) {
      surf->format = xml_strdup(xml, heap, surf);
    } else if (DAE_XML_TAG_EQ(xml, format_hint)) {
      AkImageFormat *format;
      xml_t         *xfmt;
      
      format = ak_heap_calloc(heap, memp, sizeof(*format));
      
      xfmt = xml->val;
      while (xfmt) {
        if (DAE_XML_TAG_EQ8(xfmt, channels) && (sval = xmls(xfmt))) {
          format->channel = dae_enumChannel(sval->val, sval->valsize);
        } else if (DAE_XML_TAG_EQ8(xfmt, range) && (sval = xmls(xfmt))) {
          format->range = dae_range(sval->val, sval->valsize);
        } else if (DAE_XML_TAG_EQ(xfmt, precision) && (sval = xmls(xfmt))) {
          format->precision = dae_precision(sval->val, sval->valsize);
        } else if (DAE_XML_TAG_EQ8(xfmt, option)) {
          format->space = xml_strdup(xml, heap, format);
        } else if (DAE_XML_TAG_EQ8(xfmt, exact)) {
          format->exact = xml_strdup(xml, heap, format);
        }
        xfmt = xfmt->next;
      }
    } else if (DAE_XML_TAG_EQ4(xml, size) && (sval = xmls(xml))) {
      AkUInt size[3];
      xml_strtoui_fast(sval, size, 3);
      
      surf->size.width  = size[0];
      surf->size.height = size[1];
      surf->size.depth  = size[2];
    } else if (DAE_XML_TAG_EQ(xml, viewport_ratio) && (sval = xmls(xml))) {
      xml_strtof_fast(sval, surf->viewportRatio, 2);
    } else if (DAE_XML_TAG_EQ(xml, mip_levels) && (sval = xmls(xml))) {
      surf->mipLevels = (int)xml__parse_int64(xml__value_begin(sval),
                                               xml__value_end(sval),
                                               0);
    } else if (DAE_XML_TAG_EQ(xml, mipmap_generate) && (sval = xmls(xml))) {
      surf->mipmapGenerate = (bool)xml__parse_int64(xml__value_begin(sval),
                                                    xml__value_end(sval),
                                                    0);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      surf->extra = tree_fromxml(heap, surf, xml);
    }
    xml = xml->next;
  }

  return surf;
}
