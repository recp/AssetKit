/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_blinn_phong.h"
#include "dae_fx_color_or_tex.h"
#include "dae_fx_float_or_param.h"
#include "../../default/ak_def_material.h"
#include "dae_fx_enums.h"
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
ak_dae_blinn_phong(AkXmlState * __restrict xst,
                   void * __restrict memParent,
                   const char * elm,
                   ak_blinn_phong ** __restrict dest) {
  ak_blinn_phong *blinn_phong;
  AkXmlElmState   xest;

  blinn_phong = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(*blinn_phong));

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
        AkColorDesc *colorOrTex;
        AkResult     ret;
        AkOpaque     opaque;

        opaque = ak_xml_attrenum(xst,
                                 _s_dae_opaque,
                                 ak_dae_fxEnumOpaque);

        ret = ak_dae_colorOrTex(xst,
                                blinn_phong,
                                (const char *)xst->nodeName,
                                &colorOrTex);
        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_emission:
              blinn_phong->phong.emission = colorOrTex;
              break;
            case k_s_dae_ambient:
              blinn_phong->phong.ambient = colorOrTex;
              break;
            case k_s_dae_diffuse:
              blinn_phong->phong.diffuse = colorOrTex;
              break;
            case k_s_dae_specular:
              blinn_phong->phong.specular = colorOrTex;
              break;
            case k_s_dae_reflective: {
              if (!blinn_phong->phong.base.reflective) {
                AkReflective *refl;
                refl = ak_heap_calloc(xst->heap,
                                      blinn_phong,
                                      sizeof(*refl));
                blinn_phong->phong.base.reflective = refl;
              }

              blinn_phong->phong.base.reflective->color = colorOrTex;
              break;
            }
            case k_s_dae_transparent: {
              if (!blinn_phong->phong.base.transparent) {
                AkTransparent    *transp;
                transp = ak_heap_calloc(xst->heap,
                                        blinn_phong,
                                        sizeof(*transp));
                transp->amount = ak_def_transparency();
                blinn_phong->phong.base.transparent = transp;
              }

              blinn_phong->phong.base.transparent->color  = colorOrTex;
              blinn_phong->phong.base.transparent->opaque = opaque;
              break;
            }
            default:
              ak_free(colorOrTex);
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

        ret = ak_dae_floatOrParam(xst,
                                  blinn_phong,
                                  (const char *)xst->nodeName,
                                  &floatOrParam);

        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_shininess:
              blinn_phong->phong.shininess = floatOrParam;
              break;
            case k_s_dae_reflectivity: {
              if (!blinn_phong->phong.base.reflective) {
                AkReflective *refl;
                refl = ak_heap_calloc(xst->heap,
                                      blinn_phong,
                                      sizeof(*refl));
                blinn_phong->phong.base.reflective = refl;
              }

              blinn_phong->phong.base.reflective->amount = floatOrParam;
              break;
            }
            case k_s_dae_transparency: {
              if (!blinn_phong->phong.base.transparent) {
                AkTransparent *transp;
                transp = ak_heap_calloc(xst->heap,
                                        blinn_phong,
                                        sizeof(*transp));
                blinn_phong->phong.base.transparent = transp;
              }

              blinn_phong->phong.base.transparent->amount = floatOrParam;

              /* some old version of tools e.g. SketchUp exports incorrect */
              if (ak_opt_get(AK_OPT_BUGFIXES))
                dae_bugfix_transp(blinn_phong->phong.base.transparent);

              break;
            }
            case k_s_dae_index_of_refraction:
              blinn_phong->phong.base.indexOfRefraction = floatOrParam;
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
  
  *dest = blinn_phong;
  
  return AK_OK;
}
