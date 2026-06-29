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

#include "../test_export_common.h"

#include <math.h>

static
bool
ak_test_near(float a, float b) {
  return fabsf(a - b) < 0.00001f;
}

static
bool
ak_test_vec3_eq(const float *v, float x, float y, float z) {
  return ak_test_near(v[0], x)
         && ak_test_near(v[1], y)
         && ak_test_near(v[2], z);
}

TEST_IMPL(coord_change_doc_converts_scene_payloads) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root, *node;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkObject           *translateObj, *quatObj;
  AkTranslate        *translate;
  AkQuaternion       *quat;
  AkGpuInstancing    *instancing;
  AkSkin             *skin;
  AkAnimation        *anim;
  AkAnimSampler      *posSampler, *quatSampler;
  AkChannel          *posChannel, *quatChannel;
  AkInput            *posOutput, *quatOutput;
  AkResolvedTarget   *posTarget, *quatTarget;
  AkMorph            *morph;
  AkMorphTarget      *morphTarget;
  AkObject           *morphObj;
  AkMorphable        *morphable;
  AkInput            *morphInput;
  float              *posData;
  float              *animPosData;
  float              *animQuatData;
  float              *instPosData;
  float              *instScaleData;
  float              *instQuatData;
  float              *morphData;
  const float positions[9] = {
    1.0f, 2.0f, 3.0f,
    4.0f, 5.0f, 6.0f,
    7.0f, 8.0f, 9.0f
  };
  const float animPos[3] = {1.0f, 2.0f, 3.0f};
  const float animQuat[4] = {0.0f, 0.70710677f, 0.0f, 0.70710677f};
  const float instPos[3] = {1.0f, 2.0f, 3.0f};
  const float instScale[3] = {1.0f, 2.0f, 3.0f};
  const float morphPos[3] = {1.0f, 2.0f, 3.0f};

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->coordSys = AK_ZUP;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(geom != NULL);
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  translateObj = ak_getTransformTRS(node, AKT_TRANSLATE);
  quatObj      = ak_getTransformTRS(node, AKT_QUATERNION);
  ASSERT(translateObj != NULL);
  ASSERT(quatObj != NULL);
  translate = ak_objGet(translateObj);
  quat      = ak_objGet(quatObj);
  translate->val[0] = 1.0f;
  translate->val[1] = 2.0f;
  translate->val[2] = 3.0f;
  quat->val[0] = 0.0f;
  quat->val[1] = 0.70710677f;
  quat->val[2] = 0.0f;
  quat->val[3] = 0.70710677f;

  instancing = ak_heap_calloc(heap, node, sizeof(*instancing));
  instancing->translation = ak_test_make_float_accessor(heap,
                                                        instancing,
                                                        instPos,
                                                        3,
                                                        1);
  instancing->scale       = ak_test_make_float_accessor(heap,
                                                        instancing,
                                                        instScale,
                                                        3,
                                                        1);
  instancing->rotation    = ak_test_make_float_accessor(heap,
                                                        instancing,
                                                        animQuat,
                                                        4,
                                                        1);
  instancing->count       = 1;
  node->gpuInstancing     = instancing;

  skin = ak_heap_calloc(heap, doc, sizeof(*skin));
  skin->nJoints = 1;
  skin->invBindPoses = ak_heap_calloc(heap,
                                      skin,
                                      sizeof(*skin->invBindPoses));
  skin->bindShapeMatrix[0][0] = 1.0f;
  skin->bindShapeMatrix[1][1] = 1.0f;
  skin->bindShapeMatrix[2][2] = 1.0f;
  skin->bindShapeMatrix[3][3] = 1.0f;
  skin->bindShapeMatrix[3][0] = 1.0f;
  skin->bindShapeMatrix[3][1] = 2.0f;
  skin->bindShapeMatrix[3][2] = 3.0f;
  skin->invBindPoses[0][0][0] = 1.0f;
  skin->invBindPoses[0][1][1] = 1.0f;
  skin->invBindPoses[0][2][2] = 1.0f;
  skin->invBindPoses[0][3][3] = 1.0f;
  skin->invBindPoses[0][3][0] = 1.0f;
  skin->invBindPoses[0][3][1] = 2.0f;
  skin->invBindPoses[0][3][2] = 3.0f;
  doc->lib.skins.first = skin;
  doc->lib.skins.last  = skin;
  doc->lib.skins.count = 1;

  anim        = ak_heap_calloc(heap, doc, sizeof(*anim));
  posSampler  = ak_heap_calloc(heap, anim, sizeof(*posSampler));
  quatSampler = ak_heap_calloc(heap, anim, sizeof(*quatSampler));
  posChannel  = ak_heap_calloc(heap, anim, sizeof(*posChannel));
  quatChannel = ak_heap_calloc(heap, anim, sizeof(*quatChannel));
  posOutput   = ak_heap_calloc(heap, posSampler, sizeof(*posOutput));
  quatOutput  = ak_heap_calloc(heap, quatSampler, sizeof(*quatOutput));
  posTarget   = ak_heap_calloc(heap, posChannel, sizeof(*posTarget));
  quatTarget  = ak_heap_calloc(heap, quatChannel, sizeof(*quatTarget));

  posOutput->semantic = AK_INPUT_OUTPUT;
  posOutput->accessor = ak_test_make_float_accessor(heap,
                                                    posOutput,
                                                    animPos,
                                                    3,
                                                    1);
  quatOutput->semantic = AK_INPUT_OUTPUT;
  quatOutput->accessor = ak_test_make_float_accessor(heap,
                                                     quatOutput,
                                                     animQuat,
                                                     4,
                                                     1);
  posSampler->input = posOutput;
  posSampler->outputInput = posOutput;
  quatSampler->input = quatOutput;
  quatSampler->outputInput = quatOutput;
  posTarget->target = translateObj;
  quatTarget->target = quatObj;
  posChannel->source.ptr = posSampler;
  posChannel->resolvedTarget = posTarget;
  posChannel->targetType = AK_TARGET_POSITION;
  quatChannel->source.ptr = quatSampler;
  quatChannel->resolvedTarget = quatTarget;
  quatChannel->targetType = AK_TARGET_QUAT;
  posChannel->next = quatChannel;
  anim->sampler = posSampler;
  posSampler->base.next = (AkOneWayIterBase *)quatSampler;
  anim->channel = posChannel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last  = anim;
  doc->lib.animations.count = 1;

  morph       = ak_heap_calloc(heap, doc, sizeof(*morph));
  morphTarget = ak_heap_calloc(heap, morph, sizeof(*morphTarget));
  morphObj    = ak_objAlloc(heap,
                            morphTarget,
                            sizeof(*morphable),
                            AK_MORPHABLE_MORPHABLE,
                            true);
  morphable   = ak_objGet(morphObj);
  morphInput  = ak_heap_calloc(heap, morphObj, sizeof(*morphInput));
  morphInput->semantic = AK_INPUT_POSITION;
  morphInput->accessor = ak_test_make_float_accessor(heap,
                                                     morphInput,
                                                     morphPos,
                                                     3,
                                                     1);
  morphable->input = morphInput;
  morphable->inputCount = 1;
  morphTarget->target = morphObj;
  morph->target = morphTarget;
  doc->lib.morphs.first = morph;
  doc->lib.morphs.last  = morph;
  doc->lib.morphs.count = 1;

  ak_changeCoordSys(doc, AK_YUP);

  posData = prim->pos->accessor->buffer->data;
  ASSERT(ak_test_vec3_eq(posData, 1.0f, 3.0f, -2.0f));
  ASSERT(ak_test_vec3_eq(posData + 3, 4.0f, 6.0f, -5.0f));
  ASSERT(ak_test_vec3_eq(posData + 6, 7.0f, 9.0f, -8.0f));
  ASSERT(ak_test_vec3_eq(translate->val, 1.0f, 3.0f, -2.0f));
  ASSERT(ak_test_vec3_eq(quat->val, 0.0f, 0.0f, -0.70710677f));
  ASSERT(ak_test_near(quat->val[3], 0.70710677f));

  instPosData   = instancing->translation->buffer->data;
  instScaleData = instancing->scale->buffer->data;
  instQuatData  = instancing->rotation->buffer->data;
  ASSERT(ak_test_vec3_eq(instPosData, 1.0f, 3.0f, -2.0f));
  ASSERT(ak_test_vec3_eq(instScaleData, 1.0f, 3.0f, 2.0f));
  ASSERT(ak_test_vec3_eq(instQuatData, 0.0f, 0.0f, -0.70710677f));
  ASSERT(ak_test_near(instQuatData[3], 0.70710677f));

  animPosData  = posOutput->accessor->buffer->data;
  animQuatData = quatOutput->accessor->buffer->data;
  ASSERT(ak_test_vec3_eq(animPosData, 1.0f, 3.0f, -2.0f));
  ASSERT(ak_test_vec3_eq(animQuatData, 0.0f, 0.0f, -0.70710677f));
  ASSERT(ak_test_near(animQuatData[3], 0.70710677f));

  ASSERT(ak_test_vec3_eq(skin->bindShapeMatrix[3], 1.0f, 3.0f, -2.0f));
  ASSERT(ak_test_vec3_eq(skin->invBindPoses[0][3], 1.0f, 3.0f, -2.0f));

  morphData = morphInput->accessor->buffer->data;
  ASSERT(ak_test_vec3_eq(morphData, 1.0f, 3.0f, -2.0f));

  ASSERT(doc->coordSys == AK_YUP);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}
