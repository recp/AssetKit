/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_morph.h"
#include "ak_collada_source.h"
#include "ak_collada_enums.h"
#include "../../ak_array.h"

AkResult _assetkit_hide
ak_dae_morph(AkXmlState * __restrict xst,
             void * __restrict memParent,
             bool asObject,
             AkMorph ** __restrict dest) {
  AkObject *obj;
  AkMorph  *morph;
  AkSource *last_source;
  void     *memPtr;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*morph),
                      0,
                      true,
                      false);

    morph = ak_objGet(obj);

    memPtr = obj;
  } else {
    morph = ak_heap_calloc(xst->heap, memParent, sizeof(*morph), false);
    memPtr = morph;
  }

  _xml_readAttr(memPtr, morph->baseMesh, _s_dae_source);
  _xml_readAttrAsEnumWithDef(morph->method,
                             _s_dae_method,
                             ak_dae_enumMorphMethod,
                             AK_MORPH_METHOD_NORMALIZED);

  last_source = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_morph))
      break;

    if (_xml_eqElm(_s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(xst, memPtr, &source);
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

      targets = ak_heap_calloc(xst->heap,
                               memPtr,
                               sizeof(*targets),
                               false);

      last_input = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_targets))
            break;

        if (_xml_eqElm(_s_dae_input)) {
          AkInputBasic *input;

          input = ak_heap_calloc(xst->heap,
                                 targets,
                                 sizeof(*input),
                                 false);

          _xml_readAttr(input, input->semanticRaw, _s_dae_semantic);

          ak_url_from_attr(xst->reader,
                           _s_dae_source,
                           input,
                           &input->source);

          if (!input->semanticRaw || !input->source.url)
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

          nodePtr = xmlTextReaderExpand(xst->reader);
          tree = NULL;

          ak_tree_fromXmlNode(xst->heap,
                              memPtr,
                              nodePtr,
                              &tree,
                              NULL);
          morph->extra = tree;

          ak_xml_skipelm(xst);;

        } else {
          ak_xml_skipelm(xst);;
        }

        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);

      morph->targets = targets;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;
      
      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;
      
      ak_tree_fromXmlNode(xst->heap,
                          memPtr,
                          nodePtr,
                          &tree,
                          NULL);
      morph->extra = tree;
      
      ak_xml_skipelm(xst);;
    } else {
      ak_xml_skipelm(xst);;
    }
    
    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = morph;
  
  return AK_OK;
}
