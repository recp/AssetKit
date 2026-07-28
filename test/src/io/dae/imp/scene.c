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

static char ak_test_dae_image_load_path[PATH_MAX];

static
AkImageData*
ak_test_dae_image_loader(AkHeap     * __restrict heap,
                         AkImage    * __restrict image,
                         const char * __restrict path,
                         bool                    flipVertically) {
  size_t pathLen;

  (void)heap;
  (void)image;
  (void)flipVertically;

  pathLen = path ? strlen(path) : 0;
  if (pathLen >= sizeof(ak_test_dae_image_load_path))
    pathLen = sizeof(ak_test_dae_image_load_path) - 1u;
  if (pathLen)
    memcpy(ak_test_dae_image_load_path, path, pathLen);
  ak_test_dae_image_load_path[pathLen] = '\0';

  return NULL;
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

TEST_IMPL(dae_skin_idref_joints_populate_default_joints) {
  AkDoc              *doc;
  AkSkin             *skin;
  AkNode             *meshNode;
  AkInstanceGeometry *geomInst;
  char                dirTemplate[PATH_MAX];
  char               *tmpdir;
  char                daePath[PATH_MAX];
  const char         *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-skin-joints-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "skin.dae"));
  ASSERT(ak_test_write_dae_skin_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.skins.count == 1);

  skin = doc->lib.skins.first;
  ASSERT(skin != NULL);
  ASSERT(skin->nJoints == 2);
  ASSERT(skin->joints != NULL);
  ASSERT(skin->joints[0] != NULL);
  ASSERT(skin->joints[1] != NULL);
  ASSERT(strcmp((const char *)ak_getId(skin->joints[0]), "joint0") == 0);
  ASSERT(strcmp((const char *)ak_getId(skin->joints[1]), "joint1") == 0);

  meshNode = ak_getObjectById(doc, "meshNode");
  ASSERT(meshNode != NULL);
  geomInst = meshNode->geometry;
  ASSERT(geomInst != NULL);
  ASSERT(geomInst->skinner != NULL);
  ASSERT(geomInst->skinner->overrideJoints != NULL);
  ASSERT(geomInst->skinner->overrideJoints[0] == skin->joints[0]);
  ASSERT(geomInst->skinner->overrideJoints[1] == skin->joints[1]);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_skin_multi_source_primitives_keep_weight_offsets) {
  AkDoc         *doc;
  AkSkin        *skin;
  AkBoneWeights *weights0;
  AkBoneWeights *weights1;
  char           dirTemplate[PATH_MAX];
  char          *tmpdir;
  char           daePath[PATH_MAX];
  const char    *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-skin-offsets-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "skin.dae"));
  ASSERT(ak_test_write_dae_skin_multi_source_primitives(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  ASSERT(doc->lib.skins.count == 1);
  skin = doc->lib.skins.first;
  ASSERT(skin != NULL);
  ASSERT(skin->nPrims == 2);
  ASSERT(skin->weights != NULL);

  weights0 = skin->weights[0];
  weights1 = skin->weights[1];
  ASSERT(weights0 != NULL);
  ASSERT(weights1 != NULL);
  ASSERT(weights0->nVertex == 3);
  ASSERT(weights1->nVertex == 3);
  ASSERT(weights0->counts[0] == 1);
  ASSERT(weights1->counts[0] == 1);
  ASSERT(weights0->weights[weights0->indexes[0]].joint == 0);
  ASSERT(weights1->weights[weights1->indexes[0]].joint == 1);
  ASSERT(weights0->weights[weights0->indexes[0]].weight == 1.0f);
  ASSERT(weights1->weights[weights1->indexes[0]].weight == 1.0f);

  ak_free(doc);
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

TEST_IMPL(dae14_broken_texture_ref_recovers_unique_image_and_path) {
  AkDoc           *doc;
  AkMaterial      *material;
  AkTextureRef    *textureRef;
  AkImage         *recoverImage;
  AkImage         *exactImage;
  AkImage         *image;
  char             dirTemplate[PATH_MAX];
  char            *tmpdir;
  char             daePath[PATH_MAX];
  char             recoveredTexturePath[PATH_MAX];
  char             exactTexturePath[PATH_MAX];
  char             exactCandidatePath[PATH_MAX];
  char             pathCandidateA[PATH_MAX];
  char             pathCandidateB[PATH_MAX];
  const char      *tmpBase;
  FILE            *file;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae14-broken-texture-ref-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/broken_texture_ref.dae", tmpdir);
  snprintf(recoveredTexturePath,
           sizeof(recoveredTexturePath),
           "%s/bbr_metal-2048.dds",
           tmpdir);
  snprintf(exactTexturePath, sizeof(exactTexturePath), "%s/exact.dds", tmpdir);
  snprintf(exactCandidatePath,
           sizeof(exactCandidatePath),
           "%s/bbr_wood-2048.dds",
           tmpdir);
  snprintf(pathCandidateA, sizeof(pathCandidateA), "%s/path_a.dds", tmpdir);
  snprintf(pathCandidateB, sizeof(pathCandidateB), "%s/path_b.dds", tmpdir);

  ASSERT(ak_test_write_dae14_broken_texture_refs(daePath));
  file = fopen(recoveredTexturePath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(exactCandidatePath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(pathCandidateA, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(pathCandidateB, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(exactTexturePath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  recoverImage = ak_getObjectById(doc, "bbr_metal-2048_dds");
  ASSERT(recoverImage != NULL);
  material = ak_getObjectById(doc, "mat_recover");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  ASSERT(textureRef->texture->image == recoverImage);
  ASSERT(recoverImage->source != NULL);
  ASSERT(strcmp(recoverImage->source->uri, "metal-2048.dds") == 0);
  ASSERT(recoverImage->source->resolvedPath != NULL);
  ASSERT(strcmp(recoverImage->source->resolvedPath, recoveredTexturePath) == 0);

  ak_test_dae_image_load_path[0] = '\0';
  ak_imageInitLoader(ak_test_dae_image_loader, NULL);
  ak_imageLoad(recoverImage);
  ak_imageInitLoader(NULL, NULL);
  ASSERT(strcmp(ak_test_dae_image_load_path, recoveredTexturePath) == 0);

  exactImage = ak_getObjectById(doc, "wood-2048_dds");
  ASSERT(exactImage != NULL);
  material = ak_getObjectById(doc, "mat_exact");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  ASSERT(textureRef->texture->image == exactImage);
  ASSERT(exactImage->source != NULL);
  ASSERT(exactImage->source->resolvedPath == NULL);
  ASSERT(strcmp(ak_imageResolvePath(exactImage), exactTexturePath) == 0);
  ASSERT(strcmp(exactImage->source->resolvedPath, exactTexturePath) == 0);

  material = ak_getObjectById(doc, "mat_ambiguous");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  image = textureRef->texture->image;
  ASSERT(image == NULL);

  image = ak_getObjectById(doc, "path_a_dds");
  ASSERT(image != NULL);
  material = ak_getObjectById(doc, "mat_path_ambiguous");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  ASSERT(textureRef->texture->image == image);
  ASSERT(image->source != NULL);
  ASSERT(image->source->resolvedPath == NULL);

  ak_free(doc);
  unlink(pathCandidateB);
  unlink(pathCandidateA);
  unlink(exactCandidatePath);
  unlink(exactTexturePath);
  unlink(recoveredTexturePath);
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
