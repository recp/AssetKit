/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_brep_topology.h"
#include "../core/ak_collada_enums.h"
#include "../../ak_array.h"

AkResult _assetkit_hide
ak_dae_edges(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkEdges ** __restrict dest) {
  AkEdges *edges;
  AkInput *last_input;

  edges = ak_heap_calloc(xst->heap,
                         memParent,
                         sizeof(*edges),
                         true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, edges->name, _s_dae_name);
  _xml_readAttrUsingFn(edges->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_edges))
      break;

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(xst->heap,
                             edges,
                             sizeof(*input),
                             false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(xst->reader,
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
        edges->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(xst->heap,
                              edges,
                              content,
                              &doubleArray);
        if (ret == AK_OK) {
          edges->primitives = doubleArray;
        }

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          edges,
                          nodePtr,
                          &tree,
                          NULL);
      edges->extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = edges;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_wires(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkWires ** __restrict dest) {
  AkWires *wires;
  AkInput *last_input;

  wires = ak_heap_calloc(xst->heap,
                         memParent,
                         sizeof(*wires),
                         true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, wires->name, _s_dae_name);
  _xml_readAttrUsingFn(wires->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_wires))
      break;

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(xst->heap,
                             wires,
                             sizeof(*input),
                             false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(xst->reader,
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
        wires->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(xst->heap,
                              wires,
                              content,
                              &intArray);
        if (ret == AK_OK)
          wires->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(xst->heap,
                              wires,
                              content,
                              &doubleArray);
        if (ret == AK_OK)
          wires->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          wires,
                          nodePtr,
                          &tree,
                          NULL);
      wires->extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = wires;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_faces(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkFaces ** __restrict dest) {
  AkFaces *faces;
  AkInput *last_input;

  faces = ak_heap_calloc(xst->heap,
                         memParent,
                         sizeof(*faces),
                         true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, faces->name, _s_dae_name);
  _xml_readAttrUsingFn(faces->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_faces))
      break;

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(xst->heap,
                             faces,
                             sizeof(*input),
                             false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(xst->reader,
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
        faces->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(xst->heap,
                              faces,
                              content,
                              &intArray);
        if (ret == AK_OK)
          faces->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(xst->heap,
                              faces,
                              content,
                              &doubleArray);
        if (ret == AK_OK)
          faces->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          faces,
                          nodePtr,
                          &tree,
                          NULL);
      faces->extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = faces;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_pcurves(AkXmlState * __restrict xst,
               void * __restrict memParent,
               AkPCurves ** __restrict dest) {
  AkPCurves *pcurves;
  AkInput   *last_input;

  pcurves = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*pcurves),
                           true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, pcurves->name, _s_dae_name);
  _xml_readAttrUsingFn(pcurves->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_pcurves))
      break;

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(xst->heap,
                             pcurves,
                             sizeof(*input),
                             false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(xst->reader,
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
        pcurves->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(xst->heap,
                              pcurves,
                              content,
                              &intArray);
        if (ret == AK_OK)
          pcurves->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(xst->heap,
                              pcurves,
                              content,
                              &doubleArray);
        if (ret == AK_OK)
          pcurves->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          pcurves,
                          nodePtr,
                          &tree,
                          NULL);
      pcurves->extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = pcurves;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_shells(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkShells ** __restrict dest) {
  AkShells *shells;
  AkInput  *last_input;

  shells = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*shells),
                          true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, shells->name, _s_dae_name);
  _xml_readAttrUsingFn(shells->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_shells))
      break;

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(xst->heap,
                             shells,
                             sizeof(*input),
                             false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(xst->reader,
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
        shells->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(xst->heap,
                              shells,
                              content,
                              &intArray);
        if (ret == AK_OK)
          shells->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(xst->heap,
                              shells,
                              content,
                              &doubleArray);
        if (ret == AK_OK)
          shells->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          shells,
                          nodePtr,
                          &tree,
                          NULL);
      shells->extra = tree;

      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = shells;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_solids(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkSolids ** __restrict dest) {
  AkSolids *solids;
  AkInput  *last_input;

  solids = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*solids),
                          true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, solids->name, _s_dae_name);
  _xml_readAttrUsingFn(solids->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_solids))
      break;

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(xst->heap,
                             solids,
                             sizeof(*input),
                             false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(xst->reader,
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
        solids->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(xst->heap,
                              solids,
                              content,
                              &intArray);
        if (ret == AK_OK)
          solids->vcount = intArray;
        
        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;
      
      content = ak_xml_rawval(xst);
      
      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;
        
        ret = ak_strtod_array(xst->heap,
                              solids,
                              content,
                              &doubleArray);
        if (ret == AK_OK)
          solids->primitives = doubleArray;
        
        xmlFree(content);
      }
      
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;
      
      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;
      
      ak_tree_fromXmlNode(xst->heap,
                          solids,
                          nodePtr,
                          &tree,
                          NULL);
      solids->extra = tree;
      
      ak_xml_skipelm(xst);;
    }
    
    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = solids;
  
  return AK_OK;
}
