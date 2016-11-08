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
ak_dae_nurbs(AkDaeState * __restrict daestate,
             void * __restrict memParent,
             bool asObject,
             AkNurbs ** __restrict dest) {
  AkObject *obj;
  AkNurbs  *nurbs;
  AkSource *last_source;
  void     *memPtr;

  if (asObject) {
    obj = ak_objAlloc(daestate->heap,
                      memParent,
                      sizeof(*nurbs),
                      0,
                      true,
                      false);

    nurbs = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbs = ak_heap_calloc(daestate->heap,
                           memParent,
                           sizeof(*nurbs),
                           false);
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

      ret = ak_dae_source(daestate, memPtr,  &source);
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

      cverts = ak_heap_calloc(daestate->heap,
                              memPtr,
                              sizeof(*cverts),
                              false);

      last_input = NULL;

      do {
        _xml_beginElement(_s_dae_control_vertices);

        if (_xml_eqElm(_s_dae_input)) {
          AkInputBasic *input;

          input = ak_heap_calloc(daestate->heap,
                                 memPtr,
                                 sizeof(*input),
                                 false);

          _xml_readAttr(input, input->semanticRaw, _s_dae_semantic);

          ak_url_from_attr(daestate->reader,
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
            cverts->input = input;

          last_input = input;
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(daestate->reader);
          tree = NULL;

          ak_tree_fromXmlNode(daestate->heap,
                              memPtr,
                              nodePtr,
                              &tree,
                              NULL);
          cverts->extra = tree;

          _xml_skipElement;

        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (daestate->nodeRet);
      
      nurbs->cverts = cverts;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(daestate->reader);
      tree = NULL;

      ak_tree_fromXmlNode(daestate->heap,
                          memPtr,
                          nodePtr,
                          &tree,
                          NULL);
      nurbs->extra = tree;

      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);
  
  *dest = nurbs;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_nurbs_surface(AkDaeState * __restrict daestate,
                     void * __restrict memParent,
                     bool asObject,
                     AkNurbsSurface ** __restrict dest) {
  AkObject       *obj;
  AkNurbsSurface *nurbsSurface;
  AkSource       *last_source;
  void           *memPtr;

  if (asObject) {
    obj = ak_objAlloc(daestate->heap,
                      memParent,
                      sizeof(*nurbsSurface),
                      0,
                      true,
                      false);

    nurbsSurface = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbsSurface = ak_heap_calloc(daestate->heap,
                                  memParent,
                                  sizeof(*nurbsSurface),
                                  false);
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

      ret = ak_dae_source(daestate, memPtr, &source);
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

      cverts = ak_heap_calloc(daestate->heap,
                              memPtr,
                              sizeof(*cverts),
                              false);

      last_input = NULL;

      do {
        _xml_beginElement(_s_dae_control_vertices);

        if (_xml_eqElm(_s_dae_input)) {
          AkInputBasic *input;

          input = ak_heap_calloc(daestate->heap,
                                 memPtr,
                                 sizeof(*input),
                                 false);

          _xml_readAttr(input, input->semanticRaw, _s_dae_semantic);

          ak_url_from_attr(daestate->reader,
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
            cverts->input = input;

          last_input = input;
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(daestate->reader);
          tree = NULL;

          ak_tree_fromXmlNode(daestate->heap,
                              memPtr,
                              nodePtr,
                              &tree,
                              NULL);
          cverts->extra = tree;

          _xml_skipElement;

        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (daestate->nodeRet);

      nurbsSurface->cverts = cverts;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(daestate->reader);
      tree = NULL;

      ak_tree_fromXmlNode(daestate->heap,
                          memPtr,
                          nodePtr,
                          &tree,
                          NULL);
      nurbsSurface->extra = tree;
      
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);
  
  *dest = nurbsSurface;
  
  return AK_OK;
}
