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
  uint32_t indexoff;
  
  heap  = dst->heap;
  lines = ak_heap_calloc(heap, memp, sizeof(*lines));
  
  lines->mode              = mode;
  lines->base.type         = AK_PRIMITIVE_LINES;

  lines->base.name         = xmla_strdup_by(xml, heap, _s_dae_name, lines);
  lines->base.bindmaterial = xmla_strdup_by(xml, heap, _s_dae_material, lines);
  lines->base.count        = xmla_u32(xmla(xml, _s_dae_count), 0);
  
  indexoff = 0;
  xml      = xml->val;
  
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, lines, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;
        
        inputSemantic = dae_semantic(inp->semanticRaw);
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_u32(xmla(xml, _s_dae_offset), 0);
        inp->set      = xmla_u32(xmla(xml, _s_dae_set),    0);
        
        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_semantic(inp->semanticRaw);
          
          inp->next       = lines->base.input;
          lines->base.input = inp;
          lines->base.inputCount++;
          
          if (inp->offset > indexoff)
            indexoff = inp->offset;
          
          url = url_from(xml, _s_dae_source, memp);
          rb_insert(dst->inputmap, inp, url);
        } else {
          /* don't store VERTEX because it will be duplicated to all prims */
          lines->base.reserved1 = inp->offset;
          lines->base.reserved2 = inp->set;
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_p) && xml->val) {
      AkUIntArray *uintArray;
      AkResult     ret;

      ret = xml_strtoui_array(heap, lines, xml->val, &uintArray);
      if (ret == AK_OK)
        lines->base.indices = uintArray;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      lines->base.extra = tree_fromxml(heap, lines, xml);
    }
    xml = xml->next;
  }
  
  lines->base.indexStride = indexoff + 1;
  
  return lines;
}
