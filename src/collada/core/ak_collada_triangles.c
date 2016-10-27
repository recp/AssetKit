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
ak_dae_triangles(AkHeap * __restrict heap,
                 void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 const char * elm,
                 AkTriangleMode mode,
                 AkTriangles ** __restrict dest) {
  AkDoc          *doc;
  AkTriangles    *triangles;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  doc       = ak_heap_attachment(heap);
  triangles = ak_heap_calloc(heap, memParent, sizeof(*triangles), false);
  triangles->mode = mode;
  triangles->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;

  _xml_readAttr(triangles, triangles->base.name, _s_dae_name);
  _xml_readAttr(triangles, triangles->base.material, _s_dae_material);
  _xml_readAttrUsingFnWithDef(triangles->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(heap, triangles, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);
      _xml_readAttr(input, input->base.source, _s_dae_source);

      if (!input->base.semanticRaw || !input->base.source)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = ak_dae_enumInputSemantic(input->base.semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->base.semantic = inputSemantic;
      }

      _xml_readAttrUsingFn(input->offset,
                           _s_dae_offset,
                           (AkUInt)strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           (AkUInt)strtoul, NULL, 10);

      if (last_input)
        last_input->base.next = &input->base;
      else
        triangles->base.input = input;

      last_input = input;

      triangles->base.inputCount++;

      /* attach vertices for convenience */
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
        triangles->base.vertices = ak_getObjectByUrl(doc, input->base.source);
    } else if (_xml_eqElm(_s_dae_p)) {
      AkUIntArray *uintArray;
      char *content;

      _xml_readMutText(content);
      if (content) {
        AkResult ret;
        ret = ak_strtoui_array(heap, triangles, content, &uintArray);
        if (ret == AK_OK)
          triangles->base.indices = uintArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;
      
      ak_tree_fromXmlNode(heap, triangles, nodePtr, &tree, NULL);
      triangles->base.extra = tree;
      
      _xml_skipElement;
    } else {
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = triangles;
  
  return AK_OK;
}
