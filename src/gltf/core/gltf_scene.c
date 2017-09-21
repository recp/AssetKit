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
gltf_scenes(AkGLTFState * __restrict gst) {
  AkHeap        *heap;
  AkDoc         *doc;
  json_t        *jscenes;
  AkLibItem     *lib;
  AkVisualScene *last_scene;
  size_t         jsceneCount, i;

  heap        = gst->heap;
  doc         = gst->doc;
  lib         = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_scene  = NULL;

  jscenes     = json_object_get(gst->root, _s_gltf_scenes);
  jsceneCount = json_array_size(jscenes);
  for (i = 0; i < jsceneCount; i++) {
    AkVisualScene *scene;
    json_t        *jscene, *jnodes;
    const char    *sval;

    jscene = json_array_get(jscenes, i);
    scene  = ak_heap_calloc(heap, lib, sizeof(*scene));

    scene->cameras = ak_heap_calloc(heap,
                                    scene,
                                    sizeof(*scene->cameras));

    /* root node: to store node instances */
    scene->node = ak_heap_calloc(heap,
                                 scene,
                                 sizeof(*scene->node));

    if ((sval = json_cstr(jscene, _s_gltf_name)))
      scene->name = ak_heap_strdup(gst->heap, scene, sval);

    if ((jnodes = json_object_get(jscene, _s_gltf_nodes))) {
      AkInstanceNode *last_instNode;
      int32_t j, nodeCount, nodeIndex;

      last_instNode = NULL;
      nodeCount     = (int32_t)json_array_size(jnodes);

      /* create instanceNode for each node */
      for (j = nodeCount - 1; j >= 0; j--) {
        char            nodeid[16];
        AkNode         *node;
        AkInstanceNode *instNode;

        nodeIndex = (int32_t)json_integer_value(json_array_get(jnodes, j));
        instNode  = ak_heap_calloc(heap, scene, sizeof(*instNode));

        sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex + 1);
        if (!(node = ak_getObjectById(doc, nodeid)))
          continue;

        instNode->base.node    = node;
        instNode->base.url.ptr = node;
        instNode->base.type    = AK_INSTANCE_NODE;

        if (last_instNode)
          last_instNode->base.next = &instNode->base;
        else
          scene->node->node = instNode;

        last_instNode = instNode;

        if (!scene->firstCamNode)
          gltf_setFirstCamera(scene, node);
      }
    }

    if (last_scene)
      last_scene->next = scene;
    else
      lib->chld = scene;

    last_scene = scene;
    lib->count++;
  }

  doc->lib.visualScenes = lib;
}

static
void
gltf_setFirstCamera(AkVisualScene *scene, AkNode *node) {
  if (node->camera) {
    if (!scene->firstCamNode)
      scene->firstCamNode = node;

    ak_instanceListAdd(scene->cameras,
                       node->camera);

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
