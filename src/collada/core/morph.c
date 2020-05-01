/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "morph.h"
#include "source.h"
#include "enum.h"
#include "../../array.h"

//AkObject* _assetkit_hide
//dae_morph(DAEState * __restrict dst,
//          xml_t    * __restrict xml,
//          void     * __restrict memp) {
//  AkHeap     *heap;
//  AkObject   *obj;
//  AkMorph    *morph;
//  xml_attr_t *att;
//
//  heap  = dst->heap;
//  obj   = ak_objAlloc(heap, memp, sizeof(*morph), AK_CONTROLLER_MORPH, true);
//  morph = ak_objGet(obj);
//
//  url_set(dst, xml, _s_dae_source, memp, &morph->baseGeom);
//
//  if ((att = xmla(xml, _s_dae_method)))
//    morph->method = dae_morphMethod(att);
//  else
//    morph->method = AK_MORPH_METHOD_NORMALIZED;
//  
//  xml = xml->val;
//  while (xml) {
//    if (xml_tag_eq(xml, _s_dae_source)) {
//      AkSource *source;
//      if ((source = dae_source(dst, xml, NULL, 0))) {
//        source->next  = morph->source;
//        morph->source = source;
//      }
//    } else if (xml_tag_eq(xml, _s_dae_targets)) {
//      AkMorphTarget *targets;
//      AkInput   *inp;
//      xml_t     *xtarg;
//      
//      targets  = ak_heap_calloc(heap,   obj, sizeof(*targets));
//      xtarg = xml->val;
//      
//      while (xtarg) {
//        if (xml_tag_eq(xtarg, _s_dae_input)) {
//          inp              = ak_heap_calloc(heap, obj, sizeof(*inp));
//          inp->semanticRaw = xmla_strdup_by(xtarg, heap, _s_dae_semantic, inp);
//          
//          if (!inp->semanticRaw) {
//            ak_free(inp);
//          } else {
//            AkURL *url;
//            AkEnum  inputSemantic;
//            
//            inputSemantic = dae_semantic(inp->semanticRaw);
//            inp->semantic = inputSemantic;
//            
//            if (inputSemantic < 0)
//              inputSemantic = AK_INPUT_SEMANTIC_OTHER;
//            
//            inp->semantic = inputSemantic;
//            inp->offset   = xmla_u32(xmla(xtarg, _s_dae_offset), 0);
//            
//            inp->semantic = dae_semantic(inp->semanticRaw);
//            
//            url           = url_from(xtarg, _s_dae_source, memp);
//            rb_insert(dst->inputmap, inp, url);
//
//            inp->next      = targets->input;
//            targets->input = inp;
//          }
//        } else if (xml_tag_eq(xml, _s_dae_extra)) {
//          targets->extra = tree_fromxml(heap, targets, xml);
//        }
//        xtarg = xtarg->next;
//      }
//    } else if (xml_tag_eq(xml, _s_dae_extra)) {
//      morph->extra = tree_fromxml(heap, obj, xml);
//    }
//    xml = xml->next;
//  }
//
//  return obj;
//}
