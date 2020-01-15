/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_spline.h"
#include "dae_source.h"
#include "dae_enums.h"

AkResult _assetkit_hide
dae_spline(AkXmlState * __restrict xst,
           void * __restrict memParent,
           bool asObject,
           AkSpline ** __restrict dest) {
  AkObject    *obj;
  AkSpline    *spline;
  AkSource    *last_source;
  void         *memPtr;
  AkXmlElmState xest;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*spline),
                      AK_GEOMETRY_TYPE_SPLINE,
                      true);

    spline = ak_objGet(obj);
    memPtr = obj;
  } else {
    spline = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*spline));
    memPtr = spline;
  }

  spline->closed = ak_xml_attrui(xst, _s_dae_closed);

  last_source = NULL;

  ak_xest_init(xest, _s_dae_spline)

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
          spline->source = source;

        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_control_vertices)) {
      AkControlVerts *cverts;
      AkInput        *last_input;
      AkXmlElmState   xest2;

      cverts     = ak_heap_calloc(xst->heap, memPtr, sizeof(*cverts));
      last_input = NULL;

      ak_xest_init(xest2, _s_dae_control_vertices)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;

          input = ak_heap_calloc(xst->heap, memPtr, sizeof(*input));
          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          if (!input->semanticRaw)
            ak_free(input);
          else {
            AkURL *url;
            AkEnum inputSemantic;
            inputSemantic = dae_enumInputSemantic(input->semanticRaw);

            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_SEMANTIC_OTHER;

            input->semantic = inputSemantic;

            url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
            rb_insert(xst->inputmap, input, url);

            if (last_input)
              last_input->next = input;
            else
              cverts->input = input;

            last_input = input;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          dae_extra(xst, cverts, &cverts->extra);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      spline->cverts = cverts;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, spline, &spline->extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = spline;

  return AK_OK;
}
