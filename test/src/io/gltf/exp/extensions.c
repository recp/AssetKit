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

TEST_IMPL(gltf_export_material_extensions) {
  AkHeap                     *heap;
  AkDoc                      *doc;
  AkDoc                      *roundTrip;
  AkScene                    *scene;
  AkNode                     *root, *node;
  AkGeometry                 *geom;
  AkMesh                     *mesh;
  AkMeshPrimitive            *prim;
  AkMaterial                 *mat;
  AkMaterialSurface          *surface;
  AkMaterialClearcoatFeature *clearcoat;
  AkMaterialSpecularFeature  *specular;
  AkMaterialInput            *clearcoatFactor;
  AkMaterialInput            *specularColor;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_ext");
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

  mat             = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface         = ak_heap_calloc(heap, mat, sizeof(*surface));
  clearcoat       = ak_heap_calloc(heap, surface, sizeof(*clearcoat));
  specular        = ak_heap_calloc(heap, surface, sizeof(*specular));
  clearcoatFactor = ak_heap_calloc(heap, clearcoat, sizeof(*clearcoatFactor));
  specularColor   = ak_heap_calloc(heap, specular, sizeof(*specularColor));

  mat->surface                = surface;
  surface->type               = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior                = 1.33f;
  surface->emissiveStrength   = 2.0f;
  surface->features           = &clearcoat->base;
  clearcoat->base.next        = &specular->base;
  clearcoat->base.type        = AK_MATERIAL_FEATURE_CLEARCOAT;
  clearcoat->factor           = clearcoatFactor;
  specular->base.type         = AK_MATERIAL_FEATURE_SPECULAR;
  specular->color             = specularColor;
  clearcoatFactor->source     = AK_MATERIAL_INPUT_CONSTANT;
  clearcoatFactor->valueType  = AK_MATERIAL_VALUE_FLOAT;
  clearcoatFactor->value[0]   = 0.7f;
  specularColor->source       = AK_MATERIAL_INPUT_CONSTANT;
  specularColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  specularColor->color.vec[0] = 0.5f;
  specularColor->color.vec[1] = 0.6f;
  specularColor->color.vec[2] = 0.7f;
  specularColor->color.vec[3] = 1.0f;
  prim->material              = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_materials_unlit\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_materials_emissive_strength\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"emissiveStrength\":2"));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_ior\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"ior\":1.33000004"));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_clearcoat\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"clearcoatFactor\":0.699999988"));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_specular\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"specularColorFactor\":[0.5,0.600000024,0.699999988]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_specular_glossiness) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkMaterial      *mat;
  AkMaterialSurface *surface;
  AkMaterialSpecularGlossinessFeature *sg;
  AkMaterialInput *diffuse;
  AkMaterialInput *specular;
  AkMaterialInput *glossiness;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_specgloss");
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
  sg        = ak_heap_calloc(heap, surface, sizeof(*sg));
  diffuse   = ak_heap_calloc(heap, sg, sizeof(*diffuse));
  specular  = ak_heap_calloc(heap, sg, sizeof(*specular));
  glossiness = ak_heap_calloc(heap, sg, sizeof(*glossiness));

  surface->type             = AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->features         = (AkMaterialFeature *)sg;
  sg->base.type             = AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS;
  sg->diffuse               = diffuse;
  sg->specular              = specular;
  sg->glossiness            = glossiness;

  diffuse->source      = AK_MATERIAL_INPUT_CONSTANT;
  diffuse->valueType   = AK_MATERIAL_VALUE_COLOR;
  diffuse->color.rgba.R = 0.2f;
  diffuse->color.rgba.G = 0.3f;
  diffuse->color.rgba.B = 0.4f;
  diffuse->color.rgba.A = 0.5f;

  specular->source      = AK_MATERIAL_INPUT_CONSTANT;
  specular->valueType   = AK_MATERIAL_VALUE_COLOR;
  specular->color.rgba.R = 0.6f;
  specular->color.rgba.G = 0.7f;
  specular->color.rgba.B = 0.8f;
  specular->color.rgba.A = 1.0f;

  glossiness->source    = AK_MATERIAL_INPUT_CONSTANT;
  glossiness->valueType = AK_MATERIAL_VALUE_FLOAT;
  glossiness->value[0]  = 0.9f;

  mat->surface = surface;
  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_materials_pbrSpecularGlossiness\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"diffuseFactor\":[0.200000003,0.300000012,0.400000006,0.5]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"specularFactor\":[0.600000024,0.699999988,0.800000012]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"glossinessFactor\":0.899999976"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_volume_scatter) {
  AkHeap                     *heap;
  AkDoc                      *doc;
  AkDoc                      *roundTrip;
  AkScene                    *scene;
  AkNode                     *root, *node;
  AkGeometry                 *geom;
  AkMesh                     *mesh;
  AkMeshPrimitive            *prim;
  AkMaterial                 *mat;
  AkMaterialSurface          *surface;
  AkMaterialSubsurfaceFeature *subsurface;
  AkMaterialInput            *color;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_volscatter");
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

  mat        = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface    = ak_heap_calloc(heap, mat, sizeof(*surface));
  subsurface = ak_heap_calloc(heap, surface, sizeof(*subsurface));
  color      = ak_heap_calloc(heap, subsurface, sizeof(*color));

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->features         = &subsurface->base;
  subsurface->base.type     = AK_MATERIAL_FEATURE_SUBSURFACE;
  subsurface->color         = color;
  subsurface->anisotropy    = 0.125f;

  color->source       = AK_MATERIAL_INPUT_CONSTANT;
  color->valueType    = AK_MATERIAL_VALUE_COLOR;
  color->color.vec[0] = 0.25f;
  color->color.vec[1] = 0.5f;
  color->color.vec[2] = 0.75f;
  color->color.vec[3] = 1.0f;

  mat->surface   = surface;
  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_materials_volume_scatter\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"multiscatterColorFactor\":[0.25,0.5,0.75]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"scatterAnisotropy\":0.125"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_variants) {
  AkHeap                   *heap;
  AkDoc                    *doc;
  AkDoc                    *roundTrip;
  AkScene                  *scene;
  AkNode                   *root, *node;
  AkGeometry               *geom;
  AkMesh                   *mesh;
  AkMeshPrimitive          *prim;
  AkMaterial               *baseMat;
  AkMaterial               *redMat;
  AkMaterial               *blueMat;
  AkMaterial               *greenMat;
  AkMaterialVariant        *redVariant;
  AkMaterialVariant        *blueVariant;
  AkMaterialVariant        *greenVariant;
  AkMaterialVariantMapping *redMapping;
  AkMaterialVariantMapping *blueMapping;
  AkMaterialVariantMapping *greenMapping;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_variants");
  const char *roundDir      = "./assetkit_export_material_variants_round";
  const char *roundGltfPath = "./assetkit_export_material_variants_round/model.gltf";
  const char *roundBinPath  = "./assetkit_export_material_variants_round/model.bin";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
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

  redVariant   = ak_heap_calloc(heap, doc, sizeof(*redVariant));
  blueVariant  = ak_heap_calloc(heap, doc, sizeof(*blueVariant));
  greenVariant = ak_heap_calloc(heap, doc, sizeof(*greenVariant));
  redVariant->name       = "red";
  redVariant->next       = blueVariant;
  blueVariant->name      = "blue";
  blueVariant->next      = greenVariant;
  greenVariant->name     = "green";
  doc->materialVariants  = redVariant;
  doc->materialVariantCount = 0;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  baseMat      = ak_heap_calloc(heap, doc, sizeof(*baseMat));
  redMat       = ak_heap_calloc(heap, doc, sizeof(*redMat));
  blueMat      = ak_heap_calloc(heap, doc, sizeof(*blueMat));
  greenMat     = ak_heap_calloc(heap, doc, sizeof(*greenMat));
  redMapping   = ak_heap_calloc(heap, prim, sizeof(*redMapping));
  blueMapping  = ak_heap_calloc(heap, prim, sizeof(*blueMapping));
  greenMapping = ak_heap_calloc(heap, prim, sizeof(*greenMapping));
  baseMat->name  = "base";
  redMat->name   = "redMat";
  blueMat->name  = "blueMat";
  greenMat->name = "greenMat";
  prim->material = baseMat;

  redMapping->material     = redMat;
  redMapping->variantIndex = 0;
  redMapping->next         = blueMapping;
  blueMapping->material     = blueMat;
  blueMapping->variantIndex = 1;
  blueMapping->next         = greenMapping;
  greenMapping->material     = greenMat;
  greenMapping->variantIndex = 2;
  prim->variantMappings      = redMapping;
  prim->variantMappingCount  = 3;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_variants\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"variants\":[{\"name\":\"red\"},{\"name\":\"blue\"},{\"name\":\"green\"}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"mappings\":[{\"material\":1,\"variants\":[0]},{\"material\":2,\"variants\":[1]},{\"material\":3,\"variants\":[2]}]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->materialVariantCount == 3);
  ASSERT(ak_export(roundTrip, roundDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_files_equal(gltfPath, roundGltfPath));
  ASSERT(ak_test_files_equal(binPath, roundBinPath));
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_root_xmp_extension) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkTreeNode     *extra;
  AkTreeNode     *extensions;
  AkTreeNode     *xmp;
  AkTreeNode     *packets;
  AkTreeNode     *packet;
  AkTreeNode     *xml;
  AkTreeNodeAttr *arrayAttr;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_root_xmp");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  extra      = ak_heap_calloc(heap, doc, sizeof(*extra));
  extensions = ak_heap_calloc(heap, extra, sizeof(*extensions));
  xmp        = ak_heap_calloc(heap, extensions, sizeof(*xmp));
  packets    = ak_heap_calloc(heap, xmp, sizeof(*packets));
  packet     = ak_heap_calloc(heap, packets, sizeof(*packet));
  xml        = ak_heap_calloc(heap, packet, sizeof(*xml));
  arrayAttr  = ak_heap_calloc(heap, packets, sizeof(*arrayAttr));
  ASSERT(extra != NULL);
  ASSERT(extensions != NULL);
  ASSERT(xmp != NULL);
  ASSERT(packets != NULL);
  ASSERT(packet != NULL);
  ASSERT(xml != NULL);
  ASSERT(arrayAttr != NULL);

  extra->name      = "root";
  extra->chld      = extensions;
  extra->chldc     = 1;
  extensions->name = "extensions";
  extensions->parent = extra;
  extensions->chld   = xmp;
  extensions->chldc  = 1;
  xmp->name       = "KHR_xmp";
  xmp->parent     = extensions;
  xmp->chld       = packets;
  xmp->chldc      = 1;
  packets->name   = "packets";
  packets->parent = xmp;
  packets->attribs = arrayAttr;
  packets->attrc   = 1;
  packets->chld    = packet;
  packets->chldc   = 1;
  arrayAttr->name  = "type";
  arrayAttr->val   = (char *)"array";
  packet->parent   = packets;
  packet->chld     = xml;
  packet->chldc    = 1;
  xml->name        = "xml";
  xml->val         = (char *)"<x:xmpmeta/>";
  xml->parent      = packet;

  ak_extra_set(doc, extra);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_xmp\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsUsed\":[\"KHR_xmp\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"packets\":[{\"xml\":\"<x:xmpmeta/>\"}]"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"extensionsRequired\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_preserved_root_extension) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkTreeNode     *extra;
  AkTreeNode     *extensions;
  AkTreeNode     *required;
  AkTreeNode     *requiredItem;
  AkTreeNode     *ibl;
  AkTreeNode     *lights;
  AkTreeNode     *light;
  AkTreeNode     *intensity;
  AkTreeNodeAttr *lightsArrayAttr;
  AkTreeNodeAttr *requiredArrayAttr;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_preserved_root_extension");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  extra             = ak_heap_calloc(heap, doc, sizeof(*extra));
  extensions        = ak_heap_calloc(heap, extra, sizeof(*extensions));
  required          = ak_heap_calloc(heap, extra, sizeof(*required));
  requiredItem      = ak_heap_calloc(heap, required, sizeof(*requiredItem));
  ibl               = ak_heap_calloc(heap, extensions, sizeof(*ibl));
  lights            = ak_heap_calloc(heap, ibl, sizeof(*lights));
  light             = ak_heap_calloc(heap, lights, sizeof(*light));
  intensity         = ak_heap_calloc(heap, light, sizeof(*intensity));
  lightsArrayAttr   = ak_heap_calloc(heap, lights, sizeof(*lightsArrayAttr));
  requiredArrayAttr = ak_heap_calloc(heap, required, sizeof(*requiredArrayAttr));
  ASSERT(extra != NULL);
  ASSERT(extensions != NULL);
  ASSERT(required != NULL);
  ASSERT(requiredItem != NULL);
  ASSERT(ibl != NULL);
  ASSERT(lights != NULL);
  ASSERT(light != NULL);
  ASSERT(intensity != NULL);
  ASSERT(lightsArrayAttr != NULL);
  ASSERT(requiredArrayAttr != NULL);

  extra->name       = "root";
  extra->chld       = extensions;
  extra->chldc      = 2;
  extensions->name  = "extensions";
  extensions->parent = extra;
  extensions->next  = required;
  extensions->chld  = ibl;
  extensions->chldc = 1;
  required->name    = "extensionsRequired";
  required->parent  = extra;
  required->attribs = requiredArrayAttr;
  required->attrc   = 1;
  required->chld    = requiredItem;
  required->chldc   = 1;
  requiredArrayAttr->name = "type";
  requiredArrayAttr->val  = (char *)"array";
  requiredItem->parent = required;
  requiredItem->val    = (char *)"EXT_lights_image_based";

  ibl->name       = "EXT_lights_image_based";
  ibl->parent     = extensions;
  ibl->chld       = lights;
  ibl->chldc      = 1;
  lights->name    = "lights";
  lights->parent  = ibl;
  lights->attribs = lightsArrayAttr;
  lights->attrc   = 1;
  lights->chld    = light;
  lights->chldc   = 1;
  lightsArrayAttr->name = "type";
  lightsArrayAttr->val  = (char *)"array";
  light->parent    = lights;
  light->chld      = intensity;
  light->chldc     = 1;
  intensity->name  = "intensity";
  intensity->val   = (char *)"2";
  intensity->parent = light;

  ak_extra_set(doc, extra);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"EXT_lights_image_based\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"EXT_lights_image_based\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_lights_image_based\":{\"lights\":[{\"intensity\":2}]}}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_preserved_object_extension) {
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
  AkTreeNode        *docExtra;
  AkTreeNode        *required;
  AkTreeNode        *requiredItem;
  AkTreeNodeAttr    *requiredArrayAttr;
  AkTreeNode        *matExtra;
  AkTreeNode        *extensions;
  AkTreeNode        *metadata;
  AkTreeNode        *level;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_preserved_object_extension");
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
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);

  mat->name         = "preservedExtMat";
  mat->surface      = surface;
  surface->type     = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  surface->emissiveStrength = 1.0f;
  surface->ior      = 1.5f;
  baseColor->source = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.4f;
  baseColor->color.rgba.G = 0.5f;
  baseColor->color.rgba.B = 0.6f;
  baseColor->color.rgba.A = 1.0f;

  matExtra   = ak_heap_calloc(heap, mat, sizeof(*matExtra));
  extensions = ak_heap_calloc(heap, matExtra, sizeof(*extensions));
  metadata   = ak_heap_calloc(heap, extensions, sizeof(*metadata));
  level      = ak_heap_calloc(heap, metadata, sizeof(*level));
  ASSERT(matExtra != NULL);
  ASSERT(extensions != NULL);
  ASSERT(metadata != NULL);
  ASSERT(level != NULL);

  matExtra->name      = "root";
  matExtra->chld      = extensions;
  matExtra->chldc     = 1;
  extensions->name    = "extensions";
  extensions->parent  = matExtra;
  extensions->chld    = metadata;
  extensions->chldc   = 1;
  metadata->name      = "AGI_stk_metadata";
  metadata->parent    = extensions;
  metadata->chld      = level;
  metadata->chldc     = 1;
  level->name         = "level";
  level->val          = (char *)"7";
  level->parent       = metadata;
  ak_extra_set(mat, matExtra);

  docExtra          = ak_heap_calloc(heap, doc, sizeof(*docExtra));
  required          = ak_heap_calloc(heap, docExtra, sizeof(*required));
  requiredItem      = ak_heap_calloc(heap, required, sizeof(*requiredItem));
  requiredArrayAttr = ak_heap_calloc(heap, required, sizeof(*requiredArrayAttr));
  ASSERT(docExtra != NULL);
  ASSERT(required != NULL);
  ASSERT(requiredItem != NULL);
  ASSERT(requiredArrayAttr != NULL);

  docExtra->name          = "root";
  docExtra->chld          = required;
  docExtra->chldc         = 1;
  required->name          = "extensionsRequired";
  required->parent        = docExtra;
  required->attribs       = requiredArrayAttr;
  required->attrc         = 1;
  required->chld          = requiredItem;
  required->chldc         = 1;
  requiredArrayAttr->name = "type";
  requiredArrayAttr->val  = (char *)"array";
  requiredItem->parent    = required;
  requiredItem->val       = (char *)"AGI_stk_metadata";
  ak_extra_set(doc, docExtra);

  prim->material = mat;
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"AGI_stk_metadata\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"AGI_stk_metadata\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"AGI_stk_metadata\":{\"level\":7}}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_preserved_object_extensions_across_kinds) {
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
  const char *requiredNames[] = {
    "EXT_scene_object",
    "EXT_node_object",
    "EXT_mesh_object",
    "EXT_primitive_object",
    "EXT_texinfo_object",
    "EXT_texture_object",
    "EXT_image_object",
    "EXT_sampler_object"
  };
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_preserved_object_ext_kinds");
  const char *sourceDir     = "./assetkit_export_preserved_object_ext_src";
  const char *sourceTexDir  = "./assetkit_export_preserved_object_ext_src/textures";
  const char *sourceTexPath = "./assetkit_export_preserved_object_ext_src/textures/Ext.PNG";
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
  source->uri    = "textures/Ext.PNG";
  image->source  = source;
  texture->image = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;
  ak_setypeid(texref, AKT_TEXTURE_REF);
  ak_setypeid(texture, AKT_TEXTURE);
  ak_setypeid(sampler, AKT_SAMPLER2D);

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
  surface->emissiveStrength = 1.0f;
  surface->ior       = 1.5f;
  prim->material     = mat;

  ak_extra_set(doc, ak_test_extra_required_extensions(
                 heap, doc, requiredNames, (uint32_t)AK_ARRAY_LEN(requiredNames)));
  ak_extra_set(scene, ak_test_extra_extension_pair(
                 heap, doc, "EXT_scene_object", "scope", "scene"));
  ak_extra_set(node, ak_test_extra_extension_pair(
                 heap, doc, "EXT_node_object", "scope", "node"));
  mesh->extra = ak_test_extra_extension_pair(
    heap, doc, "EXT_mesh_object", "scope", "mesh");
  prim->extra = ak_test_extra_extension_pair(
    heap, doc, "EXT_primitive_object", "scope", "primitive");
  ak_extra_set(texref, ak_test_extra_extension_pair(
                 heap, doc, "EXT_texinfo_object", "scope", "texinfo"));
  ak_extra_set(texture, ak_test_extra_extension_pair(
                 heap, doc, "EXT_texture_object", "scope", "texture"));
  ak_extra_set(image, ak_test_extra_extension_pair(
                 heap, doc, "EXT_image_object", "scope", "image"));
  ak_extra_set(sampler, ak_test_extra_extension_pair(
                 heap, doc, "EXT_sampler_object", "scope", "sampler"));

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsUsed\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsRequired\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"material\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_materials_emissive_strength\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_scene_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_node_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_mesh_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_primitive_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_texinfo_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_texture_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_image_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_sampler_object\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_scene_object\":{\"scope\":\"scene\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_node_object\":{\"scope\":\"node\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_mesh_object\":{\"scope\":\"mesh\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_primitive_object\":{\"scope\":\"primitive\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"extensions\":{\"EXT_texinfo_object\":{\"scope\":\"texinfo\"}}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_texture_object\":{\"scope\":\"texture\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_image_object\":{\"scope\":\"image\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_sampler_object\":{\"scope\":\"sampler\"}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_omits_unused_material_variants) {
  AkHeap                   *heap;
  AkDoc                    *doc;
  AkScene                  *scene;
  AkNode                   *root, *node;
  AkGeometry               *geom;
  AkMesh                   *mesh;
  AkMeshPrimitive          *prim;
  AkMaterial               *noopMat;
  AkMaterial               *badSlotMat;
  AkMaterialSurface        *noopSurface;
  AkMaterialSurface        *badSlotSurface;
  AkMaterialInput          *baseColor;
  AkTextureRef             *texref;
  AkTexture                *texture;
  AkImage                  *image;
  AkImageSource            *source;
  AkMaterialVariant        *variant;
  AkMaterialVariantMapping *mapping;
  AkMaterialVariantMapping *badSlotMapping;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_unused_material_variants");
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

  variant = ak_heap_calloc(heap, doc, sizeof(*variant));
  variant->name = "unused";
  doc->materialVariants = variant;
  doc->materialVariantCount = 1;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  noopMat = ak_heap_calloc(heap, doc, sizeof(*noopMat));
  noopSurface = ak_heap_calloc(heap, noopMat, sizeof(*noopSurface));
  noopSurface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  noopSurface->alphaCutoff      = 0.5f;
  noopSurface->ior              = 1.5f;
  noopSurface->emissiveStrength = 1.0f;
  noopMat->surface              = noopSurface;

  badSlotMat     = ak_heap_calloc(heap, doc, sizeof(*badSlotMat));
  badSlotSurface = ak_heap_calloc(heap, badSlotMat, sizeof(*badSlotSurface));
  baseColor      = ak_heap_calloc(heap, badSlotSurface, sizeof(*baseColor));
  texref         = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture        = ak_heap_calloc(heap, doc, sizeof(*texture));
  image          = ak_heap_calloc(heap, doc, sizeof(*image));
  source         = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "data:image/png;base64,QUJD";
  image->source = source;
  texture->image = image;
  texref->texture = texture;
  texref->slot = 9;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  badSlotSurface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  badSlotSurface->alphaCutoff      = 0.5f;
  badSlotSurface->ior              = 1.5f;
  badSlotSurface->emissiveStrength = 1.0f;
  badSlotSurface->baseColor        = baseColor;
  badSlotMat->name                 = "badSlot";
  badSlotMat->surface              = badSlotSurface;

  mapping                      = ak_heap_calloc(heap, prim, sizeof(*mapping));
  badSlotMapping               = ak_heap_calloc(heap, prim, sizeof(*badSlotMapping));
  mapping->material            = noopMat;
  mapping->variantIndex        = 0;
  mapping->next                = badSlotMapping;
  badSlotMapping->material     = badSlotMat;
  badSlotMapping->variantIndex = 0;
  prim->variantMappings        = mapping;
  prim->variantMappingCount    = 2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_materials_variants\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}
