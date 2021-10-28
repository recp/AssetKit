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

#include "skin.h"
#include "../../../array.h"
#include "source.h"
#include "enum.h"

AK_HIDE
AkSkin*
dae_skin(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp) {
  AkHeap      *heap;
  AkSkin      *skin;
  AkSkinDAE   *skindae;
  const xml_t *sval;
  bool         foundBindShape;

  heap           = dst->heap;
  skin           = ak_heap_calloc(heap, memp, sizeof(*skin));
  skindae        = ak_heap_calloc(heap, skin, sizeof(*skindae));
  foundBindShape = false;
  
  ak_heap_setUserData(heap, skin, skindae);
  flist_sp_insert(&dst->linkedUserData, skin);

  url_set(dst, xml, _s_dae_source, memp, &skindae->baseGeom);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_bind_shape_matrix) && (sval = xmls(xml))) {
      xml_strtof_fast(sval, skin->bindShapeMatrix[0], 16);
      glm_mat4_transpose(skin->bindShapeMatrix);
      foundBindShape = true;
    } else if (xml_tag_eq(xml, _s_dae_source)) {
      AkSource *source;
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next    = skindae->source;
        skindae->source = source;
      }
    } else if (xml_tag_eq(xml, _s_dae_joints)) {
      AkInput *inp;
      xml_t   *xjoints;
      
      xjoints = xml->val;
      while (xjoints) {
        if (xml_tag_eq(xjoints, _s_dae_input)) {
          inp              = ak_heap_calloc(heap, skin, sizeof(*inp));
          inp->semanticRaw = xmla_strdup_by(xjoints, heap, _s_dae_semantic, inp);
          
          if (!inp->semanticRaw) {
            ak_free(inp);
          } else {
            AkURL *url;
            AkEnum  inputSemantic;
            
            inputSemantic = dae_semantic(inp->semanticRaw);
            
            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_OTHER;
            
            inp->semantic = inputSemantic;
            inp->offset   = xmla_u32(xmla(xjoints, _s_dae_offset), 0);
            
            inp->semantic = dae_semantic(inp->semanticRaw);
            
            url           = url_from(xjoints, _s_dae_source, memp);
            
            if (inputSemantic == AK_INPUT_JOINT) {
              skindae->joints.joints = inp;
              rb_insert(dst->inputmap, inp, url);
            } else if (inputSemantic == AK_INPUT_INV_BIND_MATRIX) {
              skindae->joints.invBindMatrix = inp;
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
      
      wei  = ak_heap_calloc(heap, skin, sizeof(*wei));
      xwei = xml->val;

      while (xwei) {
        if (xml_tag_eq(xwei, _s_dae_input)) {
          inp              = ak_heap_calloc(heap, skin, sizeof(*inp));
          inp->semanticRaw = xmla_strdup_by(xwei, heap, _s_dae_semantic, inp);
          
          if (!inp->semanticRaw) {
            ak_free(inp);
          } else {
            AkURL *url;
            AkEnum  inputSemantic;
            
            inputSemantic = dae_semantic(inp->semanticRaw);
            
            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_OTHER;
            
            inp->semantic = inputSemantic;
            inp->offset   = xmla_u32(xmla(xwei, _s_dae_offset), 0);
            
            inp->semantic = dae_semantic(inp->semanticRaw);
            
            url           = url_from(xwei, _s_dae_source, memp);
            
            if (inputSemantic == AK_INPUT_JOINT) {
              skindae->weights.joints = inp;
              rb_insert(dst->inputmap, inp, url);
            } else if (inputSemantic == AK_INPUT_WEIGHT) {
              skindae->weights.weights = inp;
              rb_insert(dst->inputmap, inp, url);
            } else {
              /* do not support other inputs until needed,
               probably will not. */
              ak_free(inp);
            }
            
            skindae->inputCount++;
          }
        } else if (xml_tag_eq(xwei, _s_dae_vcount) && (sval = xmls(xwei))) {
          size_t    count, sz, i;
          uint32_t *pSum, *pCount, next;
          
          if ((count = xml_strtok_count_fast(sval, NULL)) > 0) {
            sz = count * sizeof(uint32_t);
            
            /*
             
             we use same temp array to store sum of counts to avoid to
             create new pointer. Array layout:
             
             | pCount | pCountSum |
             
             */
            
            wei->counts  = pCount = ak_heap_alloc(heap, wei, 2 * sz);
            
            /* must equal to position count, we may fix this in postscript */
            wei->nVertex = count;
            
            xml_strtoui_fast(sval, pCount, (unsigned long)count);
            
            /* calculate sum */
            pSum = wei->counts + count;
            for (i = next = 0; i < count; i++) {
              pSum[i] = next;
              next    = pCount[i] + next;
            }
          }
        } else if (xml_tag_eq(xwei, _s_dae_v) && (sval = xmls(xwei))) {
          AkUIntArray *intArray;
          AkResult     ret;
          
          ret = xml_strtoui_array(heap, wei, sval, &intArray);
          if (ret == AK_OK)
            skindae->weights.v = intArray;
        }
        
        /*
        else if (xml_tag_eq(xwei, _s_dae_extra)) {
          wei->extra = tree_fromxml(heap, wei, xwei);
        }
        */
        xwei = xwei->next;
      }
      
       skin->weights = (void *)wei;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      skindae->extra = tree_fromxml(heap, skindae, xml);
    }
    xml = xml->next;
  }

  if (!foundBindShape) {
    glm_mat4_identity(skin->bindShapeMatrix);
  }

  return skin;
}
