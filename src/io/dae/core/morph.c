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

#include "morph.h"
#include "source.h"
#include "enum.h"
#include "../../../array.h"

AK_HIDE
AkMorph*
dae_morph(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap     *heap;
  AkMorph    *morph;
  AkMorphDAE *morphdae;
  xml_attr_t *att;

  heap     = dst->heap;
  morph    = ak_heap_calloc(heap, memp, sizeof(*morph));
  morphdae = ak_heap_calloc(heap, morph, sizeof(*morphdae));

  ak_heap_setUserData(heap, morph, morphdae);
  flist_sp_insert(&dst->linkedUserData, morph);

  DAE_URL_SET(dst, xml, source, memp, &morphdae->baseGeom);

  if ((att = DAE_XMLA8(xml, method)))
    morph->method = dae_morphMethod(att);
  else
    morph->method = AK_MORPH_METHOD_NORMALIZED;
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, source)) {
      DaeSource *source;
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next     = morphdae->source;
        morphdae->source = source;
      }
    } else if (DAE_XML_TAG_EQ8(xml, targets)) {
      AkInput   *inp;
      xml_t     *xtarg;
      
      xtarg = xml->val;
      
      while (xtarg) {
        if (DAE_XML_TAG_EQ8(xtarg, input)) {
          inp              = dae_input_new(heap, morphdae);
          inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xtarg, semantic),
                                             heap,
                                             inp,
                                             &inp->semantic);
          
          if (!inp->semanticRaw) {
            ak_free(inp);
          } else {
            AkURL *url;
            inp->indexOffset = xmla_u32(DAE_XMLA8(xtarg, offset), 0);
            url              = DAE_URL_FROM(xtarg, source, memp);
            rb_insert(dst->inputmap, inp, url);

            inp->next       = morphdae->input;
            morphdae->input = inp;
          }
        }
        xtarg = xtarg->next;
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      morphdae->extra = tree_fromxml(heap, morphdae, xml);
    }
    xml = xml->next;
  }

  return morph;
}
