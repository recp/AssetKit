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
ak_dae_edges(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkEdges ** __restrict dest) {
  AkEdges        *edges;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  edges = ak_heap_calloc(heap, memParent, sizeof(*edges), true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, edges->name, _s_dae_name);
  _xml_readAttrUsingFn(edges->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_edges);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(heap, edges, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(reader,
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

      _xml_readMutText(content);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(heap, edges, content, &doubleArray);
        if (ret == AK_OK) {
          edges->primitives = doubleArray;
        }

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, edges, nodePtr, &tree, NULL);
      edges->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = edges;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_wires(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkWires ** __restrict dest) {
  AkWires        *wires;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  wires = ak_heap_calloc(heap, memParent, sizeof(*wires), true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, wires->name, _s_dae_name);
  _xml_readAttrUsingFn(wires->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_wires);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(heap, wires, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(reader,
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
      _xml_readMutText(content);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(heap, wires, content, &intArray);
        if (ret == AK_OK)
          wires->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(heap, wires, content, &doubleArray);
        if (ret == AK_OK)
          wires->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, wires, nodePtr, &tree, NULL);
      wires->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = wires;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_faces(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkFaces ** __restrict dest) {
  AkFaces        *faces;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  faces = ak_heap_calloc(heap, memParent, sizeof(*faces), true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, faces->name, _s_dae_name);
  _xml_readAttrUsingFn(faces->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_faces);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(heap, faces, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(reader,
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
      _xml_readMutText(content);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(heap, faces, content, &intArray);
        if (ret == AK_OK)
          faces->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(heap, faces, content, &doubleArray);
        if (ret == AK_OK)
          faces->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, faces, nodePtr, &tree, NULL);
      faces->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = faces;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_pcurves(AkHeap * __restrict heap,
               void * __restrict memParent,
               xmlTextReaderPtr reader,
               AkPCurves ** __restrict dest) {
  AkPCurves      *pcurves;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  pcurves = ak_heap_calloc(heap, memParent, sizeof(*pcurves), true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, pcurves->name, _s_dae_name);
  _xml_readAttrUsingFn(pcurves->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_pcurves);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(heap, pcurves, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(reader,
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
      _xml_readMutText(content);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(heap, pcurves, content, &intArray);
        if (ret == AK_OK)
          pcurves->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(heap, pcurves, content, &doubleArray);
        if (ret == AK_OK)
          pcurves->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, pcurves, nodePtr, &tree, NULL);
      pcurves->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = pcurves;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_shells(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkShells ** __restrict dest) {
  AkShells       *shells;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  shells = ak_heap_calloc(heap, memParent, sizeof(*shells), true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, shells->name, _s_dae_name);
  _xml_readAttrUsingFn(shells->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_shells);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(heap, shells, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(reader,
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
      _xml_readMutText(content);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(heap, shells, content, &intArray);
        if (ret == AK_OK)
          shells->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(heap, shells, content, &doubleArray);
        if (ret == AK_OK)
          shells->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, shells, nodePtr, &tree, NULL);
      shells->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = shells;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_solids(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkSolids ** __restrict dest) {
  AkSolids       *solids;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  solids = ak_heap_calloc(heap, memParent, sizeof(*solids), true);

  _xml_readId(memParent);
  _xml_readAttr(memParent, solids->name, _s_dae_name);
  _xml_readAttrUsingFn(solids->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_solids);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_heap_calloc(heap, solids, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(reader,
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
      _xml_readMutText(content);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(heap, solids, content, &intArray);
        if (ret == AK_OK)
          solids->vcount = intArray;
        
        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;
      
      _xml_readMutText(content);
      
      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;
        
        ret = ak_strtod_array(heap, solids, content, &doubleArray);
        if (ret == AK_OK)
          solids->primitives = doubleArray;
        
        xmlFree(content);
      }
      
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;
      
      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;
      
      ak_tree_fromXmlNode(heap, solids, nodePtr, &tree, NULL);
      solids->extra = tree;
      
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = solids;
  
  return AK_OK;
}
