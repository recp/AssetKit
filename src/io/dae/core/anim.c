/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "anim.h"
#include "asset.h"
#include "source.h"
#include "enum.h"

AK_HIDE
void*
dae_anim(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp) {
  AkHeap      *heap;
  AkAnimation *anim;

  heap = dst->heap;
  anim = ak_heap_calloc(heap, memp, sizeof(*anim));

  xmla_setid(xml, heap, anim);
  
  anim->name = DAE_XMLA_STRDUP8(xml, heap, name, anim);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, anim, NULL);
    } else if (DAE_XML_TAG_EQ8(xml, source)) {
      AkSource *source;
      
      /* store interpolation in char */
      if ((source = dae_source(dst, xml, dae_animInterp, AKT_UBYTE))) {
        source->next = anim->source;
        anim->source = source;
      }
    } else if (DAE_XML_TAG_EQ8(xml, sampler)) {
      AkAnimSampler *samp;
      if ((samp = dae_animSampler(dst, xml, anim))) {
        samp->base.next = (void *)anim->sampler;
        anim->sampler   = samp;
      }
    } else if (DAE_XML_TAG_EQ8(xml, channel)) {
      AkChannel *ch;
      if ((ch = dae_channel(dst, xml, anim))) {
        ch->next      = anim->channel;
        anim->channel = ch;
      }
    } else if (DAE_XML_TAG_EQ(xml, animation)) {
      AkAnimation *subAnim;
      if ((subAnim = dae_anim(dst, xml, anim))) {
        subAnim->base.next = (AkOneWayIterBase *)anim->animation;
        anim->animation    = subAnim;
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      anim->extra = tree_fromxml(heap, anim, xml);
    }
    xml = xml->next;
  }

  return anim;
}

AK_HIDE
AkAnimSampler*
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

  if ((att = DAE_XMLA(xml, pre_behavior)))
    samp->pre = dae_animBehavior(att);

  if ((att = DAE_XMLA(xml, post_behavior)))
    samp->post = dae_animBehavior(att);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, input)) {
      inp              = dae_input_new(heap, samp);
      inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xml, semantic),
                                         heap,
                                         inp,
                                         &inp->semantic);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;
        inp->offset   = xmla_u32(DAE_XMLA8(xml, offset), 0);
        
        url           = DAE_URL_FROM(xml, source, memp);
        rb_insert(dst->inputmap, inp, url);
        
        /* check if there are angles, because they are in degress,
         will be converted to radians, we will wait to load whole dae file
         because all sources may not be loaded at this time
         */
        if (inp->semantic == AK_INPUT_OUTPUT)
          flist_sp_insert(&dst->toRadiansSampelers, samp);
        
        switch (inp->semantic) {
          case AK_INPUT_INPUT:
            samp->inputInput = inp;
            break;
          case AK_INPUT_OUTPUT:
            samp->outputInput = inp;
            break;
          case AK_INPUT_IN_TANGENT:
            samp->inTangentInput = inp;
            break;
          case AK_INPUT_OUT_TANGENT:
            samp->outTangentInput = inp;
            break;
          case AK_INPUT_INTERPOLATION:
            samp->interpInput = inp;
            break;
          default:
            break;
        }
        
        inp->next   = samp->input;
        samp->input = inp;
      }
    }
    xml = xml->next;
  }

  return samp;
}

AK_HIDE
AkChannel*
dae_channel(DAEState * __restrict dst,
            void     * __restrict xml,
            void     * __restrict memp) {
  AkChannel *ch;

  ch = ak_heap_calloc(dst->heap, memp, sizeof(*ch));

  DAE_URL_SET(dst, xml, source, ch,  &ch->source);
  ch->target = DAE_XMLA_STRDUP8(xml, dst->heap, target, ch);

  return ch;
}
