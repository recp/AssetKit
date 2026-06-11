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

#include "triangle.h"
#include "enum.h"
#include "../../../array.h"

AK_HIDE
AkTriangles*
dae_triangles(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp,
              AkTriangleMode        mode,
              AkVertices * __restrict fallbackVertices) {
  AkTriangles *tri;
  AkHeap      *heap;
  uint32_t     indexoff;

  heap = dst->heap;
  tri  = ak_heap_calloc(heap, memp, sizeof(*tri));
  
  tri->mode      = mode;
  tri->base.type = AK_PRIMITIVE_TRIANGLES;

  tri->base.name         = DAE_XMLA_STRDUP8(xml, heap, name, tri);
  tri->base.bindmaterial = DAE_XMLA_STRDUP8(xml, heap, material, tri);
  tri->base.nPolygons    = xmla_u32(DAE_XMLA8(xml, count), 0);

  indexoff = 0;
  xml      = xml->val;

  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, input)) {
      AkInput *inp;

      inp              = dae_input_new(heap, tri);
      inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xml, semantic),
                                         heap,
                                         inp,
                                         &inp->semantic);

      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;

        inp->indexOffset = xmla_u32(DAE_XMLA8(xml, offset), 0);
        inp->set         = xmla_u32(DAE_XMLA4(xml, set),    0);

        url = DAE_URL_FROM(dst, xml, source, memp);
        rb_insert(dst->inputmap, inp, url);

        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          inp->next       = tri->base.input;
          tri->base.input = inp;
          tri->base.inputCount++;

          if (inp->indexOffset > indexoff)
            indexoff = inp->indexOffset;
        } else {
          dae_vertmap_add(dst, inp, &tri->base, fallbackVertices);
          if (inp->indexOffset > indexoff)
            indexoff = inp->indexOffset;

          /* don't store VERTEX because it will be duplicated to all prims */
        }
      }
    } else if (DAE_XML_TAG_EQ8(xml, p) && xml->val) {
      AkIndexArray *indexArray;
      AkResult     ret;
      AkUInt       maxIndex;
      uint32_t     indexStride;
      unsigned long expectedCount;
      
      indexStride   = indexoff + 1;
      expectedCount = mode == AK_TRIANGLES
                      ? (unsigned long)tri->base.nPolygons * 3ul * indexStride
                      : 0ul;
      maxIndex      = 0;
      ret = expectedCount > 0
            ? xml_strtoindex_arrayN_max(heap,
                                         tri,
                                         xml->val,
                                         expectedCount,
                                         &indexArray,
                                         &maxIndex)
            : xml_strtoindex_array_max(heap,
                                        tri,
                                        xml->val,
                                        &indexArray,
                                        &maxIndex);
      if (ret == AK_OK) {
        tri->base.indices = indexArray;
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      tri->base.extra = tree_fromxml(heap, tri, xml);
      if (tri->base.extra)
        ak_extra_set(&tri->base, tri->base.extra);
    }
    xml = xml->next;
  }

  tri->base.indexStride = indexoff + 1;
  
  return tri;
}
