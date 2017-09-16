/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_node.h"
#include "../../ak_id.h"

void _assetkit_hide
gltf_nodes(AkGLTFState * __restrict gst) {
  AkHeap     *heap;
  AkDoc      *doc;
  json_t     *jnodes;
  AkLibItem  *lib;
  AkNode     *last_node, *node;
  AkObject   *last_trans;
  AkNode    **nodechld;
  FListItem  *nodes, *nodeItem;
  size_t      jnodesCount, i;

  heap        = gst->heap;
  doc         = gst->doc;
  lib         = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_node   = NULL;
  last_trans  = NULL;
  nodes       = NULL;

  jnodes      = json_object_get(gst->root, _s_gltf_nodes);
  jnodesCount = json_array_size(jnodes);
  nodechld    = ak_calloc(NULL, sizeof(*nodechld) * jnodesCount);

  for (i = 0; i < jnodesCount; i++) {
    json_t     *jnode, *jval;
    const char *nodeid;

    jnode = json_array_get(jnodes, i);

    /* if node is child of another skip it */
    if ((jval = json_object_get(jnode, _s_gltf_ak_flg)))
      continue;

    node = gltf_node(gst, lib, jnode, nodechld);
    if (!node)
      continue;

    /* sets id "node-[i]" for node. */
    nodeid = ak_id_gen(heap, node, _s_gltf_node);

    ak_heap_setId(heap,
                  ak__alignof(node),
                  (void *)nodeid);

    flist_sp_insert(&nodes, node);
  }

  /* all nodes are parsed, now set children */
  nodeItem = nodes;
  i        = jnodesCount - 1;

  while (nodeItem) {
    AkNode *parentNode;

    node       = nodeItem->data;
    parentNode = nodechld[i];

    /* this node ha parent node, move this into parent children link. */
    if (parentNode) {
      AkNode *chld;
      chld = parentNode->chld;
      if (chld) {
        chld->prev = node;
        node->next = chld;
      }

      parentNode->chld = node;
      node->parent     = parentNode;

      /* node ownership */
      ak_heap_setpm(node, parentNode);
    }

    /* it is root node, add to library_nodes */
    else {
      if (last_node)
        last_node->next = node;
      else
        lib->chld = node;

      last_node = node;
      lib->count++;
    }

    nodeItem = nodeItem->next;
    i--;
  }

  flist_sp_destroy(&nodes);
  ak_free(nodechld);

  doc->lib.nodes = lib;
}

AkNode* _assetkit_hide
gltf_node(AkGLTFState * __restrict gst,
          void        * __restrict memParent,
          json_t      * __restrict jnode,
          AkNode     ** __restrict nodechld) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkObject   *last_trans;
  AkNode     *node;
  json_t     *jmesh, *jchld, *jval;
  const char *sval;
  size_t      i;

  heap        = gst->heap;
  doc         = gst->doc;
  last_trans  = NULL;

  node = ak_heap_calloc(heap, memParent, sizeof(*node));
  ak_setypeid(node, AKT_NODE);

  if ((sval = json_cstr(jnode, _s_gltf_name)))
    node->name = ak_heap_strdup(gst->heap, node, sval);

  /* cameras */
  if ((jval = json_object_get(jnode, _s_gltf_camera))) {
    AkCamera *camIter;
    int32_t   camIndex;

    camIndex = (int32_t)json_integer_value(jval);
    camIter  = gst->doc->lib.cameras->chld;
    while (camIndex > 0) {
      camIter = camIter->next;
      camIndex--;
    }

    if (camIter) {
      AkInstanceBase *instCamera;
      instCamera = ak_heap_calloc(heap,
                                  node,
                                  sizeof(*instCamera));
      instCamera->node    = node;
      instCamera->type    = AK_INSTANCE_CAMERA;
      instCamera->url.ptr = camIter;

      node->camera = instCamera;
    }
  }

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

  /* mark child nodes
     we will move the marked nodes into the node after read all nodes.
   */
  if ((jchld = json_object_get(jnode, _s_gltf_children))) {
    AkNode *chld;
    size_t  arrCount, chldIndex;

    arrCount  = json_array_size(jchld);

    for (i = 0; i < arrCount; i++) {
      chldIndex = json_integer_value(json_array_get(jchld, i));
      chld      = nodechld[chldIndex];

      /* this node is already child of another,
         it cannot be child of two node at same time
       */
      if (chld)
        continue;

      nodechld[chldIndex] = node;
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
        matrix->val[i][j] = json_number_value(json_array_get(jval,
                                                             i * 4 + j));
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
      translate->val[i] = json_number_value(json_array_get(jval, i));

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
      rot->val[i] = json_number_value(json_array_get(jval, i));

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
      scale->val[i] = json_number_value(json_array_get(jval, i));

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

  return node;
}
