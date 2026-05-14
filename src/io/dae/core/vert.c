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
  double      profStart, profStep;
  
  profStart = DAE_PROF_START(dst);
  heap = dst->heap;
  vert = ak_heap_calloc(heap, memp, sizeof(*vert));
  xmla_setid(xml, heap, vert);
  
  vert->name = DAE_XMLA_STRDUP8(xml, heap, name, vert);
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, input)) {
      AkInput *inp;
      profStep = DAE_PROF_START(dst);
      
      inp              = ak_heap_calloc(heap, vert, sizeof(*inp));
      inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xml, semantic),
                                         heap,
                                         inp,
                                         &inp->semantic);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;
        
        url = DAE_URL_FROM(xml, source, memp);
        rb_insert(dst->inputmap, inp, url);
        
        inp->next   = vert->input;
        vert->input = inp;
        vert->inputCount++;
      }
      DAE_PROF_ACC(dst, profGeomInput, profGeomInputCount, profStep);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      vert->extra = tree_fromxml(heap, vert, xml);
    }
    xml = xml->next;
  }
  
  DAE_PROF_ACC(dst, profGeomVertices, profGeomVerticesCount, profStart);

  return vert;
}
