/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_lines.h"
#include "dae_enums.h"
#include "../../array.h"

AkResult _assetkit_hide
dae_lines(AkXmlState * __restrict xst,
          void     * __restrict   memParent,
          AkLineMode              mode,
          AkLines ** __restrict   dest) {
  AkLines      *lines;
  AkInput      *last_input;
  AkXmlElmState xest;
  uint32_t      indexoff;

  lines = ak_heap_calloc(xst->heap, memParent, sizeof(*lines));
  lines->mode = mode;
  lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;

  lines->base.name     = ak_xml_attr(xst, lines, _s_dae_name);
  lines->base.bindmaterial = ak_xml_attr(xst, lines, _s_dae_material);
  lines->base.count    = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;
  indexoff   = 0;

  ak_xest_init(xest, _s_dae_lines)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, lines, sizeof(*input));
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
            lines->base.input = input;

          last_input = input;

          lines->base.inputCount++;

          if (input->offset > indexoff)
            indexoff = input->offset;

          ak_xml_attr_url(xst, _s_dae_source, input, &input->source);
        } else {
          /* don't store VERTEX because it will be duplicated to all prims */
          lines->base.reserved1 = input->offset;
          lines->base.reserved2 = input->set;
          ak_free(input);
        }
      }

    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *uintArray;
        AkResult ret;

        ret = ak_strtoui_array(xst->heap, lines, content, &uintArray);
        if (ret == AK_OK)
          lines->base.indices = uintArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, lines, &lines->base.extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  lines->base.indexStride = indexoff + 1;

  *dest = lines;

  return AK_OK;
}
