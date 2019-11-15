/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_node.h"
#include "../../id.h"

#include <ds/hash.h>
#include <string.h>
#include <json/print.h>

#define k_name          0
#define k_camera        1
#define k_mesh          2
#define k_skin          3
#define k_children      4
#define k_matrix        5
#define k_translation   6
#define k_rotation      7
#define k_scale         8

//void _assetkit_hide
//gltf_nodes(AkGLTFState * __restrict gst) {
//  AkHeap     *heap;
//  AkDoc      *doc;
//  json_t     *jnodes;
//  AkLibItem  *lib;
//  AkNode     *last_node, *node;
//  AkObject   *last_trans;
//  AkNode    **nodechld;
//  FListItem  *nodes, *nodeItem;
//  size_t      jnodesCount, i;
//
//  heap        = gst->heap;
//  doc         = gst->doc;
//  lib         = ak_heap_calloc(heap, doc, sizeof(*lib));
//  last_node   = NULL;
//  last_trans  = NULL;
//  nodes       = NULL;
//
//  jnodes      = json_object_get(gst->root, _s_gltf_nodes);
//  jnodesCount = json_array_size(jnodes);
//  nodechld    = ak_calloc(NULL, sizeof(*nodechld) * jnodesCount);
//
//  for (i = 0; i < jnodesCount; i++) {
//    json_t     *jnode, *jval;
//    const char *nodeid;
//
//    jnode = json_array_get(jnodes, i);
//
//    /* if node is child of another skip it */
//    if ((jval = json_object_get(jnode, _s_gltf_ak_flg)))
//      continue;
//
//    node = gltf_node(gst, lib, jnode, nodechld);
//    if (!node)
//      continue;
//
//    /* sets id "node-[i]" for node. */
//    nodeid = ak_id_gen(heap, node, _s_gltf_node);
//    ak_heap_setId(heap, ak__alignof(node), (void *)nodeid);
//
//    flist_sp_insert(&nodes, node);
//  }
//
//  /* all nodes are parsed, now set children */
//  nodeItem = nodes;
//  i        = jnodesCount - 1;
//
//  while (nodeItem) {
//    AkNode *parentNode;
//
//    node       = nodeItem->data;
//    parentNode = nodechld[i];
//
//    /* this node ha parent node, move this into parent children link. */
//    if (parentNode) {
//      AkNode *chld;
//      chld = parentNode->chld;
//      if (chld) {
//        chld->prev = node;
//        node->next = chld;
//      }
//
//      parentNode->chld = node;
//      node->parent     = parentNode;
//
//      /* node ownership */
//      ak_heap_setpm(node, parentNode);
//    }
//
//    /* it is root node, add to library_nodes */
//    else {
//      if (last_node)
//        last_node->next = node;
//      else
//        lib->chld = node;
//
//      last_node = node;
//      lib->count++;
//    }
//
//    nodeItem = nodeItem->next;
//    i--;
//  }
//
//  flist_sp_destroy(&nodes);
//  ak_free(nodechld);
//
//  doc->lib.nodes = lib;
//}
//
//AkNode* _assetkit_hide
//gltf_node(AkGLTFState * __restrict gst,
//          void        * __restrict memParent,
//          json_t      * __restrict jnode,
//          AkNode     ** __restrict nodechld) {
//  AkHeap     *heap;
//  AkObject   *last_trans;
//  AkNode     *node;
//  json_t     *jmesh, *jchld, *jval;
//  const char *sval;
//  size_t      i;
//
//  heap        = gst->heap;
//  last_trans  = NULL;
//
//  node = ak_heap_calloc(heap, memParent, sizeof(*node));
//  ak_setypeid(node, AKT_NODE);
//
//  if ((sval = json_cstr(jnode, _s_gltf_name)))
//    node->name = ak_heap_strdup(gst->heap, node, sval);
//
//  /* cameras */
//  if ((jval = json_object_get(jnode, _s_gltf_camera))) {
//    AkCamera *camIter;
//    int32_t   camIndex;
//
//    camIndex = (int32_t)json_integer_value(jval);
//    camIter  = gst->doc->lib.cameras->chld;
//    while (camIndex > 0) {
//      camIter = camIter->next;
//      camIndex--;
//    }
//
//    if (camIter) {
//      AkInstanceBase *instCamera;
//      instCamera = ak_heap_calloc(heap,
//                                  node,
//                                  sizeof(*instCamera));
//      instCamera->node    = node;
//      instCamera->type    = AK_INSTANCE_CAMERA;
//      instCamera->url.ptr = camIter;
//
//      node->camera = instCamera;
//    }
//  }
//
//  /* instance geometries */
//  if ((jmesh = json_object_get(jnode, _s_gltf_mesh))) {
//    AkGeometry *geomIter;
//    int32_t     meshIndex;
//
//    meshIndex = i = (int32_t)json_integer_value(jmesh);
//    if ((geomIter  = gst->doc->lib.geometries->chld) && i > 0) {
//      while (i > 0) {
//        if (!(geomIter = geomIter->next))
//          goto n_chld;  /* not foud */
//        i--;
//      }
//    }
//
//    /* instance geometry */
//    if (geomIter) {
//      AkInstanceGeometry *instGeom;
//      instGeom               = ak_heap_calloc(heap, node, sizeof(*instGeom));
//      instGeom->base.node    = node;
//      instGeom->base.type    = AK_INSTANCE_GEOMETRY;
//      instGeom->base.url.ptr = geomIter;
//
//      gltf_bindMaterials(gst, instGeom, meshIndex);
//
//      if ((jval = json_object_get(jnode, _s_gltf_skin))) {
//        char                  skinid[16];
//        AkInstanceController *instCtlr;
//        int32_t               skinIndex;
//
//        instCtlr               = ak_heap_calloc(heap, node, sizeof(*instCtlr));
//        instCtlr->base.type    = AK_INSTANCE_CONTROLLER;
//        instCtlr->geometry.ptr = instGeom;
//
//        skinIndex = (int32_t)json_integer_value(jval);
//        sprintf(skinid, "%s%d", _s_gltf_skin, skinIndex + 1);
//
//        ak_url_init_with_id(heap->allocator,
//                            instCtlr,
//                            skinid,
//                            &instCtlr->base.url);
//
//        node->controller = instCtlr;
//      } else {
//        /* check mesh's morph targets */
//        AkObject *prim;
//        AkResult  ret;
//
//        prim = geomIter->gdata;
//        switch ((AkGeometryType)prim->type) {
//          case AK_GEOMETRY_TYPE_MESH: {
//            AkMesh *mesh;
//            AkController *ctlr;
//
//            mesh = ak_objGet(prim);
//            if ((ctlr = rb_find(gst->meshTargets, mesh))) {
//              AkInstanceController *instCtlr;
//
//              instCtlr               = ak_heap_calloc(heap,
//                                                      node,
//                                                      sizeof(*instCtlr));
//              instCtlr->base.type    = AK_INSTANCE_CONTROLLER;
//              instCtlr->geometry.ptr = instGeom;
//              instCtlr->base.url.ptr = ctlr;
//
//              node->controller = instCtlr;
//            }
//            break;
//          }
//          default:
//            ret = AK_ERR;
//        }
//
//        if (!node->controller) {
//          node->geometry = instGeom;
//        }
//      }
//    }
//  }
//
//  /* mark child nodes
//     we will move the marked nodes into the node after read all nodes.
//   */
//n_chld:
//  if ((jchld = json_object_get(jnode, _s_gltf_children))) {
//    AkNode *chld;
//    size_t  arrCount, chldIndex;
//
//    arrCount  = json_array_size(jchld);
//
//    for (i = 0; i < arrCount; i++) {
//      chldIndex = json_integer_value(json_array_get(jchld, i));
//      chld      = nodechld[chldIndex];
//
//      /* this node is already child of another,
//         it cannot be child of two node at same time
//       */
//      if (chld)
//        continue;
//
//      nodechld[chldIndex] = node;
//    }
//  }
//
//  if ((jval = json_object_get(jnode, _s_gltf_matrix))) {
//    AkObject *obj;
//    AkMatrix *matrix;
//    size_t    j;
//
//    obj = ak_objAlloc(heap,
//                      node,
//                      sizeof(*matrix),
//                      AKT_MATRIX,
//                      true);
//
//    matrix = ak_objGet(obj);
//    for (i = 0; i < 4; i++) {
//      for (j = 0; j < 4; j++) {
//        matrix->val[i][j] = json_number_value(json_array_get(jval, i * 4 + j));
//      }
//    }
//
//    if (last_trans)
//      last_trans->next = obj;
//    else {
//      node->transform = ak_heap_calloc(heap,
//                                       node,
//                                       sizeof(*node->transform));
//      node->transform->item = obj;
//    }
//
//    last_trans = obj;
//  }
//
//  if ((jval = json_object_get(jnode, _s_gltf_translation))) {
//    AkObject    *obj;
//    AkTranslate *translate;
//
//    obj = ak_objAlloc(heap,
//                      node,
//                      sizeof(*translate),
//                      AKT_TRANSLATE,
//                      true);
//
//    translate = ak_objGet(obj);
//    for (i = 0; i < 3; i++)
//      translate->val[i] = json_number_value(json_array_get(jval, i));
//
//    if (last_trans)
//      last_trans->next = obj;
//    else {
//      node->transform = ak_heap_calloc(heap,
//                                       node,
//                                       sizeof(*node->transform));
//      node->transform->item = obj;
//    }
//
//    last_trans = obj;
//  }
//
//  if ((jval = json_object_get(jnode, _s_gltf_rotation))) {
//    AkObject     *obj;
//    AkQuaternion *rot;
//
//    obj = ak_objAlloc(heap,
//                      node,
//                      sizeof(*rot),
//                      AKT_QUATERNION,
//                      true);
//
//    rot = ak_objGet(obj);
//    rot->val[0] = json_number_value(json_array_get(jval, 0));
//    rot->val[1] = json_number_value(json_array_get(jval, 1));
//    rot->val[2] = json_number_value(json_array_get(jval, 2));
//    rot->val[3] = json_number_value(json_array_get(jval, 3));
//
//    if (last_trans)
//      last_trans->next = obj;
//    else {
//      node->transform = ak_heap_calloc(heap,
//                                       node,
//                                       sizeof(*node->transform));
//      node->transform->item = obj;
//    }
//
//    last_trans = obj;
//  }
//
//  if ((jval = json_object_get(jnode, _s_gltf_scale))) {
//    AkObject *obj;
//    AkScale  *scale;
//
//    obj = ak_objAlloc(heap,
//                      node,
//                      sizeof(*scale),
//                      AKT_SCALE,
//                      true);
//
//    scale = ak_objGet(obj);
//    for (i = 0; i < 3; i++)
//      scale->val[i] = json_number_value(json_array_get(jval, i));
//
//    if (last_trans)
//      last_trans->next = obj;
//    else {
//      node->transform = ak_heap_calloc(heap,
//                                       node,
//                                       sizeof(*node->transform));
//      node->transform->item = obj;
//    }
//  }
//
//  return node;
//}
//
void _assetkit_hide
gltf_bindMaterials(AkGLTFState        * __restrict gst,
                   AkInstanceGeometry * __restrict instGeom,
                   int32_t                         meshIndex) {
//  AkHeap             *heap;
//  AkBindMaterial     *bindMat;
//  AkInstanceMaterial *last_instMat;
//  json_t             *jmesh, *jmeshes, *jprims, *jprim, *jmat, *jattribs, *jval;
//  const char         *jkey;
//  HTable             *semanticMap;
//  size_t              jprimCount, i;
//  bool                materialFound;
//
//  heap        = gst->heap;
//  jmeshes     = json_object_get(gst->root, _s_gltf_meshes);
//  jmesh       = json_array_get(jmeshes, meshIndex); /* it was loaded */
//  semanticMap = hash_new_str(8);
//
//  /* instance material */
//  if (!(jprims = json_object_get(jmesh, _s_gltf_primitives)))
//    return;
//
//  bindMat       = ak_heap_calloc(heap, instGeom, sizeof(*bindMat));
//  jprimCount    = (int32_t)json_array_size(jprims);
//  last_instMat  = NULL;
//  materialFound = false;
//
//  for (i = 0; i < jprimCount; i++) {
//    AkMaterial         *mat;
//    AkInstanceMaterial *instMat;
//    AkBindVertexInput  *last_bvi;
//    char               *materialId, *symbol;
//    size_t              len;
//    int32_t             matIndex;
//
//    jprim = json_array_get(jprims, i);
//
//    /* bind material */
//    if (!(jmat = json_object_get(jprim, _s_gltf_material)))
//      continue;
//
//    matIndex = (int32_t)json_integer_value(jmat);
//    mat      = gst->doc->lib.materials->chld;
//    while (mat && matIndex > 0) {
//      mat = mat->next;
//      matIndex--;
//    }
//
//    /* we can use material id as semantic */
//    if (!mat)
//      continue;
//
//    materialFound = true;
//    instMat       = ak_heap_calloc(heap, bindMat, sizeof(*instMat));
//
//    materialId    = ak_mem_getId(mat);
//    len           = strlen(materialId) + ak_digitsize(i) + 1;
//    symbol        = ak_heap_alloc(heap, instMat, len + 1);
//    symbol[len]   = '\0';
//    sprintf(symbol, "%s-%zu", materialId, i);
//
//    if (last_instMat)
//      last_instMat->base.next = &instMat->base;
//    else
//      bindMat->tcommon = instMat;
//    last_instMat = instMat;
//
//    instMat->symbol  = symbol;
//    last_bvi         = NULL;
//
//    ak_url_init_with_id(heap->allocator,
//                        instMat,
//                        materialId,
//                        &instMat->base.url);
//
//    jattribs = json_object_get(jprim, _s_gltf_attributes);
//    json_object_foreach(jattribs, jkey, jval) {
//      char   *pInpIndex;
//      char   *input;
//      char   *bviSem;
//      int32_t set;
//
//      set       = 0;
//      pInpIndex = strchr(jkey, '_');
//
//      if (!pInpIndex) {
//        input = ak_heap_strdup(heap, instMat, jkey);
//      }
//
//      /* ARRAYs e.g. TEXTURE_0, TEXTURE_1 */
//      else {
//        input = ak_heap_strndup(heap, instMat, jkey, pInpIndex - jkey);
//        if (strlen(pInpIndex) > 1) /* default is 0 with calloc */
//          set = (uint32_t)strtol(pInpIndex + 1, NULL, 10);
//      }
//
//      /* currently bind only TEXCOORDs to inputs */
//      if (strcasecmp(input, _s_gltf_TEXCOORD) == 0) {
//
//        /* find if it is already bound */
//        len         = strlen(_s_gltf_sid_texcoord) + ak_digitsize(set);
//        bviSem      = ak_heap_alloc(heap, NULL, len + 1);
//        bviSem[len] = '\0';
//        sprintf(bviSem, "%s%d", _s_gltf_sid_texcoord, set);
//
//        if (!hash_get(semanticMap, bviSem)) {
//          AkBindVertexInput *bvi;
//
//          bvi = ak_heap_calloc(heap, instMat,  sizeof(*bvi));
//
//          ak_heap_setpm(input,  bvi);
//          ak_heap_setpm(bviSem, bvi);
//
//          bvi->inputSet      = set;
//          bvi->semantic      = bviSem;
//          bvi->inputSemantic = input;
//
//          if (last_bvi)
//            last_bvi->next = bvi;
//          else
//            instMat->bindVertexInput = bvi;
//          last_bvi = bvi;
//        }
//      }
//
//      else {
//        ak_free(input); /* for now */
//      }
//    } /* json_object_foreach */
//  } /* for */
//
//  if (!materialFound) {
//    AkInstanceMaterial *instMat, *tofree;
//    instMat = bindMat->tcommon;
//    while (instMat) {
//      tofree = instMat;
//      instMat = (AkInstanceMaterial *)instMat->base.next;
//      ak_free(tofree);
//    }
//
//    ak_free(bindMat);
//  } else {
//    instGeom->bindMaterial = bindMat;
//  }
//
//  hash_destroy(semanticMap);
}

