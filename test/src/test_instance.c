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

#include "test_common.h"

#include <limits.h>
#include <math.h>

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

static
bool
ak_test_write_dae_two_roots(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"rootA\" name=\"RootA\"/>"
        "<node id=\"rootB\" name=\"RootB\"/>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_dae_instance_node(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_cameras><camera id=\"cam\"><optics><technique_common>"
        "<perspective><yfov>45</yfov><znear>0.1</znear><zfar>100</zfar></perspective>"
        "</technique_common></optics></camera></library_cameras>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_node name=\"SharedUse\" url=\"#shared\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<library_nodes><node id=\"shared\" name=\"Shared\"><instance_camera url=\"#cam\"/></node></library_nodes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_gltf_root(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"nodes\":[{\"name\":\"Root\"}],"
        "\"scenes\":[{\"nodes\":[0]}],"
        "\"scene\":0"
        "}\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_gltf_light(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"extensionsUsed\":[\"KHR_lights_punctual\"],"
        "\"extensions\":{\"KHR_lights_punctual\":{\"lights\":["
        "{\"type\":\"point\",\"name\":\"Key\",\"intensity\":2.0,\"range\":10.0}"
        "]}},"
        "\"nodes\":[{\"name\":\"LightNode\",\"extensions\":{"
        "\"KHR_lights_punctual\":{\"light\":0}"
        "}}],"
        "\"scenes\":[{\"nodes\":[0]}],"
        "\"scene\":0"
        "}\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_triangle(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("o Triangle\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n",
        file);

  return fclose(file) == 0;
}

static
AkGeometry *
ak_test_make_triangle_geom(AkHeap *heap, void *parent, const float positions[9]) {
  AkGeometry  *geom;
  AkObject    *meshObj;
  AkMesh      *mesh;
  AkTriangles *tri;
  AkInput     *pos;
  AkAccessor  *acc;
  AkBuffer    *buff;

  geom    = ak_heap_calloc(heap, parent, sizeof(*geom));
  meshObj = ak_objAlloc(heap, geom, sizeof(*mesh), AK_GEOMETRY_MESH, true);
  mesh    = ak_objGet(meshObj);
  tri     = ak_heap_calloc(heap, meshObj, sizeof(*tri));
  pos     = ak_heap_calloc(heap, tri, sizeof(*pos));
  acc     = ak_heap_calloc(heap, pos, sizeof(*acc));
  buff    = ak_heap_calloc(heap, acc, sizeof(*buff));

  buff->length = sizeof(float) * 9;
  buff->data   = ak_heap_alloc(heap, buff, buff->length);
  memcpy(buff->data, positions, buff->length);

  acc->buffer                 = buff;
  acc->byteLength             = buff->length;
  acc->byteStride             = sizeof(float) * 3;
  acc->fillByteSize           = sizeof(float) * 3;
  acc->bytesPerComponent      = sizeof(float);
  acc->componentSize          = AK_COMPONENT_SIZE_VEC3;
  acc->componentType          = AKT_FLOAT;
  acc->originalComponentType  = AKT_FLOAT;
  acc->componentCount         = 3;
  acc->count                  = 3;

  pos->accessor = acc;
  pos->semantic = AK_INPUT_POSITION;

  tri->base.mesh      = mesh;
  tri->base.pos       = pos;
  tri->base.input     = pos;
  tri->base.type      = AK_PRIMITIVE_TRIANGLES;
  tri->base.nPolygons = 1;
  tri->mode           = AK_TRIANGLES;

  mesh->geom           = geom;
  mesh->primitive      = (AkMeshPrimitive *)tri;
  mesh->primitiveCount = 1;

  geom->gdata = meshObj;

  return geom;
}

