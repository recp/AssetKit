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
  AkHeap     *heap;
  AkDoc      *doc;
  AkImage    *img;
  AkInitFrom *initFrom;
  const char *format;
  void       *parent;

  heap = dst->heap;
  doc  = dst->doc;
  parent = memp ? memp : doc;

  img           = ak_heap_calloc(heap, parent, sizeof(*img));
  initFrom      = ak_heap_calloc(heap, img, sizeof(*img->initFrom));
  img->initFrom = initFrom;

  xmla_setid(xml, heap, img);
  
  img->name = DAE_XMLA_STRDUP8(xml, heap, name, img);

  format = DAE_XMLA_STRDUP8(xml, heap, format, img->initFrom);
  initFrom->mipsGenerate = false; /* 1.4's default, 1.5's is true */
  initFrom->depth        = xmla_u32(DAE_XMLA8(xml, depth), 0);
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, img, NULL);
    } else if (DAE_XML_TAG_EQ4(xml, data)) {
      AkHexData *hex;
      hex = ak_heap_calloc(heap, initFrom, sizeof(*hex));
      
      hex->format = format;
      ak_heap_setpm((void *)format, hex);
      
      if (hex->format) {
        hex->hexval        = xml_strdup(xml, heap, hex);
        img->initFrom->hex = hex;
      }
    } else if (DAE_XML_TAG_EQ(xml, init_from)) {
      img->initFrom->ref = xml_strdup(xml, heap, initFrom);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      img->extra = tree_fromxml(heap, img, xml);
    }
    xml = xml->next;
  }

  AK_LIB_PREPEND(doc->lib.images, img, next);
}
