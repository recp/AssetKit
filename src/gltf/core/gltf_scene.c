/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_scene.h"

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
      for (j = nodeCount - 1; j >= 0; j--) {
        AkNode *nodei;
        nodeIndex = (int32_t)json_integer_value(json_array_get(jnodes, j));

        nodei = doc->lib.nodes->chld;
        while (nodeIndex > 0) {
          nodei = nodei->next;
          nodeIndex--;
        }

        /* create instanceNode for each reference */
        if (nodei) {
          AkInstanceNode *instNode;
          instNode = ak_heap_calloc(heap,
                                    scene,
                                    sizeof(*instNode));

          instNode->base.type    = AK_INSTANCE_NODE;
          instNode->base.url.ptr = nodei;

          if (last_instNode)
            last_instNode->base.next = &instNode->base;
          else
            scene->node->node = instNode;

          last_instNode = instNode;
        }
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
