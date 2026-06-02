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

#include "image.h"
#include "../core/asset.h"
#include "../core/enum.h"

AK_HIDE
void
dae14_fxMigrateImg(DAEState * __restrict dst,
                   xml_t    * __restrict xml,
                   void     * __restrict memp) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImage       *img;
  AkImageSource *source;
  const char    *format;
  void          *parent;

  heap   = dst->heap;
  doc    = dst->doc;
  parent = memp ? memp : doc;

  img         = ak_heap_calloc(heap, parent, sizeof(*img));
  source      = ak_heap_calloc(heap, img, sizeof(*source));
  img->source = source;

  xmla_setid(xml, heap, img);
  
  img->name = DAE_XMLA_STRDUP8(xml, heap, name, img);

  format = DAE_XMLA_STRDUP8(xml, heap, format, img->source);
  source->generateMips = false; /* 1.4's default, 1.5's is true */
  source->depth        = xmla_u32(DAE_XMLA8(xml, depth), 0);
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, img, NULL);
    } else if (DAE_XML_TAG_EQ4(xml, data)) {
      AkHexData *hex;
      hex = ak_heap_calloc(heap, source, sizeof(*hex));
      
      hex->format = format;
      ak_heap_setpm((void *)format, hex);
      
      if (hex->format) {
        hex->hexval  = xml_strdup(xml, heap, hex);
        source->hex  = hex;
        source->type = AK_IMAGE_SOURCE_HEX;
      }
    } else if (DAE_XML_TAG_EQ(xml, init_from)) {
      source->uri  = xml_strdup(xml, heap, source);
      source->type = AK_IMAGE_SOURCE_URI;
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      img->extra = tree_fromxml(heap, img, xml);
    }
    xml = xml->next;
  }

  AK_LIB_PREPEND(doc->lib.images, img, next);
}