void _assetkit_hide
gltf_nodes(json_t * __restrict jnode,
           void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  AkLibItem          *lib;
  AkNode             *last_node, *node;
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
  last_node = NULL;
  nodes     = NULL;

  jnode       = jnodes->base.value;
  i           = jnodes->count - 1;
  jnodeCount2 = jnodes->count * 2;
  
  
  while (jnode) {
    nodechld[i * 2] = node = gltf_node(gst, lib, jnode, nodechld);
  
    /* JSON parse is reverse */
    /* sets id "node-[i]" for node. */
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
      if (last_node)
        last_node->next = node;
      else
        lib->chld = node;
      
      node->prev = last_node;
      last_node  = node;
      lib->count++;
    }
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
  AkHeap             *heap;
  AkNode             *node;
  AkObject           *last_trans;
  AkGeometry         *geomIter;
  AkInstanceGeometry *instGeom;
  void               *it;
  int32_t             i32val;

  heap       = gst->heap;
  last_trans = NULL;
  geomIter   = NULL;
  instGeom   = NULL;

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
    JSON_OBJMAP_OBJ(_s_gltf_scale,       I2P k_scale)
  };

  json_objmap(jnode, nodeMap, JSON_ARR_LEN(nodeMap));

  if ((it = nodeMap[k_name].object)) {
    node->name = json_strdup(it, heap, node);
  }

  if ((i32val = json_int32(nodeMap[k_camera].object, -1)) > -1) {
    AkCamera *camIter;

    GETCHILD(gst->doc->lib.cameras->chld, camIter, i32val);

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
  if ((i32val = json_int32(nodeMap[k_mesh].object, -1)) > -1) {
    GETCHILD(gst->doc->lib.geometries->chld, geomIter, i32val);

    /* instance geometry */
    if (geomIter) {
      instGeom               = ak_heap_calloc(heap, node, sizeof(*instGeom));
      instGeom->base.node    = node;
      instGeom->base.type    = AK_INSTANCE_GEOMETRY;
      instGeom->base.url.ptr = geomIter;


      gltf_bindMaterials(gst, instGeom, i32val);
    } /* if (geomIter) */
  }

  /* skin */
  if ((i32val = json_int32(nodeMap[k_skin].object, -1) > -1)) {
    char                  skinid[16];
    AkInstanceController *instCtlr;

    instCtlr               = ak_heap_calloc(heap, node, sizeof(*instCtlr));
    instCtlr->base.type    = AK_INSTANCE_CONTROLLER;
    instCtlr->geometry.ptr = instGeom;

    sprintf(skinid, "%s%d", _s_gltf_skin, i32val + 1);

    ak_url_init_with_id(heap->allocator,
                        instCtlr,
                        skinid,
                        &instCtlr->base.url);

    node->controller = instCtlr;
  }
  
  if (!node->controller)
    node->geometry = instGeom;

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

  /* matrix */
  if ((it = nodeMap[k_matrix].object)) {
    AkObject *obj;
    AkMatrix *matrix;
    float     rawMatrix[4][4];

    obj    = ak_objAlloc(heap, node, sizeof(*matrix), AKT_MATRIX, true);
    matrix = ak_objGet(obj);

    json_array_float(rawMatrix[0], it, 0.0f, 16, true);
    glm_mat4_ucopy(rawMatrix, matrix->val);

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      node->transform->item = obj;
    }

    last_trans = obj;
  }

  /* translation */
  if ((it = nodeMap[k_translation].object)) {
    AkObject    *obj;
    AkTranslate *translate;

    obj = ak_objAlloc(heap, node, sizeof(*translate), AKT_TRANSLATE, true);
    translate = ak_objGet(obj);

    json_array_float(translate->val, it, 0.0f, 3, true);

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      node->transform->item = obj;
    }

    last_trans = obj;
  }

  /* rotation */
  if ((it = nodeMap[k_rotation].object)) {
    AkObject     *obj;
    AkQuaternion *rot;

    obj = ak_objAlloc(heap, node, sizeof(*obj), AKT_QUATERNION, true);
    rot = ak_objGet(obj);

    json_array_float(rot->val, it, 0.0f, 4, true);

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      node->transform->item = obj;
    }

    last_trans = obj;
  }

  /* scale */
  if ((it = nodeMap[k_scale].object)) {
    AkObject *obj;
    AkScale  *scale;

    obj   = ak_objAlloc(heap, node, sizeof(*obj), AKT_SCALE, true);
    scale = ak_objGet(obj);

    json_array_float(scale->val, it, 0.0f, 3, true);

    if (last_trans)
      last_trans->next = obj;
    else {
      node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      node->transform->item = obj;
    }

    /* last_trans = obj; */
  }

  return node;
}
