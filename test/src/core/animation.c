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

typedef struct AkTestBakeFixture {
  AkHeap          *heap;
  AkDoc           *doc;
  AkNode          *node;
  AkObject        *translate;
  AkAnimation     *animation;
  AkAnimSampler   *sampler;
  AkChannel       *channel;
  AkAnimationClip *clip;
} AkTestBakeFixture;

static AkAccessor *
ak_test_make_padded_accessor(AkHeap      *heap,
                             void        *parent,
                             const void  *values,
                             AkTypeId     componentType,
                             size_t       componentBytes,
                             uint32_t     components,
                             uint32_t     count,
                             size_t       prefix,
                             size_t       padding) {
  AkAccessor    *accessor;
  AkBuffer      *buffer;
  unsigned char *data;
  size_t         rowBytes, stride, length, row;

  if (!components || !count
      || components > SIZE_MAX / componentBytes)
    return NULL;
  rowBytes = (size_t)components * componentBytes;
  if (rowBytes > SIZE_MAX - padding)
    return NULL;
  stride = rowBytes + padding;
  if ((size_t)(count - 1u) > (SIZE_MAX - prefix - rowBytes) / stride)
    return NULL;
  length = prefix + (size_t)(count - 1u) * stride + rowBytes;

  accessor = ak_heap_calloc(heap, parent, sizeof(*accessor));
  buffer   = ak_heap_calloc(heap, accessor, sizeof(*buffer));
  data     = ak_heap_calloc(heap, buffer, length);
  if (!accessor || !buffer || !data)
    return NULL;
  for (row = 0u; row < count; row++) {
    memcpy(data + prefix + row * stride,
           (const unsigned char *)values + row * rowBytes,
           rowBytes);
  }

  buffer->data                  = data;
  buffer->length                = length;
  accessor->buffer              = buffer;
  accessor->byteOffset          = prefix;
  accessor->byteStride          = stride;
  accessor->byteLength          = length - prefix;
  accessor->count               = count;
  accessor->bytesPerComponent   = (uint32_t)componentBytes;
  accessor->componentType       = componentType;
  accessor->originalComponentType = componentType;
  accessor->componentCount      = components;
  accessor->fillByteSize        = rowBytes;
  return accessor;
}

static bool
ak_test_bake_fixture_init(AkTestBakeFixture *fixture,
                          const float       *times,
                          uint32_t           keyCount,
                          const float       *values,
                          uint32_t           valueComponents,
                          bool               partial) {
  AkInput          *timeInput, *valueInput;
  AkResolvedTarget *target;
  AkAnimationClipMember *member;

  memset(fixture, 0, sizeof(*fixture));
  fixture->heap = ak_heap_new(NULL, NULL, NULL);
  if (!fixture->heap)
    return false;
  fixture->doc = ak_heap_calloc(fixture->heap, NULL, sizeof(*fixture->doc));
  ak_heap_setdata(fixture->heap, fixture->doc);
  fixture->node = ak_heap_calloc(fixture->heap,
                                 fixture->doc,
                                 sizeof(*fixture->node));
  fixture->node->transform = ak_heap_calloc(fixture->heap,
                                             fixture->node,
                                             sizeof(*fixture->node->transform));
  fixture->translate = ak_getTransformTRS(fixture->node, AKT_TRANSLATE);
  fixture->animation = ak_heap_calloc(fixture->heap,
                                      fixture->doc,
                                      sizeof(*fixture->animation));
  fixture->sampler = ak_heap_calloc(fixture->heap,
                                    fixture->animation,
                                    sizeof(*fixture->sampler));
  fixture->channel = ak_heap_calloc(fixture->heap,
                                    fixture->animation,
                                    sizeof(*fixture->channel));
  timeInput  = ak_heap_calloc(fixture->heap,
                              fixture->sampler,
                              sizeof(*timeInput));
  valueInput = ak_heap_calloc(fixture->heap,
                              fixture->sampler,
                              sizeof(*valueInput));
  target = ak_heap_calloc(fixture->heap,
                          fixture->channel,
                          sizeof(*target));
  fixture->clip = ak_heap_calloc(fixture->heap,
                                 fixture->doc,
                                 sizeof(*fixture->clip));
  member = ak_heap_calloc(fixture->heap,
                          fixture->clip,
                          sizeof(*member));
  if (!fixture->doc || !fixture->node || !fixture->node->transform
      || !fixture->translate || !fixture->animation || !fixture->sampler
      || !fixture->channel || !timeInput || !valueInput || !target
      || !fixture->clip || !member)
    return false;

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_padded_accessor(fixture->heap,
                                                     timeInput,
                                                     times,
                                                     AKT_FLOAT,
                                                     sizeof(float),
                                                     1u,
                                                     keyCount,
                                                     1u,
                                                     3u);
  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_padded_accessor(fixture->heap,
                                                      valueInput,
                                                      values,
                                                      AKT_FLOAT,
                                                      sizeof(float),
                                                      valueComponents,
                                                      keyCount,
                                                      2u,
                                                      5u);
  if (!timeInput->accessor || !valueInput->accessor)
    return false;
  timeInput->next = valueInput;
  fixture->sampler->input       = timeInput;
  fixture->sampler->inputInput  = timeInput;
  fixture->sampler->outputInput = valueInput;

  target->target    = fixture->translate;
  target->off       = 0u;
  target->isPartial = partial;
  fixture->channel->source.ptr     = fixture->sampler;
  fixture->channel->resolvedTarget = target;
  fixture->channel->targetType = partial ? AK_TARGET_FLOAT
                                         : AK_TARGET_POSITION;
  fixture->animation->sampler = fixture->sampler;
  fixture->animation->channel = fixture->channel;
  fixture->doc->lib.animations.first = fixture->animation;
  fixture->doc->lib.animations.last  = fixture->animation;
  fixture->doc->lib.animations.count = 1u;

  member->animation = fixture->animation;
  fixture->clip->members     = member;
  fixture->clip->lastMember  = member;
  fixture->clip->memberCount = 1u;
  fixture->clip->hasStart    = true;
  fixture->clip->hasEnd      = true;
  return true;
}

