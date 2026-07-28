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

TEST_IMPL(obj_export_triangle_smoke) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMesh            *roundMesh;
  AkMeshPrimitive   *prim;
  AkMeshPrimitive   *roundPrim;
  AkInput           *colorInput;
  AkInput           *roundInput;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialSurface *roundSurface;
  AkMaterialClearcoatFeature *clearcoat;
  AkMaterialSheenFeature *sheen;
  AkMaterialAnisotropyFeature *anisotropy;
  AkMaterialInput   *baseColor;
  AkMaterialInput   *clearcoatFactor;
  AkMaterialInput   *clearcoatRoughness;
  AkMaterialInput   *sheenFactor;
  AkMaterialInput   *anisotropyStrength;
  AkMaterialInput   *anisotropyRotation;
  AkMaterialInput   *metallic;
  AkMaterialInput   *opacity;
  AkMaterialInput   *roughness;
  struct stat        stObj;
  struct stat        stMtl;
  const char        *outDir  = "./assetkit_export_obj_triangle_smoke";
  const char        *objPath = "./assetkit_export_obj_triangle_smoke/model.obj";
  const char        *mtlPath = "./assetkit_export_obj_triangle_smoke/model.mtl";
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
  const float colors[12] = {
    0.25f, 0.5f, 0.75f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  node->name  = "OBJ Node";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  colorInput = ak_heap_calloc(heap, prim, sizeof(*colorInput));
  ASSERT(colorInput != NULL);
  colorInput->semantic = AK_INPUT_COLOR;
  colorInput->set      = 0;
  colorInput->index    = 0;
  colorInput->accessor = ak_test_make_float_accessor(heap, colorInput, colors, 4, 3);
  colorInput->next     = prim->input;
  prim->input          = colorInput;
  prim->inputCount++;
  prim->flags |= AK_MESH_PRIMITIVE_FLAG_SMOOTH_SHADING;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  clearcoat = ak_heap_calloc(heap, surface, sizeof(*clearcoat));
  sheen     = ak_heap_calloc(heap, surface, sizeof(*sheen));
  anisotropy = ak_heap_calloc(heap, surface, sizeof(*anisotropy));
  baseColor = ak_test_material_input(heap, surface);
  clearcoatFactor = ak_test_material_input(heap, clearcoat);
  clearcoatRoughness = ak_test_material_input(heap, clearcoat);
  sheenFactor = ak_test_material_input(heap, sheen);
  anisotropyStrength = ak_test_material_input(heap, anisotropy);
  anisotropyRotation = ak_test_material_input(heap, anisotropy);
  metallic = ak_test_material_input(heap, surface);
  opacity = ak_test_material_input(heap, surface);
  roughness = ak_test_material_input(heap, surface);

  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(clearcoat != NULL);
  ASSERT(sheen != NULL);
  ASSERT(anisotropy != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(clearcoatFactor != NULL);
  ASSERT(clearcoatRoughness != NULL);
  ASSERT(sheenFactor != NULL);
  ASSERT(anisotropyStrength != NULL);
  ASSERT(anisotropyRotation != NULL);
  ASSERT(metallic != NULL);
  ASSERT(opacity != NULL);
  ASSERT(roughness != NULL);

  mat->name          = "obj_mat";
  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  surface->metallic  = metallic;
  surface->opacity   = opacity;
  surface->roughness = roughness;
  surface->features  = &clearcoat->base;
  clearcoat->base.type = AK_MATERIAL_FEATURE_CLEARCOAT;
  clearcoat->base.next = &sheen->base;
  clearcoat->factor = clearcoatFactor;
  clearcoat->roughness = clearcoatRoughness;
  sheen->base.type = AK_MATERIAL_FEATURE_SHEEN;
  sheen->base.next = &anisotropy->base;
  sheen->color = sheenFactor;
  anisotropy->base.type = AK_MATERIAL_FEATURE_ANISOTROPY;
  anisotropy->strength = anisotropyStrength;
  anisotropy->rotation = anisotropyRotation;
  baseColor->source      = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType   = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.25f;
  baseColor->color.rgba.G = 0.5f;
  baseColor->color.rgba.B = 0.75f;
  baseColor->color.rgba.A = 1.0f;
  clearcoatFactor->source    = AK_MATERIAL_INPUT_CONSTANT;
  clearcoatFactor->valueType = AK_MATERIAL_VALUE_FLOAT;
  clearcoatFactor->value[0]  = 0.75f;
  clearcoatRoughness->source    = AK_MATERIAL_INPUT_CONSTANT;
  clearcoatRoughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  clearcoatRoughness->value[0]  = 0.2f;
  sheenFactor->source    = AK_MATERIAL_INPUT_CONSTANT;
  sheenFactor->valueType = AK_MATERIAL_VALUE_FLOAT;
  sheenFactor->value[0]  = 0.3f;
  anisotropyStrength->source    = AK_MATERIAL_INPUT_CONSTANT;
  anisotropyStrength->valueType = AK_MATERIAL_VALUE_FLOAT;
  anisotropyStrength->value[0]  = 0.4f;
  anisotropyRotation->source    = AK_MATERIAL_INPUT_CONSTANT;
  anisotropyRotation->valueType = AK_MATERIAL_VALUE_FLOAT;
  anisotropyRotation->value[0]  = 0.5f;
  metallic->source    = AK_MATERIAL_INPUT_CONSTANT;
  metallic->valueType = AK_MATERIAL_VALUE_FLOAT;
  metallic->value[0]  = 0.6f;
  opacity->source    = AK_MATERIAL_INPUT_CONSTANT;
  opacity->valueType = AK_MATERIAL_VALUE_FLOAT;
  opacity->value[0]  = 0.8f;
  roughness->source    = AK_MATERIAL_INPUT_CONSTANT;
  roughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  roughness->value[0]  = 0.4f;
  prim->material     = mat;

  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(stat(objPath, &stObj) == 0);
  ASSERT(stat(mtlPath, &stMtl) == 0);
  ASSERT(stObj.st_size > 0);
  ASSERT(stMtl.st_size > 0);
  ASSERT(ak_test_file_contains(objPath, "mtllib model.mtl"));
  ASSERT(ak_test_file_contains(objPath, "o OBJ Node"));
  ASSERT(ak_test_file_contains(objPath, "v 2 3 4 0.25 0.5 0.75 1"));
  ASSERT(ak_test_file_contains(objPath, "vt 0 0"));
  ASSERT(ak_test_file_contains(objPath, "s on"));
  ASSERT(ak_test_file_contains(objPath, "usemtl mat_0_obj_mat"));
  ASSERT(ak_test_file_contains(objPath, "f 1/1 2/2 3/3"));
  ASSERT(ak_test_file_contains(mtlPath, "newmtl mat_0_obj_mat"));
  ASSERT(ak_test_file_contains(mtlPath, "Kd 0.25 0.5 0.75"));
  ASSERT(ak_test_file_contains(mtlPath, "Pr 0.4"));
  ASSERT(ak_test_file_contains(mtlPath, "Pm 0.6"));
  ASSERT(ak_test_file_contains(mtlPath, "Ps 0.3"));
  ASSERT(ak_test_file_contains(mtlPath, "Pc 3"));
  ASSERT(ak_test_file_contains(mtlPath, "Pcr 0.2"));
  ASSERT(ak_test_file_contains(mtlPath, "aniso 0.4"));
  ASSERT(ak_test_file_contains(mtlPath, "anisor 0.5"));
  ASSERT(ak_test_file_contains(mtlPath, "d 0.8"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, objPath, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);
  ASSERT(roundTrip->lib.geometries.first != NULL);
  roundMesh = ak_objGet(roundTrip->lib.geometries.first->gdata);
  ASSERT(roundMesh != NULL);
  roundPrim = roundMesh->primitive;
  ASSERT(roundPrim != NULL);
  ASSERT(ak_meshPrimitiveSmoothShading(roundPrim));
  for (roundInput = roundPrim->input; roundInput; roundInput = roundInput->next) {
    if (roundInput->semantic == AK_INPUT_COLOR)
      break;
  }
  ASSERT(roundInput != NULL);
  ASSERT(roundTrip->lib.materials.first != NULL);
  roundSurface = roundTrip->lib.materials.first->surface;
  ASSERT(roundSurface != NULL);
  ASSERT(roundSurface->roughness != NULL);
  ASSERT(roundSurface->metallic != NULL);
  ASSERT(ak_materialRoughnessFactor(roundSurface) > 0.399f);
  ASSERT(ak_materialRoughnessFactor(roundSurface) < 0.401f);
  ASSERT(ak_materialMetallicFactor(roundSurface) > 0.599f);
  ASSERT(ak_materialMetallicFactor(roundSurface) < 0.601f);
  ASSERT(ak_materialFeature(roundSurface, AK_MATERIAL_FEATURE_CLEARCOAT) != NULL);
  ASSERT(ak_materialFeature(roundSurface, AK_MATERIAL_FEATURE_SHEEN) != NULL);
  ASSERT(ak_materialFeature(roundSurface, AK_MATERIAL_FEATURE_ANISOTROPY) != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_ubyte_vertex_colors_are_normalized) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *colorInput;
  const char      *outDir  = "./assetkit_export_obj_ubyte_color";
  const char      *objPath = "./assetkit_export_obj_ubyte_color/model.obj";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const uint8_t colors[9] = {
    255, 128, 0,
    0, 255, 0,
    0, 0, 255
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

  colorInput = ak_heap_calloc(heap, prim, sizeof(*colorInput));
  ASSERT(colorInput != NULL);
  colorInput->semantic = AK_INPUT_COLOR;
  colorInput->set      = 0;
  colorInput->index    = 0;
  colorInput->accessor = ak_test_make_ubyte_accessor(heap, colorInput, colors, 3, 3);
  colorInput->next     = prim->input;
  prim->input          = colorInput;
  prim->inputCount++;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(ak_test_file_contains(objPath,
                               "v 0 0 0 1 0.501961 0"));
  ASSERT(!ak_test_file_contains(objPath, "v 0 0 0 255 128 0"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_native_index_accessor) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  const char      *outDir  = "./assetkit_export_obj_native_index";
  const char      *objPath = "./assetkit_export_obj_native_index/model.obj";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const uint8_t indices[3] = {2u, 1u, 0u};

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
  prim->indexAccessor = ak_test_make_ubyte_accessor(heap,
                                                    prim,
                                                    indices,
                                                    1,
                                                    3);
  ASSERT(prim->indexAccessor != NULL);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(ak_test_file_contains(objPath, "f 3 2 1"));
  ASSERT(!ak_test_file_contains(objPath, "f 1 2 3"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_numbers_are_locale_independent) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  char        oldLocale[128];
  const char *locale;
  const char *old;
  AkResult    result;
  const char *outDir  = "./assetkit_export_obj_locale_numbers";
  const char *objPath = "./assetkit_export_obj_locale_numbers/model.obj";
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

  result = ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT);
  if (oldLocale[0])
    setlocale(LC_NUMERIC, oldLocale);
  else
    setlocale(LC_NUMERIC, "C");

  ASSERT(result == AK_OK);
  ASSERT(ak_test_file_contains(objPath, "v 1.25 2.5 3.75"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_missing_texture_map_is_best_effort) {
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
  struct stat        stTex;
  const char        *outDir  = "./assetkit_export_obj_missing_texture";
  const char        *objPath = "./assetkit_export_obj_missing_texture/model.obj";
  const char        *mtlPath = "./assetkit_export_obj_missing_texture/model.mtl";
  const char        *copiedPath =
    "./assetkit_export_obj_missing_texture/image_0_Missing.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(copiedPath);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = ".";

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
  baseColor = ak_test_material_input(heap, surface);
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

  source->type     = AK_IMAGE_SOURCE_URI;
  source->uri      = "textures/Missing.PNG";
  image->source    = source;
  texture->image   = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;

  baseColor->source       = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture      = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->name          = "missing_tex";
  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;
  doc->lib.images.first    = image;
  doc->lib.images.last     = image;
  doc->lib.images.count    = 1;
  doc->lib.textures.first  = texture;
  doc->lib.textures.last   = texture;
  doc->lib.textures.count  = 1;
  doc->lib.samplers.first  = sampler;
  doc->lib.samplers.last   = sampler;
  doc->lib.samplers.count  = 1;
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(ak_test_file_contains(objPath, "mtllib model.mtl"));
  ASSERT(ak_test_file_contains(objPath, "usemtl mat_0_missing_tex"));
  ASSERT(ak_test_file_contains(mtlPath, "newmtl mat_0_missing_tex"));
  ASSERT(!ak_test_file_contains(mtlPath, "map_Kd"));
  ASSERT(stat(copiedPath, &stTex) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(copiedPath);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_material_texture_copy) {
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
  AkMaterialClassicFeature *classic;
  AkMaterialInput   *baseColor;
  AkMaterialInput   *metallic;
  AkMaterialInput   *roughness;
  AkTextureRef      *texref;
  AkTextureRef      *metalRef;
  AkTextureRef      *roughRef;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  AkMaterialSurface *roundSurface;
  struct stat        stTex;
  const char        *outDir  = "./assetkit_export_obj_texture_copy";
  const char        *objPath = "./assetkit_export_obj_texture_copy/model.obj";
  const char        *mtlPath = "./assetkit_export_obj_texture_copy/model.mtl";
  const char        *copiedPath = "./assetkit_export_obj_texture_copy/image_0_Extra.PNG";
  const char        *sourceDir = "./assetkit_export_obj_texture_copy_src";
  const char        *sourceTexDir = "./assetkit_export_obj_texture_copy_src/textures";
  const char        *sourceTexPath = "./assetkit_export_obj_texture_copy_src/textures/Extra.PNG";
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

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  classic   = ak_heap_calloc(heap, surface, sizeof(*classic));
  baseColor = ak_test_material_input(heap, surface);
  metallic = ak_test_material_input(heap, surface);
  roughness = ak_test_material_input(heap, surface);
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  metalRef  = ak_heap_calloc(heap, metallic, sizeof(*metalRef));
  roughRef  = ak_heap_calloc(heap, roughness, sizeof(*roughRef));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler   = ak_heap_calloc(heap, doc, sizeof(*sampler));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(classic != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(metallic != NULL);
  ASSERT(roughness != NULL);
  ASSERT(texref != NULL);
  ASSERT(metalRef != NULL);
  ASSERT(roughRef != NULL);
  ASSERT(texture != NULL);
  ASSERT(sampler != NULL);
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  source->type     = AK_IMAGE_SOURCE_URI;
  source->uri      = "textures/Extra.PNG";
  image->source    = source;
  texture->image   = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;
  metalRef->texture = texture;
  metalRef->slot    = 0;
  roughRef->texture = texture;
  roughRef->slot    = 0;

  baseColor->source       = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture      = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;
  metallic->source    = AK_MATERIAL_INPUT_TEXTURE;
  metallic->valueType = AK_MATERIAL_VALUE_FLOAT;
  metallic->value[0]  = 0.5f;
  metallic->channels  = AK_TEXTURE_CHANNEL_B;
  metallic->texture   = metalRef;
  roughness->source    = AK_MATERIAL_INPUT_TEXTURE;
  roughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  roughness->value[0]  = 0.25f;
  roughness->channels  = AK_TEXTURE_CHANNEL_G;
  roughness->texture   = roughRef;

  mat->name          = "tex_mat";
  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  surface->metallic  = metallic;
  surface->roughness = roughness;
  surface->features  = &classic->base;
  classic->base.type = AK_MATERIAL_FEATURE_CLASSIC;
  classic->specular  = baseColor;
  prim->material     = mat;

  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;
  doc->lib.images.first    = image;
  doc->lib.images.last     = image;
  doc->lib.images.count    = 1;
  doc->lib.textures.first  = texture;
  doc->lib.textures.last   = texture;
  doc->lib.textures.count  = 1;
  doc->lib.samplers.first  = sampler;
  doc->lib.samplers.last   = sampler;
  doc->lib.samplers.count  = 1;
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(stat(copiedPath, &stTex) == 0);
  ASSERT(stTex.st_size == 7);
  ASSERT(ak_test_file_contains(mtlPath, "map_Kd image_0_Extra.PNG"));
  ASSERT(ak_test_file_contains(mtlPath, "map_Ks image_0_Extra.PNG"));
  ASSERT(ak_test_file_contains(mtlPath, "map_Pr -imfchan g image_0_Extra.PNG"));
  ASSERT(ak_test_file_contains(mtlPath, "map_Pm -imfchan b image_0_Extra.PNG"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, objPath, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.materials.first != NULL);
  roundSurface = roundTrip->lib.materials.first->surface;
  ASSERT(roundSurface != NULL);
  ASSERT(roundSurface->roughness != NULL);
  ASSERT(roundSurface->metallic != NULL);
  ASSERT(ak_materialInputChannels(roundSurface->roughness) == AK_TEXTURE_CHANNEL_G);
  ASSERT(ak_materialInputChannels(roundSurface->metallic) == AK_TEXTURE_CHANNEL_B);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_material_absolute_texture_copy) {
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
  struct stat        stTex;
  char               cwd[PATH_MAX];
  char               absSourceTexPath[PATH_MAX];
  const char        *outDir  = "./assetkit_export_obj_abs_texture";
  const char        *mtlPath = "./assetkit_export_obj_abs_texture/model.mtl";
  const char        *copiedPath =
    "./assetkit_export_obj_abs_texture/image_0_WoodFile.PNG";
  const char        *sourceDir = "./assetkit_export_obj_abs_texture_src";
  const char        *sourceTexDir =
    "./assetkit_export_obj_abs_texture_src/textures";
  const char        *sourceTexPath =
    "./assetkit_export_obj_abs_texture_src/textures/WoodFile.PNG";
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
    ASSERT(fwrite("ABSIMG", 1, 6, file) == 6);
    ASSERT(fclose(file) == 0);
  }
  ASSERT(getcwd(cwd, sizeof(cwd)) != NULL);
  snprintf(absSourceTexPath,
           sizeof(absSourceTexPath),
           "%s/%s",
           cwd,
           sourceTexPath + 2);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = ".";

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
  baseColor = ak_test_material_input(heap, surface);
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

  source->type     = AK_IMAGE_SOURCE_URI;
  source->uri      = absSourceTexPath;
  image->source    = source;
  texture->image   = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;

  baseColor->source       = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture      = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->name          = "abs_tex";
  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;
  doc->lib.images.first    = image;
  doc->lib.images.last     = image;
  doc->lib.images.count    = 1;
  doc->lib.textures.first  = texture;
  doc->lib.textures.last   = texture;
  doc->lib.textures.count  = 1;
  doc->lib.samplers.first  = sampler;
  doc->lib.samplers.last   = sampler;
  doc->lib.samplers.count  = 1;
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(stat(copiedPath, &stTex) == 0);
  ASSERT(stTex.st_size == 6);
  ASSERT(ak_test_file_contains(mtlPath, "map_Kd image_0_WoodFile.PNG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_skips_unused_material_textures) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *usedMat;
  AkMaterial        *unusedMat;
  AkMaterialSurface *usedSurface;
  AkMaterialSurface *unusedSurface;
  AkMaterialInput   *usedBaseColor;
  AkMaterialInput   *unusedBaseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  struct stat        stTex;
  const char        *outDir  = "./assetkit_export_obj_unused_texture";
  const char        *mtlPath = "./assetkit_export_obj_unused_texture/model.mtl";
  const char        *copiedPath =
    "./assetkit_export_obj_unused_texture/image_0_WoodFile.PNG";
  const char        *sourceDir = "./assetkit_export_obj_unused_texture_src";
  const char        *sourceTexDir =
    "./assetkit_export_obj_unused_texture_src/textures";
  const char        *sourceTexPath =
    "./assetkit_export_obj_unused_texture_src/textures/WoodFile.PNG";
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
    ASSERT(fwrite("UNUSED", 1, 6, file) == 6);
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

  usedMat         = ak_heap_calloc(heap, doc, sizeof(*usedMat));
  unusedMat       = ak_heap_calloc(heap, doc, sizeof(*unusedMat));
  usedSurface     = ak_heap_calloc(heap, usedMat, sizeof(*usedSurface));
  unusedSurface   = ak_heap_calloc(heap, unusedMat, sizeof(*unusedSurface));
  usedBaseColor = ak_test_material_input(heap, usedSurface);
  unusedBaseColor = ak_test_material_input(heap, unusedSurface);
  texref          = ak_heap_calloc(heap, unusedBaseColor, sizeof(*texref));
  texture         = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler         = ak_heap_calloc(heap, doc, sizeof(*sampler));
  image           = ak_heap_calloc(heap, doc, sizeof(*image));
  source          = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(usedMat != NULL);
  ASSERT(unusedMat != NULL);
  ASSERT(usedSurface != NULL);
  ASSERT(unusedSurface != NULL);
  ASSERT(usedBaseColor != NULL);
  ASSERT(unusedBaseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);
  ASSERT(sampler != NULL);
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  usedBaseColor->source       = AK_MATERIAL_INPUT_CONSTANT;
  usedBaseColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  usedBaseColor->color.rgba.R = 0.25f;
  usedBaseColor->color.rgba.G = 0.5f;
  usedBaseColor->color.rgba.B = 0.75f;
  usedBaseColor->color.rgba.A = 1.0f;

  source->type     = AK_IMAGE_SOURCE_URI;
  source->uri      = "textures/WoodFile.PNG";
  image->source    = source;
  texture->image   = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;

  unusedBaseColor->source       = AK_MATERIAL_INPUT_TEXTURE;
  unusedBaseColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  unusedBaseColor->texture      = texref;
  unusedBaseColor->color.rgba.R = 1.0f;
  unusedBaseColor->color.rgba.G = 1.0f;
  unusedBaseColor->color.rgba.B = 1.0f;
  unusedBaseColor->color.rgba.A = 1.0f;

  usedMat->name          = "used_mat";
  usedMat->surface       = usedSurface;
  usedMat->next          = unusedMat;
  usedSurface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  usedSurface->baseColor = usedBaseColor;

  unusedMat->name          = "unused_mat";
  unusedMat->surface       = unusedSurface;
  unusedSurface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  unusedSurface->baseColor = unusedBaseColor;
  prim->material           = usedMat;

  doc->lib.materials.first = usedMat;
  doc->lib.materials.last  = unusedMat;
  doc->lib.materials.count = 2;
  doc->lib.images.first    = image;
  doc->lib.images.last     = image;
  doc->lib.images.count    = 1;
  doc->lib.textures.first  = texture;
  doc->lib.textures.last   = texture;
  doc->lib.textures.count  = 1;
  doc->lib.samplers.first  = sampler;
  doc->lib.samplers.last   = sampler;
  doc->lib.samplers.count  = 1;
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(ak_test_file_contains(mtlPath, "newmtl mat_0_used_mat"));
  ASSERT(ak_test_file_count(mtlPath, "newmtl ") == 1);
  ASSERT(!ak_test_file_contains(mtlPath, "mat_1_unused_mat"));
  ASSERT(!ak_test_file_contains(mtlPath, "map_Kd"));
  ASSERT(stat(copiedPath, &stTex) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_polygon_degenerate_advances_cursor) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkTriangles     *tri;
  AkPolygon       *poly;
  AkUIntArray     *vcount;
  const char      *outDir  = "./assetkit_export_obj_poly_cursor";
  const char      *objPath = "./assetkit_export_obj_poly_cursor/model.obj";
  const float positions[15] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    2.0f, 0.0f, 0.0f,
    2.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f
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

  geom = ak_test_make_geom_with_positions(heap, doc, positions, 5);
  mesh = ak_objGet(geom->gdata);
  tri  = (AkTriangles *)mesh->primitive;
  poly = ak_heap_calloc(heap, geom->gdata, sizeof(*poly));
  vcount = ak_heap_alloc(heap, poly, sizeof(*vcount) + sizeof(AkUInt) * 2u);
  ASSERT(poly != NULL);
  ASSERT(vcount != NULL);

  poly->base        = tri->base;
  poly->base.type   = AK_PRIMITIVE_POLYGONS;
  poly->base.nPolygons = 2;
  poly->vcount      = vcount;
  vcount->count     = 2;
  vcount->items[0]  = 2;
  vcount->items[1]  = 3;
  mesh->primitive   = &poly->base;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) == AK_OK);
  ASSERT(ak_test_file_contains(objPath, "f 3 4 5"));
  ASSERT(!ak_test_file_contains(objPath, "f 1 2 3"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(obj_export_rejects_nonfinite_float) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stObj;
  const char  *outDir  = "./assetkit_export_obj_nonfinite_float";
  const char  *objPath = "./assetkit_export_obj_nonfinite_float/model.obj";
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

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_nodeSetTransformMatrix(node, matrix);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_WAVEFRONT) != AK_OK);
  ASSERT(stat(objPath, &stObj) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}
