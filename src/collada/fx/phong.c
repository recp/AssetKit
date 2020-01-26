/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "blinn_phong.h"
#include "colortex.h"
#include "fltprm.h"
#include "../../default/def_material.h"
#include "enum.h"
#include "../bugfix/transp.h"

#define k_s_dae_emission            1
#define k_s_dae_ambient             2
#define k_s_dae_diffuse             3
#define k_s_dae_specular            4
#define k_s_dae_reflective          5
#define k_s_dae_transparent         6
#define k_s_dae_shininess           7
#define k_s_dae_reflectivity        8
#define k_s_dae_transparency        9
#define k_s_dae_index_of_refraction 10

static ak_enumpair blinnPhongMap[] = {
  {_s_dae_emission,            k_s_dae_emission},
  {_s_dae_ambient,             k_s_dae_ambient},
  {_s_dae_diffuse,             k_s_dae_diffuse},
  {_s_dae_specular,            k_s_dae_specular},
  {_s_dae_reflective,          k_s_dae_reflective},
  {_s_dae_transparent,         k_s_dae_transparent},
  {_s_dae_shininess,           k_s_dae_shininess},
  {_s_dae_reflectivity,        k_s_dae_reflectivity},
  {_s_dae_transparency,        k_s_dae_transparency},
  {_s_dae_index_of_refraction, k_s_dae_index_of_refraction}
};

static size_t blinnPhongMapLen = 0;

AkResult _assetkit_hide
dae_phong(AkXmlState           * __restrict xst,
          void                 * __restrict memParent,
          const char           * elm,
          AkTechniqueFxCommon ** __restrict dest) {
  AkTechniqueFxCommon *techn;
  AkXmlElmState        xest;

  techn = ak_heap_calloc(xst->heap, memParent, sizeof(*techn));

  if (blinnPhongMapLen == 0) {
    blinnPhongMapLen = AK_ARRAY_LEN(blinnPhongMap);
    qsort(blinnPhongMap,
          blinnPhongMapLen,
          sizeof(blinnPhongMap[0]),
          ak_enumpair_cmp);
  }

  ak_xest_init(xest, elm)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    blinnPhongMap,
                    blinnPhongMapLen,
                    sizeof(blinnPhongMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case k_s_dae_emission:
      case k_s_dae_ambient:
      case k_s_dae_diffuse:
      case k_s_dae_specular:
      case k_s_dae_reflective:
      case k_s_dae_transparent: {
        AkColorDesc *colorDesc;
        AkResult     ret;
        AkOpaque     opaque;

        opaque = ak_xml_attrenum_def(xst,
                                     _s_dae_opaque,
                                     dae_fxEnumOpaque,
                                     AK_OPAQUE_A_ONE);
        ret    = dae_colorOrTex(xst,
                                techn,
                                (const char *)xst->nodeName,
                                &colorDesc);
        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_emission:
              techn->emission = colorDesc;
              break;
            case k_s_dae_ambient:
              techn->ambient = colorDesc;
              break;
            case k_s_dae_diffuse:
              techn->diffuse = colorDesc;
              break;
            case k_s_dae_specular:
              techn->specular = colorDesc;
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
      case k_s_dae_shininess:
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
            case k_s_dae_shininess:
              techn->shininess = floatOrParam;
              break;
            case k_s_dae_reflectivity: {
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
