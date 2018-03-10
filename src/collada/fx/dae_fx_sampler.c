/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_sampler.h"
#include "../core/dae_color.h"
#include "dae_fx_enums.h"
#include "dae_fx_image.h"

#include "../1.4/dae14.h"
#include "../1.4/dae14_surface.h"

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
#define k_s_dae_source         15

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
  {_s_dae_extra,          k_s_dae_extra},
  {_s_dae_source,         k_s_dae_source}
};

static size_t fxSamplerCMapLen = 0;

AkResult _assetkit_hide
ak_dae_fxSampler(AkXmlState * __restrict xst,
                 void * __restrict memParent,
                 const char *elm,
                 AkFxSamplerCommon ** __restrict dest) {
  AkFxSamplerCommon *sampler;
  AkXmlElmState      xest;

  sampler = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*sampler));
  ak_setypeid(sampler, AKT_SAMPLER);

  if (fxSamplerCMapLen == 0) {
    fxSamplerCMapLen = AK_ARRAY_LEN(fxSamplerCMap);
    qsort(fxSamplerCMap,
          fxSamplerCMapLen,
          sizeof(fxSamplerCMap[0]),
          ak_enumpair_cmp);
  }

  ak_xest_init(xest, elm)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    fxSamplerCMap,
                    fxSamplerCMapLen,
                    sizeof(fxSamplerCMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case k_s_dae_source: {
        char *source;
        source = ak_xml_val(xst, sampler);

         /* COLLADA 1.4 uses source -> <surface> for texturing */
        if (source) {
          if (xst->version < AK_COLLADA_VERSION_150) {
            ak_dae14_loadjobs_add(xst,
                                  sampler,
                                  source,
                                  AK_DAE14_LOADJOB_SURFACE);
          } else {
            ak_xml_skipelm(xst);
          }
        }

        break;
      }
      case k_s_dae_instance_image: {
        AkInstanceBase *instanceImage;
        AkResult        ret;

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
                               sizeof(*color));
        ret   = ak_dae_color(xst, color, true, false, color);

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

        ak_xml_skipelm(xst);
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

  *dest = sampler;
  
  return AK_OK;
}
