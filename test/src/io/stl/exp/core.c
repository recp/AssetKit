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

static AkDoc *
ak_test_make_stl_triangle_doc(void) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
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

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  if (!heap || !doc)
    return NULL;
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  if (!scene || !root || !node)
    return NULL;
  node->name  = "STL Node";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  if (!geom)
    return NULL;
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  if (!ak_nodeAttachGeometry(node, geom))
    return NULL;

  return doc;
}

TEST_IMPL(stl_export_binary_triangle_smoke) {
  AkDoc     *doc;
  uintptr_t  savedFormat;
  uint32_t   triCount;
  struct stat st;
  const char *outDir  = "./assetkit_export_stl_binary_triangle_smoke";
  const char *stlPath = "./assetkit_export_stl_binary_triangle_smoke/model.stl";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_stl_triangle_doc();
  ASSERT(doc != NULL);

  savedFormat = ak_opt_get(AK_OPT_STL_EXPORT_FORMAT);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, AK_STL_EXPORT_BINARY);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_STL) == AK_OK);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, savedFormat);

  ASSERT(stat(stlPath, &st) == 0);
  ASSERT(st.st_size == 84 + 50);
  ASSERT(ak_test_read_u32le(stlPath, 80, &triCount));
  ASSERT(triCount == 1);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(stl_export_ascii_triangle_smoke) {
  AkDoc     *doc;
  uintptr_t  savedFormat;
  struct stat st;
  const char *outDir  = "./assetkit_export_stl_ascii_triangle_smoke";
  const char *stlPath = "./assetkit_export_stl_ascii_triangle_smoke/model.stl";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_stl_triangle_doc();
  ASSERT(doc != NULL);

  savedFormat = ak_opt_get(AK_OPT_STL_EXPORT_FORMAT);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, AK_STL_EXPORT_ASCII);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_STL) == AK_OK);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, savedFormat);

  ASSERT(stat(stlPath, &st) == 0);
  ASSERT(st.st_size > 0);
  ASSERT(ak_test_file_contains(stlPath, "solid assetkit"));
  ASSERT(ak_test_file_contains(stlPath, "facet normal"));
  ASSERT(ak_test_file_contains(stlPath, "vertex 2 3 4"));
  ASSERT(ak_test_file_contains(stlPath, "endsolid assetkit"));

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(stl_export_unlabelled_space_is_yup_metres) {
  AkDoc      *doc;
  AkUnit      unit;
  uintptr_t   savedFormat;
  const char *outDir  = "./assetkit_export_stl_canonical_space";
  const char *stlPath = "./assetkit_export_stl_canonical_space/model.stl";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_stl_triangle_doc();
  ASSERT(doc != NULL);

  memset(&unit, 0, sizeof(unit));
  unit.name     = "inch";
  unit.dist     = 0.0254;
  doc->unit     = &unit;
  doc->coordSys = AK_ZUP;

  savedFormat = ak_opt_get(AK_OPT_STL_EXPORT_FORMAT);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, AK_STL_EXPORT_ASCII);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_STL) == AK_OK);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, savedFormat);

  ASSERT(ak_test_file_contains(stlPath,
                               "vertex 0.0508 0.1016 -0.0762"));

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}
