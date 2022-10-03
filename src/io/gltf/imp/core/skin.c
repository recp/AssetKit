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
#include "accessor.h"

AK_HIDE
void
gltf_skin(json_t * __restrict jskin,
          void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jskins;
  uint32_t            skinIndex;

  if (!(jskins = json_array(jskin)))
    return;

  gst       = userdata;
  heap      = gst->heap;
  doc       = gst->doc;
  jskin     = jskins->base.value;
  skinIndex = jskins->count - 1;

  while (jskin) {
    AkSkin *skin;
    json_t *jskinVal;
    char    skinid[16];
    
    jskinVal = jskin->value;
    skin     = ak_heap_calloc(heap, doc, sizeof(*skin));

    sprintf(skinid, "%s%d", _s_gltf_skin, skinIndex);
    ak_heap_setId(heap,
                  ak__alignof(skin),
                  ak_heap_strdup(heap, skin, (void *)skinid));

    glm_mat4_identity(skin->bindShapeMatrix);

    while (jskinVal) {
      if (json_key_eq(jskinVal, _s_gltf_inverseBindMatrices)) {
        AkAccessor *acc;
        AkBuffer   *buff;
        float      *pbuff;
        
        if ((acc = flist_sp_at(&doc->lib.accessors, json_int32(jskinVal, -1)))
            && (buff = acc->buffer)) {
          pbuff = (void *)((char *)buff->data + acc->byteOffset);
          skin->invBindPoses = ak_heap_alloc(heap, skin,
                                             acc->count * sizeof(mat4));
          
          for (size_t k = 0; k < acc->count * 16; k++) {
            *((float *)skin->invBindPoses + k) = *(pbuff + k);
          }
        }
      } else if (json_key_eq(jskinVal, _s_gltf_joints)) {
        json_array_t *jjoints;
        json_t       *jjoint;
        uint32_t      j;
        
        if ((jjoints = json_array(jskinVal))) {
          skin->joints = ak_heap_alloc(heap,
                                       skin,
                                       sizeof(void **) * jjoints->count);

          jjoint = jjoints->base.value;
          j      = jjoints->count - 1; /* json parser is reverse */
          while (jjoint) {
            char     nodeid[16];
            AkNode  *node;
            int32_t  nodeIndex;

            if ((nodeIndex = json_int32(jjoint, -1)) > -1) {
              sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex);

              if ((node = ak_getObjectById(doc, nodeid))) {
                skin->joints[j] = node;
              } else {
                skin->joints[j] = NULL;
              }
            } else {
              skin->joints[j] = NULL;
            }

            j--;
            skin->nJoints++;
            jjoint = jjoint->next;
          } /* while (jjoint) */
        }
      } /* if _s_gltf_joints */

      jskinVal = jskinVal->next;
    }
    
    if (doc->lib.skins)
      skin->base.next = &doc->lib.skins->base;
    
    doc->lib.skins = skin;

    skinIndex--;
    jskin = jskin->next;
  }
}
