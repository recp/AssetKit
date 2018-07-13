/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_anim.h"
#include "dae_asset.h"
#include "dae_source.h"
#include "dae_enums.h"

AkResult _assetkit_hide
ak_dae_anim(AkXmlState   * __restrict xst,
            void         * __restrict memParent,
            void        ** __restrict dest) {
  AkAnimation   *anim;
  AkSource      *last_source;
  AkChannel     *last_channel;
  AkAnimation   *last_anim;
  AkAnimSampler *last_samp;
  AkXmlElmState  xest;

  anim = ak_heap_calloc(xst->heap, memParent, sizeof(*anim));

  ak_xml_readid(xst, anim);
  anim->name = ak_xml_attr(xst, anim, _s_dae_name);

  ak_xest_init(xest, _s_dae_animation)

  last_source  = NULL;
  last_channel = NULL;
  last_anim    = NULL;
  last_samp    = NULL;

  do {
    if (ak_xml_begin(&xest))
    break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)ak_dae_assetInf(xst, anim, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_source)) {
      AkSource *source;
      AkResult ret;

      /* store interpolation in char */
      ret = ak_dae_source(xst, anim, ak_dae_enumAnimInterp, 1, &source);
      if (ret == AK_OK) {
        if (last_source)
           last_source->next = source;
        else
           anim->source = source;
        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_sampler)) {
      AkAnimSampler *sampler;
      AkResult       ret;

      ret = ak_dae_animSampler(xst, anim, &sampler);
      if (ret == AK_OK) {
        if (last_samp)
          last_samp->next = sampler;
        else
          anim->sampler = sampler;
        last_samp = sampler;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_channel)) {
      AkChannel *channel;
      AkResult   ret;

      ret = ak_dae_channel(xst, anim, &channel);
      if (ret == AK_OK) {
        if (last_channel)
           last_channel->next = channel;
        else
           anim->channel = channel;
        last_channel = channel;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_animation)) {
      AkAnimation *subAnim;
      AkResult     ret;

      ret     = ak_dae_anim(xst, anim, (void **)&subAnim);
      subAnim = NULL;

      if (ret == AK_OK) {
        if (last_anim)
           last_anim->next = subAnim;
        else
           anim->animation = subAnim;
        last_anim = subAnim;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          anim,
                          nodePtr,
                          &tree,
                          NULL);
      anim->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = anim;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_animSampler(AkXmlState     * __restrict xst,
                   void           * __restrict memParent,
                   AkAnimSampler ** __restrict dest) {
  AkAnimSampler *sampler;
  AkInput       *last_input;
  AkXmlElmState  xest;

  sampler = ak_heap_calloc(xst->heap, memParent, sizeof(*sampler));

  ak_xml_readid(xst, sampler);

  sampler->pre  = ak_xml_attrenum(xst,
                                  _s_dae_pre_behavior,
                                  ak_dae_enumAnimBehavior);
  sampler->post = ak_xml_attrenum(xst,
                                  _s_dae_post_behavior,
                                  ak_dae_enumAnimBehavior);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_sampler);

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, sampler, sizeof(*input));
      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;

        inputSemantic = ak_dae_enumInputSemantic(input->semanticRaw);
        input->offset = ak_xml_attrui(xst, _s_dae_offset);
        input->set    = ak_xml_attrui(xst, _s_dae_set);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;

        if (last_input)
          last_input->next = input;
        else
          sampler->input = input;

        last_input = input;

        ak_xml_attr_url(xst, _s_dae_source, input, &input->source);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = sampler;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_channel(AkXmlState  * __restrict xst,
               void        * __restrict memParent,
               AkChannel  ** __restrict dest) {
  AkChannel     *channel;
  AkXmlElmState  xest;

  channel = ak_heap_calloc(xst->heap, memParent, sizeof(*channel));

  ak_xml_attr_url(xst, _s_dae_source, channel, &channel->source);
  channel->target = ak_xml_attr(xst, channel, _s_dae_target);

  ak_xest_init(xest, _s_dae_channel);

  do {
    if (ak_xml_begin(&xest))
      break;
    /* end element */
    if (ak_xml_end(&xest))
       break;
  } while (xst->nodeRet);

  *dest = channel;

  return AK_OK;
}
