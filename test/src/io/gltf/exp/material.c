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

TEST_IMPL(gltf_export_primitive_material) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkMaterialInput   *opacity;
  AkMaterialInput   *metallic;
  AkMaterialInput   *roughness;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_primitive_material");
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  opacity   = ak_heap_calloc(heap, surface, sizeof(*opacity));
  metallic  = ak_heap_calloc(heap, surface, sizeof(*metallic));
  roughness = ak_heap_calloc(heap, surface, sizeof(*roughness));

  mat->name        = "mat_red";
  mat->surface     = surface;
  surface->type    = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->flags   = AK_MATERIAL_FLAG_ALPHA_BLEND
                     | AK_MATERIAL_FLAG_DOUBLE_SIDED;
  surface->baseColor = baseColor;
  surface->opacity   = opacity;
  surface->metallic  = metallic;
  surface->roughness = roughness;

  baseColor->source      = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType   = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.25f;
  baseColor->color.rgba.G = 0.5f;
  baseColor->color.rgba.B = 0.75f;
  baseColor->color.rgba.A = 1.0f;

  opacity->source    = AK_MATERIAL_INPUT_CONSTANT;
  opacity->valueType = AK_MATERIAL_VALUE_FLOAT;
  opacity->value[0]  = 0.8f;

  metallic->source    = AK_MATERIAL_INPUT_CONSTANT;
  metallic->valueType = AK_MATERIAL_VALUE_FLOAT;
  metallic->value[0]  = 0.0f;

  roughness->source    = AK_MATERIAL_INPUT_CONSTANT;
  roughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  roughness->value[0]  = 0.5f;

  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"materials\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"material\":0"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorFactor\":[0.25,0.5,0.75,0.800000012]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"metallicFactor\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"roughnessFactor\":0.5"));
  ASSERT(ak_test_file_contains(gltfPath, "\"alphaMode\":\"BLEND\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"doubleSided\":true"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_extras) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTreeNode        *extra;
  AkTreeNode        *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_extras");
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

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  extra     = ak_heap_calloc(heap, mat, sizeof(*extra));
  note      = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  baseColor->source  = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "materialNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(mat, extra);

  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"materialNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_mesh_primitive_extras) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkTreeNode      *meshExtra;
  AkTreeNode      *meshNote;
  AkTreeNode      *primExtra;
  AkTreeNode      *primNote;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_mesh_primitive_extras");
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

  meshExtra = ak_heap_calloc(heap, doc, sizeof(*meshExtra));
  meshNote  = ak_heap_calloc(heap, meshExtra, sizeof(*meshNote));
  primExtra = ak_heap_calloc(heap, doc, sizeof(*primExtra));
  primNote  = ak_heap_calloc(heap, primExtra, sizeof(*primNote));
  ASSERT(meshExtra != NULL);
  ASSERT(meshNote != NULL);
  ASSERT(primExtra != NULL);
  ASSERT(primNote != NULL);

  meshExtra->name  = "extras";
  meshExtra->chld  = meshNote;
  meshExtra->chldc = 1;
  meshNote->name   = "meshNote";
  meshNote->val    = (char *)"roundtrip";
  meshNote->parent = meshExtra;
  mesh->extra = meshExtra;

  primExtra->name  = "extras";
  primExtra->chld  = primNote;
  primExtra->chldc = 1;
  primNote->name   = "primitiveNote";
  primNote->val    = (char *)"roundtrip";
  primNote->parent = primExtra;
  prim->extra = primExtra;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"meshNote\":\"roundtrip\"}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"primitiveNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_omits_imported_default_material) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         gltfPath[PATH_MAX];
  char         binPath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outGltfPath[PATH_MAX];
  char         outBinPath[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-materialless-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/materialless.gltf", tmpdir);
  snprintf(binPath, sizeof(binPath), "%s/tri.bin", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outGltfPath, sizeof(outGltfPath), "%s/materialless.gltf", outDir);
  snprintf(outBinPath, sizeof(outBinPath), "%s/materialless.bin", outDir);

  ASSERT(ak_test_write_gltf_materialless_triangle(gltfPath,
                                                  "tri.bin",
                                                  binPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(outGltfPath, "\"meshes\""));
  ASSERT(!ak_test_file_contains(outGltfPath, "\"materials\""));
  ASSERT(!ak_test_file_contains(outGltfPath, "\"material\":"));

  ak_free(doc);
  unlink(outGltfPath);
  unlink(outBinPath);
  rmdir(outDir);
  unlink(gltfPath);
  unlink(binPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_uri) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_uri");
  const char *sourceDir = "./assetkit_export_material_texture_uri_src";
  const char *sourceTexDir = "./assetkit_export_material_texture_uri_src/textures";
  const char *sourceTexPath = "./assetkit_export_material_texture_uri_src/textures/Wood File.PNG";
  const char *copiedTexPath = "./assetkit_export_material_texture_uri/textures/Wood File.PNG";
  const char *glbOutDir = "./assetkit_export_material_texture_uri_glb";
  const char *glbPath = "./assetkit_export_material_texture_uri_glb/model.glb";
  const char *glbCopiedTexPath = "./assetkit_export_material_texture_uri_glb/textures/Wood File.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(glbOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceDir;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler   = ak_heap_calloc(heap, doc, sizeof(*sampler));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "textures/Wood File.PNG";
  image->name  = "wood";
  image->source = source;

  sampler->wrapS     = AK_WRAP_MODE_CLAMP;
  sampler->wrapT     = AK_WRAP_MODE_WRAP;
  sampler->minfilter = AK_MINFILTER_NEAREST;
  sampler->magfilter = AK_MAGFILTER_NEAREST;

  texture->image   = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 1;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface        = surface;
  surface->type       = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor  = baseColor;
  prim->material      = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"samplers\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"wrapS\":33071"));
  ASSERT(ak_test_file_contains(gltfPath, "\"minFilter\":9728"));
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"textures/Wood%20File.PNG\""));
  ASSERT(!ak_test_file_contains(gltfPath, "Wood%2520File"));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));
  ASSERT(ak_test_file_contains(gltfPath, "\"textures\":["));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"texCoord\":1}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ASSERT(ak_export(doc, glbOutDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(ak_test_file_contains(glbPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(glbPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(glbPath, "\"uri\":\"textures/Wood%20File.PNG\""));
  ASSERT(!ak_test_file_contains(glbCopiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(glbOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_metallic_roughness_texture_channels) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *metallic;
  AkMaterialInput   *roughness;
  AkTextureRef      *metalRef;
  AkTextureRef      *roughRef;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  const char        *compatibleOutDir = "./assetkit_export_mr_texture_channels";
  const char        *compatibleGltfPath = "./assetkit_export_mr_texture_channels/model.gltf";
  const char        *incompatibleOutDir = "./assetkit_export_mr_texture_channels_incompatible";
  const char        *incompatibleGltfPath = "./assetkit_export_mr_texture_channels_incompatible/model.gltf";
  const char        *sourceDir = "./assetkit_export_mr_texture_channels_src";
  const char        *sourceTexDir = "./assetkit_export_mr_texture_channels_src/textures";
  const char        *sourceTexPath = "./assetkit_export_mr_texture_channels_src/textures/MR.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(compatibleOutDir);
  ak_test_export_cleanup(incompatibleOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceDir;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  metallic  = ak_heap_calloc(heap, surface, sizeof(*metallic));
  roughness = ak_heap_calloc(heap, surface, sizeof(*roughness));
  metalRef  = ak_heap_calloc(heap, metallic, sizeof(*metalRef));
  roughRef  = ak_heap_calloc(heap, roughness, sizeof(*roughRef));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "textures/MR.PNG";
  image->source = source;
  texture->image = image;

  metalRef->texture  = texture;
  metalRef->channels = AK_TEXTURE_CHANNEL_GB;
  roughRef->texture  = texture;
  roughRef->channels = AK_TEXTURE_CHANNEL_GB;

  metallic->source    = AK_MATERIAL_INPUT_TEXTURE;
  metallic->valueType = AK_MATERIAL_VALUE_FLOAT;
  metallic->value[0]  = 0.25f;
  metallic->channels  = AK_TEXTURE_CHANNEL_B;
  metallic->texture   = metalRef;

  roughness->source    = AK_MATERIAL_INPUT_TEXTURE;
  roughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  roughness->value[0]  = 0.75f;
  roughness->channels  = AK_TEXTURE_CHANNEL_G;
  roughness->texture   = roughRef;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->metallic  = metallic;
  surface->roughness = roughness;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, compatibleOutDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(compatibleGltfPath,
                               "\"metallicRoughnessTexture\":{\"index\":0}"));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"metallicFactor\":0.25"));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"roughnessFactor\":0.75"));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"textures\":["));

  roughRef->channels = AK_TEXTURE_CHANNEL_R;
  ASSERT(ak_export(doc, incompatibleOutDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(incompatibleGltfPath,
                                "\"metallicRoughnessTexture\""));
  ASSERT(ak_test_file_contains(incompatibleGltfPath, "\"metallicFactor\":0.25"));
  ASSERT(ak_test_file_contains(incompatibleGltfPath, "\"roughnessFactor\":0.75"));
  ASSERT(!ak_test_file_contains(incompatibleGltfPath, "\"images\""));
  ASSERT(!ak_test_file_contains(incompatibleGltfPath, "\"textures\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(compatibleOutDir);
  ak_test_export_cleanup(incompatibleOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}
