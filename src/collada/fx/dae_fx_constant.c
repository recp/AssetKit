/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_constant.h"
#include "dae_fx_color_or_tex.h"
#include "dae_fx_float_or_param.h"
#include "../../default/ak_def_material.h"
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
ak_dae_fxConstant(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkConstantFx ** __restrict dest) {
  AkConstantFx *constant;
  AkXmlElmState xest;

  constant = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*constant));

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
        AkColorDesc *colorOrTex;
        AkResult     ret;
        AkOpaque     opaque;

        opaque = ak_xml_attrenum(xst,
                                 _s_dae_opaque,
                                 ak_dae_fxEnumOpaque);

        ret = ak_dae_colorOrTex(xst,
                                constant,
                                (const char *)xst->nodeName,
                                &colorOrTex);
        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_emission:
              constant->emission = colorOrTex;
              break;
            case k_s_dae_reflective: {
              if (!constant->base.reflective) {
                AkReflective *refl;
                refl = ak_heap_calloc(xst->heap,
                                      constant,
                                      sizeof(*refl));
                constant->base.reflective = refl;
              }

              constant->base.reflective->color = colorOrTex;
              break;
            }
            case k_s_dae_transparent: {
              if (!constant->base.transparent) {
                AkTransparent    *transp;
                transp = ak_heap_calloc(xst->heap,
                                        constant,
                                        sizeof(*transp));
                transp->amount = ak_def_transparency();
                constant->base.transparent = transp;
              }

              constant->base.transparent->color  = colorOrTex;
              constant->base.transparent->opaque = opaque;
              break;
            }
            default:
              ak_free(colorOrTex);
              break;
          }
        }

        break;
      }
      case k_s_dae_reflectivity:
      case k_s_dae_transparency:
      case k_s_dae_index_of_refraction: {
        AkFxFloatOrParam *floatOrParam;
        AkResult          ret;

        ret = ak_dae_floatOrParam(xst,
                                  constant,
                                  (const char *)xst->nodeName,
                                  &floatOrParam);

        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_reflectivity:{
              if (!constant->base.reflective) {
                AkReflective *refl;
                refl = ak_heap_calloc(xst->heap,
                                      constant,
                                      sizeof(*refl));

                constant->base.reflective = refl;
              }

              constant->base.reflective->amount = floatOrParam;
              break;
            }
            case k_s_dae_transparency: {
              if (!constant->base.transparent) {
                AkTransparent *transp;
                transp = ak_heap_calloc(xst->heap,
                                        constant,
                                        sizeof(*transp));
                constant->base.transparent = transp;
              }

              constant->base.transparent->amount = floatOrParam;

              /* some old version of tools e.g. SketchUp exports incorrect */
              if (ak_opt_get(AK_OPT_BUGFIXES))
                dae_bugfix_transp(constant->base.transparent);

              break;
            }
            case k_s_dae_index_of_refraction:
              constant->base.indexOfRefraction = floatOrParam;
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
  
  *dest = constant;
  
  return AK_OK;
}
