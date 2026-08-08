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

#include "../../../test_export_common.h"

TEST_IMPL(gltf_export_triangle_smoke) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  struct stat stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_triangle_smoke");
  const float matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    2.0f, 3.0f, 4.0f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  node->name  = "Node \"A\"\\B\n";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stGltf.st_size > 0);
  ASSERT(stBin.st_size == (off_t)(sizeof(float) * 9));
  ASSERT(ak_test_file_contains(gltfPath, "\"min\":[0,0,0]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[1,1,0]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"target\":34962"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"name\":\"Node \\\"A\\\"\\\\B\\n\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\\\\\\\\B"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"translation\":[2,3,4]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_glb_triangle_smoke) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  struct stat stGlb, stBin;
  uint32_t    magic;
  uint32_t    version;
  uint32_t    length;
  uint32_t    jsonChunkType;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_triangle_smoke");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 0, &magic));
  ASSERT(ak_test_read_u32le(glbPath, 4, &version));
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(ak_test_read_u32le(glbPath, 16, &jsonChunkType));
  ASSERT(magic == 0x46546c67u);
  ASSERT(version == 2u);
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(jsonChunkType == 0x4e4f534au);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_version_option) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  uintptr_t   savedVersion;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_gltf_version");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  savedVersion = ak_opt_get(AK_OPT_GLTF_EXPORT_VERSION);
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, AK_GLTF_EXPORT_VERSION_2_1);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, savedVersion);

  ASSERT(ak_test_file_contains(gltfPath, "\"version\":\"2.1\""));

  ak_test_export_cleanup(outDir);
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, AK_GLTF_EXPORT_VERSION_AUTO);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"version\":\"2.0\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"version\":\"2.1\""));
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, savedVersion);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_21_nonsequential_attribute_sets) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  uintptr_t        savedVersion;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_gltf21_nonseq_attrs");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 3) != NULL);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  savedVersion = ak_opt_get(AK_OPT_GLTF_EXPORT_VERSION);
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, AK_GLTF_EXPORT_VERSION_2_0);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_0\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"TEXCOORD_3\""));

  ak_test_export_cleanup(outDir);
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, AK_GLTF_EXPORT_VERSION_2_1);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"version\":\"2.1\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_3\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"TEXCOORD_0\""));
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, savedVersion);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_zero_triangle_mode_defaults_to_list) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkTriangles     *tri;
  struct stat      stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_zero_triangle_mode");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  tri  = (AkTriangles *)mesh->primitive;
  tri->mode = 0;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stGltf.st_size > 0);
  ASSERT(stBin.st_size == (off_t)(sizeof(float) * 9));
  ASSERT(!ak_test_file_contains(gltfPath, "\"mode\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_numbers_are_locale_independent) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  char        oldLocale[128];
  const char *locale;
  const char *old;
  AkResult    result;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_locale_numbers");
  const float matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    1.25f, 2.5f, 3.75f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  old = setlocale(LC_NUMERIC, NULL);
  if (old) {
    strncpy(oldLocale, old, sizeof(oldLocale) - 1u);
    oldLocale[sizeof(oldLocale) - 1u] = '\0';
  } else {
    oldLocale[0] = '\0';
  }

  locale = setlocale(LC_NUMERIC, "de_DE.UTF-8");
  if (!locale)
    locale = setlocale(LC_NUMERIC, "fr_FR.UTF-8");
  if (!locale)
    locale = setlocale(LC_NUMERIC, "tr_TR.ISO8859-9");

  if (!locale) {
    ak_heap_destroy(heap);
    ak_test_export_cleanup(outDir);
    TEST_SUCCESS
  }

  result = ak_export(doc, outDir, AK_FILE_TYPE_GLTF);
  if (oldLocale[0])
    setlocale(LC_NUMERIC, oldLocale);
  else
    setlocale(LC_NUMERIC, "C");

  ASSERT(result == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"translation\":[1.25,2.5,3.75]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_empty_scene) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  struct stat  stGltf;
  const char *outDir   = "./assetkit_export_empty_scene";
  const char *gltfPath = "./assetkit_export_empty_scene/Empty.gltf";
  const char *binPath  = "./assetkit_export_empty_scene/Empty.bin";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene      = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->name = "Empty";
  doc->scene = scene;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stGltf) != 0);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"scenes\":[{\"name\":\"Empty\"}],\"scene\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"nodes\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"buffers\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_null_scene) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  struct stat  stGltf;
  const char  *outDir   = "./assetkit_export_null_scene";
  const char  *gltfPath = "./assetkit_export_null_scene/model.gltf";
  const char  *binPath  = "./assetkit_export_null_scene/model.bin";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stGltf) != 0);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\":[{}],\"scene\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"nodes\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"buffers\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_empty_scene_glb) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  struct stat  stGlb;
  struct stat  stBin;
  uint32_t     magic;
  uint32_t     version;
  uint32_t     length;
  uint32_t     jsonChunkType;
  const char  *outDir  = "./assetkit_export_empty_scene";
  const char  *glbPath = "./assetkit_export_empty_scene/Empty.glb";
  const char  *binPath = "./assetkit_export_empty_scene/Empty.bin";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->name = "Empty";
  doc->scene  = scene;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 0, &magic));
  ASSERT(ak_test_read_u32le(glbPath, 4, &version));
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(ak_test_read_u32le(glbPath, 16, &jsonChunkType));
  ASSERT(magic == 0x46546c67u);
  ASSERT(version == 2u);
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(jsonChunkType == 0x4e4f534au);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_scene_entrypoint_transform) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_scene_entrypoint_transform");
  const float rootMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 7.0f, 0.0f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);

  ak_nodeSetTransformMatrix(root, rootMatrix);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\":[{\"nodes\":[0]}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"children\":[1]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"translation\":[0,7,0]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_converts_document_units_to_metres) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_document_units_to_metres");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene         = ak_heap_calloc(heap, doc, sizeof(*scene));
  root          = ak_heap_calloc(heap, scene, sizeof(*root));
  node          = ak_heap_calloc(heap, doc, sizeof(*node));
  doc->unit     = ak_heap_calloc(heap, doc, sizeof(*doc->unit));
  ASSERT(doc->unit != NULL);
  doc->unit->name = "inch";
  doc->unit->dist = 0.0254;
  doc->coordSys   = AK_YUP;
  scene->node     = root;
  doc->scene      = scene;
  root->visible   = true;
  node->visible   = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
    "\"matrix\":[0.0254,0,0,0,0,0.0254,0,0,0,0,0.0254,0,0,0,0,1]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"scenes\":[{\"nodes\":[1]}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"children\":[0]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_nested_sibling_children_are_contiguous) {
  AkHeap  *heap;
  AkDoc   *doc;
  AkScene *scene;
  AkNode  *root, *parent, *childA, *grandChild, *childB;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_nested_sibling_children");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  parent      = ak_heap_calloc(heap, doc, sizeof(*parent));
  childA      = ak_heap_calloc(heap, doc, sizeof(*childA));
  grandChild  = ak_heap_calloc(heap, doc, sizeof(*grandChild));
  childB      = ak_heap_calloc(heap, doc, sizeof(*childB));
  scene->node = root;
  doc->scene  = scene;

  root->visible       = true;
  parent->visible     = true;
  childA->visible     = true;
  grandChild->visible = true;
  childB->visible     = true;

  ak_addSubNode(root, parent, false);
  ak_addSubNode(parent, childA, false);
  ak_addSubNode(childA, grandChild, false);
  ak_addSubNode(parent, childB, false);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\":[{\"nodes\":[0]}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"children\":[1,2]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"children\":[3]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_nonfinite_float) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stFile;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_nonfinite_float");
  const float matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    INFINITY, 0.0f, 0.0f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_nodeSetTransformMatrix(node, matrix);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stFile) != 0);
  ASSERT(stat(binPath, &stFile) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_auto_file_type) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_auto_type_nested/out");
  const char *parentDir = "./assetkit_export_auto_type_nested";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  rmdir(parentDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_AUTO) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stGltf.st_size > 0);
  ASSERT(stBin.st_size == (off_t)(sizeof(float) * 9));
  ASSERT(ak_test_file_contains(gltfPath, "\"asset\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"buffers\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  rmdir(parentDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_sanitizes_output_name) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stGltf, stBin;
  const char  *outDir   = "./assetkit_export_sanitized_name";
  const char  *gltfPath = "./assetkit_export_sanitized_name/Box_Name_.gltf";
  const char  *binPath  = "./assetkit_export_sanitized_name/Box_Name_.bin";
  const char  *modelPath = "./assetkit_export_sanitized_name/model.gltf";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf       = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name = "/tmp/Box:Name?.dae";

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stat(modelPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_blank_output_name_uses_model) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_blank_name");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf       = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name = "   .dae";

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_file_output_dir) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stFile;
  FILE        *file;
  const char  *outDir = "./assetkit_export_collision_file";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  unlink(outDir);
  file = fopen(outDir, "wb");
  ASSERT(file != NULL);
  ASSERT(fclose(file) == 0);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_EBADF);
  ASSERT(stat(outDir, &stFile) == 0);

  ak_heap_destroy(heap);
  unlink(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_out_of_bounds_accessor) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_accessor");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->pos->accessor->buffer->length = sizeof(float) * 3;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_accepts_component_count_without_component_size) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_component_count_type");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->pos->accessor->componentSize = AK_COMPONENT_SIZE_UNKNOWN;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"VEC3\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_invalid_attribute_accessor_shape) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  struct stat      stGltf;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_attribute_accessor_shape");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->pos->accessor->componentCount = 2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stGltf) != 0);
  ASSERT(stat(binPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_position_input_without_prim_pos) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_position_input_only");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(prim->pos != NULL);
  prim->pos = NULL;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"attributes\":{\"POSITION\":0}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_duplicate_attribute_semantic) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *normalA;
  AkInput         *normalB;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_duplicate_attribute");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float normals[9] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  normalA = ak_heap_calloc(heap, prim, sizeof(*normalA));
  normalB = ak_heap_calloc(heap, prim, sizeof(*normalB));
  normalA->semantic = AK_INPUT_NORMAL;
  normalB->semantic = AK_INPUT_NORMAL;
  normalA->accessor = ak_test_make_float_accessor(heap,
                                                  normalA,
                                                  normals,
                                                  3,
                                                  3);
  normalB->accessor = ak_test_make_float_accessor(heap,
                                                  normalB,
                                                  normals,
                                                  3,
                                                  3);
  ASSERT(normalA->accessor != NULL);
  ASSERT(normalB->accessor != NULL);

  normalB->next = prim->input;
  normalA->next = normalB;
  prim->input = normalA;
  prim->inputCount += 2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"NORMAL\""));
  ASSERT(ak_test_file_count(gltfPath, "\"NORMAL\"") == 1);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_normalizes_float_normals) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkGeometry      *roundGeom;
  AkMesh          *mesh;
  AkMesh          *roundMesh;
  AkMeshPrimitive *prim;
  AkMeshPrimitive *roundPrim;
  AkInput         *normal;
  AkInput         *roundInput;
  AkAccessor      *acc;
  uint32_t         i;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_normalized_normals");
  const char      *roundDir      = "./assetkit_export_normalized_normals_round";
  const char      *roundGltfPath = "./assetkit_export_normalized_normals_round/model.gltf";
  const char      *roundBinPath  = "./assetkit_export_normalized_normals_round/model.bin";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float normals[9] = {
    0.0f, 0.0f, 2.0f,
    0.0f, 3.0f, 0.0f,
    4.0f, 0.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  normal = ak_heap_calloc(heap, prim, sizeof(*normal));
  normal->semantic = AK_INPUT_NORMAL;
  normal->accessor = ak_test_make_float_accessor(heap, normal, normals, 3, 3);
  ASSERT(normal->accessor != NULL);
  normal->next = prim->input;
  prim->input = normal;
  prim->inputCount++;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"NORMAL\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  roundGeom = roundTrip->lib.geometries.first;
  ASSERT(roundGeom != NULL);
  roundMesh = ak_objGet(roundGeom->gdata);
  ASSERT(roundMesh != NULL);
  roundPrim = roundMesh->primitive;
  ASSERT(roundPrim != NULL);

  acc = NULL;
  for (roundInput = roundPrim->input; roundInput; roundInput = roundInput->next) {
    if (roundInput->semantic == AK_INPUT_NORMAL) {
      acc = roundInput->accessor;
      break;
    }
  }
  ASSERT(acc != NULL);
  ASSERT(acc->buffer != NULL);
  ASSERT(acc->buffer->data != NULL);
  ASSERT(acc->componentType == AKT_FLOAT);
  ASSERT(acc->componentCount == 3);

  for (i = 0; i < acc->count; i++) {
    const unsigned char *item;
    size_t               stride;
    float                x;
    float                y;
    float                z;
    float                len;

    stride = acc->byteStride ? acc->byteStride : acc->fillByteSize;
    item   = (const unsigned char *)acc->buffer->data
             + acc->byteOffset
             + (size_t)i * stride;
    memcpy(&x, item, sizeof(x));
    memcpy(&y, item + sizeof(float), sizeof(y));
    memcpy(&z, item + sizeof(float) * 2u, sizeof(z));
    len = sqrtf(x * x + y * y + z * z);
    ASSERT(fabsf(len - 1.0f) < 0.00001f);
  }

  ASSERT(ak_export(roundTrip, roundDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_files_equal(gltfPath, roundGltfPath));
  ASSERT(ak_test_files_equal(binPath, roundBinPath));

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_sanitizes_mixed_invalid_normals) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkGeometry      *roundGeom;
  AkMesh          *mesh;
  AkMesh          *roundMesh;
  AkMeshPrimitive *prim;
  AkMeshPrimitive *roundPrim;
  AkInput         *normal;
  AkInput         *roundInput;
  AkAccessor      *normalAcc;
  uint32_t         i;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_sanitized_normals");
  const float positions[21] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    2.0f, 0.0f, 0.0f,
    3.0f, 0.0f, 0.0f,
    2.0f, 1.0f, 0.0f,
    9.0f, 9.0f, 9.0f
  };
  const float normals[21] = {
     0.0f,  0.0f, -2.0f,
     0.0f, -3.0f,  0.0f,
    -4.0f,  0.0f,  0.0f,
     0.0f,  0.0f, -5.0f,
     0.0f,  0.0f, -6.0f,
     0.0f,  0.0f,  0.0f,
     NAN,   0.0f,  0.0f
  };
  const float expected[21] = {
     0.0f,  0.0f, -1.0f,
     0.0f, -1.0f,  0.0f,
    -1.0f,  0.0f,  0.0f,
     0.0f,  0.0f, -1.0f,
     0.0f,  0.0f, -1.0f,
     0.0f,  0.0f,  1.0f,
     0.0f,  0.0f,  1.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_geom_with_positions(heap, doc, positions, 7u);
  ASSERT(geom != NULL);
  mesh = ak_objGet(geom->gdata);
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);

  normal = ak_heap_calloc(heap, prim, sizeof(*normal));
  ASSERT(normal != NULL);
  normal->semantic = AK_INPUT_NORMAL;
  normal->accessor = ak_test_make_float_accessor(heap,
                                                  normal,
                                                  normals,
                                                  3u,
                                                  7u);
  ASSERT(normal->accessor != NULL);
  normal->next = prim->input;
  prim->input  = normal;
  prim->inputCount++;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"NORMAL\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  roundGeom = roundTrip->lib.geometries.first;
  ASSERT(roundGeom != NULL);
  roundMesh = ak_objGet(roundGeom->gdata);
  ASSERT(roundMesh != NULL);
  roundPrim = roundMesh->primitive;
  ASSERT(roundPrim != NULL);

  normalAcc = NULL;
  for (roundInput = roundPrim->input; roundInput; roundInput = roundInput->next) {
    if (roundInput->semantic == AK_INPUT_NORMAL) {
      normalAcc = roundInput->accessor;
      break;
    }
  }
  ASSERT(normalAcc != NULL);
  ASSERT(normalAcc->count == 7u);
  ASSERT(normalAcc->componentType == AKT_FLOAT);
  ASSERT(normalAcc->componentCount == 3u);

  for (i = 0; i < normalAcc->count; i++) {
    const unsigned char *item;
    size_t               stride;
    float                value[3];
    float                len2;
    uint32_t             c;

    stride = normalAcc->byteStride
             ? normalAcc->byteStride
             : normalAcc->fillByteSize;
    item = (const unsigned char *)normalAcc->buffer->data
           + normalAcc->byteOffset
           + (size_t)i * stride;
    memcpy(value, item, sizeof(value));
    len2 = value[0] * value[0]
           + value[1] * value[1]
           + value[2] * value[2];
    ASSERT(isfinite(len2));
    ASSERT(fabsf(len2 - 1.0f) < 0.00001f);
    for (c = 0; c < 3u; c++)
      ASSERT(fabsf(value[c] - expected[(size_t)i * 3u + c]) < 0.00001f);
  }

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(mesh_gen_normals_position_accessor_layout) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkAccessor      *posAcc;
  AkBuffer        *posBuff;
  AkInput         *input;
  AkAccessor      *normalAcc;
  uint8_t         *indices;
  unsigned char   layout[64];
  size_t          stride;
  uint32_t        i;
  bool            foundNormal;
  const float     positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float     v0[4] = {0.0f, 0.0f, 0.0f, 77.0f};
  const float     v1[4] = {1.0f, 0.0f, 0.0f, 88.0f};
  const float     v2[4] = {0.0f, 1.0f, 0.0f, 99.0f};

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(geom != NULL);

  mesh = ak_objGet(geom->gdata);
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);

  posAcc  = prim->pos->accessor;
  posBuff = posAcc->buffer;
  ASSERT(posBuff != NULL);

  memset(layout, 0x7f, sizeof(layout));
  memcpy(layout + 16u, v0, sizeof(v0));
  memcpy(layout + 32u, v1, sizeof(v1));
  memcpy(layout + 48u, v2, sizeof(v2));

  posBuff->length = sizeof(layout);
  posBuff->data   = ak_heap_alloc(heap, posBuff, sizeof(layout));
  ASSERT(posBuff->data != NULL);
  memcpy(posBuff->data, layout, sizeof(layout));

  posAcc->byteOffset    = 16;
  posAcc->byteStride    = sizeof(float) * 4u;
  posAcc->fillByteSize  = sizeof(float) * 3u;
  posAcc->byteLength    = posAcc->byteStride * 3u;
  posAcc->componentSize = AK_COMPONENT_SIZE_VEC3;
  posAcc->componentType = AKT_FLOAT;
  posAcc->componentCount = 3;
  posAcc->count         = 3;

  prim->indices = ak_indexArrayAlloc(heap, prim, 3, AKT_UBYTE);
  ASSERT(prim->indices != NULL);
  indices    = (uint8_t *)prim->indices->items;
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  prim->indices->max = 2;
  prim->indexStride  = 1;

  ak_meshGenNormals(mesh);

  foundNormal = false;
  normalAcc   = NULL;
  for (input = prim->input; input; input = input->next) {
    if (input->semantic == AK_INPUT_NORMAL) {
      foundNormal = true;
      normalAcc = input->accessor;
      break;
    }
  }

  ASSERT(foundNormal);
  ASSERT(normalAcc != NULL);
  ASSERT(normalAcc->buffer != NULL);
  ASSERT(normalAcc->buffer->data != NULL);
  ASSERT(normalAcc->componentType == AKT_FLOAT);
  ASSERT(normalAcc->componentCount == 3);
  ASSERT(normalAcc->count > 0);

  stride = normalAcc->byteStride ? normalAcc->byteStride : normalAcc->fillByteSize;
  for (i = 0; i < normalAcc->count; i++) {
    const unsigned char *item;
    float                x;
    float                y;
    float                z;

    item = (const unsigned char *)normalAcc->buffer->data
           + normalAcc->byteOffset
           + (size_t)i * stride;
    memcpy(&x, item, sizeof(x));
    memcpy(&y, item + sizeof(float), sizeof(y));
    memcpy(&z, item + sizeof(float) * 2u, sizeof(z));

    ASSERT(fabsf(x) < 0.00001f);
    ASSERT(fabsf(y) < 0.00001f);
    ASSERT(fabsf(z - 1.0f) < 0.00001f);
  }

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_unsupported_mesh_input_accessor) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *other;
  const double     extraValues[3] = {1.0, 2.0, 3.0};
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_skip_unsupported_input");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  other = ak_heap_calloc(heap, prim, sizeof(*other));
  other->semantic = AK_INPUT_OTHER;
  other->semanticRaw = "ROTATION";
  other->accessor = ak_heap_calloc(heap, other, sizeof(*other->accessor));
  other->accessor->buffer = ak_heap_calloc(heap,
                                           other->accessor,
                                           sizeof(*other->accessor->buffer));
  other->accessor->buffer->data = (void *)extraValues;
  other->accessor->buffer->length = sizeof(extraValues);
  other->accessor->byteLength = sizeof(extraValues);
  other->accessor->byteStride = sizeof(double);
  other->accessor->fillByteSize = sizeof(double);
  other->accessor->bytesPerComponent = sizeof(double);
  other->accessor->componentSize = AK_COMPONENT_SIZE_SCALAR;
  other->accessor->componentType = AKT_DOUBLE;
  other->accessor->originalComponentType = AKT_DOUBLE;
  other->accessor->componentCount = 1;
  other->accessor->count = 3;
  other->next = prim->input;
  prim->input = other;
  prim->inputCount++;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"attributes\":{\"POSITION\":0}"));
  ASSERT(!ak_test_file_contains(gltfPath, "ROTATION"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_native_index_accessor) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_native_index_accessor");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const uint8_t indices[3] = {0u, 1u, 2u};

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indexAccessor = ak_test_make_ubyte_accessor(heap,
                                                    prim,
                                                    indices,
                                                    1,
                                                    3);
  ASSERT(prim->indexAccessor != NULL);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"attributes\":{\"POSITION\":1}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"indices\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5121"));
  ASSERT(ak_test_file_contains(gltfPath, "\"target\":34963"));
  ASSERT(ak_test_file_contains(gltfPath, "\"target\":34962"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_promotes_primitive_restart_index_array) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  uint8_t         *items;
  float            positions[256 * 3];
  uint32_t         i;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_promote_restart_index");

  for (i = 0; i < 256; i++) {
    positions[(size_t)i * 3u + 0u] = (float)i;
    positions[(size_t)i * 3u + 1u] = 0.0f;
    positions[(size_t)i * 3u + 2u] = 0.0f;
  }

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_geom_with_positions(heap, doc, positions, 256);
  ASSERT(geom != NULL);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indices = ak_indexArrayAlloc(heap, prim, 3, AKT_UBYTE);
  ASSERT(prim->indices != NULL);

  items       = (uint8_t *)prim->indices->items;
  items[0]    = 0u;
  items[1]    = UINT8_MAX;
  items[2]    = 1u;
  prim->indices->max = UINT8_MAX;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"indices\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5123"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"componentType\":5121"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_invalid_index_accessor_type) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  struct stat      stGltf;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_index_accessor");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float badIndices[3] = {0.0f, 1.0f, 2.0f};

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indexAccessor = ak_test_make_float_accessor(heap,
                                                    prim,
                                                    badIndices,
                                                    1,
                                                    3);
  ASSERT(prim->indexAccessor != NULL);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stGltf) != 0);
  ASSERT(stat(binPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_invalid_index_accessor_shape) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  struct stat      stGltf;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_index_accessor_shape");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const uint8_t indices[3] = {0u, 1u, 2u};

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indexAccessor = ak_test_make_ubyte_accessor(heap,
                                                    prim,
                                                    indices,
                                                    1,
                                                    3);
  ASSERT(prim->indexAccessor != NULL);
  prim->indexAccessor->componentSize = AK_COMPONENT_SIZE_VEC2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stGltf) != 0);
  ASSERT(stat(binPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_shared_node_reuses_mesh) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *shared;
  AkNode         *roundRoot;
  AkGeometry     *geom;
  AkInstanceNode *useA, *useB;
  uint32_t        rootCount;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_shared_node");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  shared      = ak_heap_calloc(heap, doc, sizeof(*shared));
  scene->node = root;
  doc->scene  = scene;

  shared->name = "Shared";
  geom         = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(ak_nodeAttachGeometry(shared, geom) != NULL);

  useA = ak_nodeAttachNodeInstance(root, shared);
  useB = ak_nodeAttachNodeInstance(root, shared);
  ASSERT(useA != NULL);
  ASSERT(useB != NULL);
  useA->name = "UseA";
  useB->name = "UseB";

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"meshes\":["));
  ASSERT(ak_test_file_count(gltfPath, "\"mesh\":0") == 2);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);

  rootCount = 0;
  for (roundRoot = roundTrip->scene->node->chld;
       roundRoot;
       roundRoot = roundRoot->next)
    rootCount++;
  ASSERT(rootCount == 2);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}
