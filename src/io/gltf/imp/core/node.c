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

#include "node.h"
#include "../../../../id.h"

#include <ds/hash.h>
#include <string.h>

#define k_name          0
#define k_camera        1
#define k_mesh          2
#define k_skin          3
#define k_children      4
#define k_matrix        5
#define k_translation   6
#define k_rotation      7
#define k_scale         8
#define k_weights       9

AK_HIDE
void
gltf_nodes(json_t * __restrict jnode,
           void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  AkLibrary          *lib;
  AkNode             *node;
  const json_array_t *jnodes;
  FListItem          *nodes;
  AkNode            **nodechld, *parentNode;
  char                nodeid[32];
  int                 i, jnodeCount2;

  if (!(jnodes = json_array(jnode)))
    return;

  gst       = userdata;
  heap      = gst->heap;
  doc       = gst->doc;
  lib       = ak_heap_calloc(heap, doc, sizeof(*lib));
  nodechld  = ak_calloc(NULL, sizeof(*nodechld) * jnodes->count * 2);
  nodes     = NULL;

  jnode       = jnodes->base.value;
  i           = jnodes->count - 1;
  jnodeCount2 = jnodes->count * 2;
  
  while (jnode) {
    nodechld[i * 2] = node = gltf_node(gst, lib, jnode, nodechld);
  
    /* JSON parse is reverse */
    sprintf(nodeid, "%s%d", _s_gltf_node, i);
    ak_heap_setId(heap, ak__alignof(node), ak_heap_strdup(heap, node, nodeid));

    i--;
    jnode = jnode->next;
  }
  
  for (i = jnodeCount2 - 2; i >= 0; i -= 2) {
    node = nodechld[i];
    
    /* this node has parent node, move this into parent children link. */
    if ((parentNode = nodechld[i + 1])) {
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
      node->next = (void *)lib->chld;
      lib->chld  = (void *)node;

      lib->count++;
    }
  }

  flist_sp_destroy(&nodes);
  ak_free(nodechld);

  doc->lib.nodes = lib;
}

