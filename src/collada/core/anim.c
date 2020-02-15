/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "anim.h"
#include "asset.h"
#include "source.h"
#include "enum.h"

void* _assetkit_hide
dae_anim(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp) {
  AkHeap      *heap;
  AkAnimation *anim;

  heap = dst->heap;
  anim = ak_heap_calloc(heap, memp, sizeof(*anim));

  xmla_setid(xml, heap, anim);
  
  anim->name = xmla_strdup_by(xml, heap, _s_dae_name, anim);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, anim, NULL);
    } else if (xml_tag_eq(xml, _s_dae_source)) {
      AkSource *source;
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next = anim->source;
        anim->source = source;
      }
    } else if (xml_tag_eq(xml, _s_dae_sampler)) {
      AkAnimSampler *samp;
      if ((samp = dae_animSampler(dst, xml, anim))) {
        samp->base.next = (void *)anim->sampler;
        anim->sampler   = samp;
      }
    } else if (xml_tag_eq(xml, _s_dae_channel)) {
      AkChannel *ch;
      if ((ch = dae_channel(dst, xml, anim))) {
        ch->next      = anim->channel;
        anim->channel = ch;
      }
    } else if (xml_tag_eq(xml, _s_dae_animation)) {
      AkAnimation *subAnim;
      if ((subAnim = dae_anim(dst, xml, anim))) {
        subAnim->next   = anim->animation;
        anim->animation = subAnim;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      anim->extra = tree_fromxml(heap, anim, xml);
    }
    xml = xml->next;
  }

  return anim;
}

AkAnimSampler* _assetkit_hide
dae_animSampler(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp) {
  AkHeap        *heap;
  AkAnimSampler *samp;
  AkInput       *inp;
  xml_attr_t    *att;

  heap = dst->heap;
  samp = ak_heap_calloc(heap, memp, sizeof(*samp));

  xmla_setid(xml, heap, samp);

  if ((att = xmla(xml, _s_dae_pre_behavior)))
    samp->pre = dae_enumAnimBehavior(att->val);

  if ((att = xmla(xml, _s_dae_post_behavior)))
    samp->post = dae_enumAnimBehavior(att->val);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      inp              = ak_heap_calloc(heap, samp, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;
        AkEnum  inputSemantic;
        
        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xmla(xml, _s_dae_offset), 0);
        
        inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
        
        url           = url_from(xml, _s_dae_source, memp);
        rb_insert(dst->inputmap, inp, url);
        
        /* check if there are angles, because they are in degress,
         will be converted to radians, we will wait to load whole dae file
         because all sources may not be loaded at this time
         */
        if (inp->semantic == AK_INPUT_SEMANTIC_OUTPUT)
          flist_sp_insert(&dst->toRadiansSampelers, samp);
        
        switch (inp->semantic) {
          case AK_INPUT_SEMANTIC_INPUT:
            samp->inputInput = inp;
            break;
          case AK_INPUT_SEMANTIC_OUTPUT:
            samp->outputInput = inp;
            break;
          case AK_INPUT_SEMANTIC_IN_TANGENT:
            samp->inTangentInput = inp;
            break;
          case AK_INPUT_SEMANTIC_OUT_TANGENT:
            samp->outTangentInput = inp;
            break;
          case AK_INPUT_SEMANTIC_INTERPOLATION:
            samp->interpInput = inp;
            break;
          default:
            break;
        }
      }
    }
    xml = xml->next;
  }

  return samp;
}

AkChannel* _assetkit_hide
dae_channel(DAEState * __restrict dst,
            void     * __restrict xml,
            void     * __restrict memp) {
  AkChannel *ch;

  ch = ak_heap_calloc(dst->heap, memp, sizeof(*ch));

  url_set(dst, xml, _s_dae_source, ch,  &ch->source);
  ch->target = xmla_strdup_by(xml, dst->heap, _s_dae_target, ch);

  return ch;
}
