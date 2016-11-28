/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_triangles.h"
#include "ak_collada_enums.h"
#include "../../ak_array.h"

AkResult _assetkit_hide
ak_dae_triangles(AkXmlState * __restrict xst,
                 void * __restrict memParent,
                 const char * elm,
                 AkTriangleMode mode,
                 AkTriangles ** __restrict dest) {
  AkTriangles *triangles;
  AkInput     *last_input;
  int          inputIndex;

  triangles = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*triangles));

  triangles->mode = mode;
  triangles->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;

  triangles->base.name     = ak_xml_attr(xst, triangles, _s_dae_name);
  triangles->base.material = ak_xml_attr(xst, triangles, _s_dae_material);
  triangles->count         = ak_xml_attrui64(xst, _s_dae_count);

  last_input = NULL;
  inputIndex = 0;

  do {
    if (ak_xml_beginelm(xst, elm))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, triangles, sizeof(*input));

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
      input->index  = inputIndex++;

      if (last_input)
        last_input->base.next = &input->base;
      else
        triangles->base.input = input;

      last_input = input;

      triangles->base.inputCount++;

      /* attach vertices for convenience */
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
        triangles->base.vertices = ak_getObjectByUrl(&input->base.source);
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
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;
      
      ak_tree_fromXmlNode(xst->heap,
                          triangles,
                          nodePtr,
                          &tree,
                          NULL);
      triangles->base.extra = tree;
      
      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }
    
    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  *dest = triangles;
  
  return AK_OK;
}
