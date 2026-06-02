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
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_node url=\"#shared\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<library_nodes><node id=\"shared\" name=\"Shared\"/></library_nodes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
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

TEST_IMPL(instance_attach_helpers) {
  AkHeap             *heap;
  AkNode             *node, *subNode;
  AkNode             *targetNode;
  AkGeometry         *geomA, *geomB;
  AkInstanceGeometry *instA, *instB;
  AkNodeRef          *nodeRef;

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

  nodeRef = ak_nodeAttachNodeRef(node, targetNode);
  ASSERT(nodeRef != NULL);
  ASSERT(node->nodeRefs == nodeRef);
  ASSERT(nodeRef->owner == node);
  ASSERT(nodeRef->target == targetNode);
  ASSERT(ak_nodeRefTarget(nodeRef) == targetNode);
  ASSERT(nodeRef->prev == NULL);
  ASSERT(nodeRef->next == NULL);

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
  ASSERT(ak_nodeAttachNodeRef(root, nodeA) != NULL);
  ASSERT(ak_nodeAttachNodeRef(root, nodeB) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 11.0f);

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
  ASSERT(ak_nodeAttachNodeRef(root, ownerA) != NULL);
  ASSERT(ak_nodeAttachNodeRef(root, ownerB) != NULL);
  ASSERT(ak_nodeAttachNodeRef(ownerA, target) != NULL);
  ASSERT(ak_nodeAttachNodeRef(ownerB, target) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 11.0f);
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
  ASSERT(ak_nodeAttachNodeRef(root, camNode) != NULL);

  ASSERT(ak_firstCamera(doc, &found, matrix, NULL) == AK_OK);
  ASSERT(found == camera);
  ASSERT(fabsf(matrix[12] - 15.0f) < 0.001f);
  ASSERT(fabsf(matrix[13]) < 0.001f);
  ASSERT(fabsf(matrix[14]) < 0.001f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(scene_find_or_make_root_uses_node_refs) {
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
  ASSERT(scene->node->nodeRefs != NULL);
  ASSERT(scene->node->nodeRefs->target == madeRoot);
  ASSERT(scene->node->nodeRefs->next == NULL);
  ASSERT(scene->node->geometry == NULL);
  ASSERT(scene->node->chld == NULL);
  ASSERT(madeRoot->prev == NULL);
  ASSERT(madeRoot->next == NULL);
  ASSERT(doc->lib.nodes.count == 1);

  sameRoot = ak_sceneFindOrMakeRoot(doc, scene, "User Cameras");
  ASSERT(sameRoot == madeRoot);
  ASSERT(doc->lib.nodes.count == 1);
  ASSERT(scene->node->nodeRefs->next == NULL);

  otherRoot = ak_sceneFindOrMakeRoot(doc, scene, "Other Root");
  ASSERT(otherRoot != NULL);
  ASSERT(otherRoot != madeRoot);
  ASSERT(scene->node->nodeRefs->target == otherRoot);
  ASSERT(scene->node->nodeRefs->next != NULL);
  ASSERT(scene->node->nodeRefs->next->target == madeRoot);
  ASSERT(doc->lib.nodes.count == 2);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(dae_scene_roots_are_refs) {
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *rootA, *rootB;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  const char  *tmpBase;
  uint32_t     rootRefCount;
  AkNodeRef   *ref;

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
  ASSERT(scene->node->nodeRefs != NULL);
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
  ASSERT(rootA->next == NULL);
  ASSERT(rootB->next == NULL);
  ASSERT(rootA->prev == NULL);
  ASSERT(rootB->prev == NULL);

  rootRefCount = 0;
  for (ref = scene->node->nodeRefs; ref; ref = ref->next)
    rootRefCount++;
  ASSERT(rootRefCount == 2);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_instance_node_is_node_ref) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  AkNode     *shared;
  AkNodeRef  *ref;
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
           "%s/assetkit-dae-node-ref-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/node_ref.dae", tmpdir);
  ASSERT(ak_test_write_dae_instance_node(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->nodeRefs != NULL);
  ASSERT(doc->lib.nodes.count == 2);

  root = ak_sceneFindRoot(scene, "Root");
  ASSERT(root != NULL);
  ASSERT(root->nodeRefs != NULL);

  ref = root->nodeRefs;
  ASSERT(ref->owner == root);
  ASSERT(ref->target == NULL);
  ASSERT(ref->reserved != NULL);
  ASSERT(ref->proxy == NULL);

  shared = ak_nodeRefTarget(ref);
  ASSERT(shared != NULL);
  ASSERT(shared == ref->target);
  ASSERT(shared->name && strcmp(shared->name, "Shared") == 0);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(obj_scene_root_is_ref) {
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
  ASSERT(scene->node->nodeRefs != NULL);
  ASSERT(scene->node->geometry == NULL);

  root = ak_nodeRefTarget(scene->node->nodeRefs);
  ASSERT(root != NULL);
  ASSERT(root != scene->node);
  ASSERT(root->geometry != NULL);
  ASSERT(doc->lib.nodes.count == 1);

  ak_free(doc);
  unlink(objPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
