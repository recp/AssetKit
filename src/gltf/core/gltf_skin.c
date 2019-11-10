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
//  AkHeap       *heap;
//  AkDoc        *doc;
//  json_t       *jskins, *jaccessors;
//  AkLibItem    *lib;
//  AkController *last_ctlr;
//  size_t        jskinCount, i;
//
//  heap       = gst->heap;
//  doc        = gst->doc;
//  lib        = ak_heap_calloc(heap, doc, sizeof(*lib));
//  last_ctlr  = NULL;
//
//  jskins     = json_object_get(gst->root, _s_gltf_skins);
//  jskinCount = (int32_t)json_array_size(jskins);
//  jaccessors = json_object_get(gst->root, _s_gltf_accessors);
//
//  for (i = 0; i < jskinCount; i++) {
//    AkController *ctlr;
//    AkObject     *obj;
//    AkSkin       *skin;
//    json_t       *jskin, *jinvMatAcc, *jval, *jjoints;
//    const char   *skinid;
//    size_t        jjointsCount, j;
//
//    ctlr = ak_heap_calloc(heap, lib, sizeof(*ctlr));
//
//    ak_setypeid(ctlr, AKT_CONTROLLER);
//
//    ctlr->data = obj = ak_objAlloc(heap,
//                                   ctlr,
//                                   sizeof(*skin),
//                                   AK_CONTROLLER_SKIN,
//                                   true);
//    skin = ak_objGet(obj);
//
//    /* sets id "skinid-[i]" for node. */
//    skinid = ak_id_gen(heap, ctlr, _s_gltf_skin);
//    ak_heap_setId(heap, ak__alignof(ctlr), (void *)skinid);
//
//    glm_mat4_identity(skin->bindShapeMatrix);
//    jskin = json_array_get(jskins, i);
//
//    if ((jval = json_object_get(jskin, _s_gltf_inverseBindMatrices))) {
//      AkAccessor *acc;
//      AkBuffer   *buff;
//
//      jinvMatAcc         = json_array_get(jaccessors, json_integer_value(jval));
//      acc                = gltf_accessor(gst, obj, jinvMatAcc);
//      buff               = ak_getObjectByUrl(&acc->source);
//      skin->invBindPoses = ak_heap_alloc(heap, obj, acc->count * sizeof(mat4));
//
//      for (size_t k = 0; k < acc->count * 16; k++) {
//        *((float *)skin->invBindPoses + k) = *((float *)buff->data + k);
//      }
//    }
//
//    /* map joint index with node pointers */
//    jjoints      = json_object_get(jskin, _s_gltf_joints);
//    jjointsCount = (int32_t)json_array_size(jjoints);
//    skin->joints = ak_heap_alloc(heap, obj, sizeof(void **) * jjointsCount);
//
//    for (j = 0; j < jjointsCount; j++) {
//      char     nodeid[16];
//      AkNode  *node;
//      uint32_t nodeIndex;
//
//      nodeIndex = (uint32_t)json_integer_value(json_array_get(jjoints, j));
//      sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex + 1);
//
//      if ((node = ak_getObjectById(doc, nodeid))) {
//        skin->joints[j] = node;
//      } else {
//        skin->joints[j] = NULL;
//      }
//
//      skin->nJoints++;
//    } /* for */
//
//    if (last_ctlr)
//      last_ctlr->next = ctlr;
//    else
//      lib->chld = ctlr;
//
//    last_ctlr = ctlr;
//    lib->count++;
//  }
  
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jskins;
  AkLibItem          *lib;
  AkController       *last_ctlr;

  if (!(jskins = json_array(jskin)))
    return;

  gst       = userdata;
  heap      = gst->heap;
  doc       = gst->doc;
  jskin     = jskins->base.value;
  lib       = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_ctlr = NULL;
  
  while (jskin) {
    AkController *ctlr;
    AkObject     *obj;
    AkSkin       *skin;
    json_t       *jskinVal;
    const char   *skinid;
    
    jskinVal = jskin->value;

    ctlr = ak_heap_calloc(heap, lib, sizeof(*ctlr));
    
    ak_setypeid(ctlr, AKT_CONTROLLER);
    
    ctlr->data = obj = ak_objAlloc(heap,
                                   ctlr,
                                   sizeof(*skin),
                                   AK_CONTROLLER_SKIN,
                                   true);
    skin = ak_objGet(obj);
    
    /* sets id "skinid-[i]" for node. */
    skinid = ak_id_gen(heap, ctlr, _s_gltf_skin);
    ak_heap_setId(heap, ak__alignof(ctlr), (void *)skinid);
    
    glm_mat4_identity(skin->bindShapeMatrix);
    
    while (jskinVal) {
      if (json_key_eq(jskinVal, _s_gltf_inverseBindMatrices)) {
         AkAccessor   *acc;
         AkBuffer     *buff;
         AkBufferView *buffView;
         
         if ((acc = flist_sp_at(&doc->lib.accessors, json_int32(jskinVal, -1)))
             && (buffView = acc->bufferView)
             && (buff = buffView->buffer)) {
           skin->invBindPoses = ak_heap_alloc(heap, obj,
                                              acc->count * sizeof(mat4));

           for (size_t k = 0; k < acc->count * 16; k++) {
             *((float *)skin->invBindPoses + k) = *((float *)buff->data + k);
           }
         }
      } else if (json_key_eq(jskinVal, _s_gltf_joints)) {
        json_array_t *jjoints;
        json_t       *jjoint;
        uint32_t      jjointsCount, j;
        
        if ((jjoints = json_array(jskinVal))) {
          jjointsCount = jjoints->count;
          skin->joints = ak_heap_alloc(heap,
                                       obj,
                                       sizeof(void **) * jjointsCount);
          
          jjoint = jjoints->base.value;
          j      = jjoints->count - 1; /* json parser is reverse */
          while (jjoint) {
            char     nodeid[16];
            AkNode  *node;
            uint32_t nodeIndex;
            
            if ((nodeIndex = json_uint32(jjoint, -1)) > -1) {
              sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex + 1);
              
              if ((node = ak_getObjectById(doc, nodeid))) {
                skin->joints[j] = node;
              } else {
                skin->joints[j] = NULL;
              }
            } else {
              skin->joints[j] = NULL;
            }

            skin->nJoints++;
            jjoint = jjoint->next;
          } /* while (jjoint) */
        }
      } /* if _s_gltf_joints */
      
      jskinVal = jskinVal->next;
    }
    
    if (last_ctlr)
      last_ctlr->next = ctlr;
    else
      lib->chld = ctlr;
    
    last_ctlr = ctlr;
    lib->count++;
    
    jskin = jskin->next;
  }

  doc->lib.controllers = lib;
}
