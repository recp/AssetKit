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

#include "scene.h"
#include "../extra.h"

static
void
gltf_setFirstCamera(AkScene *scene, AkNode *node);

AK_HIDE
void
gltf_scenes(json_t * __restrict jscene,
            void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jscenes;

  if (!(jscenes = json_array(jscene)))
    return;

  gst    = userdata;
  heap   = gst->heap;
  doc    = gst->doc;
  jscene = jscenes->base.value;
  
  while (jscene) {
    AkScene *scene;
    json_t        *jsceneVal;
    
    jsceneVal = jscene->value;
    scene     = ak_heap_calloc(heap, doc, sizeof(*scene));
    ak_setypeid(scene, AKT_SCENE);
    gltf_extra(gst,
               scene,
               GLTF_JSON_GET8(jscene, extras),
               GLTF_JSON_GET(jscene, extensions));
    
    scene->cameras = ak_heap_calloc(heap, scene, sizeof(*scene->cameras));
    scene->lights  = ak_heap_calloc(heap, scene, sizeof(*scene->lights));
    
    /* root node: to store node instances */
    scene->node = ak_heap_calloc(heap, scene, sizeof(*scene->node));
    scene->node->visible = true;
    
    while (jsceneVal) {
      if (GLTF_JSON_KEY_EQ8(jsceneVal, name)) {
        scene->name = json_strdup(jsceneVal, heap, scene);
      } else if (GLTF_JSON_KEY_EQ8(jsceneVal, nodes)) {
        json_array_t *jnodes;
        json_t       *jnode;
        int32_t       nodeIndex;
        
        if (!(jnodes = json_array(jsceneVal)))
          goto scn_nxt;
        
        /* create instanceNode for each node */
        jnode = jnodes->base.value;
        
        while (jnode) {
          AkNode         *node;
          AkInstanceNode *instNode;
          
          instNode  = ak_heap_calloc(heap, scene, sizeof(*instNode));
          if ((nodeIndex = json_int32(jnode, -1)) < 0)
            goto jnode_nxt;
          
          if (!(node = gltf_node_at(gst, nodeIndex)))
            goto jnode_nxt;
          
          instNode->base.node    = node;
          instNode->base.url.ptr = node;
          instNode->base.type    = AK_INSTANCE_NODE;
          
          if (scene->node) {
            if (scene->node->node)
              instNode->base.next = &scene->node->node->base;
            scene->node->node   = instNode;
          }
          
          if (!scene->firstCamNode)
            gltf_setFirstCamera(scene, node);
          
        jnode_nxt:
          jnode = jnode->next;
        }
      }
      jsceneVal = jsceneVal->next;
    }

  scn_nxt:

    AK_LIB_PREPEND(doc->lib.scenes, scene, next);
    
    jscene = jscene->next;
  }
}

AK_HIDE
void
gltf_scene(json_t * __restrict jscene,
           void   * __restrict userdata) {
  AkGLTFState *gst;
  AkDoc       *doc;
  AkScene     *scene;
  int32_t      sceneIndex;

  gst  = userdata;
  doc  = gst->doc;
  
  /* set default scene */
  sceneIndex = json_int32(jscene, -1);
  scene = NULL;
  if (sceneIndex >= 0) {
    scene = doc->lib.scenes.first;
    while (scene && sceneIndex-- > 0)
      scene = scene->next;
  }

  /* set first scene as default scene if not specified  */
  if (scene)
    doc->scene = scene;
}

static
void
gltf_setFirstCamera(AkScene *scene, AkNode *node) {
  if (node->camera) {
    if (!scene->firstCamNode)
      scene->firstCamNode = node;

    ak_instanceListAdd(scene->cameras, node->camera);
    return;
  }

  if (node->chld) {
    AkNode *nodei;
    nodei = node->chld;
    while (nodei) {
      gltf_setFirstCamera(scene, nodei);
      nodei = nodei->next;
    }
  }
}