static void
ak_test_bake_fixture_destroy(AkTestBakeFixture *fixture) {
  if (fixture && fixture->heap)
    ak_heap_destroy(fixture->heap);
}

static float
ak_test_baked_translation(AkTestBakeFixture *fixture, float time) {
  AkBakedAnimation *baked;
  float             value;

  fixture->clip->start = time;
  fixture->clip->end   = time;
  baked = ak_nodeBakeAnimationForClip(fixture->doc,
                                      fixture->node,
                                      fixture->clip);
  if (!baked || baked->count != 1u) {
    if (baked) ak_free(baked);
    return NAN;
  }
  value = baked->matrices[12];
  ak_free(baked);
  return value;
}

static AkAnimSampler *
ak_test_make_time_sampler(AkHeap     *heap,
                          void       *parent,
                          const float *times,
                          uint32_t    count) {
  AkAnimSampler *sampler;
  AkInput       *timeInput;
  AkInput       *valueInput;
  static const float values[2] = {0.0f, 1.0f};

  sampler    = ak_heap_calloc(heap, parent, sizeof(*sampler));
  timeInput  = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  if (!sampler || !timeInput || !valueInput)
    return NULL;

  timeInput->semantic  = AK_INPUT_INPUT;
  timeInput->accessor  = ak_test_make_float_accessor(heap,
                                                     timeInput,
                                                     times,
                                                     1,
                                                     count);
  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     values,
                                                     1,
                                                     2);
  if (!timeInput->accessor || !valueInput->accessor)
    return NULL;

  timeInput->next = valueInput;
  sampler->input  = timeInput;
  return sampler;
}

static AkAnimation *
ak_test_make_time_animation(AkHeap      *heap,
                            void        *parent,
                            const float *times,
                            uint32_t     count) {
  AkAnimation *anim;
  AkChannel   *channel;

  anim    = ak_heap_calloc(heap, parent, sizeof(*anim));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  if (!anim || !channel)
    return NULL;

  anim->sampler       = ak_test_make_time_sampler(heap, anim, times, count);
  channel->source.ptr = anim->sampler;
  anim->channel       = channel;
  return anim;
}

