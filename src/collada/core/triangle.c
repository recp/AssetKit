/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "triangle.h"
#include "enum.h"
#include "../../array.h"

_assetkit_hide
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
  tri->base.bindmaterial = xmla_strdup_by(xml, heap, _s_dae_name, tri);
  tri->base.count        = xmla_uint32(xml_attr(xml, _s_dae_count), 0);
  
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

        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xml_attr(xml, _s_dae_offset), 0);
        inp->set      = xmla_uint32(xml_attr(xml, _s_dae_set),    0);

        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
        
          inp->next       = tri->base.input;
          tri->base.input = inp;
          tri->base.inputCount++;

          if (inp->offset > indexoff)
            indexoff = inp->offset;
          
          url = url_from(xml, _s_dae_source, memp);
          rb_insert(dst->inputmap, inp, url);
        } else {
          /* don't store VERTEX because it will be duplicated to all prims */
          tri->base.reserved1 = inp->offset;
          tri->base.reserved2 = inp->set;
          ak_free(inp);
        }

        url = url_from(xml, _s_dae_source, memp);
        rb_insert(dst->inputmap, inp, url);

        inp->next       = tri->base.input;
        tri->base.input = inp;
        tri->base.inputCount++;
      }
    } else if (xml_tag_eq(xml, _s_dae_p)) {
      AkUIntArray *uintArray;
      char        *content;
      
      if ((content = xml->val)) {
        AkResult ret;
        ret = ak_strtoui_array(heap, tri, content, &uintArray);
        if (ret == AK_OK)
          tri->base.indices = uintArray;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      tri->base.extra = tree_fromxml(heap, tri, xml);
    }
    xml = xml->next;
  }

  tri->base.indexStride = indexoff + 1;
  
  return tri;
}
