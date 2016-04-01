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
ak_dae_lines(void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkLineMode mode,
             bool asObject,
             AkLines ** __restrict dest) {
  AkObject       *obj;
  AkLines        *lines;
  AkInput        *last_input;
  AkDoubleArrayL *last_array;
  void           *memPtr;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  if (asObject) {
    obj = ak_objAlloc(memParent,
                      sizeof(*lines),
                      0,
                      true);

    lines = ak_objGet(obj);
    memPtr = obj;
  } else {
    lines = ak_calloc(memParent, sizeof(*lines), 1);
    memPtr = lines;
  }

  lines->mode = mode;

  _xml_readAttr(memPtr, lines->name, _s_dae_name);
  _xml_readAttr(memPtr, lines->material, _s_dae_material);
  _xml_readAttrUsingFnWithDef(lines->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);

  last_input = NULL;
  last_array = NULL;

  do {
    _xml_beginElement(_s_dae_lines);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;

      input = ak_calloc(memPtr, sizeof(*input), 1);

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

      if (last_input)
        last_input->base.next = &input->base;
      else
        lines->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArrayL *doubleArray;
        AkResult ret;

        ret = ak_strtod_arrayL(memPtr, content, &doubleArray);
        if (ret == AK_OK) {
          if (last_array)
            last_array->next = doubleArray;
          else
            lines->primitives = doubleArray;

          last_array = doubleArray;
        }

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
      lines->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = lines;

  return AK_OK;
}