static bool
ak_test_set_strided_time_accessor(AkHeap       *heap,
                                  AkAnimation  *animation,
                                  const float  *times,
                                  uint32_t      count,
                                  size_t        byteOffset,
                                  size_t        byteStride) {
  AkInput    *input;
  AkAccessor *accessor;
  AkBuffer   *buffer;
  size_t      required, length;
  uint32_t    i;

  if (!animation || !animation->sampler || !times || count == 0
      || byteStride < sizeof(float))
    return false;
  input = animation->sampler->input;
  if (!input || input->semantic != AK_INPUT_INPUT)
    return false;

  required = (size_t)(count - 1u) * byteStride + sizeof(float);
  if (byteOffset > SIZE_MAX - required)
    return false;
  length = byteOffset + required;
  accessor = ak_heap_calloc(heap, input, sizeof(*accessor));
  buffer   = ak_heap_calloc(heap, accessor, sizeof(*buffer));
  if (!accessor || !buffer)
    return false;
  buffer->data = ak_heap_calloc(heap, buffer, length);
  if (!buffer->data)
    return false;

  for (i = 0; i < count; i++) {
    memcpy((unsigned char *)buffer->data
           + byteOffset + (size_t)i * byteStride,
           &times[i],
           sizeof(times[i]));
  }

  buffer->length                  = length;
  accessor->buffer                = buffer;
  accessor->byteOffset            = byteOffset;
  accessor->byteLength            = required;
  accessor->byteStride            = byteStride;
  accessor->fillByteSize          = sizeof(float);
  accessor->bytesPerComponent     = sizeof(float);
  accessor->componentSize         = AK_COMPONENT_SIZE_SCALAR;
  accessor->componentType         = AKT_FLOAT;
  accessor->originalComponentType = AKT_FLOAT;
  accessor->componentCount        = 1u;
  accessor->count                 = count;
  input->accessor                 = accessor;
  return true;
}

TEST_IMPL(animation_time_range_walks_child_tree) {
  AkHeap      *heap;
  AkAnimation *root;
  AkAnimation *childA;
  AkAnimation *childB;
  float        start, end;
  const float  rootTimes[2]  = {1.0f, 3.0f};
  const float  childATimes[3] = {5.0f, -2.0f, 3.0f};
  const float  childBTimes[2] = {4.0f, 7.5f};

  heap = ak_heap_new(NULL, NULL, NULL);
  ASSERT(heap != NULL);

  root   = ak_test_make_time_animation(heap, NULL, rootTimes, 2);
  childA = ak_test_make_time_animation(heap, root, childATimes, 3);
  childB = ak_test_make_time_animation(heap, root, childBTimes, 2);
  ASSERT(root != NULL);
  ASSERT(childA != NULL);
  ASSERT(childB != NULL);
  /* The timeline must honor an unaligned, padded accessor and compute the
   * actual range rather than assuming sorted endpoint samples. */
  ASSERT(ak_test_set_strided_time_accessor(heap,
                                           childA,
                                           childATimes,
                                           3,
                                           1,
                                           8));

  root->animation = childA;
  childA->next    = childB;

  ASSERT(ak_animationTimeRange(root, &start, &end));
  ASSERT(start == -2.0f);
  ASSERT(end == 7.5f);

  ak_heap_destroy(heap);
  TEST_SUCCESS
}

