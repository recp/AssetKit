/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "skin.h"
#include "../../array.h"
#include "source.h"
#include "enum.h"

AkObject* _assetkit_hide
dae_skin(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp) {
  AkHeap   *heap;
  AkObject *obj;
  AkSkin   *skin;
  char     *sval;
  bool      foundBindShape;

  heap           = dst->heap;
  obj            = ak_objAlloc(heap,
                               memp,
                               sizeof(*skin),
                               AK_CONTROLLER_SKIN,
                               true);
  skin           = ak_objGet(obj);
  foundBindShape = false;

  url_set(dst, xml, _s_dae_source, memp, &skin->baseGeom);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_bind_shape_matrix) && (sval = xml->val)) {
      ak_strtof(&sval, skin->bindShapeMatrix[0], 16);
      glm_mat4_transpose(skin->bindShapeMatrix);
      foundBindShape = true;
    } else if (xml_tag_eq(xml, _s_dae_source)) {
      AkSource *source;
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next = skin->source;
        skin->source = source;
      }
    } else if (xml_tag_eq(xml, _s_dae_joints)) {
      AkInput *inp;
      xml_t   *xjoints;
      
      xjoints = xml->val;
      while (xjoints) {
        if (xml_tag_eq(xjoints, _s_dae_input)) {
          inp              = ak_heap_calloc(heap, obj, sizeof(*inp));
          inp->semanticRaw = xmla_strdup_by(xjoints, heap, _s_dae_semantic, inp);
          
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
            inp->offset   = xmla_uint32(xml_attr(xjoints, _s_dae_offset), 0);
            
            inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
            
            url           = url_from(xjoints, _s_dae_source, memp);
            
            if (inputSemantic == AK_INPUT_SEMANTIC_JOINT) {
              skin->reserved[0] = inp;
              rb_insert(dst->inputmap, inp, url);
            } else if (inputSemantic == AK_INPUT_SEMANTIC_INV_BIND_MATRIX) {
              skin->reserved[1] = inp;
              rb_insert(dst->inputmap, inp, url);
            } else {
              /* do not support other inputs until needed,
               probably will not. */
              ak_free(inp);
            }
          }
        }
        xjoints = xjoints->next;
      }
    } else if (xml_tag_eq(xml, _s_dae_vertex_weights)) {
      AkBoneWeights *wei;
      AkInput       *inp;
      xml_t         *xwei;
      
      wei  = ak_heap_calloc(heap, obj, sizeof(*wei));
      xwei = xml->val;

      while (xwei) {
        if (xml_tag_eq(xwei, _s_dae_input)) {
          inp              = ak_heap_calloc(heap, obj, sizeof(*inp));
          inp->semanticRaw = xmla_strdup_by(xwei, heap, _s_dae_semantic, inp);
          
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
            inp->offset   = xmla_uint32(xml_attr(xwei, _s_dae_offset), 0);
            
            inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
            
            url           = url_from(xwei, _s_dae_source, memp);
            
            if (inputSemantic == AK_INPUT_SEMANTIC_JOINT) {
              skin->reserved[2] = inp;
              rb_insert(dst->inputmap, inp, url);
            } else if (inputSemantic == AK_INPUT_SEMANTIC_WEIGHT) {
              skin->reserved[3] = inp;
              rb_insert(dst->inputmap, inp, url);
            } else {
              /* do not support other inputs until needed,
               probably will not. */
              ak_free(inp);
            }
            
            skin->reserved2++;
          }
        } else if (xml_tag_eq(xwei, _s_dae_vcount) && (sval = xwei->val)) {
          size_t    count, sz, i;
          uint32_t *pSum, *pCount, next;
          
          if ((count = ak_strtok_count_fast(sval, NULL)) > 0) {
            sz = count * sizeof(uint32_t);
            
            /*
             
             we use same temp array to store sum of counts to avoid to
             create new pointer. Array layout:
             
             | pCount | pCountSum |
             
             */
            
            wei->counts  = pCount = ak_heap_alloc(heap, wei, 2 * sz);
            
            /* must equal to position count, we may fix this in postscript */
            wei->nVertex = count;
            
            ak_strtoui_fast(sval, pCount, count);
            
            /* calculate sum */
            pSum = wei->counts + count;
            for (next = i = 0; i < count; i++) {
              pSum[i] = next;
              next    = pCount[i] + next;
            }
          }
        } else if (xml_tag_eq(xwei, _s_dae_v) && (sval = xwei->val)) {
          AkUIntArray *intArray;
          AkResult     ret;
          
          ret = ak_strtoui_array(heap, wei, sval, &intArray);
          if (ret == AK_OK)
            skin->reserved[4] = intArray;
        } else if (xml_tag_eq(xwei, _s_dae_extra)) {
          wei->extra = tree_fromxml(heap, wei, xwei);
        }
        xwei = xwei->next;
      }
      
       skin->weights = (void *)wei;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      skin->extra = tree_fromxml(heap, obj, xml);
    }
    xml = xml->next;
  }

  if (!foundBindShape) {
    glm_mat4_identity(skin->bindShapeMatrix);
  }

  return obj;
}
