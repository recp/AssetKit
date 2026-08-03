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

TEST_IMPL(dae_export_triangle_smoke) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node, *roundNode;
  AkGeometry *geom;
  AkMatrix    roundMatrix;
  struct stat stDae;
  const char *outDir  = "./assetkit_export_dae_triangle_smoke";
  const char *daePath = "./assetkit_export_dae_triangle_smoke/model.dae";
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
  node->name  = "DAE Node";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(
    daePath,
    "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\""));
  ASSERT(ak_test_file_contains(daePath, "<library_geometries>"));
  ASSERT(ak_test_file_contains(daePath, "<triangles count=\"1\">"));
  ASSERT(ak_test_file_contains(daePath, "<instance_geometry url=\"#geom_0\">"));
  ASSERT(ak_test_file_contains(daePath,
                               "<matrix sid=\"matrix\">"
                               "1 0 0 2 0 1 0 3 0 0 1 4 0 0 0 1"
                               "</matrix>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);
  roundNode = roundTrip->scene->node->chld;
  ASSERT(roundNode->transform != NULL);
  ak_transformCombine(roundNode->transform, roundMatrix.val[0]);
  ASSERT(roundMatrix.val[3][0] == 2.0f);
  ASSERT(roundMatrix.val[3][1] == 3.0f);
  ASSERT(roundMatrix.val[3][2] == 4.0f);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_index_mode_option) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *tex;
  uint8_t         *items;
  uintptr_t        oldMode;
  bool             multiOk, autoOk, singleOk;
  const char      *outDir  = "./assetkit_export_dae_index_mode";
  const char      *daePath = "./assetkit_export_dae_index_mode/model.dae";
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
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  mesh = ak_objGet(geom->gdata);
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  tex = ak_test_add_texcoord_input(heap, prim, 0);
  ASSERT(tex != NULL);

  prim->pos->semanticRaw = "POSITION";
  tex->semanticRaw       = "TEXCOORD";

  prim->indices = ak_indexArrayAlloc(heap, prim, 6, AKT_UBYTE);
  ASSERT(prim->indices != NULL);
  items    = prim->indices->items;
  items[0] = 0u;
  items[1] = 0u;
  items[2] = 1u;
  items[3] = 1u;
  items[4] = 2u;
  items[5] = 2u;
  prim->indices->max = 2;
  prim->indexStride  = 2;
  prim->pos->isIndexed = true;
  prim->pos->indexOffset = 0;
  tex->isIndexed = true;
  tex->indexOffset = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  oldMode = ak_opt_get(AK_OPT_DAE_EXPORT_INDEX_MODE);

  ak_opt_set(AK_OPT_DAE_EXPORT_INDEX_MODE, AK_DAE_EXPORT_INDEX_MULTI);
  multiOk = ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK
            && ak_test_file_contains(daePath,
                                     "<input semantic=\"VERTEX\" source=\"#geom_0_prim_0_vertices\" offset=\"0\"/>")
            && ak_test_file_contains(daePath,
                                     "<input semantic=\"TEXCOORD\" source=\"#geom_0_prim_0_TEXCOORD_0\" offset=\"1\" set=\"0\"/>")
            && ak_test_file_contains(daePath,
                                     "<param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>")
            && ak_test_file_contains(daePath, "<p>0 0 1 1 2 2</p>");

  ak_test_export_cleanup(outDir);
  ak_opt_set(AK_OPT_DAE_EXPORT_INDEX_MODE, AK_DAE_EXPORT_INDEX_AUTO);
  autoOk = ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK
           && ak_test_file_contains(daePath, "<p>0 0 1 1 2 2</p>");

  ak_test_export_cleanup(outDir);
  ak_opt_set(AK_OPT_DAE_EXPORT_INDEX_MODE, AK_DAE_EXPORT_INDEX_SINGLE);
  singleOk = ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK
             && ak_test_file_contains(daePath,
                                      "<vertices id=\"geom_0_prim_0_vertices\"><input semantic=\"POSITION\"")
             && ak_test_file_contains(daePath,
                                      "<input semantic=\"TEXCOORD\" source=\"#geom_0_prim_0_TEXCOORD_0\" offset=\"0\" set=\"0\"/>")
             && ak_test_file_contains(daePath,
                                      "<input semantic=\"VERTEX\" source=\"#geom_0_prim_0_vertices\" offset=\"0\"/>")
             && ak_test_file_contains(daePath, "<p>0 1 2</p>");

  ak_opt_set(AK_OPT_DAE_EXPORT_INDEX_MODE, oldMode);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  ASSERT(multiOk);
  ASSERT(autoOk);
  ASSERT(singleOk);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_version_option_texture_syntax) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         texPath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;
  FILE        *file;
  uintptr_t    oldVersion;
  AkResult     exportResult;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-export-version-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/texture.dae", tmpdir);
  snprintf(texPath, sizeof(texPath), "%s/duckCM.tga", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/texture.dae", outDir);

  ASSERT(ak_test_write_dae14_missing_surface_texture(daePath));
  file = fopen(texPath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("TGADATA", file) >= 0);
  ASSERT(fclose(file) == 0);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  oldVersion = ak_opt_get(AK_OPT_DAE_EXPORT_VERSION);
  ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, AK_DAE_EXPORT_VERSION_1_5_1);
  exportResult = ak_export(doc, outDir, AK_FILE_TYPE_DAE);
  ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, oldVersion);

  ASSERT(exportResult == AK_OK);
  ASSERT(ak_test_file_contains(
    outDae,
    "<COLLADA xmlns=\"http://www.collada.org/2008/03/COLLADASchema\" version=\"1.5.0\""));
  ASSERT(ak_test_file_contains(outDae, "<init_from><ref>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<sampler2D><instance_image url=\"#image_0\"/>"));
  ASSERT(!ak_test_file_contains(outDae, "<source>surface_0</source>"));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  snprintf(texPath, sizeof(texPath), "%s/duckCM.tga", outDir);
  unlink(texPath);
  snprintf(texPath, sizeof(texPath), "%s/image_0_duckCM.tga", outDir);
  unlink(texPath);
  rmdir(outDir);
  snprintf(texPath, sizeof(texPath), "%s/duckCM.tga", tmpdir);
  unlink(texPath);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_version_14_rejects_brep) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  const char  *tmpBase;
  uintptr_t    oldVersion;
  AkResult     exportResult;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-export-version-brep-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/brep.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);

  ASSERT(ak_test_write_dae_brep_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  oldVersion = ak_opt_get(AK_OPT_DAE_EXPORT_VERSION);
  ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, AK_DAE_EXPORT_VERSION_1_4);
  exportResult = ak_export(doc, outDir, AK_FILE_TYPE_DAE);
  ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, oldVersion);

  ASSERT(exportResult == AK_EINVAL);

  ak_free(doc);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_native_strip_modes) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkGeometry  *stripGeom;
  AkGeometry  *fanGeom;
  AkGeometry  *lineStripGeom;
  AkGeometry  *lineLoopGeom;
  AkMesh      *mesh;
  const char  *outDir  = "./assetkit_export_dae_native_strip_modes";
  const char  *daePath = "./assetkit_export_dae_native_strip_modes/model.dae";
  const float  positions[12] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  stripGeom     = ak_test_make_geom_with_positions(heap, doc, positions, 4);
  fanGeom       = ak_test_make_geom_with_positions(heap, doc, positions, 4);
  lineStripGeom = ak_test_make_line_geom_with_positions(heap,
                                                        doc,
                                                        positions,
                                                        4,
                                                        AK_LINE_STRIP);
  lineLoopGeom  = ak_test_make_line_geom_with_positions(heap,
                                                        doc,
                                                        positions,
                                                        4,
                                                        AK_LINE_LOOP);
  ASSERT(stripGeom != NULL);
  ASSERT(fanGeom != NULL);
  ASSERT(lineStripGeom != NULL);
  ASSERT(lineLoopGeom != NULL);

  mesh = ak_objGet(stripGeom->gdata);
  ((AkTriangles *)mesh->primitive)->mode = AK_TRIANGLE_STRIP;
  mesh = ak_objGet(fanGeom->gdata);
  ((AkTriangles *)mesh->primitive)->mode = AK_TRIANGLE_FAN;

  stripGeom->next = fanGeom;
  fanGeom->next   = lineStripGeom;
  lineStripGeom->next = lineLoopGeom;
  doc->lib.geometries.first = stripGeom;
  doc->lib.geometries.last  = lineLoopGeom;
  doc->lib.geometries.count = 4;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<tristrips count=\"1\">"));
  ASSERT(ak_test_file_contains(daePath, "</tristrips>"));
  ASSERT(ak_test_file_contains(daePath, "<trifans count=\"1\">"));
  ASSERT(ak_test_file_contains(daePath, "</trifans>"));
  ASSERT(ak_test_file_contains(daePath, "<linestrips count=\"1\">"));
  ASSERT(ak_test_file_contains(daePath, "</linestrips>"));
  ASSERT(ak_test_file_contains(daePath, "<lines count=\"4\">"));
  ASSERT(ak_test_file_contains(daePath, "<p>0 1 1 2 2 3 3 0</p>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 4);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_skips_unsupported_nonposition_input) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkGeometry    *geom;
  AkMesh        *mesh;
  AkMeshPrimitive *prim;
  AkInput       *customInput;
  AkAccessor    *customAccessor;
  const char    *outDir  = "./assetkit_export_dae_skip_custom_input";
  const char    *daePath = "./assetkit_export_dae_skip_custom_input/model.dae";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  customInput    = ak_heap_calloc(heap, prim, sizeof(*customInput));
  customAccessor = ak_heap_calloc(heap, customInput, sizeof(*customAccessor));
  ASSERT(customInput != NULL);
  ASSERT(customAccessor != NULL);

  customAccessor->count = 3;
  customInput->semantic = 0;
  customInput->semanticRaw = "CUSTOM";
  customInput->accessor = customAccessor;
  customInput->next     = prim->input;
  prim->input           = customInput;
  prim->inputCount++;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<triangles count=\"1\">"));
  ASSERT(!ak_test_file_contains(daePath, "semantic=\"CUSTOM\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_imported_gltf_triangle_smoke) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *imported;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  const char *gltfOutDir = "./assetkit_export_dae_from_gltf_src";
  const char *daeOutDir  = "./assetkit_export_dae_from_gltf";
  const char *gltfPath   = "./assetkit_export_dae_from_gltf_src/model.gltf";
  const char *daePath    = "./assetkit_export_dae_from_gltf/model.dae";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(gltfOutDir);
  ak_test_export_cleanup(daeOutDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  node->name  = "Imported Root";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, gltfOutDir, AK_FILE_TYPE_GLTF) == AK_OK);
  imported = NULL;
  ASSERT(ak_load(&imported, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(imported != NULL);
  ASSERT(ak_export(imported, daeOutDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<library_geometries>"));
  ASSERT(ak_test_file_contains(daePath, "<instance_geometry url=\"#geom_0\">"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);

  ak_free(roundTrip);
  ak_free(imported);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(gltfOutDir);
  ak_test_export_cleanup(daeOutDir);

  TEST_SUCCESS
}
