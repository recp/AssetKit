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

TEST_IMPL(gltf_export_native_skin) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *jointA, *jointB, *meshNode;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceSkin *skinner;
  AkSkin         *skin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_native_skin");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  size_t i;

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  jointA      = ak_heap_calloc(heap, doc, sizeof(*jointA));
  jointB      = ak_heap_calloc(heap, doc, sizeof(*jointB));
  meshNode    = ak_heap_calloc(heap, doc, sizeof(*meshNode));
  scene->node = root;
  doc->scene  = scene;

  jointA->name = "JointA";
  jointB->name = "JointB";

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ak_test_add_skin_inputs(heap, prim);

  skin = ak_heap_calloc(heap, doc, sizeof(*skin));
  skin->nJoints = 2;
  skin->joints = ak_heap_alloc(heap, skin, sizeof(*skin->joints) * 2);
  skin->joints[0] = jointA;
  skin->joints[1] = jointB;
  skin->skeleton = jointA;
  skin->invBindPoses = ak_heap_alloc(heap,
                                     skin,
                                     sizeof(*skin->invBindPoses) * 2);
  for (i = 0; i < 32; i++)
    ((float *)skin->invBindPoses)[i] = ((i % 16) % 5) == 0 ? 1.0f : 0.0f;

  skinner = ak_heap_calloc(heap, meshNode, sizeof(*skinner));
  skinner->skin = skin;

  ak_addSubNode(root, jointA, false);
  ak_addSubNode(jointA, jointB, false);
  ak_addSubNode(root, meshNode, false);
  instGeom = ak_nodeAttachGeometry(meshNode, geom);
  ASSERT(instGeom != NULL);
  instGeom->skinner = skinner;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"skins\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"skin\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"inverseBindMatrices\":3"));
  ASSERT(ak_test_file_contains(gltfPath, "\"skeleton\":"));
  ASSERT(ak_test_file_contains(gltfPath, "\"joints\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"JOINTS_0\":1"));
  ASSERT(ak_test_file_contains(gltfPath, "\"WEIGHTS_0\":2"));
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"MAT4\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_unscened_skin_joints_scene_root) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *jointA, *jointB, *meshNode;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceSkin  *skinner;
  AkSkin          *skin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_unscened_skin");
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
  jointA      = ak_heap_calloc(heap, doc, sizeof(*jointA));
  jointB      = ak_heap_calloc(heap, doc, sizeof(*jointB));
  meshNode    = ak_heap_calloc(heap, doc, sizeof(*meshNode));
  scene->node = root;
  doc->scene  = scene;

  root->name     = "SceneRoot";
  jointA->name   = "JointA";
  jointB->name   = "JointB";
  meshNode->name = "Mesh";

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ak_test_add_skin_inputs(heap, prim);

  skin = ak_heap_calloc(heap, doc, sizeof(*skin));
  skin->nJoints = 2;
  skin->joints = ak_heap_alloc(heap, skin, sizeof(*skin->joints) * 2);
  skin->joints[0] = jointA;
  skin->joints[1] = jointB;
  skin->skeleton  = jointA;

  skinner = ak_heap_calloc(heap, meshNode, sizeof(*skinner));
  skinner->skin = skin;

  ak_addSubNode(jointA, jointB, false);
  ak_addSubNode(root, meshNode, false);
  instGeom = ak_nodeAttachGeometry(meshNode, geom);
  ASSERT(instGeom != NULL);
  instGeom->skinner = skinner;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"skeleton\":"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "Mesh") != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "JointA") != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_instanced_skin_joints_not_scene_root) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkDoc              *roundTrip;
  AkScene            *scene;
  AkNode             *root, *group, *jointA, *jointB, *meshNode;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceSkin     *skinner;
  AkSkin             *skin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_instanced_skin_joint");
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
  group       = ak_heap_calloc(heap, doc, sizeof(*group));
  jointA      = ak_heap_calloc(heap, doc, sizeof(*jointA));
  jointB      = ak_heap_calloc(heap, doc, sizeof(*jointB));
  meshNode    = ak_heap_calloc(heap, doc, sizeof(*meshNode));
  scene->node = root;
  doc->scene  = scene;

  root->name     = "SceneRoot";
  group->name    = "Group";
  jointA->name   = "JointA";
  jointB->name   = "JointB";
  meshNode->name = "Mesh";

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ak_test_add_skin_inputs(heap, prim);

  skin = ak_heap_calloc(heap, doc, sizeof(*skin));
  skin->nJoints = 2;
  skin->joints = ak_heap_alloc(heap, skin, sizeof(*skin->joints) * 2);
  skin->joints[0] = jointA;
  skin->joints[1] = jointB;
  skin->skeleton  = jointA;

  skinner = ak_heap_calloc(heap, meshNode, sizeof(*skinner));
  skinner->skin = skin;

  ak_addSubNode(jointA, jointB, false);
  ak_addSubNode(root, group, false);
  ak_addSubNode(root, meshNode, false);
  ASSERT(ak_nodeAttachNodeInstance(group, jointA) != NULL);
  instGeom = ak_nodeAttachGeometry(meshNode, geom);
  ASSERT(instGeom != NULL);
  instGeom->skinner = skinner;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"skeleton\":"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "Group") != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "Mesh") != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "JointA") == NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_bakes_non_trs_leaf_mesh_transform) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkObject       *matrixObj;
  AkMatrix       *matrix;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_non_trs_bake");
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

  node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
  matrixObj       = ak_objAlloc(heap,
                                node->transform,
                                sizeof(*matrix),
                                AKT_MATRIX,
                                true);
  matrix          = ak_objGet(matrixObj);
  matrix->val[0][0] = 1.0f;
  matrix->val[0][1] = 0.0f;
  matrix->val[0][2] = 1.0f;
  matrix->val[0][3] = 0.0f;
  matrix->val[1][0] = 0.0f;
  matrix->val[1][1] = 1.0f;
  matrix->val[1][2] = 1.0f;
  matrix->val[1][3] = 0.0f;
  matrix->val[2][0] = 0.0f;
  matrix->val[2][1] = 0.0f;
  matrix->val[2][2] = 1.0f;
  matrix->val[2][3] = 0.0f;
  matrix->val[3][0] = 0.0f;
  matrix->val[3][1] = 0.0f;
  matrix->val[3][2] = 0.0f;
  matrix->val[3][3] = 1.0f;
  node->transform->base = matrixObj;
  node->transform->item = matrixObj;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_count(gltfPath, "\"matrix\"") == 0);
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[1,1,1]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"mesh\":0"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_morph_target) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceMorph *morpher;
  AkMorph        *morph;
  AkMorphTarget  *target;
  AkMorphable    *morphable;
  AkObject       *targetObj;
  AkInput        *targetPos;
  AkAccessor     *targetAcc;
  AkBuffer       *targetBuff;
  AkFloatArray   *meshWeights;
  AkFloatArray   *nodeWeights;
  const char     *targetNames[1] = {"raise"};
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_morph_target");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float deltas[9] = {
    0.0f, 0.0f, 0.1f,
    0.0f, 0.0f, 0.2f,
    0.0f, 0.0f, 0.3f
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

  morph      = ak_heap_calloc(heap, doc, sizeof(*morph));
  target     = ak_heap_calloc(heap, morph, sizeof(*target));
  targetObj  = ak_objAlloc(heap,
                           target,
                           sizeof(*morphable),
                           AK_MORPHABLE_MORPHABLE,
                           true);
  morphable  = ak_objGet(targetObj);
  targetPos  = ak_heap_calloc(heap, targetObj, sizeof(*targetPos));
  targetAcc  = ak_heap_calloc(heap, targetPos, sizeof(*targetAcc));
  targetBuff = ak_heap_calloc(heap, targetAcc, sizeof(*targetBuff));

  targetBuff->length = sizeof(deltas);
  targetBuff->data   = ak_heap_alloc(heap, targetBuff, targetBuff->length);
  memcpy(targetBuff->data, deltas, sizeof(deltas));

  targetAcc->buffer                = targetBuff;
  targetAcc->byteLength            = targetBuff->length;
  targetAcc->byteStride            = sizeof(float) * 3;
  targetAcc->fillByteSize          = sizeof(float) * 3;
  targetAcc->bytesPerComponent     = sizeof(float);
  targetAcc->componentSize         = AK_COMPONENT_SIZE_VEC3;
  targetAcc->componentType         = AKT_FLOAT;
  targetAcc->originalComponentType = AKT_FLOAT;
  targetAcc->componentCount        = 3;
  targetAcc->count                 = 3;

  targetPos->accessor = targetAcc;
  targetPos->semantic = AK_INPUT_POSITION;
  morphable->input = targetPos;
  morphable->inputCount = 1;

  target->target = targetObj;
  target->primitiveCount = 1;
  morph->target = target;
  morph->method = AK_MORPH_METHOD_ADDITIVE;
  morph->targetCount = 1;
  morph->targetNames = targetNames;

  meshWeights = ak_heap_alloc(heap,
                              doc,
                              sizeof(*meshWeights)
                              + sizeof(meshWeights->items[0]));
  meshWeights->count = 1;
  meshWeights->items[0] = 0.25f;
  mesh->weights = meshWeights;

  nodeWeights = ak_heap_alloc(heap,
                              node,
                              sizeof(*nodeWeights)
                              + sizeof(nodeWeights->items[0]));
  nodeWeights->count = 1;
  nodeWeights->items[0] = 0.75f;

  morpher = ak_heap_calloc(heap, node, sizeof(*morpher));
  morpher->morph = morph;
  morpher->overrideWeights = nodeWeights;

  ak_addSubNode(root, node, false);
  instGeom = ak_nodeAttachGeometry(node, geom);
  ASSERT(instGeom != NULL);
  instGeom->morpher = morpher;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"targets\":[{\"POSITION\":1}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"min\":[0,0,0.100000001"));
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[0,0,0.300000011"));
  ASSERT(ak_test_file_contains(gltfPath, "\"weights\":[0.25]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"weights\":[0.75]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"targetNames\":[\"raise\"]}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.morphs.first != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_translation_animation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkGeometry    *geom;
  AkObject      *translateObj;
  AkTranslate   *translate;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_anim_translation");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float times[2] = {0.0f, 1.0f};
  const float translations[6] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 2.0f, 3.0f
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
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  translateObj = ak_getTransformTRS(node, AKT_TRANSLATE);
  ASSERT(translateObj != NULL);
  translate = ak_objGet(translateObj);
  translate->val[0] = 0.0f;
  translate->val[1] = 0.0f;
  translate->val[2] = 0.0f;

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  anim->name = "Move";
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     translations,
                                                     3,
                                                     2);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input = timeInput;
  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  target->target = translateObj;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_POSITION;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"animations\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"samplers\":[{\"input\":1,\"output\":2}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"translation\":[0,0,0]"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"matrix\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"min\":[0]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[1]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_reused_node_animation_duplicates_channels) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *targetNode;
  AkGeometry    *geom;
  AkObject      *translateObj;
  AkTranslate   *translate;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_reused_node_anim");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float times[2] = {0.0f, 1.0f};
  const float translations[6] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  targetNode  = ak_heap_calloc(heap, doc, sizeof(*targetNode));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  targetNode->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(ak_nodeAttachGeometry(targetNode, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, targetNode) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, targetNode) != NULL);

  translateObj = ak_getTransformTRS(targetNode, AKT_TRANSLATE);
  ASSERT(translateObj != NULL);
  translate = ak_objGet(translateObj);
  translate->val[0] = 0.0f;
  translate->val[1] = 0.0f;
  translate->val[2] = 0.0f;

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     translations,
                                                     3,
                                                     2);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input = timeInput;
  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  target->target = translateObj;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_POSITION;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_count(gltfPath, "\"primitives\"") == 1);
  ASSERT(ak_test_file_count(gltfPath, "\"mesh\":0") == 2);
  ASSERT(ak_test_file_count(gltfPath, "\"path\":\"translation\"") == 2);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"target\":{\"node\":0,\"path\":\"translation\"}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"target\":{\"node\":1,\"path\":\"translation\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_cubic_animation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkGeometry    *geom;
  AkObject      *translateObj;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_anim_cubic");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float times[2] = {0.0f, 1.0f};
  const float packedCubic[18] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 2.0f, 3.0f,
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
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  translateObj = ak_getTransformTRS(node, AKT_TRANSLATE);
  ASSERT(translateObj != NULL);

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     packedCubic,
                                                     3,
                                                     6);
  ASSERT(valueInput->accessor != NULL);

  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_HERMITE;

  target->target = translateObj;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_POSITION;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"interpolation\":\"CUBICSPLINE\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"translation\":[0,0,0]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_visibility_animation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkDoc         *roundTrip;
  AkScene       *scene;
  AkNode        *root, *node;
  AkNode        *rtNode;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkChannel     *rtChannel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_anim_visibility");
  const float    times[2] = {0.0f, 1.0f};
  const uint8_t  values[2] = {1u, 0u};

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
  ak_addSubNode(root, node, false);

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_ubyte_accessor(heap,
                                                     valueInput,
                                                     values,
                                                     1,
                                                     2);
  ASSERT(valueInput->accessor != NULL);

  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_STEP;

  target->target = &node->visible;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_BOOL;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_animation_pointer\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_node_visibility\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_node_visibility\":{\"visible\":true}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"target\":{\"path\":\"pointer\",\"extensions\":{\"KHR_animation_pointer\":{\"pointer\":\"/nodes/0/extensions/KHR_node_visibility/visible\"}}}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"interpolation\":\"STEP\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5121"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  rtNode = roundTrip->scene->node->chld;
  ASSERT(rtNode != NULL);
  ASSERT(roundTrip->lib.animations.first != NULL);
  rtChannel = roundTrip->lib.animations.first->channel;
  ASSERT(rtChannel != NULL);
  ASSERT(rtChannel->targetType == AK_TARGET_BOOL);
  ASSERT(rtChannel->resolvedTarget != NULL);
  ASSERT(rtChannel->resolvedTarget->target == &rtNode->visible);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_unsupported_animation_channel) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkResolvedTarget *target;
  float          unsupportedTarget[2] = {0.0f, 1.0f};
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_skip_unsupported_anim");

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
  ak_addSubNode(root, node, false);

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  target->target = unsupportedTarget;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_VEC2;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"animations\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_brep) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-brep-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/brep.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);

  ASSERT(ak_test_write_dae_brep_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.geometries.first != NULL);
  ASSERT(doc->lib.geometries.first->gdata != NULL);
  ASSERT(doc->lib.geometries.first->gdata->type == AK_GEOMETRY_BREP);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_EINVAL);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_EINVAL);

  ak_free(doc);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_scene_root_is_child_node) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        gltfPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-root-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/root.gltf", tmpdir);
  ASSERT(ak_test_write_gltf_root(gltfPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);

  root = scene->node->chld;
  ASSERT(root != NULL);
  ASSERT(root != scene->node);
  ASSERT(root->parent == scene->node);
  ASSERT(root->name && strcmp(root->name, "Root") == 0);
  ASSERT(root->next == NULL);
  ASSERT(doc->lib.nodes.count == 1);

  ak_free(doc);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_scene_light_cache) {
  AkDoc      *doc;
  AkScene    *scene;
  AkLight    *light;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        gltfPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-light-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/light.gltf", tmpdir);
  ASSERT(ak_test_write_gltf_light(gltfPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->lights.count == 1);
  ASSERT(scene->lights.useCount == 1);
  ASSERT(scene->lights.first != NULL);
  ASSERT(scene->lights.first->firstInstance != NULL);
  ASSERT(scene->lights.first->firstInstance->node == scene->node->chld);

  light = scene->lights.first->light;
  ASSERT(light != NULL);
  ASSERT(ak_instanceObject(scene->lights.first->firstInstance) == light);
  ASSERT(light->name && strcmp(light->name, "Key") == 0);
  ASSERT(light->data != NULL);
  ASSERT(light->data->type == AK_LIGHT_TYPE_POINT);
  ASSERT(fabsf(light->data->intensity - 2.0f) < 0.001f);
  ASSERT(fabsf(light->data->range - 10.0f) < 0.001f);

  ak_free(doc);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