TEST_IMPL(animation_bake_samples_stride_and_curves) {
  AkTestBakeFixture fixture;
  AkInput          *interpInput, *inTangentInput, *outTangentInput;
  const float       mixedTimes[3]  = {0.0f, 1.0f, 2.0f};
  const float       mixedValues[3] = {0.0f, 1.0f, 3.0f};
  const float       times[2] = {0.0f, 1.0f};
  const float       values[2] = {0.0f, 1.0f};
  const float       bezierIn[2]  = {0.0f, 0.0f};
  const float       bezierOut[2] = {1.0f, 1.0f};
  const float       pairedIn[4]  = {0.0f, 0.0f, 2.0f / 3.0f, 1.0f};
  const float       pairedOut[4] = {1.0f / 3.0f, 1.0f, 1.0f, 1.0f};
  const float       pairedVecIn[8]  = {0.0f, 0.0f, 0.0f, 0.0f,
                                       2.0f / 3.0f, 1.0f,
                                       2.0f / 3.0f, 1.0f};
  const float       pairedVecOut[8] = {1.0f / 3.0f, 1.0f,
                                       1.0f / 3.0f, 1.0f,
                                       1.0f, 1.0f, 1.0f, 1.0f};
  const float       vectorValues[4] = {0.0f, 0.0f, 1.0f, 1.0f};
  const float       flattenedValues[4] = {0.0f, 10.0f, 2.0f, 14.0f};
  const float       packedHermite[6] = {0.0f, 0.0f, 0.0f,
                                         0.0f, 1.0f, 0.0f};
  const float       hermiteIn[2]  = {0.0f, 0.0f};
  const float       hermiteOut[2] = {0.0f, 0.0f};
  const float       pairedHermiteIn[4]  = {1.0f, 6.0f, 1.0f, 6.0f};
  const float       pairedHermiteOut[4] = {1.0f, 6.0f, 1.0f, 6.0f};
  const uint8_t     mixed[3] = {
    AK_INTERPOLATION_STEP,
    AK_INTERPOLATION_LINEAR,
    AK_INTERPOLATION_LINEAR
  };
  const uint8_t     bezier[2] = {
    AK_INTERPOLATION_BEZIER, AK_INTERPOLATION_BEZIER
  };
  const uint8_t     hermite[2] = {
    AK_INTERPOLATION_HERMITE, AK_INTERPOLATION_HERMITE
  };
  float             sampled;

  ASSERT(ak_test_bake_fixture_init(&fixture,
                                   mixedTimes,
                                   3u,
                                   mixedValues,
                                   1u,
                                   true));
  interpInput = ak_heap_calloc(fixture.heap,
                               fixture.sampler,
                               sizeof(*interpInput));
  ASSERT(interpInput != NULL);
  interpInput->semantic = AK_INPUT_INTERPOLATION;
  interpInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                       interpInput,
                                                       mixed,
                                                       AKT_UBYTE,
                                                       sizeof(uint8_t),
                                                       1u,
                                                       3u,
                                                       1u,
                                                       2u);
  ASSERT(interpInput->accessor != NULL);
  fixture.sampler->interpInput = interpInput;
  interpInput->next = fixture.sampler->input;
  fixture.sampler->input = interpInput;

  sampled = ak_test_baked_translation(&fixture, 0.5f);
  ASSERT(isfinite(sampled) && fabsf(sampled) < 0.00001f);
  sampled = ak_test_baked_translation(&fixture, 1.0f);
  ASSERT(isfinite(sampled) && fabsf(sampled - 1.0f) < 0.00001f);
  sampled = ak_test_baked_translation(&fixture, 1.5f);
  ASSERT(isfinite(sampled) && fabsf(sampled - 2.0f) < 0.0002f);

  ak_test_bake_fixture_destroy(&fixture);
  ASSERT(ak_test_bake_fixture_init(&fixture,
                                   times,
                                   2u,
                                   values,
                                   1u,
                                   true));
  interpInput = ak_heap_calloc(fixture.heap,
                               fixture.sampler,
                               sizeof(*interpInput));
  ASSERT(interpInput != NULL);
  interpInput->semantic = AK_INPUT_INTERPOLATION;
  interpInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                       interpInput,
                                                       bezier,
                                                       AKT_UBYTE,
                                                       sizeof(uint8_t),
                                                       1u, 2u, 1u, 2u);
  ASSERT(interpInput->accessor != NULL);
  fixture.sampler->interpInput = interpInput;
  interpInput->next = fixture.sampler->input;
  fixture.sampler->input = interpInput;

  /* COLLADA's one-dimensional Bézier special case stores control values
     without times. At t=.25 this curve is 0.4375, not the linear .25. */
  inTangentInput  = ak_heap_calloc(fixture.heap,
                                   fixture.sampler,
                                   sizeof(*inTangentInput));
  outTangentInput = ak_heap_calloc(fixture.heap,
                                   fixture.sampler,
                                   sizeof(*outTangentInput));
  ASSERT(inTangentInput && outTangentInput);
  inTangentInput->semantic  = AK_INPUT_IN_TANGENT;
  outTangentInput->semantic = AK_INPUT_OUT_TANGENT;
  inTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                          inTangentInput,
                                                          bezierIn,
                                                          AKT_FLOAT,
                                                          sizeof(float),
                                                          1u, 2u, 1u, 1u);
  outTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                           outTangentInput,
                                                           bezierOut,
                                                           AKT_FLOAT,
                                                           sizeof(float),
                                                           1u, 2u, 3u, 1u);
  ASSERT(inTangentInput->accessor && outTangentInput->accessor);
  fixture.sampler->inTangentInput  = inTangentInput;
  fixture.sampler->outTangentInput = outTangentInput;
  inTangentInput->next = fixture.sampler->input;
  fixture.sampler->input = inTangentInput;
  outTangentInput->next = fixture.sampler->input;
  fixture.sampler->input = outTangentInput;
  sampled = ak_test_baked_translation(&fixture, 0.25f);
  ASSERT(isfinite(sampled) && fabsf(sampled - 0.4375f) < 0.0002f);

  /* Standard TIME/VALUE control points solve the time cubic before the
     value cubic. With x controls at 1/3 and 2/3, u=t and y(.25)=.578125. */
  inTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                          inTangentInput,
                                                          pairedIn,
                                                          AKT_FLOAT,
                                                          sizeof(float),
                                                          2u, 2u, 1u, 1u);
  outTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                           outTangentInput,
                                                           pairedOut,
                                                           AKT_FLOAT,
                                                           sizeof(float),
                                                           2u, 2u, 3u, 1u);
  ASSERT(inTangentInput->accessor && outTangentInput->accessor);
  sampled = ak_test_baked_translation(&fixture, 0.25f);
  ASSERT(isfinite(sampled) && fabsf(sampled - 0.578125f) < 0.0003f);

  /* Full-vector DAE may share one TIME followed by N VALUE components. */
  ak_test_bake_fixture_destroy(&fixture);
  ASSERT(ak_test_bake_fixture_init(&fixture,
                                   times,
                                   2u,
                                   vectorValues,
                                   2u,
                                   false));
  inTangentInput  = ak_heap_calloc(fixture.heap,
                                   fixture.sampler,
                                   sizeof(*inTangentInput));
  outTangentInput = ak_heap_calloc(fixture.heap,
                                   fixture.sampler,
                                   sizeof(*outTangentInput));
  ASSERT(inTangentInput && outTangentInput);
  inTangentInput->semantic  = AK_INPUT_IN_TANGENT;
  outTangentInput->semantic = AK_INPUT_OUT_TANGENT;
  inTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                          inTangentInput,
                                                          pairedVecIn,
                                                          AKT_FLOAT,
                                                          sizeof(float),
                                                          4u, 2u, 1u, 1u);
  outTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                           outTangentInput,
                                                           pairedVecOut,
                                                           AKT_FLOAT,
                                                           sizeof(float),
                                                           4u, 2u, 1u, 1u);
  ASSERT(inTangentInput->accessor && outTangentInput->accessor);
  fixture.sampler->inTangentInput  = inTangentInput;
  fixture.sampler->outTangentInput = outTangentInput;
  inTangentInput->next = fixture.sampler->input;
  fixture.sampler->input = inTangentInput;
  outTangentInput->next = fixture.sampler->input;
  fixture.sampler->input = outTangentInput;
  fixture.sampler->uniInterpolation = AK_INTERPOLATION_BEZIER;
  {
    float vectorSample[2] = {NAN, NAN};
    float sentinel = 42.0f;
    ASSERT(ak_animationSamplerSample(fixture.sampler,
                                     0.25f,
                                     NULL,
                                     0u) == 2u);
    ASSERT(ak_animationSamplerSample(fixture.sampler,
                                     0.25f,
                                     &sentinel,
                                     1u) == 2u);
    ASSERT(sentinel == 42.0f);
    ASSERT(ak_animationSamplerSample(fixture.sampler,
                                     0.25f,
                                     vectorSample,
                                     2u) == 2u);
    ASSERT(isfinite(vectorSample[0]) && isfinite(vectorSample[1]));
  }

  /* Morph-weight tracks may flatten N scalar OUTPUT rows per input key. */
  ak_test_bake_fixture_destroy(&fixture);
  ASSERT(ak_test_bake_fixture_init(&fixture,
                                   times,
                                   2u,
                                   values,
                                   1u,
                                   true));
  fixture.sampler->outputInput->accessor
    = ak_test_make_padded_accessor(fixture.heap,
                                   fixture.sampler->outputInput,
                                   flattenedValues,
                                   AKT_FLOAT,
                                   sizeof(float),
                                   1u, 4u, 2u, 3u);
  ASSERT(fixture.sampler->outputInput->accessor != NULL);
  fixture.sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;
  {
    float flattenedSample[2] = {NAN, NAN};
    ASSERT(ak_animationSamplerSample(fixture.sampler,
                                     0.5f,
                                     flattenedSample,
                                     2u) == 2u);
    ASSERT(fabsf(flattenedSample[0] - 1.0f) < 0.0002f);
    ASSERT(fabsf(flattenedSample[1] - 12.0f) < 0.0002f);
  }

  /* Return to scalar for Hermite and malformed-layout checks. */
  ak_test_bake_fixture_destroy(&fixture);
  ASSERT(ak_test_bake_fixture_init(&fixture,
                                   times,
                                   2u,
                                   values,
                                   1u,
                                   true));
  interpInput = ak_heap_calloc(fixture.heap,
                               fixture.sampler,
                               sizeof(*interpInput));
  inTangentInput = ak_heap_calloc(fixture.heap,
                                  fixture.sampler,
                                  sizeof(*inTangentInput));
  outTangentInput = ak_heap_calloc(fixture.heap,
                                   fixture.sampler,
                                   sizeof(*outTangentInput));
  ASSERT(interpInput && inTangentInput && outTangentInput);
  interpInput->semantic = AK_INPUT_INTERPOLATION;
  interpInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                       interpInput,
                                                       hermite,
                                                       AKT_UBYTE,
                                                       sizeof(uint8_t),
                                                       1u, 2u, 1u, 2u);
  ASSERT(interpInput->accessor != NULL);
  fixture.sampler->interpInput = interpInput;
  inTangentInput->semantic  = AK_INPUT_IN_TANGENT;
  outTangentInput->semantic = AK_INPUT_OUT_TANGENT;
  fixture.sampler->inTangentInput  = inTangentInput;
  fixture.sampler->outTangentInput = outTangentInput;

  /* Hermite uses slopes; zero slopes produce smoothstep(.25) = .15625. */
  inTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                          inTangentInput,
                                                          hermiteIn,
                                                          AKT_FLOAT,
                                                          sizeof(float),
                                                          1u, 2u, 1u, 1u);
  outTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                           outTangentInput,
                                                           hermiteOut,
                                                           AKT_FLOAT,
                                                           sizeof(float),
                                                           1u, 2u, 3u, 1u);
  ASSERT(inTangentInput->accessor && outTangentInput->accessor);
  sampled = ak_test_baked_translation(&fixture, 0.25f);
  ASSERT(isfinite(sampled) && fabsf(sampled - 0.15625f) < 0.0002f);
  sampled = NAN;
  ASSERT(ak_animationSamplerSample(fixture.sampler,
                                   0.25f,
                                   &sampled,
                                   1u) == 1u);
  ASSERT(isfinite(sampled) && fabsf(sampled - 0.15625f) < 0.0002f);

  /* COLLADA HERMITE TIME/VALUE pairs are tangent vectors. For P0=(10,4),
     P1=(12,8), and T0=T1=(1,6), parameter s=.25 gives time 10.40625 and
     value 5.1875. Treating the pair as an absolute point, or reducing the
     time curve to a linear alpha, produces a different value. */
  fixture.sampler->inputInput->accessor
    = ak_test_make_padded_accessor(fixture.heap,
                                   fixture.sampler->inputInput,
                                   (const float[2]){10.0f, 12.0f},
                                   AKT_FLOAT,
                                   sizeof(float),
                                   1u, 2u, 1u, 3u);
  fixture.sampler->outputInput->accessor
    = ak_test_make_padded_accessor(fixture.heap,
                                   fixture.sampler->outputInput,
                                   (const float[2]){4.0f, 8.0f},
                                   AKT_FLOAT,
                                   sizeof(float),
                                   1u, 2u, 2u, 5u);
  inTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                          inTangentInput,
                                                          pairedHermiteIn,
                                                          AKT_FLOAT,
                                                          sizeof(float),
                                                          2u, 2u, 1u, 1u);
  outTangentInput->accessor = ak_test_make_padded_accessor(fixture.heap,
                                                           outTangentInput,
                                                           pairedHermiteOut,
                                                           AKT_FLOAT,
                                                           sizeof(float),
                                                           2u, 2u, 3u, 1u);
  ASSERT(fixture.sampler->inputInput->accessor
         && fixture.sampler->outputInput->accessor
         && inTangentInput->accessor
         && outTangentInput->accessor);
  sampled = ak_test_baked_translation(&fixture, 10.40625f);
  ASSERT(isfinite(sampled) && fabsf(sampled - 5.1875f) < 0.0003f);

  /* Non-monotonic input and truncated buffers fail closed. */
  {
    const float badTime = -1.0f;
    memcpy((char *)fixture.sampler->inputInput->accessor->buffer->data
           + fixture.sampler->inputInput->accessor->byteOffset
           + fixture.sampler->inputInput->accessor->byteStride,
           &badTime,
           sizeof(badTime));
    ASSERT(isnan(ak_test_baked_translation(&fixture, 0.5f)));
    memcpy((char *)fixture.sampler->inputInput->accessor->buffer->data
           + fixture.sampler->inputInput->accessor->byteOffset
           + fixture.sampler->inputInput->accessor->byteStride,
           times + 1u,
           sizeof(float));
    fixture.sampler->inputInput->accessor->buffer->length = 1u;
    ASSERT(isnan(ak_test_baked_translation(&fixture, 0.5f)));
  }

  ak_test_bake_fixture_destroy(&fixture);

  /* glTF CUBICSPLINE packs [in tangent, value, out tangent] for every key. */
  ASSERT(ak_test_bake_fixture_init(&fixture,
                                   times,
                                   2u,
                                   values,
                                   1u,
                                   true));
  fixture.sampler->outputInput->accessor
    = ak_test_make_padded_accessor(fixture.heap,
                                   fixture.sampler->outputInput,
                                   packedHermite,
                                   AKT_FLOAT,
                                   sizeof(float),
                                   1u, 6u, 1u, 3u);
  ASSERT(fixture.sampler->outputInput->accessor != NULL);
  fixture.sampler->uniInterpolation = AK_INTERPOLATION_HERMITE;
  sampled = ak_test_baked_translation(&fixture, 0.25f);
  ASSERT(isfinite(sampled) && fabsf(sampled - 0.15625f) < 0.0002f);
  ak_test_bake_fixture_destroy(&fixture);
  TEST_SUCCESS
}

