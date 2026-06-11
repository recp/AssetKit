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

TEST_IMPL(gltf_export_texture_extras) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_texture_extras");
  const char *sourceDir     = "./assetkit_export_texture_extras_src";
  const char *sourceTexDir  = "./assetkit_export_texture_extras_src/textures";
  const char *sourceTexPath = "./assetkit_export_texture_extras_src/textures/Extra.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
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
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler   = ak_heap_calloc(heap, doc, sizeof(*sampler));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);
  ASSERT(sampler != NULL);
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "textures/Extra.PNG";
  image->source  = source;
  texture->image = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;

  ak_setypeid(texref, AKT_TEXTURE_REF);
  ak_setypeid(texture, AKT_TEXTURE);
  ak_setypeid(sampler, AKT_SAMPLER2D);
  ak_extra_set(texref, ak_test_extra_pair(heap, texref, "texrefNote", "roundtrip"));
  ak_extra_set(texture, ak_test_extra_pair(heap, texture, "textureNote", "roundtrip"));
  ak_extra_set(image, ak_test_extra_pair(heap, image, "imageNote", "roundtrip"));
  ak_extra_set(sampler, ak_test_extra_pair(heap, sampler, "samplerNote", "roundtrip"));

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"extras\":{\"texrefNote\":\"roundtrip\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"samplers\":[{\"extras\":{\"samplerNote\":\"roundtrip\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"images\":[{\"uri\":\"textures/Extra.PNG\",\"extras\":{\"imageNote\":\"roundtrip\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"textures\":[{\"sampler\":0,\"source\":0,\"extras\":{\"textureNote\":\"roundtrip\"}}]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_file_uri) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_file_uri");
  const char *sourceDir = "./assetkit_export_material_texture_file_uri_src";
  const char *sourceTexPath = "./assetkit_export_material_texture_file_uri_src/WoodFile.PNG";
  const char *copiedTexPath = "./assetkit_export_material_texture_file_uri/image_0_WoodFile.PNG";
  char        absTexPath[PATH_MAX];
  char        fileUri[PATH_MAX + 8u];
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }
  ASSERT(realpath(sourceTexPath, absTexPath) != NULL);
  snprintf(fileUri, sizeof(fileUri), "file://%s", absTexPath);

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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = fileUri;
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_0_WoodFile.PNG\""));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_cwd_relative_uri) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_cwd_relative_uri");
  const char *sourceTexPath = "./assetkit_export_cwd_relative_texture.png";
  const char *sourceUri     = "assetkit_export_cwd_relative_texture.png";
  const char *copiedTexPath = "./assetkit_export_material_texture_cwd_relative_uri/assetkit_export_cwd_relative_texture.png";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = sourceUri;
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"uri\":\"assetkit_export_cwd_relative_texture.png\""));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rewrites_encoded_unsafe_texture_uri) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_encoded_unsafe_uri");
  const char *sourceDir = "./assetkit_export_encoded_unsafe_uri_src";
  const char *sourceTexDir = "./assetkit_export_encoded_unsafe_uri_src/textures";
  const char *sourceTexPath = "./assetkit_export_encoded_unsafe_uri_src/leak.png";
  const char *copiedTexPath = "./assetkit_export_encoded_unsafe_uri/image_0_leak.png";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
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
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "textures/%2e%2e/leak.png";
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_0_leak.png\""));
  ASSERT(!ak_test_file_contains(gltfPath, "%2e%2e"));
  ASSERT(!ak_test_file_contains(gltfPath, "../"));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_uri_collision) {
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
  AkMaterialInput   *normal;
  AkTextureRef      *baseRef;
  AkTextureRef      *normalRef;
  AkTexture         *baseTexture;
  AkTexture         *normalTexture;
  AkImage           *baseImage;
  AkImage           *normalImage;
  AkImageSource     *baseSource;
  AkImageSource     *normalSource;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_image_uri_collision");
  const char *sourceDir = "./assetkit_export_image_uri_collision_src";
  const char *sourceTexPath = "./assetkit_export_image_uri_collision_src/WoodFile.PNG";
  const char *authoredTexPath = "./assetkit_export_image_uri_collision/image_1_WoodFile.PNG";
  const char *generatedTexPath = "./assetkit_export_image_uri_collision/image_1_1_WoodFile.PNG";
  char        absTexPath[PATH_MAX];
  char        fileUri[PATH_MAX + 8u];
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }
  ASSERT(realpath(sourceTexPath, absTexPath) != NULL);
  snprintf(fileUri, sizeof(fileUri), "file://%s", absTexPath);

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

  mat           = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface       = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor     = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  normal        = ak_heap_calloc(heap, surface, sizeof(*normal));
  baseRef       = ak_heap_calloc(heap, baseColor, sizeof(*baseRef));
  normalRef     = ak_heap_calloc(heap, normal, sizeof(*normalRef));
  baseTexture   = ak_heap_calloc(heap, doc, sizeof(*baseTexture));
  normalTexture = ak_heap_calloc(heap, doc, sizeof(*normalTexture));
  baseImage     = ak_heap_calloc(heap, doc, sizeof(*baseImage));
  normalImage   = ak_heap_calloc(heap, doc, sizeof(*normalImage));
  baseSource    = ak_heap_calloc(heap, baseImage, sizeof(*baseSource));
  normalSource  = ak_heap_calloc(heap, normalImage, sizeof(*normalSource));

  baseSource->type = AK_IMAGE_SOURCE_URI;
  baseSource->uri  = "image_1_WoodFile.PNG";
  baseImage->source = baseSource;
  baseTexture->image = baseImage;
  baseRef->texture = baseTexture;

  normalSource->type = AK_IMAGE_SOURCE_URI;
  normalSource->uri  = fileUri;
  normalImage->source = normalSource;
  normalTexture->image = normalImage;
  normalRef->texture = normalTexture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = baseRef;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  normal->source    = AK_MATERIAL_INPUT_TEXTURE;
  normal->valueType = AK_MATERIAL_VALUE_TEXTURE;
  normal->texture   = normalRef;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  surface->normal    = normal;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_1_WoodFile.PNG\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_1_1_WoodFile.PNG\""));
  ASSERT(!ak_test_file_contains(authoredTexPath, "PNGDATA"));
  ASSERT(ak_test_file_contains(generatedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_instance_texcoord_binding) {
  AkHeap                 *heap;
  AkDoc                  *doc;
  AkScene                *scene;
  AkNode                 *root, *node;
  AkGeometry             *geom;
  AkMesh                 *mesh;
  AkMeshPrimitive        *prim;
  AkInstanceGeometry     *inst;
  AkMaterial             *mat;
  AkMaterialSurface      *surface;
  AkMaterialInput        *baseColor;
  AkTextureRef           *texref;
  AkTexture              *texture;
  AkImage                *image;
  AkImageSource          *source;
  AkMaterialBinding      *binding;
  AkMaterialInputBinding *inputBinding;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_instance_texcoord_binding");
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "data:image/png;base64,QUJD";
  image->source  = source;
  texture->image = image;

  texref->slot     = 0;
  texref->texcoord = "UVSET";
  texref->texture  = texture;
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  mat->surface         = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor   = baseColor;

  binding      = ak_heap_calloc(heap, node, sizeof(*binding));
  inputBinding = ak_heap_calloc(heap, binding, sizeof(*inputBinding));
  binding->material      = mat;
  binding->primitive     = prim;
  binding->scope         = AK_MATERIAL_BIND_OBJECT;
  binding->propertyIndex = UINT32_MAX;
  binding->variantIndex  = UINT32_MAX;
  binding->inputBindings = inputBinding;
  inputBinding->semantic = "UVSET";
  inputBinding->inputSet = 1;

  ak_addSubNode(root, node, false);
  inst = ak_nodeAttachGeometry(node, geom);
  ASSERT(inst != NULL);
  inst->objectBindings = binding;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_0\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"TEXCOORD_1\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":1"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_texcoord_binding_uses_source_set_zero) {
  AkHeap                 *heap;
  AkDoc                  *doc;
  AkScene                *scene;
  AkNode                 *root, *node;
  AkGeometry             *geom;
  AkMesh                 *mesh;
  AkMeshPrimitive        *prim;
  AkInput                *tex0;
  AkInput                *tex1;
  AkInstanceGeometry     *inst;
  AkMaterial             *mat;
  AkMaterialSurface      *surface;
  AkMaterialInput        *baseColor;
  AkTextureRef           *texref;
  AkTexture              *texture;
  AkImage                *image;
  AkImageSource          *source;
  AkMaterialBinding      *binding;
  AkMaterialInputBinding *inputBinding;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_texcoord_source_set_zero");
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
  tex0 = ak_test_add_texcoord_input(heap, prim, 0);
  tex1 = ak_test_add_texcoord_input(heap, prim, 1);
  ASSERT(tex0 != NULL);
  ASSERT(tex1 != NULL);

  tex0->index = 1;
  tex1->index = 0;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "data:image/png;base64,QUJD";
  image->source  = source;
  texture->image = image;

  texref->slot     = 0;
  texref->texcoord = "UVSET0";
  texref->texture  = texture;
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  mat->surface         = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor   = baseColor;

  binding      = ak_heap_calloc(heap, node, sizeof(*binding));
  inputBinding = ak_heap_calloc(heap, binding, sizeof(*inputBinding));
  binding->material      = mat;
  binding->primitive     = prim;
  binding->scope         = AK_MATERIAL_BIND_OBJECT;
  binding->propertyIndex = UINT32_MAX;
  binding->variantIndex  = UINT32_MAX;
  binding->inputBindings = inputBinding;
  inputBinding->semantic = "UVSET0";
  inputBinding->inputSet = 0;

  ak_addSubNode(root, node, false);
  inst = ak_nodeAttachGeometry(node, geom);
  ASSERT(inst != NULL);
  inst->objectBindings = binding;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_0\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_1\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"material\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":1"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_unsupported_image_uri) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_unsupported_image_uri");
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "textures/diffuse.tga";
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"images\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"textures\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"baseColorTexture\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_data_uri) {
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
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_data_uri");
  const char *dataUri = "data:image/png;base64,QUJD";
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = dataUri;
  image->source = source;

  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"images\":[{\"uri\":\"data:image/png;base64,QUJD\"}]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_converts_loaded_unsupported_image_to_png) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AkResult           exportResult;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_converted_image_png");
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

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "textures/diffuse.tga";
  image->source  = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ak_imageInitLoader(ak_test_load_rgba_file, NULL);
  exportResult = ak_export(doc, outDir, AK_FILE_TYPE_GLTF);
  ak_imageInitLoader(NULL, NULL);

  ASSERT(exportResult == AK_OK);
  ASSERT(source->resolvedPath != NULL);
  ASSERT(strcmp(source->resolvedPath, "textures/diffuse.tga") == 0);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(gltfPath, "diffuse.tga"));
  ASSERT(ak_test_file_contains(binPath, "PNG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_converts_bmp_uri_to_png_without_loader) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bmp_image_png");
  const char *sourceDir = "./assetkit_export_bmp_source";
  const char *sourceBmp = "./assetkit_export_bmp_source/diffuse.bmp";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceBmp);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(ak_test_write_bmp_1x1(sourceBmp));

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

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = sourceBmp;
  image->source  = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  mat->surface         = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor   = baseColor;
  prim->material       = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ak_imageInitLoader(NULL, NULL);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(gltfPath, "diffuse.bmp"));
  ASSERT(ak_test_file_contains(binPath, "PNG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceBmp);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_converts_decoded_image_to_png) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_decoded_image_png");
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
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));

  image->data = ak_test_load_rgba_file(heap, image, NULL, false);
  ASSERT(image->data != NULL);
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"mimeType\":\"image/png\""));
  ASSERT(ak_test_file_count(gltfPath, "\"uri\"") == 1u);
  ASSERT(ak_test_file_contains(binPath, "PNG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_glb_embeds_decoded_uri_image) {
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
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_glb_decoded_uri_image");
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

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "missing/Wood.PNG";
  image->source = source;
  image->data   = ak_test_load_rgba_file(heap, image, NULL, false);
  ASSERT(image->data != NULL);

  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(ak_test_file_contains(glbPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(glbPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(glbPath, "\"uri\":\"missing/Wood.PNG\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_ktx2) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkScene          *scene;
  AkNode           *root, *node;
  AkGeometry       *geom;
  AkMesh           *mesh;
  AkMeshPrimitive  *prim;
  AkMaterial       *mat;
  AkMaterialSurface *surface;
  AkMaterialInput  *baseColor;
  AkTextureRef     *texref;
  AkTexture        *texture;
  AkImage          *image;
  AkImageSource    *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_ktx2");
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
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "albedo.ktx2";
  image->source = source;
  texture->image = image;
  texref->texture = texture;
  texref->slot = 0;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor        = baseColor;
  mat->surface              = surface;
  prim->material            = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"KHR_texture_basisu\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"KHR_texture_basisu\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_texture_basisu\":{\"source\":0}}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"albedo.ktx2\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_webp) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkScene          *scene;
  AkNode           *root, *node;
  AkGeometry       *geom;
  AkMesh           *mesh;
  AkMeshPrimitive  *prim;
  AkMaterial       *mat;
  AkMaterialSurface *surface;
  AkMaterialInput  *baseColor;
  AkTextureRef     *texref;
  AkTexture        *texture;
  AkImage          *image;
  AkImageSource    *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_webp");
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
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "albedo.webp";
  image->source = source;
  texture->image = image;
  texref->texture = texture;
  texref->slot = 0;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor        = baseColor;
  mat->surface              = surface;
  prim->material            = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"EXT_texture_webp\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"EXT_texture_webp\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_texture_webp\":{\"source\":0}}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"albedo.webp\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_transform) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root, *node;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkMaterial         *mat;
  AkMaterialSurface  *surface;
  AkMaterialInput    *baseColor;
  AkTextureRef       *texref;
  AkTextureTransform *transform;
  AkTexture          *texture;
  AkImage            *image;
  AkImageSource      *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_transform");
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  transform = ak_heap_calloc(heap, texref, sizeof(*transform));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "albedo.png";
  image->source = source;
  texture->image = image;

  transform->offset[0] = 0.25f;
  transform->offset[1] = 0.5f;
  transform->rotation  = 1.0f;
  transform->scale[0]  = 2.0f;
  transform->scale[1]  = 3.0f;
  transform->slot      = 1;

  texref->texture   = texture;
  texref->slot      = 0;
  texref->transform = transform;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior       = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_texture_transform\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsUsed\":["));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"texCoord\":1,\"extensions\":{\"KHR_texture_transform\":{\"offset\":[0.25,0.5],\"rotation\":1,\"scale\":[2,3],\"texCoord\":1}}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_transform_invalid_slot) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root, *node;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkMaterial         *mat;
  AkMaterialSurface  *surface;
  AkMaterialInput    *baseColor;
  AkTextureRef       *texref;
  AkTextureTransform *transform;
  AkTexture          *texture;
  AkImage            *image;
  AkImageSource      *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_transform_bad_slot");
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  transform = ak_heap_calloc(heap, texref, sizeof(*transform));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "data:image/png;base64,QUJD";
  image->source = source;
  texture->image = image;

  transform->offset[0] = 0.25f;
  transform->offset[1] = 0.5f;
  transform->scale[0]  = 1.0f;
  transform->scale[1]  = 1.0f;
  transform->slot      = 7;

  texref->texture   = texture;
  texref->slot      = 0;
  texref->transform = transform;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior       = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_texture_transform\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"extensions\":{\"KHR_texture_transform\":{\"offset\":[0.25,0.5]}}}"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":7"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":0"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_buffer_view) {
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
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AkBuffer          *imageBuffer;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_buffer");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const unsigned char pngBytes[8] = {
    0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'
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

  mat         = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface     = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor   = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref      = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture     = ak_heap_calloc(heap, doc, sizeof(*texture));
  image       = ak_heap_calloc(heap, doc, sizeof(*image));
  source      = ak_heap_calloc(heap, image, sizeof(*source));
  imageBuffer = ak_heap_calloc(heap, source, sizeof(*imageBuffer));

  imageBuffer->length = sizeof(pngBytes);
  imageBuffer->data = ak_heap_alloc(heap, imageBuffer, imageBuffer->length);
  memcpy(imageBuffer->data, pngBytes, sizeof(pngBytes));

  source->type     = AK_IMAGE_SOURCE_BUFFER;
  source->buffer   = imageBuffer;
  source->mimeType = "image/png";
  image->source    = source;

  texture->image = image;
  texref->texture = texture;
  texref->slot = 0;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"accessors\":[{\"bufferView\":0"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"images\":[{\"bufferView\":1,\"mimeType\":\"image/png\"}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"byteLength\":8"));
  ASSERT(ak_test_file_contains(gltfPath, "\"baseColorTexture\":{\"index\":0}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}
