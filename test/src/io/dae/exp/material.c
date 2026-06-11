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

TEST_IMPL(dae_export_material_texture_smoke) {
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
  AkMaterialInput   *emissive;
  AkMaterialInput   *metallic;
  AkMaterialInput   *opacity;
  AkMaterialInput   *roughness;
  AkTextureRef      *texref;
  AkTextureRef      *emissiveRef;
  AkTextureRef      *opacityRef;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  struct stat        stDae;
  const char        *outDir  = "./assetkit_export_dae_material_texture";
  const char        *daePath = "./assetkit_export_dae_material_texture/model.dae";
  const char        *sourceDir = "./assetkit_export_dae_material_texture_src";
  const char        *sourceTexDir =
    "./assetkit_export_dae_material_texture_src/textures";
  const char        *sourceTexPath =
    "./assetkit_export_dae_material_texture_src/textures/Wood File.PNG";
  const char        *copiedTexPath =
    "./assetkit_export_dae_material_texture/textures/Wood File.PNG";
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
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  emissive  = ak_heap_calloc(heap, surface, sizeof(*emissive));
  metallic  = ak_heap_calloc(heap, surface, sizeof(*metallic));
  opacity   = ak_heap_calloc(heap, surface, sizeof(*opacity));
  roughness = ak_heap_calloc(heap, surface, sizeof(*roughness));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  emissiveRef = ak_heap_calloc(heap, emissive, sizeof(*emissiveRef));
  opacityRef  = ak_heap_calloc(heap, opacity, sizeof(*opacityRef));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler   = ak_heap_calloc(heap, texture, sizeof(*sampler));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(emissive != NULL);
  ASSERT(metallic != NULL);
  ASSERT(opacity != NULL);
  ASSERT(roughness != NULL);
  ASSERT(texref != NULL);
  ASSERT(emissiveRef != NULL);
  ASSERT(opacityRef != NULL);
  ASSERT(texture != NULL);
  ASSERT(sampler != NULL);
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "textures/Wood File.PNG";
  image->name   = "wood";
  image->source = source;

  texture->image = image;
  texture->sampler = sampler;
  sampler->wrapS = AK_WRAP_MODE_CLAMP;
  sampler->wrapT = AK_WRAP_MODE_WRAP;
  sampler->minfilter = AK_MINFILTER_NEAREST;
  sampler->magfilter = AK_MAGFILTER_NEAREST;
  texref->texture = texture;
  texref->slot = 0;
  emissiveRef->texture = texture;
  emissiveRef->slot    = 1;
  opacityRef->texture  = texture;
  opacityRef->slot     = 1;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.8f;
  baseColor->color.rgba.G = 0.7f;
  baseColor->color.rgba.B = 0.6f;
  baseColor->color.rgba.A = 1.0f;
  baseColor->texture   = texref;
  emissive->source     = AK_MATERIAL_INPUT_TEXTURE;
  emissive->valueType  = AK_MATERIAL_VALUE_COLOR;
  emissive->texture    = emissiveRef;
  metallic->source     = AK_MATERIAL_INPUT_CONSTANT;
  metallic->valueType  = AK_MATERIAL_VALUE_FLOAT;
  metallic->value[0]   = 0.75f;
  opacity->source      = AK_MATERIAL_INPUT_TEXTURE;
  opacity->valueType   = AK_MATERIAL_VALUE_FLOAT;
  opacity->texture     = opacityRef;
  roughness->source    = AK_MATERIAL_INPUT_CONSTANT;
  roughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  roughness->value[0]  = 0.25f;
  mat->name            = "wood_mat";
  mat->surface         = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->flags       = AK_MATERIAL_FLAG_HAS_IOR;
  surface->ior         = 1.45f;
  surface->baseColor   = baseColor;
  surface->emissive    = emissive;
  surface->metallic    = metallic;
  surface->opacity     = opacity;
  surface->roughness   = roughness;
  prim->material       = mat;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;
  doc->lib.materials.first  = mat;
  doc->lib.materials.last   = mat;
  doc->lib.materials.count  = 1;
  doc->lib.images.first     = image;
  doc->lib.images.last      = image;
  doc->lib.images.count     = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath, "<library_images>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<init_from>textures/Wood File.PNG</init_from>"));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));
  ASSERT(ak_test_file_contains(daePath, "<library_effects>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<texture texture=\"sampler_0\" texcoord=\"TEXCOORD0\"/>"));
  ASSERT(ak_test_file_contains(daePath, "<wrap_s>CLAMP</wrap_s>"));
  ASSERT(ak_test_file_contains(daePath, "<wrap_t>WRAP</wrap_t>"));
  ASSERT(ak_test_file_contains(daePath, "<minfilter>NEAREST</minfilter>"));
  ASSERT(ak_test_file_contains(daePath, "<magfilter>NEAREST</magfilter>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<emission><texture texture=\"sampler_0_emission\" texcoord=\"TEXCOORD1\"/></emission>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<transparent opaque=\"A_ONE\"><texture texture=\"sampler_0_transparent\" texcoord=\"TEXCOORD1\"/></transparent>"));
  ASSERT(ak_test_file_contains(daePath, "<specular><color>"));
  ASSERT(ak_test_file_contains(daePath, "<shininess><float>30</float></shininess>"));
  ASSERT(ak_test_file_contains(daePath, "<reflective><color>"));
  ASSERT(ak_test_file_contains(daePath, "<reflectivity><float>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<index_of_refraction><float>1.45"));
  ASSERT(ak_test_file_contains(daePath,
                               "<instance_material symbol=\"mat_0\" target=\"#material_0\">"));
  ASSERT(ak_test_file_contains(daePath,
                               "<bind_vertex_input semantic=\"TEXCOORD0\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<bind_vertex_input semantic=\"TEXCOORD1\" input_semantic=\"TEXCOORD\" input_set=\"1\"/>"));
  ASSERT(ak_test_file_count(daePath, "<bind_vertex_input") == 2);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.images.count == 1);
  ASSERT(roundTrip->lib.materials.count == 1);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_material_skips_unused_images) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *usedImage;
  AkImage           *unusedImage;
  AkImageSource     *usedSource;
  AkImageSource     *unusedSource;
  const char        *outDir  = "./assetkit_export_dae_skip_unused_images";
  const char        *daePath = "./assetkit_export_dae_skip_unused_images/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  mat          = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface      = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor    = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref       = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture      = ak_heap_calloc(heap, doc, sizeof(*texture));
  usedImage    = ak_heap_calloc(heap, doc, sizeof(*usedImage));
  unusedImage  = ak_heap_calloc(heap, doc, sizeof(*unusedImage));
  usedSource   = ak_heap_calloc(heap, usedImage, sizeof(*usedSource));
  unusedSource = ak_heap_calloc(heap, unusedImage, sizeof(*unusedSource));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);
  ASSERT(usedImage != NULL);
  ASSERT(unusedImage != NULL);
  ASSERT(usedSource != NULL);
  ASSERT(unusedSource != NULL);

  usedSource->type    = AK_IMAGE_SOURCE_URI;
  usedSource->uri     = "data:image/png;base64,VVNFRA==";
  usedImage->name     = "used";
  usedImage->source   = usedSource;
  usedImage->next     = unusedImage;

  unusedSource->type  = AK_IMAGE_SOURCE_URI;
  unusedSource->uri   = "data:image/png;base64,VU5VU0VE";
  unusedImage->name   = "unused";
  unusedImage->source = unusedSource;

  texture->image       = usedImage;
  texref->texture      = texture;
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  surface->baseColor   = baseColor;
  mat->surface         = surface;

  doc->lib.images.first    = usedImage;
  doc->lib.images.last     = unusedImage;
  doc->lib.images.count    = 2;
  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_count(daePath, "<image ") == 1);
  ASSERT(ak_test_file_contains(daePath, "data:image/png;base64,VVNFRA=="));
  ASSERT(!ak_test_file_contains(daePath, "data:image/png;base64,VU5VU0VE"));
  ASSERT(ak_test_file_contains(daePath,
                               "<diffuse><texture texture=\"sampler_0\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_material_scalar_channels_smoke) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkMaterialInput   *opacity;
  AkMaterialInput   *emissive;
  AkMaterialSurface *roundSurface;
  const char        *outDir  = "./assetkit_export_dae_material_scalar";
  const char        *daePath = "./assetkit_export_dae_material_scalar/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  opacity   = ak_heap_calloc(heap, surface, sizeof(*opacity));
  emissive  = ak_heap_calloc(heap, surface, sizeof(*emissive));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(opacity != NULL);
  ASSERT(emissive != NULL);

  baseColor->source       = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.5f;
  baseColor->color.rgba.G = 0.25f;
  baseColor->color.rgba.B = 0.125f;
  baseColor->color.rgba.A = 1.0f;

  opacity->source       = AK_MATERIAL_INPUT_CONSTANT;
  opacity->valueType    = AK_MATERIAL_VALUE_FLOAT;
  opacity->value[0]     = 0.5f;

  emissive->source       = AK_MATERIAL_INPUT_CONSTANT;
  emissive->valueType    = AK_MATERIAL_VALUE_COLOR;
  emissive->color.rgba.R = 0.25f;
  emissive->color.rgba.G = 0.125f;
  emissive->color.rgba.B = 0.5f;
  emissive->color.rgba.A = 1.0f;

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->flags            = AK_MATERIAL_FLAG_ALPHA_BLEND;
  surface->baseColor        = baseColor;
  surface->opacity          = opacity;
  surface->emissive         = emissive;
  surface->emissiveStrength = 2.0f;
  mat->name                 = "scalar_mat";
  mat->surface              = surface;

  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<emission><color>0.5 0.25 1 1</color></emission>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<diffuse><color>0.5 0.25 0.125 0.5</color></diffuse>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<transparent opaque=\"A_ONE\"><color>1 1 1 0.5</color></transparent>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.materials.first != NULL);
  roundSurface = roundTrip->lib.materials.first->surface;
  ASSERT(roundSurface != NULL);
  ASSERT(fabsf(ak_materialOpacityFactor(roundSurface) - 0.5f) < 0.001f);
  ASSERT(roundSurface->emissive != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_material_technique_types) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkMaterial        *matConstant;
  AkMaterial        *matLambert;
  AkMaterial        *matBlinn;
  AkMaterialSurface *surfaceConstant;
  AkMaterialSurface *surfaceLambert;
  AkMaterialSurface *surfaceBlinn;
  AkMaterialInput   *colorConstant;
  AkMaterialInput   *colorLambert;
  AkMaterialInput   *colorBlinn;
  const char        *outDir  = "./assetkit_export_dae_material_technique";
  const char        *daePath = "./assetkit_export_dae_material_technique/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  matConstant     = ak_heap_calloc(heap, doc, sizeof(*matConstant));
  matLambert      = ak_heap_calloc(heap, doc, sizeof(*matLambert));
  matBlinn        = ak_heap_calloc(heap, doc, sizeof(*matBlinn));
  surfaceConstant = ak_heap_calloc(heap, matConstant, sizeof(*surfaceConstant));
  surfaceLambert  = ak_heap_calloc(heap, matLambert, sizeof(*surfaceLambert));
  surfaceBlinn    = ak_heap_calloc(heap, matBlinn, sizeof(*surfaceBlinn));
  colorConstant   = ak_heap_calloc(heap, surfaceConstant, sizeof(*colorConstant));
  colorLambert    = ak_heap_calloc(heap, surfaceLambert, sizeof(*colorLambert));
  colorBlinn      = ak_heap_calloc(heap, surfaceBlinn, sizeof(*colorBlinn));
  ASSERT(matConstant != NULL);
  ASSERT(matLambert != NULL);
  ASSERT(matBlinn != NULL);
  ASSERT(surfaceConstant != NULL);
  ASSERT(surfaceLambert != NULL);
  ASSERT(surfaceBlinn != NULL);
  ASSERT(colorConstant != NULL);
  ASSERT(colorLambert != NULL);
  ASSERT(colorBlinn != NULL);

  colorConstant->source       = AK_MATERIAL_INPUT_CONSTANT;
  colorConstant->valueType    = AK_MATERIAL_VALUE_COLOR;
  colorConstant->color.rgba.R = 1.0f;
  colorConstant->color.rgba.G = 0.0f;
  colorConstant->color.rgba.B = 0.0f;
  colorConstant->color.rgba.A = 1.0f;

  colorLambert->source       = AK_MATERIAL_INPUT_CONSTANT;
  colorLambert->valueType    = AK_MATERIAL_VALUE_COLOR;
  colorLambert->color.rgba.R = 0.0f;
  colorLambert->color.rgba.G = 1.0f;
  colorLambert->color.rgba.B = 0.0f;
  colorLambert->color.rgba.A = 1.0f;

  colorBlinn->source       = AK_MATERIAL_INPUT_CONSTANT;
  colorBlinn->valueType    = AK_MATERIAL_VALUE_COLOR;
  colorBlinn->color.rgba.R = 0.0f;
  colorBlinn->color.rgba.G = 0.0f;
  colorBlinn->color.rgba.B = 1.0f;
  colorBlinn->color.rgba.A = 1.0f;

  surfaceConstant->type      = AK_MATERIAL_TYPE_CONSTANT;
  surfaceConstant->baseColor = colorConstant;
  surfaceLambert->type       = AK_MATERIAL_TYPE_LAMBERT;
  surfaceLambert->baseColor  = colorLambert;
  surfaceBlinn->type         = AK_MATERIAL_TYPE_BLINN;
  surfaceBlinn->baseColor    = colorBlinn;

  matConstant->surface = surfaceConstant;
  matLambert->surface  = surfaceLambert;
  matBlinn->surface    = surfaceBlinn;
  matConstant->next    = matLambert;
  matLambert->next     = matBlinn;

  doc->lib.materials.first = matConstant;
  doc->lib.materials.last  = matBlinn;
  doc->lib.materials.count = 3;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<constant><emission><color>1 0 0 1</color></emission></constant>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<lambert><diffuse><color>0 1 0 1</color></diffuse></lambert>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<blinn><diffuse><color>0 0 1 1</color></diffuse></blinn>"));
  ASSERT(!ak_test_file_contains(daePath, "<constant><diffuse>"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_instance_material_override) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root, *node;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkInstanceGeometry *inst;
  AkMaterial         *primMat;
  AkMaterial         *instMat;
  AkMaterialBinding  *binding;
  const char         *outDir  = "./assetkit_export_dae_instance_material";
  const char         *daePath = "./assetkit_export_dae_instance_material/model.dae";
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

  primMat = ak_heap_calloc(heap, doc, sizeof(*primMat));
  instMat = ak_heap_calloc(heap, doc, sizeof(*instMat));
  binding = ak_heap_calloc(heap, node, sizeof(*binding));
  ASSERT(primMat != NULL);
  ASSERT(instMat != NULL);
  ASSERT(binding != NULL);

  primMat->name   = "primitive";
  instMat->name   = "instance";
  primMat->next   = instMat;
  prim->material  = primMat;

  binding->material      = instMat;
  binding->primitive     = prim;
  binding->scope         = AK_MATERIAL_BIND_OBJECT;
  binding->propertyIndex = UINT32_MAX;
  binding->variantIndex  = UINT32_MAX;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;
  doc->lib.materials.first  = primMat;
  doc->lib.materials.last   = instMat;
  doc->lib.materials.count  = 2;

  ak_addSubNode(root, node, false);
  inst = ak_nodeAttachGeometry(node, geom);
  ASSERT(inst != NULL);
  inst->objectBindings = binding;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<instance_material symbol=\"mat_0\" target=\"#material_1\">"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_rejects_unmapped_texture_image) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  const char        *outDir = "./assetkit_export_dae_unmapped_texture";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);
  ASSERT(image != NULL);

  texture->image       = image;
  texref->texture      = texture;
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  surface->baseColor   = baseColor;
  mat->surface         = surface;

  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_EINVAL);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_ignores_texture_ref_without_image) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  const char        *outDir  = "./assetkit_export_dae_null_texture_ref";
  const char        *daePath = "./assetkit_export_dae_null_texture_ref/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);

  texref->texture      = texture; /* no image: DAE init_as_null-style source */
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 0.25f;
  baseColor->color.rgba.G = 0.5f;
  baseColor->color.rgba.B = 0.75f;
  baseColor->color.rgba.A = 1.0f;
  surface->type        = AK_MATERIAL_TYPE_LAMBERT;
  surface->baseColor   = baseColor;
  mat->surface         = surface;

  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<library_effects>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<diffuse><color>0.25 0.5 0.75 1</color></diffuse>"));
  ASSERT(!ak_test_file_contains(daePath, "<texture texture="));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_rewrites_unsafe_texture_uri) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImage       *image;
  AkImageSource *source;
  const char    *outDir     = "./assetkit_export_dae_unsafe_uri";
  const char    *daePath    = "./assetkit_export_dae_unsafe_uri/model.dae";
  const char    *copiedPath = "./assetkit_export_dae_unsafe_uri/image_0_leak.png";
  const char    *sourceDir  = "./assetkit_export_dae_unsafe_uri_src";
  const char    *sourceTexDir = "./assetkit_export_dae_unsafe_uri_src/textures";
  const char    *sourcePath = "./assetkit_export_dae_unsafe_uri_src/leak.png";

  ak_test_export_cleanup(outDir);
  unlink(sourcePath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);

  {
    FILE *file;

    file = fopen(sourcePath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("LEAK", 1, 4, file) == 4);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceTexDir;

  image  = ak_heap_calloc(heap, doc, sizeof(*image));
  source = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "../leak.png";
  image->source = source;

  doc->lib.images.first = image;
  doc->lib.images.last  = image;
  doc->lib.images.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<init_from>image_0_leak.png</init_from>"));
  ASSERT(ak_test_file_contains(copiedPath, "LEAK"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourcePath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_texture_uri_collision) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImage       *authoredImage;
  AkImage       *generatedImage;
  AkImageSource *authoredSource;
  AkImageSource *generatedSource;
  const char    *outDir = "./assetkit_export_dae_image_uri_collision";
  const char    *daePath = "./assetkit_export_dae_image_uri_collision/model.dae";
  const char    *sourceDir = "./assetkit_export_dae_image_uri_collision_src";
  const char    *sourceTexDir =
    "./assetkit_export_dae_image_uri_collision_src/textures";
  const char    *sourceAuthoredPath =
    "./assetkit_export_dae_image_uri_collision_src/textures/image_1_WoodFile.PNG";
  const char    *sourceGeneratedPath =
    "./assetkit_export_dae_image_uri_collision_src/WoodFile.PNG";
  const char    *authoredOutPath =
    "./assetkit_export_dae_image_uri_collision/image_1_WoodFile.PNG";
  const char    *generatedOutPath =
    "./assetkit_export_dae_image_uri_collision/image_1_1_WoodFile.PNG";

  ak_test_export_cleanup(outDir);
  unlink(sourceAuthoredPath);
  unlink(sourceGeneratedPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);

  {
    FILE *file;

    file = fopen(sourceAuthoredPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("AUTH", 1, 4, file) == 4);
    ASSERT(fclose(file) == 0);

    file = fopen(sourceGeneratedPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("GEN", 1, 3, file) == 3);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceTexDir;

  authoredImage    = ak_heap_calloc(heap, doc, sizeof(*authoredImage));
  generatedImage   = ak_heap_calloc(heap, doc, sizeof(*generatedImage));
  authoredSource   = ak_heap_calloc(heap, authoredImage, sizeof(*authoredSource));
  generatedSource  = ak_heap_calloc(heap, generatedImage, sizeof(*generatedSource));
  ASSERT(authoredImage != NULL);
  ASSERT(generatedImage != NULL);
  ASSERT(authoredSource != NULL);
  ASSERT(generatedSource != NULL);

  authoredSource->type  = AK_IMAGE_SOURCE_URI;
  authoredSource->uri   = "image_1_WoodFile.PNG";
  authoredImage->source = authoredSource;
  authoredImage->next   = generatedImage;

  generatedSource->type  = AK_IMAGE_SOURCE_URI;
  generatedSource->uri   = "../WoodFile.PNG";
  generatedImage->source = generatedSource;

  doc->lib.images.first = authoredImage;
  doc->lib.images.last  = generatedImage;
  doc->lib.images.count = 2;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<init_from>image_1_WoodFile.PNG</init_from>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<init_from>image_1_1_WoodFile.PNG</init_from>"));
  ASSERT(ak_test_file_contains(authoredOutPath, "AUTH"));
  ASSERT(ak_test_file_contains(generatedOutPath, "GEN"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceAuthoredPath);
  unlink(sourceGeneratedPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_texture_file_uri) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImage       *image;
  AkImageSource *source;
  const char    *outDir = "./assetkit_export_dae_file_uri";
  const char    *daePath = "./assetkit_export_dae_file_uri/model.dae";
  const char    *sourceDir = "./assetkit_export_dae_file_uri_src";
  const char    *sourceTexPath = "./assetkit_export_dae_file_uri_src/WoodFile.PNG";
  const char    *copiedTexPath = "./assetkit_export_dae_file_uri/image_0_WoodFile.PNG";
  char           absTexPath[PATH_MAX];
  char           fileUri[PATH_MAX + 8u];

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

  image  = ak_heap_calloc(heap, doc, sizeof(*image));
  source = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = fileUri;
  image->source = source;

  doc->lib.images.first = image;
  doc->lib.images.last  = image;
  doc->lib.images.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<init_from>image_0_WoodFile.PNG</init_from>"));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_writes_buffer_image_source) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImage       *image;
  AkImageSource *source;
  AkBuffer      *buffer;
  const char    *outDir  = "./assetkit_export_dae_buffer_image";
  const char    *daePath = "./assetkit_export_dae_buffer_image/model.dae";
  const char    *imgPath = "./assetkit_export_dae_buffer_image/image_0.png";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  image  = ak_heap_calloc(heap, doc, sizeof(*image));
  source = ak_heap_calloc(heap, image, sizeof(*source));
  buffer = ak_heap_calloc(heap, source, sizeof(*buffer));
  ASSERT(image != NULL);
  ASSERT(source != NULL);
  ASSERT(buffer != NULL);

  buffer->data   = ak_heap_alloc(heap, buffer, 4);
  buffer->length = 4;
  ASSERT(buffer->data != NULL);
  memcpy(buffer->data, "IMG", 4);

  source->type   = AK_IMAGE_SOURCE_BUFFER;
  source->buffer = buffer;
  source->mimeType = "image/png";
  image->source  = source;

  doc->lib.images.first = image;
  doc->lib.images.last  = image;
  doc->lib.images.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<init_from>image_0.png</init_from>"));
  ASSERT(ak_test_file_contains(imgPath, "IMG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}
