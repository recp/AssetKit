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
ak_test_make_ply_triangle_doc(bool withAttributes) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  AkMesh     *mesh;
  AkMeshPrimitive *prim;
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
  node->name  = "PLY Node";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  if (!geom)
    return NULL;
  mesh = ak_objGet(geom->gdata);
  prim = mesh ? mesh->primitive : NULL;
  if (!prim)
    return NULL;

  if (withAttributes) {
    AkInput *colorInput;
    const uint8_t colors[12] = {
      255u, 0u,   0u,   255u,
      0u,   128u, 0u,   255u,
      0u,   0u,   255u, 255u
    };

    if (!ak_test_add_texcoord_input(heap, prim, 0))
      return NULL;
    colorInput = ak_heap_calloc(heap, prim, sizeof(*colorInput));
    if (!colorInput)
      return NULL;
    colorInput->semantic = AK_INPUT_COLOR;
    colorInput->set      = 0;
    colorInput->index    = 0;
    colorInput->accessor = ak_test_make_ubyte_accessor(heap,
                                                       colorInput,
                                                       colors,
                                                       4,
                                                       3);
    if (!colorInput->accessor)
      return NULL;
    colorInput->next     = prim->input;
    prim->input          = colorInput;
    prim->inputCount++;
  }

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  if (!ak_nodeAttachGeometry(node, geom))
    return NULL;

  return doc;
}

TEST_IMPL(ply_export_binary_triangle_smoke) {
  AkDoc     *doc;
  AkDoc     *roundTrip;
  uintptr_t  savedFormat;
  uintptr_t  savedUV;
  uintptr_t  savedColorMode;
  struct stat st;
  const char *outDir  = "./assetkit_export_ply_binary_triangle_smoke";
  const char *plyPath = "./assetkit_export_ply_binary_triangle_smoke/model.ply";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_ply_triangle_doc(false);
  ASSERT(doc != NULL);

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_BINARY_LITTLE);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_NONE);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);

  ASSERT(stat(plyPath, &st) == 0);
  ASSERT(st.st_size > 0);
  ASSERT(ak_test_file_contains(plyPath, "format binary_little_endian 1.0"));
  ASSERT(ak_test_file_contains(plyPath, "element vertex 3"));
  ASSERT(ak_test_file_contains(plyPath, "element face 1"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, plyPath, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(ply_export_ascii_attributes_smoke) {
  AkDoc     *doc;
  AkDoc     *roundTrip;
  uintptr_t  savedFormat;
  uintptr_t  savedUV;
  uintptr_t  savedColorMode;
  const char *outDir  = "./assetkit_export_ply_ascii_attributes_smoke";
  const char *plyPath = "./assetkit_export_ply_ascii_attributes_smoke/model.ply";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_ply_triangle_doc(true);
  ASSERT(doc != NULL);

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, true);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_LINEAR);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);

  ASSERT(ak_test_file_contains(plyPath, "format ascii 1.0"));
  ASSERT(ak_test_file_contains(plyPath, "property float s"));
  ASSERT(ak_test_file_contains(plyPath, "property uchar red"));
  ASSERT(ak_test_file_contains(plyPath, "property uchar alpha"));
  ASSERT(ak_test_file_contains(plyPath, "2 3 4 0 0 255 0 0 255"));
  ASSERT(ak_test_file_contains(plyPath, "3 0 1 2"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, plyPath, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}
