/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_sampler.h"
#include "../core/ak_collada_color.h"
#include "ak_collada_fx_enums.h"
#include "ak_collada_fx_image.h"

#define k_s_dae_instance_image 1
#define k_s_dae_texcoord       2
#define k_s_dae_wrap_s         3
#define k_s_dae_wrap_t         4
#define k_s_dae_wrap_p         5
#define k_s_dae_minfilter      6
#define k_s_dae_magfilter      7
#define k_s_dae_mipfilter      8
#define k_s_dae_border_color   9
#define k_s_dae_mip_max_level  10
#define k_s_dae_mip_min_level  11
#define k_s_dae_mip_bias       12
#define k_s_dae_max_anisotropy 13
#define k_s_dae_extra          14

static ak_enumpair fxSamplerCMap[] = {
  {_s_dae_instance_image, k_s_dae_instance_image},
  {_s_dae_texcoord,       k_s_dae_texcoord},
  {_s_dae_wrap_s,         k_s_dae_wrap_s},
  {_s_dae_wrap_t,         k_s_dae_wrap_t},
  {_s_dae_wrap_p,         k_s_dae_wrap_p},
  {_s_dae_minfilter,      k_s_dae_minfilter},
  {_s_dae_magfilter,      k_s_dae_magfilter},
  {_s_dae_mipfilter,      k_s_dae_mipfilter},
  {_s_dae_border_color,   k_s_dae_border_color},
  {_s_dae_mip_max_level,  k_s_dae_mip_max_level},
  {_s_dae_mip_min_level,  k_s_dae_mip_min_level},
  {_s_dae_mip_bias,       k_s_dae_mip_bias},
  {_s_dae_max_anisotropy, k_s_dae_max_anisotropy},
  {_s_dae_extra,          k_s_dae_extra}
};

static size_t fxSamplerCMapLen = 0;

AkResult _assetkit_hide
ak_dae_fxSampler(AkDaeState * __restrict daestate,
                 void * __restrict memParent,
                 const char *elm,
                 AkFxSamplerCommon ** __restrict dest) {
  AkFxSamplerCommon *sampler;

  sampler = ak_heap_calloc(daestate->heap,
                           memParent,
                           sizeof(*sampler),
                           false);

  if (fxSamplerCMapLen == 0) {
    fxSamplerCMapLen = AK_ARRAY_LEN(fxSamplerCMap);
    qsort(fxSamplerCMap,
          fxSamplerCMapLen,
          sizeof(fxSamplerCMap[0]),
          ak_enumpair_cmp);
  }

  do {
    const ak_enumpair *found;

    _xml_beginElement(elm);

    found = bsearch(daestate->nodeName,
                    fxSamplerCMap,
                    fxSamplerCMapLen,
                    sizeof(fxSamplerCMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_instance_image: {
        AkInstanceImage * instanceImage;
        AkResult ret;

        ret = ak_dae_fxInstanceImage(daestate, sampler, &instanceImage);

        if (ret == AK_OK)
          sampler->instanceImage = instanceImage;
        break;
      }
      case k_s_dae_texcoord:
        _xml_readAttr(sampler,
                      sampler->texcoordSemantic,
                      _s_dae_semantic);
        break;
      case k_s_dae_wrap_s:
        _xml_readTextAsEnum(sampler->wrapS,
                            _s_dae_wrap_s,
                            ak_dae_fxEnumWrap);
        break;
      case k_s_dae_wrap_t:
        _xml_readTextAsEnum(sampler->wrapT,
                            _s_dae_wrap_t,
                            ak_dae_fxEnumWrap);
        break;
      case k_s_dae_wrap_p:
        _xml_readTextAsEnum(sampler->wrapP,
                            _s_dae_wrap_p,
                            ak_dae_fxEnumWrap);
        break;
      case k_s_dae_minfilter:
        _xml_readTextAsEnum(sampler->minfilter,
                            _s_dae_minfilter,
                            ak_dae_fxEnumMinfilter);
        break;
      case k_s_dae_magfilter:
        _xml_readTextAsEnum(sampler->magfilter,
                            _s_dae_magfilter,
                            ak_dae_fxEnumMagfilter);
        break;
      case k_s_dae_mipfilter:
        _xml_readTextAsEnum(sampler->mipfilter,
                            _s_dae_mipfilter,
                            ak_dae_fxEnumMipfilter);
        break;
      case k_s_dae_border_color: {
        AkColor *color;
        AkResult  ret;

        color = ak_heap_calloc(daestate->heap,
                               sampler,
                               sizeof(*color),
                               false);
        ret   = ak_dae_color(daestate, true, color);

        if (ret == AK_OK)
          sampler->borderColor = color;
        else
          ak_free(color);
        break;
      }
      case k_s_dae_mip_max_level:
        _xml_readTextUsingFn(sampler->mipMaxLevel,
                             strtol, NULL, 10);
        break;
      case k_s_dae_mip_min_level:
        _xml_readTextUsingFn(sampler->mipMinLevel,
                             strtol, NULL, 10);
        break;
      case k_s_dae_mip_bias:
        _xml_readTextUsingFn(sampler->mipBias,
                             strtof, NULL);
        break;
      case k_s_dae_max_anisotropy: {
        char *tmp;

        tmp = NULL;
        _xml_readTextUsingFn(sampler->maxAnisotropy,
                             strtol, &tmp, 10);

        if (tmp && *tmp == '\0')
          sampler->mipMaxLevel = 1;
        break;
      }
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree   *tree;

        nodePtr = xmlTextReaderExpand(daestate->reader);
        tree = NULL;

        ak_tree_fromXmlNode(daestate->heap,
                            sampler,
                            nodePtr,
                            &tree,
                            NULL);
        sampler->extra = tree;

        _xml_skipElement;
        break;
      }
      default:
         _xml_skipElement;
        break;
    }

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);

  *dest = sampler;
  
  return AK_OK;
}