TEST_IMPL(instance_attach_helpers) {
  AkHeap             *heap;
  AkNode             *node, *subNode;
  AkNode             *targetNode;
  AkGeometry         *geomA, *geomB;
  AkInstanceGeometry *instA, *instB;
  AkInstanceNode          *instNode;

  heap  = ak_heap_new(NULL, NULL, NULL);
  node  = ak_heap_calloc(heap, NULL, sizeof(*node));
  targetNode = ak_heap_calloc(heap, NULL, sizeof(*targetNode));
  geomA = ak_heap_calloc(heap, NULL, sizeof(*geomA));
  geomB = ak_heap_calloc(heap, NULL, sizeof(*geomB));

  instA = ak_nodeAttachGeometry(node, geomA);
  ASSERT(instA != NULL);
  ASSERT(node->geometry == instA);
  ASSERT(instA->base.node == node);
  ASSERT(instA->base.object == geomA);
  ASSERT(instA->base.prev == NULL);
  ASSERT(instA->base.next == NULL);

  instB = ak_nodeAttachGeometry(node, geomB);
  ASSERT(instB != NULL);
  ASSERT(node->geometry == instB);
  ASSERT(instB->base.node == node);
  ASSERT(instB->base.object == geomB);
  ASSERT(instB->base.prev == NULL);
  ASSERT(instB->base.next == &instA->base);
  ASSERT(instA->base.prev == &instB->base);
  ASSERT(instA->base.next == NULL);

  subNode = ak_instanceMoveToSubNode(node, &instB->base);
  ASSERT(subNode != NULL);
  ASSERT(subNode->geometry == instB);
  ASSERT(instB->base.node == subNode);
  ASSERT(instB->base.prev == NULL);
  ASSERT(instB->base.next == NULL);
  ASSERT(node->geometry == instA);
  ASSERT(instA->base.prev == NULL);
  ASSERT(instA->base.next == NULL);

  instNode = ak_nodeAttachNodeInstance(node, targetNode);
  ASSERT(instNode != NULL);
  ASSERT(node->node == instNode);
  ASSERT(instNode->owner == node);
  ASSERT(instNode->target == targetNode);
  ASSERT(ak_instanceNodeTarget(instNode) == targetNode);
  ASSERT(instNode->prev == NULL);
  ASSERT(instNode->next == NULL);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_traversal) {
  AkHeap        *heap;
  AkScene       *scene;
  AkNode        *root, *nodeA, *nodeB;
  AkGeometry    *geomA, *geomB;
  AkBoundingBox *bboxA, *bboxB;

  heap  = ak_heap_new(NULL, NULL, NULL);
  scene = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root  = ak_heap_calloc(heap, scene, sizeof(*root));
  nodeA = ak_heap_calloc(heap, NULL, sizeof(*nodeA));
  nodeB = ak_heap_calloc(heap, NULL, sizeof(*nodeB));
  geomA = ak_heap_calloc(heap, NULL, sizeof(*geomA));
  geomB = ak_heap_calloc(heap, NULL, sizeof(*geomB));
  bboxA = ak_heap_calloc(heap, geomA, sizeof(*bboxA));
  bboxB = ak_heap_calloc(heap, geomB, sizeof(*bboxB));

  scene->node = root;

  bboxA->min[0] = 0.0f;
  bboxA->min[1] = 0.0f;
  bboxA->min[2] = 0.0f;
  bboxA->max[0] = 1.0f;
  bboxA->max[1] = 1.0f;
  bboxA->max[2] = 1.0f;
  bboxA->isvalid = true;

  bboxB->min[0] = 10.0f;
  bboxB->min[1] = 0.0f;
  bboxB->min[2] = 0.0f;
  bboxB->max[0] = 11.0f;
  bboxB->max[1] = 1.0f;
  bboxB->max[2] = 1.0f;
  bboxB->isvalid = true;

  geomA->bbox = bboxA;
  geomB->bbox = bboxB;

  ASSERT(ak_nodeAttachGeometry(nodeA, geomA) != NULL);
  ASSERT(ak_nodeAttachGeometry(nodeB, geomB) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, nodeA) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, nodeB) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 11.0f);

  bboxB->min[0] = 5.0f;
  bboxB->max[0] = 6.0f;
  ak_bbox_scene(scene);

  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 6.0f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_lazy_geometry) {
  AkHeap     *heap;
  AkScene    *scene;
  AkNode     *root, *target;
  AkGeometry *geom;
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  heap   = ak_heap_new(NULL, NULL, NULL);
  scene  = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root   = ak_heap_calloc(heap, scene, sizeof(*root));
  target = ak_heap_calloc(heap, NULL, sizeof(*target));
  geom   = ak_test_make_triangle_geom(heap, NULL, positions);

  scene->node = root;

  ASSERT(geom->bbox == NULL);
  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, target) != NULL);

  ak_bbox_scene(scene);

  ASSERT(geom->bbox != NULL);
  ASSERT(geom->bbox->isvalid);
  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(isfinite(scene->bbox->min[0]));
  ASSERT(isfinite(scene->bbox->max[0]));
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 1.0f);
  ASSERT(scene->bbox->min[1] == 0.0f);
  ASSERT(scene->bbox->max[1] == 1.0f);
  ASSERT(scene->bbox->min[2] == 0.0f);
  ASSERT(scene->bbox->max[2] == 0.0f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_path_state) {
  AkHeap        *heap;
  AkScene       *scene;
  AkNode        *root, *ownerA, *ownerB, *target;
  AkGeometry    *geom;
  AkBoundingBox *bbox;
  float          identity[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
  float          translateX[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    10.0f, 0.0f, 0.0f, 1.0f
  };

  heap   = ak_heap_new(NULL, NULL, NULL);
  scene  = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root   = ak_heap_calloc(heap, scene, sizeof(*root));
  ownerA = ak_heap_calloc(heap, NULL, sizeof(*ownerA));
  ownerB = ak_heap_calloc(heap, NULL, sizeof(*ownerB));
  target = ak_heap_calloc(heap, NULL, sizeof(*target));
  geom   = ak_heap_calloc(heap, NULL, sizeof(*geom));
  bbox   = ak_heap_calloc(heap, geom, sizeof(*bbox));

  scene->node = root;

  bbox->min[0] = 0.0f;
  bbox->min[1] = 0.0f;
  bbox->min[2] = 0.0f;
  bbox->max[0] = 1.0f;
  bbox->max[1] = 1.0f;
  bbox->max[2] = 1.0f;
  bbox->isvalid = true;
  geom->bbox = bbox;

  ak_nodeSetTransformMatrix(ownerA, identity);
  ak_nodeSetTransformMatrix(ownerB, translateX);

  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, ownerA) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, ownerB) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(ownerA, target) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(ownerB, target) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 11.0f);
  ASSERT(target->matrixWorld == NULL);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_rotated_ref) {
  AkHeap        *heap;
  AkScene       *scene;
  AkNode        *root, *target;
  AkGeometry    *geom;
  AkBoundingBox *bbox;
  float          rotZ45[16] = {
    0.70710678f, 0.70710678f, 0.0f, 0.0f,
   -0.70710678f, 0.70710678f, 0.0f, 0.0f,
    0.0f,        0.0f,        1.0f, 0.0f,
    0.0f,        0.0f,        0.0f, 1.0f
  };

  heap   = ak_heap_new(NULL, NULL, NULL);
  scene  = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root   = ak_heap_calloc(heap, scene, sizeof(*root));
  target = ak_heap_calloc(heap, NULL, sizeof(*target));
  geom   = ak_heap_calloc(heap, NULL, sizeof(*geom));
  bbox   = ak_heap_calloc(heap, geom, sizeof(*bbox));

  scene->node = root;

  bbox->min[0] = 0.0f;
  bbox->min[1] = 0.0f;
  bbox->min[2] = 0.0f;
  bbox->max[0] = 1.0f;
  bbox->max[1] = 1.0f;
  bbox->max[2] = 0.0f;
  bbox->isvalid = true;
  geom->bbox = bbox;

  ak_nodeSetTransformMatrix(root, rotZ45);

  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, target) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(fabsf(scene->bbox->min[0] + 0.70710678f) < 0.001f);
  ASSERT(fabsf(scene->bbox->max[0] - 0.70710678f) < 0.001f);
  ASSERT(fabsf(scene->bbox->min[1]) < 0.001f);
  ASSERT(fabsf(scene->bbox->max[1] - 1.41421356f) < 0.001f);
  ASSERT(target->matrixWorld == NULL);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_camera_world_path) {
  AkHeap   *heap;
  AkDoc    *doc;
  AkScene  *scene;
  AkNode   *root, *camNode;
  AkCamera *camera;
  AkCamera *found;
  float     rootTrans[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    5.0f, 0.0f, 0.0f, 1.0f
  };
  float     camTrans[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    10.0f, 0.0f, 0.0f, 1.0f
  };
  float     matrix[16];

  heap    = ak_heap_new(NULL, NULL, NULL);
  doc     = ak_heap_calloc(heap, NULL, sizeof(*doc));
  scene   = ak_heap_calloc(heap, doc, sizeof(*scene));
  root    = ak_heap_calloc(heap, scene, sizeof(*root));
  camNode = ak_heap_calloc(heap, doc, sizeof(*camNode));

  doc->scene           = scene;
  scene->node          = root;
  scene->firstCamNode  = camNode;

  ak_nodeSetTransformMatrix(root, rootTrans);
  ak_nodeSetTransformMatrix(camNode, camTrans);

  camera = ak_camMakePerspective(doc, doc, 1.0f, 1.0f, 0.1f, 100.0f);
  ASSERT(camera != NULL);
  ASSERT(ak_nodeAttachCamera(camNode, camera) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, camNode) != NULL);

  ASSERT(ak_firstCamera(doc, &found, matrix, NULL) == AK_OK);
  ASSERT(found == camera);
  ASSERT(fabsf(matrix[12] - 15.0f) < 0.001f);
  ASSERT(fabsf(matrix[13]) < 0.001f);
  ASSERT(fabsf(matrix[14]) < 0.001f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(scene_find_or_make_root_uses_child_roots) {
  AkHeap  *heap;
  AkDoc   *doc;
  AkScene *scene;
  AkNode  *madeRoot;
  AkNode  *sameRoot;
  AkNode  *otherRoot;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  scene = ak_heap_calloc(heap, doc, sizeof(*scene));

  madeRoot = ak_sceneFindOrMakeRoot(doc, scene, "User Cameras");
  ASSERT(madeRoot != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld == madeRoot);
  ASSERT(scene->node->node == NULL);
  ASSERT(scene->node->geometry == NULL);
  ASSERT(madeRoot->parent == scene->node);
  ASSERT(madeRoot->prev == NULL);
  ASSERT(madeRoot->next == NULL);
  ASSERT(doc->lib.nodes.count == 1);

  sameRoot = ak_sceneFindOrMakeRoot(doc, scene, "User Cameras");
  ASSERT(sameRoot == madeRoot);
  ASSERT(doc->lib.nodes.count == 1);
  ASSERT(scene->node->chld->next == NULL);

  otherRoot = ak_sceneFindOrMakeRoot(doc, scene, "Other Root");
  ASSERT(otherRoot != NULL);
  ASSERT(otherRoot != madeRoot);
  ASSERT(scene->node->chld == otherRoot);
  ASSERT(scene->node->chld->next == madeRoot);
  ASSERT(otherRoot->parent == scene->node);
  ASSERT(madeRoot->parent == scene->node);
  ASSERT(doc->lib.nodes.count == 2);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(dae_scene_roots_are_child_nodes) {
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *rootA, *rootB;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  const char  *tmpBase;
  uint32_t     rootCount;
  AkNode      *root;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-roots-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/roots.dae", tmpdir);
  ASSERT(ak_test_write_dae_two_roots(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);
  ASSERT(scene->node->geometry == NULL);
  ASSERT(scene->node->next == NULL);
  ASSERT(doc->lib.nodes.count == 2);

  rootA = ak_sceneFindRoot(scene, "RootA");
  rootB = ak_sceneFindRoot(scene, "RootB");
  ASSERT(rootA != NULL);
  ASSERT(rootB != NULL);
  ASSERT(rootA != rootB);
  ASSERT(rootA->name && strcmp(rootA->name, "RootA") == 0);
  ASSERT(rootB->name && strcmp(rootB->name, "RootB") == 0);
  ASSERT(rootA->parent == scene->node);
  ASSERT(rootB->parent == scene->node);

  rootCount = 0;
  for (root = scene->node->chld; root; root = root->next)
    rootCount++;
  ASSERT(rootCount == 2);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_instance_node_is_instance_node) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  AkNode     *shared;
  AkInstanceNode *inst;
  AkInstanceBase *camInst;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        daePath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-instance-node-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/instance_node.dae", tmpdir);
  ASSERT(ak_test_write_dae_instance_node(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(doc->lib.nodes.count == 2);

  root = ak_sceneFindRoot(scene, "Root");
  ASSERT(root != NULL);
  ASSERT(root->node != NULL);

  inst = root->node;
  ASSERT(inst->owner == root);
  ASSERT(inst->target != NULL);
  ASSERT(inst->reserved != NULL);
  ASSERT(inst->name && strcmp(inst->name, "SharedUse") == 0);
  ASSERT(inst->proxy == NULL);

  shared = ak_instanceNodeTarget(inst);
  ASSERT(shared != NULL);
  ASSERT(shared == inst->target);
  ASSERT(shared->name && strcmp(shared->name, "Shared") == 0);
  ASSERT(scene->firstCamNode == shared);
  ASSERT(scene->cameras.count == 1);
  ASSERT(scene->cameras.useCount == 1);
  ASSERT(scene->cameras.first != NULL);
  ASSERT(scene->cameras.first->camera != NULL);
  camInst = scene->cameras.first->firstInstance;
  ASSERT(camInst != NULL);
  ASSERT(camInst->node == shared);
  ASSERT(ak_instanceObject(camInst) == scene->cameras.first->camera);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_scene_root_is_child_node) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        gltfPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-root-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/root.gltf", tmpdir);
  ASSERT(ak_test_write_gltf_root(gltfPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);

  root = scene->node->chld;
  ASSERT(root != NULL);
  ASSERT(root != scene->node);
  ASSERT(root->parent == scene->node);
  ASSERT(root->name && strcmp(root->name, "Root") == 0);
  ASSERT(root->next == NULL);
  ASSERT(doc->lib.nodes.count == 1);

  ak_free(doc);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_scene_light_cache) {
  AkDoc      *doc;
  AkScene    *scene;
  AkLight    *light;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        gltfPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-light-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/light.gltf", tmpdir);
  ASSERT(ak_test_write_gltf_light(gltfPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->lights.count == 1);
  ASSERT(scene->lights.useCount == 1);
  ASSERT(scene->lights.first != NULL);
  ASSERT(scene->lights.first->firstInstance != NULL);
  ASSERT(scene->lights.first->firstInstance->node == scene->node->chld);

  light = scene->lights.first->light;
  ASSERT(light != NULL);
  ASSERT(ak_instanceObject(scene->lights.first->firstInstance) == light);
  ASSERT(light->name && strcmp(light->name, "Key") == 0);
  ASSERT(light->data != NULL);
  ASSERT(light->data->type == AK_LIGHT_TYPE_POINT);
  ASSERT(fabsf(light->data->intensity - 2.0f) < 0.001f);
  ASSERT(fabsf(light->data->range - 10.0f) < 0.001f);

  ak_free(doc);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(obj_scene_root_is_child_node) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        objPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-obj-root-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(objPath, sizeof(objPath), "%s/triangle.obj", tmpdir);
  ASSERT(ak_test_write_obj_triangle(objPath));
  ASSERT(ak_load(&doc, objPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);
  ASSERT(scene->node->geometry == NULL);

  root = scene->node->chld;
  ASSERT(root != NULL);
  ASSERT(root != scene->node);
  ASSERT(root->parent == scene->node);
  ASSERT(root->geometry != NULL);
  ASSERT(doc->lib.nodes.count == 1);

  ak_free(doc);
  unlink(objPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
