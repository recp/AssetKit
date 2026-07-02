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

TEST_IMPL(animation_time_range_walks_child_tree) {
  AkHeap      *heap;
  AkAnimation *root;
  AkAnimation *childA;
  AkAnimation *childB;
  float        start, end;
  const float  rootTimes[2]  = {1.0f, 3.0f};
  const float  childATimes[2] = {0.25f, 2.0f};
  const float  childBTimes[2] = {4.0f, 7.5f};

  heap = ak_heap_new(NULL, NULL, NULL);
  ASSERT(heap != NULL);

  root   = ak_test_make_time_animation(heap, NULL, rootTimes, 2);
  childA = ak_test_make_time_animation(heap, root, childATimes, 2);
  childB = ak_test_make_time_animation(heap, root, childBTimes, 2);
  ASSERT(root != NULL);
  ASSERT(childA != NULL);
  ASSERT(childB != NULL);

  root->animation = childA;
  childA->next    = childB;

  ASSERT(ak_animationTimeRange(root, &start, &end));
  ASSERT(start == 0.25f);
  ASSERT(end == 7.5f);

  ak_heap_destroy(heap);
  TEST_SUCCESS
}
