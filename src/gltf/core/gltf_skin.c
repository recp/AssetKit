/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_skin.h"
#include "gltf_accessor.h"

void _assetkit_hide
gltf_skin(json_t * __restrict jskin,
          void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jskins;
  AkLibItem          *lib;
  uint32_t            skinIndex;

  if (!(jskins = json_array(jskin)))
    return;

  gst       = userdata;
  heap      = gst->heap;
  doc       = gst->doc;
  jskin     = jskins->base.value;
  lib       = ak_heap_calloc(heap, doc, sizeof(*lib));
  skinIndex = jskins->count - 1;

  while (jskin) {
    AkController *ctlr;
    AkObject     *obj;
    AkSkin       *skin;
    json_t       *jskinVal;
    char          skinid[16];
    
    jskinVal = jskin->value;

    ctlr = ak_heap_calloc(heap, lib, sizeof(*ctlr));

    ak_setypeid(ctlr, AKT_CONTROLLER);

    ctlr->data = obj = ak_objAlloc(heap,
                                   ctlr,
                                   sizeof(*skin),
                                   AK_CONTROLLER_SKIN,
                                   true);
    skin = ak_objGet(obj);
    
    sprintf(skinid, "%s%d", _s_gltf_skin, skinIndex);
    ak_heap_setId(heap,
                  ak__alignof(ctlr),
                  ak_heap_strdup(heap, ctlr, (void *)skinid));

    glm_mat4_identity(skin->bindShapeMatrix);

    while (jskinVal) {
      if (json_key_eq(jskinVal, _s_gltf_inverseBindMatrices)) {
        AkAccessor *acc;
        AkBuffer   *buff;
        float      *pbuff;
        
        if ((acc = flist_sp_at(&doc->lib.accessors, json_int32(jskinVal, -1)))
            && (buff = acc->buffer)) {
          pbuff = (void *)((char *)buff->data + acc->byteOffset);
          skin->invBindPoses = ak_heap_alloc(heap, obj,
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
                                       obj,
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

    ctlr->next = lib->chld;
    lib->chld  = ctlr;
    lib->count++;

    flist_sp_insert(&doc->lib.skins, ctlr);

    skinIndex--;
    jskin = jskin->next;
  }

  doc->lib.controllers = lib;
}
