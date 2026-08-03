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

static
AkAnimation*
ak_test_find_animation_with_channel(AkAnimation *anim,
                                    const char  *target,
                                    AkChannel  **outChannel) {
  AkAnimation *found;
  AkChannel   *channel;

  for (; anim; anim = anim->next) {
    for (channel = anim->channel; channel; channel = channel->next) {
      if (channel->target && strcmp(channel->target, target) == 0) {
        if (outChannel)
          *outChannel = channel;
        return anim;
      }
    }

    found = ak_test_find_animation_with_channel(anim->animation,
                                                target,
                                                outChannel);
    if (found)
      return found;
  }

  return NULL;
}

static
AkAnimSampler*
ak_test_channel_sampler(AkChannel *channel) {
  return channel ? ak_getObjectByUrl(&channel->source) : NULL;
}

static
const float*
ak_test_input_floats(AkInput *input) {
  AkAccessor *acc;

  acc = input ? input->accessor : NULL;
  if (!acc || !acc->buffer || !acc->buffer->data)
    return NULL;

  return (const float *)((const char *)acc->buffer->data + acc->byteOffset);
}

static
bool
ak_test_float_near(float a, float b) {
  return fabsf(a - b) < 0.0001f;
}

static
AkMeshPrimitive*
ak_test_animation_first_primitive(AkDoc *doc) {
  AkGeometry *geometry;

  for (geometry = doc ? doc->lib.geometries.first : NULL;
       geometry;
       geometry = geometry->next) {
    AkMesh *mesh;

    if (!geometry->gdata || geometry->gdata->type != AK_GEOMETRY_MESH)
      continue;
    mesh = ak_objGet(geometry->gdata);
    if (mesh && mesh->primitive)
      return mesh->primitive;
  }
  return NULL;
}

static
AkInput*
ak_test_animation_input(AkMeshPrimitive *primitive,
                        AkInputSemantic  semantic) {
  AkInput *input;

  for (input = primitive ? primitive->input : NULL; input; input = input->next) {
    if (input->semantic == semantic)
      return input;
  }
  return NULL;
}

static
float
ak_test_animation_accessor_f32(AkAccessor *accessor,
                               uint32_t    row,
                               uint32_t    component) {
  const char *base;
  size_t      stride;

  base   = (const char *)accessor->buffer->data + accessor->byteOffset;
  stride = accessor->byteStride
           ? accessor->byteStride
           : (size_t)accessor->componentCount * sizeof(float);
  return *(const float *)(const void *)(base + (size_t)row * stride
                                       + (size_t)component * sizeof(float));
}

static
float
ak_test_srgb_to_linear_derivative(float channel) {
  if (channel <= 0.04045f)
    return 1.0f / 12.92f;
  return (2.4f / 1.055f)
         * powf((channel + 0.055f) / 1.055f, 1.4f);
}

