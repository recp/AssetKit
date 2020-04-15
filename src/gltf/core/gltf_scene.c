/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_scene.h"

static
void
gltf_setFirstCamera(AkVisualScene *scene, AkNode *node);

void _assetkit_hide
gltf_scenes(json_t * __restrict jscene,
            void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jscenes;
  AkLibrary          *lib;

  if (!(jscenes = json_array(jscene)))
    return;

  gst    = userdata;
  heap   = gst->heap;
  doc    = gst->doc;
  jscene = jscenes->base.value;
  lib    = ak_heap_calloc(heap, doc, sizeof(*lib));
  
  while (jscene) {
    AkVisualScene *scene;
    json_t        *jsceneVal;
    
    jsceneVal = jscene->value;
    scene     = ak_heap_calloc(heap, lib, sizeof(*scene));
    ak_setypeid(scene, AKT_SCENE);
    
    scene->cameras = ak_heap_calloc(heap, scene, sizeof(*scene->cameras));
    
    /* root node: to store node instances */
    scene->node = ak_heap_calloc(heap, scene, sizeof(*scene->node));
    
    while (jsceneVal) {
      if (json_key_eq(jsceneVal, _s_gltf_name)) {
        scene->name = json_strdup(jsceneVal, heap, scene);
      } else if (json_key_eq(jsceneVal, _s_gltf_nodes)) {
        json_array_t *jnodes;
        json_t       *jnode;
        int32_t       nodeIndex;
        
        if (!(jnodes = json_array(jsceneVal)))
          goto scn_nxt;
        
        /* create instanceNode for each node */
        jnode = jnodes->base.value;
        
        while (jnode) {
          char            nodeid[16];
          AkNode         *node;
          AkInstanceNode *instNode;
          
          instNode  = ak_heap_calloc(heap, scene, sizeof(*instNode));
          if ((nodeIndex = json_int32(jnode, -1)) < 0)
            goto jnode_nxt;
          
          sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex);
          if (!(node = ak_getObjectById(doc, nodeid)))
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

    scene->base.next = lib->chld;
    lib->chld        = (void *)scene;
    lib->count++;
    
    jscene = jscene->next;
  }
  
   doc->lib.visualScenes = lib;
}

void _assetkit_hide
gltf_scene(json_t * __restrict jscene,
           void   * __restrict userdata) {
  AkGLTFState   *gst;
  AkHeap        *heap;
  AkDoc         *doc;
  AkVisualScene *scene;
  int32_t        sceneIndex;

  gst  = userdata;
  heap = gst->heap;
  doc  = gst->doc;
  
  /* set default scene */
  sceneIndex = json_int32(jscene, -1);
  GETCHILD(doc->lib.visualScenes->chld, scene, sceneIndex);

  /* set first scene as default scene if not specified  */
  if (scene) {
    AkInstanceBase *instScene;
    instScene = ak_heap_calloc(heap, doc, sizeof(*instScene));
    
    instScene->url.ptr     = scene;
    doc->scene.visualScene = instScene;
  }
}

static
void
gltf_setFirstCamera(AkVisualScene *scene, AkNode *node) {
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
