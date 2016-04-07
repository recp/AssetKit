/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_morph.h"
#include "ak_collada_enums.h"
#include "../../ak_array.h"

AkResult _assetkit_hide
ak_dae_morph(void * __restrict memParent,
             xmlTextReaderPtr reader,
             bool asObject,
             AkMorph ** __restrict dest) {
  AkObject       *obj;
  AkMorph        *morph;
  AkSource       *last_source;
  void           *memPtr;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  if (asObject) {
    obj = ak_objAlloc(memParent,
                      sizeof(*morph),
                      0,
                      true);

    morph = ak_objGet(obj);

    memPtr = obj;
  } else {
    morph = ak_calloc(memParent, sizeof(*morph), 1);
    memPtr = morph;
  }

  _xml_readAttr(morph, morph->baseMesh, _s_dae_source);
  _xml_readAttrAsEnumWithDef(morph->method,
                             _s_dae_method,
                             ak_dae_enumMorphMethod,
                             AK_MORPH_METHOD_NORMALIZED);

  last_source = NULL;

  do {
    _xml_beginElement(_s_dae_morph);

    if (_xml_eqElm(_s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(morph, reader, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          morph->source = source;

        last_source = source;
      }
    } else if (_xml_eqElm(_s_dae_targets)) {
      AkTargets    *targets;
      AkInputBasic *last_input;

      targets = ak_calloc(morph, sizeof(*targets), 1);

      last_input = NULL;

      do {
        _xml_beginElement(_s_dae_targets);

        if (_xml_eqElm(_s_dae_input)) {
          AkInputBasic *input;

          input = ak_calloc(targets, sizeof(*input), 1);

          _xml_readAttr(input, input->semanticRaw, _s_dae_semantic);
          _xml_readAttr(input, input->source, _s_dae_source);

          if (!input->semanticRaw || !input->source)
            ak_free(input);
          else {
            AkEnum inputSemantic;
            inputSemantic = ak_dae_enumInputSemantic(input->semanticRaw);

            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_SEMANTIC_OTHER;

            input->semantic = inputSemantic;
          }

          if (last_input)
            last_input->next = input;
          else
            targets->input = input;

          last_input = input;
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(morph, nodePtr, &tree, NULL);
          morph->extra = tree;

          _xml_skipElement;

        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      morph->targets = targets;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;
      
      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;
      
      ak_tree_fromXmlNode(morph, nodePtr, &tree, NULL);
      morph->extra = tree;
      
      _xml_skipElement;
    } else {
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = morph;
  
  return AK_OK;
}
