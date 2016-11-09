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
ak_dae_nurbs(AkXmlState * __restrict xst,
             void * __restrict memParent,
             bool asObject,
             AkNurbs ** __restrict dest) {
  AkObject *obj;
  AkNurbs  *nurbs;
  AkSource *last_source;
  void     *memPtr;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*nurbs),
                      0,
                      true,
                      false);

    nurbs = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbs = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*nurbs),
                           false);
    memPtr = nurbs;
  }

  nurbs->degree = ak_xml_attrui(xst, _s_dae_degree);
  nurbs->closed = ak_xml_attrui_def(xst, _s_dae_closed, false);

  last_source = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_lines))
      break;

    if (ak_xml_eqelm(xst, _s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(xst, memPtr,  &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          nurbs->source = source;

        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInputBasic   *last_input;

      cverts = ak_heap_calloc(xst->heap,
                              memPtr,
                              sizeof(*cverts),
                              false);

      last_input = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_control_vertices))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInputBasic *input;

          input = ak_heap_calloc(xst->heap,
                                 memPtr,
                                 sizeof(*input),
                                 false);

          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

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
            cverts->input = input;

          last_input = input;
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(xst->reader);
          tree = NULL;

          ak_tree_fromXmlNode(xst->heap,
                              memPtr,
                              nodePtr,
                              &tree,
                              NULL);
          cverts->extra = tree;

          ak_xml_skipelm(xst);;

        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
      
      nurbs->cverts = cverts;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          memPtr,
                          nodePtr,
                          &tree,
                          NULL);
      nurbs->extra = tree;

      ak_xml_skipelm(xst);;
    }
    
    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = nurbs;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_nurbs_surface(AkXmlState * __restrict xst,
                     void * __restrict memParent,
                     bool asObject,
                     AkNurbsSurface ** __restrict dest) {
  AkObject       *obj;
  AkNurbsSurface *nurbsSurface;
  AkSource       *last_source;
  void           *memPtr;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*nurbsSurface),
                      0,
                      true,
                      false);

    nurbsSurface = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbsSurface = ak_heap_calloc(xst->heap,
                                  memParent,
                                  sizeof(*nurbsSurface),
                                  false);
    memPtr = nurbsSurface;
  }

  nurbsSurface->degree_u = ak_xml_attrui(xst, _s_dae_degree_u);
  nurbsSurface->degree_v = ak_xml_attrui(xst, _s_dae_degree_v);
  nurbsSurface->closed_u = ak_xml_attrui(xst, _s_dae_closed_u);
  nurbsSurface->closed_v = ak_xml_attrui(xst, _s_dae_closed_v);

  last_source = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_lines))
      break;

    if (ak_xml_eqelm(xst, _s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(xst, memPtr, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          nurbsSurface->source = source;

        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInputBasic   *last_input;

      cverts = ak_heap_calloc(xst->heap,
                              memPtr,
                              sizeof(*cverts),
                              false);

      last_input = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_control_vertices))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInputBasic *input;

          input = ak_heap_calloc(xst->heap,
                                 memPtr,
                                 sizeof(*input),
                                 false);

          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

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
            cverts->input = input;

          last_input = input;
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(xst->reader);
          tree = NULL;

          ak_tree_fromXmlNode(xst->heap,
                              memPtr,
                              nodePtr,
                              &tree,
                              NULL);
          cverts->extra = tree;

          ak_xml_skipelm(xst);;

        } else {
          ak_xml_skipelm(xst);;
        }

        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);

      nurbsSurface->cverts = cverts;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          memPtr,
                          nodePtr,
                          &tree,
                          NULL);
      nurbsSurface->extra = tree;
      
      ak_xml_skipelm(xst);;
    }
    
    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = nurbsSurface;
  
  return AK_OK;
}
