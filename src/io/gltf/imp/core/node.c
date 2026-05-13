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
#include "ext.h"
#include "../extra.h"
#include "../../../../id.h"

#include <ds/hash.h>
#include <string.h>

typedef struct AkGLTFNodeProps {
  json_t *name;
  json_t *camera;
  json_t *mesh;
  json_t *skin;
  json_t *children;
  json_t *matrix;
  json_t *translation;
  json_t *rotation;
  json_t *scale;
  json_t *weights;
  json_t *extensions;
  json_t *extras;
} AkGLTFNodeProps;

static inline
void
gltf_nodeProps(json_t          * __restrict jnode,
               AkGLTFNodeProps * __restrict props) {
  json_t *it;
  char    first;

  if (!jnode || jnode->type != JSON_OBJECT)
    return;

  for (it = jnode->value; it; it = it->next) {
    if (!it->key)
      continue;

    first = it->key[0];

    switch (it->keysize) {
      case 4:
        if (first == 'n' && gltf_jsonKeyEqLen(it, _s_gltf_name, 4)) {
          props->name = it;
        } else if (first == 'm' && gltf_jsonKeyEqLen(it, _s_gltf_mesh, 4)) {
          props->mesh = it;
        } else if (first == 's' && gltf_jsonKeyEqLen(it, _s_gltf_skin, 4)) {
          props->skin = it;
        }
        break;
      case 5:
        if (first == 's' && gltf_jsonKeyEqLen(it, _s_gltf_scale, 5))
          props->scale = it;
        break;
      case 6:
        if (first == 'c' && gltf_jsonKeyEqLen(it, _s_gltf_camera, 6)) {
          props->camera = it;
        } else if (first == 'm' && gltf_jsonKeyEqLen(it, _s_gltf_matrix, 6)) {
          props->matrix = it;
        } else if (first == 'e' && gltf_jsonKeyEqLen(it, _s_gltf_extras, 6)) {
          props->extras = it;
        }
        break;
      case 7:
        if (first == 'w' && gltf_jsonKeyEqLen(it, _s_gltf_weights, 7))
          props->weights = it;
        break;
      case 8:
        if (first == 'c' && gltf_jsonKeyEqLen(it, _s_gltf_children, 8)) {
          props->children = it;
        } else if (first == 'r' && gltf_jsonKeyEqLen(it, _s_gltf_rotation, 8)) {
          props->rotation = it;
        }
        break;
      case 10:
        if (first == 'e' && gltf_jsonKeyEqLen(it, _s_gltf_extensions, 10))
          props->extensions = it;
        break;
      case 11:
        if (first == 't' && gltf_jsonKeyEqLen(it, _s_gltf_translation, 11))
          props->translation = it;
        break;
      default:
        break;
    }
  }
}

