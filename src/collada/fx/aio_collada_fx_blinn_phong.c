/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_blinn_phong.h"
#include "../aio_collada_common.h"
#include "aio_collada_fx_color_or_tex.h"
#include "aio_collada_fx_float_or_param.h"

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

static aio_enumpair blinnPhongMap[] = {
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

int _assetio_hide
aio_dae_blinn_phong(xmlTextReaderPtr __restrict reader,
                    const char * elm,
                    aio_blinn_phong ** __restrict dest) {
  aio_blinn_phong *blinn_phong;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;


  blinn_phong = aio_calloc(sizeof(*blinn_phong), 1);

  if (blinnPhongMapLen == 0) {
    blinnPhongMapLen = AIO_ARRAY_LEN(blinnPhongMap);
    qsort(blinnPhongMap,
          blinnPhongMapLen,
          sizeof(blinnPhongMap[0]),
          aio_enumpair_cmp);
  }

  do {
    const aio_enumpair *found;

    _xml_beginElement(elm);

    found = bsearch(nodeName,
                    blinnPhongMap,
                    blinnPhongMapLen,
                    sizeof(blinnPhongMap[0]),
                    aio_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_emission:
      case k_s_dae_ambient:
      case k_s_dae_diffuse:
      case k_s_dae_specular:
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
            case k_s_dae_reflective:
              blinn_phong->phong.reflective = colorOrTex;
              break;
            case k_s_dae_transparent:
              blinn_phong->phong.transparent = colorOrTex;
              break;
            default: break;
          }
        }

        break;
      }
      case k_s_dae_shininess:
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
            case k_s_dae_shininess:
              blinn_phong->phong.shininess = floatOrParam;
              break;
            case k_s_dae_reflectivity:
              blinn_phong->phong.reflectivity = floatOrParam;
              break;
            case k_s_dae_transparency:
              blinn_phong->phong.transparency = floatOrParam;
              break;
            case k_s_dae_index_of_refraction:
              blinn_phong->phong.index_of_refraction = floatOrParam;
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
  
  *dest = blinn_phong;
  
  return 0;
}
