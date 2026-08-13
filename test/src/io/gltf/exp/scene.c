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

TEST_IMPL(gltf_export_shared_accessor_prefix) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkInput           *texcoord;
  AkInput           *input;
  AkMaterial        *material;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  uint8_t           *indices;
  bool               foundTexcoord;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_shared_accessor_prefix");
  const float positions[18] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    2.0f, 0.0f, 0.0f,
    3.0f, 0.0f, 0.0f,
    2.0f, 1.0f, 0.0f
  };
  const float texcoords[12] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene         = ak_heap_calloc(heap, doc, sizeof(*scene));
  root          = ak_heap_calloc(heap, scene, sizeof(*root));
  node          = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node   = root;
  doc->scene    = scene;

  geom = ak_test_make_geom_with_positions(heap, doc, positions, 6u);
  ASSERT(geom != NULL);
  mesh = ak_objGet(geom->gdata);
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);

  texcoord = ak_heap_calloc(heap, prim, sizeof(*texcoord));
  ASSERT(texcoord != NULL);
  texcoord->semantic = AK_INPUT_TEXCOORD;
  texcoord->set      = 0u;
  texcoord->index    = 0u;
  texcoord->accessor = ak_test_make_float_accessor(heap,
                                                    texcoord,
                                                    texcoords,
                                                    2u,
                                                    6u);
  ASSERT(texcoord->accessor != NULL);
  texcoord->next   = prim->input;
  prim->input      = texcoord;
  prim->inputCount = 2u;

  prim->indices = ak_indexArrayAlloc(heap, prim, 3u, AKT_UBYTE);
  ASSERT(prim->indices != NULL);
  indices    = prim->indices->items;
  indices[0] = 0u;
  indices[1] = 1u;
  indices[2] = 2u;
  prim->indices->max = 2u;
  prim->indexStride  = 1u;

  material  = ak_heap_calloc(heap, doc, sizeof(*material));
  surface   = ak_heap_calloc(heap, material, sizeof(*surface));
  baseColor = ak_test_material_input(heap, surface);
  ASSERT(material != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  material->name       = "Prefix Material";
  material->surface    = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor   = baseColor;
  baseColor->source    = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.25f;
  baseColor->color.rgba.G = 0.5f;
  baseColor->color.rgba.B = 0.75f;
  baseColor->color.rgba.A = 1.0f;
  prim->material = material;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1u;
  doc->lib.materials.first  = material;
  doc->lib.materials.last   = material;
  doc->lib.materials.count  = 1u;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"material\":0"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 1u);
  ASSERT(roundTrip->lib.materials.count == 1u);
  geom = roundTrip->lib.geometries.first;
  ASSERT(geom != NULL);
  mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->material != NULL);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);
  ASSERT(prim->pos->accessor->count == 3u);

  foundTexcoord = false;
  for (input = prim->input; input; input = input->next) {
    if (input->semantic == AK_INPUT_TEXCOORD) {
      foundTexcoord = true;
      ASSERT(input->accessor != NULL);
      ASSERT(input->accessor->count == 3u);
      break;
    }
  }
  ASSERT(foundTexcoord);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_multiple_geometries_on_node) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geomA, *geomB;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_multiple_geometries_on_node");
  const float positionsA[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float positionsB[9] = {
    2.0f, 0.0f, 0.0f,
    3.0f, 0.0f, 0.0f,
    2.0f, 1.0f, 0.0f
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
  node->name    = "MultiMesh";

  geomA = ak_test_make_triangle_geom(heap, doc, positionsA);
  geomB = ak_test_make_triangle_geom(heap, doc, positionsB);
  geomA->name = "First";
  geomB->name = "Second";
  geomA->next = geomB;
  doc->lib.geometries.first = geomA;
  doc->lib.geometries.last  = geomB;
  doc->lib.geometries.count = 2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geomA) != NULL);
  ASSERT(ak_nodeAttachGeometry(node, geomB) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"name\":\"MultiMesh\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"name\":\"First\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"name\":\"Second\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 2);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_perspective_camera) {
  AkHeap   *heap;
  AkDoc    *doc;
  AkScene  *scene;
  AkNode   *root, *node;
  AkCamera *camera;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_perspective_camera");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);
  camera->name = "MainCamera";

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"cameras\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"camera\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"perspective\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"yfov\":0.785398185"));
  ASSERT(ak_test_file_contains(gltfPath, "\"znear\":0.100000001"));
  ASSERT(ak_test_file_contains(gltfPath, "\"zfar\":1000"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_perspective_camera_glb) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkCamera   *camera;
  struct stat stGlb;
  struct stat stBin;
  uint32_t    length;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_perspective_camera_glb");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);
  camera->name = "MainCamera";

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(ak_test_file_contains(glbPath, "\"cameras\":["));
  ASSERT(ak_test_file_contains(glbPath, "\"camera\":0"));
  ASSERT(ak_test_file_contains(glbPath, "\"type\":\"perspective\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_camera_extras) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkCamera   *camera;
  AkTreeNode *extra;
  AkTreeNode *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_camera_extras");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  extra       = ak_heap_calloc(heap, node, sizeof(*extra));
  note        = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "cameraNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(camera, extra);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"cameraNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_punctual_light) {
  AkHeap  *heap;
  AkDoc   *doc;
  AkScene *scene;
  AkNode  *root, *node;
  AkLight *light;
  AkLightBase *base;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_punctual_light");

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

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_POINT);
  ASSERT(light != NULL);
  light->name = "PointLight";
  base = light->data;
  ASSERT(base != NULL);
  base->color.rgba.R = 0.25f;
  base->color.rgba.G = 0.5f;
  base->color.rgba.B = 0.75f;
  base->intensity    = 4.0f;
  base->range        = 12.0f;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"KHR_lights_punctual\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_lights_punctual\":{\"lights\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"point\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"color\":[0.25,0.5,0.75]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"intensity\":4"));
  ASSERT(ak_test_file_contains(gltfPath, "\"range\":12"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_lights_punctual\":{\"light\":0}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_light_extras) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkLight    *light;
  AkTreeNode *extra;
  AkTreeNode *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_light_extras");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  extra       = ak_heap_calloc(heap, node, sizeof(*extra));
  note        = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_POINT);
  ASSERT(light != NULL);

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "lightNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(light, extra);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"lightNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_punctual_light_glb) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  AkNode      *root, *node;
  AkLight     *light;
  AkLightBase *base;
  AkResolvedLight resolved;
  struct stat  stGlb;
  struct stat  stBin;
  uint32_t     length;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_punctual_light_glb");

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

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_POINT);
  ASSERT(light != NULL);
  light->name = "PointLight";
  base = light->data;
  ASSERT(base != NULL);
  base->color.rgba.R = 0.25f;
  base->color.rgba.G = 0.5f;
  base->color.rgba.B = 0.75f;
  base->intensity    = 4.0f;
  base->range        = 12.0f;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(ak_test_file_contains(glbPath, "\"KHR_lights_punctual\""));
  ASSERT(ak_test_file_contains(glbPath, "\"type\":\"point\""));
  ASSERT(ak_test_file_contains(glbPath, "\"light\":0"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->lib.lights.first != NULL);
  light = roundTrip->lib.lights.first;
  ASSERT(ak_lightResolve(roundTrip,
                         light,
                         AK_LIGHT_RESOLVE_RAW,
                         &resolved));
  ASSERT(fabsf(resolved.attenuation.constant) < 0.000001f);
  ASSERT(fabsf(resolved.attenuation.linear) < 0.000001f);
  ASSERT(fabsf(resolved.attenuation.quadratic - 1.0f) < 0.000001f);
  ASSERT(fabsf(resolved.attenuationFalloffExponent - 2.0f) < 0.000001f);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_invalid_camera) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkCamera      *camera;
  AkPerspective *persp;
  struct stat    stFile;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_invalid_camera");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);
  persp = (AkPerspective *)camera->optics->proj;
  persp->zfar = INFINITY;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stFile) == 0);
  ASSERT(!ak_test_file_contains(gltfPath, "\"cameras\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"camera\":"));
  ASSERT(stat(binPath, &stFile) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_invalid_punctual_light) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkLight    *light;
  AkSpotLight *spot;
  struct stat stFile;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_invalid_punctual_light");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_SPOT);
  ASSERT(light != NULL);
  spot = (AkSpotLight *)light->data;
  ASSERT(spot != NULL);
  spot->innerConeAngle = spot->outerConeAngle;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stFile) == 0);
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_lights_punctual\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"light\":"));
  ASSERT(stat(binPath, &stFile) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_visibility) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_node_visibility");

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
  node->visible = false;
  node->name    = "hidden";

  ak_addSubNode(root, node, false);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_node_visibility\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_node_visibility\":{\"visible\":false}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_extras) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkTreeNode *extra;
  AkTreeNode *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_node_extras");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  extra       = ak_heap_calloc(heap, node, sizeof(*extra));
  note        = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;
  node->name    = "extraNode";

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "authorNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(node, extra);

  ak_addSubNode(root, node, false);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"authorNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_gpu_instancing) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkGpuInstancing *instancing;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_gpu_instancing");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float translations[6] = {
    0.0f, 0.0f, 0.0f,
    2.0f, 0.0f, 0.0f
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
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  instancing              = ak_heap_calloc(heap, node, sizeof(*instancing));
  instancing->translation = ak_test_make_float_accessor(heap,
                                                        instancing,
                                                        translations,
                                                        3,
                                                        2);
  instancing->count       = 2;
  node->gpuInstancing     = instancing;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"EXT_mesh_gpu_instancing\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"EXT_mesh_gpu_instancing\":{\"attributes\":{\"TRANSLATION\":1}}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_mesh_quantization) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkAccessor      *posAcc;
  AkBuffer        *posBuff;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_mesh_quantization");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const int16_t quantized[9] = {
    0, 0, 0,
    1000, 0, 0,
    0, 1000, 0
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
  posAcc = prim->pos->accessor;
  posBuff = ak_heap_calloc(heap, posAcc, sizeof(*posBuff));
  posBuff->length = sizeof(quantized);
  posBuff->data   = ak_heap_alloc(heap, posBuff, posBuff->length);
  memcpy(posBuff->data, quantized, sizeof(quantized));

  posAcc->buffer                = posBuff;
  posAcc->byteLength            = posBuff->length;
  posAcc->byteStride            = sizeof(int16_t) * 3;
  posAcc->fillByteSize          = sizeof(int16_t) * 3;
  posAcc->bytesPerComponent     = sizeof(int16_t);
  posAcc->componentType         = AKT_SHORT;
  posAcc->originalComponentType = AKT_SHORT;
  posAcc->normalized            = false;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"KHR_mesh_quantization\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"KHR_mesh_quantization\"]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5122"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}
