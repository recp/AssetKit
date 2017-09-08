/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_node.h"

void _assetkit_hide
gltf_nodes(AkGLTFState * __restrict gst) {
  AkHeap     *heap;
  AkDoc      *doc;
  json_t     *jnodes;
  AkLibItem  *lib;
  AkNode     *last_node;
  AkObject   *last_trans;
  size_t      jnodesCount, i;

  heap        = gst->heap;
  doc         = gst->doc;
  lib         = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_node   = NULL;
  last_trans  = NULL;

  jnodes      = json_object_get(gst->root, _s_gltf_nodes);
  jnodesCount = json_array_size(jnodes);

  for (i = 0; i < jnodesCount; i++) {
    AkNode *node;
    json_t *jnode, *jval;

    jnode = json_array_get(jnodes, i);

    /* if node is child of another skip it */
    if ((jval = json_object_get(jnode, _s_gltf_ak_flg)))
      continue;

    node = gltf_node(gst, lib, jnode);

    if (last_node)
      last_node->next = node;
    else
      lib->chld = node;

    last_node = node;
    lib->count++;
  }

  doc->lib.nodes = lib;
}

AkNode* _assetkit_hide
gltf_node(AkGLTFState * __restrict gst,
          void        * __restrict memParent,
          json_t      * __restrict jnode) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkLibItem  *lib;
  AkNode     *last_node;
  AkObject   *last_trans;
  AkNode     *node;
  json_t     *jmesh, *jchld, *jval;
  const char *sval;
  size_t      i;

  heap        = gst->heap;
  doc         = gst->doc;
  lib         = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_node   = NULL;
  last_trans  = NULL;

  node = ak_heap_calloc(heap, memParent, sizeof(*node));
  ak_setypeid(node, AKT_NODE);

  if ((sval = json_cstr(jnode, _s_gltf_name)))
    node->name = ak_heap_strdup(gst->heap, node, sval);

  /* instance geometries */
  if ((jmesh = json_object_get(jnode, _s_gltf_mesh))) {
    AkGeometry *geomIter;
    int32_t     meshIndex;

    meshIndex = (int32_t)json_integer_value(jmesh);
    geomIter  = gst->doc->lib.geometries->chld;
    while (meshIndex > 0) {
      geomIter = geomIter->next;
      meshIndex--;
    }

    if (geomIter) {
      AkInstanceGeometry *instGeom;
      instGeom = ak_heap_calloc(heap,
                                node,
                                sizeof(*instGeom));

      instGeom->base.node    = node;
      instGeom->base.type    = AK_INSTANCE_GEOMETRY;
      instGeom->base.url.ptr = geomIter;

      node->geometry = instGeom;
    }
  }

  /* child nodes */
  if ((jchld = json_object_get(jnode, _s_gltf_children))) {
    AkNode *chld, *last_chld;
    json_t *jchldi;
    size_t  arrCount;

    arrCount  = json_array_size(jchld);
    last_chld = NULL;

    for (i = 0; i < arrCount; i++) {
      jchldi = json_array_get(jchld, i);
      chld   = gltf_node(gst, node, jchldi);

      /* set flag to prevent being re-child of another node */
      json_object_set(jchldi, _s_gltf_ak_flg, json_integer(1));

      if (last_chld)
        last_chld->next = chld;
      else
        node->chld = chld;

      last_chld = chld;
    }
  }

  if ((jval = json_object_get(jnode, _s_gltf_matrix))) {
    AkObject *obj;
    AkMatrix *matrix;
    size_t    arrSize, j;

    arrSize = json_array_size(jval);

    obj = ak_objAlloc(heap,
                      node,
                      sizeof(*matrix),
                      AK_TRANSFORM_MATRIX,
                      true);

    matrix = ak_objGet(obj);
    for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
        matrix->val[i][j] = json_number_value(jval);
      }
    }

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap,
                                       node,
                                       sizeof(*node->transform));
      node->transform->item = obj;
    }

    last_trans = obj;
  }

  if ((jval = json_object_get(jnode, _s_gltf_translation))) {
    AkObject    *obj;
    AkTranslate *translate;
    size_t       arrSize;

    arrSize = json_array_size(jval);

    obj = ak_objAlloc(heap,
                      node,
                      sizeof(*translate),
                      AK_TRANSFORM_TRANSLATE,
                      true);

    translate = ak_objGet(obj);
    for (i = 0; i < 3; i++)
      translate->val[i] = json_number_value(jval);

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap,
                                       node,
                                       sizeof(*node->transform));
      node->transform->item = obj;
    }

    last_trans = obj;
  }

  if ((jval = json_object_get(jnode, _s_gltf_rotation))) {
    AkObject *obj;
    AkRotate *rot;
    size_t    arrSize;

    arrSize = json_array_size(jval);

    obj = ak_objAlloc(heap,
                      node,
                      sizeof(*rot),
                      AK_TRANSFORM_ROTATE,
                      true);

    rot = ak_objGet(obj);
    for (i = 0; i < 4; i++)
      rot->val[i] = json_number_value(jval);

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap,
                                       node,
                                       sizeof(*node->transform));
      node->transform->item = obj;
    }

    last_trans = obj;
  }

  if ((jval = json_object_get(jnode, _s_gltf_scale))) {
    AkObject *obj;
    AkScale  *scale;
    size_t    arrSize;

    arrSize = json_array_size(jval);

    obj = ak_objAlloc(heap,
                      node,
                      sizeof(*scale),
                      AK_TRANSFORM_SCALE,
                      true);

    scale = ak_objGet(obj);
    for (i = 0; i < 3; i++)
      scale->val[i] = json_number_value(jval);

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap,
                                       node,
                                       sizeof(*node->transform));
      node->transform->item = obj;
    }

    last_trans = obj;
  }

  if (last_node)
    last_node->next = node;
  else
    lib->chld = node;

  return node;
}