static inline
void
gltf_nodeId(char * __restrict dst, unsigned int index) {
  char         tmp[16];
  char        *p;
  unsigned int n;

  memcpy(dst, _s_gltf_node, 4);
  p = dst + 4;
  n = 0;

  do {
    tmp[n++] = (char)('0' + index % 10);
    index /= 10;
  } while (index);

  do {
    *p++ = tmp[--n];
  } while (n);

  *p = '\0';
}

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
  gst->nodesCount   = jnodes->count;
  gst->nodesByIndex = ak_heap_calloc(heap,
                                     gst->tmpParent,
                                     sizeof(*gst->nodesByIndex)
                                     * gst->nodesCount);
  nodechld  = ak_calloc(NULL, sizeof(*nodechld) * jnodes->count * 2);
  nodes     = NULL;

  jnode       = jnodes->base.value;
  i           = jnodes->count - 1;
  jnodeCount2 = jnodes->count * 2;
  
  while (jnode) {
    nodechld[i * 2] = node = gltf_node(gst, lib, jnode, nodechld);
    gst->nodesByIndex[i] = node;
  
    /* JSON parse is reverse */
    gltf_nodeId(nodeid, (unsigned int)i);
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
      
      /* Keep heap ownership in the node library; parent is logical scene graph. */
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
  AkGLTFNodeProps     props;
  void               *it;
  AkMorph            *morph;
  int32_t             i32val;

  heap     = gst->heap;
  geomIter = NULL;
  instGeom = NULL;
  props    = (AkGLTFNodeProps){0};

  node = ak_heap_calloc(heap, memParent, sizeof(*node));
  ak_setypeid(node, AKT_NODE);
  node->visible = true;

  gltf_nodeProps(jnode, &props);

  if ((it = props.name)) {
    node->name = json_strdup(it, heap, node);
  }

  gltf_extra(gst,
             node,
             props.extras,
             props.extensions);

  if ((it = props.extensions)
      && !gltf_ext_node(gst, node, it)) {
    gst->stop = true;
    return node;
  }

  if (gst->doc->lib.cameras
      && (i32val = json_int32(props.camera, -1)) > -1) {
    AkCamera *camIter;

    camIter = gltf_camera_at(gst, i32val);

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
  if ((i32val = json_int32(props.mesh, -1)) > -1) {
    geomIter = gltf_geometry_at(gst, i32val);

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
  if ((it = props.children)) {
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
  if ((it = props.matrix)) {
    AkObject *obj;
    AkMatrix *matrix;

    obj    = ak_objAlloc(heap, node, sizeof(*matrix), AKT_MATRIX, false);
    matrix = ak_objGet(obj);

    json_array_float(matrix->val[0], it, 0.0f, 16, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }
  
  /* scale */
  if ((it = props.scale)) {
    AkObject *obj;
    AkScale  *scale;

    obj   = ak_objAlloc(heap, node, sizeof(*scale), AKT_SCALE, false);
    scale = ak_objGet(obj);

    json_array_float(scale->val, it, 0.0f, 3, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }

  /* rotation */
  if ((it = props.rotation)) {
    AkObject     *obj;
    AkQuaternion *rot;

    obj = ak_objAlloc(heap, node, sizeof(*rot), AKT_QUATERNION, false);
    rot = ak_objGet(obj);

    json_array_float(rot->val, it, 0.0f, 4, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }

  /* translation */
  if ((it = props.translation)) {
    AkObject    *obj;
    AkTranslate *translate;

    obj = ak_objAlloc(heap, node, sizeof(*translate), AKT_TRANSLATE, false);
    translate = ak_objGet(obj);

    json_array_float(translate->val, it, 0.0f, 3, true);

    if (!node->transform) {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
    }
    
    obj->next             = node->transform->item;
    node->transform->item = obj;
  }
  
  /* morph instance + (optional) node-level weight override */
  if (geomIter && instGeom && (morph = rb_find(gst->meshTargets, geomIter))) {
    AkInstanceMorph *morpher;

    morpher = ak_heap_calloc(heap, node, sizeof(*morpher));
    ak_setypeid(morpher, AKT_MORPH_INST);

    morpher->morph    = morph;
    instGeom->morpher = morpher;

    /* overrideWeights is set ONLY when the glTF node carries an explicit
       "weights" property. Otherwise it stays NULL so that:
         - ak_morphHasOverride()     returns false
         - defaults flow through:    morph.defaultWeights → mesh.weights → 0
       Allocating a zero-filled array unconditionally would silently override
       any defaults — see ak_morphInspect_initialWeight precedence. */
    if ((it = json_array(props.weights))) {
      AkFloatArray *weights;
      json_array_t *jsonArr;

      jsonArr = it;
      weights = ak_heap_calloc(heap,
                               morpher,
                               sizeof(*weights)
                               + sizeof(weights->items[0]) * morph->targetCount);
      json_array_float(weights->items, it, 0.0f, jsonArr->count, true);
      weights->count           = morph->targetCount;
      morpher->overrideWeights = weights;
    }
  }
  
  /* bind skinnerr after skin is loaded */
  if (instGeom && (i32val = json_int32(props.skin, -1)) > -1) {

    rb_insert(gst->skinBound, node, I2P i32val);

    /* TODO: what if there is no Geomerty? */
  }

  return node;
}
