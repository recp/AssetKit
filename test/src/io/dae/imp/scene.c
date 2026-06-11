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

TEST_IMPL(dae_same_file_external_refs_are_internal) {
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *geoNode;
  AkNode             *camNode;
  AkNode             *lightNode;
  AkInstanceGeometry *geomInst;
  char                dirTemplate[PATH_MAX];
  char               *tmpdir;
  char                daePath[PATH_MAX];
  char                outDir[PATH_MAX];
  char                outDaePath[PATH_MAX];
  const char         *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-same-file-refs-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/self.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDaePath, sizeof(outDaePath), "%s/self.dae", outDir);

  ASSERT(ak_test_write_dae_same_file_refs(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);

  geoNode   = ak_sceneFindRoot(scene, "GeoNode");
  camNode   = ak_sceneFindRoot(scene, "CamNode");
  lightNode = ak_sceneFindRoot(scene, "LightNode");
  ASSERT(geoNode != NULL && camNode != NULL && lightNode != NULL);
  ASSERT(geoNode->geometry != NULL);
  ASSERT(camNode->camera != NULL);
  ASSERT(lightNode->light != NULL);

  geomInst = geoNode->geometry;
  ASSERT(ak_instanceObject(&geomInst->base) == doc->lib.geometries.first);
  ASSERT(ak_instanceObject(camNode->camera) == doc->lib.cameras.first);
  ASSERT(ak_instanceObject(lightNode->light) == doc->lib.lights.first);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDaePath, "<library_geometries>"));
  ASSERT(ak_test_file_contains(outDaePath, "<instance_geometry url=\"#geom_0\">"));
  ASSERT(ak_test_file_contains(outDaePath, "<instance_camera url=\"#camera_0\"/>"));
  ASSERT(ak_test_file_contains(outDaePath, "<instance_light url=\"#light_0\"/>"));

  ak_free(doc);
  ak_test_export_cleanup(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_load_utf16le) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         gltfPath[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-utf16le-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/utf16.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(gltfPath, sizeof(gltfPath), "%s/utf16.gltf", outDir);

  ASSERT(ak_test_write_dae_utf16le_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->scene != NULL);
  ASSERT(doc->scene->node != NULL);
  ASSERT(doc->scene->node->chld != NULL);
  ASSERT(doc->scene->node->chld->name
         && strcmp(doc->scene->node->chld->name, "Root") == 0);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\""));

  ak_free(doc);
  unlink(gltfPath);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae14_missing_surface_sampler_resolves_image) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         texPath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;
  FILE        *file;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae14-sampler-surface-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/missing_surface.dae", tmpdir);
  snprintf(texPath, sizeof(texPath), "%s/duckCM.tga", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/missing_surface.dae", outDir);

  ASSERT(ak_test_write_dae14_missing_surface_texture(daePath));
  file = fopen(texPath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("TGADATA", file) >= 0);
  ASSERT(fclose(file) == 0);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "duckCM.tga"));
  ASSERT(ak_test_file_contains(outDae, "<texture texture=\"sampler_0\""));

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

TEST_IMPL(dae14_nested_init_from_ref_image) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         texDir[PATH_MAX];
  char         texPath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outTexDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;
  FILE        *file;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae14-image-ref-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/nested_ref.dae", tmpdir);
  snprintf(texDir, sizeof(texDir), "%s/Textures", tmpdir);
  snprintf(texPath, sizeof(texPath), "%s/WoodFloor-01.png", texDir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outTexDir, sizeof(outTexDir), "%s/Textures", outDir);
  snprintf(outDae, sizeof(outDae), "%s/nested_ref.dae", outDir);

  ASSERT(mkdir(texDir, 0777) == 0);
  ASSERT(ak_test_write_dae14_nested_ref_image(daePath));
  file = fopen(texPath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("PNGDATA", file) >= 0);
  ASSERT(fclose(file) == 0);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "WoodFloor-01.png"));

  ak_free(doc);
  unlink(outDae);
  snprintf(texPath, sizeof(texPath), "%s/WoodFloor-01.png", outTexDir);
  unlink(texPath);
  rmdir(outTexDir);
  snprintf(texPath, sizeof(texPath), "%s/image_0_WoodFloor-01.png", outDir);
  unlink(texPath);
  rmdir(outDir);
  snprintf(texPath, sizeof(texPath), "%s/WoodFloor-01.png", texDir);
  unlink(texPath);
  rmdir(texDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
