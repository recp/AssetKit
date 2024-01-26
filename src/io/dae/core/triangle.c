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
              AkTriangleMode        mode) {
  AkTriangles *tri;
  AkHeap      *heap;
  uint32_t     indexoff;

  heap = dst->heap;
  tri  = ak_heap_calloc(heap, memp, sizeof(*tri));
  
  tri->mode      = mode;
  tri->base.type = AK_PRIMITIVE_TRIANGLES;

  tri->base.name         = xmla_strdup_by(xml, heap, _s_dae_name, tri);
  tri->base.bindmaterial = xmla_strdup_by(xml, heap, _s_dae_material, tri);
  tri->base.nPolygons    = xmla_u32(xmla(xml, _s_dae_count), 0);

  indexoff = 0;
  xml      = xml->val;

  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;

      inp              = ak_heap_calloc(heap, tri, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);

      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;
        AkEnum inputSemantic;

        inputSemantic = dae_semantic(inp->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_OTHER;

        inp->semantic = inputSemantic;
        inp->offset   = xmla_u32(xmla(xml, _s_dae_offset), 0);
        inp->set      = xmla_u32(xmla(xml, _s_dae_set),    0);

        url = url_from(xml, _s_dae_source, memp);
        rb_insert(dst->inputmap, inp, url);

        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          inp->semantic   = dae_semantic(inp->semanticRaw);
          inp->next       = tri->base.input;
          tri->base.input = inp;
          tri->base.inputCount++;

          if (inp->offset > indexoff)
            indexoff = inp->offset;
        } else {
          dae_vertmap_add(dst, inp, &tri->base);

          /* don't store VERTEX because it will be duplicated to all prims */
          // tri->base.reserved1 = inp->offset;
          // tri->base.reserved2 = inp->set;
          // ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_p) && xml->val) {
      AkUIntArray *uintArray;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, tri, xml->val, &uintArray);
      if (ret == AK_OK)
        tri->base.indices = uintArray;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      tri->base.extra = tree_fromxml(heap, tri, xml);
    }
    xml = xml->next;
  }

  tri->base.indexStride = indexoff + 1;
  
  return tri;
}
