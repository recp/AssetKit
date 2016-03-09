/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_lambert.h"
#include "../aio_collada_common.h"
#include "aio_collada_fx_color_or_tex.h"
#include "aio_collada_fx_float_or_param.h"

#define k_s_dae_emission            1
#define k_s_dae_ambient             2
#define k_s_dae_diffuse             3
#define k_s_dae_reflective          5
#define k_s_dae_transparent         6
#define k_s_dae_reflectivity        8
#define k_s_dae_transparency        9
#define k_s_dae_index_of_refraction 10

static aio_enumpair lambertMap[] = {
  {_s_dae_emission,            k_s_dae_emission},
  {_s_dae_ambient,             k_s_dae_ambient},
  {_s_dae_diffuse,             k_s_dae_diffuse},
  {_s_dae_reflective,          k_s_dae_reflective},
  {_s_dae_transparent,         k_s_dae_transparent},
  {_s_dae_reflectivity,        k_s_dae_reflectivity},
  {_s_dae_transparency,        k_s_dae_transparency},
  {_s_dae_index_of_refraction, k_s_dae_index_of_refraction}
};

static size_t lambertMapLen = 0;

int _assetio_hide
aio_dae_fxLambert(xmlTextReaderPtr __restrict reader,
                  aio_lambert ** __restrict dest) {
  aio_lambert   *lambert;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;


  lambert = aio_calloc(sizeof(*lambert), 1);

  if (lambertMapLen == 0) {
    lambertMapLen = AIO_ARRAY_LEN(lambertMap);
    qsort(lambertMap,
          lambertMapLen,
          sizeof(lambertMap[0]),
          aio_enumpair_cmp);
  }

  do {
    const aio_enumpair *found;

    _xml_beginElement(_s_dae_lambert);

    found = bsearch(nodeName,
                    lambertMap,
                    lambertMapLen,
                    sizeof(lambertMap[0]),
                    aio_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_emission:
      case k_s_dae_ambient:
      case k_s_dae_diffuse:
      case k_s_dae_reflective:
      case k_s_dae_transparent: {
        aio_fx_color_or_tex *colorOrTex;
        int ret;

        ret = aio_dae_colorOrTex(reader,
                                 (const char *)nodeName,
                                 &colorOrTex);
        if (ret == 0) {
          switch (found->val) {
            case k_s_dae_emission:
              lambert->emission = colorOrTex;
              break;
            case k_s_dae_ambient:
              lambert->ambient = colorOrTex;
              break;
            case k_s_dae_diffuse:
              lambert->diffuse = colorOrTex;
              break;
            case k_s_dae_reflective:
              lambert->reflective = colorOrTex;
              break;
            case k_s_dae_transparent:
              lambert->transparent = colorOrTex;
              break;
            default: break;
          }
        }

        break;
      }
      case k_s_dae_reflectivity:
      case k_s_dae_transparency:
      case k_s_dae_index_of_refraction: {
        aio_fx_float_or_param * floatOrParam;
        int ret;

        ret = aio_dae_floatOrParam(reader,
                                   (const char *)nodeName,
                                   &floatOrParam);

        if (ret == 0) {
          switch (found->val) {
            case k_s_dae_reflectivity:
              lambert->reflectivity = floatOrParam;
              break;
            case k_s_dae_transparency:
              lambert->transparency = floatOrParam;
              break;
            case k_s_dae_index_of_refraction:
              lambert->index_of_refraction = floatOrParam;
              break;
            default: break;
          }
        }

        break;
      }
      default:
        _xml_skipElement;
        break;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = lambert;
  
  return 0;
}
