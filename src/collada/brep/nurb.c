/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "nurbs.h"
#include "../core/source.h"
#include "../core/enums.h"

AkResult _assetkit_hide
dae_nurbs(AkXmlState * __restrict xst,
          void * __restrict memParent,
          bool asObject,
          AkNurbs ** __restrict dest) {
  AkObject     *obj;
  AkNurbs      *nurbs;
  AkSource     *last_source;
  void         *memPtr;
  AkXmlElmState xest;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*nurbs),
                      0,
                      true);

    nurbs = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbs = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*nurbs));
    memPtr = nurbs;
  }

  nurbs->degree = ak_xml_attrui(xst, _s_dae_degree);
  nurbs->closed = ak_xml_attrui_def(xst, _s_dae_closed, false);

  last_source = NULL;

  ak_xest_init(xest, _s_dae_nurbs)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = dae_source(xst, memPtr, NULL, 0, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          nurbs->source = source;

        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInput        *last_input;
      AkXmlElmState   xest2;

      cverts = ak_heap_calloc(xst->heap,
                              memPtr,
                              sizeof(*cverts));

      last_input = NULL;

      ak_xest_init(xest2, _s_dae_control_vertices)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;
          AkURL   *url;

          input = ak_heap_calloc(xst->heap,
                                 memPtr,
                                 sizeof(*input));

          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
          rb_insert(xst->inputmap, input, url);

          if (!input->semanticRaw)
            ak_free(input);
          else {
            AkEnum inputSemantic;
            inputSemantic = dae_enumInputSemantic(input->semanticRaw);

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
          dae_extra(xst, memPtr, &cverts->extra);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      nurbs->cverts = cverts;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, memPtr, &nurbs->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = nurbs;

  return AK_OK;
}

AkResult _assetkit_hide
dae_nurbs_surface(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  bool asObject,
                  AkNurbsSurface ** __restrict dest) {
  AkObject       *obj;
  AkNurbsSurface *nurbsSurface;
  AkSource       *last_source;
  void           *memPtr;
  AkXmlElmState   xest;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*nurbsSurface),
                      0,
                      true);

    nurbsSurface = ak_objGet(obj);
    memPtr = obj;
  } else {
    nurbsSurface = ak_heap_calloc(xst->heap,
                                  memParent,
                                  sizeof(*nurbsSurface));
    memPtr = nurbsSurface;
  }

  nurbsSurface->degree_u = ak_xml_attrui(xst, _s_dae_degree_u);
  nurbsSurface->degree_v = ak_xml_attrui(xst, _s_dae_degree_v);
  nurbsSurface->closed_u = ak_xml_attrui(xst, _s_dae_closed_u);
  nurbsSurface->closed_v = ak_xml_attrui(xst, _s_dae_closed_v);

  last_source = NULL;

  ak_xest_init(xest, _s_dae_nurbs_surface)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = dae_source(xst, memPtr, NULL, 0, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          nurbsSurface->source = source;

        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInput        *last_input;
      AkXmlElmState   xest2;

      cverts = ak_heap_calloc(xst->heap,
                              memPtr,
                              sizeof(*cverts));

      last_input = NULL;

      ak_xest_init(xest2, _s_dae_control_vertices)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;
          AkURL   *url;

          input = ak_heap_calloc(xst->heap,
                                 memPtr,
                                 sizeof(*input));

          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
          rb_insert(xst->inputmap, input, url);

          if (!input->semanticRaw)
            ak_free(input);
          else {
            AkEnum inputSemantic;
            inputSemantic = dae_enumInputSemantic(input->semanticRaw);

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
          dae_extra(xst, memPtr, &cverts->extra);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      nurbsSurface->cverts = cverts;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, memPtr, &nurbsSurface->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = nurbsSurface;

  return AK_OK;
}
