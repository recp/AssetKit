/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_vertices.h"
#include "ak_collada_enums.h"

AkResult _assetkit_hide
ak_dae_vertices(void * __restrict memParent,
                xmlTextReaderPtr reader,
                AkVertices ** __restrict dest) {
  AkVertices    *vertices;
  AkInputBasic  *last_input;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  vertices = ak_calloc(memParent, sizeof(*vertices), 1);

  _xml_readAttr(vertices, vertices->id, _s_dae_id);
  _xml_readAttr(vertices, vertices->name, _s_dae_name);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_vertices);

    if (_xml_eqElm(_s_dae_input)) {
      AkInputBasic *input;

      input = ak_calloc(vertices, sizeof(*input), 1);

      _xml_readAttr(input, input->semanticRaw, _s_dae_semantic);
      _xml_readAttr(input, input->source, _s_dae_source);

      if (!input->semanticRaw || !input->source)
        ak_free(input);
      else
        input->semantic = ak_dae_enumInputSemantic(input->semanticRaw);

      if (last_input)
        last_input->next = input;
      else
        vertices->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(vertices, nodePtr, &tree, NULL);
      vertices->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = vertices;

  return AK_OK;
}
