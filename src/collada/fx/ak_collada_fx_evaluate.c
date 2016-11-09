/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_evaluate.h"
#include "../core/ak_collada_param.h"
#include "../core/ak_collada_color.h"
#include "ak_collada_fx_enums.h"
#include "ak_collada_fx_image.h"

#define k_s_dae_color_target   1
#define k_s_dae_depth_target   2
#define k_s_dae_stencil_target 3
#define k_s_dae_color_clear    4
#define k_s_dae_depth_clear    5
#define k_s_dae_stencil_clear  6
#define k_s_dae_draw           7

static ak_enumpair evaluateMap[] = {
  {_s_dae_color_target,   k_s_dae_color_target},
  {_s_dae_depth_target,   k_s_dae_depth_target},
  {_s_dae_stencil_target, k_s_dae_stencil_target},
  {_s_dae_color_clear,    k_s_dae_color_clear},
  {_s_dae_depth_clear,    k_s_dae_depth_clear},
  {_s_dae_stencil_clear,  k_s_dae_stencil_clear},
  {_s_dae_draw,           k_s_dae_draw}
};

static size_t evaluateMapLen = 0;

AkResult _assetkit_hide
ak_dae_fxEvaluate(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkEvaluate ** __restrict dest) {
  AkEvaluate *evaluate;

  evaluate = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*evaluate),
                            false);

  if (evaluateMapLen == 0) {
    evaluateMapLen = AK_ARRAY_LEN(evaluateMap);
    qsort(evaluateMap,
          evaluateMapLen,
          sizeof(evaluateMap[0]),
          ak_enumpair_cmp);
  }
  
  do {
    const ak_enumpair *found;

    if (ak_xml_beginelm(xst, _s_dae_evaluate))
      break;

    found = bsearch(xst->nodeName,
                    evaluateMap,
                    evaluateMapLen,
                    sizeof(evaluateMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_color_target:
      case k_s_dae_depth_target:
      case k_s_dae_stencil_target: {
        AkEvaluateTarget *evaluate_target;
        const xmlChar *targetNodeName;

        evaluate_target = ak_heap_calloc(xst->heap,
                                         evaluate,
                                         sizeof(*evaluate_target),
                                         false);

        evaluate_target->index = ak_xml_attrui(xst, _s_dae_index);
        evaluate_target->slice = ak_xml_attrui(xst, _s_dae_slice);
        evaluate_target->mip   = ak_xml_attrui(xst, _s_dae_mip);

        evaluate_target->face  = ak_xml_attrenum(xst,
                                                 _s_dae_face,
                                                 ak_dae_fxEnumFace);

        targetNodeName = xst->nodeName;

        do {
          if (ak_xml_beginelm(xst, (char *)targetNodeName))
            break;

          if (ak_xml_eqelm(xst, _s_dae_param)) {
            AkParam * param;
            AkResult   ret;

            ret = ak_dae_param(xst,
                               evaluate_target,
                               AK_PARAM_TYPE_BASIC,
                               &param);

            if (ret == AK_OK)
              evaluate_target->param = param;
          } else if (ak_xml_eqelm(xst, _s_dae_instance_image)) {
            AkInstanceImage *instanceImage;
            AkResult ret;

            ret = ak_dae_fxInstanceImage(xst,
                                         evaluate_target,
                                         &instanceImage);

            if (ret == AK_OK)
              evaluate_target->instanceImage = instanceImage;
          } else {
            ak_xml_skipelm(xst);;
          }

          /* end element */
          ak_xml_endelm(xst);;
        } while (xst->nodeRet);

        switch (found->val) {
          case k_s_dae_color_target:
            evaluate->colorTarget = evaluate_target;
            break;
          case k_s_dae_depth_target:
            evaluate->depthTarget = evaluate_target;
            break;
          case k_s_dae_stencil_target:
            evaluate->stencilTarget = evaluate_target;
            break;
          default: break;
        }
      }
      case k_s_dae_color_clear: {
        AkColorClear *colorClear;
        colorClear = ak_heap_calloc(xst->heap,
                                    evaluate,
                                    sizeof(*colorClear),
                                    false);

        colorClear->index = ak_xml_attrui(xst, _s_dae_index);

        ak_dae_color(xst, false, &colorClear->val);

        evaluate->colorClear = colorClear;
        break;
      }
      case k_s_dae_depth_clear:{
        AkDepthClear *depthClear;
        depthClear = ak_heap_calloc(xst->heap,
                                    evaluate,
                                    sizeof(*depthClear),
                                    false);

        depthClear->index = ak_xml_attrui(xst, _s_dae_index);
        depthClear->val   = ak_xml_valf(xst);

        evaluate->depthClear = depthClear;
        break;
      }
      case k_s_dae_stencil_clear:{
        AkStencilClear *stencilClear;
        stencilClear = ak_heap_calloc(xst->heap,
                                      evaluate,
                                      sizeof(*stencilClear),
                                      false);

        stencilClear->index = ak_xml_attrui(xst, _s_dae_index);
        stencilClear->val   = ak_xml_valul(xst);

        evaluate->stencilClear = stencilClear;
        break;
      }
      case k_s_dae_draw: {
        const char *strVal;
        
        if ((strVal = ak_xml_val(xst, evaluate))) {
          evaluate->draw.strVal = strVal;
          evaluate->draw.enumDraw = ak_dae_fxEnumDraw(strVal);
        }
      }
      default:
         ak_xml_skipelm(xst);;
        break;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = evaluate;
  
  return AK_OK;
}
