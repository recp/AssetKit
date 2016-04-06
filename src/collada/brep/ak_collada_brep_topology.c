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
ak_dae_edges(void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkEdges ** __restrict dest) {
  AkEdges        *edges;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  edges = ak_calloc(memParent, sizeof(*edges), 1);

  _xml_readAttr(memParent, edges->id, _s_dae_id);
  _xml_readAttr(memParent, edges->name, _s_dae_name);
  _xml_readAttrUsingFn(edges->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_edges);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_calloc(edges, sizeof(*input), 1);

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
                           strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           strtoul, NULL, 10);

      if (last_input)
        last_input->base.next = &input->base;
      else
        edges->input = input;
      
      last_input = input;
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArrayL *doubleArray;
        AkResult ret;

        ret = ak_strtod_arrayL(edges, content, &doubleArray);
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

      ak_tree_fromXmlNode(edges, nodePtr, &tree, NULL);
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
ak_dae_wires(void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkWires ** __restrict dest) {
  AkWires        *wires;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  wires = ak_calloc(memParent, sizeof(*wires), 1);

  _xml_readAttr(memParent, wires->id, _s_dae_id);
  _xml_readAttr(memParent, wires->name, _s_dae_name);
  _xml_readAttrUsingFn(wires->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_wires);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_calloc(wires, sizeof(*input), 1);

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
                           strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           strtoul, NULL, 10);

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

        ret = ak_strtoi_array(wires, content, &intArray);
        if (ret == AK_OK)
          wires->vcount = intArray;
        
        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArrayL *doubleArray;
        AkResult ret;

        ret = ak_strtod_arrayL(wires, content, &doubleArray);
        if (ret == AK_OK)
          wires->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(wires, nodePtr, &tree, NULL);
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
ak_dae_faces(void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkFaces ** __restrict dest) {
  AkFaces        *faces;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  faces = ak_calloc(memParent, sizeof(*faces), 1);

  _xml_readAttr(memParent, faces->id, _s_dae_id);
  _xml_readAttr(memParent, faces->name, _s_dae_name);
  _xml_readAttrUsingFn(faces->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_faces);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_calloc(faces, sizeof(*input), 1);

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
                           strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           strtoul, NULL, 10);

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

        ret = ak_strtoi_array(faces, content, &intArray);
        if (ret == AK_OK)
          faces->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArrayL *doubleArray;
        AkResult ret;

        ret = ak_strtod_arrayL(faces, content, &doubleArray);
        if (ret == AK_OK)
          faces->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(faces, nodePtr, &tree, NULL);
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
ak_dae_pcurves(void * __restrict memParent,
               xmlTextReaderPtr reader,
               AkPCurves ** __restrict dest) {
  AkPCurves      *pcurves;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  pcurves = ak_calloc(memParent, sizeof(*pcurves), 1);

  _xml_readAttr(memParent, pcurves->id, _s_dae_id);
  _xml_readAttr(memParent, pcurves->name, _s_dae_name);
  _xml_readAttrUsingFn(pcurves->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_pcurves);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_calloc(pcurves, sizeof(*input), 1);

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
                           strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           strtoul, NULL, 10);

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

        ret = ak_strtoi_array(pcurves, content, &intArray);
        if (ret == AK_OK)
          pcurves->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArrayL *doubleArray;
        AkResult ret;

        ret = ak_strtod_arrayL(pcurves, content, &doubleArray);
        if (ret == AK_OK)
          pcurves->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(pcurves, nodePtr, &tree, NULL);
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
ak_dae_shells(void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkShells ** __restrict dest) {
  AkShells       *shells;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  shells = ak_calloc(memParent, sizeof(*shells), 1);

  _xml_readAttr(memParent, shells->id, _s_dae_id);
  _xml_readAttr(memParent, shells->name, _s_dae_name);
  _xml_readAttrUsingFn(shells->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_shells);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_calloc(shells, sizeof(*input), 1);

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
                           strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           strtoul, NULL, 10);

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

        ret = ak_strtoi_array(shells, content, &intArray);
        if (ret == AK_OK)
          shells->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArrayL *doubleArray;
        AkResult ret;

        ret = ak_strtod_arrayL(shells, content, &doubleArray);
        if (ret == AK_OK)
          shells->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(shells, nodePtr, &tree, NULL);
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
ak_dae_solids(void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkSolids ** __restrict dest) {
  AkSolids       *solids;
  AkInput        *last_input;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  solids = ak_calloc(memParent, sizeof(*solids), 1);

  _xml_readAttr(memParent, solids->id, _s_dae_id);
  _xml_readAttr(memParent, solids->name, _s_dae_name);
  _xml_readAttrUsingFn(solids->count,
                       _s_dae_count,
                       (AkUInt)strtoul, NULL, 10);

  last_input = NULL;

  do {
    _xml_beginElement(_s_dae_solids);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;
      input = ak_calloc(solids, sizeof(*input), 1);

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
                           strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           strtoul, NULL, 10);

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

        ret = ak_strtoi_array(solids, content, &intArray);
        if (ret == AK_OK)
          solids->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArrayL *doubleArray;
        AkResult ret;

        ret = ak_strtod_arrayL(solids, content, &doubleArray);
        if (ret == AK_OK)
          solids->primitives = doubleArray;

        xmlFree(content);
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(solids, nodePtr, &tree, NULL);
      solids->extra = tree;

      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = solids;
  
  return AK_OK;
}
