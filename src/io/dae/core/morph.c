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

AkMorph* AK_HIDE
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

  url_set(dst, xml, _s_dae_source, memp, &morphdae->baseGeom);

  if ((att = xmla(xml, _s_dae_method)))
    morph->method = dae_morphMethod(att);
  else
    morph->method = AK_MORPH_METHOD_NORMALIZED;
  
  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source)) {
      AkSource *source;
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next     = morphdae->source;
        morphdae->source = source;
      }
    } else if (xml_tag_eq(xml, _s_dae_targets)) {
      AkMorphTarget *targets;
      AkInput   *inp;
      xml_t     *xtarg;
      
      targets  = ak_heap_calloc(heap, morph, sizeof(*targets));
      xtarg = xml->val;
      
      while (xtarg) {
        if (xml_tag_eq(xtarg, _s_dae_input)) {
          inp              = ak_heap_calloc(heap, morph, sizeof(*inp));
          inp->semanticRaw = xmla_strdup_by(xtarg, heap, _s_dae_semantic, inp);
          
          if (!inp->semanticRaw) {
            ak_free(inp);
          } else {
            AkURL *url;
            AkEnum  inputSemantic;
            
            inputSemantic = dae_semantic(inp->semanticRaw);
            
            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_OTHER;
            
            inp->semantic = inputSemantic;
            inp->offset   = xmla_u32(xmla(xtarg, _s_dae_offset), 0);
            
            inp->semantic = dae_semantic(inp->semanticRaw);
            
            url           = url_from(xtarg, _s_dae_source, memp);
            rb_insert(dst->inputmap, inp, url);

            inp->next      = targets->input;
            targets->input = inp;
          }
        }
        xtarg = xtarg->next;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      morphdae->extra = tree_fromxml(heap, morphdae, xml);
    }
    xml = xml->next;
  }

  return morph;
}
