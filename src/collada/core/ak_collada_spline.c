/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_spline.h"

AkResult _assetkit_hide
ak_dae_spline(void * __restrict memParent,
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
    obj = ak_objAlloc(memParent,
                      sizeof(*spline),
                      0,
                      true);

    spline = ak_objGet(obj);
    memPtr = obj;
  } else {
    spline = ak_calloc(memParent, sizeof(*spline), 1);
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

      ret = ak_dae_source(memPtr, reader, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          spline->source = source;

        last_source = source;
      }
    } else if (_xml_eqElm(_s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInput        *last_input;

      cverts = ak_calloc(memPtr, sizeof(*cverts), 1);

      last_input = NULL;
      
      do {
        _xml_beginElement(_s_dae_control_vertices);

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
            cverts->input = input;
          
          last_input = input;
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
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

      ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
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