TEST_IMPL(animation_bake_behaviors_and_limits) {
  AkTestBakeFixture fixture;
  AkObject         *object, *last;
  const float       times[2]  = {0.0f, 1.0f};
  const float       values[2] = {0.0f, 1.0f};

  ASSERT(ak_test_bake_fixture_init(&fixture,
                                   times,
                                   2u,
                                   values,
                                   1u,
                                   true));
  fixture.sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;
  fixture.sampler->pre  = AK_SAMPLER_BEHAVIOR_CONSTANT;
  fixture.sampler->post = AK_SAMPLER_BEHAVIOR_CONSTANT;
  ASSERT(fabsf(ak_test_baked_translation(&fixture, -0.5f)) < 0.0002f);
  ASSERT(fabsf(ak_test_baked_translation(&fixture, 1.5f) - 1.0f) < 0.0002f);
  fixture.sampler->pre  = AK_SAMPLER_BEHAVIOR_GRADIENT;
  fixture.sampler->post = AK_SAMPLER_BEHAVIOR_GRADIENT;
  ASSERT(fabsf(ak_test_baked_translation(&fixture, -0.5f) + 0.5f) < 0.0002f);
  ASSERT(fabsf(ak_test_baked_translation(&fixture, 1.5f) - 1.5f) < 0.0002f);
  fixture.sampler->pre = AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE;
  ASSERT(fabsf(ak_test_baked_translation(&fixture, -0.5f) + 0.5f) < 0.0002f);
  fixture.sampler->post = AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE;
  ASSERT(fabsf(ak_test_baked_translation(&fixture, 1.5f) - 1.5f) < 0.0002f);
  ASSERT(fabsf(ak_test_baked_translation(&fixture, 2.0f) - 2.0f) < 0.0002f);
  fixture.sampler->post = AK_SAMPLER_BEHAVIOR_OSCILLATE;
  ASSERT(fabsf(ak_test_baked_translation(&fixture, 1.25f) - 0.75f) < 0.0002f);

  /* 65 authored transform objects must fail instead of silently ignoring the
     tail and producing a plausible but wrong local matrix. */
  last = fixture.node->transform->item;
  ASSERT(last != NULL);
  for (size_t i = 1u; i < 65u; i++) {
    object = ak_objAlloc(fixture.heap,
                         fixture.node->transform,
                         sizeof(AkTranslate),
                         AKT_TRANSLATE,
                         true);
    ASSERT(object != NULL);
    last->next = object;
    last = object;
  }
  ASSERT(ak_nodeBakeAnimationForAnimation(fixture.doc,
                                          fixture.node,
                                          fixture.animation) == NULL);

  ak_test_bake_fixture_destroy(&fixture);
  TEST_SUCCESS
}
