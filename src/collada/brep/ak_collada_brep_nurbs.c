/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_brep_nurbs.h"
#include "../core/ak_collada_source.h"
#include "../core/ak_collada_enums.h"

AkResult _assetkit_hide
ak_dae_nurbs(void * __restrict memParent,
             xmlTextReaderPtr reader,
             bool asObject,
             AkNurbs ** __restrict dest) {
  AkObject       *obj;
  AkNurbs        *nurbs;
  AkSource       *last_source;
  void           *memPtr;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  if (asObject) {
    obj = ak_objAlloc(memParent,
                      sizeof(*nurbs),
                      0,
                      true);

    nurbs = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbs = ak_calloc(memParent, sizeof(*nurbs), 1);
    memPtr = nurbs;
  }

  _xml_readAttrUsingFn(nurbs->degree,
                       _s_dae_degree,
                       (AkUInt)strtoul, NULL, 10);

  _xml_readAttrUsingFnWithDef(nurbs->closed,
                              _s_dae_closed,
                              false,
                              (AkBool)strtoul, NULL, 10);

  last_source = NULL;

  do {
    _xml_beginElement(_s_dae_lines);

    if (_xml_eqElm(_s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(memPtr, reader, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          nurbs->source = source;

        last_source = source;
      }
    } else if (_xml_eqElm(_s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInputBasic   *last_input;

      cverts = ak_calloc(memPtr, sizeof(*cverts), 1);

      last_input = NULL;

      do {
        _xml_beginElement(_s_dae_control_vertices);

        if (_xml_eqElm(_s_dae_input)) {
          AkInputBasic *input;

          input = ak_calloc(memPtr, sizeof(*input), 1);

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

          ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
          cverts->extra = tree;

          _xml_skipElement;

        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
      
      nurbs->cverts = cverts;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
      nurbs->extra = tree;

      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = nurbs;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_nurbs_surface(void * __restrict memParent,
                     xmlTextReaderPtr reader,
                     bool asObject,
                     AkNurbsSurface ** __restrict dest) {
  AkObject       *obj;
  AkNurbsSurface *nurbsSurface;
  AkSource       *last_source;
  void           *memPtr;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  if (asObject) {
    obj = ak_objAlloc(memParent,
                      sizeof(*nurbsSurface),
                      0,
                      true);

    nurbsSurface = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbsSurface = ak_calloc(memParent, sizeof(*nurbsSurface), 1);
    memPtr = nurbsSurface;
  }

  _xml_readAttrUsingFn(nurbsSurface->degree_u,
                       _s_dae_degree_u,
                       (AkUInt)strtoul, NULL, 10);

  _xml_readAttrUsingFnWithDef(nurbsSurface->closed_u,
                              _s_dae_closed_u,
                              false,
                              (AkBool)strtoul, NULL, 10);

  _xml_readAttrUsingFn(nurbsSurface->degree_v,
                       _s_dae_degree_v,
                       (AkUInt)strtoul, NULL, 10);

  _xml_readAttrUsingFnWithDef(nurbsSurface->closed_v,
                              _s_dae_closed_v,
                              false,
                              (AkBool)strtoul, NULL, 10);

  last_source = NULL;

  do {
    _xml_beginElement(_s_dae_lines);

    if (_xml_eqElm(_s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(memPtr, reader, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          nurbsSurface->source = source;

        last_source = source;
      }
    } else if (_xml_eqElm(_s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInputBasic   *last_input;

      cverts = ak_calloc(memPtr, sizeof(*cverts), 1);

      last_input = NULL;

      do {
        _xml_beginElement(_s_dae_control_vertices);

        if (_xml_eqElm(_s_dae_input)) {
          AkInputBasic *input;

          input = ak_calloc(memPtr, sizeof(*input), 1);

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

          ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
          cverts->extra = tree;

          _xml_skipElement;

        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      nurbsSurface->cverts = cverts;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
      nurbsSurface->extra = tree;
      
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = nurbsSurface;
  
  return AK_OK;
}
