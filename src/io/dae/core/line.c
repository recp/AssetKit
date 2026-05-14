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

#include "line.h"
#include "enum.h"
#include "../../../array.h"

AK_HIDE
AkLines*
dae_lines(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp,
          AkLineMode            mode) {
  AkLines *lines;
  AkHeap  *heap;
  double   profStart, profStep;
  uint32_t indexoff;
  
  profStart = DAE_PROF_START(dst);
  heap  = dst->heap;
  lines = ak_heap_calloc(heap, memp, sizeof(*lines));
  
  lines->mode              = mode;
  lines->base.type         = AK_PRIMITIVE_LINES;

  lines->base.name         = DAE_XMLA_STRDUP8(xml, heap, name, lines);
  lines->base.bindmaterial = DAE_XMLA_STRDUP8(xml, heap, material, lines);
  lines->base.nPolygons    = xmla_u32(DAE_XMLA8(xml, count), 0);
  
  indexoff = 0;
  xml      = xml->val;
  
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, input)) {
      AkInput *inp;
      profStep = DAE_PROF_START(dst);
      
      inp              = ak_heap_calloc(heap, lines, sizeof(*inp));
      inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xml, semantic),
                                         heap,
                                         inp,
                                         &inp->semantic);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;

        inp->offset   = xmla_u32(DAE_XMLA8(xml, offset), 0);
        inp->set      = xmla_u32(DAE_XMLA4(xml, set),    0);

        url = DAE_URL_FROM(xml, source, memp);
        rb_insert(dst->inputmap, inp, url);

        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          inp->next         = lines->base.input;
          lines->base.input = inp;
          lines->base.inputCount++;

          if (inp->offset > indexoff)
            indexoff = inp->offset;
        } else {
          dae_vertmap_add(dst, inp, &lines->base);

          /* don't store VERTEX because it will be duplicated to all prims */
          // lines->base.reserved1 = inp->offset;
          // lines->base.reserved2 = inp->set;
          // ak_free(inp);
        }
      }
      DAE_PROF_ACC(dst, profGeomInput, profGeomInputCount, profStep);
    } else if (DAE_XML_TAG_EQ8(xml, p) && xml->val) {
      AkUIntArray *uintArray;
      AkResult     ret;
      uint32_t     indexStride;
      unsigned long expectedCount;
      profStep = DAE_PROF_START(dst);

      indexStride   = indexoff + 1;
      expectedCount = mode == AK_LINES
                      ? (unsigned long)lines->base.nPolygons * 2ul * indexStride
                      : 0ul;
      ret = expectedCount > 0
            ? xml_strtoui_arrayN(heap, lines, xml->val, expectedCount, &uintArray)
            : xml_strtoui_array(heap, lines, xml->val, &uintArray);
      if (ret == AK_OK)
        lines->base.indices = uintArray;
      DAE_PROF_ACC(dst, profGeomIndexArray, profGeomIndexArrayCount, profStep);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      lines->base.extra = tree_fromxml(heap, lines, xml);
    }
    xml = xml->next;
  }
  
  lines->base.indexStride = indexoff + 1;
  
  DAE_PROF_ACC(dst, profGeomLines, profGeomLinesCount, profStart);

  return lines;
}
