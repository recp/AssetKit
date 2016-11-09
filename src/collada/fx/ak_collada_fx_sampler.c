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
ak_dae_fxSampler(AkXmlState * __restrict xst,
                 void * __restrict memParent,
                 const char *elm,
                 AkFxSamplerCommon ** __restrict dest) {
  AkFxSamplerCommon *sampler;

  sampler = ak_heap_calloc(xst->heap,
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

    if (ak_xml_beginelm(xst, elm))
      break;

    found = bsearch(xst->nodeName,
                    fxSamplerCMap,
                    fxSamplerCMapLen,
                    sizeof(fxSamplerCMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_instance_image: {
        AkInstanceImage * instanceImage;
        AkResult ret;

        ret = ak_dae_fxInstanceImage(xst, sampler, &instanceImage);

        if (ret == AK_OK)
          sampler->instanceImage = instanceImage;
        break;
      }
      case k_s_dae_texcoord:
        sampler->texcoordSemantic = ak_xml_attr(xst,
                                                sampler,
                                                _s_dae_semantic);
        break;
      case k_s_dae_wrap_s:
        sampler->wrapS = ak_xml_readenum(xst, ak_dae_fxEnumWrap);
        break;
      case k_s_dae_wrap_t:
        sampler->wrapT = ak_xml_readenum(xst, ak_dae_fxEnumWrap);
        break;
      case k_s_dae_wrap_p:
        sampler->wrapP = ak_xml_readenum(xst, ak_dae_fxEnumWrap);
        break;
      case k_s_dae_minfilter:
        sampler->minfilter = ak_xml_readenum(xst, ak_dae_fxEnumMinfilter);
        break;
      case k_s_dae_magfilter:
        sampler->magfilter = ak_xml_readenum(xst, ak_dae_fxEnumMagfilter);
        break;
      case k_s_dae_mipfilter:
        sampler->mipfilter = ak_xml_readenum(xst, ak_dae_fxEnumMipfilter);
        break;
      case k_s_dae_border_color: {
        AkColor *color;
        AkResult  ret;

        color = ak_heap_calloc(xst->heap,
                               sampler,
                               sizeof(*color),
                               false);
        ret   = ak_dae_color(xst, true, color);

        if (ret == AK_OK)
          sampler->borderColor = color;
        else
          ak_free(color);
        break;
      }
      case k_s_dae_mip_max_level:
        sampler->mipMaxLevel = ak_xml_vall(xst);
        break;
      case k_s_dae_mip_min_level:
        sampler->mipMinLevel = ak_xml_vall(xst);
        break;
      case k_s_dae_mip_bias:
        sampler->mipBias = ak_xml_valf(xst);
        break;
      case k_s_dae_max_anisotropy:
        sampler->maxAnisotropy = ak_xml_valul_def(xst, 1l);
        break;
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree   *tree;

        nodePtr = xmlTextReaderExpand(xst->reader);
        tree = NULL;

        ak_tree_fromXmlNode(xst->heap,
                            sampler,
                            nodePtr,
                            &tree,
                            NULL);
        sampler->extra = tree;

        ak_xml_skipelm(xst);;
        break;
      }
      default:
         ak_xml_skipelm(xst);;
        break;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = sampler;
  
  return AK_OK;
}
