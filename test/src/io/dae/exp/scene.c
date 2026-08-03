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

#include <ak/version.h>

TEST_IMPL(dae_export_instance_node_reuse_smoke) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *shared;
  AkNode         *roundRoot;
  AkInstanceNode *useA, *useB;
  uint32_t        rootCount;
  struct stat     stDae;
  const char     *outDir  = "./assetkit_export_dae_instance_node";
  const char     *daePath = "./assetkit_export_dae_instance_node/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  shared      = ak_heap_calloc(heap, doc, sizeof(*shared));
  shared->name = "Shared";
  scene->node = root;
  doc->scene  = scene;

  doc->lib.nodes.first = shared;
  doc->lib.nodes.last  = shared;
  doc->lib.nodes.count = 1;

  useA = ak_nodeAttachNodeInstance(root, shared);
  useB = ak_nodeAttachNodeInstance(root, shared);
  ASSERT(useA != NULL);
  ASSERT(useB != NULL);
  useA->name = "UseA";
  useB->name = "UseB";

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath, "<library_nodes>"));
  ASSERT(ak_test_file_contains(daePath, "<node id=\"node_0\" name=\"Shared\">"));
  ASSERT(ak_test_file_count(daePath, "<instance_node") == 2);
  ASSERT(ak_test_file_contains(daePath,
                               "<instance_node name=\"UseA\" url=\"#node_0\"/>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<instance_node name=\"UseB\" url=\"#node_0\"/>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.nodes.count >= 3);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);
  rootCount = 0;
  for (roundRoot = roundTrip->scene->node->chld;
       roundRoot;
       roundRoot = roundRoot->next) {
    ASSERT(roundRoot->node != NULL);
    ASSERT(roundRoot->node->target != NULL);
    ASSERT(roundRoot->node->target->name != NULL);
    ASSERT(strcmp(roundRoot->node->target->name, "Shared") == 0);
    rootCount++;
  }
  ASSERT(rootCount == 2);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_scene_child_instance_target_keeps_node_id) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *owner, *target;
  AkGeometry     *geom;
  AkInstanceNode *use;
  struct stat     stDae;
  const char     *outDir  = "./assetkit_export_dae_scene_child_ref";
  const char     *daePath = "./assetkit_export_dae_scene_child_ref/model.dae";
  const float     positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene        = ak_heap_calloc(heap, doc, sizeof(*scene));
  root         = ak_heap_calloc(heap, scene, sizeof(*root));
  owner        = ak_heap_calloc(heap, doc, sizeof(*owner));
  target       = ak_heap_calloc(heap, doc, sizeof(*target));
  owner->name  = "Owner";
  target->name = "DirectShared";
  scene->node  = root;
  doc->scene   = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;
  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);

  doc->lib.nodes.first = target;
  doc->lib.nodes.last  = target;
  doc->lib.nodes.count = 1;

  ak_addSubNode(root, target, false);
  ak_addSubNode(root, owner, false);
  use = ak_nodeAttachNodeInstance(owner, target);
  ASSERT(use != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(!ak_test_file_contains(daePath, "<library_nodes>"));
  ASSERT(ak_test_file_count(daePath, "id=\"node_0\"") == 1);
  ASSERT(ak_test_file_contains(daePath,
                               "<node id=\"node_0\" name=\"DirectShared\">"));
  ASSERT(ak_test_file_contains(daePath,
                               "<instance_node url=\"#node_0\"/>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_active_scene_outside_library_is_written) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *libraryScene;
  AkScene    *activeScene;
  AkNode     *libraryRoot;
  AkNode     *activeRoot;
  AkNode     *activeNode;
  AkGeometry *geom;
  struct stat stDae;
  const char *outDir  = "./assetkit_export_dae_active_extra_scene";
  const char *daePath = "./assetkit_export_dae_active_extra_scene/ActiveScene.dae";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  libraryScene       = ak_heap_calloc(heap, doc, sizeof(*libraryScene));
  activeScene        = ak_heap_calloc(heap, doc, sizeof(*activeScene));
  libraryRoot        = ak_heap_calloc(heap, libraryScene, sizeof(*libraryRoot));
  activeRoot         = ak_heap_calloc(heap, activeScene, sizeof(*activeRoot));
  activeNode         = ak_heap_calloc(heap, doc, sizeof(*activeNode));
  libraryScene->name = "LibraryScene";
  activeScene->name  = "ActiveScene";
  activeNode->name   = "ActiveNode";
  libraryScene->node = libraryRoot;
  activeScene->node  = activeRoot;
  doc->scene         = activeScene;

  doc->lib.scenes.first = libraryScene;
  doc->lib.scenes.last  = libraryScene;
  doc->lib.scenes.count = 1;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(activeRoot, activeNode, false);
  ASSERT(ak_nodeAttachGeometry(activeNode, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath,
                               "<visual_scene id=\"scene_0\" name=\"LibraryScene\">"));
  ASSERT(ak_test_file_contains(daePath,
                               "<visual_scene id=\"scene_1\" name=\"ActiveScene\">"));
  ASSERT(ak_test_file_contains(daePath,
                               "<scene><instance_visual_scene url=\"#scene_1\"/>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<node id=\"vnode_"));
  ASSERT(ak_test_file_contains(daePath, "name=\"ActiveNode\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->name != NULL);
  ASSERT(strcmp(roundTrip->scene->name, "ActiveScene") == 0);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);
  ASSERT(roundTrip->scene->node->chld->name != NULL);
  ASSERT(strcmp(roundTrip->scene->node->chld->name, "ActiveNode") == 0);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_camera_light_smoke) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  AkNode      *root, *node;
  AkCamera    *camera;
  AkLight     *light;
  AkSpotLight *spot;
  struct stat  stDae;
  const char  *outDir  = "./assetkit_export_dae_camera_light";
  const char  *daePath = "./assetkit_export_dae_camera_light/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  node->name  = "CameraLightNode";
  scene->node = root;
  doc->scene  = scene;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 250.0f);
  ASSERT(camera != NULL);
  camera->name = "Camera";

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_SPOT);
  ASSERT(light != NULL);
  light->name = "Spot";
  spot = (AkSpotLight *)light->data;
  ASSERT(spot != NULL);
  spot->base.color.rgba.R = 0.25f;
  spot->base.color.rgba.G = 0.5f;
  spot->base.color.rgba.B = 0.75f;
  spot->attenuation.constant  = 1.0f;
  spot->attenuation.linear    = 0.125f;
  spot->attenuation.quadratic = 0.25f;
  spot->outerConeAngle        = 0.5f;
  spot->coneFalloffExponent   = 2.0f;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath, "<library_cameras>"));
  ASSERT(ak_test_file_contains(daePath, "<instance_camera url=\"#camera_0\"/>"));
  ASSERT(ak_test_file_contains(daePath, "<yfov>45"));
  ASSERT(ak_test_file_contains(daePath, "<library_lights>"));
  ASSERT(ak_test_file_contains(daePath, "<instance_light url=\"#light_0\"/>"));
  ASSERT(ak_test_file_contains(daePath, "<spot>"));
  ASSERT(ak_test_file_contains(daePath, "<falloff_angle>28.647"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.cameras.count == 1);
  ASSERT(roundTrip->lib.lights.count == 1);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_modern_light_range_roundtrip) {
  AkHeap       *heap;
  AkDoc        *doc;
  AkDoc        *roundTrip;
  AkScene      *scene;
  AkNode       *root, *node;
  AkLight      *light;
  AkLight      *roundLight;
  AkPointLight *point;
  const char   *outDir  = "./assetkit_export_dae_modern_light_range";
  const char   *daePath = "./assetkit_export_dae_modern_light_range/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_POINT);
  ASSERT(light != NULL);
  point = (AkPointLight *)light->data;
  ASSERT(point != NULL);
  point->base.color.rgba.R = 0.25f;
  point->base.color.rgba.G = 0.5f;
  point->base.color.rgba.B = 0.75f;
  point->base.intensity    = 2.0f;
  point->base.range        = 10.0f;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<color>0.5 1 1.5</color>"));
  ASSERT(ak_test_file_contains(daePath, "<quadratic_attenuation>0.99"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.lights.count == 1);
  roundLight = roundTrip->lib.lights.first;
  ASSERT(roundLight != NULL);
  ASSERT(roundLight->data != NULL);
  ASSERT(fabsf(roundLight->data->range - 10.0f) < 0.01f);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_asset_metadata_smoke) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  AkResult    loadRet;
  uintptr_t   coordCvtType;
  const char *outDir  = "./assetkit_export_dae_asset_metadata";
  const char *daePath = "./assetkit_export_dae_asset_metadata/model.dae";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  doc->coordSys    = AK_ZUP;
  doc->inf         = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  ASSERT(doc->inf != NULL);
  doc->inf->name          = "model";
  doc->inf->base.created  = (time_t)1609459200;
  doc->inf->base.modified = (time_t)1609545600;
  doc->inf->base.keywords = "house architecture";
  doc->inf->base.revision = "7";
  doc->inf->base.subject  = "metadata round trip";
  doc->inf->base.title    = "Asset metadata";
  doc->unit        = ak_heap_calloc(heap, doc, sizeof(*doc->unit));
  ASSERT(doc->unit != NULL);
  doc->unit->name  = "centimeter";
  doc->unit->dist  = 0.01;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<authoring_tool>" AK_AUTHORING_TOOL "</authoring_tool>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<created>2021-01-01T00:00:00Z</created>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<keywords>house architecture</keywords>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<modified>2021-01-02T00:00:00Z</modified>"));
  ASSERT(ak_test_file_contains(daePath, "<revision>7</revision>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<subject>metadata round trip</subject>"));
  ASSERT(ak_test_file_contains(daePath, "<title>Asset metadata</title>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<unit name=\"centimeter\" meter=\"0.01\"/>"));
  ASSERT(ak_test_file_contains(daePath, "<up_axis>Z_UP</up_axis>"));

  roundTrip = NULL;
  coordCvtType = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);
  loadRet = ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, coordCvtType);
  ASSERT(loadRet == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->coordSys == AK_ZUP);
  ASSERT(roundTrip->unit != NULL);
  ASSERT(roundTrip->unit->name != NULL);
  ASSERT(strcmp(roundTrip->unit->name, "centimeter") == 0);
  ASSERT(fabs(roundTrip->unit->dist - 0.01) < 0.000001);
  ASSERT(roundTrip->inf != NULL);
  ASSERT(roundTrip->inf->base.keywords != NULL);
  ASSERT(strcmp(roundTrip->inf->base.keywords, "house architecture") == 0);
  ASSERT(roundTrip->inf->base.revision != NULL);
  ASSERT(strcmp(roundTrip->inf->base.revision, "7") == 0);
  ASSERT(roundTrip->inf->base.subject != NULL);
  ASSERT(strcmp(roundTrip->inf->base.subject, "metadata round trip") == 0);
  ASSERT(roundTrip->inf->base.title != NULL);
  ASSERT(strcmp(roundTrip->inf->base.title, "Asset metadata") == 0);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_core_extras_smoke) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkMaterial  *mat;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  AkInstanceGeometry *instGeom;
  AkMesh      *mesh;
  AkMeshPrimitive *prim;
  const char  *outDir  = "./assetkit_export_dae_core_extras";
  const char  *daePath = "./assetkit_export_dae_core_extras/model.dae";
  const float  positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  doc->inf = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  ASSERT(doc->inf != NULL);
  doc->inf->base.extra = ak_test_dae_extra(heap, doc->inf, "asset-extra");
  doc->extra           = ak_test_dae_extra(heap, doc, "scene-wrapper-extra");
  ASSERT(doc->inf->base.extra != NULL);
  ASSERT(doc->extra != NULL);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  scene->node  = root;
  scene->extra = ak_test_dae_extra(heap, scene, "visual-scene-extra");
  node->extra  = ak_test_dae_extra(heap, node, "node-extra");
  doc->scene   = scene;
  ASSERT(scene->extra != NULL);
  ASSERT(node->extra != NULL);

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh ? mesh->primitive : NULL;
  ASSERT(geom != NULL);
  ASSERT(mesh != NULL);
  ASSERT(prim != NULL);
  geom->extra = ak_test_dae_extra(heap, geom, "geometry-extra");
  mesh->extra = ak_test_dae_extra(heap, mesh, "mesh-extra");
  prim->extra = ak_test_dae_extra(heap, prim, "primitive-extra");
  ASSERT(geom->extra != NULL);
  ASSERT(mesh->extra != NULL);
  ASSERT(prim->extra != NULL);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  mat = ak_heap_calloc(heap, doc, sizeof(*mat));
  ASSERT(mat != NULL);
  mat->extra = ak_test_dae_extra(heap, mat, "material-extra");
  ASSERT(mat->extra != NULL);
  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;

  ak_addSubNode(root, node, false);
  instGeom = ak_nodeAttachGeometry(node, geom);
  ASSERT(instGeom != NULL);
  instGeom->base.extra = ak_test_dae_extra(heap, instGeom, "instance-extra");
  ASSERT(instGeom->base.extra != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<technique profile=\"AssetKit\"><note>asset-extra</note></technique>"));
  ASSERT(ak_test_file_contains(daePath, "<note>scene-wrapper-extra</note>"));
  ASSERT(ak_test_file_contains(daePath, "<note>visual-scene-extra</note>"));
  ASSERT(ak_test_file_contains(daePath, "<note>node-extra</note>"));
  ASSERT(ak_test_file_contains(daePath, "<note>geometry-extra</note>"));
  ASSERT(ak_test_file_contains(daePath, "<note>mesh-extra</note>"));
  ASSERT(ak_test_file_contains(daePath, "<note>primitive-extra</note>"));
  ASSERT(ak_test_file_contains(daePath, "<note>instance-extra</note>"));
  ASSERT(ak_test_file_contains(daePath, "<note>material-extra</note>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_empty_scene) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  struct stat  stDae;
  const char  *outDir  = "./assetkit_export_dae_empty_scene";
  const char  *daePath = "./assetkit_export_dae_empty_scene/Empty.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->name = "Empty";
  doc->scene  = scene;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath, "<visual_scene id=\"scene_0\" name=\"Empty\">"));
  ASSERT(ak_test_file_contains(daePath, "<node id=\"vnode_0\"/>"));
  ASSERT(!ak_test_file_contains(daePath, "<library_geometries>"));
  ASSERT(!ak_test_file_contains(daePath, "<geometry "));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_null_scene) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  struct stat  stDae;
  const char  *outDir  = "./assetkit_export_dae_null_scene";
  const char  *daePath = "./assetkit_export_dae_null_scene/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath, "<visual_scene id=\"scene_0\">"));
  ASSERT(ak_test_file_contains(daePath, "<node id=\"vnode_0\"/>"));
  ASSERT(!ak_test_file_contains(daePath, "<library_geometries>"));
  ASSERT(!ak_test_file_contains(daePath, "<geometry "));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_rejects_too_deep_node_graph) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *parent;
  uint32_t     i;
  struct stat  stDae;
  const char  *outDir  = "./assetkit_export_dae_deep_nodes";
  const char  *daePath = "./assetkit_export_dae_deep_nodes/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene  = ak_heap_calloc(heap, doc, sizeof(*scene));
  parent = ak_heap_calloc(heap, scene, sizeof(*parent));
  ASSERT(scene != NULL);
  ASSERT(parent != NULL);

  scene->node = parent;
  doc->scene  = scene;

  for (i = 0; i < 514u; i++) {
    AkNode *child;

    child = ak_heap_calloc(heap, doc, sizeof(*child));
    ASSERT(child != NULL);
    ak_addSubNode(parent, child, false);
    parent = child;
  }

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_EINVAL);
  ASSERT(stat(daePath, &stDae) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_empty_mesh_geometry) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkGeometry  *geom;
  AkObject    *meshObj;
  AkMesh      *mesh;
  struct stat  stDae;
  const char  *outDir  = "./assetkit_export_dae_empty_mesh_geometry";
  const char  *daePath = "./assetkit_export_dae_empty_mesh_geometry/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  geom    = ak_heap_calloc(heap, doc, sizeof(*geom));
  meshObj = ak_objAlloc(heap, geom, sizeof(*mesh), AK_GEOMETRY_MESH, true);
  mesh    = ak_objGet(meshObj);

  geom->name = "NoPrimitive";
  geom->gdata = meshObj;
  mesh->geom = geom;
  mesh->name = "NoPrimitive";

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath, "<library_geometries>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<geometry id=\"geom_0\" name=\"NoPrimitive\"><mesh></mesh></geometry>"));
  ASSERT(!ak_test_file_contains(daePath, "<triangles"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_skips_unresolved_geometry_instance) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkDoc              *roundTrip;
  AkScene            *scene;
  AkNode             *root;
  AkNode             *node;
  AkInstanceGeometry *instGeom;
  struct stat         stDae;
  const char         *outDir  = "./assetkit_export_dae_unresolved_geom";
  const char         *daePath = "./assetkit_export_dae_unresolved_geom/model.dae";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene = ak_heap_calloc(heap, doc, sizeof(*scene));
  root  = ak_heap_calloc(heap, scene, sizeof(*root));
  node  = ak_heap_calloc(heap, doc, sizeof(*node));
  ASSERT(scene != NULL && root != NULL && node != NULL);

  scene->node = root;
  doc->scene  = scene;
  ak_addSubNode(root, node, false);

  instGeom = ak_heap_calloc(heap, node, sizeof(*instGeom));
  ASSERT(instGeom != NULL);
  instGeom->base.type = AK_INSTANCE_GEOMETRY;
  instGeom->base.node = node;
  node->geometry = instGeom;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(!ak_test_file_contains(daePath, "<instance_geometry"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_resolves_geometry_instance_url_ptr) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root;
  AkNode             *node;
  AkGeometry         *geom;
  AkInstanceGeometry *instGeom;
  struct stat         stDae;
  const char         *outDir  = "./assetkit_export_dae_url_ptr_geom";
  const char         *daePath = "./assetkit_export_dae_url_ptr_geom/model.dae";
  const float         positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene = ak_heap_calloc(heap, doc, sizeof(*scene));
  root  = ak_heap_calloc(heap, scene, sizeof(*root));
  node  = ak_heap_calloc(heap, doc, sizeof(*node));
  ASSERT(scene != NULL && root != NULL && node != NULL);

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(geom != NULL);
  ak_setypeid(geom, AKT_GEOMETRY);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  scene->node = root;
  doc->scene  = scene;
  ak_addSubNode(root, node, false);

  instGeom = ak_heap_calloc(heap, node, sizeof(*instGeom));
  ASSERT(instGeom != NULL);
  instGeom->base.type    = AK_INSTANCE_GEOMETRY;
  instGeom->base.node    = node;
  instGeom->base.url.ptr = geom;
  node->geometry         = instGeom;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(instGeom->base.object == geom);
  ASSERT(stat(daePath, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(ak_test_file_contains(daePath, "<instance_geometry url=\"#geom_0\">"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_rejects_unsupported_morph_instance) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root;
  AkNode             *node;
  AkGeometry         *geom;
  AkInstanceGeometry *instGeom;
  AkInstanceMorph    *morpher;
  AkMorph            *morph;
  AkFloatArray       *overrideWeights;
  struct stat         stDae;
  const char         *outDir  = "./assetkit_export_dae_bad_morph";
  const char         *daePath = "./assetkit_export_dae_bad_morph/model.dae";
  const float         positions[9] = {
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

  morph = ak_heap_calloc(heap, doc, sizeof(*morph));
  ASSERT(morph != NULL);
  morph->method      = AK_MORPH_METHOD_NORMALIZED;
  morph->targetCount = 1;
  doc->lib.morphs.first = morph;
  doc->lib.morphs.last  = morph;
  doc->lib.morphs.count = 1;

  overrideWeights = ak_heap_alloc(heap,
                                  node,
                                  sizeof(*overrideWeights)
                                  + sizeof(overrideWeights->items[0]));
  ASSERT(overrideWeights != NULL);
  overrideWeights->count = 1;
  overrideWeights->items[0] = 0.5f;

  morpher = ak_heap_calloc(heap, node, sizeof(*morpher));
  ASSERT(morpher != NULL);
  morpher->morph = morph;
  morpher->overrideWeights = overrideWeights;

  ak_addSubNode(root, node, false);
  instGeom = ak_nodeAttachGeometry(node, geom);
  ASSERT(instGeom != NULL);
  instGeom->morpher = morpher;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_EINVAL);
  ASSERT(stat(daePath, &stDae) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_expands_gpu_instancing_node_instance) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root;
  AkNode          *target;
  AkGeometry      *geom;
  AkGpuInstancing *instancing;
  const char      *outDir  = "./assetkit_export_dae_gpu_instancing";
  const char      *daePath = "./assetkit_export_dae_gpu_instancing/model.dae";
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
  target      = ak_heap_calloc(heap, doc, sizeof(*target));
  instancing  = ak_heap_calloc(heap, target, sizeof(*instancing));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(target != NULL);
  ASSERT(instancing != NULL);

  scene->node           = root;
  doc->scene            = scene;
  target->visible       = true;
  target->gpuInstancing = instancing;
  doc->lib.nodes.first  = target;
  doc->lib.nodes.last   = target;
  doc->lib.nodes.count  = 1;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;
  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);

  instancing->translation = ak_test_make_float_accessor(heap,
                                                        instancing,
                                                        translations,
                                                        3,
                                                        2);
  instancing->count       = 2;

  ASSERT(ak_nodeAttachNodeInstance(root, target) != NULL);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(!ak_test_file_contains(daePath, "<library_nodes>"));
  ASSERT(ak_test_file_count(daePath, "<instance_geometry url=\"#geom_0\"") == 2);
  ASSERT(ak_test_file_count(daePath, "<matrix>") == 2);
  ASSERT(ak_test_file_contains(daePath,
                               "<matrix>1 0 0 2 0 1 0 0 0 0 1 0 0 0 0 1</matrix>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_expands_gpu_instancing_direct_node) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root;
  AkNode          *node;
  AkGeometry      *geom;
  AkGpuInstancing *instancing;
  const char      *outDir  = "./assetkit_export_dae_gpu_instancing_direct";
  const char      *daePath = "./assetkit_export_dae_gpu_instancing_direct/model.dae";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float translations[6] = {
    0.0f, 0.0f, 0.0f,
    3.0f, 0.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene      = ak_heap_calloc(heap, doc, sizeof(*scene));
  root       = ak_heap_calloc(heap, scene, sizeof(*root));
  node       = ak_heap_calloc(heap, doc, sizeof(*node));
  instancing = ak_heap_calloc(heap, node, sizeof(*instancing));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(instancing != NULL);

  scene->node          = root;
  doc->scene           = scene;
  node->visible        = true;
  node->gpuInstancing  = instancing;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  instancing->translation = ak_test_make_float_accessor(heap,
                                                        instancing,
                                                        translations,
                                                        3,
                                                        2);
  instancing->count       = 2;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_count(daePath, "<instance_geometry url=\"#geom_0\"") == 2);
  ASSERT(ak_test_file_count(daePath, "<matrix>") == 2);
  ASSERT(ak_test_file_contains(daePath,
                               "<matrix>1 0 0 3 0 1 0 0 0 0 1 0 0 0 0 1</matrix>"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_polygon_vcount_validation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkGeometry    *geom;
  AkMesh        *mesh;
  AkPolygon     *poly;
  AkUIntArray   *vcount;
  struct stat    stDae;
  const char    *outDir  = "./assetkit_export_dae_polygon_vcount";
  const char    *daePath = "./assetkit_export_dae_polygon_vcount/model.dae";
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

  geom  = ak_test_make_triangle_geom(heap, doc, positions);
  mesh  = ak_objGet(geom->gdata);
  poly  = ak_heap_calloc(heap, geom->gdata, sizeof(*poly));
  vcount = ak_heap_alloc(heap, poly, sizeof(*vcount) + sizeof(AkUInt));
  ASSERT(poly != NULL);
  ASSERT(vcount != NULL);

  poly->base           = *mesh->primitive;
  poly->base.type      = AK_PRIMITIVE_POLYGONS;
  poly->base.nPolygons = 0;
  poly->vcount         = vcount;
  vcount->count        = 1;
  vcount->items[0]     = 3;
  mesh->primitive      = &poly->base;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<polylist count=\"1\">"));
  ASSERT(ak_test_file_contains(daePath, "<vcount>3</vcount>"));

  ak_test_export_cleanup(outDir);
  vcount->items[0] = 4;
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_EINVAL);
  ASSERT(stat(daePath, &stDae) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_rejects_nonfinite_float) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  const char  *outDir = "./assetkit_export_dae_nonfinite_float";
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
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_nodeSetTransformMatrix(node, matrix);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_ERR);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}
