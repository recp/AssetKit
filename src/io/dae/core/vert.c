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

#include "vert.h"
#include "enum.h"

AK_HIDE
AkVertices*
dae_vert(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp) {
  AkHeap     *heap;
  AkVertices *vert;
  
  heap = dst->heap;
  vert = ak_heap_calloc(heap, memp, sizeof(*vert));
  xmla_setid(xml, heap, vert);
  
  vert->name = xmla_strdup_by(xml, heap, _s_dae_name, vert);
  
  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, vert, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;
        inp->semantic = dae_semantic(inp->semanticRaw);
        
        url = url_from(xml, _s_dae_source, memp);
        rb_insert(dst->inputmap, inp, url);
        
        inp->next   = vert->input;
        vert->input = inp;
        vert->inputCount++;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      vert->extra = tree_fromxml(heap, vert, xml);
    }
    xml = xml->next;
  }
  
  return vert;
}