TEST_IMPL(dae_export_animation_roundtrip) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkAnimation *anim;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-animation-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "anim.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "anim.dae"));

  ASSERT(ak_test_write_dae_animation_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.animations.count == 1);
  heap = ak_heap_getheap(doc);
  anim = doc->lib.animations.first;
  ASSERT(heap != NULL);
  ASSERT(anim != NULL);
  anim->extra = ak_test_dae_extra(heap, anim, "animation-extra");
  ASSERT(anim->extra != NULL);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<library_animations>"));
  ASSERT(ak_test_file_contains(outDae, "<Name_array"));
  ASSERT(ak_test_file_contains(outDae, "<note>animation-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<node id=\"node\" name=\"Node\">"));
  ASSERT(ak_test_file_contains(outDae,
                               "<channel source=\"#animation_0_sampler_0\" "
                               "target=\"node/translate.X\"/>"));

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.animations.count == 1);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_animation_vec3_interpolation_roundtrip) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-animation-vec3-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "anim.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "anim.dae"));

  ASSERT(ak_test_write_dae_animation_vec3_interp(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.animations.count == 1);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<Name_array"));
  ASSERT(ak_test_file_contains(outDae, "_INTERPOLATION_array\" count=\"2\""));
  ASSERT(ak_test_file_contains(outDae, "BEZIER LINEAR"));
  ASSERT(ak_test_file_contains(outDae,
                               "<channel source=\"#animation_0_sampler_0\" "
                               "target=\"node/translate\"/>"));

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.animations.count == 1);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_resolved_transform_animation_roundtrip) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root;
  AkNode          *node;
  AkAnimation     *anim;
  AkAnimSampler   *sampler;
  AkChannel       *channel;
  AkInput         *timeInput;
  AkInput         *valueInput;
  AkResolvedTarget *target;
  AkObject        *translate;
  const char      *outDir  = "./assetkit_export_dae_resolved_anim";
  const char      *daePath = "./assetkit_export_dae_resolved_anim/model.dae";
  const float      times[2] = {0.0f, 1.0f};
  const float      values[6] = {
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
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(ak_setId(node, "moving") == AK_OK);
  ak_addSubNode(root, node, false);

  node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
  ASSERT(node->transform != NULL);
  translate = ak_getTransformTRS(node, AKT_TRANSLATE);
  ASSERT(translate != NULL);

  anim       = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler    = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel    = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput  = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target     = ak_heap_calloc(heap, channel, sizeof(*target));
  ASSERT(anim != NULL);
  ASSERT(sampler != NULL);
  ASSERT(channel != NULL);
  ASSERT(timeInput != NULL);
  ASSERT(valueInput != NULL);
  ASSERT(target != NULL);

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     values,
                                                     3,
                                                     2);
  ASSERT(timeInput->accessor != NULL);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input  = timeInput;
  sampler->inputInput  = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  target->target    = translate;
  target->off       = 0;
  target->isPartial = false;

  channel->source.ptr     = sampler;
  channel->resolvedTarget = target;
  channel->targetType     = AK_TARGET_POSITION;
  anim->sampler           = sampler;
  anim->channel           = channel;

  doc->lib.animations.first = anim;
  doc->lib.animations.last  = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<translate sid=\"translation\">0 0 0</translate>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<channel source=\"#animation_0_sampler_0\" "
                               "target=\"moving/translation\"/>"));
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.animations.count == 1);
  channel = NULL;
  anim = ak_test_find_animation_with_channel(roundTrip->lib.animations.first,
                                             "moving/translation",
                                             &channel);
  ASSERT(anim != NULL);
  ASSERT(channel != NULL);
  ASSERT(channel->targetType == AK_TARGET_POSITION);
  ASSERT(channel->resolvedTarget != NULL);
  ASSERT(channel->resolvedTarget->target != NULL);
  ASSERT(!channel->resolvedTarget->isPartial);
  sampler = ak_test_channel_sampler(channel);
  ASSERT(sampler != NULL);
  ASSERT(sampler->inputInput != NULL);
  ASSERT(sampler->inputInput->accessor != NULL);
  ASSERT(sampler->inputInput->accessor->count == 2);
  ASSERT(ak_test_input_floats(sampler->inputInput) != NULL);
  ASSERT(ak_test_float_near(ak_test_input_floats(sampler->inputInput)[0],
                            0.0f));
  ASSERT(ak_test_float_near(ak_test_input_floats(sampler->inputInput)[1],
                            1.0f));

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_parent_translate_child_rotate_roundtrip) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkDoc            *roundTrip;
  AkScene          *scene;
  AkNode           *root;
  AkNode           *parent;
  AkNode           *child;
  AkAnimation      *anim;
  AkAnimation      *loadedAnim;
  AkAnimSampler    *moveSampler;
  AkAnimSampler    *rotSampler;
  AkAnimSampler    *loadedSampler;
  AkChannel        *moveChannel;
  AkChannel        *rotChannel;
  AkChannel        *loadedChannel;
  AkInput          *moveTimeInput;
  AkInput          *moveValueInput;
  AkInput          *rotTimeInput;
  AkInput          *rotValueInput;
  AkResolvedTarget *moveTarget;
  AkResolvedTarget *rotTarget;
  AkObject         *translate;
  AkObject         *rotateObj;
  AkQuaternion     *rotate;
  const float      *loadedTimes;
  const char       *outDir  = "./assetkit_export_dae_box_anim";
  const char       *daePath = "./assetkit_export_dae_box_anim/model.dae";
  const float       moveTimes[4] = {
    0.0f, 1.25f, 2.5f, 3.708329916f
  };
  const float       moveValues[12] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 2.52f, 0.0f,
    0.0f, 2.52f, 0.0f,
    0.0f, 0.0f, 0.0f
  };
  const float       rotTimes[2] = {
    1.25f, 2.5f
  };
  const float       rotValues[8] = {
    0.0f, 0.0f, 0.0f, -1.0f,
    1.0f, 0.0f, 0.0f,  0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene        = ak_heap_calloc(heap, doc, sizeof(*scene));
  root         = ak_heap_calloc(heap, scene, sizeof(*root));
  parent       = ak_heap_calloc(heap, doc, sizeof(*parent));
  child        = ak_heap_calloc(heap, doc, sizeof(*child));
  scene->node  = root;
  doc->scene   = scene;
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(parent != NULL);
  ASSERT(child != NULL);
  ASSERT(ak_setId(parent, "node0") == AK_OK);
  ASSERT(ak_setId(child, "node2") == AK_OK);
  ak_addSubNode(root, parent, false);
  ak_addSubNode(parent, child, false);

  parent->transform = ak_heap_calloc(heap, parent, sizeof(*parent->transform));
  child->transform  = ak_heap_calloc(heap, child, sizeof(*child->transform));
  ASSERT(parent->transform != NULL);
  ASSERT(child->transform != NULL);
  translate = ak_getTransformTRS(parent, AKT_TRANSLATE);
  rotateObj = ak_getTransformTRS(child, AKT_QUATERNION);
  ASSERT(translate != NULL);
  ASSERT(rotateObj != NULL);
  rotate = ak_objGet(rotateObj);
  ASSERT(rotate != NULL);
  rotate->val[3] = -1.0f;

  anim           = ak_heap_calloc(heap, doc, sizeof(*anim));
  moveSampler    = ak_heap_calloc(heap, anim, sizeof(*moveSampler));
  rotSampler     = ak_heap_calloc(heap, anim, sizeof(*rotSampler));
  moveChannel    = ak_heap_calloc(heap, anim, sizeof(*moveChannel));
  rotChannel     = ak_heap_calloc(heap, anim, sizeof(*rotChannel));
  moveTimeInput  = ak_heap_calloc(heap, moveSampler, sizeof(*moveTimeInput));
  moveValueInput = ak_heap_calloc(heap, moveSampler, sizeof(*moveValueInput));
  rotTimeInput   = ak_heap_calloc(heap, rotSampler, sizeof(*rotTimeInput));
  rotValueInput  = ak_heap_calloc(heap, rotSampler, sizeof(*rotValueInput));
  moveTarget     = ak_heap_calloc(heap, moveChannel, sizeof(*moveTarget));
  rotTarget      = ak_heap_calloc(heap, rotChannel, sizeof(*rotTarget));
  ASSERT(anim != NULL);
  ASSERT(moveSampler != NULL);
  ASSERT(rotSampler != NULL);
  ASSERT(moveChannel != NULL);
  ASSERT(rotChannel != NULL);
  ASSERT(moveTimeInput != NULL);
  ASSERT(moveValueInput != NULL);
  ASSERT(rotTimeInput != NULL);
  ASSERT(rotValueInput != NULL);
  ASSERT(moveTarget != NULL);
  ASSERT(rotTarget != NULL);

  moveTimeInput->semantic = AK_INPUT_INPUT;
  moveTimeInput->accessor = ak_test_make_float_accessor(heap,
                                                        moveTimeInput,
                                                        moveTimes,
                                                        1,
                                                        4);
  moveValueInput->semantic = AK_INPUT_OUTPUT;
  moveValueInput->accessor = ak_test_make_float_accessor(heap,
                                                         moveValueInput,
                                                         moveValues,
                                                         3,
                                                         4);
  rotTimeInput->semantic = AK_INPUT_INPUT;
  rotTimeInput->accessor = ak_test_make_float_accessor(heap,
                                                       rotTimeInput,
                                                       rotTimes,
                                                       1,
                                                       2);
  rotValueInput->semantic = AK_INPUT_OUTPUT;
  rotValueInput->accessor = ak_test_make_float_accessor(heap,
                                                        rotValueInput,
                                                        rotValues,
                                                        4,
                                                        2);
  ASSERT(moveTimeInput->accessor != NULL);
  ASSERT(moveValueInput->accessor != NULL);
  ASSERT(rotTimeInput->accessor != NULL);
  ASSERT(rotValueInput->accessor != NULL);

  moveTimeInput->next = moveValueInput;
  moveSampler->input = moveTimeInput;
  moveSampler->inputInput = moveTimeInput;
  moveSampler->outputInput = moveValueInput;
  moveSampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  rotTimeInput->next = rotValueInput;
  rotSampler->input = rotTimeInput;
  rotSampler->inputInput = rotTimeInput;
  rotSampler->outputInput = rotValueInput;
  rotSampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  moveTarget->target = translate;
  moveTarget->off = 0;
  moveTarget->isPartial = false;
  moveChannel->source.ptr = moveSampler;
  moveChannel->resolvedTarget = moveTarget;
  moveChannel->targetType = AK_TARGET_POSITION;

  rotTarget->target = rotateObj;
  rotTarget->off = 0;
  rotTarget->isPartial = false;
  rotChannel->source.ptr = rotSampler;
  rotChannel->resolvedTarget = rotTarget;
  rotChannel->targetType = AK_TARGET_QUAT;

  moveSampler->base.next = &rotSampler->base;
  moveChannel->next = rotChannel;
  anim->sampler = moveSampler;
  anim->channel = moveChannel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "target=\"node0/translation\""));
  ASSERT(ak_test_file_contains(daePath,
                               "target=\"node2/rotation\""));
  ASSERT(ak_load(&roundTrip, daePath, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);

  loadedChannel = NULL;
  loadedAnim = ak_test_find_animation_with_channel(roundTrip->lib.animations.first,
                                                   "node0/translation",
                                                   &loadedChannel);
  ASSERT(loadedAnim != NULL);
  ASSERT(loadedChannel != NULL);
  ASSERT(loadedChannel->targetType == AK_TARGET_POSITION);
  ASSERT(loadedChannel->resolvedTarget != NULL);
  ASSERT(loadedChannel->resolvedTarget->target != NULL);
  ASSERT(!loadedChannel->resolvedTarget->isPartial);
  loadedSampler = ak_test_channel_sampler(loadedChannel);
  ASSERT(loadedSampler != NULL);
  ASSERT(loadedSampler->inputInput != NULL);
  ASSERT(loadedSampler->inputInput->accessor != NULL);
  ASSERT(loadedSampler->inputInput->accessor->count == 4);
  loadedTimes = ak_test_input_floats(loadedSampler->inputInput);
  ASSERT(loadedTimes != NULL);
  ASSERT(ak_test_float_near(loadedTimes[0], 0.0f));
  ASSERT(ak_test_float_near(loadedTimes[3], 3.70833f));

  loadedChannel = NULL;
  loadedAnim = ak_test_find_animation_with_channel(roundTrip->lib.animations.first,
                                                   "node2/rotation",
                                                   &loadedChannel);
  ASSERT(loadedAnim != NULL);
  ASSERT(loadedChannel != NULL);
  ASSERT(loadedChannel->targetType == AK_TARGET_ROTATE);
  ASSERT(loadedChannel->resolvedTarget != NULL);
  ASSERT(loadedChannel->resolvedTarget->target != NULL);
  ASSERT(!loadedChannel->resolvedTarget->isPartial);
  loadedSampler = ak_test_channel_sampler(loadedChannel);
  ASSERT(loadedSampler != NULL);
  ASSERT(loadedSampler->inputInput != NULL);
  ASSERT(loadedSampler->inputInput->accessor != NULL);
  ASSERT(loadedSampler->inputInput->accessor->count == 2);
  loadedTimes = ak_test_input_floats(loadedSampler->inputInput);
  ASSERT(loadedTimes != NULL);
  ASSERT(ak_test_float_near(loadedTimes[0], 1.25f));
  ASSERT(ak_test_float_near(loadedTimes[1], 2.5f));

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_quaternion_axis_angle_shortest_path) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root;
  AkNode          *node;
  AkAnimation     *anim;
  AkAnimSampler   *sampler;
  AkChannel       *channel;
  AkInput         *timeInput;
  AkInput         *valueInput;
  AkResolvedTarget *target;
  AkObject        *rotateObj;
  AkQuaternion    *rotate;
  const char      *outDir  = "./assetkit_export_dae_quat_short";
  const char      *daePath = "./assetkit_export_dae_quat_short/model.dae";
  const float      times[2] = {0.0f, 1.0f};
  const float      values[8] = {
     0.0f, 0.0f,  0.0f,       -1.0f,
     0.0f, 0.0f, -0.5f, -0.8660254f
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
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(ak_setId(node, "rotating") == AK_OK);
  ak_addSubNode(root, node, false);

  node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
  ASSERT(node->transform != NULL);
  rotateObj = ak_getTransformTRS(node, AKT_QUATERNION);
  ASSERT(rotateObj != NULL);
  rotate = ak_objGet(rotateObj);
  ASSERT(rotate != NULL);
  rotate->val[0] = 0.0f;
  rotate->val[1] = 0.0f;
  rotate->val[2] = 0.0f;
  rotate->val[3] = -1.0f;

  anim       = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler    = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel    = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput  = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target     = ak_heap_calloc(heap, channel, sizeof(*target));
  ASSERT(anim != NULL);
  ASSERT(sampler != NULL);
  ASSERT(channel != NULL);
  ASSERT(timeInput != NULL);
  ASSERT(valueInput != NULL);
  ASSERT(target != NULL);

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     values,
                                                     4,
                                                     2);
  ASSERT(timeInput->accessor != NULL);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input  = timeInput;
  sampler->inputInput  = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  target->target    = rotateObj;
  target->off       = 0;
  target->isPartial = false;

  channel->source.ptr     = sampler;
  channel->resolvedTarget = target;
  channel->targetType     = AK_TARGET_QUAT;
  anim->sampler           = sampler;
  anim->channel           = channel;

  doc->lib.animations.first = anim;
  doc->lib.animations.last  = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "<rotate sid=\"rotation\">0 0 1 0</rotate>"));
  ASSERT(ak_test_file_contains(daePath,
                               "<channel source=\"#animation_0_sampler_0\" "
                               "target=\"rotating/rotation\"/>"));
  ASSERT(!ak_test_file_contains(daePath, ">300"));
  ASSERT(!ak_test_file_contains(daePath, ">360"));
  ASSERT(!ak_test_file_contains(daePath, " 300"));
  ASSERT(!ak_test_file_contains(daePath, " 360"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_quaternion_zero_angle_keeps_axis_hint) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root;
  AkNode          *node;
  AkAnimation     *anim;
  AkAnimSampler   *sampler;
  AkChannel       *channel;
  AkInput         *timeInput;
  AkInput         *valueInput;
  AkResolvedTarget *target;
  AkObject        *rotateObj;
  AkQuaternion    *rotate;
  const char      *outDir  = "./assetkit_export_dae_quat_axis_hint";
  const char      *daePath = "./assetkit_export_dae_quat_axis_hint/model.dae";
  const float      times[2] = {0.0f, 1.0f};
  const float      values[8] = {
    0.0f, 0.0f, 0.0f, -1.0f,
    1.0f, 0.0f, 0.0f,  0.0f
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
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(ak_setId(node, "rotating") == AK_OK);
  ak_addSubNode(root, node, false);

  node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
  ASSERT(node->transform != NULL);
  rotateObj = ak_getTransformTRS(node, AKT_QUATERNION);
  ASSERT(rotateObj != NULL);
  rotate = ak_objGet(rotateObj);
  ASSERT(rotate != NULL);
  rotate->val[0] = 0.0f;
  rotate->val[1] = 0.0f;
  rotate->val[2] = 0.0f;
  rotate->val[3] = -1.0f;

  anim       = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler    = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel    = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput  = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target     = ak_heap_calloc(heap, channel, sizeof(*target));
  ASSERT(anim != NULL);
  ASSERT(sampler != NULL);
  ASSERT(channel != NULL);
  ASSERT(timeInput != NULL);
  ASSERT(valueInput != NULL);
  ASSERT(target != NULL);

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     values,
                                                     4,
                                                     2);
  ASSERT(timeInput->accessor != NULL);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input  = timeInput;
  sampler->inputInput  = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  target->target    = rotateObj;
  target->off       = 0;
  target->isPartial = false;

  channel->source.ptr     = sampler;
  channel->resolvedTarget = target;
  channel->targetType     = AK_TARGET_QUAT;
  anim->sampler           = sampler;
  anim->channel           = channel;

  doc->lib.animations.first = anim;
  doc->lib.animations.last  = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath,
                               "animation_0_sampler_0_OUTPUT_array\" "
                               "count=\"8\">1 0 0 0 1 0 0 180"
                               "</float_array>"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_skips_unsupported_animation) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  AkAnimation *anim;
  AkChannel   *channel;
  const char  *outDir  = "./assetkit_export_dae_skip_bad_anim";
  const char  *daePath = "./assetkit_export_dae_skip_bad_anim/model.dae";
  const float  positions[9] = {
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
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  anim    = ak_heap_calloc(heap, doc, sizeof(*anim));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  ASSERT(anim != NULL);
  ASSERT(channel != NULL);

  anim->channel = channel; /* no target: unsupported, must be skipped */
  doc->lib.animations.first = anim;
  doc->lib.animations.last  = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<library_geometries>"));
  ASSERT(!ak_test_file_contains(daePath, "<library_animations>"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_keeps_supported_child_animation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkGeometry    *geom;
  AkAnimation   *parentAnim;
  AkAnimation   *childAnim;
  AkChannel     *badChannel;
  AkChannel     *channel;
  AkAnimSampler *sampler;
  AkInput       *timeInput;
  AkInput       *valueInput;
  const char    *outDir  = "./assetkit_export_dae_child_anim";
  const char    *daePath = "./assetkit_export_dae_child_anim/model.dae";
  const float    positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float times[2] = {0.0f, 1.0f};
  const float values[2] = {0.0f, 2.0f};

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

  parentAnim = ak_heap_calloc(heap, doc, sizeof(*parentAnim));
  childAnim  = ak_heap_calloc(heap, parentAnim, sizeof(*childAnim));
  badChannel = ak_heap_calloc(heap, parentAnim, sizeof(*badChannel));
  channel    = ak_heap_calloc(heap, childAnim, sizeof(*channel));
  sampler    = ak_heap_calloc(heap, childAnim, sizeof(*sampler));
  timeInput  = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  ASSERT(parentAnim != NULL);
  ASSERT(childAnim != NULL);
  ASSERT(badChannel != NULL);
  ASSERT(channel != NULL);
  ASSERT(sampler != NULL);
  ASSERT(timeInput != NULL);
  ASSERT(valueInput != NULL);

  badChannel->target = NULL; /* unsupported wrapper animation */

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
                                                     values,
                                                     1,
                                                     2);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input  = timeInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  channel->target     = "node/translate.X";
  channel->source.ptr = sampler;

  childAnim->sampler  = sampler;
  childAnim->channel  = channel;
  parentAnim->channel = badChannel;
  parentAnim->animation = childAnim;

  doc->lib.animations.first = parentAnim;
  doc->lib.animations.last  = parentAnim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(daePath, "<library_animations>"));
  ASSERT(ak_test_file_contains(daePath, "<animation id=\"animation_0\">"));
  ASSERT(ak_test_file_contains(daePath,
                               "<channel source=\"#animation_0_sampler_0\" "
                               "target=\"node/translate.X\"/>"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_skin_controller_roundtrip) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-skin-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "skin.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "skin.dae"));

  ASSERT(ak_test_write_dae_skin_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.skins.count == 1);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<library_controllers>"));
  ASSERT(ak_test_file_contains(outDae, "<instance_controller url=\"#skin_0\">"));
  ASSERT(ak_test_file_contains(outDae, "<IDREF_array"));
  ASSERT(ak_test_file_contains(outDae, "<vertex_weights count=\"3\">"));
  ASSERT(ak_test_file_count(outDae, "type=\"JOINT\"") == 2);

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.skins.count == 1);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_multi_primitive_skin_roundtrip) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkSkin      *skin;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-skin-multi-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "skin.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "skin.dae"));

  ASSERT(ak_test_write_dae_skin_multi_primitive(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.skins.count == 1);
  skin = doc->lib.skins.first;
  ASSERT(skin != NULL);
  ASSERT(skin->nPrims == 2);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<vertex_weights count=\"6\">"));
  ASSERT(ak_test_file_count(outDae, "<triangles count=\"1\"") == 2);

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.skins.count == 1);
  ASSERT(roundTrip->lib.skins.first != NULL);
  ASSERT(roundTrip->lib.skins.first->nPrims == 2);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_morph_controller_roundtrip) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkMorph     *morph;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-morph-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "morph.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "morph.dae"));

  ASSERT(ak_test_write_dae_morph_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.morphs.count == 1);
  morph = doc->lib.morphs.first;
  ASSERT(morph != NULL);
  ASSERT(morph->targetCount == 2);
  ASSERT(morph->defaultWeights != NULL);
  ASSERT(morph->defaultWeights->count == 2);
  ASSERT(doc->lib.animations.count == 1);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<library_controllers>"));
  ASSERT(ak_test_file_contains(outDae, "<library_animations>"));
  ASSERT(ak_test_file_contains(outDae, "<instance_controller url=\"#morph_0\">"));
  ASSERT(ak_test_file_contains(outDae, "<IDREF_array"));
  ASSERT(ak_test_file_contains(outDae, "MORPH_TARGET"));
  ASSERT(ak_test_file_contains(outDae, "MORPH_WEIGHT"));
  ASSERT(ak_test_file_contains(outDae, "<input semantic=\"NORMAL\" source=\"#geom_"));
  ASSERT(ak_test_file_contains(outDae, "<input semantic=\"TEXCOORD\" source=\"#geom_"));
  ASSERT(ak_test_file_contains(outDae, "<p>0 0 1 1 2 2</p>"));
  ASSERT(ak_test_file_contains(outDae, "target=\"morph_0_weights(1)\""));

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.morphs.count == 1);
  ASSERT(roundTrip->lib.morphs.first != NULL);
  ASSERT(roundTrip->lib.morphs.first->targetCount == 2);
  ASSERT(roundTrip->lib.animations.count == 1);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_morph_weight_vector_animation_split) {
  static const float vectorWeights[] = {
    0.10f, 0.90f,
    0.30f, 0.70f
  };

  AkDoc         *doc;
  AkDoc         *roundTrip;
  AkAnimation   *anim;
  AkChannel     *channel;
  AkAnimSampler *sampler;
  AkAccessor    *outputAcc;
  AkBuffer      *outputBuff;
  char           dirTemplate[PATH_MAX];
  char          *tmpdir;
  char           daePath[PATH_MAX];
  char           outDir[PATH_MAX];
  char           outDae[PATH_MAX];
  const char    *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-morph-vector-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "morph.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "morph.dae"));

  ASSERT(ak_test_write_dae_morph_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.morphs.first != NULL);
  ASSERT(doc->lib.morphs.first->targetCount == 2);
  ASSERT(doc->lib.animations.first != NULL);

  anim = doc->lib.animations.first;
  channel = anim->channel;
  ASSERT(channel != NULL);
  sampler = ak_getObjectByUrl(&channel->source);
  ASSERT(sampler != NULL);
  ASSERT(sampler->outputInput != NULL);
  outputAcc = sampler->outputInput->accessor;
  ASSERT(outputAcc != NULL && outputAcc->buffer != NULL);
  outputBuff = outputAcc->buffer;

  channel->target = NULL;
  ASSERT(channel->resolvedTarget != NULL);
  channel->resolvedTarget->off = 0;

  outputBuff->data          = (void *)vectorWeights;
  outputBuff->length        = sizeof(vectorWeights);
  outputAcc->count          = 2;
  outputAcc->componentCount = 2;
  outputAcc->componentType  = AKT_FLOAT;
  outputAcc->bytesPerComponent = sizeof(float);
  outputAcc->fillByteSize   = sizeof(float) * 2u;
  outputAcc->byteStride     = outputAcc->fillByteSize;
  outputAcc->byteOffset     = 0;
  outputAcc->byteLength     = sizeof(vectorWeights);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_count(outDae, "<channel source=\"#") == 2);
  ASSERT(ak_test_file_contains(outDae, "target=\"morph_0_weights(0)\""));
  ASSERT(ak_test_file_contains(outDae, "target=\"morph_0_weights(1)\""));
  ASSERT(ak_test_file_contains(outDae, "animation_0_sampler_0_weight_0"));
  ASSERT(ak_test_file_contains(outDae, "animation_0_sampler_0_weight_1"));

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.animations.count == 1);
  ASSERT(roundTrip->lib.animations.first != NULL);
  ASSERT(ak_test_file_count(outDae,
                            "<source id=\"animation_0_sampler_0_weight_") == 2);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_morph_weight_flat_animation_split) {
  static const float flatWeights[] = {
    0.10f, 0.90f,
    0.30f, 0.70f
  };

  AkDoc         *doc;
  AkDoc         *roundTrip;
  AkAnimation   *anim;
  AkChannel     *channel;
  AkAnimSampler *sampler;
  AkAccessor    *outputAcc;
  AkBuffer      *outputBuff;
  char           dirTemplate[PATH_MAX];
  char          *tmpdir;
  char           daePath[PATH_MAX];
  char           outDir[PATH_MAX];
  char           outDae[PATH_MAX];
  const char    *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-morph-flat-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "morph.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "morph.dae"));

  ASSERT(ak_test_write_dae_morph_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.morphs.first != NULL);
  ASSERT(doc->lib.morphs.first->targetCount == 2);
  ASSERT(doc->lib.animations.first != NULL);

  anim = doc->lib.animations.first;
  channel = anim->channel;
  ASSERT(channel != NULL);
  sampler = ak_getObjectByUrl(&channel->source);
  ASSERT(sampler != NULL);
  ASSERT(sampler->outputInput != NULL);
  outputAcc = sampler->outputInput->accessor;
  ASSERT(outputAcc != NULL && outputAcc->buffer != NULL);
  outputBuff = outputAcc->buffer;

  channel->target = NULL;
  ASSERT(channel->resolvedTarget != NULL);
  channel->resolvedTarget->off = 0;

  outputBuff->data          = (void *)flatWeights;
  outputBuff->length        = sizeof(flatWeights);
  outputAcc->count          = 4;
  outputAcc->componentCount = 1;
  outputAcc->componentType  = AKT_FLOAT;
  outputAcc->bytesPerComponent = sizeof(float);
  outputAcc->fillByteSize   = sizeof(float);
  outputAcc->byteStride     = outputAcc->fillByteSize;
  outputAcc->byteOffset     = 0;
  outputAcc->byteLength     = sizeof(flatWeights);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_count(outDae, "<channel source=\"#") == 2);
  ASSERT(ak_test_file_contains(outDae, "target=\"morph_0_weights(0)\""));
  ASSERT(ak_test_file_contains(outDae, "target=\"morph_0_weights(1)\""));
  ASSERT(ak_test_file_contains(outDae, "animation_0_sampler_0_weight_0"));
  ASSERT(ak_test_file_contains(outDae, "animation_0_sampler_0_weight_1"));
  ASSERT(ak_test_file_count(outDae,
                            "<source id=\"animation_0_sampler_0_weight_") == 2);

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.animations.count == 1);
  ASSERT(roundTrip->lib.animations.first != NULL);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_load_invalid_morph_target_skips_morph_channel) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc       = NULL;
  roundTrip = NULL;
  tmpBase   = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-invalid-morph-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath,
                           sizeof(daePath),
                           tmpdir,
                           "invalid_morph.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae,
                           sizeof(outDae),
                           outDir,
                           "invalid_morph.dae"));

  ASSERT(ak_test_write_dae_invalid_morph_target(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.morphs.count == 0);
  ASSERT(doc->lib.animations.count == 1);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.morphs.count == 0);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_scenekit_color_carriers_are_linear) {
  AkDoc           *doc;
  AkMeshPrimitive *primitive;
  AkInput         *colorInput;
  AkAccessor      *colorAccessor;
  AkAnimation     *animation;
  AkChannel       *channel;
  AkAnimSampler   *sampler;
  AkAccessor      *outputAccessor;
  AkAccessor      *inTangentAccessor;
  AkAccessor      *outTangentAccessor;
  AkLight         *light;
  char             dirTemplate[PATH_MAX];
  char            *tmpdir;
  char             daePath[PATH_MAX];
  const char      *tmpBase;

  doc     = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-scenekit-colors-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath,
                           sizeof(daePath),
                           tmpdir,
                           "scenekit_colors.dae"));
  ASSERT(ak_test_write_dae_srgb_color_carriers(
           daePath,
           "SceneKit Collada Exporter v1.0"));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);

  primitive = ak_test_animation_first_primitive(doc);
  ASSERT(primitive != NULL);
  colorInput = ak_test_animation_input(primitive, AK_INPUT_COLOR);
  ASSERT(colorInput != NULL && colorInput->accessor != NULL);
  colorAccessor = colorInput->accessor;
  ASSERT(fabsf(ak_test_animation_accessor_f32(colorAccessor, 0u, 0u)
               - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(colorAccessor, 0u, 1u)
               - ak_sRGB_linearf(0.1f)) < 0.0001f);
  ASSERT(ak_test_animation_accessor_f32(colorAccessor, 1u, 3u) == 0.5f);

  light = doc->lib.lights.first;
  ASSERT(light != NULL && light->data != NULL);
  ASSERT(fabsf(light->data->color.rgba.R - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(fabsf(light->data->color.rgba.G - ak_sRGB_linearf(0.1f)) < 0.0001f);
  ASSERT(light->data->color.rgba.A == 1.0f);

  channel = NULL;
  animation = ak_test_find_animation_with_channel(doc->lib.animations.first,
                                                   "light/color",
                                                   &channel);
  ASSERT(animation != NULL && channel != NULL);
  ASSERT(channel->targetType == AK_TARGET_COLOR);
  ASSERT(channel->resolvedTarget != NULL);
  ASSERT(channel->resolvedTarget->target != NULL);
  ASSERT(!channel->resolvedTarget->isPartial);

  sampler = ak_test_channel_sampler(channel);
  ASSERT(sampler != NULL && sampler->outputInput != NULL);
  outputAccessor = sampler->outputInput->accessor;
  ASSERT(outputAccessor != NULL);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outputAccessor, 0u, 0u)
               - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outputAccessor, 1u, 2u)
               - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(ak_test_animation_accessor_f32(outputAccessor, 1u, 3u) == 0.5f);

  animation = ak_test_find_animation_with_channel(doc->lib.animations.first,
                                                   "bezier-light/color",
                                                   &channel);
  ASSERT(animation != NULL && channel != NULL);
  ASSERT(channel->targetType == AK_TARGET_COLOR);
  sampler = ak_test_channel_sampler(channel);
  ASSERT(sampler != NULL
         && sampler->outputInput != NULL
         && sampler->inTangentInput != NULL
         && sampler->outTangentInput != NULL);
  outputAccessor     = sampler->outputInput->accessor;
  inTangentAccessor = sampler->inTangentInput->accessor;
  outTangentAccessor = sampler->outTangentInput->accessor;
  ASSERT(outputAccessor != NULL
         && inTangentAccessor != NULL
         && outTangentAccessor != NULL);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outputAccessor, 0u, 0u)
               - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(ak_test_animation_accessor_f32(inTangentAccessor, 0u, 0u) == -0.1f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(inTangentAccessor, 0u, 1u)
               - ak_sRGB_linearf(0.3f)) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(inTangentAccessor, 1u, 3u)
               - ak_sRGB_linearf(0.3f)) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(inTangentAccessor, 0u, 4u)
               - 0.9f) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outTangentAccessor, 0u, 0u)
               - 0.2f) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outTangentAccessor, 0u, 1u)
               - ak_sRGB_linearf(0.6f)) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outTangentAccessor, 1u, 4u)
               - 0.6f) < 0.0001f);

  animation = ak_test_find_animation_with_channel(doc->lib.animations.first,
                                                   "hermite-light/color.R",
                                                   &channel);
  ASSERT(animation != NULL && channel != NULL);
  ASSERT(channel->targetType == AK_TARGET_FLOAT);
  ASSERT(channel->resolvedTarget != NULL
         && channel->resolvedTarget->isPartial
         && channel->resolvedTarget->off == 0u);
  sampler = ak_test_channel_sampler(channel);
  ASSERT(sampler != NULL
         && sampler->outputInput != NULL
         && sampler->inTangentInput != NULL
         && sampler->outTangentInput != NULL);
  outputAccessor      = sampler->outputInput->accessor;
  inTangentAccessor   = sampler->inTangentInput->accessor;
  outTangentAccessor  = sampler->outTangentInput->accessor;
  ASSERT(outputAccessor != NULL
         && inTangentAccessor != NULL
         && outTangentAccessor != NULL);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outputAccessor, 0u, 0u)
               - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(ak_test_animation_accessor_f32(inTangentAccessor, 0u, 0u) == -0.25f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(inTangentAccessor, 0u, 1u)
               - 0.5f * ak_test_srgb_to_linear_derivative(0.4f)) < 0.0001f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(inTangentAccessor, 1u, 1u)
               - 0.25f * ak_test_srgb_to_linear_derivative(0.2f)) < 0.0001f);
  ASSERT(ak_test_animation_accessor_f32(outTangentAccessor, 1u, 0u) == 1.25f);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outTangentAccessor, 0u, 1u)
               - 0.75f * ak_test_srgb_to_linear_derivative(0.4f)) < 0.0001f);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_sketchup_color_carriers_are_linear) {
  AkDoc           *doc;
  AkMeshPrimitive *primitive;
  AkInput         *colorInput;
  AkMaterial      *material;
  AkLight         *light;
  AkAnimation     *animation;
  AkChannel       *channel;
  AkAnimSampler   *sampler;
  AkAccessor      *outputAccessor;
  char             dirTemplate[PATH_MAX];
  char            *tmpdir;
  char             daePath[PATH_MAX];
  const char      *tmpBase;

  doc     = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-sketchup-colors-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath,
                           sizeof(daePath),
                           tmpdir,
                           "sketchup_colors.dae"));
  ASSERT(ak_test_write_dae_srgb_color_carriers(daePath, "SketchUp 17.0.1"));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);

  material = doc->lib.materials.first;
  ASSERT(material != NULL
         && material->surface != NULL
         && material->surface->baseColor != NULL);
  ASSERT(fabsf(material->surface->baseColor->color.rgba.R
               - ak_sRGB_linearf(0.8823529f)) < 0.0001f);
  ASSERT(fabsf(material->surface->baseColor->color.rgba.B
               - ak_sRGB_linearf(0.7843137f)) < 0.0001f);
  ASSERT(material->surface->baseColor->color.rgba.A == 1.0f);

  primitive = ak_test_animation_first_primitive(doc);
  ASSERT(primitive != NULL);
  colorInput = ak_test_animation_input(primitive, AK_INPUT_COLOR);
  ASSERT(colorInput != NULL && colorInput->accessor != NULL);
  ASSERT(fabsf(ak_test_animation_accessor_f32(colorInput->accessor, 0u, 0u)
               - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(ak_test_animation_accessor_f32(colorInput->accessor, 1u, 3u) == 0.5f);

  light = doc->lib.lights.first;
  ASSERT(light != NULL && light->data != NULL);
  ASSERT(fabsf(light->data->color.rgba.R - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(light->data->color.rgba.A == 1.0f);

  channel = NULL;
  animation = ak_test_find_animation_with_channel(doc->lib.animations.first,
                                                   "light/color",
                                                   &channel);
  ASSERT(animation != NULL && channel != NULL);
  sampler = ak_test_channel_sampler(channel);
  ASSERT(sampler != NULL && sampler->outputInput != NULL);
  outputAccessor = sampler->outputInput->accessor;
  ASSERT(outputAccessor != NULL);
  ASSERT(fabsf(ak_test_animation_accessor_f32(outputAccessor, 0u, 0u)
               - ak_sRGB_linearf(0.4f)) < 0.0001f);
  ASSERT(ak_test_animation_accessor_f32(outputAccessor, 1u, 3u) == 0.5f);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_linear_color_carriers_stay_linear) {
  AkDoc           *doc;
  AkMeshPrimitive *primitive;
  AkInput         *colorInput;
  AkMaterial      *material;
  AkLight         *light;
  AkAnimation     *animation;
  AkChannel       *channel;
  AkAnimSampler   *sampler;
  char             dirTemplate[PATH_MAX];
  char            *tmpdir;
  char             daePath[PATH_MAX];
  const char      *tmpBase;

  doc     = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-linear-colors-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath,
                           sizeof(daePath),
                           tmpdir,
                           "linear_colors.dae"));
  ASSERT(ak_test_write_dae_srgb_color_carriers(daePath, "Blender 5.0.0"));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);

  material = doc->lib.materials.first;
  ASSERT(material != NULL
         && material->surface != NULL
         && material->surface->baseColor != NULL);
  ASSERT(fabsf(material->surface->baseColor->color.rgba.R - 0.8823529f)
         < 0.0001f);
  ASSERT(fabsf(material->surface->baseColor->color.rgba.B - 0.7843137f)
         < 0.0001f);

  primitive = ak_test_animation_first_primitive(doc);
  ASSERT(primitive != NULL);
  colorInput = ak_test_animation_input(primitive, AK_INPUT_COLOR);
  ASSERT(colorInput != NULL && colorInput->accessor != NULL);
  ASSERT(fabsf(ak_test_animation_accessor_f32(colorInput->accessor, 0u, 0u)
               - 0.4f) < 0.0001f);

  light = doc->lib.lights.first;
  ASSERT(light != NULL && light->data != NULL);
  ASSERT(fabsf(light->data->color.rgba.R - 0.4f) < 0.0001f);

  channel = NULL;
  animation = ak_test_find_animation_with_channel(doc->lib.animations.first,
                                                   "light/color",
                                                   &channel);
  ASSERT(animation != NULL && channel != NULL);
  sampler = ak_test_channel_sampler(channel);
  ASSERT(sampler != NULL
         && sampler->outputInput != NULL
         && sampler->outputInput->accessor != NULL);
  ASSERT(fabsf(ak_test_animation_accessor_f32(sampler->outputInput->accessor,
                                              0u,
                                              0u)
               - 0.4f) < 0.0001f);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_scenekit_profile_without_authoring_is_linear) {
  AkDoc      *doc;
  AkMaterial *material;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        daePath[PATH_MAX];
  const char *tmpBase;

  doc     = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-scenekit-profile-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath,
                           sizeof(daePath),
                           tmpdir,
                           "scenekit_profile.dae"));
  ASSERT(ak_test_write_dae_srgb_color_carriers(daePath, NULL));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);

  material = doc->lib.materials.first;
  ASSERT(material != NULL
         && material->surface != NULL
         && material->surface->baseColor != NULL);
  ASSERT(fabsf(material->surface->baseColor->color.rgba.R
               - ak_sRGB_linearf(0.8823529f)) < 0.0001f);
  ASSERT(fabsf(material->surface->baseColor->color.rgba.B
               - ak_sRGB_linearf(0.7843137f)) < 0.0001f);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
