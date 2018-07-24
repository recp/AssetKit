/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_triangles.h"
#include "dae_enums.h"
#include "../../array.h"

AkResult _assetkit_hide
dae_triangles(AkXmlState   * __restrict xst,
              void         * __restrict memParent,
              const char               *elm,
              AkTriangleMode            mode,
              AkTriangles ** __restrict dest) {
  AkTriangles  *triangles;
  AkInput      *last_input;
  AkXmlElmState xest;
  uint32_t      indexoff;

  triangles = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*triangles));

  triangles->mode = mode;
  triangles->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;

  triangles->base.name     = ak_xml_attr(xst, triangles, _s_dae_name);
  triangles->base.material = ak_xml_attr(xst, triangles, _s_dae_material);
  triangles->base.count    = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;
  indexoff   = 0;

  ak_xest_init(xest, elm)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, triangles, sizeof(*input));
      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = dae_enumInputSemantic(input->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;
        input->offset   = ak_xml_attrui(xst, _s_dae_offset);
        input->set      = ak_xml_attrui(xst, _s_dae_set);

        if ((uint32_t)input->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          if (last_input)
            last_input->next = input;
          else
            triangles->base.input = input;

          last_input = input;

          triangles->base.inputCount++;

          if (input->offset > indexoff)
            indexoff = input->offset;

          ak_xml_attr_url(xst, _s_dae_source, input, &input->source);
        } else {
          /* don't store VERTEX because it will be duplicated to all prims */
          triangles->base.reserved1 = input->offset;
          triangles->base.reserved2 = input->set;
          ak_free(input);
        }
      }
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      AkUIntArray *uintArray;
      char *content;

      content = ak_xml_rawval(xst);
      if (content) {
        AkResult ret;
        ret = ak_strtoui_array(xst->heap, triangles, content, &uintArray);
        if (ret == AK_OK)
          triangles->base.indices = uintArray;

        xmlFree(content);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, triangles, &triangles->base.extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  triangles->base.indexStride = indexoff + 1;

  *dest = triangles;

  return AK_OK;
}
