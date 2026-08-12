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

#include <cglm/cglm.h>
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
  float               animPosAligned[3];
  float               animQuatAligned[4];
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

  skin = ak_heap_aligned_calloc(heap,
                                doc,
                                AK_ALIGNOF(AkSkin),
                                sizeof(*skin));
  skin->nJoints = 1;
  skin->invBindPoses = ak_heap_aligned_calloc(heap,
                                              skin,
                                              AK_ALIGNOF(AkFloat4x4),
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
  /* Byte-granular DAE accessors may be valid but unaligned.  Exercise the
     coordinate vector/quaternion paths without relying on float pointer
     alignment. */
  {
    AkAccessor *accessor;
    AkBuffer   *buffer;

    accessor = posOutput->accessor;
    buffer   = accessor->buffer;
    ak_free(buffer->data);
    buffer->length = 14u;
    buffer->data = ak_heap_calloc(heap, buffer, buffer->length);
    memcpy((unsigned char *)buffer->data + 1u, animPos, sizeof(animPos));
    accessor->byteOffset = 1u;
    accessor->byteStride = 13u;
    accessor->byteLength = buffer->length;

    accessor = quatOutput->accessor;
    buffer   = accessor->buffer;
    ak_free(buffer->data);
    buffer->length = 18u;
    buffer->data = ak_heap_calloc(heap, buffer, buffer->length);
    memcpy((unsigned char *)buffer->data + 1u, animQuat, sizeof(animQuat));
    accessor->byteOffset = 1u;
    accessor->byteStride = 17u;
    accessor->byteLength = buffer->length;
  }
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

  memcpy(animPosAligned,
         (unsigned char *)posOutput->accessor->buffer->data
           + posOutput->accessor->byteOffset,
         sizeof(animPosAligned));
  memcpy(animQuatAligned,
         (unsigned char *)quatOutput->accessor->buffer->data
           + quatOutput->accessor->byteOffset,
         sizeof(animQuatAligned));
  animPosData  = animPosAligned;
  animQuatData = animQuatAligned;
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

TEST_IMPL(coord_change_nested_channels_clone_shared_sampler) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkObject         *translateObj;
  AkAnimation      *parentAnim, *childAnim;
  AkAnimSampler    *sampler, *parentResult, *childResult;
  AkChannel        *parentChannel, *childChannel;
  AkInput          *output;
  AkResolvedTarget *parentTarget, *childTarget;
  float             sourceValue, parentValue, childValue;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ASSERT(heap && doc);
  ak_heap_setdata(heap, doc);
  doc->coordSys = AK_ZUP;

  translateObj = ak_objAlloc(heap,
                             doc,
                             sizeof(AkTranslate),
                             AKT_TRANSLATE,
                             true);
  parentAnim    = ak_heap_calloc(heap, doc, sizeof(*parentAnim));
  childAnim     = ak_heap_calloc(heap, parentAnim, sizeof(*childAnim));
  sampler       = ak_heap_calloc(heap, parentAnim, sizeof(*sampler));
  parentChannel = ak_heap_calloc(heap, parentAnim, sizeof(*parentChannel));
  childChannel  = ak_heap_calloc(heap, childAnim, sizeof(*childChannel));
  output        = ak_heap_calloc(heap, sampler, sizeof(*output));
  parentTarget  = ak_heap_calloc(heap, parentChannel, sizeof(*parentTarget));
  childTarget   = ak_heap_calloc(heap, childChannel, sizeof(*childTarget));
  ASSERT(translateObj && parentAnim && childAnim && sampler
         && parentChannel && childChannel && output
         && parentTarget && childTarget);

  sourceValue = 2.0f;
  output->semantic = AK_INPUT_OUTPUT;
  output->accessor = ak_test_make_float_accessor(heap,
                                                  output,
                                                  &sourceValue,
                                                  1u,
                                                  1u);
  ASSERT(output->accessor != NULL);
  sampler->input = sampler->outputInput = output;

  /* Z-up -> Y-up maps authored Y to -Z and authored Z to +Y.  These two
     nested channels therefore require different transforms even though they
     legally share one parent sampler. */
  parentTarget->target    = translateObj;
  parentTarget->off       = 1u;
  parentTarget->isPartial = true;
  parentChannel->source.ptr     = sampler;
  parentChannel->resolvedTarget = parentTarget;
  parentChannel->targetType     = AK_TARGET_FLOAT;

  childTarget->target    = translateObj;
  childTarget->off       = 2u;
  childTarget->isPartial = true;
  childChannel->source.ptr     = sampler;
  childChannel->resolvedTarget = childTarget;
  childChannel->targetType     = AK_TARGET_FLOAT;

  parentAnim->sampler   = sampler;
  parentAnim->channel   = parentChannel;
  parentAnim->animation = childAnim;
  childAnim->channel    = childChannel;
  doc->lib.animations.first = doc->lib.animations.last = parentAnim;
  doc->lib.animations.count = 1u;

  ak_changeCoordSys(doc, AK_YUP);

  parentResult = parentChannel->source.ptr;
  childResult  = childChannel->source.ptr;
  ASSERT(parentResult && childResult && parentResult != childResult);
  ASSERT(parentChannel->resolvedTarget->off == 2u);
  ASSERT(childChannel->resolvedTarget->off == 1u);
  memcpy(&parentValue,
         parentResult->outputInput->accessor->buffer->data,
         sizeof(parentValue));
  memcpy(&childValue,
         childResult->outputInput->accessor->buffer->data,
         sizeof(childValue));
  ASSERT(parentValue == -2.0f);
  ASSERT(childValue == 2.0f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(coord_change_skew_matches_matrix_basis_change) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *skewNode, *matrixNode;
  AkObject    *skewObj, *matrixObj;
  AkSkew      *skew;
  AkMatrix    *matrix;
  mat4         sourceSkew, skewResult, matrixResult;
  uint32_t     column, row;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->coordSys = AK_ZUP;

  scene      = ak_heap_calloc(heap, doc, sizeof(*scene));
  root       = ak_heap_calloc(heap, scene, sizeof(*root));
  skewNode   = ak_heap_calloc(heap, scene, sizeof(*skewNode));
  matrixNode = ak_heap_calloc(heap, scene, sizeof(*matrixNode));
  ASSERT(scene && root && skewNode && matrixNode);
  scene->node = root;
  doc->scene  = scene;
  ak_addSubNode(root, skewNode, false);
  ak_addSubNode(root, matrixNode, false);

  skewNode->transform = ak_heap_calloc(heap,
                                       skewNode,
                                       sizeof(*skewNode->transform));
  skewObj = ak_objAlloc(heap, skewNode, sizeof(*skew), AKT_SKEW, true);
  ASSERT(skewNode->transform && skewObj);
  skewNode->transform->item = skewObj;
  skew = ak_objGet(skewObj);
  skew->angle         = glm_rad(27.0f);
  skew->rotateAxis[0] = 1.0f;
  skew->rotateAxis[1] = 2.0f;
  skew->rotateAxis[2] = 3.0f;
  skew->aroundAxis[0] = -2.0f;
  skew->aroundAxis[1] = 1.0f;
  skew->aroundAxis[2] = 0.0f;
  glm_vec3_normalize(skew->rotateAxis);
  glm_vec3_normalize(skew->aroundAxis);
  ak_transformSkewMatrix(skew, sourceSkew[0]);

  matrixNode->transform = ak_heap_calloc(heap,
                                         matrixNode,
                                         sizeof(*matrixNode->transform));
  matrixObj = ak_objAlloc(heap,
                          matrixNode,
                          sizeof(*matrix),
                          AKT_MATRIX,
                          true);
  ASSERT(matrixNode->transform && matrixObj);
  matrixNode->transform->item = matrixObj;
  matrix = ak_objGet(matrixObj);
  memcpy(matrix->val, sourceSkew, sizeof(sourceSkew));

  ak_changeCoordSys(doc, AK_YUP);
  ak_transformCombine(skewNode->transform, skewResult[0]);
  ak_transformCombine(matrixNode->transform, matrixResult[0]);

  for (column = 0; column < 4; column++) {
    for (row = 0; row < 4; row++) {
      ASSERT(ak_test_near(skewResult[column][row],
                          matrixResult[column][row]));
    }
  }

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(coord_change_rotation_handedness_matches_matrix) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkScene          *scene;
  AkNode           *root, *rotateNode, *matrixNode, *partialNode;
  AkObject         *rotateObj, *matrixObj, *partialObj;
  AkRotate         *rotate, *partialRotate;
  AkMatrix         *matrix;
  AkAnimation      *anim;
  AkAnimSampler    *fullSampler, *lastSampler;
  AkChannel        *fullChannel, *lastChannel;
  AkInput          *output;
  AkResolvedTarget *resolved;
  float             axis[3];
  float             sourceValues[4], partialValues[4];
  mat4              sourceMatrix, rotateMatrix, matrixResult;
  mat4              fullMatrix, partialMatrix;
  uint32_t          c, column, row;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->coordSys = AK_YUP;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  rotateNode  = ak_heap_calloc(heap, scene, sizeof(*rotateNode));
  matrixNode  = ak_heap_calloc(heap, scene, sizeof(*matrixNode));
  partialNode = ak_heap_calloc(heap, scene, sizeof(*partialNode));
  ASSERT(scene && root && rotateNode && matrixNode && partialNode);
  scene->node = root;
  doc->scene  = scene;
  ak_addSubNode(root, rotateNode, false);
  ak_addSubNode(root, matrixNode, false);
  ak_addSubNode(root, partialNode, false);

  axis[0] = 1.0f;
  axis[1] = 2.0f;
  axis[2] = 3.0f;
  glm_vec3_normalize(axis);
  sourceValues[0] = axis[0];
  sourceValues[1] = axis[1];
  sourceValues[2] = axis[2];
  sourceValues[3] = glm_rad(37.0f);

  rotateNode->transform = ak_heap_calloc(heap,
                                          rotateNode,
                                          sizeof(*rotateNode->transform));
  rotateObj = ak_objAlloc(heap, rotateNode, sizeof(*rotate), AKT_ROTATE, true);
  ASSERT(rotateNode->transform && rotateObj);
  rotateNode->transform->item = rotateObj;
  rotate = ak_objGet(rotateObj);
  memcpy(rotate->val, sourceValues, sizeof(sourceValues));
  ak_transformCombine(rotateNode->transform, sourceMatrix[0]);

  matrixNode->transform = ak_heap_calloc(heap,
                                          matrixNode,
                                          sizeof(*matrixNode->transform));
  matrixObj = ak_objAlloc(heap, matrixNode, sizeof(*matrix), AKT_MATRIX, true);
  ASSERT(matrixNode->transform && matrixObj);
  matrixNode->transform->item = matrixObj;
  matrix = ak_objGet(matrixObj);
  memcpy(matrix->val, sourceMatrix, sizeof(sourceMatrix));

  partialNode->transform = ak_heap_calloc(heap,
                                           partialNode,
                                           sizeof(*partialNode->transform));
  partialObj = ak_objAlloc(heap,
                           partialNode,
                           sizeof(*partialRotate),
                           AKT_ROTATE,
                           true);
  ASSERT(partialNode->transform && partialObj);
  partialNode->transform->item = partialObj;
  partialRotate = ak_objGet(partialObj);
  memcpy(partialRotate->val, sourceValues, sizeof(sourceValues));

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  fullSampler = ak_heap_calloc(heap, anim, sizeof(*fullSampler));
  fullChannel = ak_heap_calloc(heap, anim, sizeof(*fullChannel));
  output = ak_heap_calloc(heap, fullSampler, sizeof(*output));
  resolved = ak_heap_calloc(heap, fullChannel, sizeof(*resolved));
  ASSERT(anim && fullSampler && fullChannel && output && resolved);
  output->semantic = AK_INPUT_OUTPUT;
  output->accessor = ak_test_make_float_accessor(heap,
                                                  output,
                                                  sourceValues,
                                                  4u,
                                                  1u);
  fullSampler->input = fullSampler->outputInput = output;
  resolved->target = rotateObj;
  fullChannel->source.ptr = fullSampler;
  fullChannel->resolvedTarget = resolved;
  fullChannel->targetType = AK_TARGET_ROTATE;
  anim->sampler = fullSampler;
  anim->channel = fullChannel;
  lastSampler = fullSampler;
  lastChannel = fullChannel;

  for (c = 0; c < 4u; c++) {
    AkAnimSampler *sampler;
    AkChannel     *channel;
    float          value;

    sampler  = ak_heap_calloc(heap, anim, sizeof(*sampler));
    channel  = ak_heap_calloc(heap, anim, sizeof(*channel));
    output   = ak_heap_calloc(heap, sampler, sizeof(*output));
    resolved = ak_heap_calloc(heap, channel, sizeof(*resolved));
    value    = sourceValues[c];
    ASSERT(sampler && channel && output && resolved);
    output->semantic = AK_INPUT_OUTPUT;
    output->accessor = ak_test_make_float_accessor(heap,
                                                    output,
                                                    &value,
                                                    1u,
                                                    1u);
    sampler->input = sampler->outputInput = output;
    resolved->target = partialObj;
    resolved->off = c;
    resolved->isPartial = true;
    channel->source.ptr = sampler;
    channel->resolvedTarget = resolved;
    channel->targetType = AK_TARGET_FLOAT;
    lastSampler->base.next = (AkOneWayIterBase *)sampler;
    lastChannel->next = channel;
    lastSampler = sampler;
    lastChannel = channel;
  }
  doc->lib.animations.first = doc->lib.animations.last = anim;
  doc->lib.animations.count = 1u;

  ak_changeCoordSys(doc, AK_YUP_LH);
  ak_transformCombine(rotateNode->transform, rotateMatrix[0]);
  ak_transformCombine(matrixNode->transform, matrixResult[0]);

  output = fullSampler->outputInput;
  memcpy(sourceValues, output->accessor->buffer->data, sizeof(sourceValues));
  glm_rotate_make(fullMatrix, sourceValues[3], sourceValues);

  memset(partialValues, 0, sizeof(partialValues));
  for (lastChannel = fullChannel->next;
       lastChannel;
       lastChannel = lastChannel->next) {
    const float *value;

    lastSampler = lastChannel->source.ptr;
    output = lastSampler->outputInput;
    value = output->accessor->buffer->data;
    partialValues[lastChannel->resolvedTarget->off] = value[0];
  }
  glm_rotate_make(partialMatrix, partialValues[3], partialValues);

  ASSERT(ak_test_vec3_eq(rotate->val, -axis[0], -axis[1], axis[2]));
  ASSERT(ak_test_near(rotate->val[3], glm_rad(37.0f)));
  ASSERT(ak_test_vec3_eq(sourceValues, -axis[0], -axis[1], axis[2]));
  ASSERT(ak_test_vec3_eq(partialValues, -axis[0], -axis[1], axis[2]));
  ASSERT(ak_test_near(partialValues[3], glm_rad(37.0f)));

  for (column = 0; column < 4u; column++) {
    for (row = 0; row < 4u; row++) {
      ASSERT(ak_test_near(rotateMatrix[column][row],
                          matrixResult[column][row]));
      ASSERT(ak_test_near(fullMatrix[column][row],
                          matrixResult[column][row]));
      ASSERT(ak_test_near(partialMatrix[column][row],
                          matrixResult[column][row]));
    }
  }

  ak_heap_destroy(heap);

  TEST_SUCCESS
}
