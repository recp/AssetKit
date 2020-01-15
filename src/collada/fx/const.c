/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_constant.h"
#include "dae_fx_color_or_tex.h"
#include "dae_fx_float_or_param.h"
#include "../../default/def_material.h"
#include "dae_fx_enums.h"
#include "../bugfix/transp.h"

#define k_s_dae_emission            1
#define k_s_dae_reflective          2
#define k_s_dae_transparent         3
#define k_s_dae_reflectivity        4
#define k_s_dae_transparency        5
#define k_s_dae_index_of_refraction 6

static ak_enumpair constantMap[] = {
  {_s_dae_emission,            k_s_dae_emission},
  {_s_dae_reflective,          k_s_dae_reflective},
  {_s_dae_transparent,         k_s_dae_transparent},
  {_s_dae_reflectivity,        k_s_dae_reflectivity},
  {_s_dae_transparency,        k_s_dae_transparency},
  {_s_dae_index_of_refraction, k_s_dae_index_of_refraction}
};

static size_t constantMapLen = 0;

AkResult _assetkit_hide
dae_fxConstant(AkXmlState           * __restrict xst,
               void                 * __restrict memParent,
               AkTechniqueFxCommon ** __restrict dest) {
  AkTechniqueFxCommon *techn;
  AkXmlElmState        xest;

  techn = ak_heap_calloc(xst->heap, memParent, sizeof(*techn));

  if (constantMapLen == 0) {
    constantMapLen = AK_ARRAY_LEN(constantMap);
    qsort(constantMap,
          constantMapLen,
          sizeof(constantMap[0]),
          ak_enumpair_cmp);
  }

  ak_xest_init(xest, _s_dae_constant)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    constantMap,
                    constantMapLen,
                    sizeof(constantMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case k_s_dae_emission:
      case k_s_dae_reflective:
      case k_s_dae_transparent: {
        AkColorDesc *colorDesc;
        AkResult     ret;
        AkOpaque     opaque;

        opaque = ak_xml_attrenum_def(xst,
                                     _s_dae_opaque,
                                     dae_fxEnumOpaque,
                                     AK_OPAQUE_A_ONE);

        ret = dae_colorOrTex(xst,
                             techn,
                             (const char *)xst->nodeName,
                             &colorDesc);
        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_emission:
              techn->emission = colorDesc;
              break;
            case k_s_dae_reflective: {
              if (!techn->reflective) {
                AkReflective *refl;
                refl = ak_heap_calloc(xst->heap, techn, sizeof(*refl));
                techn->reflective = refl;
              }

              techn->reflective->color = colorDesc;
              break;
            }
            case k_s_dae_transparent: {
              if (!techn->transparent) {
                AkTransparent    *transp;
                transp = ak_heap_calloc(xst->heap, techn, sizeof(*transp));
                transp->amount = ak_def_transparency();
                techn->transparent = transp;
              }

              techn->transparent->color  = colorDesc;
              techn->transparent->opaque = opaque;
              break;
            }
            default:
              ak_free(colorDesc);
              break;
          }
        }

        break;
      }
      case k_s_dae_reflectivity:
      case k_s_dae_transparency:
      case k_s_dae_index_of_refraction: {
        AkFloatOrParam *floatOrParam;
        AkResult        ret;

        ret = dae_floatOrParam(xst,
                               techn,
                               (const char *)xst->nodeName,
                               &floatOrParam);

        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_reflectivity:{
              if (!techn->reflective) {
                AkReflective *refl;
                refl = ak_heap_calloc(xst->heap, techn, sizeof(*refl));
                techn->reflective = refl;
              }

              techn->reflective->amount = floatOrParam;
              break;
            }
            case k_s_dae_transparency: {
              if (!techn->transparent) {
                AkTransparent *transp;
                transp = ak_heap_calloc(xst->heap, techn, sizeof(*transp));
                transp->opaque     = AK_OPAQUE_A_ONE;
                techn->transparent = transp;
              }

              techn->transparent->amount = floatOrParam;

              /* some old version of tools e.g. SketchUp exports incorrect */
              if (ak_opt_get(AK_OPT_BUGFIXES))
                dae_bugfix_transp(techn->transparent);

              break;
            }
            case k_s_dae_index_of_refraction:
              techn->indexOfRefraction = floatOrParam;
              break;
            default:
              ak_free(floatOrParam);
              break;
          }
        }

        break;
      }
      default:
        ak_xml_skipelm(xst);
        break;
    }

  skip:
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = techn;
  
  return AK_OK;
}
