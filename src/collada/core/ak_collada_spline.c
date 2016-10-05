/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_spline.h"
#include "ak_collada_source.h"
#include "ak_collada_enums.h"

AkResult _assetkit_hide
ak_dae_spline(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              bool asObject,
              AkSpline ** __restrict dest) {
  AkObject       *obj;
  AkSpline       *spline;
  AkSource       *last_source;
  void           *memPtr;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  if (asObject) {
    obj = ak_objAlloc(heap,
                      memParent,
                      sizeof(*spline),
                      AK_GEOMETRY_TYPE_SPLINE,
                      true,
                      false);

    spline = ak_objGet(obj);
    memPtr = obj;
  } else {
    spline = ak_heap_calloc(heap, memParent, sizeof(*spline), false);
    memPtr = spline;
  }

  _xml_readAttrUsingFnWithDef(spline->closed,
                              _s_dae_closed,
                              false,
                              (AkBool)strtoul, NULL, 10);

  last_source = NULL;

  do {
    _xml_beginElement(_s_dae_spline);

    if (_xml_eqElm(_s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(heap, memPtr, reader, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          spline->source = source;

        last_source = source;
      }
    } else if (_xml_eqElm(_s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInputBasic   *last_input;

      cverts = ak_heap_calloc(heap, memPtr, sizeof(*cverts), false);

      last_input = NULL;
      
      do {
        _xml_beginElement(_s_dae_control_vertices);

        if (_xml_eqElm(_s_dae_input)) {
          AkInputBasic *input;

          input = ak_heap_calloc(heap, memPtr, sizeof(*input), false);

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
            cverts->input = input;
          
          last_input = input;
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(heap, memPtr, nodePtr, &tree, NULL);
          cverts->extra = tree;
          
          _xml_skipElement;

        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);

      spline->cverts = cverts;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, memPtr, nodePtr, &tree, NULL);
      spline->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = spline;
  
  return AK_OK;
}
