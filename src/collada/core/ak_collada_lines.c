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
ak_dae_lines(AkDaeState * __restrict daestate,
             void * __restrict memParent,
             AkLineMode mode,
             AkLines ** __restrict dest) {
  AkDoc   *doc;
  AkLines *lines;
  AkInput *last_input;

  doc   = ak_heap_data(daestate->heap);
  lines = ak_heap_calloc(daestate->heap, memParent, sizeof(*lines), false);
  lines->mode = mode;
  lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;

  _xml_readAttr(lines, lines->base.name, _s_dae_name);
  _xml_readAttr(lines, lines->base.material, _s_dae_material);
  _xml_readAttrUsingFnWithDef(lines->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_lines);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(daestate->heap, lines, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(daestate->reader,
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

      _xml_readAttrUsingFn(input->offset,
                           _s_dae_offset,
                           (AkUInt)strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           (AkUInt)strtoul, NULL, 10);

      if (last_input)
        last_input->base.next = &input->base;
      else
        lines->base.input = input;

      last_input = input;

      lines->base.inputCount++;

      /* attach vertices for convenience */
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
        lines->base.vertices = ak_getObjectByUrl(&input->base.source);

    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkUIntArray *uintArray;
        AkResult ret;
        
        ret = ak_strtoui_array(daestate->heap, lines, content, &uintArray);
        if (ret == AK_OK)
          lines->base.indices = uintArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(daestate->reader);
      tree = NULL;

      ak_tree_fromXmlNode(daestate->heap,
                          lines,
                          nodePtr,
                          &tree,
                          NULL);
      lines->base.extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);
  
  *dest = lines;

  return AK_OK;
}
