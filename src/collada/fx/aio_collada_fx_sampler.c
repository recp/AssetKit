/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_sampler.h"
#include "../aio_collada_common.h"
#include "../aio_collada_color.h"
#include "aio_collada_fx_enums.h"
#include "aio_collada_fx_image.h"

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

static aio_enumpair fxSamplerCMap[] = {
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

int _assetio_hide
aio_dae_fxSampler(xmlTextReaderPtr __restrict reader,
                  const char *elm,
                  aio_fx_sampler_common ** __restrict dest) {
  aio_fx_sampler_common *sampler;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  sampler = aio_calloc(sizeof(*sampler), 1);

  if (fxSamplerCMapLen == 0) {
    fxSamplerCMapLen = AIO_ARRAY_LEN(fxSamplerCMap);
    qsort(fxSamplerCMap,
          fxSamplerCMapLen,
          sizeof(fxSamplerCMap[0]),
          aio_enumpair_cmp);
  }

  do {
    const aio_enumpair *found;

    _xml_beginElement(elm);

    found = bsearch(nodeName,
                    fxSamplerCMap,
                    fxSamplerCMapLen,
                    sizeof(fxSamplerCMap[0]),
                    aio_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_instance_image: {
        aio_image_instance * imageInst;
        int ret;

        ret = aio_dae_fxImageInstance(reader, &imageInst);

        if (ret == 0)
          sampler->image_inst = imageInst;
        break;
      }
      case k_s_dae_texcoord:
        _xml_readAttr(sampler->texcoord.semantic,
                      _s_dae_semantic);
        break;
      case k_s_dae_wrap_s:
        _xml_readTextAsEnum(sampler->wrap_s,
                            _s_dae_wrap_s,
                            aio_dae_fxEnumWrap);
        break;
      case k_s_dae_wrap_t:
        _xml_readTextAsEnum(sampler->wrap_t,
                            _s_dae_wrap_t,
                            aio_dae_fxEnumWrap);
        break;
      case k_s_dae_wrap_p:
        _xml_readTextAsEnum(sampler->wrap_p,
                            _s_dae_wrap_p,
                            aio_dae_fxEnumWrap);
        break;
      case k_s_dae_minfilter:
        _xml_readTextAsEnum(sampler->minfilter,
                            _s_dae_minfilter,
                            aio_dae_fxEnumMinfilter);
        break;
      case k_s_dae_magfilter:
        _xml_readTextAsEnum(sampler->magfilter,
                            _s_dae_magfilter,
                            aio_dae_fxEnumMagfilter);
        break;
      case k_s_dae_mipfilter:
        _xml_readTextAsEnum(sampler->mipfilter,
                            _s_dae_mipfilter,
                            aio_dae_fxEnumMipfilter);
        break;
      case k_s_dae_border_color: {
        aio_color *color;
        int        ret;

        color = aio_calloc(sizeof(*color), 1);
        ret   = aio_dae_color(reader, true, color);

        if (ret == 0)
          sampler->border_color = color;
        else
          aio_free(color);
        break;
      }
      case k_s_dae_mip_max_level:
        _xml_readTextUsingFn(sampler->mip_max_level,
                             strtol, NULL, 10);
        break;
      case k_s_dae_mip_min_level:
        _xml_readTextUsingFn(sampler->mip_min_level,
                             strtol, NULL, 10);
        break;
      case k_s_dae_mip_bias:
        _xml_readTextUsingFn(sampler->mip_bias,
                             strtol, NULL, 10);
        break;
      case k_s_dae_max_anisotropy: {
        char *tmp;

        tmp = NULL;
        _xml_readTextUsingFn(sampler->max_anisotropy,
                             strtol, &tmp, 10);

        if (*tmp == '\0')
          sampler->mip_max_level = 1;
        break;
      }
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        aio_tree  *tree;

        nodePtr = xmlTextReaderExpand(reader);
        tree = NULL;

        aio_tree_fromXmlNode(nodePtr, &tree, NULL);
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
  } while (nodeRet);

  *dest = sampler;
  
  return 0;
}