AK_HIDE
AkNode*
gltf_node(AkGLTFState * __restrict gst,
          void        * __restrict memParent,
          json_t      * __restrict jnode,
          AkNode     ** __restrict nodechld) {
  AkHeap             *heap;
  AkNode             *node;
  AkGeometry         *geomIter;
  AkInstanceGeometry *instGeom;
  void               *it;
  AkMorph            *morph;
  int32_t             i32val;

  heap     = gst->heap;
  geomIter = NULL;
  instGeom = NULL;

  node = ak_heap_calloc(heap, memParent, sizeof(*node));
  ak_setypeid(node, AKT_NODE);

  json_objmap_t nodeMap[] = {
    JSON_OBJMAP_OBJ(_s_gltf_name,        I2P k_name),
    JSON_OBJMAP_OBJ(_s_gltf_camera,      I2P k_camera),
    JSON_OBJMAP_OBJ(_s_gltf_mesh,        I2P k_mesh),
    JSON_OBJMAP_OBJ(_s_gltf_skin,        I2P k_skin),
    JSON_OBJMAP_OBJ(_s_gltf_children,    I2P k_children),
    JSON_OBJMAP_OBJ(_s_gltf_matrix,      I2P k_matrix),
    JSON_OBJMAP_OBJ(_s_gltf_translation, I2P k_translation),
    JSON_OBJMAP_OBJ(_s_gltf_rotation,    I2P k_rotation),
    JSON_OBJMAP_OBJ(_s_gltf_scale,       I2P k_scale),
    JSON_OBJMAP_OBJ(_s_gltf_weights,     I2P k_weights)
  };

  json_objmap(jnode, nodeMap, JSON_ARR_LEN(nodeMap));

  if ((it = nodeMap[k_name].object)) {
    node->name = json_strdup(it, heap, node);
  }

  if (gst->doc->lib.cameras
      && (i32val = json_int32(nodeMap[k_camera].object, -1)) > -1) {
    AkCamera *camIter;

    GETCHILD(gst->doc->lib.cameras->chld, camIter, i32val);

    if (camIter) {
      AkInstanceBase *instCamera;
      instCamera          = ak_heap_calloc(heap, node, sizeof(*instCamera));
      instCamera->node    = node;
      instCamera->type    = AK_INSTANCE_CAMERA;
      instCamera->url.ptr = camIter;

      node->camera = instCamera;
    }
  }

  /* instance geometries */
  if ((i32val = json_int32(nodeMap[k_mesh].object, -1)) > -1) {
    GETCHILD(gst->doc->lib.geometries->chld, geomIter, i32val);

    /* instance geometry */
    if (geomIter) {
      instGeom               = ak_heap_calloc(heap, node, sizeof(*instGeom));
      instGeom->base.node    = node;
      instGeom->base.type    = AK_INSTANCE_GEOMETRY;
      instGeom->base.url.ptr = geomIter;

      node->geometry         = instGeom;
    } /* if (geomIter) */
  }

  /* children */
  if ((it = nodeMap[k_children].object)) {
    json_array_t *jchildren;
    json_t       *jchld;
    int           chldIndex;

    if ((jchildren = json_array(it))) {
      jchld = jchildren->base.value;

      while (jchld) {
        if ((chldIndex = json_int32(jchld, -1)) > -1) {
          chldIndex = chldIndex * 2 + 1;

          if (!nodechld[chldIndex]) {
            nodechld[chldIndex] = node;
          }

          /* else:
              this node is already child of another,
              it cannot be child of two node at same time
           */
        }

        jchld = jchld->next;
      }
    } /* if children */
  }

  /* first parsed is added to the end so TRS. */

  /* matrix */
  if ((it = nodeMap[k_matrix].object)) {
    AkObject *obj;
    AkMatrix *matrix;

    obj    = ak_objAlloc(heap, node, sizeof(*matrix), AKT_MATRIX, true);
    matrix = ak_objGet(obj);

    json_array_float(matrix->val[0], it, 0.0f, 16, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }
  
  /* scale */
  if ((it = nodeMap[k_scale].object)) {
    AkObject *obj;
    AkScale  *scale;

    obj   = ak_objAlloc(heap, node, sizeof(*obj), AKT_SCALE, true);
    scale = ak_objGet(obj);

    json_array_float(scale->val, it, 0.0f, 3, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }

  /* rotation */
  if ((it = nodeMap[k_rotation].object)) {
    AkObject     *obj;
    AkQuaternion *rot;

    obj = ak_objAlloc(heap, node, sizeof(*obj), AKT_QUATERNION, true);
    rot = ak_objGet(obj);

    json_array_float(rot->val, it, 0.0f, 4, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }

  /* translation */
  if ((it = nodeMap[k_translation].object)) {
    AkObject    *obj;
    AkTranslate *translate;

    obj = ak_objAlloc(heap, node, sizeof(*translate), AKT_TRANSLATE, true);
    translate = ak_objGet(obj);

    json_array_float(translate->val, it, 0.0f, 3, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }
  
  /* morph target weights */
  if (geomIter && instGeom && (morph = rb_find(gst->meshTargets, geomIter))) {
    AkInstanceMorph *morpher;
    AkFloatArray    *weights;
    
    morpher = ak_heap_calloc(heap, node, sizeof(*morpher));
    weights = ak_heap_calloc(heap,
                             morpher,
                             sizeof(*weights)
                             + sizeof(weights->items[0]) * morph->targetCount);

    if ((it = json_array(nodeMap[k_weights].object))) {
      json_array_t *jsonArr;
      jsonArr = it;
      json_array_float(weights->items, it, 0.0f, jsonArr->count, true);
    }

    weights->count           = morph->targetCount;
    
    morpher->overrideWeights = weights;
    morpher->morph           = rb_find(gst->meshTargets, geomIter);
    instGeom->morpher        = morpher;
    
    /* TODO: what if there is no Geomerty? */
  }
  
  /* bind skinnerr after skin is loaded */
  if (instGeom && (i32val = json_int32(nodeMap[k_skin].object, -1)) > -1) {

    rb_insert(gst->skinBound, node, I2P i32val);

    /* TODO: what if there is no Geomerty? */
  }

  return node;
}
