/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_constant.h"
#include "ak_collada_fx_color_or_tex.h"
#include "ak_collada_fx_float_or_param.h"

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
ak_dae_fxConstant(AkHeap * __restrict heap,
                  void * __restrict memParent,
                  xmlTextReaderPtr reader,
                  AkConstantFx ** __restrict dest) {
  AkConstantFx  *constant;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;


  constant = ak_heap_calloc(heap, memParent, sizeof(*constant), false);

  if (constantMapLen == 0) {
    constantMapLen = AK_ARRAY_LEN(constantMap);
    qsort(constantMap,
          constantMapLen,
          sizeof(constantMap[0]),
          ak_enumpair_cmp);
  }

  do {
    const ak_enumpair *found;

    _xml_beginElement(_s_dae_constant);

    found = bsearch(nodeName,
                    constantMap,
                    constantMapLen,
                    sizeof(constantMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_emission:
      case k_s_dae_reflective:
      case k_s_dae_transparent: {
        AkFxColorOrTex *colorOrTex;
        AkResult ret;

        ret = ak_dae_colorOrTex(heap,
                                constant,
                                reader,
                                (const char *)nodeName,
                                &colorOrTex);
        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_emission:
              constant->emission = colorOrTex;
              break;
            case k_s_dae_reflective:
              constant->reflective = colorOrTex;
              break;
            case k_s_dae_transparent:
              constant->transparent = colorOrTex;
              break;
            default: break;
          }
        }

        break;
      }
      case k_s_dae_reflectivity:
      case k_s_dae_transparency:
      case k_s_dae_index_of_refraction: {
        AkFxFloatOrParam * floatOrParam;
        AkResult ret;

        ret = ak_dae_floatOrParam(heap,
                                  constant,
                                  reader,
                                  (const char *)nodeName,
                                  &floatOrParam);

        if (ret == AK_OK) {
          switch (found->val) {
            case k_s_dae_reflectivity:
              constant->reflectivity = floatOrParam;
              break;
            case k_s_dae_transparency:
              constant->transparency = floatOrParam;
              break;
            case k_s_dae_index_of_refraction:
              constant->indexOfRefraction = floatOrParam;
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
  
  *dest = constant;
  
  return AK_OK;
}
