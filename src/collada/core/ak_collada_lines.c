/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_lines.h"
#include "ak_collada_enums.h"
#include "../../ak_array.h"

AkResult _assetkit_hide
ak_dae_lines(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkLineMode mode,
             AkLines ** __restrict dest) {
  AkLines *lines;
  AkInput *last_input;

  lines = ak_heap_calloc(xst->heap, memParent, sizeof(*lines));
  lines->mode = mode;
  lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;

  lines->base.name     = ak_xml_attr(xst, lines, _s_dae_name);
  lines->base.material = ak_xml_attr(xst, lines, _s_dae_material);
  lines->count         = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_lines))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, lines, sizeof(*input));

      input->base.semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);
      
      ak_xml_attr_url(xst->reader,
                       _s_dae_source,
                       input,
                       &input->base.source);

      if (!input->base.semanticRaw || !input->base.source.url)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = ak_dae_enumInputSemantic(input->base.semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->base.semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->base.next = &input->base;
      else
        lines->base.input = input;

      last_input = input;

      lines->base.inputCount++;

      /* attach vertices for convenience */
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
        lines->base.vertices = ak_getObjectByUrl(&input->base.source);

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
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          lines,
                          nodePtr,
                          &tree,
                          NULL);
      lines->base.extra = tree;

      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  *dest = lines;

  return AK_OK;
}
